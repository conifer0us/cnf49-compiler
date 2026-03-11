#include "ir.h"
#include <queue>
#include <algorithm>

void CFG::convertSSA() {
    for (auto & [_, method] : methodinfo) {
        method->convertSSA();
    }
}

void MethodIR::computeBlockPredecessors() {
    for (auto& block : blocks)
        block->predecessors.clear();
    
    for (auto& block : blocks)
        for (auto* succ : block->getNextBlocks()) 
            succ->predecessors.insert(block.get());
}

// get elements that are in all sets present in a vector
// used to get all blocks that are in the dominator sets of predecessor blocks
std::set<BasicBlock *> compoundIntersection(const std::vector<std::set<BasicBlock *>>& sets) {
    if (sets.empty()) return {};

    std::set<BasicBlock *> result = sets[0];

    for (size_t i = 1; i < sets.size(); ++i) {
        std::set<BasicBlock *> temp;
        std::set_intersection(result.begin(), result.end(),
                              sets[i].begin(), sets[i].end(),
                              std::inserter(temp, temp.begin()));

        result = std::move(temp);
    }

    return result;
}

void MethodIR::populateDominators() {
    // make sure to calculate block predecessors
    computeBlockPredecessors();

    int numblocks = blocks.size();

    std::set<BasicBlock *> allBlocks;

    for (auto& block : blocks)
        allBlocks.insert(block.get());

    // aside from start, set all block dominators to all other blocks
    for (auto &block : blocks) {
        block->immediateDominator = nullptr;
        block->dominancefront.clear();
        block->dominators.clear();
        block->domChildren.clear();
        block->dominators = allBlocks;
    }

    blocks[0]->dominators.clear();
    blocks[0]->dominators.insert(blocks[0].get());
        
    // calculate dominators by iterating until fixed point reached
    bool changed = true;
    while (changed) {
        changed = false;

        for (auto& block : blocks) {
            std::vector<std::set<BasicBlock *>> predecessorDoms;

            for (auto& pred : block->predecessors)
                predecessorDoms.push_back(pred->dominators);

            auto temp = compoundIntersection(predecessorDoms);
            temp.insert(block.get());

            if (temp != block->dominators) {
                block->dominators = temp;
                changed = true;
            }
        }
    }

    // calculate immediate dominator and dominator children
    for (auto& block : blocks) {
        if (block.get() == blocks[0].get())
            continue;

        for (auto dom : block->dominators) {
            if (dom == block.get())
                continue;

            bool isIdom = true;

            // if current dominator dominates another dominator of current block, must not be dom parent
            for (auto other : block->dominators) {
                if (other == dom || other == block.get())
                    continue;

                if (other->dominators.contains(dom))
                    isIdom = false;
            }

            // populate info if immediate dom found
            if (isIdom) {
                block->immediateDominator = dom;
                dom->domChildren.insert(block.get());
                break;
            }
        }
    }

    // calculate dominance front
    for (auto& block : blocks) {
        for (auto &pred : block->predecessors) {
            auto runner = pred;

            while (runner != block->immediateDominator) {
                runner->dominancefront.insert(block.get());
                runner = runner->immediateDominator;
            }
        }
    }
}

void MethodIR::convertSSA() {
    populateDominators();

    std::set<std::string> globals;
    std::map<std::string, std::set<BasicBlock *>> defBlocks;

    // build out set of global vars and maps to where the vars are defined
    for (auto &block : blocks) {
        for (auto &inst : block->instructions) {

            for (auto &varused : inst->varsUsed())
                globals.insert((*varused)->getString());

            for (auto &vardef : inst->varsDef()) {
                globals.insert((*vardef)->getString());
                defBlocks[(*vardef)->getString()].insert(block.get());
            }
        }

        for (auto varused : block->blockTransfer->varsUsed())
            globals.insert((*varused)->getString());
    }

    // place required phi functions
    for (auto global : globals) {
        std::set<BasicBlock*> hasPhi;
        std::vector<BasicBlock*> worklist(defBlocks[global].begin(), defBlocks[global].end());

        while (!worklist.empty()) {
            BasicBlock *b = worklist.back();
            worklist.pop_back();

            for (auto *domfrontBlock : b->dominancefront) {
                if (!hasPhi.contains(domfrontBlock)) {
                    // insert a phi for the given variable
                    domfrontBlock->blockPhi.push_back(std::move(std::make_unique<Phi>(global)));
                    
                    hasPhi.insert(domfrontBlock);
                    
                    if (!defBlocks[global].contains(domfrontBlock))
                        worklist.push_back(domfrontBlock);
                }
            }
        }
    }

    std::map<std::string, int> counter;
    std::map<std::string, std::vector<int>> stack;

    for (auto &v : globals) {
        counter[v] = 0;
        stack[v].push_back(0);
    }

    // rename recursively down dominator tree from start block
    getStartBlock()->renameVars(counter, stack);
}

