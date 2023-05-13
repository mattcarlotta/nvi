import config from './config';
import loadEnvConfig from './loadEnvConfig';
import readEnvFile from './readEnvFile';

export type ParsedEnvs = Map<string, string>;

export type Option = boolean | string | undefined;

export type Path = string | string[];

export type ConfigOptions = {
    dir?: string,
    paths?: Path,
    encoding?: BufferEncoding,
    override?: Option,
    debug?: Option,
    required?: string[]
}

/**
 * Immediately loads an env.config.json file when the package is preloaded or imported.
 */
(function(): void {
    const { LOAD_CONFIG } = process.env;

    // checks if LOAD_CONFIG is defined and assigns config options
    if (LOAD_CONFIG) {
        const envConfig = loadEnvConfig(LOAD_CONFIG);

        if (envConfig) config(envConfig);

        // prevents the IFFE from reloading the config file
        delete process.env.LOAD_CONFIG;
    }
})();

export { config, loadEnvConfig, readEnvFile };

const env = {
    config,
    loadEnvConfig,
    readEnvFile
};

export default env;
