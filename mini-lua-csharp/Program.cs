using System.Diagnostics;
using System.Text;

namespace MiniLuaCSharp;

static class Program {
    public static void Main() {
        string source = Console.In.ReadToEnd();

        var lexer = new Lexer(source);

        lexer.ScanTokens();
    }
}

sealed class Lexer(string source) {
    private readonly string source = source;
    private int current = 0;
    private int start = 0;
    private static readonly Dictionary<string, ReservedWord> reservedWords = new() {
        ["and"] = ReservedWord.And,
        ["break"] = ReservedWord.Break,
        ["do"] = ReservedWord.Do,
        ["else"] = ReservedWord.Else,
        ["elseif"] = ReservedWord.ElseIf,
        ["end"] = ReservedWord.End,
        ["false"] = ReservedWord.False,
        ["for"] = ReservedWord.For,
        ["function"] = ReservedWord.Function,
        ["if"] = ReservedWord.If,
        ["in"] = ReservedWord.In,
        ["local"] = ReservedWord.Local,
        ["nil"] = ReservedWord.Nil,
        ["not"] = ReservedWord.Not,
        ["or"] = ReservedWord.Or,
        ["repeat"] = ReservedWord.Repeat,
        ["return"] = ReservedWord.Return,
        ["then"] = ReservedWord.Then,
        ["true"] = ReservedWord.True,
        ["until"] = ReservedWord.Until,
        ["while"] = ReservedWord.While,
    };
    public List<IToken> TokenList = [];

    public void ScanTokens() {
        while (!IsAtEnd()) {
            IToken token = ScanToken();
            if (token is not TokenComment) {
                string word = ExtractWord();
                string tokenType = token switch {
                    TokenReserved => "[RESERVED]",
                    TokenNumber => "[NUMBER]",
                    TokenString => "[STRING]",
                    TokenSymbol => "[SYMBOL]",
                    TokenName => "[NAME]",
                    TokenEOL => "[EOL]",
                    TokenComment => throw new UnreachableException("Unexpected Comment token."),
                    TokenEOF => throw new UnreachableException("Unexpected EOF token."),
                    _ => throw new UnreachableException("Unknown token type."),
                };
                Console.Write($"{tokenType}");
                if (token is TokenEOL) {
                    Console.WriteLine();
                } else {
                    Console.WriteLine($" {word}");
                }
                TokenList.Add(token);
            }
        }
        TokenList.Add(new TokenEOF());
    }

    private IToken ScanToken() {
        SkipWhitespace();
        AlignPointer();

        switch (Advance()) {
            case '\n': return new TokenEOL();
            case '+': return new TokenSymbol(Symbol.Add);
            case '-':
                if (MatchChar('-')) {
                    ScanComment();
                    return new TokenComment();
                } else {
                    return new TokenSymbol(Symbol.Minus);
                }
            case '*': return new TokenSymbol(Symbol.Multiply);
            case '/': return new TokenSymbol(Symbol.Divide);
            case '%': return new TokenSymbol(Symbol.Mod);
            case '^': return new TokenSymbol(Symbol.Power);
            case '#': return new TokenSymbol(Symbol.Length);
            case '=':
                if (MatchChar('=')) {
                    return new TokenSymbol(Symbol.Equal);
                } else {
                    return new TokenSymbol(Symbol.Assign);
                }
            case '>':
                if (MatchChar('=')) {
                    return new TokenSymbol(Symbol.GreaterEqual);
                } else {
                    return new TokenSymbol(Symbol.Greater);
                }
            case '<':
                if (MatchChar('=')) {
                    return new TokenSymbol(Symbol.LessEqual);
                } else {
                    return new TokenSymbol(Symbol.Less);
                }
            case '~':
                if (MatchChar('=')) {
                    return new TokenSymbol(Symbol.NotEqual);
                } else {
                    throw new UnreachableException("Invalid token.");
                }
            case '(': return new TokenSymbol(Symbol.LeftParen);
            case ')': return new TokenSymbol(Symbol.RightParen);
            case '{': return new TokenSymbol(Symbol.LeftBrace);
            case '}': return new TokenSymbol(Symbol.RightBrace);
            case '[': return new TokenSymbol(Symbol.LeftBracket);
            case ']': return new TokenSymbol(Symbol.RightBracket);
            case ';': return new TokenSymbol(Symbol.Semicolon);
            case ':': return new TokenSymbol(Symbol.Colon);
            case ',': return new TokenSymbol(Symbol.Comma);
            case '.':
                if (char.IsDigit(Peek())) {
                    return ScanNumber();
                } else if (MatchChar('.')) {
                    if (MatchChar('.')) {
                        return new TokenSymbol(Symbol.Ellipsis);
                    } else {
                        return new TokenSymbol(Symbol.Concat);
                    }
                } else {
                    return new TokenSymbol(Symbol.Dot);
                }
            case '"' or '\'': return ScanString();
            case char ch when char.IsDigit(ch): return ScanNumber();
            case char ch when IsNameChar(ch, true): return ScanName();
            default: throw new UnreachableException("Invalid token.");
        }
    }

