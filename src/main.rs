mod api;
mod arg;
mod generator;
mod globals;
mod lexer;
mod logger;
mod options;
mod parser;
use std::env;

use generator::Generator;
use lexer::Lexer;
use options::Options;
use parser::Parser;

fn main() {
    let options = Options::new(env::args().collect());

    let mut lexer = Lexer::new(&options);
    lexer.tokenize();

    let mut parser = Parser::new(&options, lexer.get_tokens());
    parser.parse_tokens();

    Generator::new(&options).run(parser.get_envs());
}
