# Rust Example

Requirements:
- rustup
- cargo

Example usage:
```DOSINI
# assigning ENVs via nvi.toml
nvi --config build
# assigning ENVs via execute command
nvi -- cargo build --release

# assigning ENVs via nvi.toml
nvi --config debug
# assigning ENVs via execute command
nvi -- cargo run

# assigning ENVs via nvi.toml
nvi --config release
# assigning ENVs via execute command
nvi -- cargo run --release
```
