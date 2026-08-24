import { crc32String } from "./crc32.mjs";

export const PROTOCOL_VERSION = 1;
export const MAX_LINE_BYTES = 4096;
export const MAX_COMMAND_BYTES = 160;
export const MAX_SUMMARY_BYTES = 240;
export const MAX_PROMPT_BYTES = 512;

export function canonicalJson(value) {
  if (value === null || typeof value !== "object" || Array.isArray(value)) return JSON.stringify(value);
  const keys = Object.keys(value).filter((k) => k !== "crc32").sort();
  const parts = keys.map((k) => `${JSON.stringify(k)}:${canonicalJson(value[k])}`);
  return `{${parts.join(",")}}`;
}

export function withCrc(message) {
  const body = { ...message };
  const crcInput = canonicalJson(body);
  return { ...body, crc32: crc32String(crcInput) };
}
