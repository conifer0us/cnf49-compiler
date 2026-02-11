#include "ASTNodes.h"

Value ThisExpr::convertToIR(IRBuilder& builder, Local *out) const {
    auto newLocal = Local("this", 0);
    if (out) {
        builder.addInstruction(std::move(std::make_unique<Assign>(*out, newLocal)));
        return *out;
    }
    else {
        return newLocal;
    }
}

Value Constant::convertToIR(IRBuilder& builder, Local *out) const {
    auto newConst = Const(value);
    if (out) {
        builder.addInstruction(std::move(std::make_unique<Assign>(*out, newConst)));
        return *out;
    }
    else {
        return newConst;
    }
}

Value Var::convertToIR(IRBuilder& builder, Local *out) const {
    // do not increment for SSA since var is only being read in this context
    // if written to, var incremented at statement level, with var being passed in as Local *
    auto newVar = builder.getSSAVar(name, false);
    if (out) {
        builder.addInstruction(std::move(std::make_unique<Assign>(*out, newVar)));
        return *out;
    }
    else {
        return newVar;
    }
}

Value ClassRef::convertToIR(IRBuilder& builder, Local *out) const {
    auto var = (out) ? *out : builder.getNextTemp();
    
    auto vtable = Global(VTABLE(classname));
    auto ftable = Global(FTABLE(classname));

    // 2 already factored into object size so take as is
    int memspace = builder.getClassSize(classname);

    auto allocInst = std::make_unique<Alloc>(var, memspace);

    builder.addInstruction(std::move(allocInst));
    
    auto storeVtbl = std::make_unique<Store>(var, vtable);
    builder.addInstruction(std::move(storeVtbl));

    auto ftblVar = builder.getNextTemp();
    auto ftblAddr = std::make_unique<BinInst>(ftblVar, Oper::Add, var, Const(8));
    builder.addInstruction(std::move(ftblAddr));

    auto storeFtbl = std::make_unique<Store>(ftblAddr, ftblVar);
    builder.addInstruction(std::move(storeFtbl));

    return var;
}

Value Binop::convertToIR(IRBuilder& builder, Local *out) const {
    auto lhsVar = lhs->convertToIR(builder, nullptr);
    auto rhsVar = rhs->convertToIR(builder, nullptr);

    auto result = (out) ? *out : builder.getNextTemp();

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
            optype =Oper::Ne;
            break;
        default:
            std::runtime_error(std::format("Unknown Operation: %c", op));
    }

    auto binInst = std::make_unique<BinInst>(result, optype, lhsVar, rhsVar);
    builder.addInstruction(std::move(binInst));

    return result;
}

Value FieldRead::convertToIR(IRBuilder& builder, Local* out) const {
    auto objVar = base->convertToIR(builder, nullptr);
    auto target = (out) ? *out : builder.getNextTemp();

    // Offset 8 is position of the field table
    auto fmapAddr = builder.getNextTemp();
    builder.addInstruction(std::make_unique<BinInst>(fmapAddr, Oper::Add, objVar, Const(8)));

    auto fmap = builder.getNextTemp();
    builder.addInstruction(std::make_unique<Load>(fmap, fmapAddr));

    auto fieldOffset = builder.getFieldOffset(fieldname);
    auto fieldEntry = builder.getNextTemp();
    builder.addInstruction(std::make_unique<GetElt>(fieldEntry, fmap, Const(fieldOffset)));

    auto dneBlock = builder.createBlock();
    builder.setCurrentBlock(dneBlock);
    builder.terminate(std::make_unique<Fail>(FailReason::NoSuchField));

    auto existsBlock = builder.createBlock();
    builder.terminate(std::make_unique<Conditional>(fieldEntry, existsBlock, dneBlock));

    builder.setCurrentBlock(existsBlock);

    auto fieldAddr = builder.getNextTemp();
    builder.addInstruction(std::make_unique<BinInst>(
        fieldAddr, Oper::Add, objVar, fieldEntry) // jump to offset (fieldEntry) from table
    );

    builder.addInstruction(std::make_unique<Load>(target, fieldAddr));
    return target;
}

