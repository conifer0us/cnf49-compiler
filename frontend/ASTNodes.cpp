#include "ASTNodes.h"
#include <stdexcept>

std::string ThisExpr::getType(const TypeEnv& tenv) const {
    if (!tenv.curClass)
        throw std::runtime_error("Cannot use %this outside of class declaration.");

    return tenv.curClass->name;
}

std::string NullExpr::getType(const TypeEnv&) const {
    return type;
}

std::string Constant::getType(const TypeEnv&) const {
    return "int";
}

std::string ClassRef::getType(const TypeEnv& tenv) const {
    if (!tenv.classes.contains(classname)) 
        throw std::runtime_error("Unknown class: " + classname);
    
    return classname;
}

std::string Binop::getType(const TypeEnv& tenv) const {
    auto lt = lhs->getType(tenv);
    auto rt = rhs->getType(tenv);
    
    if (lt != rt) {
        rhs->print(1);
        lhs->print(1);
        throw std::runtime_error("Binary operation requires arguments of matching type.");
    }

    // equal and not equal operations require pointers but all others require integers
    if (op != 'n' && op != 'e' && (lt != "int" || rt != "int")) {
        throw std::runtime_error("Operation requires integer arguments" + op);
    }
        
    return "int";
}

std::string FieldRead::getType(const TypeEnv& tenv) const {
    auto baseType = base->getType(tenv);

    if (!tenv.classes.contains(baseType))
        throw std::runtime_error("Unknown class: " + baseType);

    auto& baseClass = tenv.classes[baseType];
    auto& fields = baseClass->fieldTypes;

    if (!fields.contains(fieldname))
        throw std::runtime_error("Unknown field: " + fieldname);

    return fields[fieldname];
}

std::string Var::getType(const TypeEnv& tenv) const {    
    if (!tenv.locals.contains(name)) 
        throw std::runtime_error("Unknown variable: " + name);
    
    return tenv.locals[name];
}

std::string MethodCall::getType(const TypeEnv& tenv) const {
    auto baseType = base->getType(tenv);

    if (!tenv.classes.contains(baseType))
        throw std::runtime_error("Unknown class: " + baseType);

    auto& baseClass = tenv.classes[baseType];
    auto& methods = baseClass->methods;

    if (!methods.contains(methodname))
        throw std::runtime_error("Unknown method: " + methodname);

    auto& method = methods[methodname];

    if (method->typedArgs.size() != args.size())
        throw std::runtime_error("Incorrect number of arguments for method: " + methodname);

    for (int i = 0; i < args.size(); i++) {
        auto argType = args[i]->getType(tenv);
        auto paramType = method->typedArgs[i].second;

        if (argType != paramType)
            throw std::runtime_error("Argument type mismatch in call to: " + methodname);
    }

    return method->retType;
}

void AssignStatement::typeCheck(const TypeEnv& tenv) const {
    if (!tenv.locals.contains(name))
        throw std::runtime_error("Unknown variable: " + name);
        
    auto& var = tenv.locals[name];

    if (!tenv.locals.contains(name))
        throw std::runtime_error("Unknown local: " + name);

    auto varType = tenv.locals[name];
    auto valType = value->getType(tenv);

    if (varType != valType)
        throw std::runtime_error("Assignment type mismatch for variable: " + name);
}

void DiscardStatement::typeCheck(const TypeEnv& tenv) const {
    expr->getType(tenv);
}

void FieldAssignStatement::typeCheck(const TypeEnv& tenv) const {
    auto baseType = object->getType(tenv);

    if (!tenv.classes.contains(baseType))
        throw std::runtime_error("Unknown class: " + baseType);

    auto& baseClass = tenv.classes[baseType];
    auto& fields = baseClass->fieldTypes;

    if (!fields.contains(field))
        throw std::runtime_error("Unknown field: " + field);

    auto& varType = fields[field];
    auto valType = value->getType(tenv);

    if (valType != varType)
        throw std::runtime_error("Field assignment type mismatch: " + field);
}

void IfStatement::typeCheck(const TypeEnv& tenv) const {
    auto condType = condition->getType(tenv);

    if (condType != "int")
        throw std::runtime_error("If condition must be int");

    for (const auto& stmt : thenBranch)
        stmt->typeCheck(tenv);

    for (const auto& stmt : elseBranch)
        stmt->typeCheck(tenv);
}

void IfOnlyStatement::typeCheck(const TypeEnv& tenv) const {
    auto condType = condition->getType(tenv);

    if (condType != "int")
        throw std::runtime_error("If condition must be int");

    for (const auto& stmt : body)
        stmt->typeCheck(tenv);
}

void WhileStatement::typeCheck(const TypeEnv& tenv) const {
    auto condType = condition->getType(tenv);

    if (condType != "int")
        throw std::runtime_error("While condition must be int");

    for (const auto& stmt : body)
        stmt->typeCheck(tenv);
}

void ReturnStatement::typeCheck(const TypeEnv& tenv) const {
    auto valType = value->getType(tenv);
    auto retType = tenv.curMethod->retType;

    if (retType != valType)
        throw std::runtime_error("Return type incorrect. Method should return: " + retType);
}

void PrintStatement::typeCheck(const TypeEnv& tenv) const {
    auto valType = value->getType(tenv);

    if (valType != "int")
        throw std::runtime_error("Print requires int expression");
}

void Program::typeCheck() {
    // check all types in methods
    for (auto &[_, cls] : classes) {
        for (auto &[_, method] : cls->methods) {
            method->typeCheck(classes, cls.get());
        }
    }

    main->typeCheck(classes, nullptr);
}

void Method::typeCheck(std::map<std::string, ClassPtr>& classes, Class* curClass) {
    // populate locals with method locals and args
    std::map<std::string, std::string> scopeVars;

    for (auto &[aname, atype] : typedArgs) {
        scopeVars[aname] = atype;
    }

    for (auto &[lname, ltype] : typedLcls) {
        scopeVars[lname] = ltype;
    }

    TypeEnv tenv {
        classes,
        curClass,
        scopeVars,
        this
    };

    for (auto &stmt : body) {
        stmt->typeCheck(tenv);
    }
}

ASTNode::~ASTNode() = default;
void ASTNode::print(int ind) const {
    throw std::runtime_error("Tried to print base value on IR conversion");
    return;
}

Expression::~Expression() = default;
ValPtr Expression::convertToIR(IRBuilder& builder, LclPtr out) const {
    throw std::runtime_error("Tried to print base value on IR conversion");
    return std::shared_ptr<Const>(0);
}

Statement::~Statement() = default;
void Statement::convertToIR(IRBuilder& builder) const {
    throw std::runtime_error("Tried to print base value on IR conversion");
    return;
}
