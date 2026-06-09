const arg = @import("arg.zig");
const tk = @import("tokenizer.zig");
const pr = @import("parser.zig");

pub const args = arg.argParser;
pub const tokenizer = tk.parseFiles;
pub const parse = pr.parseTokens;

test {
    _ = arg;
}

test {
    _ = tk;
}

test {
    _ = pr;
}
