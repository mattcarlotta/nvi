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
- OS: Linuxmint 21.1 vera
- Kernel: Linux 5.15.0-73-generic x86_64
- Node: v18.16.0

**Compiled Timestamp**: Wednesday, June 7, 2023 at 4:43:32 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Wednesday, June 7, 2023 at 2:28:42 PM | 1000000 | 5.296s, 5.311s, 5.314s | 5.307s | 100.00% |
| @noshot/env | Wednesday, June 7, 2023 at 2:37:00 PM | 1000000 | 9.172s, 9.242s, 9.268s | 9.227s | 57.74% |
| dotenv | Wednesday, June 7, 2023 at 3:02:08 PM | 1000000 | 20.571s, 20.642s, 20.701s | 20.638s | 25.74% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Wednesday, June 7, 2023 at 2:28:42 PM | 10000 | 12.634s, 12.742s, 12.808s | 12.728s | 100.00% |
| @noshot/env | Wednesday, June 7, 2023 at 2:37:00 PM | 10000 | 43.587s, 43.601s, 43.759s | 43.649s | 28.99% |
| dotenv | Wednesday, June 7, 2023 at 3:02:08 PM | 10000 | 183.641s, 183.809s, 185.249s | 184.233s | 6.88% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Wednesday, June 7, 2023 at 2:28:42 PM | 1000000 | 16.56s, 16.601s, 16.614s | 16.592s | 100.00% |
| @noshot/env | Wednesday, June 7, 2023 at 2:37:00 PM | 1000000 | 27.07s, 27.084s, 27.196s | 27.117s | 61.17% |
| dotenv | Wednesday, June 7, 2023 at 3:02:08 PM | 1000000 | 40.816s, 41.193s, 41.235s | 41.081s | 40.57% |
