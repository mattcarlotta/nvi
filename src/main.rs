mod api;
mod arg;
mod globals;
mod logger;
mod options;

use options::Options;
use std::process::Command;

use crate::logger::Logger;

fn main() {
    let options = Options::new();

    if !options.commands.is_empty() {
        if let Err(err) = Command::new(&options.commands[0])
            .args(&options.commands[1..options.commands.len()])
            .spawn()
        {
            let logger = Logger::new("Command");
            logger.fatal(format!(
                "was unable to run: {}. Reason: {}",
                &options.commands.join(" "),
                err
            ));
        }
    }
}
