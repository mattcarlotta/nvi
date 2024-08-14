use std::fmt::{Display, Formatter, Result as FmtResult};

enum LexerValue {
    Normal,
    Interpolated,
    Multiline,
    Unknown,
}

pub struct LexerValueToken {
    token_type: LexerValue,
    value: Option<String>,
    byte: usize,
    line: usize,
}

pub type LexerValueType = Vec<LexerValue>;

impl LexerValueToken {
    pub fn new() -> Self {
        return LexerValueToken {
            token_type: LexerValue::Unknown,
            value: None,
            byte: 0,
            line: 0,
        };
    }
    // pub fn as_str(&self) -> &'static str {
    //     match self {
    //         ValueType::Normal => "a normal value",
    //         ValueType::Interpolated => "an interpolated value",
    //         ValueType::Multiline => "a multi",
    //     }
    // }
}

impl Display for LexerValue {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        let v = match self {
            LexerValue::Normal => "a normal value",
            LexerValue::Interpolated => "an interpolated value",
            LexerValue::Multiline => "a multiline value",
            LexerValue::Unknown => "an unknown value",
        };
        write!(f, "{}", v)
    }
}

pub struct LexerToken {
    pub key: Option<String>,
    pub values: LexerValueType,
    pub file: String,
}

pub type LexerTokensType = Vec<LexerToken>;

impl LexerToken {
    pub fn new() -> Self {
        return LexerToken {
            key: None,
            values: vec![],
            file: String::new(),
        };
    }
    // pub fn as_str(&self) -> &'static str {
    //     match self {
    //         ValueType::Normal => "a normal value",
    //         ValueType::Interpolated => "an interpolated value",
    //         ValueType::Multiline => "a multi",
    //     }
    // }
}
