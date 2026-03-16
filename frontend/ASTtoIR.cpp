#include "ASTNodes.h"
#include "irbuilder.h"

ValPtr ThisExpr::convertToIR(IRBuilder& builder, LclPtr out) const {
    auto newLocal = std::make_shared<Local>(Local("this", 0));
    
    if (out) {
        builder.addInstruction(std::move(std::make_unique<Assign>(out, newLocal)));
        return out;
    }
    else
        return newLocal;
}

ValPtr NullExpr::convertToIR(IRBuilder& builder, LclPtr out) const {
    // null just represents a 0 pointer
    auto newNull = std::make_shared<Const>(0);

    if (out) {
        builder.addInstruction(std::move(std::make_unique<Assign>(out, newNull)));
        return out;
    } 
    else
        return newNull;
}

ValPtr Constant::convertToIR(IRBuilder& builder, LclPtr out) const {
    // mark any const read from the program to tag on output
    auto newConst = std::make_shared<Const>(value);
    
    if (out) {
        builder.addInstruction(std::move(std::make_unique<Assign>(out, newConst)));
        return out;
    }
    else
        return newConst;
}

ValPtr Var::convertToIR(IRBuilder& builder, LclPtr out) const {
    // do not increment for SSA since var is only being read in this context
    // if written to, var incremented at statement level, with var being passed in as LclPtr 
    auto newVar = std::make_shared<Local>(name, 0);
    
    if (out) { 
        builder.addInstruction(std::move(std::make_unique<Assign>(out, newVar)));
        return out;
    }
    else
        return newVar;
}

ValPtr ClassRef::convertToIR(IRBuilder& builder, LclPtr out) const {
    auto var = (out) ? out : builder.getNextTemp();
    
    auto vtable = std::make_shared<Global>(VTABLE(classname));
    auto ftable = std::make_shared<Global>(FTABLE(classname));

    // 2 already factored into object size so take as is
    int memspace = builder.getClassSize(classname);
    auto allocInst = std::make_unique<Alloc>(var, memspace);

    builder.addInstruction(std::move(allocInst));
    
    auto storeVtbl = std::make_unique<Store>(var, vtable);
    builder.addInstruction(std::move(storeVtbl));

    auto ftblVar = builder.getNextTemp();
    auto ftblAddr = std::make_unique<BinInst>(ftblVar, Oper::Add, var, std::make_shared<Const>(8));
    builder.addInstruction(std::move(ftblAddr));

    auto storeFtbl = std::make_unique<Store>(ftblVar, ftable);
    builder.addInstruction(std::move(storeFtbl));

    return var;
}

ValPtr Binop::convertToIR(IRBuilder& builder, LclPtr out) const {
    // make sure every value passed into binop is placed in a local so that tagging and untagging can be done
    auto lOut = builder.getNextTemp();
    auto rOut = builder.getNextTemp();
    
    auto result = out ? out : builder.getNextTemp();

    // operations by default allow for only ints, but equal and not equal allow pointers to be considered
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
            optype = Oper::Eq;
            break; 
        case 'n':
            optype = Oper::Ne;
            break;
        default:
            throw std::runtime_error("Unknown Operation: " + op);
    }

    auto lhsVar = lhs->convertToIR(builder, lOut);
    auto rhsVar = rhs->convertToIR(builder, rOut);

    auto binInst = std::make_unique<BinInst>(result, optype, lhsVar, rhsVar);
    builder.addInstruction(std::move(binInst));

    return result;
}

ValPtr FieldRead::convertToIR(IRBuilder& builder, LclPtr out) const {
    auto objVar = base->convertToIR(builder, nullptr);
    auto target = out ? out : builder.getNextTemp();

    // Offset 8 is position of the field table
    auto fmapAddr = builder.getNextTemp();
    builder.addInstruction(
        std::move(std::make_unique<BinInst>(fmapAddr, Oper::Add, objVar, std::make_shared<Const>(8)))
    );

    auto fmap = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<Load>(fmap, fmapAddr)));

    auto fieldOffset = builder.getFieldOffset(fieldname);
    auto offset = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<GetElt>(offset, fmap, std::make_shared<Const>(fieldOffset))));
    
    auto fieldEntry = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<BinInst>(fieldEntry, Oper::Mul, offset, std::make_shared<Const>(8))));

    auto dneBlock = builder.createBlock();
    auto existsBlock = builder.createBlock();
    
    builder.terminate(std::move(std::make_unique<Conditional>(fieldEntry, existsBlock, dneBlock)));

    builder.setCurrentBlock(dneBlock);
    builder.terminate(std::move(std::make_unique<Fail>(FailReason::NoSuchField)));
    builder.setCurrentBlock(existsBlock);
    
    auto fieldAddr = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<BinInst>(
        fieldAddr, Oper::Add, objVar, fieldEntry) // jump to offset (fieldEntry) from table
    ));

    builder.addInstruction(std::move(std::make_unique<Load>(target, fieldAddr)));

    return target;
}

