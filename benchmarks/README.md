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

**Compiled Timestamp**: Wednesday, June 7, 2023 at 9:47:21 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Wednesday, June 7, 2023 at 9:09:13 PM | 500000 | 8.615s, 8.812s, 8.884s | 8.77s | 100.00% |
| @noshot/env | Wednesday, June 7, 2023 at 9:25:36 PM | 500000 | 11.933s, 12.02s, 12.052s | 12.002s | 72.19% |
| dotenv | Wednesday, June 7, 2023 at 9:47:07 PM | 500000 | 14.617s, 14.834s, 14.913s | 14.788s | 58.94% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Wednesday, June 7, 2023 at 9:09:13 PM | 5000 | 35.469s, 35.472s, 36.097s | 35.679s | 100.00% |
| @noshot/env | Wednesday, June 7, 2023 at 9:25:36 PM | 5000 | 119.848s, 121.108s, 121.177s | 120.711s | 29.59% |
| dotenv | Wednesday, June 7, 2023 at 9:47:07 PM | 5000 | 164.952s, 165.842s, 166.103s | 165.632s | 21.50% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Wednesday, June 7, 2023 at 9:09:13 PM | 500000 | 16.634s, 16.695s, 16.735s | 16.688s | 100.00% |
| @noshot/env | Wednesday, June 7, 2023 at 9:25:36 PM | 500000 | 24.641s, 24.81s, 24.879s | 24.777s | 67.51% |
| dotenv | Wednesday, June 7, 2023 at 9:47:07 PM | 500000 | 26.753s, 27.065s, 27.102s | 26.973s | 62.18% |
