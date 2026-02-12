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
        return;
    }

    auto istagbranch = createBlock();
    auto nottagbranch = createBlock();

    auto tmp = std::make_shared<Local>(getNextTemp());
    addInstruction(std::move(std::make_unique<BinInst>(tmp, Oper::BitAnd, lcl, std::make_shared<Const>(tag))));

    if (tag)
        terminate(std::move(std::make_unique<Conditional>(tmp, istagbranch, nottagbranch)));
    else
        terminate(std::move(std::make_unique<Conditional>(tmp, nottagbranch, istagbranch)));

    setCurrentBlock(nottagbranch);
    terminate(std::move(std::make_unique<Fail>(FailReason::NotANumber)));

    setCurrentBlock(istagbranch);
}

ValPtr IRBuilder::tagVal(ValPtr val, TagType tag) {
    // PINHOLE OPTIMIZATION: AVOID TAG CHECKS IF WORKING WITH %THIS
    if (pinhole && val->getString() == "this") {
        return val;
    }

    // increment SSA since variable is going to be updated by tag check
    auto ret = std::make_shared<Local>(getSSAVar(val->getString(), true));
    auto tmp = std::make_shared<Local>(getNextTemp()); 
    addInstruction(std::move(std::make_unique<BinInst>(tmp, Oper::Mul, val, std::make_shared<Const>(2))));
    addInstruction(std::move(std::make_unique<BinInst>(ret, Oper::Add, tmp, std::make_shared<Const>(tag))));
    return ret;
}

ValPtr IRBuilder::untagVal(ValPtr val) {
    // PINHOLE OPTIMIZATION: AVOID TAG CHECKS IF WORKING WITH %THIS
    if (pinhole && val->getString() == "this") {
        return val;
    }

    auto ret = std::make_shared<Local>(getSSAVar(val->getString()));
    addInstruction(std::move(std::make_unique<BinInst>(ret, Oper::Div, val, std::make_shared<Const>(2))));
    return ret;
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
