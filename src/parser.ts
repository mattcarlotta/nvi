import type { ConfigOptions, ParsedEnvs } from './index';
import { readFileSync } from 'fs';
import { join } from 'path';
import process from 'process';

const PARSER_INTERPOLATION_WARNING = 0;
const PARSER_INTERPOLATION_ERROR = 1;
const PARSER_DEBUG = 2;
const PARSER_DEBUG_FILE_PROCESSED = 3;
const PARSER_REQUIRED_ENV_ERROR = 4;
const PARSER_FILE_ERROR = 5;
const COMMENT = '#';
const LINE_DELIMITER = '\n';
const BACK_SLASH = '\\';
const ASSIGN_OP = '=';
const DOLLAR_SIGN = '$';
const OPEN_BRACE = '{';
const CLOSE_BRACE = '}';

export default class EnvParser {
    private debug: boolean;
    private dir: string;
    private envFile: string;
    private fileName: string;
    private override: boolean;
    private filePath: string;
    private fileLength: number;
    private byteCount: number;
    private lineCount: number;
    private valByteCount: number;
    private assignmentIndex: number;
    private key: string;
    private keyProp: string;
    private value: string;
    private undefinedKeys: string[];
    private requiredEnvs: string[];
    private envMap: ParsedEnvs;

    constructor(options = {} as ConfigOptions) {
        this.debug = options?.debug || false;
        this.dir = options?.dir || '';
        this.envFile = '';
        this.override = options?.override || false;
        this.requiredEnvs = options?.required || [];
        this.filePath = '';
        this.fileName = '';
        this.fileLength = 0;
        this.byteCount = 0;
        this.lineCount = 0;
        this.valByteCount = 0;
        this.assignmentIndex = -1;
        this.key = '';
        this.keyProp = '';
        this.value = '';
        this.undefinedKeys = [];
        this.requiredEnvs = [];
        this.envMap = {};
    }

    public read(envFileName: string): EnvParser | void {
        try {
            this.envFile = readFileSync(join(this.dir || process.cwd(), envFileName), {
                encoding: 'utf-8',
            });
            this.fileName = envFileName;
            this.fileLength = this.envFile.length;
            this.byteCount = 0;
            this.lineCount = 0;

            return this;
        } catch (error) {
            this.log(PARSER_FILE_ERROR);
            this.exitProcess();
        }
    }

    public parse(): void {
        while (this.byteCount < this.fileLength) {
            // skip lines that begin with comments
            const line = this.envFile.substring(this.byteCount, this.fileLength);
            let lineDelimiterIndex = line.indexOf(LINE_DELIMITER);
            if (line[0] === COMMENT || line[0] === LINE_DELIMITER) {
                ++this.lineCount;
                this.byteCount += lineDelimiterIndex + 1;
                continue;
            }

            // split key from value by assignment "="
            this.assignmentIndex = line.indexOf(ASSIGN_OP);
            if (lineDelimiterIndex >= 0 && this.assignmentIndex >= 0) {
                this.key = line.substring(0, this.assignmentIndex);

                // skip writing to process.env if key exists and override wasn't set
                if (process.env[this.key] && !this.override) {
                    // if there's a multi-line value, then locate the next new line byte that's not preceded with a multi-line "\" byte
                    // NOTE: This will collect other keys and values if the multi-line value hasn't been properly delineated
                    while (
                        line[lineDelimiterIndex - 1] === BACK_SLASH &&
                        lineDelimiterIndex < line.length
                    ) {
                        ++this.lineCount;

                        lineDelimiterIndex +=
                            line
                                .substring(lineDelimiterIndex + 1, line.length)
                                .indexOf(LINE_DELIMITER) + 1;
                    }

                    ++this.lineCount;
                    this.byteCount += lineDelimiterIndex + 1;
                    continue;
                }

                const valSlice = line.substring(this.assignmentIndex + 1, line.length);
                this.valByteCount = 0;
                this.value = '';
                while (this.valByteCount < valSlice.length) {
                    const currentChar = valSlice[this.valByteCount];
                    const nextChar = valSlice[this.valByteCount + 1];

                    // stop parsing if there's a new line "\n" byte
                    if (currentChar === LINE_DELIMITER) {
                        break;
                    }

                    // skip parsing multi-line "\\n" bytes
                    if (currentChar === BACK_SLASH && nextChar === LINE_DELIMITER) {
                        ++this.lineCount;
                        this.valByteCount += 2;
                        continue;
                    }

                    // interpolate a value from key "${key}"
                    if (currentChar === DOLLAR_SIGN && nextChar === OPEN_BRACE) {
                        const valSliceBuf = valSlice.substring(this.valByteCount, valSlice.length);

                        const interpCloseIndex = valSliceBuf.indexOf(CLOSE_BRACE);
                        if (interpCloseIndex >= 0) {
                            this.keyProp = valSliceBuf.substring(2, interpCloseIndex);

                            const interpolatedValue = process.env[this.keyProp] || '';
                            if (!interpolatedValue) {
                                this.log(PARSER_INTERPOLATION_WARNING);
                            }

                            this.value += interpolatedValue;

                            // factor in the interpolated key length with the "$", "{" and "}" bytes
                            this.valByteCount += this.keyProp.length + 3;

                            // the next byte could be an interpolated value, so skip this iteration
                            continue;
                        } else {
                            this.log(PARSER_INTERPOLATION_ERROR);
                            this.exitProcess();
                        }
                    }

                    this.value += valSlice[this.valByteCount];

                    ++this.valByteCount;
                }

                if (this.key) {
                    process.env[this.key] = this.value;
                    this.envMap[this.key] = this.value;
                    if (this.debug) {
                        this.log(PARSER_DEBUG);
                    }
                }

                this.byteCount += this.assignmentIndex + this.valByteCount + 1;
            } else {
                this.byteCount = this.fileLength;
            }
        }

        if (this.debug) {
            this.log(PARSER_DEBUG_FILE_PROCESSED);
        }
    }

