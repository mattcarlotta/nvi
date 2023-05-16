import type { MinifyOptions } from 'terser';
import { readFileSync, statSync, writeFileSync } from 'fs';
import { minify } from 'terser';
import { join } from 'path';

const terserOptions = {
    compress: {
        warnings: false,
        comparisons: false,
        inline: 2,
    },
    mangle: {
        safari10: true,
    },
    output: {
        comments: false,
        ascii_only: true,
    },
};

function fileExists(file: string): boolean {
    try {
        return statSync(file).isFile();
    } catch (e) {
        return false;
    }
}

/**
 * A utility function to compress a list of files.
 *
 * @param files - an array of string filenames
 * @param opts - optional terser {@link https://github.com/terser/terser#minify-options `Options`}
 * @returns a promise
 * @example ```await compressFiles(["index.ts", "index.d.ts"], opts);```
 */
async function compressFiles(files: Array<string>, opts?: MinifyOptions): Promise<void> {
    try {
        for (let i = 0; i < files.length; i += 1) {
            const file = files[i];

            const filePath = join(process.cwd(), file);

            if (!fileExists(filePath))
                throw String(`Unable to locate ${file}. The file doesn't appear to exist!`);

            const { code } = await minify(
                readFileSync(filePath, { encoding: 'utf-8' }),
                opts || terserOptions
            );

            if (!code)
                throw String(
                    `Unable to minify ${file}. No minified code was returned from terser!`
                );

            writeFileSync(filePath, code, { encoding: 'utf-8' });
        }
    } catch (error: any) {
        throw Error(error);
    }
}

(async (): Promise<void> => {
    try {
        const files = ['config.js', 'index.js', 'log.js', 'loadEnvConfig.js', 'readEnvFile.js'];

        await compressFiles(files);
        process.exit(0);
    } catch (error: any) {
        console.error(error);
        process.exit(1);
    }
})();
