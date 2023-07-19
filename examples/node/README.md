# Node Example

Example usage:
```DOSINI
# assigning ENVs via env.config.json
nvi -c mjs
# assigning ENVs via execute command
nvi -e node index.mjs
# assigning ENVs via command within package.json
nvi -e npm run mjs

# assigning ENVs via env.config.json
nvi -c ts
# assigning ENVs via command within package.json
nvi -e npm run ts

# piping ENVs to node process.stdin, so they can be assigned manually
nvi | node stdin.mjs
```
