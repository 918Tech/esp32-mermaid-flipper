import http from "node:http";
import { randomUUID } from "node:crypto";
import { createReadStream, readFileSync } from "node:fs";
import { extname, join } from "node:path";
import { fileURLToPath } from "node:url";
const schema = JSON.parse(readFileSync(new URL("../shared/schema.json", import.meta.url), "utf8"));
const publicDir = fileURLToPath(new URL("./public", import.meta.url));

const MAX_BODY = 1024 * 1024;
const MAX_PROMPT = 512;
const MAX_IMAGE_B64 = 900000;
const RATE_WINDOW_MS = 60_000;
const RATE_MAX = 20;
const hits = new Map();

function json(res, code, obj) {
  const body = JSON.stringify(obj);
  res.writeHead(code, { "content-type": "application/json; charset=utf-8" });
  res.end(body);
}

function sendFile(res, filePath) {
  const ext = extname(filePath);
  const types = new Map([
    [".html", "text/html; charset=utf-8"],
    [".js", "application/javascript; charset=utf-8"],
    [".css", "text/css; charset=utf-8"]
  ]);
  res.writeHead(200, { "content-type": types.get(ext) || "application/octet-stream" });
  createReadStream(filePath).on("error", () => {
    res.writeHead(404, { "content-type": "application/json; charset=utf-8" });
    res.end(JSON.stringify({ error: "not_found" }));
  }).pipe(res);
}

function validateProposal(obj) {
  if (!obj || typeof obj !== "object" || Array.isArray(obj)) return false;
  const keys = ["summary", "risk", "target", "command", "requires_approval"];
  if (!keys.every((k) => k in obj)) return false;
  if (typeof obj.summary !== "string" || typeof obj.command !== "string" || typeof obj.requires_approval !== "boolean") return false;
  if (!["read_only", "state_change", "radio_transmit", "firmware_change"].includes(obj.risk)) return false;
  if (!["none", "flipper", "marauder"].includes(obj.target)) return false;
  return true;
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    let size = 0; const chunks = [];
    req.on("data", (c) => { size += c.length; if (size > MAX_BODY) reject(new Error("body")); else chunks.push(c); });
    req.on("end", () => resolve(Buffer.concat(chunks).toString("utf8")));
    req.on("error", reject);
  });
}

function allow(req) {
  const token = process.env.CODEX_DEVICE_TOKEN;
  if (!token) return true;
  return req.headers.authorization === `Bearer ${token}`;
}

function rateOk(ip) {
  const now = Date.now();
  const cur = hits.get(ip) || [];
  const fresh = cur.filter((t) => now - t < RATE_WINDOW_MS);
  if (fresh.length >= RATE_MAX) return false;
  fresh.push(now);
  hits.set(ip, fresh);
  return true;
}

async function proxyCamera(req, res, fetchImpl) {
  const cameraUrl = process.env.S3_CAMERA_URL;
  if (!cameraUrl) return json(res, 503, { error: "camera_unavailable" });
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 5000);
  try {
    const resp = await fetchImpl(cameraUrl, { signal: controller.signal, headers: { accept: "image/jpeg" } });
    if (!resp.ok) return json(res, 502, { error: "camera_fetch_failed" });
    const contentType = resp.headers.get("content-type") || "image/jpeg";
    if (!contentType.startsWith("image/jpeg")) return json(res, 502, { error: "invalid_camera_type" });
    const body = Buffer.from(await resp.arrayBuffer());
    if (!body.length || body.length > 2_000_000) return json(res, 502, { error: "invalid_camera_frame" });
    res.writeHead(200, {
      "content-type": "image/jpeg",
      "cache-control": "no-store, max-age=0"
    });
    res.end(body);
  } catch {
    json(res, 504, { error: "camera_timeout" });
  } finally {
    clearTimeout(timeout);
  }
}

