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

**Compiled Timestamp**: Tuesday, May 30, 2023 at 5:10:11 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Tuesday, May 30, 2023 at 5:05:12 PM | 500000 | 5.625s, 5.725s, 5.761s | 5.704s | 83.34% |
| @noshot/env | Tuesday, May 30, 2023 at 5:09:24 PM | 500000 | 4.688s, 4.699s, 4.706s | 4.698s | 100.00% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 500000 | 10.044s, 10.068s, 10.464s | 10.192s | 46.67% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Tuesday, May 30, 2023 at 5:05:12 PM | 5000 | 27.299s, 27.329s, 27.37s | 27.333s | 78.96% |
| @noshot/env | Tuesday, May 30, 2023 at 5:09:24 PM | 5000 | 21.554s, 22.012s, 22.171s | 21.912s | 100.00% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 5000 | 92.559s, 93.278s, 93.284s | 93.04s | 23.29% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Tuesday, May 30, 2023 at 5:05:12 PM | 500000 | 11.323s, 11.328s, 11.338s | 11.33s | 100.00% |
| @noshot/env | Tuesday, May 30, 2023 at 5:09:24 PM | 500000 | 13.516s, 13.545s, 13.57s | 13.544s | 83.77% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 500000 | 20.205s, 20.39s, 20.419s | 20.338s | 56.04% |
