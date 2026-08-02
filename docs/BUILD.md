# Building nvi from source

Building from source gives the best platform compatibility and it's the only option on macOS x86_64 and Linux musl aarch64 (there are no prebuilt binaries for those).

- [POSIX (Linux, macOS, WSL)](#posix-linux-macos-wsl)
- [PowerShell (Windows)](#powershell-windows)
- [Build variants](#build-variants)

Build system:
- [nob.h](https://github.com/tsoding/nob.h)

Optional tooling:
- [Clangd](https://clangd.llvm.org/)
- [Clang Format](https://clang.llvm.org/docs/ClangFormat.html)

## POSIX (Linux, macOS, WSL)

Requirements:
- [Clang](https://clang.llvm.org/) (default), or [GCC](https://gcc.gnu.org/) via `NVI_CC=gcc`
- [LLD](https://lld.llvm.org/) on Linux when building with clang (release builds link with `-fuse-ld=lld`; usually packaged as `lld`; not needed for gcc builds)

Clone the repo and build `nob`:
```sh
cd ~/Downloads

git clone git@github.com:mattcarlotta/nvi.git && cd nvi

clang nob.c
```

Build for debugging (not required):
```sh
./nob
```

Build for release (not required):
```sh
./nob release
```

Pick a destination directory that your shell recognizes and that is owned by `$USER` rather than `root`. See [verifying your PATH](../README.md#verifying-your-path) if you're not sure which directories qualify.

Build and install the release binary into the destination `<DIR>`:
```sh
# install in a directory that is recognized by the shell $PATH
# for example: ./nob install $HOME/.local/bin
./nob install <DIR>
```

Verify the installation:
```sh
which nvi
# <DIR>

nvi version
# nvi <version> (<build_type>)
# commit <commit>
# clang|gcc <version>
# <architecture>
```

## PowerShell (Windows)

Requirements:
- [MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/)
- [Clang for MSVC](https://clang.llvm.org/get_started.html#buildWindows)

1. Install MSVC Build Tools:
```powershell
winget install Microsoft.VisualStudio.2022.BuildTools --source winget
```

> [!NOTE]
> It should open a GUI installer, where you need to select and install the `Desktop development with C++` workload. This gives you the MSVC linker, Windows SDK, and CRT libraries.
> If it closes without the workload installed: Relaunch the `Visual Studio Installer` from the Windows Menu, click on the installed version and click `Modify`,
> then select the `Desktop development with C++` workload, then `Modify` again.

2. Install LLVM/Clang:
```powershell
winget install LLVM.LLVM --source winget
```

3. Add clang to `Path`:
```powershell
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Program Files\LLVM\bin", "User")
```

4. Close and reopen PowerShell.

5. Launch a developer shell (or open `Developer PowerShell for VS 2022` from the Windows Menu):
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64
```

> [!CAUTION]
> Spawning a VS Dev Shell without the `-Arch` and `-HostArch` flags may result in a 32bit (instead of 64bit) shell environment.

6. Clone the repo (assumes `git` is installed, if not then install via: `winget install --id Git.Git -e --source winget`):
```powershell
cd Documents

git clone git@github.com:mattcarlotta/nvi.git

cd nvi
```

7. Set up git tracking (the git commit is used within the output for `nvi version`; otherwise, it'll just report the commit as "unknown"):
```powershell
git init
git remote add origin https://github.com/mattcarlotta/nvi.git
git fetch origin
git reset origin
```

8. Build `nob.c`:
```powershell
cl nob.c
```

Build for debugging (not required):
```powershell
.\nob.exe
```

Build for release (not required):
```powershell
.\nob.exe release
```

Pick a destination directory that PowerShell recognizes. See [verifying your PATH](../README.md#verifying-your-path) if you're not sure.

Build and install a release binary to the destination directory (change `<DIR>` to the destination directory):
```powershell
.\nob.exe install <DIR>
```

Close and reopen PowerShell, then verify the installation:
```powershell
Get-Command nvi

nvi version
```

## Build variants

| Variant | Command | Notes |
| --- | --- | --- |
| clang + glibc | `./nob <release\|install>` | Default. Smallest release binaries. |
| musl (static) | `NVI_LIBC=musl ./nob <release\|install>` | Fully static, portable Linux binary. Requires `musl-tools`. |
| GCC | `NVI_CC=gcc ./nob <cmd>` | Any GCC 11+. A versioned name like `NVI_CC=gcc-14` also works. |

> [!NOTE]
> `NVI_LIBC=musl` takes precedence over `NVI_CC`. GCC release builds use a conservative flag set (no `-flto`/lld pipeline), so clang remains the recommended compiler for the smallest release binaries. Fuzzing always requires clang.
