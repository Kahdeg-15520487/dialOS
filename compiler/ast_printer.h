#ifndef DIALOS_AST_PRINTER_H
#define DIALOS_AST_PRINTER_H

#include "ast.h"
#include <string>

namespace dialos {
namespace compiler {

// Simple AST printer for debugging
class ASTPrinter : public ASTVisitor {
public:
    ASTPrinter() : indent_(0) {}
    
    std::string print(ASTNode& node);
    
    // Expressions
    void visit(NumberLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(NullLiteral& node) override;
    void visit(Identifier& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(TernaryExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(MemberAccess& node) override;
    void visit(ArrayAccess& node) override;
    void visit(ArrayLiteral& node) override;
    void visit(ConstructorCall& node) override;
    void visit(TemplateLiteral& node) override;
    void visit(ParenthesizedExpression& node) override;
    
    // Types
    void visit(PrimitiveType& node) override;
    void visit(NamedType& node) override;
    void visit(ArrayType& node) override;
    void visit(NullableType& node) override;
    
    // Statements
    void visit(VariableDeclaration& node) override;
    void visit(Assignment& node) override;
    void visit(Block& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(TryStatement& node) override;
    
    // Declarations
    void visit(Parameter& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(FieldDeclaration& node) override;
    void visit(ConstructorDeclaration& node) override;
    void visit(MethodDeclaration& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(Program& node) override;
    
private:
    std::string output_;
    int indent_;
    
    void printIndent();
    void increaseIndent() { indent_ += 2; }
    void decreaseIndent() { indent_ -= 2; }
    void println(const std::string& str);
    void print(const std::string& str);
    
    std::string binOpToString(BinaryExpression::Operator op);
    std::string unOpToString(UnaryExpression::Operator op);
    std::string typeKindToString(PrimitiveType::Kind kind);
};

} // namespace compiler
} // namespace dialos

#endif // DIALOS_AST_PRINTER_H
