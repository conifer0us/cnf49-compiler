#include "ir.h"

#include <map>
#include <functional>
#include <tuple>

int Const::hash() const {
    return std::hash<std::string>()("<const|" + std::to_string(value) + ">");
}

int Local::hash() const {
    return std::hash<std::string>()("<local|" + name + "|v" + std::to_string(version) + ">");
}

int Global::hash() const {
    return std::hash<std::string>()("<global|" + name + ">");
}

int BinInst::hash(int lhsVN, int rhsVN) const {
    return std::hash<std::string>()
        ("<" + std::to_string((int) op) + "|" + std::to_string(lhsVN) + "|" + std::to_string(rhsVN) + ">");
}

static std::tuple<int, bool> getVN(const ValPtr &v,
                 std::map<int,int> &VN,
                 std::map<int,ValPtr> &name,
                 int &nextvn)
{
    int h = v->hash();

    // return value existing value number and false to indicate value is not new
    if (VN.contains(h))
        return std::make_tuple(VN[h], false);

    // add hash to value number map and name map
    int vn = nextvn++;
    VN[h] = vn;
    name[vn] = v;

    // return new value and true to indicate value has just been added
    return std::make_tuple(vn, true);
}

void BasicBlock::valueNumberingPass() {
    std::map<int,int> VN;
    std::map<int,ValPtr> name;
   
    int nextvn = 1;

    for (auto &instPtr : instructions) {
        if (auto asn = dynamic_cast<Assign *>(instPtr.get())) {
            auto [srcVN, newval] = getVN(asn->src, VN, name, nextvn);
            
            ValPtr subVal = name[srcVN];
            ValPtr dest = asn->dest;
        
            if (!newval) {
                instPtr = std::make_unique<Assign>(asn->dest, subVal);
            }        

            VN[dest->hash()] = srcVN;
        }
        else if (auto bin = dynamic_cast<BinInst *>(instPtr.get())) {
            auto [lhsVN, newLHS] = getVN(bin->lhs, VN, name, nextvn);
            auto [rhsVN, newRHS] = getVN(bin->rhs, VN, name, nextvn);

            int H = bin->hash(lhsVN, rhsVN);

            auto dest = bin->dest;

            if (VN.contains(H)) {
                int vn = VN[H];
                ValPtr subVal = name[vn];
                instPtr = std::make_unique<Assign>(bin->dest, subVal);
                VN[dest->hash()] = vn;
            } else {
                int vn = nextvn++;
                VN[H] = vn;
                
                if (!name.contains(vn))
                    name[vn] = bin->dest;

                VN[dest->hash()] = vn;
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
