const arg = @import("arg.zig");
const tk = @import("tokenizer.zig");
const pr = @import("parser.zig");
const em = @import("emitter.zig");
const fmt = @import("formatter.zig");
const sc = @import("scanner.zig");

pub const Format = fmt.Format;
pub const args = arg.argParser;
pub const tokenizer = tk.parseFiles;
pub const parser = pr.parseTokens;
pub const emitter = em.emitter;
pub const scanner = sc.scanFiles;

test {
    _ = @import("arg_test.zig");
    _ = @import("emitter_test.zig");
    _ = @import("formatter_test.zig");
    _ = @import("parser_test.zig");
    _ = @import("scanner_test.zig");
    _ = @import("tokenizer_test.zig");
    _ = @import("utils_test.zig");
}
