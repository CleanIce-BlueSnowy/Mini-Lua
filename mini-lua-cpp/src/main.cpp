#include <cstddef>
#include <iostream>
#include <iterator>
#include <locale>
#include <print>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

enum class ReservedWord {
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
};

enum class Symbol {
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
};

class Token {
public:
    virtual ~Token() = default;
};

class TokenReserved final: public Token {
public:
    ReservedWord value;

    TokenReserved(ReservedWord init);
};

class TokenNumber final: public Token {
public:
    double value;

    TokenNumber(double init);
};

class TokenString final: public Token {
public:
    std::string value;

    TokenString(std::string&& init);
};

class TokenSymbol final: public Token {
public:
    Symbol value;

    TokenSymbol(Symbol init);
};

class TokenName final: public Token {
public:
    std::string value;

    TokenName(std::string&& init);
};

class TokenComment final: public Token {};

class TokenEOL final: public Token {};

class TokenEOF final: public Token {};

class Lexer final {
private:
    std::string source;
    std::size_t current;
    std::size_t start;

    inline static std::unordered_map<std::string, ReservedWord> reserved_words = {
        {"and", ReservedWord::And},
        {"break", ReservedWord::Break},
        {"do", ReservedWord::Do},
        {"else", ReservedWord::Else},
        {"elseif", ReservedWord::ElseIf},
        {"end", ReservedWord::End},
        {"false", ReservedWord::False},
        {"for", ReservedWord::For},
        {"function", ReservedWord::Function},
        {"if", ReservedWord::If},
        {"in", ReservedWord::In},
        {"local", ReservedWord::Local},
        {"nil", ReservedWord::Nil},
        {"not", ReservedWord::Not},
        {"or", ReservedWord::Or},
        {"repeat", ReservedWord::Repeat},
        {"return", ReservedWord::Return},
        {"then", ReservedWord::Then},
        {"true", ReservedWord::True},
        {"until", ReservedWord::Until},
        {"while", ReservedWord::While},
    };

    Token* scan_token();
    void scan_comment();
    TokenNumber* scan_number();
    TokenString* scan_string();
    Token* scan_name();
    char parse_escape();
    std::string extract_word();
    void skip_whitespace();
    void align_pointer();
    static bool is_name_char(char ch, bool start);
    bool is_at_end();
    char advance();
    char previous();
    char peek();
    bool match_char(char target);

public:
    std::vector<Token*> token_list;

    Lexer(std::string&& init);
    void scan_tokens();
};

void clear_resources(const Lexer& lexer);

int main() {
    std::string source{std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>()};

    Lexer lexer(std::move(source));

    lexer.scan_tokens();

    clear_resources(lexer);
    return 0;
}

void clear_resources(const Lexer& lexer) {
    for (const Token* token : lexer.token_list) {
        delete token;
    }
}

TokenReserved::TokenReserved(ReservedWord init): value(init) {}
TokenNumber::TokenNumber(double init): value(init) {}
TokenString::TokenString(std::string&& init): value(init) {}
TokenSymbol::TokenSymbol(Symbol init): value(init) {}
TokenName::TokenName(std::string&& init): value(init) {}

Lexer::Lexer(std::string&& init): source(init), current(0), start(0), token_list() {}

void Lexer::scan_tokens() {
    while (!is_at_end()) {
        Token* token = scan_token();
        const std::type_info& type_info = typeid(*token);
        if (type_info != typeid(TokenComment)) {
            std::string word = extract_word();
            std::string token_type;
            if (type_info == typeid(TokenReserved)) {
                token_type = "[RESERVED]";
            } else if (type_info == typeid(TokenNumber)) {
                token_type = "[NUMBER]";
            } else if (type_info == typeid(TokenString)) {
                token_type = "[STRING]";
            } else if (type_info == typeid(TokenSymbol)) {
                token_type = "[SYMBOL]";
            } else if (type_info == typeid(TokenName)) {
                token_type = "[NAME]";
            } else if (type_info == typeid(TokenEOL)) {
                token_type = "[EOL]";
            } else if (type_info == typeid(TokenComment)) {
                throw std::logic_error("Unexpected Comment token.");
            } else if (type_info == typeid(TokenEOF)) {
                throw std::logic_error("Unexpected EOF token.");
            } else {
                std::unreachable();
            }
            std::print("{}", token_type);
            if (type_info == typeid(TokenEOL)) {
                std::println();
            } else {
                std::println(" {}", word);
            }
            token_list.push_back(token);
        }
    }
    token_list.push_back(new TokenEOF());
}