Value MethodCall::convertToIR(IRBuilder& builder, Local* out) const {
    auto objVar = base->convertToIR(builder, nullptr);
    auto retVar = (out) ? *out : builder.getNextTemp();

    // Get vtable from address of pointer
    auto vtableAddr = builder.getNextTemp();
    builder.addInstruction(std::make_unique<BinInst>(vtableAddr, Oper::Add, objVar, Const(0)));

    auto vtable = builder.getNextTemp();
    builder.addInstruction(std::make_unique<Load>(vtable, vtableAddr));

    auto methodIndex = builder.getMethodOffset(methodname);
    auto funcEntry = builder.getNextTemp();
    builder.addInstruction(std::make_unique<GetElt>(funcEntry, vtable, Const(methodIndex)));

    auto dneBlock = builder.createBlock();

    auto existsBlock = builder.createBlock();

    builder.terminate(std::make_unique<Conditional>(funcEntry, existsBlock, dneBlock));

    builder.setCurrentBlock(dneBlock);
    builder.terminate(std::make_unique<Fail>(FailReason::NoSuchMethod));
    
    builder.setCurrentBlock(existsBlock);

    std::vector<Value> argVars = {objVar};
    for (auto& arg : args) {
        argVars.push_back(arg->convertToIR(builder, nullptr));
    }

    builder.addInstruction(std::make_unique<Call>(retVar, funcEntry, objVar, argVars));

    return retVar;
}

void AssignStatement::convertToIR(IRBuilder& builder) const {
    auto target = builder.getSSAVar(name, true);
    auto val = value->convertToIR(builder, &target);
}

void DiscardStatement::convertToIR(IRBuilder& builder) const {
    expr->convertToIR(builder, nullptr);
}

void FieldAssignStatement::convertToIR(IRBuilder& builder) const {
    auto objVar = object->convertToIR(builder, nullptr);
    auto targetVal = value->convertToIR(builder, nullptr);

    // Use ftable to find the field being assigned to for the given object (ftable is +8)
    auto fmapAddr = builder.getNextTemp();
    builder.addInstruction(std::make_unique<BinInst>(fmapAddr, Oper::Add, objVar, Const(8)));

    auto fmap = builder.getNextTemp();
    builder.addInstruction(std::make_unique<Load>(fmap, fmapAddr));

    // field offset from in ftable
    auto fieldOffset = builder.getFieldOffset(field);
    auto fieldEntry = builder.getNextTemp();
    builder.addInstruction(std::make_unique<GetElt>(fieldEntry, fmap, Const(fieldOffset)));

    auto existsBlock = builder.createBlock();
    auto dneBlock = builder.createBlock();
    builder.terminate(std::make_unique<Conditional>(fieldEntry, existsBlock, dneBlock));    
    
    builder.setCurrentBlock(dneBlock);
    builder.terminate(std::make_unique<Fail>(FailReason::NoSuchField));

    builder.setCurrentBlock(existsBlock);

    auto fieldAddr = builder.getNextTemp();
    builder.addInstruction(std::make_unique<BinInst>(fieldAddr, Oper::Add, objVar, fieldEntry));

    builder.addInstruction(std::make_unique<Store>(fieldAddr, targetVal));
}

void IfStatement::convertToIR(IRBuilder& builder) const {
    auto condVar = condition->convertToIR(builder, nullptr);
    
    auto thenBlock = builder.createBlock();
    auto elseBlock = builder.createBlock();
    auto mergeBlock = builder.createBlock();
    
    builder.terminate(std::make_unique<Conditional>(condVar, thenBlock, elseBlock));
    builder.setCurrentBlock(thenBlock);
    
    auto terminated = builder.processBlock(thenBranch);
    if (!terminated)
        builder.terminate(std::make_unique<Jump>(mergeBlock));
    
    builder.setCurrentBlock(elseBlock);
    
    terminated = builder.processBlock(elseBranch);
    if (!terminated)
        builder.terminate(std::make_unique<Jump>(mergeBlock));
    
    builder.setCurrentBlock(mergeBlock);
}

