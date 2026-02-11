#pragma once

#include "ir.h"

class IRBuilder {
    MethodIR& method;
    BasicBlock* current;
    std::map<std::string, int> ssaVersion;
    std::map<std::string, ClassMetadata>& classes;
    std::vector<std::string>& members;
    std::vector<std::string>& methods;

public:
    IRBuilder(MethodIR& m, std::map<std::string, ClassMetadata>& cls, std::vector<std::string>& mem, std::vector<std::string>& mthd): 
        method(m), current(m.getStartingBlock()), classes(cls), members(mem), methods(mthd) { 
            auto lcls = method.getLocals();

            ssaVersion[""] = 0;
            for (std::string str : lcls)
                ssaVersion[str] = 0;
        }

    BasicBlock* createBlock() {
        return method.newBasicBlock();
    }

    void setCurrentBlock(BasicBlock* b) {
        current = b;
    }

    void addInstruction(std::unique_ptr<IROp> op) {
        current->instructions.push_back(std::move(op));
    }

    void terminate(std::unique_ptr<ControlTransfer> blockTerm) {
        current->blockTransfer = std::move(blockTerm);
    }

    // turn on the increment flag when writing new value to the var
    Local getSSAVar(std::string name, bool increment = false) {
        if (!ssaVersion.contains(name)) {
            std::runtime_error(std::format("Unknown variable: %d\n", name));
        }

        auto ver = ssaVersion[name];
        
        if (increment) {
            ssaVersion[name] += 1;
            ver++;
        }
            
        return Local(name, ver);
    }

    // Use SSA machinery to produce temp values
    Local getNextTemp() {
        return getSSAVar("", true);
    }

    int getClassSize(std::string classname) {
        if (!classes.contains(classname)) 
            std::runtime_error(std::format("Could not find classname %d", classname));
        
        return classes[classname].size();
    }

    int getFieldOffset(std::string member) {
        for (size_t i = 0; i < members.size(); i++)
            if (members[i] == member)
                return i;
    
        std::runtime_error(std::format("Could not find member %d", member));    
    }

    int getMethodOffset(std::string method) {
        for (size_t i = 0; i < methods.size(); i++)
            if (methods[i] == method)
                return i;
        std::runtime_error(std::format("Could not find method %d", method));    
    }

    // Process a set of statements from the current position
    // If the block control transfer is set, return true
    // Refuses to process statements in a block after return statement
    bool processBlock(std::vector<StmtPtr> statements) {
        for (auto &stmt : statements) {
            stmt->convertToIR(*this);

            if (typeid(*stmt.get()) == typeid(ReturnStatement))
                return true;
        }

        return false;
    }
};