void BasicBlock::renameVars(std::map<std::string,int> &counter, std::map<std::string,std::vector<int>> &stack) {
    // populate phi nodes
    for (auto& phi: blockPhi) {
        stack[phi->outputVar].push_back(++counter[phi->outputVar]);

        phi->resultVersion = stack[phi->outputVar].back();
    }

    // rename all variable uses and definitions in instructions
    for (auto &inst : instructions)
    {
        for (auto use : inst->varsUsed())
        {
            if ((*use)->getValType() == VarType)
            {
                auto name = (*use)->getString();
                (*use) = std::make_shared<Local>(name, stack[name].back());
            }
        }

        for (auto def : inst->varsDef())
        {
            if ((*def)->getValType() == VarType)
            {
                auto name = (*def)->getString();

                int newver = ++counter[name];
                stack[name].push_back(newver);

                (*def) = std::make_shared<Local>(name, newver);
            }
        }
    }

    for (auto use : blockTransfer->varsUsed())
    {
        if ((*use)->getValType() == VarType)
        {
            auto name = (*use)->getString();
            (*use) = std::make_shared<Local>(name, stack[name].back());
        }
    }

    // populate phi nodes in block successors
    for (auto succ : getNextBlocks()) {
        for (auto &phi : succ->blockPhi) {
            int version = 0;
            
            if (!stack[phi->outputVar].empty())
                version = stack[phi->outputVar].back();
                
            phi->incoming.push_back({label, version});
        }
    }

    for (auto child : domChildren)
        child->renameVars(counter, stack);

    // pop all vars that have been updated by phis
    for (auto& phinode: blockPhi) {
        // assert(!stack[phinode->outputVar].empty());
        stack[phinode->outputVar].pop_back();
    }
        
    // pop stack of all variables with new definitions in the function
    for (auto &inst : instructions) {
        for (auto &defVar : inst->varsDef()) {
            // assert(!stack[(*defVar)->getString()].empty());
            stack[(*defVar)->getString()].pop_back();
        }
    }
}

/*
NAIVE SSA IMPLEMENTATION FROM MILESTONE 1
BROKEN IMPLEMENTATION BUT STUCK HERE LIKE A CIRCUS FREAK
RENAMEUSES REMOVED FOR BEING ABSURD IN NEW IMPLEMENTATION

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

    for (auto&tmp : temps) {
        globalVersion[tmp] = 0;
    } 
        
    // Compute block predecessors
    for (auto& block : blocks) {
        for (auto* succ : block->getNextBlocks()) {
            predecessors[succ].push_back(block.get());
        }
    }

    std::queue<BasicBlock *> worklist;
    std::set<BasicBlock *> done;
    worklist.push(startblock);

    // insert phi nodes for every variable in blocks with multiple predecessors
    while (!worklist.empty()) {
        auto block = worklist.front();
        worklist.pop();

        if (done.contains(block))
            continue;

        // if block has multiple predecessors, insert placeholders for phi and move on
        if (predecessors[block].size() > 1) {
            for (auto& [var, _] : globalVersion) {
                phiout[block][var] = std::make_shared<Local>(var, ++globalVersion[var]);
            }
        }

        // if predecessor of block not done, do predecessors first
        else if (predecessors[block].size() == 1){
            if (!done.contains(predecessors[block][0])) {
                worklist.push(predecessors[block][0]);
                worklist.push(block);
                continue;
            }
        }

        for (auto& inst : block->instructions) {
            inst->renameUses(globalVersion);
        }

        block->blockTransfer->renameUses(globalVersion);
        versionsEnd[block] = globalVersion;
        done.insert(block);
        
        for (auto item : block->getNextBlocks())
            worklist.push(item);
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
*/