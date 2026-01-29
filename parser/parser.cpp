#include "parser.h"

Expression Parser::parseExpr() {
    switch (tok.next().type) {
        case TokenType::ENDOFFILE: throw "No expression to parse: EOF";
        case TokenType::NUMBER: return Constant(std::get<int>(tok.peek().value));
        case TokenType::IDENTIFIER: return Variable(std::get<std::string>(tok.peek().value));
        case TokenType::LEFT_PAREN:
            // Should be start of a binary operation
            Expression lhs = parseExpr();
            Token optok = tok.next();
            if (optok.getType() != TokenType.OPERATOR)
                throw new IllegalArgumentException("Expected operator but found "+optok);
            Expression rhs = parseExpr();
            Token closetok = tok.next();
            if (closetok.getType() != TokenType.RIGHT_PAREN)
                throw new IllegalArgumentException("Expected right paren but found "+closetok);
            return new Binop(lhs, ((Operator)optok).op(), rhs);
        case Ampersand a:
            // Should be field read
            Expression base = parseExpr();
            Token dot = tok.next();
            if (dot.getType() != TokenType.DOT)
                throw new IllegalArgumentException("Expected dot but found "+dot);
            Token fname = tok.next();
            if (fname.getType() != TokenType.IDENTIFIER)
                throw new IllegalArgumentException("Expected valid field name but found "+fname);
            return new FieldRead(base, ((Identifier)fname).name());
        case Caret c:
            // Should be method call
            Expression mbase = parseExpr();
            Token mdot = tok.next();
            if (mdot.getType() != TokenType.DOT)
                throw new IllegalArgumentException("Expected dot but found "+mdot);
            Token mname = tok.next();
            if (mname.getType() != TokenType.IDENTIFIER)
                throw new IllegalArgumentException("Expected valid method name but found "+mname);
            Token open = tok.next();
            if (open.getType() != TokenType.LEFT_PAREN)
                throw new IllegalArgumentException("Expected left paren but found "+open);
            // Now we iterate through arguments
            ArrayList<Expression> args = new ArrayList<>();
            while (tok.peek().getType() != TokenType.RIGHT_PAREN) {
                Expression e = parseExpr();
                System.err.println("Parsed arg: "+e);
                args.add(e);
                // Now either a paren or a comma
                Token punc = tok.peek();
                if (punc.getType() == TokenType.COMMA)
                    tok.next(); // throw away the comma
            }
            return new MethodCall(mbase, ((Identifier)mname).name(), args);
        case AtSign a:
            Token cname = tok.next();
            if (cname.getType() != TokenType.IDENTIFIER)
                throw new IllegalArgumentException("Expected valid class name but found: "+cname);
            return new ClassRef(((Identifier)cname).name());
        case This t: return new ThisExpr();
        case Token o:
            throw new IllegalArgumentException("Token "+o+" is not a valid start of an expression");
    }
}

// TODO: You'll need to add parseStatement, parseClass, etc
