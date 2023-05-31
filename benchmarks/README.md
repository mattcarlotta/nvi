# Env Metrics

## Commands

Run individual tests by running the following commands:

| `yarn <command>` | Description                                                                                     |
| ---------------- | ----------------------------------------------------------------------------------------------- |
| `dotenv`         | Runs tests for `dotenv` + `dotenv-expand` (outputs results to `results.json` file as `dotenv`). |
| `noshot`         | Runs tests for `noshot` (outputs results to `results.json` file as `@noshot/env`).                |
| `nvi`            | Runs tests for `nvi` (outputs results to `results.json` file as `nvi`).                       |
| `generate:readme`| Generates a `README.md` from the `results.json` file.                                           |

⚠️ Warning: The tests can take a quite a long time to complete! Adjust the iterations or runs if needed.


## Metrics

**System Specs**:

- CPU: AMD Ryzen 9 5950x (stock)
- MOBO: Asus x570 ROG Crosshair Dark Hero
- RAM: G.Skill Trident Z Neo 32 GB (4 x 8 GB) running @ 2100Mhz
- Storage: Sabrent 1TB Rocket 4 Plus NVMe 4.0 Gen4
- OS: Linuxmint 20.2 ulyssa
- Kernel: Linux 5.13.0-23-generic x86_64

**Compiled Timestamp**: Tuesday, May 30, 2023 at 7:18:58 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Tuesday, May 30, 2023 at 7:12:56 PM | 500000 | 5.471s, 5.472s, 5.482s | 5.475s | 85.98% |
| @noshot/env | Tuesday, May 30, 2023 at 7:17:04 PM | 500000 | 4.704s, 4.709s, 4.728s | 4.714s | 100.00% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 500000 | 10.044s, 10.068s, 10.464s | 10.192s | 46.83% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Tuesday, May 30, 2023 at 7:12:56 PM | 5000 | 25.061s, 25.094s, 25.144s | 25.1s | 81.13% |
| @noshot/env | Tuesday, May 30, 2023 at 7:17:04 PM | 5000 | 20.332s, 20.485s, 20.503s | 20.44s | 100.00% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 5000 | 92.559s, 93.278s, 93.284s | 93.04s | 21.97% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Tuesday, May 30, 2023 at 7:12:56 PM | 500000 | 11.152s, 11.163s, 11.195s | 11.17s | 100.00% |
| @noshot/env | Tuesday, May 30, 2023 at 7:17:04 PM | 500000 | 13.592s, 13.691s, 13.779s | 13.687s | 82.05% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 500000 | 20.205s, 20.39s, 20.419s | 20.338s | 55.19% |
