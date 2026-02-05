#pragma once

#include "tokenizer.h"
#include "ASTNodes.h"

class Parser {
private:
    Tokenizer tok;

public:
    Parser(Tokenizer t) :
        tok(t) {};

    ExprPtr parseExpr();
    StmtPtr parseStatement();
    ClassPtr parseClass();
    ProgramPtr parseProgram();
};
