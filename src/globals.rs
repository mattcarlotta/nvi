use crate::arg::ARG;
use lazy_static::lazy_static;
use std::collections::HashMap;

// pub const COMMENT: char = '#'; // 0x23
// pub const SPACE: char = ' '; // 0x20
// pub const OPEN_BRACKET: char = '['; // 0x5b
// pub const CLOSE_BRACKET: char = ']'; // 0x5d
// pub const COMMA: char = ','; // 0x2c
// pub const DOUBLE_QUOTE: char = '\"'; // 0x22
// pub const LINE_DELIMITER: char = '\n'; // 0x0a
// pub const ASSIGN_OP: char = '='; // 0x3d
//
pub const API_URL: &'static str = match option_env!("API_URL") {
    Some(v) => v,
    None => "http://127.0.0.1:4000",
};

lazy_static! {
    pub static ref ARGS: HashMap<&'static str, ARG> = {
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
    };
}
