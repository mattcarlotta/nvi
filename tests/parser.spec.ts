import assert from 'node:assert';
import { describe, it } from 'node:test';
import EnvParser from '../src/parser';

describe('Env Parser', () => {
    const parser = new EnvParser({
        debug: true,
        dir: 'tests/envs',
        files: ['.env'],
        override: true,
    });
    const envMap = parser.parseEnvs().getEnvs();

    if (!envMap) {
        assert.fail('Unable to locate the .env file within tests/envs.');
    }

    it('returns an Object of ENVs', () => {
        assert.deepEqual(Object.keys(envMap).length, 35);
    });

    it('sets basic attributes', () => {
        assert.deepEqual(envMap['BASIC_ENV'], 'true');
    });

    it('defaults empty values to empty string', () => {
        assert.deepEqual(envMap['NO_VALUE'], '');
    });

    it('parses multi values', () => {
        assert.deepEqual(
            envMap['MULTI_LINE_KEY'],
            'ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com'
        );
    });

    it('parsed values with hashes', () => {
        assert.deepEqual(envMap['HASH_KEYS'], '1#2#3#4#');
    });

    it('parses values with just hashes', () => {
        assert.deepEqual(envMap['JUST_HASHES'], '#####');
    });

    it('parses values with just dollar signs', () => {
        assert.deepEqual(envMap['JUST_DOLLARS'], '$$$$$');
    });

    it('parses values with just braces', () => {
        assert.deepEqual(envMap['JUST_BRACES'], '{{{{}}}}');
    });

    it('parses values with just spaces', () => {
        assert.deepEqual(envMap['JUST_SPACES'], '          ');
    });

    it('parses sentence values', () => {
        assert.deepEqual(
            envMap['SENTENCE'],
            'chat gippity is just a junior engineer that copies/pastes from StackOverflow'
        );
    });

    it('parses interpolated values from previous keys', () => {
        assert.deepEqual(envMap['INTERP'], 'aatruecdhelloef');
    });

    it('parses interpolated values from process.env keys', () => {
        assert.ok(envMap['INTERP_ENV_FROM_PROCESS']);

        assert.deepEqual(envMap['ENVIRO'], 'test');
    });

    it('parses and writes values to an already defined process.env key', () => {
        assert.deepEqual(envMap['USER'], 'HAXOR');
    });

    it('retains values despite an invalid interpolated value', () => {
        assert.deepEqual(envMap['EMPTY_INTRP_KEY'], 'abc123');
    });

    it('retains valid interpolated values but ignores invalid interpolated value', () => {
        assert.deepEqual(envMap['CONTAINS_INVALID_INTRP_KEY'], 'hello');
    });

    it('defaults invalid interpolated values to an empty string', () => {
        assert.deepEqual(envMap['NO_INTERP'], '');
    });

    it('defaults invalid interpolated values to an empty string', () => {
        assert.deepEqual(envMap['NO_INTERP'], '');
    });

    it('discards invalid interpolated values with non-delineated braces', () => {
        assert.deepEqual(envMap['BAD_INTERP'], 'lmnotuv');
    });

    it('respects single quotes and spaces', () => {
        assert.deepEqual(envMap['SINGLE_QUOTES_SPACES'], "'  SINGLE QUOTES  '");
    });

    it('respects double quotes and spaces', () => {
        assert.deepEqual(envMap['DOUBLE_QUOTES_SPACES'], '"  DOUBLE QUOTES  "');
    });

    it('respects quotes within letters', () => {
        assert.deepEqual(envMap['QUOTES'], 'sad"wow"bak');
    });

    it('handles invalid single quotes within letters', () => {
        assert.deepEqual(envMap['INVALID_SINGLE_QUOTES'], "bad'dq");
    });

    it('handles invalid double quotes within letters', () => {
        assert.deepEqual(envMap['INVALID_DOUBLE_QUOTES'], 'bad"dq');
    });

    it('handles invalid template literals within letters', () => {
        assert.deepEqual(envMap['INVALID_TEMPLATE_LITERAL'], 'bad`as');
    });

    it('handles invalid mix quotes within letters', () => {
        assert.deepEqual(envMap['INVALID_MIX_QUOTES'], 'bad"\'`mq');
    });

    it('handles empty single quotes', () => {
        assert.deepEqual(envMap['EMPTY_SINGLE_QUOTES'], "''");
    });

    it('handles empty double quotes', () => {
        assert.deepEqual(envMap['EMPTY_DOUBLE_QUOTES'], '""');
    });

    it('handles empty template literals', () => {
        assert.deepEqual(envMap['EMPTY_TEMPLATE_LITERAL'], '``');
    });

    it('overrides previous values in env file', () => {
        assert.deepEqual(envMap['OVERWRITTEN'], 'true');
    });

    it('concatenates values from previous keys in file', () => {
        assert.deepEqual(
            envMap['MONGOLAB_URI'],
            'mongodb://root:password@abcd1234.mongolab.com:12345/localhost'
        );
    });
});
