import { spawnSync } from "node:child_process";

const python = process.platform === "win32" ? "py" : "python3";
const pythonArgs = process.platform === "win32"
  ? ["-3", "tools/mermaid_flasher.py", "self-test"]
  : ["tools/mermaid_flasher.py", "self-test"];

const steps = [
  ["node", ["scripts/check-secrets.mjs"]],
  [python, pythonArgs],
  ["node", ["--test", "test/gateway.test.mjs", "test/protocol.test.mjs"]],
  ["pio", ["run"], { cwd: "firmware/e32r28t" }],
  ["pio", ["run"], { cwd: "firmware/s3cam" }]
];

for (const [cmd, args, opts = {}] of steps) {
  const r = spawnSync(cmd, args, { stdio: "inherit", shell: false, ...opts });
  if (r.status !== 0) process.exit(r.status ?? 1);
}
