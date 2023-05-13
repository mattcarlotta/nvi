import type { ConfigOptions, ParsedEnvs } from 'index';
import { readFileSync } from 'fs';
import { join } from 'path';
import { env } from 'process';
import { logError, logInfo, logWarning } from './log';

const COMMENT = 0x23;
const LINE_DELIMITER = 0x0A;
const ASSIGN_OP = 0x3D;
const MULTI_LINE_DELIMITER = 0x5C0A;
const INTRP_OPEN = 0x247B;
const INTRP_CLOSE = 0x7D;

export type ReadEnvFileOptions = {
    debug?: ConfigOptions["debug"],
    dir: ConfigOptions["dir"],
    envMap?: ParsedEnvs,
    encoding: ConfigOptions["encoding"],
    override: ConfigOptions["override"]
}

const defaultEnvMap: ParsedEnvs = new Map();

const defaultOptions: ReadEnvFileOptions = {
    debug: true,
    dir: "",
    encoding: "utf-8",
    envMap: defaultEnvMap,
    override: false
}

/**
 * Reads an ".env" file and parses keys and values into a Map object
 *
 * @param fileName - the name of the ".env.*" file (without a path)
 * @param options - an option object of option args: { `debug`: boolean | string, `dir`: string, `encoding`: BufferEncoding, `envMap`: Map<string, string> | undefined, `override`: boolean | string, }
 * @returns a single Hash Map of Envs
 * @example readEnvFile(".env", { dir: "example", debug: false, encoding: "utf-8", envMap: new Map(), override: true });
 */
export default function readEnvFile(fileName: string, options = defaultOptions): ParsedEnvs | void {
    try {
        const envMap = options?.envMap || defaultEnvMap;
        const file = join(options?.dir || process.cwd(), fileName);
        const fileBuf = readFileSync(file);

        let byteCount = 0;
        let lineCount = 0;
        for (let i = 0; i < fileBuf.length; ++i) {
            if (fileBuf[i] === LINE_DELIMITER) {
                ++lineCount;
                // check if there is a 2 byte multi-line delimiter
                const multiChunk = fileBuf.subarray(i - 1, i + 1);
                if (multiChunk.length > 1 && multiChunk.readUint16BE() === MULTI_LINE_DELIMITER) {
                    continue;
                }

                const lineBuf = fileBuf.subarray(byteCount, i);

                // skip lines that begin with comments
                if (lineBuf[0] === COMMENT) {
                    const firstNewLineByte = fileBuf
                        .subarray(byteCount, fileBuf.length)
                        .indexOf(LINE_DELIMITER);

                    if (firstNewLineByte >= 0) {
                        byteCount += firstNewLineByte + 1;
                        continue;
                    } else {
                        // this shouldn't ever throw, but here just in case
                        throw Error('Unable to find the end of the current line!');
                    }
                }

                // check for assignment '=' to split key from value
                const assignIndex = lineBuf.indexOf(ASSIGN_OP);
                if (assignIndex >= 0) {
                    const key = lineBuf.subarray(0, assignIndex).toString(options.encoding);
                    if (env[key] && !options.override) {
                        logWarning(
                            `The '${key}' key is already defined in process.env. If you wish to override this key, then you must pass an 'override' argument. Skipping.`,
                            fileName,
                            lineCount
                        );
                        byteCount += lineBuf.length + 1;
                        continue;
                    }

                    let valBuf = lineBuf.subarray(assignIndex + 1, lineBuf.length);
                    let valByteCount = 0;
                    while (valByteCount < valBuf.length) {
                        // check if chunk contains multi-line breaks
                        let chunk = valBuf.subarray(valByteCount - 1, valByteCount + 1);

                        // NOTE: This may be improved by checking per byte
                        // rather than per 2 bytes: chunk[0] === "$", chunk[1] === "{"
                        if (chunk.length > 1 && chunk.readUint16BE() === MULTI_LINE_DELIMITER) {
                            const prevValBuf = valBuf.subarray(0, valByteCount - 1);
                            const afterValBuf = valBuf.subarray(valByteCount + 1, valBuf.length);

                            valBuf = Buffer.concat([prevValBuf, afterValBuf], valBuf.length - 2);
                            valByteCount -= 2;
                        }

                        // check if chunk contains an interpolated variable
                        chunk = valBuf.subarray(valByteCount, valByteCount + 2);

                        if (chunk.length > 1 && chunk.readUint16BE() === INTRP_OPEN) {
                            const valSliceBuf = valBuf.subarray(valByteCount, valBuf.length);

                            const interpCloseIndex = valSliceBuf.indexOf(INTRP_CLOSE);
                            if (interpCloseIndex >= 0) {
                                const keyProp = valSliceBuf.subarray(2, interpCloseIndex).toString(options.encoding);

                                const keyVal = env[keyProp] || envMap.get(keyProp);
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

                    if (key) envMap.set(key, valBuf.toString(options.encoding));
                }

                byteCount = i + 1;
            }
        }

        if (options.debug) {
            logInfo(`Processed ${lineCount} lines and ${byteCount} bytes!`, fileName, lineCount);
            logInfo(`Parsed ENVs: ${JSON.stringify(Object.fromEntries(envMap), null, 2)}`, fileName, lineCount);
        }

        return envMap;
    } catch (error: any) {
        logError(error.message, fileName);
    }
}
