#pragma once

#include <string>
#include <optional>
#include <variant>

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
    PLACEHOLDER,
    NEWLINE,
    EQUAL,

    // Keywords
    THIS,
    IF,
    IFONLY,
    WHILE,
    RETURN,
    PRINT,
    ENDOFFILE,
    ELSE,

    // Tokens with data
    OPERATOR,
    NUMBER,
    IDENTIFIER
};

struct Token {
    TokenType type;
    std::variant<std::monostate, int, char, std::string> value;
};

class Tokenizer {
private:
    std::string text;
    
    int current = 0;
    std::optional<Token> cached;

public:
    explicit Tokenizer(std::string t) :
        text(std::move(t)) {};

    Token peek();
    Token next();
    Token peekNext();
    Token advanceCurrent(); 
};
