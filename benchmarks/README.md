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

**Compiled Timestamp**: Tuesday, May 30, 2023 at 1:46:14 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Tuesday, May 30, 2023 at 1:40:45 PM | 500000 | 5.34s, 5.371s, 5.372s | 5.361s | 86.20% |
| @noshot/env | Tuesday, May 30, 2023 at 1:45:12 PM | 500000 | 4.603s, 4.609s, 4.632s | 4.615s | 100.00% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 500000 | 10.044s, 10.068s, 10.464s | 10.192s | 45.83% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Tuesday, May 30, 2023 at 1:40:45 PM | 5000 | 26.734s, 26.797s, 27.239s | 26.923s | 78.41% |
| @noshot/env | Tuesday, May 30, 2023 at 1:45:12 PM | 5000 | 20.963s, 21.112s, 21.172s | 21.082s | 100.00% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 5000 | 92.559s, 93.278s, 93.284s | 93.04s | 22.65% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Tuesday, May 30, 2023 at 1:40:45 PM | 500000 | 10.918s, 10.922s, 10.934s | 10.925s | 100.00% |
| @noshot/env | Tuesday, May 30, 2023 at 1:45:12 PM | 500000 | 13.426s, 13.464s, 13.494s | 13.461s | 81.32% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 500000 | 20.205s, 20.39s, 20.419s | 20.338s | 54.04% |
