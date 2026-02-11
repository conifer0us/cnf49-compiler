#pragma once

#include <memory>
#include <string>
#include <format>
#include <vector>
#include <set>
#include <map>

struct Value {
    virtual ~Value();
    virtual void outputIR() const;
};

struct Local : Value {
    std::string name;
    int version;

    explicit Local(std::string n, int v):
        name(std::move(n)), version(v) {}

    void outputIR() const override;
};

struct Global : Value {
    std::string name;
    
    explicit Global(std::string n):
        name(std::move(n)) {}

    void outputIR() const override;
};

struct Const : Value {
    long value;
    explicit Const(long v) : value(v) {}

    void outputIR() const override;
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
    Local dest;
    Value src;

    void outputIR() const override;
    
    Assign(Local d, Value s): 
        dest(d), src(s) {}
};

struct BinInst : IROp {
    Local dest;
    Oper op;
    Value lhs;
    Value rhs;

    void outputIR() const override;
    
    BinInst(Local d, Oper o, Value l, Value r): 
        dest(d), op(o), lhs(l), rhs(r) {}
};

struct Call : IROp {
    Local dest;
    Value code;
    Value receiver;
    std::vector<Value> args;

    void outputIR() const override;
    
    Call(Local d, Value c, Value r, std::vector<Value> a): 
        dest(d), code(c), receiver(r), args(std::move(a)) {}
};

struct Phi : IROp {
    Local dest;

    // even number of pairs: (predecessor name, value)
    std::vector<std::pair<std::string, Value>> incoming;

    void outputIR() const override;
    
    Phi(Local d, std::vector<std::pair<std::string, Value>> in): 
        dest(d), incoming(std::move(in)) {}
};

struct Alloc : IROp {
    Local dest;
    int numSlots;

    void outputIR() const override;
    
    Alloc(Local d, int n): 
        dest(d), numSlots(n) {}
};

struct Print : IROp {
    Value val;
    
    void outputIR() const override;
    
    explicit Print(Value v): 
        val(v) {}
};

struct GetElt : IROp {
    Local dest;
    Value array;
    Value index;

    void outputIR() const override;
    
    GetElt(Local d, Value a, Value i): 
        dest(d), array(a), index(i) {}
};

struct SetElt : IROp {
    Value array;
    Value index;
    Value val;

    void outputIR() const override;
    
    SetElt(Value a, Value i, Value v): 
           array(a), index(i), val(v) {}
};

struct Load : IROp {
    Local dest;
    Value addr;

    void outputIR() const override;
    
    Load(Local d, Value addy): 
        dest(d), addr(addy) {}
};

struct Store : IROp {
    Value addr;
    Value val;

    void outputIR() const override;
    
    Store(Value addy, Value v): 
        addr(addy), val(v) {}
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
    Value condition;
    BasicBlock *trueTarget;
    BasicBlock *falseTarget;

    Conditional(Value cond, BasicBlock *t, BasicBlock *f): 
        condition(cond), trueTarget(t), falseTarget(f) {}

    void outputIR() const override;
    
    std::set<BasicBlock *> successors() const override {
        return {trueTarget, falseTarget};
    }
};

struct Return : ControlTransfer {
    Value val;

    std::set<BasicBlock *> successors() const override {
        return {};
    }

    void outputIR() const override;

    explicit Return(Value v): 
        val(v) {}
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

    int lastblknum = 0;

public:
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    BasicBlock *getStartingBlock() { return startBlock; }

    BasicBlock *newBasicBlock() {
        std::string bname;
        if (lastblknum++ == 0)
            bname = name;
        else
            bname = std::format("%s%d", name, lastblknum);

        auto newBlock = std::make_unique<BasicBlock>(bname);
        blocks.push_back(std::move(newBlock));
        return newBlock.get();
    }

    std::vector<std::string> getLocals() {
        return locals;
    }

    void outputIR() const;

    ~MethodIR() = default;
    MethodIR(std::string nm, std::vector<std::string> lcls): name(std::move(nm)), locals(lcls) {
        startBlock = newBasicBlock();
    }
};

struct CFG {
    std::vector<std::string> classfields;
    std::vector<std::string> classmethods;
    std::map<std::string, std::unique_ptr<ClassMetadata>> classinfo;
    std::map<std::string, std::unique_ptr<MethodIR>> methodinfo;

    void outputIR() const;

    CFG (std::vector<std::string> allfields, std::vector<std::string> allmethods,
            std::map<std::string, std::unique_ptr<ClassMetadata>> classdata,
            std::map<std::string, std::unique_ptr<MethodIR>> methodIR):
        classfields(std::move(allfields)), 
        classmethods(std::move(allmethods)), 
        classinfo(std::move(classdata)),
        methodinfo(std::move(methodIR)) {}
};
