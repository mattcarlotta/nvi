import { env } from 'process';
import config from './config';
import loadEnvConfig from './loadEnvConfig';
import readEnvFile from './readEnvFile';

export type ParsedEnvs = Map<string, string>;

export type Option = boolean | string | undefined;

export type ConfigOptions = {
    dir?: string;
    envMap?: ParsedEnvs;
    files?: string[];
    encoding?: BufferEncoding;
    override?: Option;
    debug?: Option;
    required?: string[];
};

/**
 * Immediately loads an env.config.json file when the package is preloaded or imported.
 */
(function(): void {
    const { LOAD_CONFIG } = env;

    // checks if LOAD_CONFIG is defined and assigns config options
    if (LOAD_CONFIG) {
        const envConfig = loadEnvConfig(LOAD_CONFIG);

        if (envConfig) config(envConfig);

        // prevents the IFFE from reloading the config file
        delete process.env.LOAD_CONFIG;
    }
})();

export { config, loadEnvConfig, readEnvFile };

const envFuncs = {
    config,
    loadEnvConfig,
    readEnvFile,
};

export default envFuncs;
