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

**Compiled Timestamp**: Thursday, June 8, 2023 at 1:18:40 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Thursday, June 8, 2023 at 12:56:21 PM | 500000 | 8.843s, 8.91s, 8.925s | 8.893s | 100.00% |
| @noshot/env | Thursday, June 8, 2023 at 1:06:23 PM | 500000 | 12.53s, 12.56s, 12.752s | 12.614s | 70.57% |
| dotenv | Thursday, June 8, 2023 at 1:18:19 PM | 500000 | 15.149s, 15.23s, 15.289s | 15.223s | 58.37% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Thursday, June 8, 2023 at 12:56:21 PM | 5000 | 17.462s, 17.672s, 17.711s | 17.615s | 100.00% |
| @noshot/env | Thursday, June 8, 2023 at 1:06:23 PM | 5000 | 60.258s, 60.614s, 60.706s | 60.526s | 28.98% |
| dotenv | Thursday, June 8, 2023 at 1:18:19 PM | 5000 | 74.339s, 74.762s, 75.4s | 74.834s | 23.49% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Thursday, June 8, 2023 at 12:56:21 PM | 500000 | 16.639s, 16.706s, 16.767s | 16.704s | 100.00% |
| @noshot/env | Thursday, June 8, 2023 at 1:06:23 PM | 500000 | 25.169s, 25.288s, 25.298s | 25.252s | 66.11% |
| dotenv | Thursday, June 8, 2023 at 1:18:19 PM | 500000 | 26.774s, 26.959s, 27.051s | 26.928s | 62.15% |
