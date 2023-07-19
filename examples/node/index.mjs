import assert from "node:assert";

assert.deepEqual(process.env.BASIC_ENV, "true");
assert.deepEqual(process.env.QUOTES, 'sad"wow"bak');
assert.deepEqual(process.env.INTERP, "aatruecdhelloef")

console.log("Successfully assigned ENVs to process.env!");
process.exit(0);
