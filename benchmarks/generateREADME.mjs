import { readFileSync, writeFileSync } from "fs";
import { join } from "path";

const iterations = [500000, 5000, 500000];

const head = compiled => `# Env Metrics

## Commands

Run individual tests by running the following commands:

| \`yarn <command>\` | Description                                                                                     |
| ---------------- | ----------------------------------------------------------------------------------------------- |
| \`dotenv\`         | Runs tests for \`dotenv\` + \`dotenv-expand\` (outputs results to \`results.json\` file as \`dotenv\`). |
| \`nvi\`            | Runs tests for \`nvi\` (outputs results to \`results.json\` file as \`nvi\`).                       |
| \`generate:readme\`| Generates a \`README.md\` from the \`results.json\` file.                                           |

⚠️ Warning: The tests can take a quite a long time to complete! Adjust the iterations or runs if needed.


## Metrics

**System Specs**:

- CPU: AMD Ryzen 9 5950x (stock)
- MOBO: Asus x570 ROG Crosshair Dark Hero
- RAM: G.Skill Trident Z Neo 32 GB (4 x 8 GB) running @ 2100Mhz
- Storage: Sabrent 1TB Rocket 4 Plus NVMe 4.0 Gen4
- OS: Linuxmint 21.1 vera
- Kernel: Linux 5.15.0-73-generic x86_64
- Node: v18.16.0

**Compiled Timestamp**: ${compiled}

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
`;

const singleLargeEnv = `
Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
`;

const multiEnvs = `
Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
`;

const runs = ["single", "interpolated", "multiple"];
const divider = "|";

const addRowItem = item => `| ${item} `;
const readResultFile = () => {
    try {
        const file = readFileSync(join(process.cwd(), "result.json"), {
            encoding: "utf-8"
        });

        return JSON.parse(file);
    } catch (error) {
        return null;
    }
};

const date = new Intl.DateTimeFormat(
    'en-US',
    {
        dateStyle: 'full',
        timeStyle: 'medium',
        timeZone: 'America/Los_Angeles'
    }
).format(new Date());


(() => {
    try {
        const results = readResultFile();
        if (!results) throw String("You must run the test suites first!");

        let file = head(date);

        const addResultsToTable = (pkg, cfg, idx) => {
            const result = results?.[pkg];
            const config = result?.[cfg];
            if (!result || !config)
                throw String(
                    `Unable to locate a '${pkg}' or '${cfg}' property within the results!`
                );

            const [fastestPackage] = Object.keys(results)
                .map(pckg => results[pckg][cfg].fastest[0])
                .sort((a, b) => a - b);

            const pckg = addRowItem(pkg);
            const runDate = addRowItem(config.date);
            const numberOfIterations = addRowItem(iterations[idx]);
            const fastestIterations = addRowItem(
                config.fastest.map(i => `${i}s`).join(", ")
            );
            const avgIteration = addRowItem(`${config.avg}s`);
            const comparePackageSpeed = addRowItem(
                `${((fastestPackage / config.fastest[0]) * 100).toFixed(2)}%`
            )
                .concat(divider)
                .concat("\n");

            file = file.concat(
                pckg,
                runDate,
                numberOfIterations,
                fastestIterations,
                avgIteration,
                comparePackageSpeed
            );
        };

        for (let i = 0; i < 3; i += 1) {
            if (i === 1) file = file.concat(singleLargeEnv);
            if (i === 2) file = file.concat(multiEnvs);

            addResultsToTable("nvi", runs[i], i);
            addResultsToTable("dotenv", runs[i], i);
        }

        writeFileSync("README.md", file, { encoding: "utf-8" });
    } catch (error) {
        console.error(error);
        process.exit(1);
    }
})();
