#pragma once

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>

enum TagType { Pointer = 0, Integer = 1 };
enum ValType { VarType = 0, ConstType = 1, GlobalType = 2};

struct Value {
    bool ignoreSSA;

    virtual ~Value();
    virtual void outputIR() const = 0;
    virtual std::string getString() const = 0;
    virtual ValType getValType() const = 0;
    virtual int hash() const = 0;
};

using ValPtr = std::shared_ptr<Value>;

struct Local : Value {
    std::string name;
    int version;

    explicit Local(std::string n, int v, bool tempVal = false):
        name(std::move(n)), version(v) {
            ignoreSSA = tempVal;
        }

    void outputIR() const override;
    std::string getString() const override;
    ValType getValType() const override;
    int hash() const override;
};

using LclPtr = std::shared_ptr<Local>;

struct Global : Value {
    std::string name;
    
    explicit Global(std::string n):
        name(std::move(n)) {
            ignoreSSA = true;
        }

    void outputIR() const override;
    std::string getString() const override;
    ValType getValType() const override;
    int hash() const override;
};

struct Const : Value {
    long value;

    explicit Const(long v, bool tag = false) {
        ignoreSSA = true;

        if (tag)
            value = (v << 1) | 1;
        else
            value = v;
    }

    void outputIR() const override;
    std::string getString() const override;
    ValType getValType() const override;
    int hash() const override;
};

enum class Oper {    
    Add, Sub, Mul, Div,
    BitOr, BitAnd, BitXor, 
    Eq, Gt, Lt, Ne
};

struct IROp {
    virtual ~IROp() = default;
    virtual void outputIR() const = 0;
    virtual std::set<ValPtr *> varsUsed() = 0;
    virtual std::set<ValPtr *> varsDef() = 0;
};

struct Assign : IROp {
    ValPtr dest;
    ValPtr src;

    void outputIR() const override;

    Assign(ValPtr d, ValPtr s): 
        dest(d), src(std::move(s)) {}

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;

        if (src && !src->ignoreSSA && src->getString() != "this")
            ret.insert(&src);
        
        return ret;
    }

    std::set<ValPtr *> varsDef() {
        std::set<ValPtr *> ret;

        if (dest && !dest->ignoreSSA && dest->getString() != "this")
            ret.insert(&dest);

        return ret;
    }
};

struct BinInst : IROp {
    ValPtr dest;
    Oper op;
    ValPtr lhs;
    ValPtr rhs;

    void outputIR() const override;
    int hash() const;

    BinInst(ValPtr d, Oper o, ValPtr l, ValPtr r): 
        dest(d), op(o), lhs(std::move(l)), rhs(std::move(r)) {}

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;

        if (lhs && !lhs->ignoreSSA && lhs->getString() != "this")
            ret.insert(&lhs);

        if (rhs && !rhs->ignoreSSA && rhs->getString() != "this")
            ret.insert(&rhs);

        return ret;
    }

    std::set<ValPtr *> varsDef() {
        std::set<ValPtr *> ret;

        if (dest && !dest->ignoreSSA && dest->getString() != "this")
            ret.insert(&dest);

        return ret;
    }
};

struct Call : IROp {
    ValPtr dest;
    ValPtr code;
    std::vector<ValPtr> args;

    void outputIR() const override;
    
    Call(ValPtr d, ValPtr c, std::vector<ValPtr> a): 
        dest(d), code(std::move(c)), args(std::move(a)) {}

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;

        for (auto& arg : args)
            if (arg && !arg->ignoreSSA && arg->getString() != "this")
                ret.insert(&arg);

        if (code && !code->ignoreSSA && code->getString() != "this")
            ret.insert(&code);

        return ret;
    }

    std::set<ValPtr *> varsDef() {
        std::set<ValPtr *> ret;

        if (dest && !dest->ignoreSSA && dest->getString() != "this")
            ret.insert(&dest);

        return ret;
    }
};

struct Phi : IROp {
    std::string outputVar;
    int resultVersion;
    std::vector<std::pair<std::string, int>> incoming;

    void outputIR() const override;
    
    explicit Phi(std::string varname): 
        outputVar(varname) {}

    // phis not checked as part of basic block body
    std::set<ValPtr *> varsUsed() {
        return {};
    }

    std::set<ValPtr *> varsDef() {
        return {};
    }
};

struct Alloc : IROp {
    ValPtr dest;
    int numSlots;

    void outputIR() const override;
    
    Alloc(ValPtr d, int n): 
        dest(d), numSlots(n) {}

    std::set<ValPtr *> varsUsed() {
        return {};
    }

