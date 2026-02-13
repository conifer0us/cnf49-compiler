#include "ir.h"
#include <queue>

void CFG::naiveSSA() {
// loops through all methods
    for (auto & [_, method] : methodinfo) {
        method->naiveSSA();
    }
}

void MethodIR::naiveSSA() {
    std::map<BasicBlock*, std::vector<BasicBlock*>> predecessors;
    std::map<std::string, int> globalVersion;
    std::map<BasicBlock*, std::map<std::string, int>> versionsEnd;
    std::map<BasicBlock*, std::map<std::string, ValPtr>> phiout;

    auto startblock = getStartBlock();

    // Initialize args (skip 'this')
    for (int i = 1; i < args.size(); i++)
        globalVersion[args[i]] = 0;

    // Initialize locals
    for (auto& lcl : locals)
        globalVersion[lcl] = 0;

    // Initialize temps
    for (auto&tmp : temps)
        globalVersion[tmp] = 0;

    // Compute block predecessors
    for (auto& block : blocks) {
        for (auto* succ : block->getNextBlocks()) {
            predecessors[succ].push_back(block.get());
        }
    }

    // insert phi nodes for every variable in blocks with multiple predecessors
    for (auto& block : blocks) {
        if (predecessors[block.get()].size() > 1) {
            for (auto& [var, _] : globalVersion) {
                phiout[block.get()][var] = std::make_shared<Local>(var, ++globalVersion[var]);
            }
        }

        for (auto& inst : block->instructions) {
            inst->renameUses(globalVersion);
        }

        block->blockTransfer->renameUses(globalVersion);
        versionsEnd[block.get()] = globalVersion;
    }

    // nested mess for compiling Phi statement from computed block previous and allotted phi statement dest
    for (auto& block : blocks) {
        if (predecessors[block.get()].size() > 1) {
            for (auto &[var, _]: globalVersion) {
                std::vector<std::pair<std::string, ValPtr>> phiArgs;

                for (auto* pred : predecessors[block.get()]) {
                    phiArgs.push_back({pred->label, std::make_shared<Local>(var, versionsEnd[pred][var])});
                }

                auto phiInst = std::make_unique<Phi>(phiout[block.get()][var], phiArgs);
                block->blockPhi.push_back(std::move(phiInst));
            }
        }
    }
}


// disgusting amount of copy/paste + search/replace in this section but the idea is the same
// for each instruction, replace all locals with the latest version, increment if assigning

// not doing anything for phi; handled above as part of the SSA buildout
void Phi::renameUses(std::map<std::string, int>& versions) {}

void Assign::renameUses(std::map<std::string, int>& versions) {
    if (src->getValType() == VarType) {
        auto name = src->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            src = std::make_shared<Local>(name, it->second);
    }
    
    if (dest->getValType() == VarType) {
        auto destname = dest->getString();
        auto it = versions.find(destname);
        if (it != versions.end())
            dest = std::make_shared<Local>(destname, ++versions[destname]);
    }
}

void BinInst::renameUses(std::map<std::string,int>& versions) {
    if (lhs->getValType() == VarType) {
        auto name = lhs->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            lhs = std::make_shared<Local>(name, it->second);
    }
    if (rhs->getValType() == VarType) {
        auto name = rhs->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            rhs = std::make_shared<Local>(name, it->second);
    }

    if (dest->getValType() == VarType) {
        auto destname = dest->getString();
        auto it = versions.find(destname);
        if (it != versions.end())
            dest = std::make_shared<Local>(destname, ++versions[destname]);
    }
}

void Call::renameUses(std::map<std::string, int>& versions) {
    if (code->getValType() == VarType) {
        auto name = code->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            code = std::make_shared<Local>(name, it->second);
    }
    for (auto& arg : args) {
        if (arg->getValType() == VarType) {
            auto name = arg->getString();
            auto it = versions.find(name);
            if (it != versions.end())
                arg = std::make_shared<Local>(name, it->second);
        }
    }

    if (dest->getValType() == VarType) {
        auto destname = dest->getString();
        auto it = versions.find(destname);
        if (it != versions.end())
            dest = std::make_shared<Local>(destname, ++versions[destname]);
    }
}

void Alloc::renameUses(std::map<std::string, int>& versions) {
    if (dest->getValType() == VarType) {
        auto destname = dest->getString();
        auto it = versions.find(destname);
        if (it != versions.end())
            dest = std::make_shared<Local>(destname, ++versions[destname]);
    }
}

void Print::renameUses(std::map<std::string, int>& versions) {
    if (val->getValType() == VarType) {
        auto name = val->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            val = std::make_shared<Local>(name, it->second);
    }
}

void GetElt::renameUses(std::map<std::string, int>& versions) {
    if (array->getValType() == VarType) {
        auto name = array->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            array = std::make_shared<Local>(name, it->second);
    }
    if (index->getValType() == VarType) {
        auto name = index->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            index = std::make_shared<Local>(name, it->second);
    }

    if (dest->getValType() == VarType) {
        auto destname = dest->getString();
        auto it = versions.find(destname);
        if (it != versions.end())
            dest = std::make_shared<Local>(destname, ++versions[destname]);
    }
}

void SetElt::renameUses(std::map<std::string, int>& versions) {
    if (array->getValType() == VarType) {
        auto name = array->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            array = std::make_shared<Local>(name, it->second);
    }
    if (index->getValType() == VarType) {
        auto name = index->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            index = std::make_shared<Local>(name, it->second);
    }
    if (val->getValType() == VarType) {
        auto name = val->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            val = std::make_shared<Local>(name, it->second);
    }
}

void Load::renameUses(std::map<std::string, int>& versions) {
    if (addr->getValType() == VarType) {
        auto name = addr->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            addr = std::make_shared<Local>(name, it->second);
    }
    
    if (dest->getValType() == VarType) {
        auto destname = dest->getString();
        auto it = versions.find(destname);
        if (it != versions.end())
            dest = std::make_shared<Local>(destname, ++versions[destname]);
    }
}

void Store::renameUses(std::map<std::string, int>& versions) {
    if (addr->getValType() == VarType) {
        auto name = addr->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            addr = std::make_shared<Local>(name, it->second);
    }
    if (val->getValType() == VarType) {
        auto name = val->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            val = std::make_shared<Local>(name, it->second);
    }
}

void Conditional::renameUses(std::map<std::string, int>& versions) {
    if (condition->getValType() == VarType) {
        auto name = condition->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            condition = std::make_shared<Local>(name, it->second);
    }
}

void Return::renameUses(std::map<std::string, int>& versions) {
    if (val->getValType() == VarType) {
        auto name = val->getString();
        auto it = versions.find(name);
        if (it != versions.end())
            val = std::make_shared<Local>(name, it->second);
    }
}

void Jump::renameUses(std::map<std::string, int>& versions) {
    return;
}

void HangingBlock::renameUses(std::map<std::string, int>& versions) {
    return;
}

void Fail::renameUses(std::map<std::string, int>& versions) {
    return;
}