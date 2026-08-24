import { readFileSync, readdirSync, statSync } from "node:fs";
import { join } from "node:path";

const root = new URL("..", import.meta.url).pathname.replace(/^\//, "");
const skip = new Set([".git", ".pio", "node_modules"]);
for (const dir of walk(root)) {
  if (dir.endsWith("scripts/check-secrets.mjs")) continue;
  const txt = readFileSync(dir, "utf8");
  if (/\bsk-[A-Za-z0-9]{20,}\b/.test(txt)) throw new Error(`secret-like content in ${dir}`);
}
function* walk(dir) {
  for (const name of readdirSync(dir)) {
    if (skip.has(name)) continue;
    const p = join(dir, name);
    const st = statSync(p);
    if (st.isDirectory()) yield* walk(p);
    else if (/\.(mjs|js|md|json|ini|cpp|h|txt|example)$/.test(name)) yield p;
  }
}
