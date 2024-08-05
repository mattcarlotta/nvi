extern crate chrono;
extern crate colored;

use chrono::Datelike;
use colored::*;
use std::process::exit;

pub struct Logger {}

impl Logger {
    pub fn print_version_and_exit() {
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
