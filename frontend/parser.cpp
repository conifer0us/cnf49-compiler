#include "parser.h"

ExprPtr Parser::parseExpr() {
    switch (tok.next().type) {
        case ENDOFFILE: tok.failCurrentLine("No expression to parse: EOF");
        case NUMBER: {
            auto curtok = tok.peek().value;
            auto num = std::get_if<int>(&curtok);
            if (num)
                return std::make_unique<Constant>(*num);
            else
                tok.failCurrentLine("Parser failed parsing num variant (bad initialization)");
        }
        case IDENTIFIER: {
            auto curtok = tok.peek().value;
            auto ch = std::get_if<std::string>(&curtok);
            if (ch)
                return std::make_unique<Var>(*ch);
            else
                tok.failCurrentLine("Parser failed parsing variable name variant (bad initialization)"); 
        } 
        case LEFT_PAREN: {
            // Should be start of a binary operation
            ExprPtr lhs = parseExpr();
            Token optok = tok.next();
            if (optok.type != OPERATOR)
                tok.failCurrentLine("Expected operator");
            ExprPtr rhs = parseExpr();
            Token closetok = tok.next();
            if (closetok.type != RIGHT_PAREN)
                tok.failCurrentLine("Expected right paren");

            auto curtok = std::get_if<char>(&optok.value);
            if (curtok)
                return std::make_unique<Binop>(std::move(lhs), *curtok, std::move(rhs));
            else
                tok.failCurrentLine("Parser failed parsing char variant (bad initialization)");
        }
        case AMPERSAND: {
            // Should be field read
            ExprPtr base = parseExpr();
            Token dot = tok.next();
            if (dot.type != DOT)
                tok.failCurrentLine("Expected dot");
            Token fname = tok.next();
            if (fname.type != IDENTIFIER)
                tok.failCurrentLine("Expected valid field");

            auto curtok = std::get_if<std::string>(&fname.value);
            if (curtok)
                return std::make_unique<FieldRead>(std::move(base), *curtok);
            else
                tok.failCurrentLine("Parser failed parsing fieldread variant (bad initialization)");
        }
        case CARET: {
            ExprPtr mbase = parseExpr();
            
            Token mdot = tok.next();
            if (mdot.type != DOT)
                tok.failCurrentLine("Expected dot");
            Token mname = tok.next();
            if (mname.type != IDENTIFIER)
                tok.failCurrentLine("Expected valid method name");
            Token open = tok.next();
            if (open.type != LEFT_PAREN)
                tok.failCurrentLine("Expected left paren");
            
            std::vector<ExprPtr> args = {};
            while (tok.peekNext().type != RIGHT_PAREN) {
                ExprPtr e = parseExpr();
                args.push_back(std::move(e));

                // Now either a paren or a comma
                Token punc = tok.peekNext();
                if (punc.type == COMMA)
                    tok.next(); // throw away the comma
            }
            
            tok.next();

            auto curtok = std::get_if<std::string>(&mname.value);
            if (curtok)
                return std::make_unique<MethodCall>(std::move(mbase), *curtok, std::move(args));
            else
                tok.failCurrentLine("Parser failed parsing method call variant (bad initialization)");
        }
        case ATSIGN: {
            Token cname = tok.next();
            if (cname.type != IDENTIFIER)
                tok.failCurrentLine("Expected valid class name");
            
            auto curtok = std::get_if<std::string>(&cname.value);
            if (curtok)
                return std::make_unique<ClassRef>(*curtok);
            else
                tok.failCurrentLine("Parser failed parsing classref variant (bad initialization)");
        }
        case THIS: return std::make_unique<ThisExpr>();
        default: 
            tok.failCurrentLine("Unexpected character; failed to parse expression.");
    }

    throw std::runtime_error("Failed to parse expression (something is very wrong if this is being seen)");
}

