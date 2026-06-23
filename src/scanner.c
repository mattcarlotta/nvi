#include "scanner.h"
#include "accessors.h"
#include "arg.h"
#include "errors.h"
#include "list.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const char *blacklist[] = {
    "node_modules",  "bower_components", "storybook-static", "dist",    "build",         "out",
    "coverage",      "target",           "vendor",           "zig-out", "__pycache__",   "venv",
    "site-packages", "htmlcov",          "_build",           "deps",    "dist-newstyle", "nimcache",
    "Pods",          "Carthage",         "DerivedData",      "obj",
};
static const size_t blacklist_count = sizeof(blacklist) / sizeof(blacklist[0]);

bool is_blacklisted(const char *name) {
    for (size_t i = 0; i < blacklist_count; i++) {
        if (strcmp(name, blacklist[i]) == 0) {
            return true;
        }
    }

    return false;
}

static const ext_pair *ext_map_get(const ext_map *m, const char *ext) {
    for (size_t i = 0; i < m->count; i++) {
        if (strcmp(m->items[i].ext, ext) == 0) {
            return &m->items[i];
        }
    }

    return NULL;
}

static void ext_map_put(ext_map *map, const ext_entry *entry) {
    if (ext_map_get(map, entry->ext) != NULL) {
        return;
    }

    if (map->count == map->capacity) {
        size_t new_cap = map->capacity == 0 ? DA_INIT_CAP : map->capacity * 2;
        ext_pair *pair = realloc(map->items, new_cap * sizeof(*map->items));
        if (pair == NULL) {
            abort();
        }
        map->items = pair;
        map->capacity = new_cap;
    }

    map->items[map->count].ext = entry->ext;
    map->items[map->count].accessors = entry->accessors;
    map->items[map->count].accessor_count = entry->count;
    map->count++;
}

result_t run_scanner(args_t *args, scanner_t *scanner) {
    scanner->dry_run = scanner->dry_run || args->command.count > 0;

    if (args->dry_run) {
        fprintf(stderr, "[INFO] Scanning for environment variables in ");

        for (size_t i = 0; i < args->scan_exts.count; ++i) {
            if (i != 0) {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "*.%s", args->scan_exts.items[i]);
        }

        fprintf(stderr, " files...\n\n");
    }

    for (size_t i = 0; i < args->scan_exts.count; ++i) {
        const char *ext = args->scan_exts.items[i];
        const ext_entry *entry = find_ext(ext);

        if (entry == NULL) {
            return usage_error("unsupported scan file extension '%s'", ext);
        }

        ext_map_put(&scanner->scan_exts, entry);
    }

    // var root = try Io.Dir.cwd().openDir(self.io, ".", .{.iterate = true});
    // defer root.close(self.io);

    // try self.walkFileTree(root, "");

    // if (self.dry_run) {
    //     try self.logger.print(
    //         tty.cyan++ "info: " ++tty.reset++ "Scanned {d} file(s) and found {d} reference(s) to {d} unique
    //         key(s)\n\n",
    //         .{self.files_scanned, self.references, self.envs.count()}, );
    // }

    // try self.mergeRequiredEnvs();

    // if (self.args.command.len == 0) {
    //     return error.NoCommand;
    // }
    return (result_t){.ok = true};
}

// fn walkFileTree(self : *Self, dir : Io.Dir, prefix : [] const u8) !void {
//     var it = dir.iterate();
//     while (try it.next(self.io)) {
//         | entry | {
//             switch (entry.kind) {
//                 .directory => {
//                     if (mem.startsWith(u8, entry.name, ".") or blacklist.has(entry.name)) {
//                         continue;
//                     }

//                     var child = try dir.openDir(self.io, entry.name, .{.iterate = true});
//                     defer child.close(self.io);

