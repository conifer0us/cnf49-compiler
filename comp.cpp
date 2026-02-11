#include <stdio.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include "tokenizer.h"
#include "parser.h"
#include "ASTNodes.h"

#define helpstr "Usage: <comp> {-help | -printAST | -noSSA | -noopt} sourcefile\n"

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) {
        std::cout << helpstr;
        return 1;
    }

    if (strcmp(argv[1], "-help") == 0) {
        std::cout << helpstr;
        printf("Provide one or no arguments. -help shows this menu.\n-printAST, -noSSA, and -noopt stop the compiler after the corresponding pass and print results.");
        return 0;
    }

    char *filename = argv[argc - 1];
    auto infile = std::ifstream(filename);

    if (!infile.is_open()) {
        std::cout << "Could not find input file '" << filename << "'\n";
        return 1;
    }
    
    std::ostringstream content;
    content << infile.rdbuf();
    std::string sourcestring = content.str();

    Tokenizer tok = Tokenizer(sourcestring);
    Parser parser = Parser(tok);

    ProgramPtr AST = parser.parseProgram();

    if (strcmp(argv[1], "-printAST")) {
        AST->print(0);
        return 0;
    }

    return 0;
}
