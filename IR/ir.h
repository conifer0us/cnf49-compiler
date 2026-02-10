#pragma once

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <variant>

struct Value {
    virtual ~Value() = default;
};

struct Local : Value {
    std::string name;
    int version = 0;

    explicit Local(std::string n): 
        name(std::move(n)) {}
};

struct Constant : Value {
    int value;
    explicit Constant(int v) : value(v) {}
};

enum class BinOp {    
    Add, Sub, Mul, Div,
    BitOr, BitAnd, BitXor, 
    Eq, Gt, Lt
};

struct IROp {
    virtual ~IROp() = default;
};

struct Assign : IROp {
    std::shared_ptr<Local> dest;
    std::shared_ptr<Value> src;

    Assign(std::shared_ptr<Local> d, std::shared_ptr<Value> s): 
        dest(std::move(d)), src(std::move(s)) {}
};

struct BinInst : IROp {
    std::shared_ptr<Local> dest;
    BinOp op;
    std::shared_ptr<Value> lhs;
    std::shared_ptr<Value> rhs;

    BinInst(std::shared_ptr<Local> d, BinOp o, std::shared_ptr<Value> l, std::shared_ptr<Value> r): 
        dest(std::move(d)), op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

struct Call : IROp {
    std::shared_ptr<Local> dest;
    std::shared_ptr<Value> code;
    std::shared_ptr<Value> receiver;
    std::vector<std::shared_ptr<Value>> args;

    Call(std::shared_ptr<Local> d, std::shared_ptr<Value> c, std::shared_ptr<Value> r, std::vector<std::shared_ptr<Value>> a): 
        dest(std::move(d)), code(std::move(c)), receiver(std::move(r)), args(std::move(a)) {}
};

struct Phi : IROp {
    std::shared_ptr<Local> dest;

    // even number of pairs: (predecessor name, value)
    std::vector<std::pair<std::string, std::shared_ptr<Value>>> incoming;

    Phi(std::shared_ptr<Local> d, std::vector<std::pair<std::string, std::shared_ptr<Value>>> in): 
        dest(std::move(d)), incoming(std::move(in)) {}
};

struct Alloc : IROp {
    std::shared_ptr<Local> dest;
    int numSlots;

    Alloc(std::shared_ptr<Local> d, int n): 
        dest(std::move(d)), numSlots(n) {}
};

struct Print : IROp {
    std::shared_ptr<Value> val;
    
    explicit Print(std::shared_ptr<Value> v): 
        val(std::move(v)) {}
};

struct GetElt : IROp {
    std::shared_ptr<Local> dest;
    std::shared_ptr<Value> array;
    std::shared_ptr<Value> index;

    GetElt(std::shared_ptr<Local> d, std::shared_ptr<Value> a, std::shared_ptr<Value> i): 
        dest(std::move(d)), array(std::move(a)), index(std::move(i)) {}
};

struct SetElt : IROp {
    std::shared_ptr<Value> array;
    std::shared_ptr<Value> index;
    std::shared_ptr<Value> val;

    SetElt(std::shared_ptr<Value> a, std::shared_ptr<Value> i, std::shared_ptr<Value> v): 
           array(std::move(a)), index(std::move(i)), val(std::move(v)) {}
};

struct Load : IROp {
    std::shared_ptr<Local> dest;
    std::shared_ptr<Value> base;

    Load(std::shared_ptr<Local> d, std::shared_ptr<Value> b): 
        dest(std::move(d)), base(std::move(b)) {}
};

struct Store : IROp {
    std::shared_ptr<Value> base;
    std::shared_ptr<Value> val;

    Store(std::shared_ptr<Value> b, std::shared_ptr<Value> v): 
        base(std::move(b)), val(std::move(v)) {}
};

struct ControlTransfer {
    virtual ~ControlTransfer() = default;

    virtual std::set<std::string> successors() const = 0;
};

struct Jump : ControlTransfer {
    std::string target;

    explicit Jump(std::string t) : target(std::move(t)) {}

    std::set<std::string> successors() const override {
        return {target};
    }
};

struct Conditional : ControlTransfer {
    std::shared_ptr<Value> condition;
    std::string trueTarget;
    std::string falseTarget;

    Conditional(std::shared_ptr<Value> cond, std::string t, std::string f)
        : condition(std::move(cond)), trueTarget(std::move(t)), falseTarget(std::move(f)) {}

    std::set<std::string> successors() const override {
        return {trueTarget, falseTarget};
    }
};

struct Return : ControlTransfer {
    std::shared_ptr<Value> val;

    explicit Return(std::shared_ptr<Value> v): 
        val(std::move(v)) {}
};

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

    explicit Fail(FailReason r): 
        reason(r) {}
};

struct BasicBlock {
    std::vector<std::shared_ptr<IROp>> instructions;
    std::shared_ptr<ControlTransfer> endblock;
    std::string label;

    ~BasicBlock() = default;
    BasicBlock(std::vector<std::shared_ptr<IROp>> insts, std::shared_ptr<ControlTransfer> end, std::string lbl):
        instructions(std::move(insts)), endblock(std::move(end)), label(std::move(lbl)) {} 
};

struct ClassMetadata {
    std::string className;
    std::vector<size_t> ftable;
    std::vector<size_t> mtable;

    ~ClassMetadata() = default;
    ClassMetadata(const std::string& name, std::vector<size_t> fields, std::vector<size_t> methods): 
        className(name), ftable(std::move(fields)), mtable(std::move(methods)) {}
};

struct CFG {
    std::vector<ClassMetadata> classinfo;
    std::vector<std::shared_ptr<BasicBlock>> blocks;

    ~CFG() = default;
    CFG(std::vector<ClassMetadata> classes, std::vector<std::shared_ptr<BasicBlock>> code):
        classinfo(std::move(classes)), blocks(code) {};
};