//                     const child_prefix = try path.join(self.alloc, &.{prefix, entry.name});
//                     try self.walkFileTree(child, child_prefix);
//                 }
//                 , .file = > try self.scanFile(dir, prefix, entry.name), else =>{},
//             }
//         }
//     }
// }

// fn scanFile(self : *Self, dir : Io.Dir, prefix : [] const u8, name : [] const u8) !void {
//     const accessors = self.getAccessors(name) orelse return;

//     const contents = dir.readFileAlloc(self.io, name, self.alloc, .limited(10 * 1024 * 1024)) catch {
//         try self.logger.print(tty.yellow++ "warning: " ++tty.reset++ "The " ++tty.bold_yellow++ "{s}" ++tty
//                                   .reset++ " file exceeds 10MB, skipping.\n",
//                               .{name}, );
//         return;
//     };

//     self.files_scanned += 1;

//     var matches : std.ArrayList(Match) =.empty;
//     defer matches.deinit(self.alloc);

//     try self.scanContents(contents, accessors, &matches);

//     if (matches.items.len == 0) {
//         return;
//     }

//     if (self.dry_run) {
//         try self.printMatches(prefix, name, matches.items);
//     }

//     for (matches.items) {
//         | m | {
//             self.references += 1;

//             const env = try self.envs.getOrPut(self.alloc, m.key);
//             if (!env.found_existing) {
//                 env.key_ptr.* = try self.alloc.dupe(u8, m.key);
//                 env.value_ptr.* = 0;
//             }

//             env.value_ptr.* += 1;
//         }
//     }
// }

// fn getAccessors(self : *Self, name : [] const u8) ? [] const ac.Accessor {
//     const dot_ext = path.extension(name);

//     if (dot_ext.len < 2) {
//         return null;
//     }

//     return self.exts.get(dot_ext[1..]);
// }

// pub fn scanContents(self : *Self, contents : [] const u8, accessors : [] const ac.Accessor,
//                     matches : *std.ArrayList(Match), ) !void {
//     var i : usize = 0;
//     var line : usize = 1;
//     var line_start : usize = 0;

//     while (i < contents.len) {
//         if (contents[i] == char.LINE_DELIMITER) {
//             line += 1;
//             line_start = i + 1;
//             i += 1;
//             continue;
//         }

//         const acc = self.matchPrefix(contents, i, accessors) orelse {
//             i += 1;
//             continue;
//         };

//         if (extractKey(contents, i + acc.prefix.len, acc.pattern)) {
//             | env | {
//                 try matches.append(self.alloc, .{
//                                                    .key = env.key,
//                                                    .line = line,
//                                                    .byte = env.start - line_start + 1,
//                                                });

//                 i = env.end;
//                 continue;
//             }
//         }

//         i += acc.prefix.len;
//     }
// }

// fn matchPrefix(_ : *Self, contents : [] const u8, i : usize, accessors : [] const ac.Accessor) ? ac.Accessor {
//     for (accessors) {
//         | acc | {
//             if (!mem.startsWith(u8, contents[i..], acc.prefix)) {
//                 continue;
//             }

//             if (i > 0 and utils.isIdentChar(contents[i - 1]) and utils.isIdentChar(acc.prefix[0])) {
//                 continue;
//             }

//             return acc;
//         }
//     }

//     return null;
// }

// fn isIgnoredKey(self : *Self, key : [] const u8) bool {
//     for (self.args.ignored.items) {
//         | ig | {
//             if (mem.eql(u8, ig, key)) {
//                 return true;
//             }
//         }
//     }

//     return false;
// }

// pub fn mergeRequiredEnvs(self : *Self) !void {
//     if (self.envs.count() == 0) {
//         return;
//     }

//     if (self.dry_run) {
//         try self.logger.writeAll(
//             tty.cyan++ "info: " ++tty.reset++ "The following ENV keys will be marked as required... \n");
//     }

//     for (self.envs.keys()) {
//         | key | {
//             if (self.isIgnoredKey(key)) {
//                 continue;
//             }

