#include "ast_printer.h"

namespace dialos {
namespace compiler {

std::string ASTPrinter::print(ASTNode& node) {
    output_ = "";
    indent_ = 0;
    node.accept(*this);
    return output_;
}

void ASTPrinter::printIndent() {
    for (int i = 0; i < indent_; i++) {
        output_ += " ";
    }
}

void ASTPrinter::println(const std::string& str) {
    printIndent();
    output_ += str;
    output_ += "\n";
}

void ASTPrinter::print(const std::string& str) {
    output_ += str;
}

std::string ASTPrinter::binOpToString(BinaryExpression::Operator op) {
    switch (op) {
        case BinaryExpression::Operator::ADD: return "+";
        case BinaryExpression::Operator::SUB: return "-";
        case BinaryExpression::Operator::MUL: return "*";
        case BinaryExpression::Operator::DIV: return "/";
        case BinaryExpression::Operator::MOD: return "%";
        case BinaryExpression::Operator::EQ: return "=";
        case BinaryExpression::Operator::NE: return "!=";
        case BinaryExpression::Operator::LT: return "<";
        case BinaryExpression::Operator::GT: return ">";
        case BinaryExpression::Operator::LE: return "<=";
        case BinaryExpression::Operator::GE: return ">=";
        case BinaryExpression::Operator::AND: return "and";
        case BinaryExpression::Operator::OR: return "or";
    }
    return "?";
}

std::string ASTPrinter::unOpToString(UnaryExpression::Operator op) {
    switch (op) {
        case UnaryExpression::Operator::NEG: return "-";
        case UnaryExpression::Operator::NOT: return "not";
        case UnaryExpression::Operator::PLUS: return "+";
    }
    return "?";
}

std::string ASTPrinter::typeKindToString(PrimitiveType::Kind kind) {
    switch (kind) {
        case PrimitiveType::Kind::INT: return "int";
        case PrimitiveType::Kind::UINT: return "uint";
        case PrimitiveType::Kind::BYTE: return "byte";
        case PrimitiveType::Kind::SHORT: return "short";
        case PrimitiveType::Kind::FLOAT: return "float";
        case PrimitiveType::Kind::BOOL: return "bool";
        case PrimitiveType::Kind::STRING: return "string";
        case PrimitiveType::Kind::VOID: return "void";
        case PrimitiveType::Kind::ANY: return "any";
    }
    return "?";
}

// Expressions
void ASTPrinter::visit(NumberLiteral& node) {
    print("Number(" + node.value + ")");
}

void ASTPrinter::visit(StringLiteral& node) {
    print("String(\"" + node.value + "\")");
}

void ASTPrinter::visit(BooleanLiteral& node) {
    print(node.value ? "Boolean(true)" : "Boolean(false)");
}

void ASTPrinter::visit(NullLiteral& node) {
    print("Null");
}

void ASTPrinter::visit(Identifier& node) {
    print("Identifier(" + node.name + ")");
}

void ASTPrinter::visit(BinaryExpression& node) {
    print("Binary(");
    node.left->accept(*this);
    print(" " + binOpToString(node.op) + " ");
    node.right->accept(*this);
    print(")");
}

void ASTPrinter::visit(UnaryExpression& node) {
    print("Unary(" + unOpToString(node.op) + " ");
    node.operand->accept(*this);
    print(")");
}

void ASTPrinter::visit(TernaryExpression& node) {
    print("Ternary(");
    node.condition->accept(*this);
    print(" ? ");
    node.consequence->accept(*this);
    print(" : ");
    node.alternative->accept(*this);
    print(")");
}

void ASTPrinter::visit(CallExpression& node) {
    print("Call(");
    node.callee->accept(*this);
    print(", [");
    for (size_t i = 0; i < node.arguments.size(); i++) {
        if (i > 0) print(", ");
        node.arguments[i]->accept(*this);
    }
    print("])");
}

void ASTPrinter::visit(MemberAccess& node) {
    print("Member(");
    node.object->accept(*this);
    print("." + node.property + ")");
}

void ASTPrinter::visit(ArrayAccess& node) {
    print("ArrayAccess(");
    node.array->accept(*this);
    print("[");
    node.index->accept(*this);
    print("])");
}

void ASTPrinter::visit(ArrayLiteral& node) {
    print("Array[");
    for (size_t i = 0; i < node.elements.size(); i++) {
        if (i > 0) print(", ");
        node.elements[i]->accept(*this);
    }
    print("]");
}

void ASTPrinter::visit(ConstructorCall& node) {
    print("Constructor(" + node.typeName + ", [");
    for (size_t i = 0; i < node.arguments.size(); i++) {
        if (i > 0) print(", ");
        node.arguments[i]->accept(*this);
    }
    print("])");
}

void ASTPrinter::visit(TemplateLiteral& node) {
    print("Template(`");
    for (auto& part : node.parts) {
        if (part.type == TemplateLiteral::Part::STRING) {
            print(part.stringValue);
        } else {
            print("${");
            part.expression->accept(*this);
            print("}");
        }
    }
    print("`)");
}

void ASTPrinter::visit(ParenthesizedExpression& node) {
    print("(");
    node.expression->accept(*this);
    print(")");
}

// Types
void ASTPrinter::visit(PrimitiveType& node) {
    print("Type(" + typeKindToString(node.kind) + ")");
}

void ASTPrinter::visit(NamedType& node) {
    print("Type(" + node.name + ")");
}

void ASTPrinter::visit(ArrayType& node) {
    print("Type(");
    node.elementType->accept(*this);
    print("[])");
}

void ASTPrinter::visit(NullableType& node) {
    print("Type(");
    node.baseType->accept(*this);
    print("?)");
}

// Statements
void ASTPrinter::visit(VariableDeclaration& node) {
    println("VarDecl: " + node.name);
    increaseIndent();
    printIndent();
    print("Value: ");
    node.initializer->accept(*this);
    print("\n");
    decreaseIndent();
}

void ASTPrinter::visit(Assignment& node) {
    println("Assignment:");
    increaseIndent();
    printIndent();
    print("Target: ");
    node.target->accept(*this);
    print("\n");
    printIndent();
    print("Value: ");
    node.value->accept(*this);
    print("\n");
    decreaseIndent();
}

void ASTPrinter::visit(Block& node) {
    println("Block {");
    increaseIndent();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    decreaseIndent();
    println("}");
}

void ASTPrinter::visit(IfStatement& node) {
    println("If:");
    increaseIndent();
    printIndent();
    print("Condition: ");
    node.condition->accept(*this);
    print("\n");
    println("Then:");
    increaseIndent();
    node.consequence->accept(*this);
    decreaseIndent();
    if (node.alternative) {
        println("Else:");
        increaseIndent();
        node.alternative->accept(*this);
        decreaseIndent();
    }
    decreaseIndent();
}

void ASTPrinter::visit(WhileStatement& node) {
    println("While:");
    increaseIndent();
    printIndent();
    print("Condition: ");
    node.condition->accept(*this);
    print("\n");
    println("Body:");
    increaseIndent();
    node.body->accept(*this);
    decreaseIndent();
    decreaseIndent();
}

void ASTPrinter::visit(ForStatement& node) {
    println("For:");
    increaseIndent();
    println("Init:");
    increaseIndent();
    node.initializer->accept(*this);
    decreaseIndent();
    printIndent();
    print("Condition: ");
    node.condition->accept(*this);
    print("\n");
    println("Increment:");
    increaseIndent();
    node.increment->accept(*this);
    decreaseIndent();
    println("Body:");
    increaseIndent();
    node.body->accept(*this);
    decreaseIndent();
    decreaseIndent();
}

void ASTPrinter::visit(ReturnStatement& node) {
    printIndent();
    print("Return");
    if (node.value) {
        print(": ");
        node.value->accept(*this);
    }
    print("\n");
}

void ASTPrinter::visit(ExpressionStatement& node) {
    printIndent();
    print("ExprStmt: ");
    node.expression->accept(*this);
    print("\n");
}

void ASTPrinter::visit(TryStatement& node) {
    println("Try:");
    increaseIndent();
    node.body->accept(*this);
    decreaseIndent();
    if (node.catchBlock) {
        println("Catch (" + node.errorVar + "):");
        increaseIndent();
        node.catchBlock->accept(*this);
        decreaseIndent();
    }
    if (node.finallyBlock) {
        println("Finally:");
        increaseIndent();
        node.finallyBlock->accept(*this);
        decreaseIndent();
    }
}

// Declarations
void ASTPrinter::visit(Parameter& node) {
    print(node.name + ": ");
    node.type->accept(*this);
}

void ASTPrinter::visit(FunctionDeclaration& node) {
    println("Function: " + node.name);
    increaseIndent();
    printIndent();
    print("Parameters: [");
    for (size_t i = 0; i < node.parameters.size(); i++) {
        if (i > 0) print(", ");
        node.parameters[i]->accept(*this);
    }
    print("]\n");
    if (node.returnType) {
        printIndent();
        print("ReturnType: ");
        node.returnType->accept(*this);
        print("\n");
    }
    node.body->accept(*this);
    decreaseIndent();
}

void ASTPrinter::visit(FieldDeclaration& node) {
    printIndent();
    print("Field: " + node.name + ": ");
    node.type->accept(*this);
    print("\n");
}

void ASTPrinter::visit(ConstructorDeclaration& node) {
    println("Constructor:");
    increaseIndent();
    printIndent();
    print("Parameters: [");
    for (size_t i = 0; i < node.parameters.size(); i++) {
        if (i > 0) print(", ");
        node.parameters[i]->accept(*this);
    }
    print("]\n");
    node.body->accept(*this);
    decreaseIndent();
}

void ASTPrinter::visit(MethodDeclaration& node) {
    println("Method: " + node.name);
    increaseIndent();
    printIndent();
    print("Parameters: [");
    for (size_t i = 0; i < node.parameters.size(); i++) {
        if (i > 0) print(", ");
        node.parameters[i]->accept(*this);
    }
    print("]\n");
    if (node.returnType) {
        printIndent();
        print("ReturnType: ");
        node.returnType->accept(*this);
        print("\n");
    }
    node.body->accept(*this);
    decreaseIndent();
}

void ASTPrinter::visit(ClassDeclaration& node) {
    println("Class: " + node.name);
    increaseIndent();
    println("Fields:");
    increaseIndent();
    for (auto& field : node.fields) {
        field->accept(*this);
    }
    decreaseIndent();
    if (node.constructor) {
        node.constructor->accept(*this);
    }
    println("Methods:");
    increaseIndent();
    for (auto& method : node.methods) {
        method->accept(*this);
    }
    decreaseIndent();
    decreaseIndent();
}

void ASTPrinter::visit(Program& node) {
    println("Program:");
    increaseIndent();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    decreaseIndent();
}

} // namespace compiler
} // namespace dialos