StmtPtr Parser::parseStatement() {
    switch (tok.next().type)
    {
    case IDENTIFIER: {
        auto curtok = tok.peek();
        auto ptr = std::get_if<std::string>(&curtok.value);
        if (!ptr)
            tok.failCurrentLine("Parser failed parsing Variable Assignment variant (bad initialization)");

        auto name = *ptr;

        if (tok.next().type != EQUAL)
            tok.failCurrentLine("Expected =");    
        
        return std::make_unique<AssignStatement>(name, std::move(parseExpr()));
    }    
    case PLACEHOLDER:
        if (tok.next().type != EQUAL)
            tok.failCurrentLine("Expected =");    
        
        return std::make_unique<DiscardStatement>(std::move(parseExpr()));
    case NOT: {
        ExprPtr obj = parseExpr();
        
        if (tok.next().type != DOT)
            tok.failCurrentLine("Expected .");    
    
        if (tok.next().type != IDENTIFIER)
            tok.failCurrentLine("Expected identifier");    
    
        auto curtok = tok.peek();
        auto ptr = std::get_if<std::string>(&curtok.value);
        if (!ptr)
            tok.failCurrentLine("Parser failed parsing Field Assignment variant (bad initialization)");

        auto name = *ptr;

        if (tok.next().type != EQUAL)
            tok.failCurrentLine("Expected =");    
    
        return std::make_unique<FieldAssignStatement>(std::move(obj), name, std::move(parseExpr()));
    }
    case IF: {
        ExprPtr cond = parseExpr();
        
        if (tok.next().type != COLON || tok.next().type != LEFT_BRACE
                || tok.next().type != NEWLINE)
            tok.failCurrentLine("Expected ': { \\n' to start if statement");    

        std::vector<StmtPtr> trueCond;

        do {
            trueCond.push_back(std::move(parseStatement()));
        } while(tok.next().type == NEWLINE && tok.peekNext().type != RIGHT_BRACE);

        if (tok.next().type != RIGHT_BRACE || tok.next().type != ELSE || tok.next().type != LEFT_BRACE || tok.next().type != NEWLINE)
            tok.failCurrentLine("Expected '} else {' to separate if/else conditional");    

        std::vector<StmtPtr> falseCond;

        do {
            falseCond.push_back(std::move(parseStatement()));
        } while (tok.next().type == NEWLINE && tok.peekNext().type != RIGHT_BRACE);

        tok.next();

        return std::make_unique<IfStatement>(std::move(cond), std::move(trueCond), std::move(falseCond));
    }
    case IFONLY: {
        ExprPtr cond = parseExpr();

        if (tok.next().type != COLON || tok.next().type != LEFT_BRACE || tok.next().type != NEWLINE)
            tok.failCurrentLine("Expected ': {\\n' to start ifonly statement");

        std::vector<StmtPtr> truecond;

        do {
            truecond.push_back(std::move(parseStatement()));
        } while (tok.next().type == NEWLINE && tok.peekNext().type != RIGHT_BRACE);

        tok.next();

        return std::make_unique<IfOnlyStatement>(std::move(cond), std::move(truecond));
    }
    case WHILE: {
        ExprPtr cond = parseExpr();

        if (tok.next().type != COLON || tok.next().type != LEFT_BRACE || tok.next().type != NEWLINE)
            tok.failCurrentLine("Expected ': {\\n' to start while statement statement");
        
        std::vector<StmtPtr> body;

        do {
            body.push_back(std::move(parseStatement()));
        } while (tok.next().type == NEWLINE && tok.peekNext().type != RIGHT_BRACE);

        tok.next();

        return std::make_unique<WhileStatement>(std::move(cond), std::move(body));
    }
    case RETURN:
        return std::make_unique<ReturnStatement>(std::move(parseExpr()));
    case PRINT: {
        if (tok.next().type != LEFT_PAREN)
            tok.failCurrentLine("Expected ( to start print statement");
        
        ExprPtr e = parseExpr();

        if (tok.next().type != RIGHT_PAREN)
            tok.failCurrentLine("Expected ) after print statement");

        return std::make_unique<PrintStatement>(std::move(e));
    }
    default: tok.failCurrentLine("Unexpected character; failed to parse statement");
    }

    throw std::runtime_error("Failed to parse statement (something is very wrong if this is being seen)");
}

