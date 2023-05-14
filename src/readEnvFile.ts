import type { ConfigOptions, ParsedEnvs } from './index';
import { readFileSync } from 'fs';
import { join } from 'path';
import process from 'process';
// import { logError, logInfo } from './log';
import { logError, logInfo, logWarning } from './log';

const COMMENT = 0x23;
const LINE_DELIMITER = 0x0a;
const ESC = 0x5c;
const ASSIGN_OP = 0x3d;
const DOLLAR_SIGN = 0x24;
const OPEN_BRACE = 0x7b;
const CLOSE_BRACE = 0x7d;

/**
 * Reads an ".env" file and parses keys and values into a Map object
 *
 * @param fileName - the name of the ".env.*" file (without a path)
 * @param options - an option object of args: { `debug`: boolean | string, `dir`: string, `encoding`: BufferEncoding, `envMap`: Map<string, string> | undefined, `override`: boolean | string | undefined, }
 * @returns a single {@link https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Map | `Map`} object of Envs
 * @example readEnvFile(".env", { dir: "example", debug: false, encoding: "utf-8", envMap: new Map(), override: true });
 */
export default function readEnvFile(fileName: string, options = {} as ConfigOptions): ParsedEnvs | void {
    try {
        options.envMap ||= new Map<string, string>();
        options.encoding ||= 'utf-8';
        const fileBuf = readFileSync(join(options.dir || process.cwd(), fileName));

        let byteCount = 0;
        let lineCount = 0;
        while (byteCount < fileBuf.length) {
            // skip lines that begin with comments
            const lineBuf = fileBuf.subarray(byteCount, fileBuf.length);
            if (lineBuf[0] === COMMENT || lineBuf[0] === LINE_DELIMITER) {
                ++lineCount;
                const firstNewLineByte = fileBuf
                    .subarray(byteCount, fileBuf.length)
                    .indexOf(LINE_DELIMITER);

                if (firstNewLineByte >= 0) {
                    byteCount += firstNewLineByte + 1;
                    continue;
                } else {
                    // this shouldn't ever throw, but here just in case
                    throw Error('Found a comment or line delimiter, but was unable to locate the end of the current line!');
                }
            }

            let lineDelimiterIndex = lineBuf.indexOf(LINE_DELIMITER);
            if (lineDelimiterIndex >= 0) {
                ++lineCount;

                // check if multi-line value and find the end of the line
                if (lineBuf[lineDelimiterIndex - 1] === ESC && lineBuf[lineDelimiterIndex] === LINE_DELIMITER) {
                    while (lineBuf[lineDelimiterIndex - 1] === ESC) {
                        ++lineCount;
                        const nextLineDelimiterIndex = lineBuf.subarray(lineDelimiterIndex + 1, lineBuf.length).indexOf(LINE_DELIMITER);
                        if (nextLineDelimiterIndex >= 0) {
                            lineDelimiterIndex += nextLineDelimiterIndex + 1;
                        } else {
                            // this shouldn't ever throw, but here just in case
                            throw Error('Found a multi-line value but was unable to locate the end of the current line!');
                        }
                    }
                }

                // check for assignment '=' to split key from value
                const assignIndex = lineBuf.indexOf(ASSIGN_OP);
                if (assignIndex >= 0) {
                    const key = lineBuf.subarray(0, assignIndex).toString(options.encoding);

                    // skip reading from process.env if override wasn't set
                    if (process.env[key] && !options.override) {
                        byteCount += lineDelimiterIndex + 1;
                        continue;
                    }

                    let valBuf = lineBuf.subarray(assignIndex + 1, lineDelimiterIndex);
                    let valByteCount = 0;
                    while (valByteCount < valBuf.length) {
                        // check if chunk contains multi-line breaks
                        let chunk = valBuf.subarray(valByteCount - 1, valByteCount + 1);
                        if (chunk[0] === ESC && chunk[1] === LINE_DELIMITER) {
                            const prevValBuf = valBuf.subarray(0, valByteCount - 1);
                            const afterValBuf = valBuf.subarray(valByteCount + 1, valBuf.length);

                            valBuf = Buffer.concat([prevValBuf, afterValBuf], valBuf.length - 2);
                            valByteCount -= 2;
                        }

                        // check if chunk contains an interpolated variable
                        chunk = valBuf.subarray(valByteCount, valByteCount + 2);
                        if (chunk[0] === DOLLAR_SIGN && chunk[1] === OPEN_BRACE) {
                            const valSliceBuf = valBuf.subarray(valByteCount, valBuf.length);

                            const interpCloseIndex = valSliceBuf.indexOf(CLOSE_BRACE);
                            if (interpCloseIndex >= 0) {
                                const keyProp = valSliceBuf
                                    .subarray(2, interpCloseIndex)
                                    .toString(options.encoding);

                                const keyVal = process.env[keyProp] || options.envMap.get(keyProp);
                                if (!keyVal) {
                                    logWarning(
                                        `The '${key}' key contains an invalid interpolated variable: '\${${keyProp}}'. Unable to locate a value that corresponds to this key.`,
                                        fileName,
                                        lineCount
                                    );
                                }

                                // replace interpolated variable bytes with the keyVal value
                                const prevBuf = valBuf.subarray(0, valByteCount);
                                const newValBuf = Buffer.from(keyVal || '');
                                const afterBuf = valBuf.subarray(
                                    valByteCount + 1 + interpCloseIndex,
                                    valBuf.length
                                );

                                valBuf = Buffer.concat(
                                    [prevBuf, newValBuf, afterBuf],
                                    prevBuf.length + newValBuf.length + afterBuf.length
                                );

                                valByteCount = prevBuf.length + newValBuf.length - 1;
                            } else {
                                logError(
                                    `The key '${key}' contains an open interpolated '\${' operator but appears to be missing a closing '}' operator.`,
                                    fileName,
                                    lineCount
                                );
                            }
                        }

                        ++valByteCount;
                    }

                    if (key) options.envMap.set(key, valBuf.toString(options.encoding));
                }

                byteCount += lineDelimiterIndex + 1;
            } else {
                byteCount = fileBuf.length
            }
        }

        if (options.debug) {
            logInfo(`Processed ${lineCount} lines and ${byteCount} bytes!`, fileName, lineCount);
            logInfo(
                `Parsed ENVs: ${JSON.stringify(Object.fromEntries(options.envMap), null, 2)}`,
                fileName,
                lineCount
            );
        }

        return options.envMap;
    } catch (error: any) {
        logError(error.message, fileName);
    }
}
