use std::collections::HashMap;
use std::io::{self, Read};

fn main() {
    let mut source = String::new();
    io::stdin().read_to_string(&mut source).unwrap();

    let mut lexer = Lexer::new(source);

    lexer.scan_tokens();
}

enum Token {
    Reserved(ReservedWord),
    Number(f64),
    String(String),
    Symbol(Symbol),
    Name(String),
    Comment,
    EOL,
    EOF,
}

#[derive(Clone)]
enum ReservedWord {
    And,
    Break,
    Do,
    Else,
    ElseIf,
    End,
    False,
    For,
    Function,
    If,
    In,
    Local,
    Nil,
    Not,
    Or,
    Repeat,
    Return,
    Then,
    True,
    Until,
    While,
}

enum Symbol {
    Add,
    Minus,
    Multiply,
    Divide,
    Mod,
    Power,
    Length,
    Equal,
    GreaterEqual,
    LessEqual,
    Less,
    Greater,
    NotEqual,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Semicolon,
    Colon,
    Comma,
    Dot,
    Concat,
    Ellipsis,
    Assign,
}

struct Lexer {
    source: Vec<char>,
    token_list: Vec<Token>,
    current: usize,
    start: usize,
    reserved_words: HashMap<String, ReservedWord>,
}

impl Lexer {
    fn new(source: String) -> Self {
        Self {
            source: source.chars().collect(),
            token_list: vec![],
            current: 0,
            start: 0,
            reserved_words: [
                ("and", ReservedWord::And),
                ("break", ReservedWord::Break),
                ("do", ReservedWord::Do),
                ("else", ReservedWord::Else),
                ("elseif", ReservedWord::ElseIf),
                ("end", ReservedWord::End),
                ("false", ReservedWord::False),
                ("for", ReservedWord::For),
                ("function", ReservedWord::Function),
                ("if", ReservedWord::If),
                ("in", ReservedWord::In),
                ("local", ReservedWord::Local),
                ("nil", ReservedWord::Nil),
                ("not", ReservedWord::Not),
                ("or", ReservedWord::Or),
                ("repeat", ReservedWord::Repeat),
                ("return", ReservedWord::Return),
                ("then", ReservedWord::Then),
                ("true", ReservedWord::True),
                ("until", ReservedWord::Until),
                ("while", ReservedWord::While),
            ].into_iter().map(|x| (x.0.to_string(), x.1)).collect(),
        }
    }

    fn scan_tokens(&mut self) {
        while !self.is_at_end() {
            let token = self.scan_token();
            if !matches!(token, Token::Comment) {
                let word = self.extract_word();
                print!(
                    "{token_type}",
                    token_type = match token {
                        Token::Reserved(_) => "[RESERVED]",
                        Token::Number(_) => "[NUMBER]",
                        Token::String(_) => "[STRING]",
                        Token::Symbol(_) => "[SYMBOL]",
                        Token::Name(_) => "[NAME]",
                        Token::EOL => "[EOL]",
                        Token::Comment => unreachable!("Unexpected Comment token."),
                        Token::EOF => unreachable!("Unexpected EOF token."),
                    }
                );
                if matches!(token, Token::EOL) {
                    println!();
                } else {
                    println!(" {word}");
                }
                self.token_list.push(token);
            }
        }
        self.token_list.push(Token::EOF);
    }

    fn scan_token(&mut self) -> Token {
        self.skip_whitespace();
        self.align_pointer();

        match self.advance() {
            '\n' => Token::EOL,
            '+' => Token::Symbol(Symbol::Add),
            '-' => if self.match_char('-') {
                self.scan_comment();
                Token::Comment
            } else {
                Token::Symbol(Symbol::Minus)
            },
            '*' => Token::Symbol(Symbol::Multiply),
            '/' => Token::Symbol(Symbol::Divide),
            '%' => Token::Symbol(Symbol::Mod),
            '^' => Token::Symbol(Symbol::Power),
            '#' => Token::Symbol(Symbol::Length),
            '=' => if self.match_char('=') {
                Token::Symbol(Symbol::Equal)
            } else {
                Token::Symbol(Symbol::Assign)
            },
            '>' => if self.match_char('=') {
                Token::Symbol(Symbol::GreaterEqual)
            } else {
                Token::Symbol(Symbol::Greater)
            },
            '<' => if self.match_char('=') {
                Token::Symbol(Symbol::LessEqual)
            } else {
                Token::Symbol(Symbol::Less)
            },
            '~' => if self.match_char('=') {
                Token::Symbol(Symbol::NotEqual)
            } else {
                unreachable!("Invalid token.")
            },
            '(' => Token::Symbol(Symbol::LeftParen),
            ')' => Token::Symbol(Symbol::RightParen),
            '{' => Token::Symbol(Symbol::LeftBrace),
            '}' => Token::Symbol(Symbol::RightBrace),
            '[' => Token::Symbol(Symbol::LeftBracket),
            ']' => Token::Symbol(Symbol::RightBracket),
            ';' => Token::Symbol(Symbol::Semicolon),
            ':' => Token::Symbol(Symbol::Colon),
            ',' => Token::Symbol(Symbol::Comma),
            '.' => if self.peek().is_numeric() {
                self.scan_number()
            } else if self.match_char('.') {
                if self.match_char('.') {
                    Token::Symbol(Symbol::Ellipsis)
                } else {
                    Token::Symbol(Symbol::Concat)
                }
            } else {
                Token::Symbol(Symbol::Dot)
            },
            '"' | '\'' => self.scan_string(),
            ch if ch.is_numeric() => self.scan_number(),
            ch if Self::is_name_char(ch, true) => self.scan_name(),
            _ => unreachable!("Invalid token."),
        }
    }