    std::set<ValPtr *> varsDef() {
        std::set<ValPtr *> ret;

        if (dest && !dest->ignoreSSA && dest->getString() != "this")
            ret.insert(&dest);

        return ret;
    }
};

struct Print : IROp {
    ValPtr val;
    
    void outputIR() const override;
    
    explicit Print(ValPtr v): 
        val(std::move(v)) {}

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;

        if (val && !val->ignoreSSA && val->getString() != "this")
            ret.insert(&val);

        return ret;
    }

    std::set<ValPtr *> varsDef() {
        return {};
    }
};

struct GetElt : IROp {
    ValPtr dest;
    ValPtr array;
    ValPtr index;

    void outputIR() const override;
    
    GetElt(ValPtr d, ValPtr a, ValPtr i): 
        dest(d), array(std::move(a)), index(std::move(i)) {}

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;

        if (array && !array->ignoreSSA && array->getString() != "this")
            ret.insert(&array);

        if (index && !index->ignoreSSA && index->getString() != "this")
            ret.insert(&index);

        return ret;
    }

    std::set<ValPtr *> varsDef() {
        std::set<ValPtr *> ret;

        if (dest && !dest->ignoreSSA && dest->getString() != "this")
            ret.insert(&dest);

        return ret;
    }
};

struct SetElt : IROp {
    ValPtr array;
    ValPtr index;
    ValPtr val;

    void outputIR() const override;
    
    SetElt(ValPtr a, ValPtr i, ValPtr v): 
           array(std::move(a)), index(std::move(i)), val(std::move(v)) {}

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;

        if (array && !array->ignoreSSA && array->getString() != "this")
            ret.insert(&array);

        if (index && !index->ignoreSSA && index->getString() != "this")
            ret.insert(&index);

        if (val && !val->ignoreSSA && val->getString() != "this")
            ret.insert(&val);

        return ret;
    }

    std::set<ValPtr *> varsDef() {
        return {};
    }
};

struct Load : IROp {
    ValPtr dest;
    ValPtr addr;

    void outputIR() const override;
    
    Load(ValPtr d, ValPtr addy): 
        dest(d), addr(std::move(addy)) {}

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;

        if (addr && !addr->ignoreSSA && addr->getString() != "this")
            ret.insert(&addr);

        return ret;
    }

    std::set<ValPtr *> varsDef() {
        std::set<ValPtr *> ret;

        if (dest && !dest->ignoreSSA && dest->getString() != "this")
            ret.insert(&dest);

        return ret;
    }
};

struct Store : IROp {
    ValPtr addr;
    ValPtr val;

    void outputIR() const override;
    
    Store(ValPtr addy, ValPtr v): 
        addr(std::move(addy)), val(std::move(v)) {}

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;
        
        if (addr && !addr->ignoreSSA && addr->getString() != "this")
            ret.insert(&addr);

        if (val && !val->ignoreSSA && val->getString() != "this")
            ret.insert(&val);
        
        return ret;
    }

    std::set<ValPtr *> varsDef() {
        return {};
    }
};

struct BasicBlock;

struct ControlTransfer {
    virtual ~ControlTransfer();
    virtual void outputIR() const;
    virtual std::vector<BasicBlock*> successors() const = 0;
    virtual std::set<ValPtr *> varsUsed() = 0;
};

struct Jump : ControlTransfer {
    BasicBlock *target;

    explicit Jump(BasicBlock *t) : target(t) {}

    void outputIR() const override;
    
    std::vector<BasicBlock *> successors() const override {
        return {target};
    }

    std::set<ValPtr *> varsUsed() {
        return {};
    }
};

struct Conditional : ControlTransfer {
    ValPtr condition;
    BasicBlock *trueTarget;
    BasicBlock *falseTarget;

    Conditional(ValPtr cond, BasicBlock *t, BasicBlock *f): 
        condition(std::move(cond)), trueTarget(t), falseTarget(f) {}

    void outputIR() const override;
    
    std::vector<BasicBlock *> successors() const override {
        return {trueTarget, falseTarget};
    }

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;

        if (condition && !condition->ignoreSSA && condition->getString() != "this")
            ret.insert(&condition);

        return ret;
    }
};

struct Return : ControlTransfer {
    ValPtr val;

    std::vector<BasicBlock *> successors() const override {
        return {};
    }

    void outputIR() const override;

    std::set<ValPtr *> varsUsed() {
        std::set<ValPtr *> ret;

        if (val && !val->ignoreSSA && val->getString() != "this")
            ret.insert(&val);
            
        return ret;
    }

