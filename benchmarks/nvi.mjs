import { EnvParser } from '../index.mjs';
import Tester from './tester.mjs';

const tester = new Tester('nvi', 6);

tester
    .add('single', () => {
        new EnvParser().parseEnvs();
        // loading a single default .env
    }, 500000)
    .add('interpolated', () => {
        // large interpolated .env loading
        new EnvParser({ files: [".env.interp"] }).parseEnvs();
    }, 5000)
    .add('multiple', () => {
        // loading multiple .env files (.env, .env.development, .env.local, .env.development.local)
        new EnvParser({ files: [".env", ".env.development", ".env.local", ".env.development.local"] }).parseEnvs();
    }, 500000)
    .run()
    .saveResults()

