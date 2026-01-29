#pragma once

#include <list>
#include <memory>
#include <vector>
#include "tokenizer.h"

class Parser {
private:
    Tokenizer tok;

public:
    Parser(Tokenizer t) :
        tok(t) {};

    Expression parseExpr();
};

using ExprPtr = std::shared_ptr<Expression>;

struct Expression {
    virtual ~Expression() = default;
};

struct ThisExpr : Expression {};

struct Constant : Expression {
    const long value;

    explicit Constant(long val) :
        value(val) {}
};

struct ClassRef : Expression {
    const std::string classname;

    explicit ClassRef(std::string cname):
        classname(std::move(cname)) {}
};

struct Binop : Expression {
    const ExprPtr lhs;
    const ExprPtr rhs;
    const char op;

    explicit Binop(ExprPtr left, char oper, ExprPtr right) :
        lhs(std::move(left)), rhs(std::move(right)), op(oper) {}
};

struct FieldRead : Expression {
    const ExprPtr base;
    const std::string fieldname;

    explicit FieldRead(ExprPtr b, std::string fname) :
        base(std::move(b)), fieldname(std::move(fname)) {}
};

struct Variable : Expression {
    const std::string name;

    explicit Variable(std::string n) :
        name(std::move(n)) {};
};

struct MethodCall : Expression {
    const ExprPtr base;
    const std::string methodname;
    const std::vector<ExprPtr> args;

    explicit MethodCall(ExprPtr b, std::string mname, std::vector<ExprPtr> arglist) :
        base(std::move(b)), methodname(std::move(mname)), args(std::move(arglist)) {}
};