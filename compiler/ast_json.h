#ifndef DIALOS_COMPILER_AST_JSON_H
#define DIALOS_COMPILER_AST_JSON_H

#include "ast.h"
#include <string>
#include <sstream>

namespace dialos {
namespace compiler {

/**
 * AST to JSON converter
 * Exports the AST as a JSON document for easy consumption by other programs
 */
class ASTJsonExporter : public ASTVisitor {
public:
    std::string toJson(const Program& program);
    
private:
    std::stringstream json_;
    int indent_;
    bool needsComma_;
    
    void writeIndent();
    void writeKey(const std::string& key);
    void writeString(const std::string& value);
    void writeNumber(int value);
    void writeBool(bool value);
    void writeNull();
    void startObject();
    void endObject();
    void startArray();
    void endArray();
    
    std::string escapeJson(const std::string& str);
    
    // Visitor methods
    void visit(Program& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(Assignment& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(TryStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(Block& node) override;
    void visit(Identifier& node) override;
    void visit(NumberLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(NullLiteral& node) override;
    void visit(ArrayLiteral& node) override;
    void visit(TemplateLiteral& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(TernaryExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(MemberAccess& node) override;
    void visit(ArrayAccess& node) override;
    void visit(ConstructorCall& node) override;
    void visit(ParenthesizedExpression& node) override;
    void visit(PrimitiveType& node) override;
    void visit(ArrayType& node) override;
    void visit(NullableType& node) override;
    void visit(NamedType& node) override;
    void visit(Parameter& node) override;
    void visit(FieldDeclaration& node) override;
    void visit(ConstructorDeclaration& node) override;
    void visit(MethodDeclaration& node) override;
};

} // namespace compiler
} // namespace dialos

#endif // DIALOS_COMPILER_AST_JSON_H
