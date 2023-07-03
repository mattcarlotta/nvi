import type { ConfigOptions, ParsedEnvs } from './index';
import EnvParser from './parser';

/**
 * Extracts and interpolates one or multiple `.env` files into an object and assigns them to {@link https://nodejs.org/api/process.html#process_process_env | `process.env`}.
 * Example: 'KEY=value' becomes { KEY: "value" }
 *
 * @param options an object of `ConfigOptions`: { `debug`: boolean | undefined, `dir`: string, `files`: string[], `override`: boolean | undefined, `required`: string[] | undefined }
 * @returns a single object of parsed `"KEY": "value"` pairs
 *
 * @example Loading many .env files
 * ```ts
 * const parsedEnvs = config({ debug: true, dir: "example", files: ["example1.env", "example2.env", "example3.env"], override: false, required: [] });
 * // parsedEnvs = { "KEY1": "value", "KEY2", "value", ...etc }
 * ```
 */
export default function config(options = {} as ConfigOptions): ParsedEnvs {
    options.files ||= ['.env'];
    const parser = new EnvParser(options);

    for (let i = 0; i < options.files.length; ++i) {
        parser.read(options.files[i])?.parse();
    }

    return parser.getEnvs();
}
