#include "ASTNodes.h"
#include "irbuilder.h"

ValPtr ThisExpr::convertToIR(IRBuilder& builder, Local *out) const {
    auto newLocal = std::make_shared<Local>(Local("this", 0));
    
    if (out) {
            auto o = std::make_shared<Local>(out->name, out->version);
            builder.addInstruction(std::move(std::make_unique<Assign>(o, newLocal)));
            return o;
    }
    else
        return newLocal;
}

ValPtr Constant::convertToIR(IRBuilder& builder, Local *out) const {
    // mark any const read from the program to tag on output
    auto newConst = std::make_shared<Const>(value, true);
    
    if (out) {
        auto o = std::make_shared<Local>(out->name, out->version);
        builder.addInstruction(std::move(std::make_unique<Assign>(o, newConst)));
        return o;
    }
    else
        return newConst;
}

ValPtr Var::convertToIR(IRBuilder& builder, Local *out) const {
    // do not increment for SSA since var is only being read in this context
    // if written to, var incremented at statement level, with var being passed in as Local *
    auto newVar = std::make_shared<Local>(name, 0);
    
    if (out) { 
        auto o = std::make_shared<Local>(out->name, out->version);
        builder.addInstruction(std::move(std::make_unique<Assign>(o, newVar)));
        return o;
    }
    else
        return newVar;
}

ValPtr ClassRef::convertToIR(IRBuilder& builder, Local *out) const {
    auto var = std::make_shared<Local>((out) ? *out : builder.getNextTemp());
    
    auto vtable = std::make_shared<Global>(VTABLE(classname));
    auto ftable = std::make_shared<Global>(FTABLE(classname));

    // 2 already factored into object size so take as is
    int memspace = builder.getClassSize(classname);
    auto allocInst = std::make_unique<Alloc>(var, memspace);

    builder.addInstruction(std::move(allocInst));
    
    auto storeVtbl = std::make_unique<Store>(var, vtable);
    builder.addInstruction(std::move(storeVtbl));

    auto ftblVar = std::make_shared<Local>(builder.getNextTemp());
    auto ftblAddr = std::make_unique<BinInst>(ftblVar, Oper::Add, var, std::make_shared<Const>(8));
    builder.addInstruction(std::move(ftblAddr));

    auto storeFtbl = std::make_unique<Store>(ftblVar, ftable);
    builder.addInstruction(std::move(storeFtbl));

    builder.tagVal(var, TagType::Pointer);

    return var;
}

ValPtr Binop::convertToIR(IRBuilder& builder, Local *out) const {
    auto lhsVar = lhs->convertToIR(builder, nullptr);
    if (lhsVar->getValType() == ValType::VarType)
        builder.tagCheck(lhsVar, TagType::Integer);

    auto rhsVar = rhs->convertToIR(builder, nullptr);

    if (rhsVar->getValType() == ValType::VarType)
        builder.tagCheck(rhsVar, TagType::Integer);

    auto result = std::make_shared<Local>((out) ? *out : builder.getNextTemp());

    // operations by default require untagging ints, but some can unmark this, such as == or !=
    bool untag = true;
    Oper optype;
    switch(op) {
        case '+':
            optype = Oper::Add;
            break;
        case '-':
            optype = Oper::Sub;
            break;
        case '*':
            optype = Oper::Mul;
            break;
        case '/':
            optype = Oper::Div;
            break;
        case '>':
            optype = Oper::Gt;
            break;
        case '<':
            optype = Oper::Lt;
            break;
        case 'e':
            untag = false;
            optype = Oper::Eq;
            break; 
        case 'n':
            untag = false;
            optype =Oper::Ne;
            break;
        default:
            std::runtime_error("Unknown Operation: " + op);
    }

    if (untag) {
        if (lhsVar->getValType() == ValType::VarType)
            builder.untagVal(lhsVar);

        if (rhsVar->getValType() == ValType::VarType)
            builder.untagVal(rhsVar);
    }

    auto binInst = std::make_unique<BinInst>(result, optype, lhsVar, rhsVar);
    builder.addInstruction(std::move(binInst));

    if (untag) {
        if (lhsVar->getValType() == ValType::VarType)
            builder.tagVal(lhsVar, TagType::Integer);

        if (rhsVar->getValType() == ValType::VarType)
            builder.tagVal(rhsVar, TagType::Integer);
    }

    return result;
}

