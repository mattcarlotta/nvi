import { config } from '../index.mjs';
import Tester from './tester.mjs';

const tester = new Tester('nvi', 6);

tester
    .add('single', () => {
        config();
    }, 500000)
    .add('interpolated', () => {
        config({ files: [".env.interp"] });
    }, 5000)
    .add('multiple', () => {
        config({ files: [".env", ".env.development", ".env.local", ".env.development.local"] });
    }, 500000)
    .run()
    .saveResults()
