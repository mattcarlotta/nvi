mod arg;
mod logger;

use arg::options::Options;
use std::process::Command;

fn main() {
    let options = Options::new();

    if options.commands.len() > 0 {
        if let Err(_) = Command::new(&options.commands[0])
            .args(&options.commands[1..options.commands.len()])
            .spawn()
        {
            eprintln!("ERROR");
        }
    }
}
