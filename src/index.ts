import { env } from 'process';
import config from './config';
import EnvConfigLoader from './configLoader';
import EnvParser from './parser';

export type ParsedEnvs = Record<string, string>;

export type ConfigOptions = {
    dir?: string;
    files?: string[];
    override?: boolean;
    debug?: boolean;
    required?: string[];
};

/**
 * Immediately loads an env configuration from an env.config.json cofiguration file when the package is preloaded or imported.
 */
(function(): void {
    if (env.LOAD_CONFIG) {
        const options = new EnvConfigLoader(env.LOAD_CONFIG).getOptions();

        if (options) config(options);

        // prevents the IFFE from reloading the config file on subsequent imports
        delete process.env.LOAD_CONFIG;
    }
})();

export { config, EnvConfigLoader, EnvParser };

const envFuncs = {
    config,
    EnvConfigLoader,
    EnvParser,
};

export default envFuncs;