ValPtr MethodCall::convertToIR(IRBuilder& builder, LclPtr out) const {
    auto objVar = base->convertToIR(builder, nullptr);
    auto retVar = out ? out : builder.getNextTemp();

    // can directly get objectVar memory since vtable is stored at 0
    auto vtable = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<Load>(vtable, objVar)));

    auto methodIndex = builder.getMethodOffset(methodname);
    auto funcEntry = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<GetElt>(funcEntry, vtable, std::make_shared<Const>(methodIndex))));

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
    auto val = value->convertToIR(builder, target);
}

void DiscardStatement::convertToIR(IRBuilder& builder) const {
    expr->convertToIR(builder, nullptr);
}

void FieldAssignStatement::convertToIR(IRBuilder& builder) const {
    auto objVar = object->convertToIR(builder, nullptr);
    
    auto targetVal = value->convertToIR(builder, nullptr);

    // Use ftable to find the field being assigned to for the given object (ftable is +8)
    auto fmapAddr = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<BinInst>(fmapAddr, Oper::Add, objVar, std::make_shared<Const>(8))));

    auto fmap = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<Load>(fmap, fmapAddr)));

    // field offset from in ftable
    auto fieldOffset = builder.getFieldOffset(field);
    auto fieldEntry = builder.getNextTemp();
    
    // tag offset values from ftable
    auto offset = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<GetElt>(offset, fmap, std::make_shared<Const>(fieldOffset))));
    builder.addInstruction(std::move(std::make_unique<BinInst>(fieldEntry, Oper::Mul, offset, std::make_shared<Const>(8))));

    auto existsBlock = builder.createBlock();
    auto dneBlock = builder.createBlock();
    builder.terminate(std::move(std::make_unique<Conditional>(fieldEntry, existsBlock, dneBlock)));    
    
    builder.setCurrentBlock(dneBlock);
    builder.terminate(std::move(std::make_unique<Fail>(FailReason::NoSuchField)));

    builder.setCurrentBlock(existsBlock);

    auto fieldAddr = builder.getNextTemp();
    builder.addInstruction(std::move(std::make_unique<BinInst>(fieldAddr, Oper::Add, objVar, fieldEntry)));

    builder.addInstruction(std::move(std::make_unique<Store>(fieldAddr, targetVal)));
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
    builder.addInstruction(std::move(std::make_unique<Print>(val)));
}

std::shared_ptr<MethodIR> Method::convertToIR(std::string classname, 
        std::map<std::string, std::unique_ptr<ClassMetadata>>& cls, 
        std::vector<std::string>& mem, 
        std::vector<std::string>& mthd, bool pinhole,
        bool mainmethod = false) const {

    std::vector<std::string> lnames;

    for (auto const& [lname, _] : typedLcls)
        lnames.push_back(lname);
    
    auto nm = mainmethod ? "main" : classname + '_' + name;

    std::vector<std::string> ags;
    ags.push_back("this");
    
    for (int i = 0; i < typedArgs.size(); i++)
        ags.push_back(typedArgs[i].first);

    auto ret = std::make_shared<MethodIR>(nm, lnames, ags);
    auto builder = IRBuilder(ret, cls, mem, mthd, pinhole);

    for (auto &name : lnames) {
        auto varVersion = std::make_shared<Local>(name, 0);

        // init method variables to 1 to make sure they're tagged
        auto initInstruction = std::make_unique<Assign>(varVersion, std::make_shared<Const>(0));
        builder.addInstruction(std::move(initInstruction));
    }

    builder.processBlock(body);
    
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
    for (const auto& [_, cls] : classes) {
        for (const auto& [_, method] : cls->methods) {
            if (!methodset.contains(method->name)) {
                methodset.insert(method->name);
                methods.push_back(method->name);
            }
        }

        for (const auto& [field, _] : cls->fieldTypes) {
            if (!fieldset.contains(field)) {
                fieldset.insert(field);
                fields.push_back(field);
            }
        }

        classinfo.insert_or_assign(cls->name, std::move(std::make_unique<ClassMetadata>((cls->name))));
    }

    // for each class build ftable for every field name
    for (const auto& [_, cls] : classes) {
        size_t offset = 2;
        int ftableindex = 0;

        for (const auto& fieldName : fields) {
            auto fieldinclass = false;

            for (const auto& [field, _] : cls->fieldTypes) {
                if (field == fieldName) {
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
    for (const auto& [_, cls] : classes) {
        for (const auto& methodName : methods) {
            auto methodinclass = false;

            for (const auto& [_, method] : cls->methods) {
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

    for (const auto& [_, cls] : classes) {
        for (const auto& [_, method] : cls->methods) {
            std::shared_ptr<MethodIR> ir = method->convertToIR(cls->name, classinfo, fields, methods, pinhole, false);

            auto nm = cls->name + '_' + method->name;
            methodinfo[nm] = ir;
        }
    }

    std::shared_ptr<MethodIR> mainir = main->convertToIR("", classinfo, fields, methods, pinhole, true);
    methodinfo["main"] = mainir;

    return std::move(std::make_unique<CFG>(fields, methods, std::move(classinfo), std::move(methodinfo)));
}