//             try self.args.required.append(self.alloc, key);

//             if (self.dry_run) {
//                 try self.logger.print("    •" ++tty.bold_green++ " {s}" ++tty.reset++ "\n", .{key});
//             }
//         }
//     }

//     if (self.dry_run) {
//         try self.logger.writeByte('\n');
//     }
// }

// fn printMatches(self : *Self, prefix : [] const u8, name : [] const u8, matches : [] const Match) !void {
//     const file = if (prefix.len == 0) name else try path.join(self.alloc, &.{prefix, name});

//     try self.logger.print(
//         tty.cyan++ "info: " ++tty.reset++ "Scanned " ++tty.green++ "{s}" ++tty.reset++ " and found {d} key(s)...\n",
//         .{file, matches.len}, );

//     for (matches) {
//         | m | {
//             try self.logger.print("    • " ++tty.bold_green++ "{s}" ++tty.reset++ " (line {d}, byte {d})\n",
//                                   .{m.key, m.line, m.byte}, );
//         }
//     }

//     try self.logger.writeByte('\n');
// }
// }
// ;

// fn extractKey(contents : [] const u8, start : usize, kind : ac.Pattern) ? Env {
//     switch (kind) {
//         .ident => {
//             var end = start;

//             while (end < contents.len and utils.isIdentChar(contents[end])) {
//                 end += 1;
//             }

//             if (end == start) {
//                 return null;
//             }

//             return.{.key = contents[start..end], .start = start, .end = end};
//         }
//         , .quoted => {
//             var cursor = start;

//             while (cursor < contents.len and(contents[cursor] == char.SPACE or contents[cursor] == char.TAB)) {
//                 cursor += 1;
//             }

//             if (cursor >= contents.len) {
//                 return null;
//             }

//             const quote = contents[cursor];
//             if (quote != char.DOUBLE_QUOTE and quote != char.SINGLE_QUOTE) {
//                 return null;
//             }

//             const key_start = cursor + 1;
//             const key_end = mem.indexOfScalarPos(u8, contents, key_start, quote) orelse return null;

//             if (!utils.sameLineOnly(contents, key_start, key_end)) {
//                 return null;
//             }

//             // empty quotes
//             if (key_end == key_start) {
//                 return null;
//             }

//             return.{.key = contents[key_start..key_end], .start = key_start, .end = key_end + 1};
//         }
//         , .braced => {
//             const brace = mem.indexOfScalarPos(u8, contents, start, char.CLOSE_BRACE) orelse return null;
//             if (!utils.sameLineOnly(contents, start, brace)) {
//                 return null;
//             }

//             var key_start = start;
//             var key_end = brace;
//             // strip a matching pair of surrounding quotes: $ENV{'FOO'} -> FOO
//             if (key_end > key_start and(contents[key_start] == char.SINGLE_QUOTE or contents[key_start] ==
//                                         char.DOUBLE_QUOTE) and contents[key_end - 1] ==
//                 contents[key_start]) {
//                 key_start += 1;
//                 key_end -= 1;
//             }

//             if (key_end <= key_start) {
//                 return null;
//             }

//             return.{.key = contents[key_start..key_end], .start = key_start, .end = brace + 1};
//         }
//         , .parened => {
//             const paren = mem.indexOfScalarPos(u8, contents, start, char.CLOSE_PAREN) orelse return null;

//             if (!utils.sameLineOnly(contents, start, paren)) {
//                 return null;
//             }

//             if (paren == start) {
//                 return null;
//             }

//             return.{.key = contents[start..paren], .start = start, .end = paren + 1};
//         }
//         ,
//     }
// }

void scanner_free(scanner_t *scanner) {
    free(scanner->scan_exts.items);
    scanner->scan_exts.items = NULL;
    scanner->scan_exts.count = 0;
    scanner->scan_exts.capacity = 0;

    list_free(&scanner->envs);
}
