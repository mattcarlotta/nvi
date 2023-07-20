# Node Example

Requirements:
- node
- pnpm (or change to npm)

Installation:
```DOSINI
pnpm i
```

Example usage:
```DOSINI
# assigning ENVs via env.config.json
nvi -c mjs
# assigning ENVs via execute command
nvi -e node index.mjs
# assigning ENVs and running a script command within package.json
nvi -e pnpm run mjs

# assigning ENVs via env.config.json
nvi -c ts
# assigning ENVs and running a script command within package.json
nvi -e pnpm run ts

# piping ENVs to node process.stdin, so they can be manually assigned to process.env
nvi | node stdin.mjs
```
