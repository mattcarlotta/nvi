# Rust Example

Requirements:
- rustup
- cargo

Example usage:
```DOSINI
# assigning ENVs via nvi.json
nvi -c build
# assigning ENVs via execute command
nvi -e cargo build --release

# assigning ENVs via nvi.json
nvi -c debug
# assigning ENVs via execute command
nvi -e cargo run

# assigning ENVs via nvi.json
nvi -c release
# assigning ENVs via execute command
nvi -e cargo run --release
```