ClassPtr Parser::parseClass() {
    if (tok.next().type != IDENTIFIER)
        tok.failCurrentLine("Class name expected after class keyword");

    auto curtok = tok.peek();
    auto ptr = std::get_if<std::string>(&curtok.value);
    if (!ptr)
        tok.failCurrentLine("Parser failed parsing Class Name variant (bad initialization)");
    
    auto name = *ptr;

    std::vector<VarPtr> fptr = {};
    if (tok.next().type != LEFT_BRACKET || tok.next().type != NEWLINE)
        tok.failCurrentLine("Expected '[\\n' after class declaration");

    if (tok.next().type == FIELDS) {
        do {
            if (tok.next().type != IDENTIFIER) {
                if (tok.peek().type == NEWLINE)
                    break;

                tok.failCurrentLine("Expected identifier for class field names");
            }
                
            curtok = tok.peek();
            auto fname = std::get_if<std::string>(&curtok.value);
            if (!fname)
                tok.failCurrentLine("Parser failed parsing Field Name variant (bad initialization)");

            fptr.push_back(std::make_unique<Var>(*fname));
        } while(tok.next().type == COMMA);

        if (tok.peek().type != NEWLINE)
            tok.failCurrentLine("Expected a newline after field definition in class definition");
    }

    std::vector<MethodPtr> mptr = {};
    while (tok.peekNext().type == METHOD) {
        tok.next();
        if (tok.next().type != IDENTIFIER)
            tok.failCurrentLine("Expected method name after method keyword");

        curtok = tok.peek();
        auto ptr = std::get_if<std::string>(&curtok.value);
        if (!ptr)
            tok.failCurrentLine("Parser failed parsing Method Name variant (bad initialization)");

        auto mname = *ptr;

        if (tok.next().type != LEFT_PAREN)
            tok.failCurrentLine("Expected ( after method declaration");

        std::vector<VarPtr> args;
        args.push_back(std::move(std::make_unique<Var>("this")));

        do {
            if (tok.next().type != IDENTIFIER) { 
                if (tok.peek().type == RIGHT_PAREN)
                    break;

                tok.failCurrentLine("Expected identifier for method argument names");
            }

            curtok = tok.peek();
            auto aname = std::get_if<std::string>(&curtok.value);
            if (!aname)
                tok.failCurrentLine("Parser failed parsing Method Argument variant (bad initialization)");

            args.push_back(std::make_unique<Var>(*aname));
        } while(tok.next().type == COMMA);

        if (tok.peek().type != RIGHT_PAREN)
            tok.failCurrentLine("Expected ) to close method arguments");

        std::vector<VarPtr> locals = {};

        if (tok.next().type == WITH) {
            if (tok.next().type != LOCALS)
                tok.failCurrentLine("'with locals' expected after method to define local variables");
            
            do {
                if (tok.next().type != IDENTIFIER) {
                    if (tok.peek().type == COLON)
                        break;

                    tok.failCurrentLine("Expected identifier for method locals");
                }

                curtok = tok.peek();
                auto lname = std::get_if<std::string>(&curtok.value);
                if (!lname)
                    tok.failCurrentLine("Parser failed parsing Method Locals variant (bad initialization)");

                locals.push_back(std::make_unique<Var>(*lname));
            } while(tok.next().type == COMMA);
        }

        if (tok.peek().type != COLON || tok.next().type != NEWLINE)
            tok.failCurrentLine("Expected ':\\n' after declaration of method locals");
            
        if (locals.size() > 6)
            tok.failCurrentLine("Method declarations allowed with 0-6 variable arguments");

        std::vector<StmtPtr> statements = {};

        do {
            statements.push_back(std::move(parseStatement()));
        } while (tok.next().type == NEWLINE && tok.peekNext().type != RIGHT_BRACKET && tok.peekNext().type != METHOD);

        mptr.push_back(std::make_unique<Method>(mname, std::move(args), std::move(locals), std::move(statements)));
    }
    
    if (tok.next().type != RIGHT_BRACKET && tok.next().type != NEWLINE)
        tok.failCurrentLine("Expected ']\\n' to close class definition");

    return std::make_unique<Class>(name, std::move(fptr), std::move(mptr));
}

ProgramPtr Parser::parseProgram() {
    std::vector<ClassPtr> classes = {};
    
    while (tok.next().type == CLASS || tok.peek().type == NEWLINE) {
        if (tok.peek().type == NEWLINE) continue;

        classes.push_back(parseClass());
    }

    auto curtok = tok.peek();
    auto ptr = std::get_if<std::string>(&curtok.value);
    if (!ptr)
        tok.failCurrentLine("Parser failed parsing main method name variant (bad initialization)");

    auto mname = *ptr;

    if (tok.peek().type != IDENTIFIER || mname != "main")
        tok.failCurrentLine("Expected main declaration after class definitions");

    std::vector<VarPtr> args = {};
    std::vector<VarPtr> locals = {};

    if (tok.next().type != WITH)
        tok.failCurrentLine("'with' expected after main to define local variables");
        
    do {
        if (tok.next().type != IDENTIFIER)
            tok.failCurrentLine("Expected identifier for method locals");

        curtok = tok.peek();
        auto lname = std::get_if<std::string>(&curtok.value);
        if (!lname)
            tok.failCurrentLine("Parser failed parsing Assignment variant (bad initialization)");

        locals.push_back(std::make_unique<Var>(*lname));
    } while(tok.next().type == COMMA);

    if (tok.peek().type != COLON || tok.next().type != NEWLINE)
        tok.failCurrentLine("Expected ':\\n' after declaration of main locals");
        
    std::vector<StmtPtr> statements = {};

    do {
        statements.push_back(std::move(parseStatement()));
    } while (tok.next().type == NEWLINE);

    MethodPtr m = std::make_unique<Method>(mname, std::move(args), std::move(locals), std::move(statements));
    return std::make_unique<Program>(std::move(m), std::move(classes));
}
