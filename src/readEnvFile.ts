import type { ConfigOptions, ParsedEnvs } from './index';
import { readFileSync } from 'fs';
import { join } from 'path';
import process from 'process';
import { logError, logInfo, logWarning } from './log';

const COMMENT = 0x23;
const LINE_DELIMITER = 0x0a;
const BACK_SLASH = 0x5c;
const ASSIGN_OP = 0x3d;
const DOLLAR_SIGN = 0x24;
const OPEN_BRACE = 0x7b;
const CLOSE_BRACE = 0x7d;

/**
 * Reads an ".env" file, parses keys and values into an object and assigns each to process.env
 *
 * @param fileName - the name of the ".env" file (without a path)
 * @param options - an option object of args: { `debug`: boolean | string | undefined, `dir`: string | undefined, `encoding`: BufferEncoding, `envMap`: Record<string, string> | undefined, `override`: boolean | string | undefined, }
 * @returns a single object of Envs
 * @example readEnvFile(".env", { dir: "example", debug: false, encoding: "utf-8", envMap: {}, override: true });
 */
export default function readEnvFile(
    fileName: string,
    options = {} as ConfigOptions
): ParsedEnvs | void {
    let byteCount = 0;
    let lineCount = 0;

    try {
        options.envMap ||= {};
        options.encoding ||= 'utf-8';
        const fileBuf = readFileSync(join(options.dir || process.cwd(), fileName));

        while (byteCount < fileBuf.length) {
            // skip lines that begin with comments
            const lineBuf = fileBuf.subarray(byteCount, fileBuf.length);
            let lineDelimiterIndex = lineBuf.indexOf(LINE_DELIMITER);
            if (lineBuf[0] === COMMENT || lineBuf[0] === LINE_DELIMITER) {
                ++lineCount;
                byteCount += lineDelimiterIndex + 1;
                continue;
            }

            // split key from value by assignment "="
            const assignmentIndex = lineBuf.indexOf(ASSIGN_OP);
            if (lineDelimiterIndex >= 0 && assignmentIndex >= 0) {
                const key = lineBuf.subarray(0, assignmentIndex).toString(options.encoding);

                // skip writing to process.env if key exists and override wasn't set
                if (process.env[key] && !options.override) {
                    // if there's a multi-line value, then locate the next new line byte that's not preceded with a multi-line "\" byte
                    // NOTE: This will collect other keys and values if the multi-line value hasn't been properly delineated
                    while (lineBuf[lineDelimiterIndex - 1] === BACK_SLASH && lineDelimiterIndex < lineBuf.length) {
                        ++lineCount;

                        lineDelimiterIndex += lineBuf
                            .subarray(lineDelimiterIndex + 1, lineBuf.length)
                            .indexOf(LINE_DELIMITER) + 1;
                    }

                    ++lineCount;
                    byteCount += lineDelimiterIndex + 1;
                    continue;
                }

                const valBuf = lineBuf.subarray(assignmentIndex + 1, lineBuf.length);
                let valByteCount = 0;
                let value = "";
                while (valByteCount < valBuf.length) {
                    const currentChar = valBuf[valByteCount];
                    const nextChar = valBuf[valByteCount + 1];

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
                        const valSliceBuf = valBuf.subarray(valByteCount, valBuf.length);

                        const interpCloseIndex = valSliceBuf.indexOf(CLOSE_BRACE);
                        if (interpCloseIndex >= 0) {
                            const keyProp = valSliceBuf
                                .subarray(2, interpCloseIndex)
                                .toString(options.encoding);

                            const interpolatedValue = process.env[keyProp] || '';
                            if (!interpolatedValue) {
                                logWarning(
                                    `The key '${key}' contains an invalid interpolated variable: '\${${keyProp}}'. Unable to locate a value that corresponds to this key.`,
                                    fileName,
                                    lineCount,
                                    valByteCount + assignmentIndex
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

                    value += String.fromCharCode(valBuf[valByteCount]);

                    ++valByteCount;
                }

                if (key) {
                    process.env[key] = value;
                    options.envMap[key] = value;
                }

                byteCount += assignmentIndex + valByteCount + 1
            } else {
                byteCount = fileBuf.length;
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
