mod arg;
mod logger;
mod options;

use arg::ArgParser;
use options::Options;

fn main() {
    let mut options = Options::new();
    ArgParser::new(&mut options).parse();

    // println!("{:?}", options);
}
