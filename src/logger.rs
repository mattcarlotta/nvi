extern crate colored;

use colored::Colorize;
use std::process::exit;

#[derive(Debug)]
pub struct Logger<'a> {
    log_type: &'a str,
    debug: bool,
}

impl<'a> Logger<'a> {
    pub fn new(log_type: &'a str) -> Self {
        return Logger {
            log_type,
            debug: false,
        };
    }

    pub fn set_debug(&mut self, debug: &bool) {
        self.debug = *debug;
    }

    pub fn debug(&self, message: String) {
        if !self.debug {
            return;
        }

        eprintln!(
            "{} {} {}",
            "[nvi::debug]".blue(),
            self.log_type.blue(),
            message.blue()
        );
    }

    pub fn fatal(&self, message: String) -> ! {
        eprintln!(
            "{} {} {}",
            "[nvi::error]".red(),
            self.log_type.red(),
            message.red()
        );
        exit(1);
    }

    pub fn warn(&self, message: String) {
        eprintln!(
            "{} {} {}",
            "[nvi::warning]".yellow(),
            self.log_type.yellow(),
            message.yellow()
        );
    }

    pub fn print(&self, message: String) {
        eprint!("{} {}", "[nvi]".cyan(), message.cyan());
    }

    pub fn println(&self, message: String) {
        eprintln!("{} {}", "[nvi]".cyan(), message.cyan());
    }

    pub fn print_version_and_exit(&self) -> ! {
        println!(
            "{}", 
            format!(
                "nvi v{}\nCopyright (C) 2024 Matt Carlotta\nThis is free software licensed under the GPL-3.0 license; see the source LICENSE for copying conditions.\nThere is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.",
                option_env!("CARGO_PKG_VERSION").unwrap_or("0.0.0"),
            ).green()
        );
        exit(0);
    }

    pub fn print_help_and_exit(&self) -> ! {
        println!("{}", format!(
            r#"┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ nvi documentation                                                                                                              │
├───────────────┬────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ flag          │ description (for more details, see the man documentation or the README)                                        │
├───────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ --api         │ specifies whether or not to retrieve ENVs from the nvi API. (ex: --api)                                        │
│ --config      │ specifies which environment configuration to load from a nvi.toml file. (ex: --config dev)                     │
│ --debug       │ specifies whether or not to log debug details. (ex: --debug)                                                   │
│ --directory   │ specifies which directory the .env files are located within. (ex: --directory path/to/envs)                    │
│ --environment │ specifies which environment config to use within a nvi API project. (ex: --environment dev)                    │
│ --files       │ specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)                     │
│ --project     │ specifies which remote project to select from the nvi API. (ex: --project my_project)                          │
│ --print       │ specifies whether or not to print envs/ENVs, flags, or json/JSON to stdout after parsing. (ex: --print json)   │
│ --required    │ specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)                         │
│ --save        │ specifies whether or not to save nvi API ENVs to disk with the selected environment name. (ex: --save)         │
│ --version     │ prints out app version. (ex: --version)                                                                        │
│ --            │ specifies which system command to run in a child process with parsed ENVs. (ex: -- cargo run)                  │
└───────────────┴────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘"#,
        ).green());
        exit(0);
    }
}
