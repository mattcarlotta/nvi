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
# assigning ENVs via .nvi
nvi -c mjs
# assigning ENVs via execute command
nvi -- node index.mjs
# assigning ENVs and running a script command within package.json
nvi -- pnpm run mjs

# assigning ENVs via .nvi
nvi -- ts
# assigning ENVs and running a script command within package.json
nvi -- pnpm run ts

# piping ENVs to node process.stdin, so they can be manually assigned to process.env
nvi -pr | node stdin.mjs
```