Token* Lexer::scan_token() {
    skip_whitespace();
    align_pointer();

    switch (advance()) {
        case '\n': return new TokenEOL();
        case '+': return new TokenSymbol(Symbol::Add);
        case '-':
            if (match_char('-')) {
                scan_comment();
                return new TokenComment();
            } else {
                return new TokenSymbol(Symbol::Minus);
            }
        case '*': return new TokenSymbol(Symbol::Multiply);
        case '/': return new TokenSymbol(Symbol::Divide);
        case '%': return new TokenSymbol(Symbol::Mod);
        case '^': return new TokenSymbol(Symbol::Power);
        case '#': return new TokenSymbol(Symbol::Length);
        case '=':
            if (match_char('=')) {
                return new TokenSymbol(Symbol::Equal);
            } else {
                return new TokenSymbol(Symbol::Assign);
            }
        case '>':
            if (match_char('=')) {
                return new TokenSymbol(Symbol::GreaterEqual);
            } else {
                return new TokenSymbol(Symbol::Greater);
            }
        case '<':
            if (match_char('=')) {
                return new TokenSymbol(Symbol::LessEqual);
            } else {
                return new TokenSymbol(Symbol::Less);
            }
        case '~':
            if (match_char('=')) {
                return new TokenSymbol(Symbol::NotEqual);
            } else {
                throw std::logic_error("Invalid token.");
            }
        case '(': return new TokenSymbol(Symbol::LeftParen);
        case ')': return new TokenSymbol(Symbol::RightParen);
        case '{': return new TokenSymbol(Symbol::LeftBrace);
        case '}': return new TokenSymbol(Symbol::RightBrace);
        case '[': return new TokenSymbol(Symbol::LeftBracket);
        case ']': return new TokenSymbol(Symbol::RightBracket);
        case ';': return new TokenSymbol(Symbol::Semicolon);
        case ':': return new TokenSymbol(Symbol::Colon);
        case ',': return new TokenSymbol(Symbol::Comma);
        case '.':
            if (std::isdigit(peek(), std::locale())) {
                return scan_number();
            } else if (match_char('.')) {
                if (match_char('.')) {
                    return new TokenSymbol(Symbol::Ellipsis);
                } else {
                    return new TokenSymbol(Symbol::Concat);
                }
            } else {
                return new TokenSymbol(Symbol::Dot);
            }
        case '"':
        case '\'':
            return scan_string();
        default:
            char ch = previous();
            if (std::isdigit(ch, std::locale())) {
                return scan_number();
            } else if (is_name_char(ch, true)) {
                return scan_name();
            } else {
                throw std::logic_error("Invalid token.");
            }
    }
}

void Lexer::scan_comment() {
    while (peek() != '\n') {
        advance();
    }
}

TokenNumber* Lexer::scan_number() {
    bool hex = false;
    bool floating = false;
    bool science = false;
    if (previous() == '0' && (match_char('x') || match_char('X'))) {
        hex = true;
    }
    if (previous() == '.') {
        floating = true;
    }
    bool signed_power = false;
    bool number_power = false;
    while (true) {
        char ch = peek();
        if (std::isdigit(ch, std::locale())) {
            if (science) {
                number_power = true;
            }
            advance();
        } else if (!hex && (ch == 'e' || ch == 'E')) {
            floating = true;
            science = true;
            advance();
        } else if ('a' <= std::tolower(ch, std::locale()) && std::tolower(ch, std::locale()) <= 'f') {
            if (hex) {
                advance();
            } else {
                throw std::logic_error("Invalid number.");
            }
        } else if (ch == '.') {
            if (hex || floating) {
                throw std::logic_error("Invalid number.");
            } else {
                advance();
                floating = true;
            }
        } else if ((ch == '-' || ch == '+') && science && !signed_power && !number_power) {
            signed_power = true;
            advance();
        } else {
            break;
        }
    }
    std::string word = extract_word();
    if (floating) {
        return new TokenNumber(std::stod(word));
    } else {
        if (hex) {
            return new TokenNumber(static_cast<double>(std::stoull(word.substr(2, word.size() - 2), nullptr, 16)));
        } else {
            return new TokenNumber(static_cast<double>(std::stoull(word, nullptr, 10)));
        }
    }
}

TokenString* Lexer::scan_string() {
    char str_tag = previous();
    std::string string;
    while (peek() != str_tag) {
        if (match_char('\\')) {
            string += parse_escape();
        } else {
            string += advance();
        }
    }
    advance();
    return new TokenString(std::move(string));
}

Token* Lexer::scan_name() {
    while (is_name_char(peek(), false)) {
        advance();
    }
    std::string word = extract_word();
    if (reserved_words.contains(word)) {
        return new TokenReserved(reserved_words[word]);
    } else {
        return new TokenString(std::move(word));
    }
}

char Lexer::parse_escape() {
    switch (advance()) {
        case '\\': return '\\';
        case 'n': return '\n';
        case '\'': return '\'';
        case '"': return '"';
        default: throw std::logic_error("Invalid escape char.");
    }
}

std::string Lexer::extract_word() {
    return source.substr(start, current - start);
}

void Lexer::skip_whitespace() {
    while (peek() != '\n' && std::isspace(peek(), std::locale())) {
        advance();
    }
}

void Lexer::align_pointer() {
    start = current;
}

bool Lexer::is_name_char(char ch, bool start) {
    if (start && (std::isalpha(ch, std::locale()) || ch == '_')) {
        return true;
    } else if (std::isalnum(ch, std::locale()) || ch == '_') {
        return true;
    } else {
        return false;
    }
}

bool Lexer::is_at_end() {
    return current >= source.size();
}

char Lexer::advance() {
    if (is_at_end()) {
        return '\0';
    } else {
        return source[current++];
    }
}

char Lexer::previous() {
    if (current == 0) {
        return '\0';
    } else {
        return source[current - 1];
    }
}

char Lexer::peek() {
    if (is_at_end()) {
        return '\0';
    } else {
        return source[current];
    }
}

bool Lexer::match_char(char target) {
    if (is_at_end()) {
        return false;
    } else {
        if (peek() == target) {
            advance();
            return true;
        } else {
            return false;
        }
    }
}
