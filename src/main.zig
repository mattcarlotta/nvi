const std = @import("std");
const Io = std.Io;
const print_error = std.log.err;
const exit = std.process.exit;

const nvi = @import("nvi");
const arg = @import("arg.zig");

pub fn main(init: std.process.Init) !void {
    const arena: std.mem.Allocator = init.arena.allocator();
    const argv = init.minimal.args.toSlice(arena) catch {
        print_error("failed to read arguments (out of memory)", .{});
        exit(2);
    };

    const args = arg.argParser(arena, argv) catch |err| {
        print_error("argument error: {s}", .{@errorName(err)});
        exit(2);
    };

    if (args.err) |err| {
        err.print();
        exit(2);
    }

    // // In order to do I/O operations need an `Io` instance.
    // const io = init.io;

    // // Stdout is for the actual output of your application, for example if you
    // // are implementing gzip, then only the compressed bytes should be sent to
    // // stdout, not any debugging messages.
    // var stdout_buffer: [1024]u8 = undefined;
    // var stdout_file_writer: Io.File.Writer = .init(.stdout(), io, &stdout_buffer);
    // const stdout_writer = &stdout_file_writer.interface;

    // try nvi.printAnotherMessage(stdout_writer);

    // try stdout_writer.flush(); // Don't forget to flush!
}

test "simple test" {
    const gpa = std.testing.allocator;
    var list: std.ArrayList(i32) = .empty;
    defer list.deinit(gpa); // Try commenting this out and see if zig detects the memory leak!
    try list.append(gpa, 42);
    try std.testing.expectEqual(@as(i32, 42), list.pop());
}

// test "fuzz example" {
//     try std.testing.fuzz({}, testOne, .{});
// }

// fn testOne(context: void, smith: *std.testing.Smith) !void {
//     _ = context;
//     // Try passing `--fuzz` to `zig build test` and see if it manages to fail this test case!

//     const gpa = std.testing.allocator;
//     var list: std.ArrayList(u8) = .empty;
//     defer list.deinit(gpa);
//     while (!smith.eos()) switch (smith.value(enum { add_data, dup_data })) {
//         .add_data => {
//             const slice = try list.addManyAsSlice(gpa, smith.value(u4));
//             smith.bytes(slice);
//         },
//         .dup_data => {
//             if (list.items.len == 0) continue;
//             if (list.items.len > std.math.maxInt(u32)) return error.SkipZigTest;
//             const len = smith.valueRangeAtMost(u32, 1, @min(32, list.items.len));
//             const off = smith.valueRangeAtMost(u32, 0, @intCast(list.items.len - len));
//             try list.appendSlice(gpa, list.items[off..][0..len]);
//             try std.testing.expectEqualSlices(
//                 u8,
//                 list.items[off..][0..len],
//                 list.items[list.items.len - len ..],
//             );
//         },
//     };
// }
