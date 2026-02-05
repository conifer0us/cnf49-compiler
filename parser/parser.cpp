#include "parser.h"

ExprPtr Parser::parseExpr() {
    switch (tok.next().type) {
        case ENDOFFILE: throw std::runtime_error("No expression to parse: EOF");
        case NUMBER: return std::make_unique<Constant>(std::get<int>(tok.peek().value));
        case IDENTIFIER: return std::make_unique<Variable>(std::get<std::string>(tok.peek().value));
        case LEFT_PAREN: {
            // Should be start of a binary operation
            ExprPtr lhs = parseExpr();
            Token optok = tok.next();
            if (optok.type != OPERATOR)
                throw std::runtime_error("Expected operator but found " + optok.type);
            ExprPtr rhs = parseExpr();
            Token closetok = tok.next();
            if (closetok.type != RIGHT_PAREN)
                throw std::runtime_error("Expected right paren but found " + closetok.type);
            return std::make_unique<Binop>(std::move(lhs), std::get<char>(optok.value), std::move(rhs));
        }
        case AMPERSAND: {
            // Should be field read
            ExprPtr base = parseExpr();
            Token dot = tok.next();
            if (dot.type != DOT)
                throw std::runtime_error("Expected dot but found " + dot.type);
            Token fname = tok.next();
            if (fname.type != IDENTIFIER)
                throw std::runtime_error("Expected valid field name but found " + fname.type);
            return std::make_unique<FieldRead>(std::move(base), std::get<std::string>(fname.value));
        }
        case CARET: {
            // Should be method call
            ExprPtr mbase = parseExpr();
            
            Token mdot = tok.next();
            if (mdot.type != DOT)
                throw std::runtime_error("Expected dot but found " + mdot.type);
            Token mname = tok.next();
            if (mname.type != IDENTIFIER)
                throw std::runtime_error("Expected valid method name but found " + mname.type);
            Token open = tok.next();
            if (open.type != LEFT_PAREN)
                throw std::runtime_error("Expected left paren but found " + open.type);
            
            // Now we iterate through arguments
            std::vector<ExprPtr> args = {};
            while (tok.peek().type != RIGHT_PAREN) {
                ExprPtr e = parseExpr();
                args.push_back(std::move(e));

                // Now either a paren or a comma
                Token punc = tok.peek();
                if (punc.type == COMMA)
                    tok.next(); // throw away the comma
            }
            
            return std::make_unique<MethodCall>(std::move(mbase), std::get<std::string>(mname.value), std::move(args));
        }
        case ATSIGN: {
            Token cname = tok.next();
            if (cname.type != IDENTIFIER)
                throw std::runtime_error("Expected valid class name but found: " + cname.type);
            return std::make_unique<ClassRef>(std::get<std::string>(cname.value));
        }
        case THIS: return std::make_unique<ThisExpr>();
        default: throw std::runtime_error("Token type is not a valid start of an expression: " + tok.peek().type);
    }
}

StmtPtr Parser::parseStatement() {
    switch (tok.next().type)
    {
    case IDENTIFIER: {
        std::string name = std::get<std::string>(tok.peek().value);
        
        if (tok.next().type != EQUAL)
            throw std::runtime_error("Expected = but found " + tok.peek().type);    
        
        return std::make_unique<AssignStatement>(name, std::move(parseExpr()));
    }    
    case PLACEHOLDER:
        if (tok.next().type != EQUAL)
            throw std::runtime_error("Expected = but found " + tok.peek().type);    
        
        return std::make_unique<DiscardStatement>(std::move(parseExpr()));
    case NOT: {
        ExprPtr obj = parseExpr();
        
        if (tok.next().type != DOT)
            throw std::runtime_error("Expected . but found " + tok.peek().type);    
    
        if (tok.next().type != IDENTIFIER)
            throw std::runtime_error("Expected identifier but found " + tok.peek().type);    
    
        std::string name = std::get<std::string>(tok.peek().value);

        if (tok.next().type != EQUAL)
            throw std::runtime_error("Expected = but found " + tok.peek().type);    
    
        return std::make_unique<FieldAssignStatement>(std::move(obj), name, std::move(parseExpr()));
    }
    case IF: {
        ExprPtr cond = parseExpr();
        
        if (tok.next().type != COLON || tok.next().type != LEFT_BRACE
                || tok.next().type != NEWLINE)
            throw std::runtime_error("Expected ': { \\n' to start if statement");    

        std::vector<StmtPtr> trueCond;

        do {
            trueCond.push_back(std::move(parseStatement()));
        } while(tok.peekNext().type != RIGHT_BRACE);

        if (tok.next().type != RIGHT_BRACE || tok.next().type != ELSE 
                || tok.next().type != LEFT_BRACE || tok.next().type != NEWLINE)
            throw std::runtime_error("Expected '} else {' to separate if/else conditional");    

        std::vector<StmtPtr> falseCond;

        do {
            falseCond.push_back(std::move(parseStatement()));
        } while(tok.peekNext().type != RIGHT_BRACE);

        return std::make_unique<IfStatement>(std::move(cond), std::move(trueCond), std::move(falseCond));
    }
    case IFONLY: {
        ExprPtr cond = parseExpr();

        if (tok.next().type != COLON || tok.next().type != LEFT_BRACE || tok.next().type != NEWLINE)
            throw std::runtime_error("Expected ': {\\n' to start ifonly statement");
        
        std::vector<StmtPtr> truecond;

        do {
            truecond.push_back(std::move(parseStatement()));
        } while (tok.peekNext().type != RIGHT_BRACE);

        return std::make_unique<IfOnlyStatement>(std::move(cond), std::move(truecond));
    }
    case WHILE: {
        ExprPtr cond = parseExpr();

        if (tok.next().type != COLON || tok.next().type != LEFT_BRACE || tok.next().type != NEWLINE)
            throw std::runtime_error("Expected ': {\\n' to start while statement statement");
        
        std::vector<StmtPtr> body;

        do {
            body.push_back(std::move(parseStatement()));
        } while (tok.peekNext().type != RIGHT_BRACE);

        return std::make_unique<WhileStatement>(std::move(cond), std::move(body));
    }
    case RETURN:
        return std::make_unique<ReturnStatement>(std::move(parseExpr()));
    case PRINT: {
        if (tok.next().type != LEFT_PAREN)
            throw std::runtime_error("Expected ( to start print statement");
        
        ExprPtr e = parseExpr();

        if (tok.next().type != RIGHT_PAREN)
            throw std::runtime_error("Expected ) after print statement");

        return std::make_unique<PrintStatement>(std::move(e));
    }
    default: throw std::runtime_error("Token type is not valid state to statement: " + tok.peek().type);
    }
}

