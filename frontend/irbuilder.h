#pragma once

#include <vector>

#include "ASTNodes.h"
#include "ir.h"

class IRBuilder {
    std::shared_ptr<MethodIR> method;
    std::map<std::string, std::unique_ptr<ClassMetadata>>& classes;;
    std::vector<std::string>& methods;
    
    BasicBlock* current;
    int nexttmp = 1;

public:
    IRBuilder(std::shared_ptr<MethodIR> m, 
        std::map<std::string, std::unique_ptr<ClassMetadata>>& cls, 
        std::vector<std::string>& mthd):
        method(m), classes(cls), methods(mthd) { 
            auto lcls = method->getLocals();
            auto args = method->getArgs();
            current = m->getStartBlock();
        }

    BasicBlock* createBlock();

    void setCurrentBlock(BasicBlock* b);

    void addInstruction(std::unique_ptr<IROp> op);
    
    void terminate(std::unique_ptr<ControlTransfer> blockTerm);

    LclPtr getNextTemp();

    int getClassSize(std::string classname);

    int getFieldOffset(std::string type, std::string fieldname);

    int getMethodOffset(std::string method);

    unsigned long getGCMap(std::string classname);

    // Process a set of statements from the current position
    // If block terminates in a return, return true
    bool processBlock(const std::vector<StmtPtr>& statements);
};
