import test from "node:test";
import assert from "node:assert/strict";
import { crc32String } from "../shared/crc32.mjs";

test("crc32 stable", () => {
  assert.equal(crc32String("abc"), 0x352441c2);
});
