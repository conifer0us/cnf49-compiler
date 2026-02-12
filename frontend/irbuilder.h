#pragma once

#include <vector>

#include "ASTNodes.h"
#include "ir.h"

class IRBuilder {
    std::shared_ptr<MethodIR> method;
    BasicBlock* current;
    int nexttmp = 1;
    std::map<std::string, std::unique_ptr<ClassMetadata>>& classes;
    std::vector<std::string>& members;
    std::vector<std::string>& methods;
    bool pinhole;

public:
    IRBuilder(std::shared_ptr<MethodIR> m, 
        std::map<std::string, std::unique_ptr<ClassMetadata>>& cls, 
        std::vector<std::string>& mem, 
        std::vector<std::string>& mthd, bool pinhole = false): 

        method(m), classes(cls), members(mem), methods(mthd), pinhole(pinhole) { 
            auto lcls = method->getLocals();
            auto args = method->getArgs();
            current = m->getStartBlock();
        }

    BasicBlock* createBlock();

    void setCurrentBlock(BasicBlock* b);

    void addInstruction(std::unique_ptr<IROp> op);

    void tagCheck(ValPtr lcl, TagType tag);
    void tagVal(ValPtr lcl, TagType tag);
    void untagVal(ValPtr lcl);

    void terminate(std::unique_ptr<ControlTransfer> blockTerm);

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
