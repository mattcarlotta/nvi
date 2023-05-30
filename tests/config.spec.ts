import assert from 'node:assert';
import { describe, it } from 'node:test';
import config from '../src/config';

describe('Config Method', () => {
    it('loads a default .env file', () => {
        const envMap = config();
        const message = envMap.get('DEFAULT_ENV');

        assert.deepEqual(message, 'Hi from the root directory');
        assert.deepEqual(process.env.DEFAULT_ENV, message);
    });

    it('accepts a dir argument as a string', () => {
        const envMap = config({ debug: true, dir: 'tests/envs', files: ['directory.env'] });
        const customDirectory = envMap.get('CUSTOM_DIRECTORY');

        assert.deepEqual(customDirectory, 'Hi from a custom directory');
        assert.deepEqual(process.env.CUSTOM_DIRECTORY, customDirectory);
    });


    it('accepts an override argument to write over an env within process.env', () => {
        process.env.PERSON = "Bob";

        const envMap = config({
            debug: true,
            dir: 'tests/envs',
            files: ['override.env'],
            override: true,
        });

        const overridenPerson = envMap.get('PERSON');

        assert.deepEqual(overridenPerson, 'SneakySnake');
        assert.deepEqual(process.env.PERSON, overridenPerson);
    });

    it('fails if non-existent files are loaded', () => {
        const envMap = config({ files: ['.env.invalid'] });

        assert.deepEqual(envMap.size, 0);
    });

    it('accepts a required argument to check for required values from extracted Envs', () => {
        const envMap = config({
            debug: true,
            dir: 'tests/envs',
            files: ['required.env'],
            required: ['REQUIRED']
        });

        assert.deepEqual(envMap.size, 1);
    });

    it('allows Envs to reference previously parsed Envs', () => {
        const envMap = config({
            debug: true,
            dir: 'tests/envs',
            files: ['base.env', 'reference.env'],
        });

        assert.deepEqual(envMap.size, 2);
        assert.deepEqual(envMap.get('BASE'), 'hello');
        assert.deepEqual(envMap.get('REFERENCE'), 'hello');
    });
});
