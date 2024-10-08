use std::fmt::{Debug, Display, Formatter, Result as FmtResult};

#[derive(Debug, Clone, PartialEq)]
pub enum LexerValue {
    Normal,
    Interpolated,
    Comment,
    Multiline,
}

impl Display for LexerValue {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        let val = match self {
            LexerValue::Normal => "NORMAL",
            LexerValue::Interpolated => "INTERPOLATED",
            LexerValue::Comment => "COMMENT",
            LexerValue::Multiline => "MULTILINE",
        };
        write!(f, "{}", val)
    }
}

pub struct LexerValueToken {
    pub token_type: LexerValue,
    pub value: Option<String>,
    pub line: usize,
    pub byte: usize,
}

impl Clone for LexerValueToken {
    fn clone(&self) -> Self {
        LexerValueToken {
            token_type: self.token_type.clone(),
            value: self.value.clone(),
            byte: self.byte,
            line: self.line,
        }
    }
}

impl Debug for LexerValueToken {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        let value = match &self.value {
            Some(v) => format!("Some({v})"),
            None => String::from("None"),
        };
        write!(
            f,
            "token_type: {}, value: {}, line: {}, byte: {}",
            &self.token_type, value, &self.line, &self.byte
        )
    }
}

impl LexerValueToken {
    pub fn new(token_type: LexerValue, value: &String, line: usize, byte: usize) -> Self {
        LexerValueToken {
            token_type,
            value: Some(value.to_owned()),
            line,
            byte,
        }
    }
}

pub struct LexerToken {
    pub key: Option<String>,
    pub values: Vec<LexerValueToken>,
    pub file: String,
}

impl Clone for LexerToken {
    fn clone(&self) -> Self {
        LexerToken {
            key: self.key.clone(),
            values: self.values.clone(),
            file: self.file.clone(),
        }
    }
}

impl Debug for LexerToken {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        f.debug_struct("LexerToken")
            .field("key", &self.key)
            .field("values", &self.values)
            .field("file", &self.file)
            .finish()
    }
}

impl LexerToken {
    pub fn new() -> Self {
        LexerToken {
            key: None,
            values: vec![],
            file: String::new(),
        }
    }
}
