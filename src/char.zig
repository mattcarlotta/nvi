pub const NULL_CHAR: u8 = 0;
pub const LINE_DELIMITER: u8 = '\n';
pub const ASSIGN_OP: u8 = '=';
pub const HASH: u8 = '#';
pub const DOLLAR_SIGN: u8 = '$';
pub const OPEN_BRACE: u8 = '{';
pub const CLOSE_BRACE: u8 = '}';
pub const BACK_SLASH: u8 = '\\';

pub const SPECIAL_CHARS = [_]u8{ NULL_CHAR, LINE_DELIMITER, ASSIGN_OP, HASH, DOLLAR_SIGN, BACK_SLASH };
