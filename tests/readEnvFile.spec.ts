import assert from 'node:assert';
import { describe, it } from 'node:test';
import readEnvFile from '../src/readEnvFile';

describe('Read Env File', () => {
    const envMap = readEnvFile('.env', { debug: true, dir: 'tests/envs', encoding: 'utf8', override: true });
    if (!envMap) {
        assert.fail('Unable to locate the .env file within tests/envs.')
    }

    it('returns a Map of Envs', () => {
        assert.deepEqual(envMap.size, 34);
    });

    it('sets basic attributes', () => {
        const basicEnv = envMap.get('BASIC_ENV');
        assert.deepEqual(basicEnv, 'true');
    });

    it('defaults empty values to empty string', () => {
        const noValue = envMap.get('NO_VALUE')
        assert.deepEqual(noValue, '');
    });

    it('parses multi values', () => {
        const multiLineValue = envMap.get('MULTI_LINE_KEY');
        assert.deepEqual(multiLineValue, 'ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com');
    });

    it('parsed values with hashes', () => {
        const hashKeys = envMap.get('HASH_KEYS');
        assert.deepEqual(hashKeys, '1#2#3#4#');
    });

    it('parses values with just hashes', () => {
        const justHashes = envMap.get('JUST_HASHES');
        assert.deepEqual(justHashes, '#####');
    });

    it('parses values with just dollar signs', () => {
        const justDollars = envMap.get('JUST_DOLLARS');
        assert.deepEqual(justDollars, '$$$$$');
    });

    it('parses values with just braces', () => {
        const justBraces = envMap.get('JUST_BRACES');
        assert.deepEqual(justBraces, '{{{{}}}}');
    });

    it('parses values with just spaces', () => {
        const justSpaces = envMap.get('JUST_SPACES');
        assert.deepEqual(justSpaces, '          ');
    });

    it('parses sentence values', () => {
        const sentence = envMap.get('SENTENCE');
        assert.deepEqual(sentence, 'chat gippity is just a junior engineer that copies/pastes from StackOverflow');
    });

    it('parses interpolated values from previous keys', () => {
        const interp = envMap.get('INTERP');
        assert.deepEqual(interp, 'aatruecdhelloef');
    });

    it('parses interpolated values from process.env keys', () => {
        const interpFromProcess = envMap.get('INTERP_ENV_FROM_PROCESS');
        assert.ok(interpFromProcess);

        const nodeEnv = envMap.get('ENVIRO');
        assert.deepEqual(nodeEnv, 'test');
    });

    it('parses and writes values to an already defined process.env key', () => {
        const overrideProcessEnv = envMap.get('USER');
        assert.deepEqual(overrideProcessEnv, 'HAXOR');
    });

    it('retains values despite an invalid interpolated value', () => {
        const emptyInterpKey = envMap.get('EMPTY_INTRP_KEY');
        assert.deepEqual(emptyInterpKey, 'abc123');
    });

    it('defaults invalid interpolated values to an empty string', () => {
        const noInterp = envMap.get('NO_INTERP');
        assert.deepEqual(noInterp, '');
    });

    it('defaults invalid interpolated values to an empty string', () => {
        const noInterp = envMap.get('NO_INTERP');
        assert.deepEqual(noInterp, '');
    });

    it('discards invalid interpolated values with non-delineated braces', () => {
        const badInterp = envMap.get('BAD_INTERP');
        assert.deepEqual(badInterp, 'lmnotuv');
    });

    it('respects single quotes and spaces', () => {
        const singleQuotes = envMap.get('SINGLE_QUOTES_SPACES');
        assert.deepEqual(singleQuotes, "'  SINGLE QUOTES  '");
    });

    it('respects double quotes and spaces', () => {
        const doubleQuotes = envMap.get('DOUBLE_QUOTES_SPACES');
        assert.deepEqual(doubleQuotes, '"  DOUBLE QUOTES  "');
    });

    it('respects quotes within letters', () => {
        const quotes = envMap.get('QUOTES');
        assert.deepEqual(quotes, 'sad"wow"bak');
    });

    it('handles invalid single quotes within letters', () => {
        const invalidSinglQuotes = envMap.get('INVALID_SINGLE_QUOTES');
        assert.deepEqual(invalidSinglQuotes, "bad'dq");
    });

    it('handles invalid double quotes within letters', () => {
        const invalidDoubleQuotes = envMap.get('INVALID_DOUBLE_QUOTES');
        assert.deepEqual(invalidDoubleQuotes, 'bad"dq');
    });

    it('handles invalid template literals within letters', () => {
        const invalidTemplateLiteral = envMap.get('INVALID_TEMPLATE_LITERAL');
        assert.deepEqual(invalidTemplateLiteral, 'bad`as');
    });

    it('handles invalid mix quotes within letters', () => {
        const invalidMixQuotes = envMap.get('INVALID_MIX_QUOTES');
        assert.deepEqual(invalidMixQuotes, "bad\"'`mq");
    });

    it('handles empty single quotes', () => {
        const emptySingleQuotes = envMap.get('EMPTY_SINGLE_QUOTES');
        assert.deepEqual(emptySingleQuotes, "''");
    });

    it('handles empty double quotes', () => {
        const emptyDoubleQuotes = envMap.get('EMPTY_DOUBLE_QUOTES');
        assert.deepEqual(emptyDoubleQuotes, '""');
    });

    it('handles empty template literals', () => {
        const emptyTemplateLiteral = envMap.get('EMPTY_TEMPLATE_LITERAL');
        assert.deepEqual(emptyTemplateLiteral, '``');
    });

    it('overrides previous values in env file', () => {
        const overwritten = envMap.get('OVERWRITTEN');
        assert.deepEqual(overwritten, 'true');
    });

    it('concatenates values from previous keys in file', () => {
        const mongolabURI = envMap.get('MONGOLAB_URI');
        assert.deepEqual(mongolabURI, 'mongodb://root:password@abcd1234.mongolab.com:12345/localhost');
    });
});
