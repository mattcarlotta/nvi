# Node Example

Requirements:
- node
- npm

Installation:
```DOSINI
npm i
```

Example usage:
```DOSINI
# assigning ENVs via env.config.json
nvi -c mjs
# assigning ENVs via execute command
nvi -e node index.mjs
# assigning ENVs and running a script command within package.json
nvi -e npm run mjs

# assigning ENVs via env.config.json
nvi -c ts
# assigning ENVs and running a script command within package.json
nvi -e npm run ts

# piping ENVs to node process.stdin, so they can be manually assigned to process.env
nvi | node stdin.mjs
```
