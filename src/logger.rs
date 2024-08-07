extern crate chrono;
extern crate colored;

use chrono::Datelike;
use colored::*;
use std::process::exit;

#[derive(Debug)]
pub struct Logger<'a> {
    log_type: &'a str,
}

impl<'a> Logger<'a> {
    pub fn new(log_type: &'a str) -> Self {
        return Logger { log_type };
    }

    pub fn debug(&self, message: String) {
        println!(
            "{} {} {}",
            "[info]".cyan(),
            self.log_type.cyan(),
            message.cyan()
        );
    }

    pub fn fatal(&self, message: String) {
        println!(
            "{} {} {}",
            "[fatal]".red(),
            self.log_type.red(),
            message.red()
        );
        exit(1);
    }

    pub fn warn(&self, message: String) {
        println!(
            "{} {} {}",
            "[warning]".yellow(),
            self.log_type.yellow(),
            message.yellow()
        );
    }

    pub fn print_version_and_exit(&self) {
        println!(
            "{}", 
            format!(
                "nvi v{}\nCopyright (C) {} Matt Carlotta\nThis is free software licensed under the GPL-3.0 license; see the source LICENSE for copying conditions.\nThere is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.",
                option_env!("CARGO_PKG_VERSION").unwrap_or("0.0.0"),
                chrono::Utc::now().year()
            ).green()
        );
        exit(0);
    }
}
