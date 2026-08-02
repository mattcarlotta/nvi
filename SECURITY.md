# Security policy

Security fixes will land in a new tag release from `main` rather than being backported, so only the [latest release](https://github.com/mattcarlotta/nvi/releases/latest) is supported.

If you've installed a precompiled binary or used the install script, `nvi version` reports the version, build type, commit, compiler, and architecture. Include that output in any report.

## Reporting a vulnerability

**Please do not open a public issue, discussion, or pull request for a security vulnerability.**

Report privately through GitHub by visiting [nvi security](https://github.com/mattcarlotta/nvi/security) and clicking **Report a vulnerability**.
See [GitHub's guide](https://docs.github.com/en/code-security/how-tos/report-and-fix-vulnerabilities/report-privately) for the full flow.

### What to include

A report is actionable much faster with:

- The output of `nvi version`
- Operating system, architecture, and libc (glibc, musl, MSVC)
- A minimal reproducer: the `.env` file, `.nvi` config, and exact argv that trigger the issue
- What you expected to happen and what actually happened
- The impact you believe it has (memory corruption, secret disclosure, command injection into the consumer, denial of service)
- Any crash artifact from the fuzzer (`crash-*`, `timeout-*`, `oom-*`, `fuzz-stall-*.bin`) if you found it that way

Proof-of-concept `.env` fixtures are welcome. Please use fake secrets.

### What to expect

- Acknowledgement within 3 business days
- An assessment of severity and scope, and a decision on whether to accept the report, within 7 days of acknowledgement
- Collaboration on a fix in a temporary private fork where useful
- Public disclosure through a GitHub Security Advisory once a patched release is available, or after 90 days, whichever comes first
- Credit in the advisory unless you'd rather stay anonymous

## Scope

In scope:

- Memory leaks and unhandled segfaults
- Any input that escapes the documented limits and exhausts memory or hangs the process (see [security model](#security-model))
- Emitted output that breaks out of its intended context
- Values or keys from an `.env` file leaking into a dry-run that should have been masked by default
- Path traversal or unintended file reads during a `--scan` walk

Out of scope:

- Behavior of the consumer
- Secrets stored in your own `.env` files or committing those files to version control
- Anything requiring an attacker who already controls your shell profile, your `$PATH`, or the binary itself
- Missing hardening flags without a demonstrated exploit path
- Denial of service that requires an input larger than the documented file size limits

## Security model

- Doesn't perform file execution operations (like [exec](https://man7.org/linux/man-pages/man3/exec.3p.html)), nor process spawning nor shell invocation
- Doesn't use any [regular expressions](https://man7.org/linux/man-pages/man3/regcomp.3.html)!
- Only parses the `.env` files you provide and writes ENVs to stdout
- Limits parsed and scanned files to 10MB, a single interpolated value to 1MB, and the total parsed ENV output to 8MB, so a malicious or corrupted `.env` file errors instead of exhausting memory (consumer handles `ARG_MAX`)
- Process execution happens entirely in the consumer you choose ([env](https://man7.org/linux/man-pages/man1/env.1.html) or PowerShell) with the command tokens you've typed
- For PowerShell, values are emitted inside single-quoted strings (the only escape being `''`), so values cannot break out of string context into executable position

## Hardening and testing

The four untrusted-input surfaces are continuously fuzzed with [libFuzzer](https://llvm.org/docs/LibFuzzer.html) under AddressSanitizer and UndefinedBehaviorSanitizer. Each target keeps a cumulative corpus, and a watchdog turns hangs into reproducible artifacts. See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md#fuzzing) for the harnesses and how to run them.

If you'd like to fuzz nvi yourself, that's encouraged, and the corpus and dictionaries are in the repository.
