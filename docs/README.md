# NVI Man Documentation

Man documentation for the nvi CLI tool. 

## Requirements
- pandoc v3.1.6+ (click [here](https://pandoc.org/installing.html) for installation)

## Download Pre-built Documentation
According to your operating system and CPU architecture, download one of the pre-built assets from the [releases](https://github.com/mattcarlotta/nvi/releases) page.

Open a terminal to where the downloaded asset resides and unzip the asset. Replace `<asset>` with the downloaded asset name and replace `</destination/directory/>` with an unzip destination:
```bash
tar –xzf <asset>.tar.gz -C </destination/directory/>
```

You'll find 2 directories within the unzipped asset:
- bin
- man


## Manually Build Documentation
```bash
git clone git@github.com:mattcarlotta/nvi.git
cd nvi/docs
pandoc --standalone --to man nvi.1.md -o nvi.1
```

View man documentation:
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

