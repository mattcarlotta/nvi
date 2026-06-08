const arg = @import("arg.zig");
const token = @import("tokenizer.zig");

pub const args = arg.argParser;
pub const tokenizer = token.parseFiles;

test {
    _ = arg;
}

test {
    _ = tokenizer;
}
