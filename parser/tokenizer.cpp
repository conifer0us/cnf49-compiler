// tokenizer.cpp : takes raw string and breaks it down into tokens that will be recognized by the parser
#include <cctype>
#include "tokenizer.h"

Token Tokenizer::peek() {
    if (!cached.has_value()) {
        cached = advanceCurrent();
    }

    return cached.value();
}

Token Tokenizer::next() {
    if (!cached.has_value()) {
        return advanceCurrent();
    } else {
        Token tmp = cached.value();
        cached = std::nullopt;
        return tmp;
    }
}

Token Tokenizer::peekNext() {
    int prev = current;

    Token val = advanceCurrent();

    current = prev;

    return val;
}

Token Tokenizer::advanceCurrent() {
    while (current < text.length() && std::isspace(text.at(current))) {
        current++;
    }

    if (current >= text.length()) {
        return Token{TokenType::ENDOFFILE};
    }

    switch (text.at(current)) {
        case '(': current++; return Token{TokenType::LEFT_PAREN};
        case ')': current++; return Token{TokenType::RIGHT_PAREN};
        case '{': current++; return Token{TokenType::LEFT_BRACE};
        case '}': current++; return Token{TokenType::RIGHT_BRACE};
        case ':': current++; return Token{TokenType::COLON};
        case '!': current++; return Token{TokenType::NOT};
        case '@': current++; return Token{TokenType::ATSIGN};
        case '^': current++; return Token{TokenType::CARET};
        case '&': current++; return Token{TokenType::AMPERSAND};
        case '.': current++; return Token{TokenType::DOT};
        case ',': current++; return Token{TokenType::COMMA};
        case '_': current++; return Token{TokenType::PLACEHOLDER};
        case '\n': current++; return Token{TokenType::NEWLINE};
        case '=': current++; return Token{TokenType::EQUAL};
    
        case '+': current++; return Token{TokenType::OPERATOR, '+'};
        case '-': current++; return Token{TokenType::OPERATOR, '-'};
        case '*': current++; return Token{TokenType::OPERATOR, '*'};
        case '/': current++; return Token{TokenType::OPERATOR, '/'};
        
        default:
            if (std::isdigit(text.at(current))) {
                // This is a digit
                int start = current++;
                while (current < text.length() && std::isdigit(text.at(current))) current ++;
                
                // current now points to the first non-digit character, or past the end of the text
                return Token{TokenType::NUMBER, atoi(text.substr(start,current).c_str())};
            }

            // Now down to keywords and identifiers
            else if (std::isalpha(text.at(current))) {
                int start = current++;
                while (current < text.length() && std::isalnum(text.at(current))) current ++;
                
                // current now points to the first non-alphanumeric character, or past the end of the string
                std::string fragment = text.substr(start,current);
                
                // Unlike the constant parsing switch above, this has already advanced current
                if (fragment == "if") return Token{TokenType::IF};
                else if (fragment == "ifonly") return Token{TokenType::IFONLY};
                else if (fragment == "while") return Token{TokenType::WHILE};
                else if (fragment == "return") return Token{TokenType::RETURN};
                else if (fragment == "print") return Token{TokenType::PRINT};
                else if (fragment == "this") return Token{TokenType::THIS};
                else if (fragment == "else") return Token{TokenType::ELSE};
                else return Token{TokenType::IDENTIFIER, fragment};
            } else {
                throw "Unsupported character: " + text.at(current);
            }
    }
}
