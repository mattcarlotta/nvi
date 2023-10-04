# NVI Man Documentation

This documentation has been pre-built and exists [here](nvi.1). 

# Usage

```bash
man ./nvi.1
```

# System Installation

You can either set the [-DINSTALL_MAN_DIR cmake flag](https://github.com/mattcarlotta/nvi/wiki/Quick-Installation#custom-cmake-compile-flags) before running the install script.

OR

Locate directory paths that `man` searches within by running one of the commands below (directories are separated by colons `:`):

GNU/Linux:
```bash
man --path
```
MAC OS:
```bash
cat /private/etc/manpaths
```

Select a directory path and run the following command below (must be located within this `/docs` directory):

GNU/LINUX (typically installed in `/usr/share/man/man1`)
MAC OS (typically installed in `/usr/local/share/man/man1`)
```bash
sudo cp nvi.1 <MANPATH_DIRECTORY>
```

To view installed documentation:
```bash
man nvi
```

# Uninstallation

```bash
sudo rm <MANPATH_DIRECTORY>/nvi.1
```

# Build
### Requirements
- pandoc v3.1.6+ (click [here](https://pandoc.org/MANUAL.html) for documentation)

To manually generate the man documentation, run the following command:
```bash
pandoc --standalone --to man nvi.1.md -o nvi.1
```
