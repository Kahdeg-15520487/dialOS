#include "ast_json.h"
#include <algorithm>

namespace dialos {
namespace compiler {

std::string ASTJsonExporter::toJson(const Program& program) {
    json_.str("");
    json_.clear();
    indent_ = 0;
    needsComma_ = false;
    
    const_cast<Program&>(program).accept(*this);
    
    return json_.str();
}

void ASTJsonExporter::writeIndent() {
    for (int i = 0; i < indent_; i++) {
        json_ << "  ";
    }
}

void ASTJsonExporter::writeKey(const std::string& key) {
    if (needsComma_) {
        json_ << ",\n";
    } else {
        json_ << "\n";
    }
    writeIndent();
    json_ << "\"" << key << "\": ";
    needsComma_ = false;
}

void ASTJsonExporter::writeString(const std::string& value) {
    json_ << "\"" << escapeJson(value) << "\"";
    needsComma_ = true;
}

void ASTJsonExporter::writeNumber(int value) {
    json_ << value;
    needsComma_ = true;
}

void ASTJsonExporter::writeBool(bool value) {
    json_ << (value ? "true" : "false");
    needsComma_ = true;
}

void ASTJsonExporter::writeNull() {
    json_ << "null";
    needsComma_ = true;
}

void ASTJsonExporter::startObject() {
    json_ << "{";
    indent_++;
    needsComma_ = false;
}

void ASTJsonExporter::endObject() {
    json_ << "\n";
    indent_--;
    writeIndent();
    json_ << "}";
    needsComma_ = true;
}

void ASTJsonExporter::startArray() {
    json_ << "[";
    indent_++;
    needsComma_ = false;
}

void ASTJsonExporter::endArray() {
    json_ << "\n";
    indent_--;
    writeIndent();
    json_ << "]";
    needsComma_ = true;
}

std::string ASTJsonExporter::escapeJson(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

void ASTJsonExporter::visit(Program& node) {
    startObject();
    writeKey("type");
    writeString("Program");
    writeKey("statements");
    startArray();
    for (size_t i = 0; i < node.statements.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.statements[i]->accept(*this);
    }
    endArray();
    endObject();
}

void ASTJsonExporter::visit(VariableDeclaration& node) {
    startObject();
    writeKey("type");
    writeString("VariableDeclaration");
    writeKey("name");
    writeString(node.name);
    if (node.initializer) {
        writeKey("initializer");
        node.initializer->accept(*this);
    }
    writeKey("line");
    writeNumber(node.line);
    writeKey("column");
    writeNumber(node.column);
    endObject();
}

void ASTJsonExporter::visit(Assignment& node) {
    startObject();
    writeKey("type");
    writeString("Assignment");
    writeKey("target");
    node.target->accept(*this);
    writeKey("value");
    node.value->accept(*this);
    writeKey("line");
    writeNumber(node.line);
    writeKey("column");
    writeNumber(node.column);
    endObject();
}

void ASTJsonExporter::visit(FunctionDeclaration& node) {
    startObject();
    writeKey("type");
    writeString("FunctionDeclaration");
    writeKey("name");
    writeString(node.name);
    writeKey("parameters");
    startArray();
    for (size_t i = 0; i < node.parameters.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.parameters[i]->accept(*this);
    }
    endArray();
    if (node.returnType) {
        writeKey("returnType");
        node.returnType->accept(*this);
    }
    writeKey("body");
    node.body->accept(*this);
    writeKey("line");
    writeNumber(node.line);
    writeKey("column");
    writeNumber(node.column);
    endObject();
}

void ASTJsonExporter::visit(ClassDeclaration& node) {
    startObject();
    writeKey("type");
    writeString("ClassDeclaration");
    writeKey("name");
    writeString(node.name);
    
    writeKey("fields");
    startArray();
    for (size_t i = 0; i < node.fields.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.fields[i]->accept(*this);
    }
    endArray();
    
    if (node.constructor) {
        writeKey("constructor");
        node.constructor->accept(*this);
    }
    
    writeKey("methods");
    startArray();
    for (size_t i = 0; i < node.methods.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.methods[i]->accept(*this);
    }
    endArray();
    
    writeKey("line");
    writeNumber(node.line);
    writeKey("column");
    writeNumber(node.column);
    endObject();
}

void ASTJsonExporter::visit(IfStatement& node) {
    startObject();
    writeKey("type");
    writeString("IfStatement");
    writeKey("condition");
    node.condition->accept(*this);
    writeKey("consequence");
    node.consequence->accept(*this);
    if (node.alternative) {
        writeKey("alternative");
        node.alternative->accept(*this);
    }
    writeKey("line");
    writeNumber(node.line);
    writeKey("column");
    writeNumber(node.column);
    endObject();
}

void ASTJsonExporter::visit(WhileStatement& node) {
    startObject();
    writeKey("type");
    writeString("WhileStatement");
    writeKey("condition");
    node.condition->accept(*this);
    writeKey("body");
    node.body->accept(*this);
    writeKey("line");
    writeNumber(node.line);
    writeKey("column");
    writeNumber(node.column);
    endObject();
}

void ASTJsonExporter::visit(ForStatement& node) {
    startObject();
    writeKey("type");
    writeString("ForStatement");
    writeKey("initializer");
    node.initializer->accept(*this);
    writeKey("condition");
    node.condition->accept(*this);
    writeKey("increment");
    node.increment->accept(*this);
    writeKey("body");
    node.body->accept(*this);
    writeKey("line");
    writeNumber(node.line);
    writeKey("column");
    writeNumber(node.column);
    endObject();
}

void ASTJsonExporter::visit(TryStatement& node) {
    startObject();
    writeKey("type");
    writeString("TryStatement");
    writeKey("body");
    node.body->accept(*this);
    if (node.catchBlock) {
        writeKey("errorVar");
        writeString(node.errorVar);
        writeKey("catchBlock");
        node.catchBlock->accept(*this);
    }
    if (node.finallyBlock) {
        writeKey("finallyBlock");
        node.finallyBlock->accept(*this);
    }
    writeKey("line");
    writeNumber(node.line);
    writeKey("column");
    writeNumber(node.column);
    endObject();
}

void ASTJsonExporter::visit(ReturnStatement& node) {
    startObject();
    writeKey("type");
    writeString("ReturnStatement");
    if (node.value) {
        writeKey("value");
        node.value->accept(*this);
    }
    writeKey("line");
    writeNumber(node.line);
    writeKey("column");
    writeNumber(node.column);
    endObject();
}

void ASTJsonExporter::visit(ExpressionStatement& node) {
    startObject();
    writeKey("type");
    writeString("ExpressionStatement");
    writeKey("expression");
    node.expression->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(Block& node) {
    startObject();
    writeKey("type");
    writeString("Block");
    writeKey("statements");
    startArray();
    for (size_t i = 0; i < node.statements.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.statements[i]->accept(*this);
    }
    endArray();
    endObject();
}

void ASTJsonExporter::visit(Identifier& node) {
    startObject();
    writeKey("type");
    writeString("Identifier");
    writeKey("name");
    writeString(node.name);
    endObject();
}

void ASTJsonExporter::visit(NumberLiteral& node) {
    startObject();
    writeKey("type");
    writeString("NumberLiteral");
    writeKey("value");
    writeString(node.value);
    writeKey("isFloat");
    writeBool(node.isFloat);
    writeKey("isHex");
    writeBool(node.isHex);
    endObject();
}

void ASTJsonExporter::visit(StringLiteral& node) {
    startObject();
    writeKey("type");
    writeString("StringLiteral");
    writeKey("value");
    writeString(node.value);
    endObject();
}

void ASTJsonExporter::visit(BooleanLiteral& node) {
    startObject();
    writeKey("type");
    writeString("BooleanLiteral");
    writeKey("value");
    writeBool(node.value);
    endObject();
}

void ASTJsonExporter::visit(NullLiteral& node) {
    startObject();
    writeKey("type");
    writeString("NullLiteral");
    endObject();
}

void ASTJsonExporter::visit(ArrayLiteral& node) {
    startObject();
    writeKey("type");
    writeString("ArrayLiteral");
    writeKey("elements");
    startArray();
    for (size_t i = 0; i < node.elements.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.elements[i]->accept(*this);
    }
    endArray();
    endObject();
}

void ASTJsonExporter::visit(TemplateLiteral& node) {
    startObject();
    writeKey("type");
    writeString("TemplateLiteral");
    writeKey("parts");
    startArray();
    for (size_t i = 0; i < node.parts.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        startObject();
        writeKey("type");
        writeString(node.parts[i].type == TemplateLiteral::Part::STRING ? "string" : "expression");
        if (node.parts[i].type == TemplateLiteral::Part::STRING) {
            writeKey("value");
            writeString(node.parts[i].stringValue);
        } else {
            writeKey("expression");
            node.parts[i].expression->accept(*this);
        }
        endObject();
    }
    endArray();
    endObject();
}

void ASTJsonExporter::visit(BinaryExpression& node) {
    startObject();
    writeKey("type");
    writeString("BinaryExpression");
    writeKey("operator");
    const char* opStr = "";
    switch (node.op) {
        case BinaryExpression::Operator::ADD: opStr = "+"; break;
        case BinaryExpression::Operator::SUB: opStr = "-"; break;
        case BinaryExpression::Operator::MUL: opStr = "*"; break;
        case BinaryExpression::Operator::DIV: opStr = "/"; break;
        case BinaryExpression::Operator::MOD: opStr = "%"; break;
        case BinaryExpression::Operator::EQ: opStr = "="; break;
        case BinaryExpression::Operator::NE: opStr = "!="; break;
        case BinaryExpression::Operator::LT: opStr = "<"; break;
        case BinaryExpression::Operator::GT: opStr = ">"; break;
        case BinaryExpression::Operator::LE: opStr = "<="; break;
        case BinaryExpression::Operator::GE: opStr = ">="; break;
        case BinaryExpression::Operator::AND: opStr = "and"; break;
        case BinaryExpression::Operator::OR: opStr = "or"; break;
    }
    writeString(opStr);
    writeKey("left");
    node.left->accept(*this);
    writeKey("right");
    node.right->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(UnaryExpression& node) {
    startObject();
    writeKey("type");
    writeString("UnaryExpression");
    writeKey("operator");
    const char* opStr = "";
    switch (node.op) {
        case UnaryExpression::Operator::NOT: opStr = "not"; break;
        case UnaryExpression::Operator::NEG: opStr = "-"; break;
        case UnaryExpression::Operator::PLUS: opStr = "+"; break;
    }
    writeString(opStr);
    writeKey("operand");
    node.operand->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(TernaryExpression& node) {
    startObject();
    writeKey("type");
    writeString("TernaryExpression");
    writeKey("condition");
    node.condition->accept(*this);
    writeKey("consequence");
    node.consequence->accept(*this);
    writeKey("alternative");
    node.alternative->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(CallExpression& node) {
    startObject();
    writeKey("type");
    writeString("CallExpression");
    writeKey("callee");
    node.callee->accept(*this);
    writeKey("arguments");
    startArray();
    for (size_t i = 0; i < node.arguments.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.arguments[i]->accept(*this);
    }
    endArray();
    endObject();
}

void ASTJsonExporter::visit(MemberAccess& node) {
    startObject();
    writeKey("type");
    writeString("MemberAccess");
    writeKey("object");
    node.object->accept(*this);
    writeKey("property");
    writeString(node.property);
    endObject();
}

void ASTJsonExporter::visit(ArrayAccess& node) {
    startObject();
    writeKey("type");
    writeString("ArrayAccess");
    writeKey("array");
    node.array->accept(*this);
    writeKey("index");
    node.index->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(ConstructorCall& node) {
    startObject();
    writeKey("type");
    writeString("ConstructorCall");
    writeKey("typeName");
    writeString(node.typeName);
    writeKey("arguments");
    startArray();
    for (size_t i = 0; i < node.arguments.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.arguments[i]->accept(*this);
    }
    endArray();
    endObject();
}

void ASTJsonExporter::visit(ParenthesizedExpression& node) {
    startObject();
    writeKey("type");
    writeString("ParenthesizedExpression");
    writeKey("expression");
    node.expression->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(PrimitiveType& node) {
    startObject();
    writeKey("type");
    writeString("PrimitiveType");
    writeKey("kind");
    const char* kindStr = "";
    switch (node.kind) {
        case PrimitiveType::Kind::INT: kindStr = "int"; break;
        case PrimitiveType::Kind::UINT: kindStr = "uint"; break;
        case PrimitiveType::Kind::BYTE: kindStr = "byte"; break;
        case PrimitiveType::Kind::SHORT: kindStr = "short"; break;
        case PrimitiveType::Kind::FLOAT: kindStr = "float"; break;
        case PrimitiveType::Kind::BOOL: kindStr = "bool"; break;
        case PrimitiveType::Kind::STRING: kindStr = "string"; break;
        case PrimitiveType::Kind::VOID: kindStr = "void"; break;
        case PrimitiveType::Kind::ANY: kindStr = "any"; break;
    }
    writeString(kindStr);
    endObject();
}

void ASTJsonExporter::visit(ArrayType& node) {
    startObject();
    writeKey("type");
    writeString("ArrayType");
    writeKey("elementType");
    node.elementType->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(NullableType& node) {
    startObject();
    writeKey("type");
    writeString("NullableType");
    writeKey("baseType");
    node.baseType->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(NamedType& node) {
    startObject();
    writeKey("type");
    writeString("NamedType");
    writeKey("name");
    writeString(node.name);
    endObject();
}

void ASTJsonExporter::visit(Parameter& node) {
    startObject();
    writeKey("type");
    writeString("Parameter");
    writeKey("name");
    writeString(node.name);
    writeKey("paramType");
    node.type->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(FieldDeclaration& node) {
    startObject();
    writeKey("type");
    writeString("FieldDeclaration");
    writeKey("name");
    writeString(node.name);
    writeKey("fieldType");
    node.type->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(ConstructorDeclaration& node) {
    startObject();
    writeKey("type");
    writeString("ConstructorDeclaration");
    writeKey("parameters");
    startArray();
    for (size_t i = 0; i < node.parameters.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.parameters[i]->accept(*this);
    }
    endArray();
    writeKey("body");
    node.body->accept(*this);
    endObject();
}

void ASTJsonExporter::visit(MethodDeclaration& node) {
    startObject();
    writeKey("type");
    writeString("MethodDeclaration");
    writeKey("name");
    writeString(node.name);
    writeKey("parameters");
    startArray();
    for (size_t i = 0; i < node.parameters.size(); i++) {
        if (i > 0) json_ << ",";
        json_ << "\n";
        writeIndent();
        node.parameters[i]->accept(*this);
    }
    endArray();
    if (node.returnType) {
        writeKey("returnType");
        node.returnType->accept(*this);
    }
    writeKey("body");
    node.body->accept(*this);
    endObject();
}

} // namespace compiler
} // namespace dialos
