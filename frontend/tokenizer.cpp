// tokenizer.cpp : takes raw string and breaks it down into tokens that will be recognized by the parser
#include <cctype>
#include <iostream>
#include "tokenizer.h"

Token Tokenizer::peek() {
    if (!cached.has_value()) {
        cached = advanceCurrent();
    }

    return cached.value();
}

Token Tokenizer::next() {
    auto ret = advanceCurrent();
    cached = ret;
    return ret;
}

Token Tokenizer::peekNext() {
    int prev = current;
    Token val = advanceCurrent();
    current = prev;

    return val;
}

unsigned char Tokenizer::curChar() {
    return static_cast<unsigned char>(text.at(current));
}

void Tokenizer::failCurrentLine(std::string error_msg) {
    std::cerr << "At Char: " << curChar() << "\nIn line: \n";

    while (curChar() != '\n') current--;
    current++;
    while (curChar() != '\n') {
        std::cerr << curChar();
        current++;
    }

    std::cerr << "\nMessage given: \n" << error_msg << "\n";

    throw std::runtime_error("Program failed to parse. See error above.");
}

Token Tokenizer::advanceCurrent() {
    while (current < text.length() && curChar() != '\n' && std::isspace(curChar()))
        current++;

    if (current >= text.length())
        return Token{TokenType::ENDOFFILE};

    switch (curChar()) {
        case '(': current++; return Token{TokenType::LEFT_PAREN};
        case ')': current++; return Token{TokenType::RIGHT_PAREN};
        case '{': current++; return Token{TokenType::LEFT_BRACE};
        case '}': current++; return Token{TokenType::RIGHT_BRACE};
        case ':': current++; return Token{TokenType::COLON};
        case '@': current++; return Token{TokenType::ATSIGN};
        case '^': current++; return Token{TokenType::CARET};
        case '&': current++; return Token{TokenType::AMPERSAND};
        case '.': current++; return Token{TokenType::DOT};
        case ',': current++; return Token{TokenType::COMMA};
        case '_': current++; return Token{TokenType::PLACEHOLDER};
        case '\n': current++; return Token{TokenType::NEWLINE};
        case '[': current++; return Token{TokenType::LEFT_BRACKET};
        case ']': current++; return Token{TokenType::RIGHT_BRACKET};
    
        case '+': current++; return Token{TokenType::OPERATOR, '+'};
        case '-': current++; return Token{TokenType::OPERATOR, '-'};
        case '*': current++; return Token{TokenType::OPERATOR, '*'};
        case '/': current++; return Token{TokenType::OPERATOR, '/'};
        case '>': current++; return Token{TokenType::OPERATOR, '>'};
        case '<': current++; return Token{TokenType::OPERATOR, '<'};

        default:
            if (curChar() == '=') {
                current++;

                if (curChar() == '=') {
                    current++;
                    return Token{TokenType::OPERATOR, 'e'};
                }

                return Token{TokenType::EQUAL};
            }

            if (curChar() == '!') {
                current++;

                if (curChar() == '=') {
                    current++;
                    return Token{TokenType::OPERATOR, 'n'};
                }

                return Token{TokenType::NOT};
            }

            if (std::isdigit(curChar())) {
                // This is a digit
                int start = current++;
                while (current < text.length() && std::isdigit(curChar())) current ++;
                
                // current now points to the first non-digit character, or past the end of the text
                return Token{TokenType::NUMBER, atoi(text.substr(start,current).c_str())};
            }

            // Now down to keywords and identifiers
            else if (std::isalpha(static_cast<unsigned char>(curChar()))) {
                int start = current++;
                int substrlen = 1;

                while (current < text.length() && std::isalnum(curChar())) { current++; substrlen++; };
                
                // current now points to the first non-alphanumeric character, or past the end of the string
                std::string fragment = text.substr(start, substrlen);
                
                // Unlike the constant parsing switch above, this has already advanced current
                if (fragment == "if") return Token{TokenType::IF};
                else if (fragment == "ifonly") return Token{TokenType::IFONLY};
                else if (fragment == "while") return Token{TokenType::WHILE};
                else if (fragment == "return") return Token{TokenType::RETURN};
                else if (fragment == "print") return Token{TokenType::PRINT};
                else if (fragment == "this") return Token{TokenType::THIS};
                else if (fragment == "else") return Token{TokenType::ELSE};
                else if (fragment == "class") return Token{TokenType::CLASS};
                else if (fragment == "with") return Token{TokenType::WITH};
                else if (fragment == "method") return Token{TokenType::METHOD};
                else if (fragment == "fields") return Token{TokenType::FIELDS};
                else if (fragment == "locals") return Token{TokenType::LOCALS};
                else return Token{TokenType::IDENTIFIER, fragment};
            } else {
                std::cerr << "Tokenizer caught unsupported character: " << curChar();
            }
            
            return Token{};
    }
}
