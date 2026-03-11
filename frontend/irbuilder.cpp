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

void IRBuilder::tagCheck(ValPtr lcl, TagType tag) {
    // PINHOLE OPTIMIZATION: AVOID TAG CHECKS IF WORKING WITH %THIS
    if (pinhole && lcl->getString() == "this") {
        if (tag == Integer)
            std::runtime_error("'this' cannot be used as a numerical value");
        
        return;
    }

    auto istagbranch = createBlock();
    auto nottagbranch = createBlock();

    auto tmp = getNextTemp();
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
}

ValPtr IRBuilder::tagVal(ValPtr val, TagType tag) {
    // PINHOLE OPTIMIZATION: AVOID TAG CHECKS IF WORKING WITH %THIS
    if (pinhole && val->getString() == "this")
        return val;

    // if temp value, get new temp to return. else do operation on existing var
    // strange name conversion required here to make new Local since types were weirdly conceived 
    LclPtr out = (val->ignoreSSA) ? getNextTemp() : std::make_shared<Local>(val->getString(), 0);

    if (tag) {
        auto untagged = getNextTemp();
        addInstruction(std::move(std::make_unique<BinInst>(untagged, Oper::Mul, val, std::make_shared<Const>(2))));
        addInstruction(std::move(std::make_unique<BinInst>(out, Oper::BitXor, untagged, std::make_shared<Const>(tag))));
    }
    else {
        addInstruction(std::move(std::make_unique<BinInst>(out, Oper::Mul, val, std::make_shared<Const>(2))));
    }

    return out;
}

LclPtr IRBuilder::tagVal(LclPtr lcl, TagType tag) {
    // PINHOLE OPTIMIZATION: AVOID TAG CHECKS IF WORKING WITH %THIS
    if (pinhole && lcl->getString() == "this")
        return lcl;

    // if temp value, get new temp to return. else do operation on existing var
    auto out = (lcl->ignoreSSA) ? getNextTemp() : lcl;

    if (tag) {
        auto untagged = getNextTemp();
        addInstruction(std::move(std::make_unique<BinInst>(untagged, Oper::Mul, lcl, std::make_shared<Const>(2))));
        addInstruction(std::move(std::make_unique<BinInst>(out, Oper::BitXor, untagged, std::make_shared<Const>(tag))));
    }
    else {
        addInstruction(std::move(std::make_unique<BinInst>(out, Oper::Mul, lcl, std::make_shared<Const>(2))));
    }

    return out;
}

LclPtr IRBuilder::untagVal(ValPtr val) {
    // PINHOLE OPTIMIZATION: AVOID TAG CHECKS IF WORKING WITH %THIS
    if (pinhole && val->getString() == "this")
        return std::make_shared<Local>("this", 0);

    auto tmp = getNextTemp();
    addInstruction(std::move(std::make_unique<BinInst>(tmp, Oper::Div, val, std::make_shared<Const>(2))));

    return tmp;
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

bool IRBuilder::getPinhole() {
    return pinhole;
}
