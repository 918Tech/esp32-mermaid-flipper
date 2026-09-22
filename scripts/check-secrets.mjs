import { readFileSync, readdirSync, statSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const skip = new Set([".git", ".pio", ".ufbt", ".venv-mermaid", "dist", "node_modules"]);

for (const file of walk(root)) {
  if (file.endsWith("scripts/check-secrets.mjs")) continue;
  const txt = readFileSync(file, "utf8");
  if (/\bsk-[A-Za-z0-9]{20,}\b/.test(txt)) {
    throw new Error(`secret-like content in ${file}`);
  }
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
