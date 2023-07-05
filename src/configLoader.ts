import type { ConfigOptions } from './index';
import { readFileSync } from 'fs';
import { join } from 'path';
import { cwd } from 'process';

const CONFIG_LOADER_FILE_ERROR = 0;
const CONFIG_LOADER_FILE_PARSE_ERROR = 1;
const CONFIG_LOADER_FILE_ENV_NOT_FOUND_ERROR = 2;
const CONFIG_LOADER_DEBUG = 3;
const CONFIG_LOADER_MISSING_FILES_ARG_ERROR = 4;

export default class EnvConfigLoader {
    private env: string;
    private configFile: string;
    private filePath: string;
    private parsedConfig: Record<string, string>;
    private debug: boolean;
    private dir: string;
    private files: string[] = [];
    private requiredEnvs: string[];
    private override: boolean;

    /**
     * Reads an env.config.json configuration file from the project root directory or a specified directory.
     *
     * @param env - name of an env configuration within env.config.json
     * @param dir - optional absolute or relative string path for where the env.config.json file is located
     *
     * @example Reading an env config from within env.config.json
     * ```ts
     * const config = new EnvConfigLoader("dev");
     * ```
     */
    constructor(env?: string, dir?: string) {
        this.env = env || '';
        this.filePath = join(dir || cwd(), 'env.config.json');
        this.configFile = '';
        this.parsedConfig = {};

        try {
            this.configFile = readFileSync(join(dir || cwd(), 'env.config.json'), {
                encoding: 'utf-8',
            });

            this.parsedConfig = JSON.parse(this.configFile);
        } catch (error: any) {
            if (error?.message?.includes('ENOENT')) {
                this.log(CONFIG_LOADER_FILE_ERROR);
            } else {
                this.log(CONFIG_LOADER_FILE_PARSE_ERROR);
            }

            this.exitProcess();
        }

        if (!this.parsedConfig?.[this.env]) {
            this.log(CONFIG_LOADER_FILE_ENV_NOT_FOUND_ERROR);
            this.exitProcess();
        }

        const config = this.parsedConfig[this.env] as ConfigOptions;
        if (config?.files?.length) {
            this.files = config.files;
        } else {
            this.log(CONFIG_LOADER_MISSING_FILES_ARG_ERROR);
            this.exitProcess();
        }

        this.debug = config?.debug || false;

        this.dir = config?.dir || '';

        this.override = config?.override || false;

        this.requiredEnvs = config?.required || [];

        if (this.debug) {
            this.log(CONFIG_LOADER_DEBUG);
        }
    }

    /**
     * Returns `ConfigOptions` object with `debug`, `dir`, `files`, `override`, and `required`.
     *
     * @example Reading and retrieving env config options from env.config.json
     * ```ts
     * const config = new EnvConfigLoader("dev");
     * const options = config.getOptions(); // { debug, dir, files, override, required }
     * ```
     */
    public getOptions(): ConfigOptions {
        return {
            debug: this.debug,
            dir: this.dir,
            files: this.files,
            override: this.override,
            required: this.requiredEnvs,
        };
    }

    private exitProcess(): void {
        if (process.env.NODE_ENV !== 'test') {
            process.exit(1);
        }
    }

    private log(code: number): void {
        switch (code) {
            case CONFIG_LOADER_FILE_ERROR: {
                console.log(
                    `[nvi] (config_loader::FILE_ERROR) Unable to locate '${this.filePath}'. The configuration file doesn't appear to exist!`
                );
                break;
            }
            case CONFIG_LOADER_FILE_PARSE_ERROR: {
                console.log(
                    `[nvi] (config_loader::FILE_PARSE_ERROR) Unable to parse the env.config.json configuration file (${this.filePath}). Ensure the configuration file is valid JSON!`
                );
                break;
            }
            case CONFIG_LOADER_FILE_ENV_NOT_FOUND_ERROR: {
                console.log(
                    `[nvi] (config_loader::FILE_ENV_NOT_FOUND_ERROR) Unable to load a '${this.env}' environment from the env.config.json configuration file (${this.filePath}). The specified environment doesn't appear to exist!`
                );
                break;
            }
            case CONFIG_LOADER_MISSING_FILES_ARG_ERROR: {
                console.log(
                    `[nvi] (config_loader::MISSING_FILES_ARG_ERROR) Unable to locate a 'files' property within the '${this.env}' environment configuration (${this.filePath})). You must specify at least 1 .env file to load!`
                );
                break;
            }
            case CONFIG_LOADER_DEBUG: {
                console.log(
                    `[nvi] (config_loader::DEBUG) Parsed the following keys from the env.config.json configuration file: '${Object.keys(
                        this.parsedConfig
                    ).join(',')}' and selected the '${this.env}' configuration.`
                );
                const options = Object.entries(this.getOptions())
                    .map(
                        ([key, value]) =>
                            `${key}='${Array.isArray(value) ? value.join(', ') : value}'`
                    )
                    .join(', ');
                console.log(
                    `[nvi] (config_loader::DEBUG) The following '${this.env}' configuration settings were set: ${options}.\n`
                );
                break;
            }
            default:
                break;
        }
    }
}
