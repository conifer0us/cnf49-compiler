#include <stdio.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include "parser/tokenizer.h"
#include "parser/parser.h"

#define helpstr "Usage: <comp> {-help | -printAST | -noSSA | -noopt} sourcefile"

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, helpstr);
        return 1;
    }

    if (strcmp(argv[1], "-help") == 0) {
        printf(helpstr);
        printf("Provide one or no arguments. -help shows this menu.\n-printAST, -printCFG, -noSSA, and -noopt stop the compiler after the corresponding pass and print results.");
    }

    char *filename = argv[argc - 1];
    auto infile = std::ifstream(filename);
    std::ostringstream content;
    content << infile.rdbuf();
    std::string sourcestring = content.str();

    Tokenizer tok = Tokenizer(sourcestring);
    Parser parser = Parser(tok);

    ProgramPtr AST = parser.parseProgram();

    return 0;
}