ValPtr FieldRead::convertToIR(IRBuilder& builder, Local* out) const {
    auto objVar = base->convertToIR(builder, nullptr);
    if (objVar->getValType() == ValType::VarType) {
        builder.tagCheck(objVar, TagType::Pointer);
        builder.untagVal(objVar);
    }
        
    auto target = std::make_shared<Local>((out) ? *out : builder.getNextTemp());

    // Offset 8 is position of the field table
    auto fmapAddr = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(
        std::move(std::make_unique<BinInst>(fmapAddr, Oper::Add, objVar, std::make_shared<Const>(8)))
    );

    auto fmap = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(std::move(std::make_unique<Load>(fmap, fmapAddr)));

    auto fieldOffset = builder.getFieldOffset(fieldname);
    auto fieldEntry = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(std::move(std::make_unique<GetElt>(fieldEntry, fmap, std::make_shared<Const>(fieldOffset * 8))));

    auto dneBlock = builder.createBlock();
    auto existsBlock = builder.createBlock();
    builder.terminate(std::move(std::make_unique<Conditional>(fieldEntry, existsBlock, dneBlock)));

    builder.setCurrentBlock(dneBlock);
    builder.terminate(std::move(std::make_unique<Fail>(FailReason::NoSuchField)));

    builder.setCurrentBlock(existsBlock);

    auto fieldAddr = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(std::move(std::make_unique<BinInst>(
        fieldAddr, Oper::Add, objVar, fieldEntry) // jump to offset (fieldEntry) from table
    ));

    builder.addInstruction(std::move(std::make_unique<Load>(target, fieldAddr)));

    builder.tagVal(objVar, TagType::Pointer);
    return target;
}

ValPtr MethodCall::convertToIR(IRBuilder& builder, Local* out) const {
    auto objVar = base->convertToIR(builder, nullptr);
    if (objVar->getValType() == ValType::VarType) {
        builder.tagCheck(objVar, TagType::Pointer);
        builder.untagVal(objVar);
    }

    auto retVar = std::make_shared<Local>((out) ? *out : builder.getNextTemp());

    // can directly get objectVar memory since vtable is stored at 0
    auto vtable = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(std::move(std::make_unique<Load>(vtable, objVar)));
    builder.tagVal(objVar, TagType::Pointer);

    auto methodIndex = builder.getMethodOffset(methodname);
    auto funcEntry = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(std::move(std::make_unique<GetElt>(funcEntry, vtable, std::make_shared<Const>(methodIndex * 8))));

    auto dneBlock = builder.createBlock();
    auto existsBlock = builder.createBlock();

    builder.terminate(std::move(std::make_unique<Conditional>(funcEntry, existsBlock, dneBlock)));

    builder.setCurrentBlock(dneBlock);
    builder.terminate(std::move(std::make_unique<Fail>(FailReason::NoSuchMethod)));
    
    builder.setCurrentBlock(existsBlock);

    // object passed to first argument for %this
    std::vector<ValPtr> argVars;
    argVars.push_back(objVar);
    for (auto& arg : args) {
        argVars.push_back(arg->convertToIR(builder, nullptr));
    }

    builder.addInstruction(std::move(std::make_unique<Call>(retVar, funcEntry, std::move(argVars))));

    return retVar;
}

void AssignStatement::convertToIR(IRBuilder& builder) const {
    auto target = std::make_shared<Local>(name, 0);
    auto val = value->convertToIR(builder, target.get());
}

void DiscardStatement::convertToIR(IRBuilder& builder) const {
    expr->convertToIR(builder, nullptr);
}