void IfOnlyStatement::convertToIR(IRBuilder& builder) const {
    auto condVar = condition->convertToIR(builder, nullptr);

    auto bodyBlock = builder.createBlock();
    auto mergeBlock = builder.createBlock();
    
    builder.terminate(std::make_unique<Conditional>(condVar, bodyBlock, mergeBlock));
    builder.setCurrentBlock(bodyBlock);
    
    auto terminated = builder.processBlock(body);
    if (!terminated)
        builder.terminate(std::make_unique<Jump>(mergeBlock));
    
    builder.setCurrentBlock(mergeBlock);
}

void WhileStatement::convertToIR(IRBuilder& builder) const {
    // separate block to evaluate condition to make it easier to jump back to later
    auto condBlock = builder.createBlock();
    builder.terminate(std::make_unique<Jump>(condBlock));
    
    builder.setCurrentBlock(condBlock);

    auto condVar = condition->convertToIR(builder, nullptr);    
    auto bodyBlock = builder.createBlock();
    auto mergeBlock = builder.createBlock();
    
    builder.terminate(std::make_unique<Conditional>(condVar, bodyBlock, mergeBlock));
    builder.setCurrentBlock(bodyBlock);
    
    auto terminated = builder.processBlock(body);
    if (!terminated)
        builder.terminate(std::make_unique<Jump>(condBlock));
    
    builder.setCurrentBlock(mergeBlock);
}

void ReturnStatement::convertToIR(IRBuilder& builder) const {
    auto val = value->convertToIR(builder, nullptr);
    builder.terminate(std::make_unique<Return>(val));
}

void PrintStatement::convertToIR(IRBuilder& builder) const {
    auto val = value->convertToIR(builder, nullptr);
    builder.addInstruction(std::make_unique<Print>(val));
}

MethodIR Method::convertToIR(std::string classname, std::map<std::string, ClassMetadata>& cls, std::vector<std::string>& mem, std::vector<std::string>& mthd) const {
    std::vector<std::string> lnames;
    std::transform(locals.begin(), locals.end(), lnames.begin(), [](Var i){ return i.name; });
    auto ret = MethodIR(classname + "_" + name, lnames);

    auto builder = IRBuilder(ret, cls, mem, mthd);

    for (auto &name : lnames) {
        auto varVersion = builder.getSSAVar(name, true);
        auto initInstruction = std::make_unique<Assign>(varVersion, Const(0));
    }

    auto terminated = builder.processBlock(body);
    
    // if method doesn't terminate, treat as an error
    if (!terminated)
        std::runtime_error(std::format("Method does not terminate: %d", name));

    return ret;
};

CFG Program::convertToIR() const {
    std::set<std::string> methodset;
    std::vector<std::string> methods;

    std::set<std::string> fieldset;
    std::vector<std::string> fields;

    std::map<std::string, ClassMetadata> classinfo;
    std::map<std::string, MethodIR> methodinfo;

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

        classinfo.insert_or_assign(cls->name, ClassMetadata(cls->name));
    }

    // for each class build ftable for every field name
    for (const auto& cls : classes) {
        size_t offset = 2;
        int ftableindex = 0;

        for (const auto& fieldName : fields) {
            for (const auto& field : cls->fields) {
                if (field->name == fieldName) {
                    classinfo[cls->name].ftable.push_back(offset++);
                }

                classinfo[cls->name].ftable.push_back(0);
            }
        }

        classinfo[cls->name].objsize = offset;
    }

    // for each class build ftable for every field name
    for (const auto& cls : classes) {
        int vtableindex = 0;

        for (const auto& methodName : methods) {
            for (const auto& method : cls->methods) {
                if (method->name == methodName) {
                    classinfo[cls->name].vtable.push_back(methodName);
                }
            }

            classinfo[cls->name].vtable.push_back("0");
            vtableindex++;
        }
    }

    for (const auto& cls : classes) {
        for (const auto& method : cls->methods) {
            MethodIR ir = method->convertToIR(cls->name, classinfo, fields, methods);

            methodinfo.insert_or_assign(cls->name + "::" + method->name, std::move(ir));
        }
    }

    return CFG(fields, methods, classinfo, methodinfo);
}