    fn scan_comment(&mut self) {
        while self.peek() != '\n' {
            self.advance();
        }
    }

    fn scan_number(&mut self) -> Token {
        let mut hex = false;
        let mut float = false;
        let mut science = false;
        if self.previous() == '0' && (self.match_char('x') || self.match_char('X')) {
            hex = true;
        }
        if self.previous() == '.' {
            float = true;
        }
        let mut signed_power = false;
        let mut number_power = false;
        loop {
            let ch = self.peek();
            if ch.is_numeric() {
                if science {
                    number_power = true;
                }
                self.advance();
            } else if !hex && (ch == 'e' || ch == 'E') {
                float = true;
                science = true;
                self.advance();
            } else if let 'a'..='f' = ch.to_lowercase().next().unwrap() {
                if hex {
                    self.advance();
                } else {
                    unreachable!("Invalid number.");
                }
            } else if ch == '.' {
                if hex || float {
                    unreachable!("Invalid number.");
                } else {
                    self.advance();
                    float = true;
                }
            } else if (ch == '-' || ch == '+') && science && !signed_power && !number_power {
                signed_power = true;
                self.advance();
            } else {
                break;
            }
        }
        let word = self.extract_word();
        if float {
            Token::Number(word.parse::<f64>().unwrap())
        } else {
            if hex {
                Token::Number(u64::from_str_radix(&word[2..], 16).unwrap() as f64)
            } else {
                Token::Number(word.parse::<u64>().unwrap() as f64)
            }
        }
    }

    fn scan_string(&mut self) -> Token {
        let str_tag = self.previous();
        let mut string = String::new();
        while self.peek() != str_tag {
            if self.match_char('\\') {
                string.push(self.parse_escape());
            } else {
                string.push(self.advance());
            }
        }
        self.advance();
        Token::String(string)
    }

    fn scan_name(&mut self) -> Token {
        while Self::is_name_char(self.peek(), false) {
            self.advance();
        }
        let word = self.extract_word();
        if let Some(reserved) = self.reserved_words.get(&word) {
            Token::Reserved(reserved.clone())
        } else {
            Token::Name(word)
        }
    }

    fn parse_escape(&mut self) -> char {
        match self.advance() {
            '\\' => '\\',
            'n' => '\n',
            '\'' => '\'',
            '"' => '"',
            _ => unreachable!("Invalid escape char.")
        }
    }

    fn extract_word(&self) -> String {
        String::from_iter(&self.source[self.start..self.current])
    }

    fn skip_whitespace(&mut self) {
        while self.peek() != '\n' && self.peek().is_whitespace() {
            self.advance();
        }
    }

    fn align_pointer(&mut self) {
        self.start = self.current;
    }

    fn is_name_char(ch: char, start: bool) -> bool {
        if start && (ch.is_alphabetic() || ch == '_') {
            true
        } else if ch.is_alphanumeric() || ch == '_' {
            true
        } else {
            false
        }
    }

    fn is_at_end(&self) -> bool {
        self.current >= self.source.len()
    }

    fn advance(&mut self) -> char {
        if self.is_at_end() {
            '\0'
        } else {
            self.current += 1;
            self.source[self.current - 1]
        }
    }

    fn previous(&self) -> char {
        if self.current == 0 {
            '\0'
        } else {
            self.source[self.current - 1]
        }
    }

    fn peek(&mut self) -> char {
        if self.is_at_end() {
            '\0'
        } else {
            self.source[self.current]
        }
    }

    fn match_char(&mut self, target: char) -> bool {
        if self.is_at_end() {
            false
        } else {
            if self.peek() == target {
                self.advance();
                true
            } else {
                false
            }
        }
    }
}
