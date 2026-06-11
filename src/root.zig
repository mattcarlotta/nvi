const arg = @import("arg.zig");
const tk = @import("tokenizer.zig");
const pr = @import("parser.zig");
const em = @import("emitter.zig");
const fmt = @import("formatter.zig");

pub const Format = fmt.Format;
pub const args = arg.argParser;
pub const tokenizer = tk.parseFiles;
pub const parser = pr.parseTokens;
pub const emitter = em.emitter;

test {
    _ = arg;
}

test {
    _ = tk;
}

test {
    _ = pr;
}

test {
    _ = em;
}

test {
    _ = fmt;
}
