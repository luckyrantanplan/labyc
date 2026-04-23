import assert from "node:assert/strict";
import test from "node:test";
import { hashString, stableStringify } from "../src/hash.js";

void test("stableStringify sorts nested object keys consistently", () => {
  const left = stableStringify({ b: 1, a: { d: 2, c: 3 } });
  const right = stableStringify({ a: { c: 3, d: 2 }, b: 1 });

  assert.equal(left, right);
});

void test("hashString returns stable hashes", () => {
  assert.equal(hashString("abc"), hashString("abc"));
  assert.notEqual(hashString("abc"), hashString("abd"));
});

void test("stableStringify preserves array order while normalizing nested objects", () => {
  const value = stableStringify([{ b: 1, a: 2 }, null, "x"]);

  assert.equal(value, '[{"a":2,"b":1},null,"x"]');
});
