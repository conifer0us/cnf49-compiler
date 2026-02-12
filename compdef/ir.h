#pragma once

#include <memory>
#include <string>
#include <format>
#include <vector>
#include <set>
#include <map>

enum ValType { VarType, ConstInt, Label };
enum TagType { Pointer = 0, Integer = 1 };

struct Value {
    virtual ~Value();
    virtual void outputIR() const = 0;
    virtual ValType getValType() const = 0;
    virtual std::string getString() const = 0;
};

// not ideal but avoids lots of ownership overheads while building AST into IR
// need to keep all Values only owned by the instructions they're a part of
using ValPtr = std::shared_ptr<Value>;

struct Local : Value {
    std::string name;
    int version;

    explicit Local(std::string n, int v):
        name(std::move(n)), version(v) {}

    ValType getValType() const override;
    void outputIR() const override;
    std::string getString() const override;
};

struct Global : Value {
    std::string name;
    
    explicit Global(std::string n):
        name(std::move(n)) {}

    ValType getValType() const override;
    void outputIR() const override;
    std::string getString() const override;
};

struct Const : Value {
    long value;
    bool tag;
    explicit Const(long v, bool tag = false) : value(v), tag(tag) {}

    ValType getValType() const override;
    void outputIR() const override;
    std::string getString() const override;
};

enum class Oper {    
    Add, Sub, Mul, Div,
    BitOr, BitAnd, BitXor, 
    Eq, Gt, Lt, Ne
};

struct IROp {
    virtual ~IROp() = default;
    virtual void outputIR() const = 0;
};

struct Assign : IROp {
    ValPtr dest;
    ValPtr src;

    void outputIR() const override;
    
    Assign(ValPtr d, ValPtr s): 
        dest(d), src(std::move(s)) {}
};

struct BinInst : IROp {
    ValPtr dest;
    Oper op;
    ValPtr lhs;
    ValPtr rhs;

    void outputIR() const override;
    
    BinInst(ValPtr d, Oper o, ValPtr l, ValPtr r): 
        dest(d), op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

struct Call : IROp {
    ValPtr dest;
    ValPtr code;
    std::vector<ValPtr> args;

    void outputIR() const override;
    
    Call(ValPtr d, ValPtr c, std::vector<ValPtr> a): 
        dest(d), code(std::move(c)), args(std::move(a)) {}
};

struct Phi : IROp {
    ValPtr dest;

    // even number of pairs: (predecessor name, value)
    std::vector<std::pair<std::string, ValPtr>> incoming;

    void outputIR() const override;
    
    Phi(ValPtr d, std::vector<std::pair<std::string, ValPtr>> in): 
        dest(d), incoming(std::move(in)) {}
};

struct Alloc : IROp {
    ValPtr dest;
    int numSlots;

    void outputIR() const override;
    
    Alloc(ValPtr d, int n): 
        dest(d), numSlots(n) {}
};

struct Print : IROp {
    ValPtr val;
    
    void outputIR() const override;
    
    explicit Print(ValPtr v): 
        val(std::move(v)) {}
};

struct GetElt : IROp {
    ValPtr dest;
    ValPtr array;
    ValPtr index;

    void outputIR() const override;
    
    GetElt(ValPtr d, ValPtr a, ValPtr i): 
        dest(d), array(std::move(a)), index(std::move(i)) {}
};

struct SetElt : IROp {
    ValPtr array;
    ValPtr index;
    ValPtr val;

    void outputIR() const override;
    
    SetElt(ValPtr a, ValPtr i, ValPtr v): 
           array(std::move(a)), index(std::move(i)), val(std::move(v)) {}
};

struct Load : IROp {
    ValPtr dest;
    ValPtr addr;

    void outputIR() const override;
    
    Load(ValPtr d, ValPtr addy): 
        dest(d), addr(std::move(addy)) {}
};

struct Store : IROp {
    ValPtr addr;
    ValPtr val;

    void outputIR() const override;
    