void FieldAssignStatement::convertToIR(IRBuilder& builder) const {
    auto objVar = object->convertToIR(builder, nullptr);
    if (objVar->getValType() == ValType::VarType) {
        builder.tagCheck(objVar, TagType::Pointer);
        builder.untagVal(objVar);
    }
        
    auto targetVal = value->convertToIR(builder, nullptr);

    // Use ftable to find the field being assigned to for the given object (ftable is +8)
    auto fmapAddr = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(std::move(std::make_unique<BinInst>(fmapAddr, Oper::Add, objVar, std::make_shared<Const>(8))));

    auto fmap = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(std::move(std::make_unique<Load>(fmap, fmapAddr)));

    // field offset from in ftable
    auto fieldOffset = builder.getFieldOffset(field);
    auto fieldEntry = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(std::move(std::make_unique<GetElt>(fieldEntry, fmap, std::make_shared<Const>(fieldOffset * 8))));

    auto existsBlock = builder.createBlock();
    auto dneBlock = builder.createBlock();
    builder.terminate(std::move(std::make_unique<Conditional>(fieldEntry, existsBlock, dneBlock)));    
    
    builder.setCurrentBlock(dneBlock);
    builder.terminate(std::move(std::make_unique<Fail>(FailReason::NoSuchField)));

    builder.setCurrentBlock(existsBlock);

    auto fieldAddr = std::make_shared<Local>(builder.getNextTemp());
    builder.addInstruction(std::move(std::make_unique<BinInst>(fieldAddr, Oper::Add, objVar, fieldEntry)));

    builder.addInstruction(std::move(std::make_unique<Store>(fieldAddr, targetVal)));

    builder.tagVal(objVar, TagType::Pointer);
}

void IfStatement::convertToIR(IRBuilder& builder) const {
    auto condVar = condition->convertToIR(builder, nullptr);
    
    auto thenBlock = builder.createBlock();
    auto elseBlock = builder.createBlock();
    BasicBlock* mergeBlock = nullptr;
    
    builder.terminate(std::move(std::make_unique<Conditional>(condVar, thenBlock, elseBlock)));
    builder.setCurrentBlock(thenBlock);
    
    auto terminated = builder.processBlock(thenBranch);
    if (!terminated) {
        if (!mergeBlock)
            mergeBlock = builder.createBlock();

        builder.terminate(std::move(std::make_unique<Jump>(mergeBlock)));
    }
        
    builder.setCurrentBlock(elseBlock);
    
    terminated = builder.processBlock(elseBranch);
    if (!terminated) {        
        if (!mergeBlock)
            mergeBlock = builder.createBlock();

        builder.terminate(std::move(std::make_unique<Jump>(mergeBlock)));
    }
}

void IfOnlyStatement::convertToIR(IRBuilder& builder) const {
    auto condVar = condition->convertToIR(builder, nullptr);

    auto bodyBlock = builder.createBlock();
    auto mergeBlock = builder.createBlock();
    
    builder.terminate(std::move(std::make_unique<Conditional>(condVar, bodyBlock, mergeBlock)));
    builder.setCurrentBlock(bodyBlock);
    
    auto terminated = builder.processBlock(body);
    if (!terminated)
        builder.terminate(std::move(std::make_unique<Jump>(mergeBlock)));
    
    builder.setCurrentBlock(mergeBlock);
}

void WhileStatement::convertToIR(IRBuilder& builder) const {
    // separate block to evaluate condition to make it easier to jump back to later
    auto condBlock = builder.createBlock();
    builder.terminate(std::move(std::make_unique<Jump>(condBlock)));
    
    builder.setCurrentBlock(condBlock);

    auto condVar = condition->convertToIR(builder, nullptr);    
    auto bodyBlock = builder.createBlock();
    auto mergeBlock = builder.createBlock();
    
    builder.terminate(std::move(std::make_unique<Conditional>(condVar, bodyBlock, mergeBlock)));
    builder.setCurrentBlock(bodyBlock);
    
    auto terminated = builder.processBlock(body);
    if (!terminated)
        builder.terminate(std::move(std::make_unique<Jump>(condBlock)));
    
    builder.setCurrentBlock(mergeBlock);
}

void ReturnStatement::convertToIR(IRBuilder& builder) const {
    auto val = value->convertToIR(builder, nullptr);
    builder.terminate(std::move(std::make_unique<Return>(val)));
}

