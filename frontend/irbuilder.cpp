#include "irbuilder.h"
 
BasicBlock* IRBuilder::createBlock() {
    return method->newBasicBlock();
}

void IRBuilder::setCurrentBlock(BasicBlock* b) {
    current = b;
}

void IRBuilder::addInstruction(std::unique_ptr<IROp> op) {
    if (!current)
        std::runtime_error("IRBuilder has corrupt block structure. Aborting.");

    current->instructions.push_back(std::move(op));
}

void IRBuilder::terminate(std::unique_ptr<ControlTransfer> blockTerm) {
    current->blockTransfer = std::move(blockTerm);
}

Local IRBuilder::getSSAVar(std::string name, bool increment) {
    if (!ssaVersion.contains(name)) {
        std::runtime_error(std::format("Unknown variable: {}\n", name));
    }

    auto ver = ssaVersion[name];
    
    if (increment) {
        ver++;
        ssaVersion[name] = ver;
    }
        
    return Local(name, ver);
}

Local IRBuilder::getNextTemp() {
    return getSSAVar("", true);
}

int IRBuilder::getClassSize(std::string classname) {
    if (!classes.contains(classname)) 
        std::runtime_error(std::format("Could not find classname {}", classname));
    
    return classes[classname]->size();
}

int IRBuilder::getFieldOffset(std::string member) {
    for (size_t i = 0; i < members.size(); i++)
        if (members[i] == member)
            return i;

    std::runtime_error(std::format("Could not find member {}", member));
    
    return -1;
}

int IRBuilder::getMethodOffset(std::string method) {
    for (size_t i = 0; i < methods.size(); i++)
        if (methods[i] == method)
            return i;

    std::runtime_error(std::format("Could not find method {}", method));
    
    return -1;
}

bool IRBuilder::processBlock(const std::vector<StmtPtr>& statements) {
    for (auto &stmt : statements) {
        stmt->convertToIR(*this);

        if (typeid(*stmt.get()) == typeid(ReturnStatement))
            return true;
    }

    return false;
}
