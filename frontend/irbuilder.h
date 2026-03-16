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
    
    void terminate(std::unique_ptr<ControlTransfer> blockTerm);

    LclPtr getNextTemp();

    int getClassSize(std::string classname);

    int getFieldOffset(std::string member);

    int getMethodOffset(std::string method);

    // Process a set of statements from the current position
    // If block terminates in a return, return true
    bool processBlock(const std::vector<StmtPtr>& statements);

    // returns if pinhole optimization is enabled
    bool getPinhole();
};
