import assert from 'node:assert';
import { describe, it } from 'node:test';
import loadEnvConfig from '../src/loadEnvConfig';

describe('Load Env Config', () => {
    it('fails to load a config file from an invalid directory', () => {
        const envConfig = loadEnvConfig('dev', 'invalid-directory');

        assert.equal(envConfig, undefined);
    });

    it('fails to load a config file from an undefined environment', () => {
        const envConfig = loadEnvConfig('dev', 'tests');

        assert.equal(envConfig, undefined);
    });

    it('loads the config file at the root and returns an environment of config arguments', () => {
        const envConfig = loadEnvConfig('test');

        assert.deepEqual(envConfig, {
            debug: true,
            dir: 'tests/envs',
            files: ['.env.base'],
        });
    });

    it('loads a config file at a specified directory and returns an environment of config arguments', () => {
        const envConfig = loadEnvConfig('test', "tests");

        assert.deepEqual(envConfig, {
            debug: true,
            dir: 'tests/envs',
            files: ['.env.base'],
        });
    });
});
