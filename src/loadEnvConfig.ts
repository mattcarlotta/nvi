import type { ConfigOptions } from './index';
import { readFileSync } from 'fs';
import { join } from 'path';
import { cwd } from 'process';
import { logError } from './log';

/**
 * Loads a configuration object from the `env.config.json` file based upon `LOAD_ENV`.
 *
 * @param env - the environment to be loaded
 * @param dir - the directory where the config is located
 * @returns a config file as { key: value } pairs to be used with the `config` function
 * @example loadConfig("development");
 */
export default function loadEnvConfig(env: string, dir?: string): ConfigOptions | void {
    try {
        const file = readFileSync(join(dir || cwd(), 'env.config.json'), {
            encoding: 'utf-8',
        });

        const config = JSON.parse(file);

        if (typeof config !== 'object' || !Object.prototype.hasOwnProperty.call(config, env))
            throw String(`Unable to locate a '${env}' configuration within the env.config.json!`);

        return config[env];
    } catch (error: any) {
        logError(error.toString());
    }
}
