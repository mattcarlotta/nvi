import typescript from 'rollup-plugin-typescript2';
import terser from '@rollup/plugin-terser';

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

export default [
    {
        input: ["./src/index.ts"],
        output: [{ dir: '.', format: 'cjs', entryFileNames: '[name].js' }],
        external: ['fs', 'path', 'process'],
        plugins: [typescript({ tsconfig: './tsconfig.json' }), terser(terserOptions)],
    },
    {
        preserveModules: true,
        input: ["./src/index.ts"],
        output: [{ dir: '.', format: 'esm', entryFileNames: '[name].mjs' }],
        external: ['fs', 'path', 'process'],
        plugins: [typescript({ tsconfig: './tsconfig.esm.json' }), terser(terserOptions)],
    },
];
