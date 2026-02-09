import enum
import sys


def main():
    source = sys.stdin.read()

    lexer = Lexer(source)

    lexer.scan_tokens()


class Token:
    pass


class ReservedWord(enum.Enum):
    And = enum.auto()
    Break = enum.auto()
    Do = enum.auto()
    Else = enum.auto()
    ElseIf = enum.auto()
    End = enum.auto()
    False_ = enum.auto()
    For = enum.auto()
    Function = enum.auto()
    If = enum.auto()
    In = enum.auto()
    Local = enum.auto()
    Nil = enum.auto()
    Not = enum.auto()
    Or = enum.auto()
    Repeat = enum.auto()
    Return = enum.auto()
    Then = enum.auto()
    True_ = enum.auto()
    Until = enum.auto()
    While = enum.auto()


class Symbol(enum.Enum):
    Add = enum.auto()
    Minus = enum.auto()
    Multiply = enum.auto()
    Divide = enum.auto()
    Mod = enum.auto()
    Power = enum.auto()
    Length = enum.auto()
    Equal = enum.auto()
    GreaterEqual = enum.auto()
    LessEqual = enum.auto()
    Less = enum.auto()
    Greater = enum.auto()
    NotEqual = enum.auto()
    LeftParen = enum.auto()
    RightParen = enum.auto()
    LeftBrace = enum.auto()
    RightBrace = enum.auto()
    LeftBracket = enum.auto()
    RightBracket = enum.auto()
    Semicolon = enum.auto()
    Colon = enum.auto()
    Comma = enum.auto()
    Dot = enum.auto()
    Concat = enum.auto()
    Ellipsis = enum.auto()
    Assign = enum.auto()


class TokenReserved(Token):
    def __init__(self, init: ReservedWord):
        self.value = init


class TokenNumber(Token):
    def __init__(self, init: float):
        self.value = init


class TokenString(Token):
    def __init__(self, init: str):
        self.value = init


class TokenSymbol(Token):
    def __init__(self, init: Symbol):
        self.value = init


class TokenName(Token):
    def __init__(self, init: str):
        self.value = init


class TokenComment(Token):
    pass


class TokenEOL(Token):
    pass


class TokenEOF(Token):
    pass


