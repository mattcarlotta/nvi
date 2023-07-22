import assert from "node:assert";

assert.deepEqual(process.env.BASIC_ENV, "true");
assert.deepEqual(process.env.QUOTES, 'sad"wow"bak');
assert.deepEqual(process.env.MULTI_LINE_KEY, "ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com");
assert.deepEqual(process.env.MONGOLAB_URI, "mongodb://root:password@abcd1234.mongolab.com:12345/localhost");

console.log("Successfully assigned ENVs to the process!");
process.exit(0);
