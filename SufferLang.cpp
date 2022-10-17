#include <string>

// Static definitions for language keywords
static const std::string function_def = "declare";
static const std::string external_function = "outside";

// The Token enum holds values to be returned for certain tokens. If the token is not recognized by the get_tok() function, a value from 0-255 is returned
enum Token {
    token_eof = -1, 
    token_def = -2,
    token_external = -3, 
    token_identifier = -4,
    token_double = -5,
    token_int = -6
};

static std::string identifierString;
static double doubleVal;
static int intVal;

// The "Lexer"; Returns the next token from the standard input
static int get_tok() {
    static int lastChar = ' ';

    while (isspace(lastChar)) {
        lastChar = getchar();
    }

    if (isalpha(lastChar)) {
        identifierString = lastChar;
        while (isalnum((lastChar = getchar()))) {
            identifierString += lastChar;
        }
        if (identifierString == function_def) {
            return token_def;
        } else if (identifierString == external_function){
            return token_external;
        }
    }

    else if (isdigit(lastChar) || lastChar == '.') {
        std::string numString = "";
        do {
            numString += lastChar;
            lastChar = getchar();
        } while (isdigit(lastChar) || lastChar == '.');
        if (numString.find(".") > -1) {
            intVal = strtol(numString.c_str(), 0, 10);
            return token_int;
        } else {
            doubleVal = strtod(numString.c_str(), 0);
            return token_double;
        }
    }

    else if (lastChar == '#') {
        do {
            lastChar = getchar();
        } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
        return get_tok();
    }

    if (lastChar == EOF) {
        return token_eof;
    }

    else {
        int currentCharVal = lastChar;
        lastChar = getchar();
        return currentCharVal;
    }
}


// Base class for all expressions in the Abstract Syntax Tree
class ExprAST {
    public:
    virtual ~ExprAST() {}
};