class Lexer:
    reserved_words = {
        "and": ReservedWord.And,
        "break": ReservedWord.Break,
        "do": ReservedWord.Do,
        "else": ReservedWord.Else,
        "elseif": ReservedWord.ElseIf,
        "end": ReservedWord.End,
        "false": ReservedWord.False_,
        "for": ReservedWord.For,
        "function": ReservedWord.Function,
        "if": ReservedWord.If,
        "in": ReservedWord.In,
        "local": ReservedWord.Local,
        "nil": ReservedWord.Nil,
        "not": ReservedWord.Not,
        "or": ReservedWord.Or,
        "repeat": ReservedWord.Repeat,
        "return": ReservedWord.Return,
        "then": ReservedWord.Then,
        "true": ReservedWord.True_,
        "until": ReservedWord.Until,
        "while": ReservedWord.While,
    }

    def __init__(self, source: str):
        self.source = source
        self.token_list: list[Token] = []
        self.current = 0
        self.start = 0


    def scan_tokens(self):
        while not self.is_at_end():
            token = self.scan_token()
            if not isinstance(token, TokenComment):
                word = self.extract_word()
                match token:
                    case TokenReserved():
                        token_type = "[RESERVED]"
                    case TokenNumber():
                        token_type = "[NUMBER]"
                    case TokenString():
                        token_type = "[STRING]"
                    case TokenSymbol():
                        token_type = "[SYMBOL]"
                    case TokenName():
                        token_type = "[NAME]"
                    case TokenEOL():
                        token_type = "[EOL]"
                    case TokenComment():
                        raise RuntimeError("Unexpected Comment token.")
                    case TokenEOF():
                        raise RuntimeError("Unexpected EOF token.")
                    case _:
                        raise RuntimeError("Unknown token type.")
                print(f"{token_type}", end="")
                if isinstance(token, TokenEOL):
                    print()
                else:
                    print(f" {word}")
                self.token_list.append(token)
        self.token_list.append(TokenEOF())


    def scan_token(self) -> Token:
        self.skip_whitespace()
        self.align_pointer()

        match self.advance():
            case "\n":
                return TokenEOL()
            case "+":
                return TokenSymbol(Symbol.Add)
            case "-":
                if self.match_char("-"):
                    self.scan_comment()
                    return TokenComment()
                else:
                    return TokenSymbol(Symbol.Minus)
            case "*":
                return TokenSymbol(Symbol.Multiply)
            case "/":
                return TokenSymbol(Symbol.Divide)
            case "%":
                return TokenSymbol(Symbol.Mod)
            case "^":
                return TokenSymbol(Symbol.Power)
            case "#":
                return TokenSymbol(Symbol.Length)
            case "=":
                if self.match_char("="):
                    return TokenSymbol(Symbol.Equal)
                else:
                    return TokenSymbol(Symbol.Assign)
            case ">":
                if self.match_char("="):
                    return TokenSymbol(Symbol.GreaterEqual)
                else:
                    return TokenSymbol(Symbol.Greater)
            case "<":
                if self.match_char("="):
                    return TokenSymbol(Symbol.LessEqual)
                else:
                    return TokenSymbol(Symbol.Less)
            case "~":
                if self.match_char("="):
                    return TokenSymbol(Symbol.NotEqual)
                else:
                    raise RuntimeError("Invalid token.")
            case "(":
                return TokenSymbol(Symbol.LeftParen)
            case ")":
                return TokenSymbol(Symbol.RightParen)
            case "{":
                return TokenSymbol(Symbol.LeftBrace)
            case "}":
                return TokenSymbol(Symbol.RightBrace)
            case "[":
                return TokenSymbol(Symbol.LeftBracket)
            case "]":
                return TokenSymbol(Symbol.RightBracket)
            case ";":
                return TokenSymbol(Symbol.Semicolon)
            case ":":
                return TokenSymbol(Symbol.Colon)
            case ",":
                return TokenSymbol(Symbol.Comma)
            case ".":
                if self.peek().isdigit():
                    return self.scan_number()
                elif self.match_char("."):
                    if self.match_char("."):
                        return TokenSymbol(Symbol.Ellipsis)
                    else:
                        return TokenSymbol(Symbol.Concat)
                else:
                    return TokenSymbol(Symbol.Dot)
            case "\"" | "'":
                return self.scan_string()
            case _:
                ch = self.previous()
                if ch.isdigit():
                    return self.scan_number()
                elif self.is_name_char(ch, True):
                    return self.scan_name()
                else:
                    raise RuntimeError("Invalid token.")


    def scan_comment(self):
        while self.peek() != "\n":
            self.advance()


    def scan_number(self) -> TokenNumber:
        hex = False
        floating = False
        science = False
        if self.previous() == "0" and (self.match_char("x") or self.match_char("X")):
            hex = True
        if self.previous() == ".":
            floating = True
        signed_power = False
        number_power = False
        while True:
            ch = self.peek()
            if ch.isdigit():
                if science:
                    number_power = True
                self.advance()
            elif not hex and (ch == 'e' or ch == 'E'):
                floating = True
                science = True
                self.advance()
            elif ch.lower() in "abcdef":
                if hex:
                    self.advance()
                else:
                    raise RuntimeError("Invalid number.")
            elif ch == ".":
                if hex or floating:
                    raise RuntimeError("Invalid number.")
                else:
                    self.advance()
                    floating = True
            elif (ch == "-" or ch == "+") and science and not signed_power and not number_power:
                signed_power = True
                self.advance()
            else:
                break
        word = self.extract_word()
        if floating:
            return TokenNumber(float(word))
        else:
            if hex:
                return TokenNumber(int(word[2:], 16))
            else:
                return TokenNumber(int(word))


    def scan_string(self) -> TokenString:
        str_tag = self.previous()
        string = ""
        while self.peek() != str_tag:
            if self.match_char("\\"):
                string += self.parse_escape()
            else:
                string += self.advance()
        self.advance()
        return TokenString(string)


    def scan_name(self) -> Token:
        while self.is_name_char(self.peek(), False):
            self.advance()
        word = self.extract_word()
        if word in self.reserved_words:
            return TokenReserved(self.reserved_words[word])
        else:
            return TokenName(word)


    def parse_escape(self) -> str:
        match self.advance():
            case "\\":
                return "\\"
            case "n":
                return "\n"
            case "'":
                return "'"
            case "\"":
                return "\""
            case _:
                raise RuntimeError("Invalid escape char.")


    def extract_word(self) -> str:
        return self.source[self.start:self.current]


    def skip_whitespace(self):
        while self.peek() != "\n" and self.peek().isspace():
            self.advance()


    def align_pointer(self):
        self.start = self.current


    @staticmethod
    def is_name_char(ch: str, start: bool) -> bool:
        if start and (ch.isalpha() or ch == "_"):
            return True
        elif ch.isalnum() or ch == "_":
            return True
        else:
            return False


    def is_at_end(self) -> bool:
        return self.current >= len(self.source)


    def advance(self) -> str:
        if self.is_at_end():
            return "\0"
        else:
            self.current += 1
            return self.source[self.current - 1]


    def previous(self) -> str:
        if self.current == 0:
            return "\0"
        else:
            return self.source[self.current - 1]


    def peek(self) -> str:
        if self.is_at_end():
            return "\0"
        else:
            return self.source[self.current]


    def match_char(self, target: str) -> bool:
        if self.is_at_end():
            return False
        else:
            if self.peek() == target:
                self.advance()
                return True
            else:
                return False


if __name__ == "__main__":
    main()
