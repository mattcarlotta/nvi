use crate::arg::ARG;
use std::collections::HashMap;
use std::env;
use std::path::PathBuf;
use std::sync::OnceLock;

// pub const SPACE: char = ' '; // 0x20
// pub const OPEN_BRACKET: char = '['; // 0x5b
// pub const CLOSE_BRACKET: char = ']'; // 0x5d
// pub const DOUBLE_QUOTE: char = '\"'; // 0x22
// pub const COMMA: char = ','; // 0x2c
pub const LINE_DELIMITER: char = '\n'; // 0x0a
pub const ASSIGN_OP: char = '='; // 0x3d
pub const HASH: char = '#'; // 0x23
pub const BACK_SLASH: char = '\\'; // 0x5c
pub const DOLLAR_SIGN: char = '$'; // 0x24
pub const OPEN_BRACE: char = '{'; // 0x7b
pub const CLOSE_BRACE: char = '}'; // 0x7d
pub const NULL_CHAR: char = '\0'; // 0x0

#[cfg(debug_assertions)]
pub const API_URL: &'static str = "http://127.0.0.1:4000";

#[cfg(not(debug_assertions))]
pub const API_URL: &'static str = "http://nvi.sh/api";

pub fn current_dir() -> &'static PathBuf {
    static CURRENT_DIR: OnceLock<PathBuf> = OnceLock::new();
    CURRENT_DIR.get_or_init(|| {
        return env::current_dir().expect("Failed to retrieve current directory.");
    })
}

pub fn args() -> &'static HashMap<&'static str, ARG> {
    static HASHMAP: OnceLock<HashMap<&'static str, ARG>> = OnceLock::new();
    HASHMAP.get_or_init(|| {
        return HashMap::from([
            (ARG::API.as_str(), ARG::API),
            (ARG::CONFIG.as_str(), ARG::CONFIG),
            (ARG::DEBUG.as_str(), ARG::DEBUG),
            (ARG::DIRECTORY.as_str(), ARG::DIRECTORY),
            (ARG::ENVIRONMENT.as_str(), ARG::ENVIRONMENT),
            (ARG::EXECUTE.as_str(), ARG::EXECUTE),
            (ARG::FILES.as_str(), ARG::FILES),
            (ARG::HELP.as_str(), ARG::HELP),
            (ARG::PRINT.as_str(), ARG::PRINT),
            (ARG::PROJECT.as_str(), ARG::PROJECT),
            (ARG::REQUIRED.as_str(), ARG::REQUIRED),
            (ARG::SAVE.as_str(), ARG::SAVE),
            (ARG::VERSION.as_str(), ARG::VERSION),
            (ARG::UNKNOWN.as_str(), ARG::UNKNOWN),
        ]);
    })
}