ClassPtr Parser::parseClass() {
    if (tok.next().type != IDENTIFIER)
        throw std::runtime_error("Class name expected after class keyword");

    std::string name = std::get<std::string>(tok.peek().value);
    
    std::vector<VarPtr> fptr = {};
    if (tok.next().type != LEFT_BRACKET)
        throw std::runtime_error("Expected [ after class declaration");

    if (tok.next().type == FIELDS) {
        do {
            if (tok.next().type != IDENTIFIER)
                throw std::runtime_error("Expected identifier for class field names");

            fptr.push_back(std::make_unique<Variable>(std::get<std::string>(tok.peek().value)));
        } while(tok.next().type == COMMA);

        if (tok.peek().type != NEWLINE)
            throw std::runtime_error("Expected a newline after field definition in class definition");
    }

    std::vector<MethodPtr> mptr = {};
    while (tok.next().type == METHOD) {
        if (tok.peek().type != IDENTIFIER)
            throw std::runtime_error("Expected method name after method keyword");

        std::string mname = std::get<std::string>(tok.peek().value);

        if (tok.next().type != LEFT_PAREN)
            throw std::runtime_error("Expected ( after method declaration");

        std::vector<VarPtr> args = {};

        do {
            if (tok.next().type != IDENTIFIER)
                throw std::runtime_error("Expected identifier for method argument names");

            args.push_back(std::make_unique<Variable>(std::get<std::string>(tok.peek().value)));
        } while(tok.next().type == COMMA);

        if (tok.peek().type != RIGHT_PAREN)
            throw std::runtime_error("Expected ) to close method arguments");

        std::vector<VarPtr> locals = {};

        if (tok.next().type == WITH) {
            if (tok.next().type != LOCALS)
                throw std::runtime_error("'with locals' expected after method to define local variables");
            
            do {
                if (tok.next().type != IDENTIFIER)
                    throw std::runtime_error("Expected identifier for method locals");

                locals.push_back(std::make_unique<Variable>(std::get<std::string>(tok.peek().value)));
            } while(tok.next().type == COMMA);
        }

        if (locals.size() > 6)
            throw std::runtime_error("Method declarations allowed with 0-6 variable arguments");

        if (tok.peek().type != COLON || tok.next().type != NEWLINE)
            throw std::runtime_error("Expected ':\\n' after declaration of method locals");
            
        std::vector<StmtPtr> statements = {};

        do {
            statements.push_back(std::move(parseStatement()));
        } while (tok.next().type == NEWLINE);

        mptr.push_back(std::make_unique<Method>(mname, std::move(args), std::move(locals), std::move(statements)));
    }
    
    if (tok.next().type != ']')
        throw std::runtime_error("Expected ] to close class definition");

    return std::make_unique<Class>(name, std::move(fptr), std::move(mptr));
}

ProgramPtr Parser::parseProgram() {
    std::vector<ClassPtr> classes = {};
    
    while (tok.next().type == CLASS || tok.peek().type == NEWLINE) {
        if (tok.peek().type == NEWLINE) continue;

        classes.push_back(parseClass());
    }

    if (tok.peek().type != IDENTIFIER || std::get<std::string>(tok.peek().value) != "main")
        throw std::runtime_error("Expected main declaration after class definitions");

    std::string mname = "main";

    if (tok.next().type != LEFT_PAREN)
        throw std::runtime_error("Expected ( after method declaration");

    std::vector<VarPtr> args = {};

    do {
        if (tok.next().type != IDENTIFIER)
            throw std::runtime_error("Expected identifier for method argument names");

        args.push_back(std::make_unique<Variable>(std::get<std::string>(tok.peek().value)));
    } while(tok.next().type == COMMA);

    if (tok.peek().type != RIGHT_PAREN)
        throw std::runtime_error("Expected ) to close method arguments");

    std::vector<VarPtr> locals = {};

    if (tok.next().type == WITH) {
        if (tok.next().type != LOCALS)
            throw std::runtime_error("'with' expected after main to define local variables");
        
        do {
            if (tok.next().type != IDENTIFIER)
                throw std::runtime_error("Expected identifier for method locals");

            locals.push_back(std::make_unique<Variable>(std::get<std::string>(tok.peek().value)));
        } while(tok.next().type == COMMA);
    }

    if (tok.peek().type != COLON || tok.next().type != NEWLINE)
        throw std::runtime_error("Expected ':\\n' after declaration of main locals");
        
    std::vector<StmtPtr> statements = {};

    do {
        statements.push_back(std::move(parseStatement()));
    } while (tok.next().type == NEWLINE);

    MethodPtr m = std::make_unique<Method>(std::move(mname), std::move(args), std::move(locals), std::move(statements));
    return std::make_unique<Program>(std::move(m), std::move(classes));
}
