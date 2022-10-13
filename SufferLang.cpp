#include <string>

// The Token enum holds values to be returned for certain tokens. If the token is not recognized by the get_tok() function, a value from 0-255 is returned
enum Token {
    token_eof = -1, 
    token_def = -2,
    tok_extern = -3, 
    tok_identifier = -4,
    tok_number = -5
};

static std::string IdentifierString;
static double doubleVal;
static int intVal;

static int get_tok() {

}