#pragma once

#include <vector>

#include "ASTNodes.h"
#include "ir.h"

class IRBuilder {
    MethodIR& method;
    BasicBlock* current;
    std::map<std::string, int> ssaVersion;
    std::map<std::string, std::unique_ptr<ClassMetadata>>& classes;
    std::vector<std::string>& members;
    std::vector<std::string>& methods;

public:
    IRBuilder(MethodIR& m, std::map<std::string, std::unique_ptr<ClassMetadata>>& cls, std::vector<std::string>& mem, std::vector<std::string>& mthd): 
        method(m), current(m.getStartingBlock()), classes(cls), members(mem), methods(mthd) { 
            auto lcls = method.getLocals();

            ssaVersion[""] = 0;
            for (std::string str : lcls)
                ssaVersion[str] = 0;
        }

    BasicBlock* createBlock();

    void setCurrentBlock(BasicBlock* b);

    void addInstruction(std::unique_ptr<IROp> op);

    void terminate(std::unique_ptr<ControlTransfer> blockTerm);

    // turn on the increment flag when writing new value to the var
    Local getSSAVar(std::string name, bool increment = false);

    // Use SSA machinery to produce temp values
    Local getNextTemp();

    int getClassSize(std::string classname);

    int getFieldOffset(std::string member);

    int getMethodOffset(std::string method);

    // Process a set of statements from the current position
    // If the block control transfer is set, return true
    // Refuses to process statements in a block after return statement
    bool processBlock(const std::vector<StmtPtr>& statements);
};
