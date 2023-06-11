import { writeFileSync } from "fs";

const ENV = new Array(10000).fill(0).reduce(
    (acc, _, index) => ({
        ...acc,
        [index]: index % 5 !== 0 ? `${index}` : `\$${index + 1}`
    }),
    {}
);

const file = Object.keys(ENV).reduce((str, index) => {
    let val = "";

    const r = index % 10;

    switch (r) {
        case 0: {
            val = `'${index}'`;
            break;
        }
        case 1: {
            val = `    ${index}    `;
            break;
        }
        case 2: {
            val = `"${index}"`;
            break;
        }
        case 3: {
            val = `{"foo": $\{${index - 1}\}, $\{${[index - 1]}\}: "bar"}`;
            break;
        }
        case 4: {
            val = `\\n${index}`;
            break;
        }
        case 5: {
            val = `# COMMENTS=${index}`;
            break;
        }
        case 6: {
            val = `$\{${index - 3}\}`
            break;
        }
        case 7: {
            val = "\\$\\$\\$\\$\\$\\$";
            break;
        }
        case 8: {
            val = ""
            break;
        }
        case 9: {
            val = "######"
            break;
        }
        default: {
            val = ENV[index];
            break;
        }
    }

    str += `${index > 0 ? "\n" : ""}${r !== 5 ? `${index}=` : ""}${val}`;

    return str;
}, "");

writeFileSync(".env.interp", file.concat("\n"), { encoding: "utf-8" });
