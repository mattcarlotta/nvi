import { config } from "@noshot/env";
import Tester from './tester.mjs';

const tester = new Tester("@noshot/env", 6);

tester
    .add("single", () => {
        // loading a single default .env
        config();
    }, 500000)
    .add("interpolated", () => {
        // large interpolated .env loading
        config({ paths: [".env.interp"] });
    }, 5000)
    .add("multiple", () => {
        // loading multiple .env files (.env, .env.development, .env.local, .env.development.local)
        config({
            paths: [
                ".env",
                ".env.development",
                ".env.local",
                ".env.development.local"
            ]
        });
    }, 500000)
    .run()
    .saveResults()
