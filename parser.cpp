#include <parser.h>

Token Tokenizer::peek() {
    if (!cached.hasValue()) {
        cached = advanceCurrent();
    }
    return cached;
}

Tokenizer::next() {
    if (cached == null) {
        return advanceCurrent();
    } else {
        Token tmp = cached;
        cached = null;
        return tmp;
    }
}
    
Tokenizer::advanceCurrent() {
    while (current < text.length() && Character.isWhitespace(text.charAt(current))) {
        current++;
    }
    if (current >= text.length()) {
        return this.eof;
    }
    switch (text.charAt(current)) {
        case '(': current++; return lp;
        case ')': current++; return rp;
        case '{': current++; return lb;
        case '}': current++; return rb;
        case ':': current++; return colon;
        case '!': current++; return not;
        case '@': current++; return at;
        case '^': current++; return caret;
        case '&': current++; return amp;
        case '.': current++; return dot;
        case ',': current++; return comma;
    
        case '+': current++; return new Operator('+');
        case '-': current++; return new Operator('-');
        case '*': current++; return new Operator('*');
        case '/': current++; return new Operator('/');
        
        default:
            if (Character.isDigit(text.charAt(current))) {
                // This is a digit
                int start = current++;
                while (current < text.length() && Character.isDigit(text.charAt(current))) current ++;
                // current now points to the first non-digit character, or past the end of the text
                return new Number(Integer.parseInt(text.substring(start,current)));
            }
            // Now down to keywords and identifiers
            else if (Character.isLetter(text.charAt(current))) {
                int start = current++;
                while (current < text.length() && Character.isLetterOrDigit(text.charAt(current))) current ++;
                // current now points to the first non-alphanumeric character, or past the end of the string
                String fragment = text.substring(start,current);
                // Unlike the constant parsing switch above, this has already advanced current
                switch (fragment) {
                    case "if": return iff;
                    case "ifonly": return ifonly;
                    case "while": return w;
                    case "return": return ret;
                    case "print": return print;
                    case "this": return th;
                    default: return new Identifier(fragment);
                }
            } else {
                throw new IllegalArgumentException("Unsupported character: "+text.charAt(current));
            }
    }
}

sealed interface Expression 
    permits Constant, Binop, MethodCall, FieldRead, ClassRef, ThisExpr, Variable {
    // TODO: You'll need to provide your own methods for operations you care about
}
record ThisExpr() implements Expression {}
record Constant(long value) implements Expression {}
record Binop(Expression lhs, char op, Expression rhs) implements Expression {}
record MethodCall(Expression base, String methodname, List<Expression> args) implements Expression {}
record FieldRead(Expression base, String fieldname) implements Expression {}
record ClassRef(String classname) implements Expression {}
record Variable(String name) implements Expression {}

class Parser {
    private Tokenizer tok;
    public Parser(Tokenizer t) {
        tok = t;
    }
    // TODO: You'll need to add parseStatement, parseClass, etc.
    /*
     * This method attempts to parse an expression starting from the tokenizer's current token,
     * and returns as soon as it has done so
     */
    public Expression parseExpr() {
        switch (tok.next()) {
            case Eof eof: throw new IllegalArgumentException("No expression to parse: EOF");
            case Number n: return new Constant(n.value());
            case Identifier i: return new Variable(i.name());
            case LeftParen p:
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
}
