// most of this implementation is in ASTNodes.h because I just didn't make this file at the time
// these functions added to avoid linker errors
#include "ASTNodes.h"

ASTNode::~ASTNode() = default;
void ASTNode::print(int ind) const {
    return;
}

Expression::~Expression() = default;
Value Expression::convertToIR(IRBuilder& builder, Local* out) const {
    std::runtime_error("Tried to print base value on IR conversion");
    return Value();
}

Statement::~Statement() = default;
void Statement::convertToIR(IRBuilder& builder) const {
    return;
}
