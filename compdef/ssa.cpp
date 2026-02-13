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
    std::map<BasicBlock*, std::map<std::string, int>> priorVers;
    std::queue<BasicBlock*> worklist;

    auto startblock = getStartBlock();

    // initialize args (skip 'this')
    for (int i = 1; i < args.size(); i++)
        globalVersion[args[i]] = 0;

    // initialize locals
    for (auto& lcl : locals)
        globalVersion[lcl] = 0;

    // compute block predecessors
    for (auto& block : blocks) {
        for (auto succ : block->getNextBlocks()) {
            predecessors[succ].push_back(block.get());
        }
    }

    // start dataflow iteration
    worklist.push(startblock);

    while (!worklist.empty()) {
        BasicBlock* block = worklist.front();
        worklist.pop();

        std::map<std::string, int> incoming;

        if (block == startblock) {
            incoming = globalVersion;
        } else {
            for (auto* pred : predecessors[block]) {
                auto& predMap = priorVers[pred];

                for (auto& [var, ver] : predMap) {
                    if (!incoming.count(var)) {
                        incoming[var] = ver;
                    } else if (incoming[var] != ver) {
                        incoming[var] = -1;  // conflict so needs phi
                    }
                }
            }
        }

        // if nothing changed, skip
        if (priorVers[block] == incoming)
            continue;

        globalVersion = incoming;

        if (block != startblock) {
            for (auto& [var, ver] : incoming) {

                if (ver == -1) {
                    int newVer = ++globalVersion[var];

                    std::vector<std::pair<std::string, ValPtr>> vers;

                    for (auto* pred : predecessors[block]) {
                        vers.push_back({
                            pred->label,
                            std::make_shared<Local>(var, priorVers[pred][var])
                        });
                    }

                    auto destvar = std::make_shared<Local>(var, newVer);
                    auto phiInst = std::make_unique<Phi>(destvar, std::move(vers));

                    block->instructions.insert(
                        block->instructions.begin(),
                        std::move(phiInst)
                    );
                }
            }
        }

        for (auto& inst : block->instructions)
            inst->renameUses(globalVersion);

        block->blockTransfer->renameUses(globalVersion);

        priorVers[block] = globalVersion;

        for (auto* succ : block->getNextBlocks())
            worklist.push(succ);
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