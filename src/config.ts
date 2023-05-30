import type { ConfigOptions, ParsedEnvs } from './index';
import { env } from 'process';
import readEnvFile from './readEnvFile';
import { logError } from './log';

/**
 * Extracts and interpolates one or multiple `.env` files into an object and assigns them to {@link https://nodejs.org/api/process.html#process_process_env | `process.env`}.
 * Example: 'KEY=value' becomes { KEY: "value" }
 *
 * @param options - accepts: { `debug`: boolean | string, `dir`: string, `files`: string[], `encoding`: BufferEncoding, `override`: boolean | string | undefined }
 * @returns a single object of Envs
 * @example config({ dir: "example", paths: ".env" });
 */
export default function config(options = {} as ConfigOptions): ParsedEnvs {
    options.envMap ||= {};
    options.files ||= ['.env'];
    options.encoding ||= 'utf-8';
    options.required ||= [];

    for (let i = 0; i < options.files.length; ++i) {
        readEnvFile(options.files[i], options);
    }

    if (options.required.length) {
        const undefReqKeys: string[] = [];
        for (let i = 0; i < options.required.length; ++i) {
            const key = options.required[i];
            if (!env[key]) undefReqKeys.push(key);
        }

        if (undefReqKeys.length) {
            logError(
                `The following Envs are marked as required: ${undefReqKeys
                    .map((v) => `'${v}'`)
                    .join(', ')}, but they are undefined after all of the .env files were parsed.`
            );
        }
    }

    return options.envMap;
}
