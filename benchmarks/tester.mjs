import { performance, PerformanceObserver } from 'node:perf_hooks';
import { readFileSync, writeFileSync } from 'fs';
import readline from 'readline';
import { join } from 'path';
import groupBy from 'lodash.groupby';

const terminal = process.stdout;

/**
 * Creates a new test with the `testName`.
 * @class
 */
class Test {
    /** @constructs
     * @param {string} testName - name of test
     * @param {number} runs - times to run each function
     */
    constructor(testName, runs) {
        this.oldenvs = Object.create(process.env);
        this.resultFile = join(process.cwd(), "result.json");
        this.runs = runs || 10;
        this.iterations = [];
        this.save = false;
        this.testName = testName;
        this.testResults = {};
        this.testRuns = [];
        this.testRunNum = 0;
        this.testType = '';
        this.obs = new PerformanceObserver(list => {
            const entries = list.getEntries();
            const groupedEntries = groupBy(entries, 'name');
            Object.keys(groupedEntries).forEach(key => {
                const runtimes = groupedEntries[key].map(({ duration }) => Math.ceil((duration / 1000) * 1000) / 1000);
                const fastest = Array.from(runtimes).sort().splice(0, Math.min(this.runs, 3));
                const avg = parseFloat((fastest.reduce((a, b) => a + b, 0) / Math.min(this.runs, 3)).toFixed(3));

                runtimes.forEach((duration, run) => {
                    this.log(key, run + 1, duration);
                })

                this.testResults[key] = {
                    date: new Intl.DateTimeFormat(
                        'en-US',
                        {
                            dateStyle: 'full',
                            timeStyle: 'medium',
                            timeZone: 'America/Los_Angeles'
                        }
                    ).format(new Date()),
                    runtimes,
                    fastest,
                    avg
                }
            });

            performance.clearMarks();
            performance.clearMeasures();
            this.obs.disconnect();
            console.table(this.testResults);
            if (this.save) this.writeResultsToFile();
        });
        this.obs.observe({ entryTypes: ["measure"] });
        terminal.write("Tests will print final results once all the runs have completed.\n");
    }

    /**
     * Clears the stdout.
     */
    // clearTerminal() {
    //     terminal.write("\n".repeat(terminal.rows));
    //     readline.cursorTo(terminal, 0, 0);
    //     readline.clearScreenDown(terminal);
    // }

    /**
     * Prints a message to the stdout's `stdout`.
     * @function
     * @param {string} testType - current test run
     * @param {number} run - current test run
     * @param {number} timeElapsed - time elapsed during run
     */
    log(testType, run, duration) {
        // terminal.clearLine(1);
        // terminal.cursorTo(0);
        terminal.write(
            `\x1b[32m[${this.testName}] test name: ${testType} | run: ${run}/${this.runs
            } | iterations: ${this.iterations[this.testRunNum - 1]
            } | duration: ${duration}s \x1b[0m\n`
        );
    }

    /**
     * Runs a test function according to `this.runs` and the number of `this.iterations`.
     * @param {Object} args
     * @param {string} args.testType - string name of test type
     * @param {function} args.testFn - the test function to run
     */
    start({ testType, testFn }) {
        this.testType = testType;

        for (let r = 1; r <= this.runs; ++r) {
            terminal.write(`\nRunning ${testType} run ${r}...\n`);
            performance.mark('start-test');

            for (let i = 0; i < this.iterations[this.testRunNum - 1]; ++i) {
                process.env = this.oldenvs;
                testFn();
            }

            performance.mark('end-test');
            performance.measure(testType, {
                detail: {
                    run: r
                },
                start: 'start-test',
                end: 'end-test'
            });
            terminal.write("Completed.\n")
        }
    }

    /**
     * Adds a test function to `this.testRuns`.
     * @param {string} testType - string name of test type
     * @param {function} testFn - the test function to run
     * @param {number} iterations - the number of times to run the function 
     * @example ```test.add("test", () => {}, 1000);```
     */
    add(testType, testFn, iterations) {
        this.testRuns.push({ testType, testFn });
        this.iterations.push(iterations);
        return this;
    }

    /**
     * Runs added tests.
     */
    run() {
        for (let runNum = 1; runNum < this.testRuns.length + 1; runNum += 1) {
            this.testRunNum = runNum;
            this.start(this.testRuns[this.testRunNum - 1]);
        }
        return this;
    }

    /**
     * Will save the collected test runs to the `result.json` file.
     */
    saveResults() {
        this.save = true;
        return this;
    }

    /**
     * Reads the result file.
     * @returns the read result of the JSON file
     */
    readResultFile() {
        try {
            const file = readFileSync(this.resultFile, { encoding: "utf-8" });

            return JSON.parse(file);
        } catch (error) {
            return {};
        }
    }

    /**
     * Writes the collected test runs from `this.result` to the result file.
     */
    writeResultsToFile() {
        try {
            const combinedFile = this.readResultFile();

            Object.assign(combinedFile, {
                [this.testName]: this.testResults
            });

            writeFileSync(this.resultFile, JSON.stringify(combinedFile, null, 2), {
                encoding: "utf-8"
            });

            // this.clearTerminal();
            console.table(this.testResults);

            process.exit(0);
        } catch (error) {
            console.error(`\r\x1b[31m${error}\x1b[0m`);
            process.exit(1);
        }
    }
}

export default Test;
