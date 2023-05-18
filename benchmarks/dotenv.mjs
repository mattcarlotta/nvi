import dotenv from 'dotenv';
import dotenvExpand from 'dotenv-expand';

import Tester from './tester.mjs';

const tester = new Tester('dotenv', 6);

tester
    .add('single', () => {
        dotenvExpand.expand(dotenv.config());
    }, 500000)
    .add("interpolated", () => {
        // large interpolated .env loading
        dotenvExpand.expand(dotenv.config({ path: ".env.interp" }));
    }, 5000)
    .add("multiple", () => {
        // loading multiple .env files (.env, .env.development, .env.local, .env.development.local)
        dotenvExpand.expand(dotenv.config({ path: ".env" }));
        dotenvExpand.expand(dotenv.config({ path: ".env.development" }));
        dotenvExpand.expand(dotenv.config({ path: ".env.local" }));
        dotenvExpand.expand(dotenv.config({ path: ".env.development.local" }));
    }, 500000)
    .run()
    .saveResults()
