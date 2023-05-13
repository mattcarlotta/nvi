import type { ConfigOptions, ParsedEnvs } from './index';
import { env } from 'process';
import readEnvFile from './readEnvFile';
import { logError } from './log';

/**
 * Extracts and interpolates one or multiple `.env` files into an object and assigns them to {@link https://nodejs.org/api/process.html#process_process_env | `process.env`}.
 * Example: 'KEY=value' becomes { KEY: "value" }
 *
 * @param options - accepts: { `dir`: string, `paths`: string | string[], `encoding`: BufferEncoding, `override`: boolean | string, `debug`: boolean | string }
 * @returns a single {@link https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Map | `Map`} object of Envs
 * @example config({ dir: "example", paths: ".env" });
 */
export default function config(options?: ConfigOptions): ParsedEnvs {
    const envMap: ParsedEnvs = new Map();
    const dir = options?.dir || process.cwd();
    const paths = options?.paths || ['.env'];
    const debug = options?.debug;
    const override = options?.override;
    const encoding = options?.encoding || 'utf-8';
    const required = options?.required || [];

    const configs = Array.isArray(paths) ? paths : paths.split(',');
    for (let i = 0; i < configs.length; ++i) {
        readEnvFile(configs[i], { dir, debug, encoding, envMap, override });
    }

    if (required.length) {
        const requiredKeys: string[] = [];
        for (let i = 0; i < required.length; ++i) {
            const key = required[i];
            if (!envMap.get(key)) requiredKeys.push(key);
        }

        if (requiredKeys.length) {
            logError(
                `The following Envs are marked as required: ${requiredKeys
                    .map((v) => `'${v}'`)
                    .join(
                        ', '
                    )}, but they are undefined after all of the .env files were parsed.`
            );
        }
    }

    for (const [key, value] of envMap) {
        env[key] = value;
    }

    return envMap;
}
