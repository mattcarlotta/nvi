import { readFileSync } from 'fs';
import { join } from 'path';
import { env } from 'process';
import { logError, logInfo, logWarning } from '../log';

const COMMENT = 0x23;
const LINE_DELIMITER = 0x0a;
const ASSIGN_OP = 0x3d;
const MULTI_LINE_DELIMITER = 0x5c0a;
const INTRP_OPEN = 0x247b;
const INTRP_CLOSE = 0x7d;

((file: string, override: boolean) => {
    try {
        const envMap = new Map<string, string>();
        const filePath = join(process.cwd(), file);
        const fileBuf = readFileSync(filePath);

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
                    const key = lineBuf.subarray(0, assignIndex).toString();
                    if (env[key] && !override) {
                        logWarning(
                            `The '${key}' key is already defined in process.env. If you wish to override this key, then you must pass an 'override' argument. Skipping.`,
                            file,
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
                                const keyProp = valSliceBuf.subarray(2, interpCloseIndex).toString();

                                const keyVal = env[keyProp] || envMap.get(keyProp);
                                if (!keyVal) {
                                    logWarning(
                                        `The '${key}' key contains an invalid interpolated variable: '\${${keyProp}}'. Unable to locate a value that corresponds to this key.`,
                                        file,
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
                                    file,
                                    lineCount
                                );
                            }
                        }

                        ++valByteCount;
                    }

                    if (key) envMap.set(key, valBuf.toString());
                }

                byteCount = i + 1;
            }
        }

        logInfo(`Processed ${lineCount} lines and ${byteCount} bytes!`, file, lineCount);

        logInfo(`Parsed ENVs: ${JSON.stringify(Object.fromEntries(envMap), null, 2)}`, file, lineCount);

        return envMap;
    } catch (error: any) {
        logError(error.message, file);
    }
})('.env', false);
