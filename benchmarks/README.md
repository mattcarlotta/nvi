# Env Metrics

## Commands

Run individual tests by running the following commands:

| `yarn <command>` | Description                                                                                     |
| ---------------- | ----------------------------------------------------------------------------------------------- |
| `dotenv`         | Runs tests for `dotenv` + `dotenv-expand` (outputs results to `results.json` file as `dotenv`). |
| `noshot`         | Runs tests for `noshot` (outputs results to `results.json` file as `@noshot/env`).              |
| `nvi`            | Runs tests for `nvi` (outputs results to `results.json` file as `nvi`).                         |
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

**Compiled Timestamp**: Wednesday, May 17, 2023 at 8:58:48 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Wednesday, May 17, 2023 at 8:06:28 PM | 500000 | 8.076s, 8.297s, 8.332s | 8.235s | 54.36% |
| @noshot/env | Wednesday, May 17, 2023 at 8:11:52 PM | 500000 | 4.39s, 4.485s, 4.526s | 4.467s | 100.00% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 500000 | 10.044s, 10.068s, 10.464s | 10.192s | 43.71% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Wednesday, May 17, 2023 at 8:06:28 PM | 5000 | 43.495s, 43.552s, 43.687s | 43.578s | 46.34% |
| @noshot/env | Wednesday, May 17, 2023 at 8:11:52 PM | 5000 | 20.155s, 20.218s, 20.351s | 20.241s | 100.00% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 5000 | 92.559s, 93.278s, 93.284s | 93.04s | 21.78% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Wednesday, May 17, 2023 at 8:06:28 PM | 500000 | 15.226s, 15.391s, 15.686s | 15.434s | 87.25% |
| @noshot/env | Wednesday, May 17, 2023 at 8:11:52 PM | 500000 | 13.285s, 13.291s, 13.301s | 13.292s | 100.00% |
| dotenv | Wednesday, May 17, 2023 at 8:39:08 PM | 500000 | 20.205s, 20.39s, 20.419s | 20.338s | 65.75% |