    explicit Return(ValPtr v): 
        val(std::move(v)) {}
};

struct HangingBlock : ControlTransfer {
    std::vector<BasicBlock *> successors() const override {
        return {};
    }

    void outputIR() const override;

    virtual ~HangingBlock();
    HangingBlock() {}

    std::set<ValPtr *> varsUsed() {
        return {};
    }
};

enum class FailReason {
    NotAPointer,
    NotANumber,
    NoSuchField,
    NoSuchMethod
};

struct Fail : ControlTransfer {
    FailReason reason;

    std::vector<BasicBlock *> successors() const override {
        return {};
    }

    void outputIR() const override;

    explicit Fail(FailReason r): 
        reason(r) {}

    std::set<ValPtr *> varsUsed() {return {};}
};

struct BasicBlock {
    std::vector<std::unique_ptr<Phi>> blockPhi;
    std::vector<std::unique_ptr<IROp>> instructions;
    std::unique_ptr<ControlTransfer> blockTransfer;
    std::string label;
    
    BasicBlock *immediateDominator;
    std::set<BasicBlock *> predecessors;
    std::set<BasicBlock *> dominators;
    std::set<BasicBlock *> domChildren;
    std::set<BasicBlock *> dominancefront;

    ~BasicBlock() = default;

    void outputIR() const;
    void valueNumberingPass();
    void convertSSA();
    void renameVars(std::map<std::string,int> &counter, std::map<std::string,std::vector<int>> &stack);
    std::vector<BasicBlock *> getNextBlocks() {
        return blockTransfer->successors();
    }
    
    BasicBlock(std::string lbl): label(std::move(lbl)) {
            // Basic Block has a hanging end while instantiating
            // Code with proper control flow will replace all instances with valid jumps
            blockTransfer = std::make_unique<HangingBlock>();
        } 
};

class MethodIR {
    std::string name;
    std::vector<std::string> locals;
    std::vector<std::string> args;
    std::vector<std::string> temps;

    int lastblknum = 0;

public:
    std::vector<std::unique_ptr<BasicBlock>> blocks;

    BasicBlock *newBasicBlock() {
        std::string bname;
        if (lastblknum == 0)
            bname = name;
        else
            bname = name + std::to_string(lastblknum);

        lastblknum++;
        auto newBlock = std::make_unique<BasicBlock>(bname);
        auto ptr = newBlock.get();
        blocks.push_back(std::move(newBlock));
        return ptr;
    }
    
    BasicBlock *getBlock(int index) { return blocks[index].get(); }
    BasicBlock *getStartBlock() { return getBlock(0); }

    std::vector<std::string> getLocals() {
        return locals;
    }

    std::vector<std::string> getArgs() {
        return args;
    }

    std::vector<std::string> getTemps() {
        return temps;
    }

    void outputIR() const;
    void computeBlockPredecessors();
    void populateDominators();
    void convertSSA();

    // register temp values with method from method builder to allow operating on them with SSA
    void registerTemp(std::string tmp) {temps.push_back(tmp);};

    ~MethodIR() = default;
    MethodIR(std::string nm, std::vector<std::string> lcls, std::vector<std::string> ars): name(nm), locals(lcls), args(ars) { newBasicBlock(); }
};

// In IR, global method table and field table labeled "vtableCLASSNAME" and "ftableCLASSNAME"
#define VTABLE(classname) Global("vtable" + classname)
#define FTABLE(classname) Global("ftable" + classname)

struct ClassMetadata {
    std::vector<std::string> vtable;
    std::vector<size_t> ftable;
    std::string name;
    size_t objsize;

    int size() {
        return objsize;
    }

    // output vtable and ftable for the method
    void outputIR(const std::vector<std::string>& methods, const std::vector<std::string>& fields) const;
    
    ClassMetadata(std::string nm) : name(nm) {}
    ~ClassMetadata() = default;
};

struct CFG {
    std::vector<std::string> classfields;
    std::vector<std::string> classmethods;
    std::map<std::string, std::unique_ptr<ClassMetadata>> classinfo;
    std::map<std::string, std::shared_ptr<MethodIR>> methodinfo;

    void outputIR() const;
    void convertSSA();
    void valueNumberingPass();

    CFG (std::vector<std::string> allfields, std::vector<std::string> allmethods,
            std::map<std::string, std::unique_ptr<ClassMetadata>> classdata,
            std::map<std::string, std::shared_ptr<MethodIR>> methodIR):
        classfields(std::move(allfields)), 
        classmethods(std::move(allmethods)), 
        classinfo(std::move(classdata)),
        methodinfo(std::move(methodIR)) {}
};
