#include "ir.h"
#include <iostream>

void Local::outputIR() const {
    if (version)
        std::cout << "%" << version << name;
    else 
        std::cout << "%" << name;
}

void Global::outputIR() const {
    std::cout << "@" << name;
}

void Const::outputIR() const {
    std::cout << value;
}

void Assign::outputIR() const {
    dest.outputIR();
    std::cout << " = ";
    src.outputIR();
}

void BinInst::outputIR() const {
    dest.outputIR();
    std::cout << " = ";
    lhs.outputIR();
    
    switch(op) {
        case Oper::Add:
            std::cout << '+';
            break;
        case Oper::BitAnd:
            std::cout << '&';
            break;
        case Oper::BitOr:
            std::cout << '|';
            break;
        case Oper::BitXor:
            std::cout << '^';
            break;
        case Oper::Div:    
            std::cout << '/';
            break;
        case Oper::Eq:
            std::cout << "==";
            break;
        case Oper::Gt:
            std::cout << '>';
            break;
        case Oper::Lt:
            std::cout << '<';
            break;
        case Oper::Mul:
            std::cout << '*';
            break;
        case Oper::Ne:
            std::cout << "!=";
            break;
        case Oper::Sub:
            std::cout << '-';
            break;
    }

    std::cout << " ";
    rhs.outputIR();
}

void Call::outputIR() const {
    dest.outputIR();

    std::cout << " = call(";

    code.outputIR();

    std::cout << ", ";

    receiver.outputIR();

    for (auto arg : args) {
        std::cout << ", ";
        arg.outputIR();
    }

    std::cout << ")";
}

void Phi::outputIR() const {
    dest.outputIR();

    std::cout << " = phi(";

    bool first = true;
    for (auto& pair : incoming) {
        if (first)
            first = false;
        else
            std::cout << ", ";

        std::cout << pair.first;
        std::cout << ", ";
        pair.second.outputIR();
    }

    std::cout << ")";
}

void Alloc::outputIR() const {
    dest.outputIR();
    
    std::cout << " = ";
    std::cout << "alloc(" << numSlots << ")";
}

void Print::outputIR() const {
    std::cout << "print(";
    val.outputIR();
    std::cout << ")";
}

void GetElt::outputIR() const {
    dest.outputIR();
    std::cout << " = getelt(";
    dest.outputIR();
    std::cout << ", ";
    index.outputIR();
    std::cout << ")";
}

void SetElt::outputIR() const {
    std::cout << "setelt(";
    array.outputIR();
    std::cout << ", ";
    index.outputIR();
    std::cout << ", ";
    val.outputIR();
    std::cout << ")";
}

void Load::outputIR() const {
    dest.outputIR();
    std::cout << " = load(";
    addr.outputIR();
    std::cout << ")";
}

void Store::outputIR() const {
    std::cout << "store(";
    addr.outputIR();
    std::cout << ", ";
    val.outputIR();
    std::cout << ")";
}

void Jump::outputIR() const {
    std::cout << "jump " << target;
}

void Conditional::outputIR() const {
    std::cout << "if ";
    condition.outputIR();
    std::cout << " then " << trueTarget << " else " << falseTarget;
}

void Return::outputIR() const {
    std::cout << "ret ";
    val.outputIR();
}

void Fail::outputIR() const {
    std::cout << "Program threw exception: ";
    switch (reason) {
        case FailReason::NotAPointer:
            std::cout << "NotAPointer";
            break;
        case FailReason::NoSuchField:
            std::cout << "NoSuchField";
            break;
        case FailReason::NoSuchMethod:
            std::cout << "NoSuchMethod";
            break;
    }
}

void ClassMetadata::outputIR(const std::vector<std::string>& methods, const std::vector<std::string>& fields) const {
    std::cout << "@";
    VTABLE(name).outputIR();
    std::cout << ": { ";
    
    for (size_t i = 0; i < vtable.size(); ++i) {
        if (i) std::cout << ", ";
        std::cout << "@" << vtable[i];
    }
    
    std::cout << " }\n";

    std::cout << "@";
    FTABLE(name).outputIR();
    std::cout << ": { ";
    for (size_t i = 0; i < ftable.size(); ++i) {
        if (i) std::cout << ", ";
        std::cout << ftable[i];
    }
    std::cout << " }\n\n";
}

void BasicBlock::outputIR() const {
    std::cout << label << ":\n";

    for (const auto& inst : instructions) {
        std::cout << "\t";
        inst->outputIR();
        std::cout << "\n";
    }

    std::cout << "\t";
    blockTransfer->outputIR();
    std::cout << "\n";
}

void MethodIR::outputIR() const {
    for (const auto& block : blocks) {
        block->outputIR();
    }

    std::cout << "\n";
}

void CFG::outputIR() const {
    std::cout << "data:\n";

    for (const auto& [_, meta] : classinfo) {
        meta.outputIR(classmethods, classfields);
    }

    std::cout << "\ncode:\n\n";

    for (const auto& [_, method] : methodinfo) {
        method.outputIR();
    }
}
