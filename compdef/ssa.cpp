#include "ir.h"

void CFG::naiveSSA() {
// loops through all methods
    for (auto & [_, method] : methodinfo) {
        method->naiveSSA();
    }
}

void MethodIR::naiveSSA() {
    std::set<BasicBlock *> coveredBlocks;
    auto startblock = getStartBlock();
    std::map<BasicBlock *, std::map<std::string, int>> varversions;
}
