# NVI Man Documentation

Man documentation nvi CLI tool. 

## Requirements
- pandoc v3.1.6+ (click [here](https://pandoc.org/MANUAL.html) for documentation)

## Manually Build
```bash
git clone git@github.com:mattcarlotta/nvi.git
cd docs 
pandoc --standalone --to man nvi.1.md -o nvi.1
```

Man from local directory:
```bash
man ./nvi.1
```

## System Installation

Locate directory paths that `man` searches within by running one of the commands below (directories are separated by colons `:`):

GNU/LINUX (typically installed in `/usr/share/man`)
```bash
man --path
```
Mac OS (typically installed in `/usr/local/share/man`)
```bash
cat /private/etc/manpaths
```

Select a directory path, add `man1` to the end of the path, and then run the following command below (must be located within this `/docs` directory):
```bash
sudo cp nvi.1 <MANPATH_DIRECTORY/man1>
```

To view installed documentation:
```bash
man nvi
```

## System Uninstallation

```bash
sudo rm <MANPATH_DIRECTORY/man1>/nvi.1
```

