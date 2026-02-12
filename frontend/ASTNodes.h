#pragma once 

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include "ir.h"

// forward declare IRBuilder because I didn't design this with a pattern like I clearly should have
struct IRBuilder;

struct ASTNode {
    virtual ~ASTNode();
    virtual void print(int ind) const;
};

inline void indent(int n) {
    while (n--) std::cout << ' ';
}

struct Expression : ASTNode {
    virtual ~Expression();
    virtual ValPtr convertToIR(IRBuilder& builder, Local* out) const;
};

using ExprPtr = std::unique_ptr<Expression>;

struct ThisExpr : Expression {
    void print(int ind) const override {
        indent(ind);
        std::cout << "this\n";
    }

    ValPtr convertToIR(IRBuilder& builder, Local *out = nullptr) const override;
};

struct Constant : Expression {
    const long value;

    void print(int ind) const override {
        indent(ind);
        std::cout << value << "\n";
    }
        
    ValPtr convertToIR(IRBuilder& builder, Local *out = nullptr) const override;
    
    explicit Constant(long val):
        value(val) {}
};

struct ClassRef : Expression {
    const std::string classname;

    void print(int ind) const override {
        indent(ind);
        std::cout << "ClassRef (" << classname << ")\n";
    }

    ValPtr convertToIR(IRBuilder& builder, Local *out = nullptr) const override;
    
    explicit ClassRef(std::string cname):
        classname(std::move(cname)) {}
};

struct Binop : Expression {
    const ExprPtr lhs;
    const ExprPtr rhs;
    const char op;

    void print(int ind) const override {
        indent(ind);
        std::cout << op << "\n";
        lhs->print(ind + 2);
        
        indent(ind);
        std::cout << "AND\n";
        rhs->print(ind + 2);
    }

    ValPtr convertToIR(IRBuilder& builder, Local *out = nullptr) const override;
    
    Binop(ExprPtr left, char oper, ExprPtr right):
        lhs(std::move(left)), rhs(std::move(right)), op(oper) {}
};

struct FieldRead : Expression {
    const ExprPtr base;
    const std::string fieldname;

    void print(int ind) const override {
        indent(ind);
        std::cout << "field read from:\n";
        
        base->print(ind + 2);
        indent(ind);
        std::cout << "to field" << fieldname << "\n";
    }

    ValPtr convertToIR(IRBuilder& builder, Local *out = nullptr) const override;
    
    FieldRead(ExprPtr b, std::string fname):
        base(std::move(b)), fieldname(std::move(fname)) {}
};

struct Var : Expression {
    const std::string name;

    void print(int ind) const override {
        indent(ind);
        std::cout << name << "\n";
    }

    ValPtr convertToIR(IRBuilder& builder, Local *out = nullptr) const override;
    
    explicit Var(std::string n):
        name(std::move(n)) {};
};

using VarPtr = std::unique_ptr<Var>;

struct MethodCall : Expression {
    const ExprPtr base;
    const std::string methodname;
    const std::vector<ExprPtr> args;

    void print(int ind) const override {
        indent(ind);
        std::cout << "call into class:\n";
        base->print(ind + 2);

        indent(ind);
        std::cout << "method " << methodname << "\n";

        for (const auto& arg : args) {
            indent(ind);
            arg->print(ind + 2);
        }

        indent(ind);
        std::cout << "END ARGS\n";
    }

    ValPtr convertToIR(IRBuilder& builder, Local *out = nullptr) const override;
    
    MethodCall(ExprPtr b, std::string mname, std::vector<ExprPtr> arglist) :
        base(std::move(b)), methodname(std::move(mname)), args(std::move(arglist)) {}
};

struct Statement : ASTNode {
    virtual ~Statement();
    virtual void convertToIR(IRBuilder& builder) const;
};

using StmtPtr = std::unique_ptr<Statement>;

struct AssignStatement : Statement {
    std::string name;
    ExprPtr value;

    void print(int ind) const override {
        indent(ind);
        std::cout << "AssignStatement\n";

        indent(ind + 2);
        std::cout << "Variable: " << name << '\n';

        indent(ind + 2);
        std::cout << "Value:\n";
        value->print(ind + 4);
    }

    void convertToIR(IRBuilder& builder) const override;
    
    AssignStatement(std::string name, ExprPtr value): 
        name(std::move(name)), value(std::move(value)) {}
};

struct DiscardStatement : Statement {
    ExprPtr expr;

    void print(int ind) const override {
        indent(ind);
        std::cout << "DiscardStatement\n";

        indent(ind + 2);
        std::cout << "Expression:\n";
        expr->print(ind + 4);
    }

    void convertToIR(IRBuilder& builder) const override;
    
    explicit DiscardStatement(ExprPtr expr): 
        expr(std::move(expr)) {}
};

struct FieldAssignStatement : Statement {
    ExprPtr object;
    std::string field;
    ExprPtr value;

    void print(int ind) const override {
        indent(ind);
        std::cout << "FieldAssignStatement\n";

        indent(ind + 2);
        std::cout << "Object:\n";
        object->print(ind + 4);

        indent(ind + 2);
        std::cout << "Field: " << field << '\n';

        indent(ind + 2);
        std::cout << "Value:\n";
        value->print(ind + 4);
    }

    void convertToIR(IRBuilder& builder) const override;
    
    FieldAssignStatement(ExprPtr object, std::string field, ExprPtr value): 
        object(std::move(object)), field(std::move(field)), value(std::move(value)) {}
};