    private void ScanComment() {
        while (Peek() != '\n') {
            Advance();
        }
    }

    private TokenNumber ScanNumber() {
        bool hex = false;
        bool floating = false;
        bool science = false;
        if (Previous() == '0' && (MatchChar('x') || MatchChar('X'))) {
            hex = true;
        }
        if (Previous() == '.') {
            floating = true;
        }
        bool signedPower = false;
        bool numberPower = false;
        while (true) {
            char ch = Peek();
            if (char.IsDigit(ch)) {
                if (science) {
                    numberPower = true;
                }
                Advance();
            } else if (!hex && (ch == 'e' || ch == 'E')) {
                floating = true;
                science = true;
                Advance();
            } else if (char.ToLower(ch) is >='a' and <='f') {
                if (hex) {
                    Advance();
                } else {
                    throw new UnreachableException("Invalid number.");
                }
            } else if (ch == '.') {
                if (hex || floating) {
                    throw new UnreachableException("Invalid number.");
                } else {
                    Advance();
                    floating = true;
                }
            } else if ((ch == '-' || ch == '+') && science && !signedPower && !numberPower) {
                signedPower = true;
                Advance();
            } else {
                break;
            }
        }
        string word = ExtractWord();
        if (floating) {
            return new(double.Parse(word));
        } else {
            if (hex) {
                return new(Convert.ToUInt64(word[2..], 16));
            } else {
                return new(ulong.Parse(word));
            }
        }
    }

    private TokenString ScanString() {
        char strTag = Previous();
        var builder = new StringBuilder();
        while (Peek() != strTag) {
            if (MatchChar('\\')) {
                builder.Append(ParseEscape());
            } else {
                builder.Append(Advance());
            }
        }
        Advance();
        return new(builder.ToString());
    }

    private IToken ScanName() {
        while (IsNameChar(Peek(), false)) {
            Advance();
        }
        string word = ExtractWord();
        if (reservedWords.TryGetValue(word, out var reserved)) {
            return new TokenReserved(reserved);
        } else {
            return new TokenName(word);
        }
    }

    private char ParseEscape() => Advance() switch {
        '\\' => '\\',
        'n' => '\n',
        '\'' => '\'',
        '"' => '"',
        _ => throw new UnreachableException("Invalid escape char."),
    };

    private string ExtractWord() => source[start..current];

    private void SkipWhitespace() {
        while (Peek() != '\n' && char.IsWhiteSpace(Peek())) {
            Advance();
        }
    }

    private void AlignPointer() => start = current;

    private static bool IsNameChar(char ch, bool start) {
        if (start && (char.IsLetter(ch) || ch == '_')) {
            return true;
        } else if (char.IsLetterOrDigit(ch) || ch == '_') {
            return true;
        } else {
            return false;
        }
    }

    private bool IsAtEnd() => current >= source.Length;

    private char Advance() {
        if (IsAtEnd()) {
            return '\0';
        } else {
            return source[current++];
        }
    }

    private char Previous() {
        if (current == 0) {
            return '\0';
        } else {
            return source[current - 1];
        }
    }

    private char Peek() {
        if (IsAtEnd()) {
            return '\0';
        } else {
            return source[current];
        }
    }

    private bool MatchChar(char target) {
        if (IsAtEnd()) {
            return false;
        } else {
            if (Peek() == target) {
                Advance();
                return true;
            } else {
                return false;
            }
        }
    }
}

interface IToken;

sealed record class TokenReserved(ReservedWord Value): IToken;

sealed record class TokenNumber(double Value): IToken;

sealed record class TokenString(string Value): IToken;

sealed record class TokenSymbol(Symbol Value): IToken;

sealed record class TokenName(string Value): IToken;

sealed record class TokenComment: IToken;

sealed record class TokenEOL: IToken;

sealed record class TokenEOF: IToken;

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
