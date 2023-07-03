import assert from 'node:assert';
import { describe, it } from 'node:test';
import EnvConfigLoader from '../src/configLoader';

describe('Env Config Loader', () => {
    it('fails to load a config file from an invalid directory', () => {
        const envConfig = new EnvConfigLoader('dev', 'invalid-directory').getOptions();

        assert.deepEqual(envConfig, {
            debug: false,
            dir: '',
            files: [],
            override: false,
            required: [],
        });
    });

    it('fails to load a config file from an undefined environment', () => {
        const envConfig = new EnvConfigLoader('dev', 'tests').getOptions();

        assert.deepEqual(envConfig, {
            debug: false,
            dir: '',
            files: [],
            override: false,
            required: [],
        });
    });

    it('loads the config file at the root and returns an environment of config arguments', () => {
        const envConfig = new EnvConfigLoader('test').getOptions();

        assert.deepEqual(envConfig, {
            debug: true,
            dir: 'tests/envs',
            files: ['.env.base'],
            override: false,
            required: [],
        });
    });

    it('loads a config file at a specified directory and returns an environment of config arguments', () => {
        const envConfig = new EnvConfigLoader('test', 'tests').getOptions();

        assert.deepEqual(envConfig, {
            debug: true,
            dir: 'tests/envs',
            files: ['.env.base'],
            override: false,
            required: [],
        });
    });
});
