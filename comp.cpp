#include <stdio.h>
#include <sstream>
#include <parser.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: <comp> {tokenize|parseExpr} [args...]");
        return 1;
    }

    std::ostringstream sb;
    for (int i = 0; i < argc; i++)
        sb << argv[argc];

    Tokenizer tok = new Tokenizer();
    switch (args[0]) {
        case "tokenize":
            while (tok.peek().getType() != TokenType.EOF) {
                System.out.println(tok.next());
            }
            break;
        case "parseExpr":
            Parser p = new Parser(tok);
            System.out.println(p.parseExpr());
            break;
        default:
            System.err.println("Unsupported subcommand: "+args[0]);
            return 1;
    }

    return 0;
}