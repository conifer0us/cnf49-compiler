#pragma once

#include <list>
#include <memory>
#include <vector>
#include "tokenizer.h"

struct Expression {
    virtual ~Expression() = default;
};

struct Statement {
    virtual ~Statement() = default;
};

struct Class {
    virtual ~Class() = default;
};

struct Program {
    virtual ~Program() = default;
};

using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;
using ClassPtr = std::unique_ptr<Class>;
using ProgramPtr = std::unique_ptr<Program>;

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

struct AssignStatement : Statement {
    std::string name;
    ExprPtr value;

    AssignStatement(std::string name, ExprPtr value): 
        name(std::move(name)), value(std::move(value)) {}
};

struct DiscardStatement : Statement {
    ExprPtr expr;

    explicit DiscardStatement(ExprPtr expr):
        expr(std::move(expr)) {}
};

struct FieldAssignStatement : Statement {
    ExprPtr object;
    std::string field;
    ExprPtr value;

    FieldAssignStatement(ExprPtr object, std::string field, ExprPtr value): 
        object(std::move(object)), field(std::move(field)), value(std::move(value)) {}
};

struct IfStatement : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> thenBranch;
    std::vector<StmtPtr> elseBranch;

    IfStatement(ExprPtr condition, std::vector<StmtPtr> thenBranch, std::vector<StmtPtr> elseBranch): 
        condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
};

struct IfOnlyStatement : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> body;

    IfOnlyStatement(ExprPtr condition, std::vector<StmtPtr> body): 
        condition(std::move(condition)), body(std::move(body)) {}
};

struct WhileStatement : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> body;

    WhileStatement(ExprPtr condition, std::vector<StmtPtr> body):
        condition(std::move(condition)), body(std::move(body)) {}
};

struct ReturnStatement : Statement {
    ExprPtr value;

    explicit ReturnStatement(ExprPtr value): 
        value(std::move(value)) {}
};

struct PrintStatement : Statement {
    ExprPtr value;

    explicit PrintStatement(ExprPtr value):
        value(std::move(value)) {}
};

struct ThisExpr : Expression {};

struct Constant : Expression {
    const long value;

    explicit Constant(long val):
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

    Binop(ExprPtr left, char oper, ExprPtr right):
        lhs(std::move(left)), rhs(std::move(right)), op(oper) {}
};

struct FieldRead : Expression {
    const ExprPtr base;
    const std::string fieldname;

    FieldRead(ExprPtr b, std::string fname):
        base(std::move(b)), fieldname(std::move(fname)) {}
};

struct Variable : Expression {
    const std::string name;

    explicit Variable(std::string n):
        name(std::move(n)) {};
};

struct MethodCall : Expression {
    const ExprPtr base;
    const std::string methodname;
    const std::vector<ExprPtr> args;

    MethodCall(ExprPtr b, std::string mname, std::vector<ExprPtr> arglist) :
        base(std::move(b)), methodname(std::move(mname)), args(std::move(arglist)) {}
};
