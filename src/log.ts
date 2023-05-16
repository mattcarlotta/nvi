import { env } from 'process';

/**
 * A utility function that logs a debug message.
 *
 * @param message
 */
export function logInfo(message: string, file?: string, line?: number): void {
    console.log(
        `\x1b[32m[nvi${file ? `::${file}` : ''}${line ? `::${line}` : ''}] INFO: ${message}\x1b[0m`
    );
}

/**
 * A utility function that logs a warning message.
 *
 * @param message
 */
export function logWarning(message: string, file?: string, line?: number): void {
    console.log(
        `\x1b[33m[nvi${file ? `::${file}` : ''}${
            line ? `::${line}` : ''
        }] WARNING: ${message}\x1b[0m`
    );
}

/**
 * A utility function that logs an error message.
 *
 * @param message
 * @throws an error message
 */
export function logError(message: string, file?: string, line?: number): void {
    console.log(
        `\x1b[31m[nvi${file ? `::${file}` : ''}${line ? `::${line}` : ''}] ERROR: ${message}\x1b[0m`
    );
    if (env.NODE_ENV !== 'test') process.exit(1);
}