    public getEnvs(): ParsedEnvs {
        if (this.requiredEnvs.length) {
            for (let i = 0; i < this.requiredEnvs.length; ++i) {
                const key = this.requiredEnvs[i];
                if (!this.envMap[key]) this.undefinedKeys.push(key);
            }

            if (this.undefinedKeys.length) {
                this.log(PARSER_REQUIRED_ENV_ERROR);
                this.exitProcess();
            }
        }

        return this.envMap;
    }

    private exitProcess(): void {
        if (process.env.NODE_ENV !== 'test') {
            process.exit(1);
        }
    }

    private log(code: number): void {
        switch (code) {
            case PARSER_INTERPOLATION_WARNING: {
                console.log(
                    `[nvi] (parser::INTERPOLATION_WARNING::${this.fileName}:${this.lineCount + 1}:${
                        this.assignmentIndex + this.valByteCount + 2
                    }) The key '${this.key}' contains an invalid interpolated variable: '${
                        this.keyProp
                    }'. Unable to locate a value that corresponds to this key.`
                );
                break;
            }
            case PARSER_INTERPOLATION_ERROR: {
                console.log(
                    `[nvi] (parser::INTERPOLATION_ERROR::${this.fileName}:${this.lineCount + 2}:${
                        this.assignmentIndex + this.valByteCount + 2
                    }) The key '${
                        this.key
                    }' contains an interpolated variable: '${' operator but appears to be missing a closing '}' operator.`
                );
                break;
            }
            case PARSER_DEBUG: {
                console.log(
                    `[nvi] (parser::DEBUG::${this.fileName}:${this.lineCount + 1}:${
                        this.assignmentIndex + this.valByteCount + 1
                    }) Set key '${this.key}' to equal value '${this.value}'.`
                );
                break;
            }
            case PARSER_DEBUG_FILE_PROCESSED: {
                console.log(
                    `[nvi] (parser::DEBUG_FILE_PROCESSED::${this.fileName}) Processed ${
                        this.lineCount
                    } line${this.lineCount > 1 ? 's' : ''} and ${this.byteCount} bytes!`
                );
                console.log(`${JSON.stringify(this.envMap, null, 4)}\n`);
                break;
            }
            case PARSER_REQUIRED_ENV_ERROR: {
                console.log(
                    `[nvi] (parser::REQUIRED_ENV_ERROR) The following ENV keys are marked as required: '${this.requiredEnvs.join(
                        ','
                    )}' but they are undefined after all of the .env files were parsed.`
                );
                break;
            }
            case PARSER_FILE_ERROR: {
                console.log(
                    `[nvi] (parser::FILE_ERROR) Unable to locate '${this.filePath}'. The .env file doesn't appear to exist!`
                );
                break;
            }
            default:
                break;
        }
    }
}
