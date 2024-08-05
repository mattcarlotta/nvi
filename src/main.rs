mod arg;
mod logger;
mod options;

use options::Options;

fn main() {
    let mut options = Options::new();
    arg::parse(&mut options);

    println!("{:?}", options);
}