void PrintStatement::convertToIR(IRBuilder& builder) const {
    auto val = value->convertToIR(builder, nullptr);
    if (val->getValType() == ValType::VarType) {
        builder.tagCheck(val, TagType::Integer);
        builder.untagVal(val);
    }
        
    builder.addInstruction(std::move(std::make_unique<Print>(val)));

    if (val->getValType() == ValType::VarType)
        builder.tagVal(val, Integer);
}

std::shared_ptr<MethodIR> Method::convertToIR(std::string classname, 
        std::map<std::string, std::unique_ptr<ClassMetadata>>& cls, 
        std::vector<std::string>& mem, 
        std::vector<std::string>& mthd, bool pinhole,
        bool mainmethod = false) const {

    std::vector<std::string> lnames;

    for (auto const& local : locals)
        lnames.push_back(local->name);
    
    auto nm = mainmethod ? "main" : classname + '_' + name;
    std::vector<std::string> ags;
    for (int i = 0; i < args.size(); i++)
        ags.push_back(args[i]->name);

    auto ret = std::make_shared<MethodIR>(nm, lnames, ags);
    auto builder = IRBuilder(ret, cls, mem, mthd, pinhole);

    for (auto &name : lnames) {
        auto varVersion = std::make_shared<Local>(name, 0);
        auto initInstruction = std::make_unique<Assign>(varVersion, std::make_shared<Const>(0));
        builder.addInstruction(std::move(initInstruction));
    }

    auto terminated = builder.processBlock(body);
    
    // if method doesn't terminate, treat as an error
    if (!terminated && !mainmethod)
        std::runtime_error("Method does not terminate: " + name);

    return ret;
};

std::unique_ptr<CFG> Program::convertToIR(bool pinhole) const {
    std::set<std::string> methodset;
    std::vector<std::string> methods;

    std::set<std::string> fieldset;
    std::vector<std::string> fields;

    std::map<std::string, std::unique_ptr<ClassMetadata>> classinfo;
    std::map<std::string, std::shared_ptr<MethodIR>> methodinfo;

    // Collect global field + method names
    for (const auto& cls : classes) {
        for (const auto& method : cls->methods) {
            if (!methodset.contains(method->name)) {
                methodset.insert(method->name);
                methods.push_back(method->name);
            }
        }

        for (const auto& field : cls->fields) {
            if (!fieldset.contains(field->name)) {
                fieldset.insert(field->name);
                fields.push_back(field->name);
            }
        }

        classinfo.insert_or_assign(cls->name, std::move(std::make_unique<ClassMetadata>((cls->name))));
    }

    // for each class build ftable for every field name
    for (const auto& cls : classes) {
        size_t offset = 2;
        int ftableindex = 0;

        for (const auto& fieldName : fields) {
            auto fieldinclass = false;

            for (const auto& field : cls->fields) {
                if (field->name == fieldName) {
                    classinfo[cls->name]->ftable.push_back(offset++);
                    fieldinclass = true;
                    break;
                }
            }
            
            if (!fieldinclass)
                classinfo[cls->name]->ftable.push_back(0);
        }

        classinfo[cls->name]->objsize = offset;
    }

    // for each class build ftable for every field name
    for (const auto& cls : classes) {
        for (const auto& methodName : methods) {
            auto methodinclass = false;

            for (const auto& method : cls->methods) {
                if (method->name == methodName) {
                    classinfo[cls->name]->vtable.push_back(cls->name + '_' + methodName);
                    methodinclass = true;
                    break;
                }
            }
            
            if (!methodinclass)
                classinfo[cls->name]->vtable.push_back("0");
        }
    }

    for (const auto& cls : classes) {
        for (const auto& method : cls->methods) {
            std::shared_ptr<MethodIR> ir = method->convertToIR(cls->name, classinfo, fields, methods, pinhole, false);

            auto nm = cls->name + '_' + method->name;
            methodinfo[nm] = ir;
        }
    }

    std::shared_ptr<MethodIR> mainir = main->convertToIR("", classinfo, fields, methods, pinhole, true);
    methodinfo["main"] = mainir;

    return std::move(std::make_unique<CFG>(fields, methods, std::move(classinfo), std::move(methodinfo)));
}