struct IfStatement : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> thenBranch;
    std::vector<StmtPtr> elseBranch;

    void print(int ind) const override {
        indent(ind);
        std::cout << "IfStatement\n";

        indent(ind + 2);
        std::cout << "Condition:\n";
        condition->print(ind + 4);

        indent(ind + 2);
        std::cout << "Then Branch:\n";
        for (const auto& stmt : thenBranch)
            stmt->print(ind + 4);

        if (!elseBranch.empty()) {
            indent(ind + 2);
            std::cout << "Else Branch:\n";
            for (const auto& stmt : elseBranch)
                stmt->print(ind + 4);
        }
    }

    void convertToIR(IRBuilder& builder) const override;
    
    IfStatement(ExprPtr condition, std::vector<StmtPtr> thenBranch, std::vector<StmtPtr> elseBranch): 
        condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
};

struct IfOnlyStatement : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> body;

    void print(int ind) const override {
        indent(ind);
        std::cout << "IfOnlyStatement\n";

        indent(ind + 2);
        std::cout << "Condition:\n";
        condition->print(ind + 4);

        indent(ind + 2);
        std::cout << "Body:\n";
        for (const auto& stmt : body)
            stmt->print(ind + 4);
    }

    void convertToIR(IRBuilder& builder) const override;
    
    IfOnlyStatement(ExprPtr condition, std::vector<StmtPtr> body): 
        condition(std::move(condition)), body(std::move(body)) {}
};

struct WhileStatement : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> body;

    void print(int ind) const override {
        indent(ind);
        std::cout << "WhileStatement\n";

        indent(ind + 2);
        std::cout << "Condition:\n";
        condition->print(ind + 4);

        indent(ind + 2);
        std::cout << "Body:\n";
        for (const auto& stmt : body)
            stmt->print(ind + 4);
    }

    void convertToIR(IRBuilder& builder) const override;
    
    WhileStatement(ExprPtr condition, std::vector<StmtPtr> body): 
        condition(std::move(condition)), body(std::move(body)) {}
};

struct ReturnStatement : Statement {
    ExprPtr value;

    void print(int ind) const override {
        indent(ind);
        std::cout << "ReturnStatement\n";

        indent(ind + 2);
        std::cout << "Value:\n";
        value->print(ind + 4);
    }

    void convertToIR(IRBuilder& builder) const override;
    
    explicit ReturnStatement(ExprPtr value): 
        value(std::move(value)) {}
};

struct PrintStatement : Statement {
    ExprPtr value;

    void print(int ind) const override {
        indent(ind);
        std::cout << "PrintStatement\n";

        indent(ind + 2);
        std::cout << "Value:\n";
        value->print(ind + 4);
    }

    void convertToIR(IRBuilder& builder) const override;
    
    explicit PrintStatement(ExprPtr value): 
        value(std::move(value)) {}
};

struct Method : ASTNode {
    std::string name;
    std::vector<VarPtr> args;
    std::vector<VarPtr> locals;
    std::vector<StmtPtr> body;

    Method(std::string nm, std::vector<VarPtr> arg, std::vector<VarPtr> lcls, std::vector<StmtPtr> bdy): 
        name(std::move(nm)), args(std::move(arg)), locals(std::move(lcls)), body(std::move(bdy)) {}

    std::shared_ptr<MethodIR> convertToIR(std::string classname, 
        std::map<std::string, std::unique_ptr<ClassMetadata>>& cls,
        std::vector<std::string>& mem, 
        std::vector<std::string>& mthd,
        bool pinhole,
        bool mainmethod) const;

    void print(int ind) const override {
        indent(ind);
        std::cout << "Method: " << name << "\n";

        indent(ind + 2);
        std::cout << "Arguments (" << args.size() << "):\n";
        for (const auto& arg : args) {
            indent(ind + 4);
            std::cout << "- " << arg->name << "\n";
        }

        indent(ind + 2);
        std::cout << "Locals (" << locals.size() << "):\n";
        for (const auto& local : locals) {
            indent(ind + 4);
            std::cout << "- " << local->name << "\n";
        }

        indent(ind + 2);
        std::cout << "Body (" << body.size() << " statements):\n";
        for (const auto& stmt : body)
            stmt->print(ind + 4);
    }
};

using MethodPtr = std::unique_ptr<Method>;

struct Class : ASTNode {
    std::string name;
    std::vector<VarPtr> fields;
    std::vector<MethodPtr> methods;

    Class(std::string n, std::vector<VarPtr> f, std::vector<MethodPtr> m): 
        name(std::move(n)), fields(std::move(f)), methods(std::move(m)) {}

    void convertToIR() const;

    void print(int ind) const override {
        indent(ind);
        std::cout << "Class: " << name << "\n";

        indent(ind + 2);
        std::cout << "Fields (" << fields.size() << "):\n";
        for (const auto& field : fields) {
            indent(ind + 4);
            std::cout << "- " << field->name << "\n";
        }

        indent(ind + 2);
        std::cout << "Methods (" << methods.size() << "):\n";
        for (const auto& method : methods)
            method->print(ind + 4);
    }
};

using ClassPtr = std::unique_ptr<Class>;

struct Program : ASTNode {
    MethodPtr main;
    std::vector<ClassPtr> classes;

    Program(MethodPtr mainmethod, std::vector<ClassPtr> classlist)
        : main(std::move(mainmethod)), classes(std::move(classlist)) {}

    std::unique_ptr<CFG> convertToIR(bool pinhole = true) const;

    void print(int ind) const override {
        indent(ind);
        std::cout << "Program\n";

        indent(ind + 2);
        std::cout << "Main Method:\n";
        main->print(ind + 4);

        indent(ind + 2);
        std::cout << "Classes (" << classes.size() << "):\n";
        for (const auto& cls : classes)
            cls->print(ind + 4);
    }
};

using ProgramPtr = std::unique_ptr<Program>;