    Store(ValPtr addy, ValPtr v): 
        addr(std::move(addy)), val(std::move(v)) {}
};

// forward declare bb so that conditionals can use it
struct BasicBlock;

struct ControlTransfer {
    virtual ~ControlTransfer();
    virtual void outputIR() const;
    virtual std::set<BasicBlock*> successors() const = 0;
};

struct Jump : ControlTransfer {
    BasicBlock *target;

    explicit Jump(BasicBlock *t) : target(t) {}

    void outputIR() const override;
    
    std::set<BasicBlock *> successors() const override {
        return {target};
    }
};

struct Conditional : ControlTransfer {
    ValPtr condition;
    BasicBlock *trueTarget;
    BasicBlock *falseTarget;

    Conditional(ValPtr cond, BasicBlock *t, BasicBlock *f): 
        condition(std::move(cond)), trueTarget(t), falseTarget(f) {}

    void outputIR() const override;
    
    std::set<BasicBlock *> successors() const override {
        return {trueTarget, falseTarget};
    }
};

struct Return : ControlTransfer {
    ValPtr val;

    std::set<BasicBlock *> successors() const override {
        return {};
    }

    void outputIR() const override;

    explicit Return(ValPtr v): 
        val(std::move(v)) {}
};

struct HangingBlock : ControlTransfer {
    std::set<BasicBlock *> successors() const override {
        return {};
    }

    void outputIR() const override;

    virtual ~HangingBlock();
    HangingBlock() {}
};

enum class FailReason {
    NotAPointer,
    NotANumber,
    NoSuchField,
    NoSuchMethod
};

struct Fail : ControlTransfer {
    FailReason reason;

    std::set<BasicBlock *> successors() const override {
        return {};
    }

    void outputIR() const override;

    explicit Fail(FailReason r): 
        reason(r) {}
};

struct BasicBlock {
    std::vector<std::unique_ptr<IROp>> instructions;
    std::unique_ptr<ControlTransfer> blockTransfer;
    std::vector<BasicBlock *> nextBlocks;
    std::string label;

    ~BasicBlock() = default;

    void outputIR() const;
    
    BasicBlock(std::string lbl): label(std::move(lbl)) {
            // Basic Block has a hanging end while instantiating
            // Code with proper control flow will replace all instances with valid jumps
            blockTransfer = std::make_unique<HangingBlock>();
        } 
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

class MethodIR {
    std::string name;
    BasicBlock* startBlock;
    std::vector<std::string> locals;
    std::vector<std::string> args;

    int lastblknum = 0;

public:
    std::vector<std::unique_ptr<BasicBlock>> blocks;

    BasicBlock *newBasicBlock() {
        std::string bname;
        if (lastblknum == 0)
            bname = name;
        else
            bname = std::format("{}{}", name, lastblknum);

        lastblknum++;
        auto newBlock = std::make_unique<BasicBlock>(bname);
        auto ptr = newBlock.get();
        blocks.push_back(std::move(newBlock));
        return ptr;
    }

    BasicBlock *getStartingBlock() { return startBlock; }

    std::vector<std::string> getLocals() {
        return locals;
    }

    std::vector<std::string> getArgs() {
        return args;
    }

    void outputIR() const;

    ~MethodIR() = default;
    MethodIR(std::string nm, std::vector<std::string> lcls, std::vector<std::string> ars): name(nm), locals(lcls), args(ars) {
        startBlock = newBasicBlock();
    }
};

struct CFG {
    std::vector<std::string> classfields;
    std::vector<std::string> classmethods;
    std::map<std::string, std::unique_ptr<ClassMetadata>> classinfo;
    std::map<std::string, std::shared_ptr<MethodIR>> methodinfo;

    void outputIR() const;
    void naiveSSA();

    CFG (std::vector<std::string> allfields, std::vector<std::string> allmethods,
            std::map<std::string, std::unique_ptr<ClassMetadata>> classdata,
            std::map<std::string, std::shared_ptr<MethodIR>> methodIR):
        classfields(std::move(allfields)), 
        classmethods(std::move(allmethods)), 
        classinfo(std::move(classdata)),
        methodinfo(std::move(methodIR)) {}
};
