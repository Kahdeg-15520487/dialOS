#include "ast.h"

namespace dialos {
namespace compiler {

// Expression nodes
void NumberLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void StringLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BooleanLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void NullLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Identifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BinaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void UnaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TernaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CallExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void MemberAccess::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ArrayAccess::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ArrayLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ConstructorCall::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TemplateLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ParenthesizedExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

// Type nodes
void PrimitiveType::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void NamedType::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ArrayType::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void NullableType::accept(ASTVisitor& visitor) { visitor.visit(*this); }

// Statement nodes
void VariableDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Assignment::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Block::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IfStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void WhileStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ForStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ReturnStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ExpressionStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TryStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

// Declaration nodes
void Parameter::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FunctionDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FieldDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ConstructorDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void MethodDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ClassDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Program::accept(ASTVisitor& visitor) { visitor.visit(*this); }

} // namespace compiler
} // namespace dialos
