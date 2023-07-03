import type { ConfigOptions, ParsedEnvs } from './index';
import EnvParser from './parser';

/**
 * Extracts and interpolates one or multiple `.env` files into an object and assigns them to {@link https://nodejs.org/api/process.html#process_process_env | `process.env`}.
 * Example: 'KEY=value' becomes { KEY: "value" }
 *
 * @param options - accepts: { `debug`: boolean | undefined, `dir`: string, `files`: string[], `override`: boolean | undefined }
 * @returns a single object of Envs
 * @example config({ dir: "example", paths: ".env" });
 */
export default function config(options = {} as ConfigOptions): ParsedEnvs {
    options.files ||= ['.env'];
    const parser = new EnvParser(options);

    for (let i = 0; i < options.files.length; ++i) {
        parser.read(options.files[i])?.parse();
    }

    return parser.getEnvs();
}
