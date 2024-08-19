use std::fmt::{Debug, Display, Formatter, Result as FmtResult};

#[derive(Clone)]
pub enum LexerValue {
    Normal,
    Interpolated,
    Comment,
    // Multiline,
}

pub struct LexerValueToken {
    pub token_type: LexerValue,
    pub value: Option<String>,
    pub line: usize,
    pub byte: usize,
}

impl Clone for LexerValueToken {
    fn clone(&self) -> Self {
        return LexerValueToken {
            token_type: self.token_type.clone(),
            value: self.value.clone(),
            byte: self.byte,
            line: self.line,
        };
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
        return LexerValueToken {
            token_type,
            value: Some(value.to_owned()),
            line,
            byte,
        };
    }
}

impl Display for LexerValue {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        let v = match self {
            LexerValue::Normal => "NORMAL",
            LexerValue::Interpolated => "INTERPOLATED",
            LexerValue::Comment => "COMMENT",
            // LexerValue::Multiline => "MULTILINE",
            // LexerValue::Unknown => "an unknown value",
        };
        write!(f, "{}", v)
    }
}

pub struct LexerToken {
    pub key: Option<String>,
    pub values: Vec<LexerValueToken>,
    pub file: String,
}

impl Clone for LexerToken {
    fn clone(&self) -> Self {
        return LexerToken {
            key: self.key.clone(),
            values: self.values.clone(),
            file: self.file.clone(),
        };
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
        return LexerToken {
            key: None,
            values: vec![],
            file: String::new(),
        };
    }
}
