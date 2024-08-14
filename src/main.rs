mod api;
mod arg;
mod globals;
mod lexer;
mod logger;
mod options;

use lexer::Lexer;
use options::Options;
use std::process::Command;

use crate::logger::Logger;

fn main() {
    let options = Options::new();

    let mut lexer = Lexer::new(&options);
    {
        lexer.parse_files();
    }
    let _tokens = lexer.get_tokens();

    if !options.commands.is_empty() {
        if let Err(err) = Command::new(&options.commands[0])
            .args(&options.commands[1..options.commands.len()])
            .spawn()
        {
            Logger::new("Command").fatal(format!(
                "was unable to run: {}. Reason: {}",
                &options.commands.join(" "),
                err
            ));
        }
    }
}
