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

ValPtr IRBuilder::tagCheck(ValPtr lcl, TagType tag) {
    // PINHOLE OPTIMIZATION: AVOID TAG CHECKS IF WORKING WITH %THIS
    if (pinhole && lcl->getString() == "this") {
        return lcl;
    }

    auto istagbranch = createBlock();
    auto nottagbranch = createBlock();

    auto tmp = std::make_shared<Local>(getNextTemp());
    addInstruction(std::move(std::make_unique<BinInst>(tmp, Oper::BitAnd, lcl, std::make_shared<Const>(1))));

    if (tag)
        terminate(std::move(std::make_unique<Conditional>(tmp, istagbranch, nottagbranch)));
    else
        terminate(std::move(std::make_unique<Conditional>(tmp, nottagbranch, istagbranch)));

    setCurrentBlock(nottagbranch);
    if (tag == TagType::Integer)
        terminate(std::move(std::make_unique<Fail>(FailReason::NotANumber)));
    else
        terminate(std::move(std::make_unique<Fail>(FailReason::NotAPointer)));

    setCurrentBlock(istagbranch);
    return tmp;
}

void IRBuilder::tagVal(ValPtr val, TagType tag) {
    // PINHOLE OPTIMIZATION: AVOID TAG CHECKS IF WORKING WITH %THIS
    if (pinhole && val->getString() == "this") {
        return;
    }

    addInstruction(std::move(std::make_unique<BinInst>(val, Oper::Mul, val, std::make_shared<Const>(2))));
    if (tag != 0)
        addInstruction(std::move(std::make_unique<BinInst>(val, Oper::BitXor, val, std::make_shared<Const>(tag))));
}

void IRBuilder::untagVal(ValPtr val) {
    // PINHOLE OPTIMIZATION: AVOID TAG CHECKS IF WORKING WITH %THIS
    if (pinhole && val->getString() == "this") {
        return;
    }

    addInstruction(std::move(std::make_unique<BinInst>(val, Oper::Div, val, std::make_shared<Const>(2))));
}

void IRBuilder::terminate(std::unique_ptr<ControlTransfer> blockTerm) {
    current->blockTransfer = std::move(blockTerm);
}

Local IRBuilder::getNextTemp() {
    auto nxtTmp = "tmp" + std::to_string(nexttmp++) + "v";
    method->registerTemp(nxtTmp);
    return Local(nxtTmp, 0);
}

int IRBuilder::getClassSize(std::string classname) {
    if (!classes.contains(classname)) 
        std::runtime_error("Could not find classname: " + classname);
    
    return classes[classname]->size();
}

int IRBuilder::getFieldOffset(std::string member) {
    for (size_t i = 0; i < members.size(); i++)
        if (members[i] == member)
            return i;

    std::runtime_error("Could not find member: " +  member);
    
    return -1;
}

int IRBuilder::getMethodOffset(std::string method) {
    for (size_t i = 0; i < methods.size(); i++)
        if (methods[i] == method)
            return i;

    std::runtime_error("Could not find method: " + method);
    
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
