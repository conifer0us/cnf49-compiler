#include "parser.h"

ExprPtr Parser::parseExpr() {
    switch (tok.next().type) {
        case TokenType::ENDOFFILE: throw "No expression to parse: EOF";
        case TokenType::NUMBER: return std::make_unique<Constant>(std::get<int>(tok.peek().value));
        case TokenType::IDENTIFIER: return std::make_unique<Variable>(std::get<std::string>(tok.peek().value));
        case TokenType::LEFT_PAREN: {
            // Should be start of a binary operation
            ExprPtr lhs = parseExpr();
            Token optok = tok.next();
            if (optok.type != TokenType::OPERATOR)
                throw "Expected operator but found " + optok.type;
            ExprPtr rhs = parseExpr();
            Token closetok = tok.next();
            if (closetok.type != TokenType::RIGHT_PAREN)
                throw "Expected right paren but found " + closetok.type;
            return std::make_unique<Binop>(lhs, std::get<char>(optok.value), rhs);
        }
        case TokenType::AMPERSAND: {
            // Should be field read
            ExprPtr base = parseExpr();
            Token dot = tok.next();
            if (dot.type != TokenType::DOT)
                throw "Expected dot but found " + dot.type;
            Token fname = tok.next();
            if (fname.type != TokenType::IDENTIFIER)
                throw "Expected valid field name but found " + fname.type;
            return std::make_unique<FieldRead>(base, std::get<std::string>(fname.value));
        }
        case TokenType::CARET: {
            // Should be method call
            ExprPtr mbase = parseExpr();
            
            Token mdot = tok.next();
            if (mdot.type != TokenType::DOT)
                throw "Expected dot but found " + mdot.type;
            Token mname = tok.next();
            if (mname.type != TokenType::IDENTIFIER)
                throw "Expected valid method name but found " + mname.type;
            Token open = tok.next();
            if (open.type != TokenType::LEFT_PAREN)
                throw "Expected left paren but found " + open.type;
            
            // Now we iterate through arguments
            std::vector<ExprPtr> args = {};
            while (tok.peek().type != TokenType::RIGHT_PAREN) {
                ExprPtr e = parseExpr();
                args.push_back(std::move(e));

                // Now either a paren or a comma
                Token punc = tok.peek();
                if (punc.type == TokenType::COMMA)
                    tok.next(); // throw away the comma
            }
            
            return std::make_unique<MethodCall>(mbase, std::get<std::string>(mname.value), args);
        }
        case TokenType::ATSIGN: {
            Token cname = tok.next();
            if (cname.type != TokenType::IDENTIFIER)
                throw "Expected valid class name but found: " + cname.type;
            return std::make_unique<ClassRef>(std::get<std::string>(cname.value));
        }
        case TokenType::THIS: return std::make_unique<ThisExpr>();
        default: throw "Token type is not a valid start of an expression: " + tok.peek().type;
    }
}

StmtPtr Parser::parseStatement() {
    switch (tok.next().type)
    {
    case TokenType::IDENTIFIER: {
        std::string name = std::get<std::string>(tok.peek().value);
        
        if (tok.next().type != TokenType::EQUAL)
            throw "Expected = but found " + tok.peek().type;    
        
        return std::make_unique<AssignStatement>(name, parseExpr());
    }    
    case TokenType::PLACEHOLDER:
        if (tok.next().type != TokenType::EQUAL)
            throw "Expected = but found " + tok.peek().type;    
        
        return std::make_unique<DiscardStatement>(parseExpr());
    case TokenType::NOT: {
        ExprPtr obj = parseExpr();
        
        if (tok.next().type != TokenType::DOT)
            throw "Expected . but found " + tok.peek().type;    
    
        if (tok.next().type != TokenType::IDENTIFIER)
            throw "Expected identifier but found " + tok.peek().type;    
    
        std::string name = std::get<std::string>(tok.peek().value);

        if (tok.next().type != TokenType::EQUAL)
            throw "Expected = but found " + tok.peek().type;    
    
        return std::make_unique<FieldAssignStatement>(obj, name, parseExpr());
    }
    case TokenType::IF: {
        ExprPtr cond = parseExpr();
        
        if (tok.next().type != TokenType::COLON || tok.next().type != TokenType::LEFT_BRACE
                || tok.next().type != TokenType::NEWLINE)
            throw "Expected ': { \\n' to start if statement";    

        std::vector<StmtPtr> trueCond;

        do {
            trueCond.push_back(std::move(parseStatement()));
        } while(tok.peekNext().type != TokenType::RIGHT_BRACE);

        if (tok.next().type != TokenType::RIGHT_BRACE || tok.next().type != TokenType::ELSE 
                || tok.next().type != TokenType::LEFT_BRACE || tok.next().type != TokenType::NEWLINE)
            throw "Expected '} else {' to separate if/else conditional";    

        std::vector<StmtPtr> falseCond;

        do {
            falseCond.push_back(std::move(parseStatement()));
        } while(tok.peekNext().type != TokenType::RIGHT_BRACE);

        return std::make_unique<IfStatement>(cond, trueCond, falseCond);
    }
    case TokenType::IFONLY:

        break;
    case TokenType::WHILE:

        break;

    case TokenType::RETURN:

        break;
    case TokenType::PRINT:

        break;
    default: throw "Token type is not valid state to statement: " + tok.peek().type;
    }
}

ClassPtr Parser::parseClass() {

}

ProgramPtr Parser::parseProgram() {

}
