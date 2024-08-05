use crate::logger::Logger;
use crate::options::Options;
use lazy_static::lazy_static;
use std::collections::HashMap;

pub enum FLAG {
    API,
    CONFIG,
    DEBUG,
    DIRECTORY,
    ENVIRONMENT,
    EXECUTE,
    FILES,
    HELP,
    PRINT,
    PROJECT,
    REQUIRED,
    SAVE,
    VERSION,
    // UNKNOWN,
}

lazy_static! {
    static ref FLAGS: HashMap<&'static str, FLAG> = {
        let mut flags = HashMap::new();
        flags.insert("--api", FLAG::API);
        flags.insert("--config", FLAG::CONFIG);
        flags.insert("--debug", FLAG::DEBUG);
        flags.insert("--directory", FLAG::DIRECTORY);
        flags.insert("--environment", FLAG::ENVIRONMENT);
        flags.insert("--", FLAG::EXECUTE);
        flags.insert("--files", FLAG::FILES);
        flags.insert("--help", FLAG::HELP);
        flags.insert("--print", FLAG::PRINT);
        flags.insert("--project", FLAG::PROJECT);
        flags.insert("--required", FLAG::REQUIRED);
        flags.insert("--save", FLAG::SAVE);
        flags.insert("--version", FLAG::VERSION);
        return flags;
    };
}

pub fn parse(options: &mut Options) {
    let mut i: usize = 1;

    while i < options.argc {
        let flag = match options.args.get(i) {
            Some(f) => f,
            None => "",
        };

        match FLAGS.get(flag) {
            Some(FLAG::API) => println!("API"),
            Some(FLAG::CONFIG) => println!("CONFIG"),
            Some(FLAG::DEBUG) => options.debug = true,
            Some(FLAG::DIRECTORY) => println!("DIRECTORY"),
            Some(FLAG::ENVIRONMENT) => println!("ENVIRONMENT"),
            Some(FLAG::EXECUTE) => println!("EXECUTE"),
            Some(FLAG::FILES) => println!("FILES"),
            Some(FLAG::HELP) => println!("HELP"),
            Some(FLAG::PRINT) => options.print = true,
            Some(FLAG::PROJECT) => println!("PROJECT"),
            Some(FLAG::REQUIRED) => println!("REQUIRED"),
            Some(FLAG::SAVE) => options.save = true,
            Some(FLAG::VERSION) => Logger::print_version(),
            None => println!("UNKNOWN {}", flag),
        }

        i += 1;
    }
}
