# Rust Example

Requirements:
- rustup
- cargo

Example usage:
```DOSINI
# assigning ENVs via .nvi
nvi -c build
# assigning ENVs via execute command
nvi -- cargo build --release

# assigning ENVs via .nvi
nvi -c debug
# assigning ENVs via execute command
nvi -- cargo run

# assigning ENVs via .nvi
nvi -c release
# assigning ENVs via execute command
nvi -- cargo run --release
```
