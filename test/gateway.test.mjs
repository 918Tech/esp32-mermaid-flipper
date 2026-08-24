import test from "node:test";
import assert from "node:assert/strict";
import { once } from "node:events";
import { startServer } from "../gateway/server.mjs";

async function withServer(fetchImpl, fn) {
  const server = startServer(0, fetchImpl);
  await once(server, "listening");
  try {
    const port = server.address().port;
    return await fn(port);
  } finally {
    server.close();
  }
}

test("health", async () => {
  await withServer(fetch, async (port) => {
    const r = await fetch(`http://127.0.0.1:${port}/health`);
    assert.equal(r.status, 200);
  });
});

test("index page", async () => {
  await withServer(fetch, async (port) => {
    const r = await fetch(`http://127.0.0.1:${port}/`);
    assert.equal(r.status, 200);
    assert.match(await r.text(), /TECH\/\/ONE/);
  });
});

test("invalid json", async () => {
  await withServer(fetch, async (port) => {
    const r = await fetch(`http://127.0.0.1:${port}/v1/propose`, { method: "POST", body: "{" });
    assert.equal(r.status, 400);
  });
});

test("successful proposal", async () => {
  const mock = async () => new Response(JSON.stringify({ output_text: JSON.stringify({ summary: "ok", risk: "read_only", target: "none", command: "status", requires_approval: true }) }), { status: 200 });
  await withServer(mock, async (port) => {
    const r = await fetch(`http://127.0.0.1:${port}/v1/propose`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ prompt: "hello", image_base64: Buffer.from("x").toString("base64") })
    });
    assert.equal(r.status, 200);
  });
});

test("camera proxy missing config", async () => {
  await withServer(fetch, async (port) => {
    const r = await fetch(`http://127.0.0.1:${port}/camera.jpg`);
    assert.equal(r.status, 503);
  });
});
