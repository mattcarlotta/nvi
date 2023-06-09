import process from 'process';
import assert from 'node:assert';
import { describe, it } from 'node:test';

describe('Binary Parser', () => {
    it('parses an .env file and returns a parsable JSON string', async () => {
        let data = '';
        for await (const chunk of process.stdin) data += chunk;
        assert.ok(data);
        const ENVS = JSON.parse(data);

        assert.deepEqual(ENVS['BASIC_ENV'], 'true');
        assert.deepEqual(ENVS['MULTI_LINE_KEY'], 'ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com');
        assert.deepEqual(ENVS['MONGOLAB_URI'], 'mongodb://root:password@abcd1234.mongolab.com:12345/localhost');
    });
});
