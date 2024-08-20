mod api;
mod arg;
mod generator;
mod globals;
mod lexer;
mod logger;
mod options;
mod parser;

use generator::Generator;
use lexer::Lexer;
use options::Options;
use parser::Parser;

fn main() {
    let options = Options::new();

    let mut lexer = Lexer::new(&options);
    lexer.tokenize();

    let tokens = lexer.get_tokens();

    let mut parser = Parser::new(&options, &tokens);
    parser.parse_tokens();

    let envs = parser.get_envs();

    let generator = Generator::new(&options, &envs);
    generator.run();
}
