import { readFileSync } from 'fs';
import { join } from 'path';
import { logError, logWarning } from '../log';

const COMMENT = 0x23;
const LINE_DELIMITER = 0x0a;
const ASSIGN_OP = 0x3d;
const MULTI_LINE_DELIMITER = 0x5c0a;
// const MULTI_LINE_DELIMITER_STR = "\\\n";
const INTRP_OPEN = 0x247b;
const INTRP_CLOSE = 0x7d;

((file: string) => {
    try {
        const envMap = new Map<string, string>();
        const filePath = join(process.cwd(), file);
        const fileBuf = readFileSync(filePath);

        let byteCount = 0;
        for (let i = 0; i < fileBuf.length; ++i) {
            if (fileBuf[i] === LINE_DELIMITER) {
                // check if there is a 2 byte multi-line delimiter
                const multiChunk = fileBuf.slice(i - 1, i + 1);
                if (multiChunk.length > 1 && multiChunk.readUint16BE() === MULTI_LINE_DELIMITER) {
                    continue;
                }

                const lineBuf = fileBuf.slice(byteCount, i);

                // skip lines that begin with comments
                if (lineBuf[0] === COMMENT) {
                    const firstNewLineByte = fileBuf
                        .slice(byteCount, fileBuf.length)
                        .indexOf(LINE_DELIMITER);

                    if (firstNewLineByte >= 0) {
                        byteCount += firstNewLineByte + 1;
                        continue;
                    } else {
                        throw Error('Unable to find the end of the current line!');
                    }
                }

                // check for assignment '=' to split key from value
                const assignIndex = lineBuf.indexOf(ASSIGN_OP);
                if (assignIndex >= 0) {
                    const key = lineBuf.slice(0, assignIndex).toString();

                    let valBuf = lineBuf.slice(assignIndex + 1, lineBuf.length);
                    let valByteCount = 0;
                    while (valByteCount < valBuf.length) {
                        // check if chunk contains multi-line breaks
                        let chunk = valBuf.slice(valByteCount - 1, valByteCount + 1);

                        // NOTE: This may be improved by checking per byte
                        // rather than per 2 bytes: chunk[0] === "$", chunk[1] === "{"
                        if (chunk.length > 1 && chunk.readUint16BE() === MULTI_LINE_DELIMITER) {
                            const prevValBuf = valBuf.slice(0, valByteCount - 1);
                            const afterValBuf = valBuf.slice(valByteCount + 1, valBuf.length);

                            valBuf = Buffer.concat([prevValBuf, afterValBuf], valBuf.length - 2);
                            valByteCount -= 2;
                        }

                        // check if chunk contains an interpolated variable
                        chunk = valBuf.slice(valByteCount, valByteCount + 2);

                        if (chunk.length > 1 && chunk.readUint16BE() === INTRP_OPEN) {
                            const valSliceBuf = valBuf.slice(valByteCount, valBuf.length);

                            const interpCloseIndex = valSliceBuf.indexOf(INTRP_CLOSE);

                            if (interpCloseIndex >= 0) {
                                const keyProp = valSliceBuf.slice(2, interpCloseIndex).toString();

                                const keyVal = envMap.get(keyProp);

                                if (!keyVal) {
                                    logWarning(
                                        `The '${key}' key contains an invalid interpolated variable: '\${${keyProp}}'. Unable to locate a value that corresponds to this key.`
                                    );
                                }

                                // replace interpolated variable bytes with the keyVal value
                                const prevBuf = valBuf.slice(0, valByteCount);
                                const newValBuf = Buffer.from(keyVal || '');
                                const afterBuf = valBuf.slice(
                                    valByteCount + 1 + interpCloseIndex,
                                    valBuf.length
                                );

                                valBuf = Buffer.concat(
                                    [prevBuf, newValBuf, afterBuf],
                                    prevBuf.length + newValBuf.length + afterBuf.length
                                );

                                valByteCount = prevBuf.length + newValBuf.length - 1;
                            } else {
                                throw Error(
                                    `The key '${key}' contains an open interpolated '\${' operator but appears to be missing a closing '}' operator.`
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

        console.log({ envMap });

        return envMap;
    } catch (error: any) {
        logError(error.message);
        process.exit(1);
    }
})('.env');
