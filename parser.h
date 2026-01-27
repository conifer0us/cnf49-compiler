// PARSER.CPP : Takes raw language file and, using recursive descent, returns an AST
#include <string>
#include <optional>

enum TokenType { 
    // Fixed punctuation
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    CARET,
    AMPERSAND,
    ATSIGN,
    NOT,
    DOT,
    COLON,
    COMMA,

    // Keywords
    THIS,
    IF,
    IFONLY,
    WHILE,
    RETURN,
    PRINT,
    ENDOFFILE,

    // Tokens with data
    OPERATOR,
    NUMBER,
    IDENTIFIER
};

class Tokenizer {
private:
    // We'll pre-allocate and reuse common tokens without data
    static LeftParen lp;
    static RightParen rp;
    static LeftBrace lb;
    static RightBrace rb;
    static Colon colon;
    static Print print;
    static Return ret;
    static While w;
    static If iff;
    static IfOnly ifonly;
    static Not nott;
    static AtSign at;
    static Caret caret;
    static Ampersand amp;
    static Dot dot;
    static Eof eof;
    static This th;
    static Comma comma;

    static std::string text;
    
    int current;
    std::optional<Token> cached;

public:
    Tokenizer(std::string t) {
        text = t;
        current = 0;
    }

    Token peek();
    Token next();
    void advanceCurrent(); 
};

class Token {
private:
    TokenType type;
public:
    Token(TokenType t) {
        type = t;
    };

    virtual TokenType getType() {
        return type;
    }
};

class Number : public Token {
public:
    int val;

    Number(int value) : Token(NUMBER) {
        val = value;
    }
};

class LeftParen : public Token {
    LeftParen() : Token(LEFT_PAREN) {}
};

class RightParen : public Token {
    RightParen() : Token(RIGHT_PAREN) {}
};

class LeftBrace : public Token {
    LeftBrace() : Token(LEFT_BRACE) {}
};

class RightBrace : public Token {
    RightBrace() : Token(RIGHT_BRACE) {}
};

class Operator : public Token {
public:
    char op;

    Operator(char oper) : Token(OPERATOR) {
        op = oper;
    }
};

class Caret : public Token {
    Caret() : Token(CARET) {}
};

class Ampersand : public Token {
    Ampersand() : Token(AMPERSAND) {}
};

class AtSign : public Token {
    AtSign() : Token(ATSIGN) {}
};

class Not : public Token {
    Not() : Token(NOT) {}
};

class Dot : public Token {
    Dot() : Token(DOT) {}
};

class If : public Token {
    If() : Token(IF) {}
};

class IfOnly : public Token {
    IfOnly() : Token(IFONLY) {}
};

class While : public Token {
    While() : Token(WHILE) {}
};

class Return : public Token {
    Return() : Token(RETURN) {}
};

class Print : public Token {
    Print() : Token(PRINT) {}
};

class Colon : public Token {
    Colon() : Token(COLON) {}
};

class Comma : public Token {
    Comma() : Token(COMMA) {}
};

class Eof : public Token {
    Eof() : Token(ENDOFFILE) {}
};

class Identifier : public Token {
    std::string name;

    Identifier(std::string id) : Token(IDENTIFIER) {
        name = id;
    }
};

class This : public Token {
    This() : Token(THIS) {}
};
