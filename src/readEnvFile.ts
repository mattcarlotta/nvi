import type { ConfigOptions, ParsedEnvs } from './index';
import { readFileSync } from 'fs';
import { join } from 'path';
import process from 'process';
import { logError, logInfo, logWarning } from './log';

const COMMENT = '#';
const LINE_DELIMITER = '\n';
const BACK_SLASH = '\\';
const ASSIGN_OP = '=';
const DOLLAR_SIGN = '$';
const OPEN_BRACE = '{';
const CLOSE_BRACE = '}';

/**
 * Reads an ".env" file, parses keys and values into an object and assigns each to process.env
 *
 * @param fileName - the name of the ".env" file (without a path)
 * @param options - an option object of args: { `debug`: boolean | string | undefined, `dir`: string | undefined, `envMap`: Record<string, string> | undefined, `override`: boolean | string | undefined, }
 * @returns a single object of Envs
 * @example readEnvFile(".env", { dir: "example", debug: false, envMap: {}, override: true });
 */
export default function readEnvFile(
    fileName: string,
    options = {} as ConfigOptions
): ParsedEnvs | void {
    let byteCount = 0;
    let lineCount = 0;

    try {
        options.envMap ||= {};
        const file = readFileSync(join(options.dir || process.cwd(), fileName), { encoding: 'utf-8' });

        while (byteCount < file.length) {
            // skip lines that begin with comments
            const line = file.substring(byteCount, file.length);
            let lineDelimiterIndex = line.indexOf(LINE_DELIMITER);
            if (line[0] === COMMENT || line[0] === LINE_DELIMITER) {
                ++lineCount;
                byteCount += lineDelimiterIndex + 1;
                continue;
            }

            // split key from value by assignment "="
            const assignmentIndex = line.indexOf(ASSIGN_OP);
            if (lineDelimiterIndex >= 0 && assignmentIndex >= 0) {
                const key = line.substring(0, assignmentIndex);

                // skip writing to process.env if key exists and override wasn't set
                if (process.env[key] && !options.override) {
                    // if there's a multi-line value, then locate the next new line byte that's not preceded with a multi-line "\" byte
                    // NOTE: This will collect other keys and values if the multi-line value hasn't been properly delineated
                    while (line[lineDelimiterIndex - 1] === BACK_SLASH && lineDelimiterIndex < line.length) {
                        ++lineCount;

                        lineDelimiterIndex += line
                            .substring(lineDelimiterIndex + 1, line.length)
                            .indexOf(LINE_DELIMITER) + 1;
                    }

                    ++lineCount;
                    byteCount += lineDelimiterIndex + 1;
                    continue;
                }

                const valSlice = line.substring(assignmentIndex + 1, line.length);
                let valByteCount = 0;
                let value = "";
                while (valByteCount < valSlice.length) {
                    const currentChar = valSlice[valByteCount];
                    const nextChar = valSlice[valByteCount + 1];

                    // stop parsing if there's a new line "\n" byte
                    if (currentChar === LINE_DELIMITER) {
                        break;
                    }

                    // skip parsing multi-line "\\n" bytes
                    if (currentChar === BACK_SLASH && nextChar === LINE_DELIMITER) {
                        ++lineCount;
                        valByteCount += 2;
                        continue;
                    }

                    // interpolate a value from key "${key}"
                    if (currentChar === DOLLAR_SIGN && nextChar === OPEN_BRACE) {
                        const valSliceBuf = valSlice.substring(valByteCount, valSlice.length);

                        const interpCloseIndex = valSliceBuf.indexOf(CLOSE_BRACE);
                        if (interpCloseIndex >= 0) {
                            const keyProp = valSliceBuf
                                .substring(2, interpCloseIndex);

                            const interpolatedValue = process.env[keyProp] || '';
                            if (!interpolatedValue) {
                                logWarning(
                                    `The key '${key}' contains an invalid interpolated variable: '\${${keyProp}}'. Unable to locate a value that corresponds to this key.`,
                                    fileName,
                                    lineCount + 1,
                                    valByteCount + assignmentIndex + 2
                                );
                            }

                            value += interpolatedValue;

                            // factor in the interpolated key length with the "$", "{" and "}" bytes
                            valByteCount += keyProp.length + 3;

                            // the next byte could be an interpolated value, so skip this iteration
                            continue;
                        } else {
                            ++lineCount;
                            throw new Error(
                                `The key '${key}' contains an open interpolated '\${' operator but appears to be missing a closing '}' operator.`,
                            );
                        }
                    }

                    value += valSlice[valByteCount];

                    ++valByteCount;
                }

                if (key) {
                    process.env[key] = value;
                    options.envMap[key] = value;
                }

                byteCount += assignmentIndex + valByteCount + 1
            } else {
                byteCount = file.length;
            }
        }

        if (options.debug) {
            logInfo(`Processed ${lineCount} lines and ${byteCount} bytes!`, fileName, lineCount, byteCount);
            logInfo(
                `Parsed ENVs: ${JSON.stringify(options.envMap, null, 2)}`,
                fileName,
                lineCount,
                byteCount
            );
        }

        return options.envMap;
    } catch (error: any) {
        logError(error.message, fileName, lineCount, byteCount);
    }
}