async function boardStatus(req, res, fetchImpl) {
  const payload = {
    gateway: "online",
    camera: process.env.S3_CAMERA_URL ? "configured" : "missing",
    touchscreen: "browser-approval",
    flipper: "optional"
  };
  if (!process.env.S3_HEALTH_URL) return json(res, 200, payload);
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 3000);
  try {
    const resp = await fetchImpl(process.env.S3_HEALTH_URL, { signal: controller.signal });
    if (resp.ok) {
      payload.camera = "online";
      const data = await resp.json().catch(() => null);
      payload.camera_health = data?.camera || "unknown";
      payload.wifi = data?.wifi || "unknown";
    } else {
      payload.camera = "degraded";
    }
  } catch {
    payload.camera = "offline";
  } finally {
    clearTimeout(timeout);
  }
  return json(res, 200, payload);
}

export async function proposeHandler(req, res, fetchImpl = fetch) {
  if (req.method !== "POST") return json(res, 405, { error: "method_not_allowed" });
  if (!allow(req)) return json(res, 401, { error: "unauthorized" });
  const ip = req.socket.remoteAddress || "unknown";
  if (!rateOk(ip)) return json(res, 429, { error: "rate_limited" });
  let raw;
  try { raw = await readBody(req); } catch { return json(res, 413, { error: "body_too_large" }); }
  let body;
  try { body = JSON.parse(raw); } catch { return json(res, 400, { error: "invalid_json" }); }
  const { prompt, image_base64, image_mime = "image/jpeg" } = body || {};
  if (typeof prompt !== "string" || Buffer.byteLength(prompt) > MAX_PROMPT) return json(res, 400, { error: "invalid_prompt" });
  if (typeof image_base64 !== "string" || image_base64.length > MAX_IMAGE_B64) return json(res, 400, { error: "invalid_image" });
  if (image_mime !== "image/jpeg") return json(res, 400, { error: "invalid_image_mime" });
  let imageBytes;
  try { imageBytes = Buffer.from(image_base64, "base64"); } catch { return json(res, 400, { error: "invalid_base64" }); }
  if (!imageBytes.length) return json(res, 400, { error: "invalid_base64" });
  const model = process.env.OPENAI_MODEL || "gpt-4.1-mini";
  const controller = new AbortController(); const timeout = setTimeout(() => controller.abort(), 30000);
  try {
    const resp = await fetchImpl("https://api.openai.com/v1/responses", {
      method: "POST",
      signal: controller.signal,
      headers: { authorization: `Bearer ${process.env.OPENAI_API_KEY}`, "content-type": "application/json" },
      body: JSON.stringify({
        model,
        store: false,
        input: [{ role: "user", content: [{ type: "input_text", text: prompt }, { type: "input_image", image_url: `data:${image_mime};base64,${image_base64}` }] }],
        text: { format: { type: "json_schema", name: "proposal", schema, strict: true } }
      })
    });
    const data = await resp.json().catch(() => null);
    if (!resp.ok) return json(res, 502, { error: "openai_error" });
    const txt = data?.output_text || data?.output?.[0]?.content?.[0]?.text;
    if (typeof txt !== "string") return json(res, 502, { error: "missing_output" });
    const proposal = JSON.parse(txt);
    if (!validateProposal(proposal)) return json(res, 502, { error: "schema_rejected" });
    return json(res, 200, { request_id: randomUUID(), proposal });
  } catch {
    return json(res, 504, { error: "timeout_or_network" });
  } finally { clearTimeout(timeout); }
}

export function createServer(fetchImpl = fetch) {
  return http.createServer((req, res) => {
    if (req.url === "/" || req.url === "/index.html") return sendFile(res, join(publicDir, "index.html"));
    if (req.url === "/health") return json(res, 200, { ok: true });
    if (req.url === "/status") return boardStatus(req, res, fetchImpl);
    if (req.url === "/camera.jpg") return proxyCamera(req, res, fetchImpl);
    if (req.url === "/v1/propose") return proposeHandler(req, res, fetchImpl);
    return json(res, 404, { error: "not_found" });
  });
}

export function startServer(port = 8787, fetchImpl = fetch) {
  return createServer(fetchImpl).listen(port);
}

if (import.meta.url === `file://${process.argv[1]}`) startServer(Number(process.env.GATEWAY_PORT || 8787));
