/**
 * A utility function that logs a debug message.
 *
 * @param message
 */
export function logInfo(message: string, file?: string, line?: number): void {
    console.log(`\x1b[32m[nvi${file && line ? `::${file}::${line}` : ''}] INFO: ${message}\x1b[0m`);
}

/**
 * A utility function that logs a debug message.
 *
 * @param message
 */
export function logMessage(message: string): void {
    console.log(`\x1b[90m[nvi]: ${message}\x1b[0m`);
}

/**
 * A utility function that logs a warning message.
 *
 * @param message
 */
export function logWarning(message: string, file?: string, line?: number): void {
    console.log(`\x1b[33m[nvi${file && line ? `::${file}::${line}` : ''}] WARNING: ${message}\x1b[0m`);
}

/**
 * A utility function that logs an error message.
 *
 * @param message
 * @throws an error message
 */
export function logError(message: string, file?: string, line?: number): void {
    console.log(`\x1b[31m[nvi${file && line ? `::${file}::${line}` : ''}] ERROR: ${message}\x1b[0m`);
    if (process.env.NODE_ENV !== 'test') process.exit(1);
}
