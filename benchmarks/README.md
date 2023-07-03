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

**Compiled Timestamp**: Monday, July 3, 2023 at 2:50:02 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Monday, July 3, 2023 at 2:49:47 PM | 500000 | 8.628s, 8.846s, 8.866s | 8.78s | 100.00% |
| dotenv | Sunday, June 11, 2023 at 3:40:57 PM | 500000 | 14.54s, 14.585s, 14.638s | 14.588s | 59.34% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Monday, July 3, 2023 at 2:49:47 PM | 5000 | 17.218s, 17.353s, 17.373s | 17.315s | 100.00% |
| dotenv | Sunday, June 11, 2023 at 3:40:57 PM | 5000 | 74.317s, 74.373s, 74.414s | 74.368s | 23.17% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Monday, July 3, 2023 at 2:49:47 PM | 500000 | 16.037s, 16.362s, 16.672s | 16.357s | 100.00% |
| dotenv | Sunday, June 11, 2023 at 3:40:57 PM | 500000 | 26.254s, 26.281s, 26.284s | 26.273s | 61.08% |
