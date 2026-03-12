#include "ir.h"

#include <map>
#include <functional>

int Const::hash() const {
    return std::hash<std::string>()("<const|" + std::to_string(value) + ">");
}

int Local::hash() const {
    return std::hash<std::string>()("<local|" + name + "|v" + std::to_string(version) + ">");
}

int Global::hash() const {
    return std::hash<std::string>()("<global|" + name + ">");
}

int BinInst::hash() const {
    return std::hash<std::string>()
        ("<" + std::to_string((int) op) + "|" + std::to_string(lhs->hash()) + "|" + std::to_string(rhs->hash()) + ">");
}

void BasicBlock::valueNumberingPass() {
    std::map<int, int> VN; // maps bininst and var hashes to value numbers
    std::map<int, ValPtr> name; // maps value numbers to variables

    int nextvn = 1;

    for (auto &instPtr : instructions) {
        if (auto bin = dynamic_cast<BinInst*>(instPtr.get())) {
            int H = bin->hash();

            if (VN.contains(H)) {
                int vn = VN[H];
                ValPtr canon = name[vn];

                instPtr = std::make_unique<Assign>(bin->dest, canon);
            } else {
                int vn = nextvn++;
                VN[H] = vn;
                name[vn] = bin->dest;
            }
        }
    }
}

void CFG::valueNumberingPass() {
    for (auto &[name, method] : methodinfo) {
        for (auto &block : method->blocks) {
            block->valueNumberingPass();
        }
    }
}