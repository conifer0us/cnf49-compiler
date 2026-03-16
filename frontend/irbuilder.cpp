#include "irbuilder.h"
 
BasicBlock* IRBuilder::createBlock() {
    return method->newBasicBlock();
}

void IRBuilder::setCurrentBlock(BasicBlock* b) {
    current = b;
}

void IRBuilder::addInstruction(std::unique_ptr<IROp> op) {
    if (!current)
        throw std::runtime_error("IRBuilder has corrupt block structure. Aborting.");

    current->instructions.push_back(std::move(op));
}

void IRBuilder::terminate(std::unique_ptr<ControlTransfer> blockTerm) {
    current->blockTransfer = std::move(blockTerm);
}

LclPtr IRBuilder::getNextTemp() {
    auto nxtTmp = "tmp" + std::to_string(nexttmp++);
    method->registerTemp(nxtTmp);
    return std::make_shared<Local>(nxtTmp, 0, true);
}

int IRBuilder::getClassSize(std::string classname) {
    if (!classes.contains(classname)) 
        throw std::runtime_error("Could not find classname: " + classname);
    
    return classes[classname]->size();
}

int IRBuilder::getFieldOffset(std::string type, std::string fieldname) {
    for (int i = 0; i < classes[type]->typedFields.size(); i++) {
        if (classes[type]->typedFields[i].first == fieldname) {
            // field offset is offset by 1 to account for vtable and multiplied by 8 to align with 64 bit values
            return 8 * (i + 1);
        }
    }
    
    throw std::runtime_error("Could not find field: " + fieldname);
}

int IRBuilder::getMethodOffset(std::string method) {
    for (size_t i = 0; i < methods.size(); i++)
        if (methods[i] == method)
            return i;

    throw std::runtime_error("Could not find method: " + method);
}

bool IRBuilder::processBlock(const std::vector<StmtPtr>& statements) {
    for (auto &stmt : statements) {
        stmt->convertToIR(*this);

        if (typeid(*stmt.get()) == typeid(ReturnStatement))
            return true;
    }

    return false;
}
