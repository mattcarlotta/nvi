# Env Metrics

## Commands

Run individual tests by running the following commands:

| `yarn <command>` | Description                                                                                     |
| ---------------- | ----------------------------------------------------------------------------------------------- |
| `dotenv`         | Runs tests for `dotenv` + `dotenv-expand` (outputs results to `results.json` file as `dotenv`). |
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

**Compiled Timestamp**: Sunday, July 9, 2023 at 3:10:46 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Sunday, July 9, 2023 at 2:56:56 PM | 500000 | 8.772s, 8.83s, 8.897s | 8.833s | 100.00% |
| dotenv | Sunday, July 9, 2023 at 3:10:29 PM | 500000 | 14.728s, 14.908s, 15.025s | 14.887s | 59.56% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Sunday, July 9, 2023 at 2:56:56 PM | 5000 | 17.334s, 17.894s, 18.078s | 17.769s | 100.00% |
| dotenv | Sunday, July 9, 2023 at 3:10:29 PM | 5000 | 75.44s, 75.475s, 75.537s | 75.484s | 22.98% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Sunday, July 9, 2023 at 2:56:56 PM | 500000 | 16.213s, 16.317s, 16.337s | 16.289s | 100.00% |
| dotenv | Sunday, July 9, 2023 at 3:10:29 PM | 500000 | 26.374s, 26.534s, 26.582s | 26.497s | 61.47% |
