#pragma once

#include <memory>
#include <string>
#include <format>
#include <vector>
#include <set>
#include <map>

struct Value {
    virtual ~Value() = default;
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
        dest(std::move(d)), incoming(std::move(in)) {}
};

struct Alloc : IROp {
    Local dest;
    int numSlots;

    void outputIR() const override;
    
    Alloc(Local d, int n): 
        dest(std::move(d)), numSlots(n) {}
};

struct Print : IROp {
    Value val;
    
    void outputIR() const override;
    
    explicit Print(Value v): 
        val(std::move(v)) {}
};

struct GetElt : IROp {
    Local dest;
    Value array;
    Value index;

    void outputIR() const override;
    
    GetElt(Local d, Value a, Value i): 
        dest(std::move(d)), array(std::move(a)), index(std::move(i)) {}
};

struct SetElt : IROp {
    Value array;
    Value index;
    Value val;

    void outputIR() const override;
    
    SetElt(Value a, Value i, Value v): 
           array(std::move(a)), index(std::move(i)), val(std::move(v)) {}
};

struct Load : IROp {
    Local dest;
    Value addr;

    void outputIR() const override;
    
    Load(Local d, Value addy): 
        dest(std::move(d)), addr(std::move(addy)) {}
};

struct Store : IROp {
    Value addr;
    Value val;

    void outputIR() const override;
    
    Store(Value addy, Value v): 
        addr(std::move(addy)), val(std::move(v)) {}
};

struct ControlTransfer {
    virtual ~ControlTransfer() = default;

    virtual void outputIR() const;
    virtual std::set<std::string> successors() const = 0;
};

struct Jump : ControlTransfer {
    std::string target;

    explicit Jump(std::string t) : target(std::move(t)) {}

    void outputIR() const override;
    
    std::set<std::string> successors() const override {
        return {target};
    }
};

struct Conditional : ControlTransfer {
    Value condition;
    std::string trueTarget;
    std::string falseTarget;

    Conditional(Value cond, std::string t, std::string f): 
        condition(std::move(cond)), trueTarget(std::move(t)), falseTarget(std::move(f)) {}

    void outputIR() const override;
    
    std::set<std::string> successors() const override {
        return {trueTarget, falseTarget};
    }
};

struct Return : ControlTransfer {
    Value val;

    void outputIR() const override;

    explicit Return(Value v): 
        val(std::move(v)) {}
};

struct HangingBlock : ControlTransfer {};

enum class FailReason {
    NotAPointer,
    NotANumber,
    NoSuchField,
    NoSuchMethod
};

struct Fail : ControlTransfer {
    FailReason reason;

    std::set<std::string> successors() const override {
        return {};
    }

    void outputIR() const override;

    explicit Fail(FailReason r): 
        reason(r) {}
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
            instructions = {};
            nextBlocks = {};
        } 
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
        blocks.push_back(newBlock);
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
    std::map<std::string, ClassMetadata> classinfo;
    std::map<std::string, MethodIR> methodinfo;

    void outputIR() const;

    CFG (std::vector<std::string> allfields, std::vector<std::string> allmethods,
            std::map<std::string, ClassMetadata> classdata,
            std::map<std::string, MethodIR> methodIR):
        classfields(std::move(allfields)), 
        classmethods(std::move(allmethods)), 
        classinfo(std::move(classdata)),
        methodinfo(std::move(methodIR)) {}
};
