import { env } from 'process';
import config from './config';
import EnvConfigLoader from './configLoader';
import EnvParser from './parser';

export type ParsedEnvs = Record<string, string>;

export type ConfigOptions = {
    dir?: string;
    envMap?: ParsedEnvs;
    files?: string[];
    override?: boolean;
    debug?: boolean;
    required?: string[];
};

/**
 * Immediately loads an env.config.json file when the package is preloaded or imported.
 */
(function (): void {
    // checks if LOAD_CONFIG is defined and assigns config options
    if (env.LOAD_CONFIG) {
        const envConfig = new EnvConfigLoader(env.LOAD_CONFIG).getOptions();

        if (envConfig) config(envConfig);

        // prevents the IFFE from reloading the config file
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
