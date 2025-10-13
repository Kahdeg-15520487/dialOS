#ifndef DIALOS_AST_H
#define DIALOS_AST_H

#include <string>
#include <vector>
#include <memory>

namespace dialos {
namespace compiler {

// Forward declarations
class ASTVisitor;

// Base AST node
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
    
    int line = 0;
    int column = 0;
};

// Expression nodes
class Expression : public ASTNode {};

class NumberLiteral : public Expression {
public:
    std::string value;
    bool isFloat = false;
    bool isHex = false;
    void accept(ASTVisitor& visitor) override;
};

class StringLiteral : public Expression {
public:
    std::string value;
    void accept(ASTVisitor& visitor) override;
};

class BooleanLiteral : public Expression {
public:
    bool value;
    void accept(ASTVisitor& visitor) override;
};

class NullLiteral : public Expression {
public:
    void accept(ASTVisitor& visitor) override;
};

class Identifier : public Expression {
public:
    std::string name;
    void accept(ASTVisitor& visitor) override;
};

class BinaryExpression : public Expression {
public:
    enum class Operator {
        ADD, SUB, MUL, DIV, MOD,
        EQ, NE, LT, GT, LE, GE,
        AND, OR
    };
    
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    Operator op;
    void accept(ASTVisitor& visitor) override;
};

class UnaryExpression : public Expression {
public:
    enum class Operator { NEG, NOT, PLUS };
    
    std::unique_ptr<Expression> operand;
    Operator op;
    void accept(ASTVisitor& visitor) override;
};

class TernaryExpression : public Expression {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> consequence;
    std::unique_ptr<Expression> alternative;
    void accept(ASTVisitor& visitor) override;
};

class CallExpression : public Expression {
public:
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;
    void accept(ASTVisitor& visitor) override;
};

class MemberAccess : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::string property;
    void accept(ASTVisitor& visitor) override;
};

class ArrayAccess : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;
    void accept(ASTVisitor& visitor) override;
};

class ArrayLiteral : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;
    void accept(ASTVisitor& visitor) override;
};

class ConstructorCall : public Expression {
public:
    std::string typeName;
    std::vector<std::unique_ptr<Expression>> arguments;
    void accept(ASTVisitor& visitor) override;
};

class TemplateLiteral : public Expression {
public:
    struct Part {
        enum Type { STRING, EXPRESSION };
        Type type;
        std::string stringValue;
        std::unique_ptr<Expression> expression;
    };
    std::vector<Part> parts;
    void accept(ASTVisitor& visitor) override;
};

class ParenthesizedExpression : public Expression {
public:
    std::unique_ptr<Expression> expression;
    void accept(ASTVisitor& visitor) override;
};

// Type nodes
class TypeNode : public ASTNode {
public:
    virtual ~TypeNode() = default;
};

class PrimitiveType : public TypeNode {
public:
    enum class Kind { INT, UINT, BYTE, SHORT, FLOAT, BOOL, STRING, VOID, ANY };
    Kind kind;
    void accept(ASTVisitor& visitor) override;
};

class NamedType : public TypeNode {
public:
    std::string name;
    void accept(ASTVisitor& visitor) override;
};

class ArrayType : public TypeNode {
public:
    std::unique_ptr<TypeNode> elementType;
    void accept(ASTVisitor& visitor) override;
};

class NullableType : public TypeNode {
public:
    std::unique_ptr<TypeNode> baseType;
    void accept(ASTVisitor& visitor) override;
};

// Statement nodes
class Statement : public ASTNode {};

class VariableDeclaration : public Statement {
public:
    std::string name;
    std::unique_ptr<Expression> initializer;
    void accept(ASTVisitor& visitor) override;
};

class Assignment : public Statement {
public:
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> value;
    void accept(ASTVisitor& visitor) override;
};

class Block : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    void accept(ASTVisitor& visitor) override;
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> consequence;
    std::unique_ptr<Statement> alternative; // Can be Block or another IfStatement
    void accept(ASTVisitor& visitor) override;
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> body;
    void accept(ASTVisitor& visitor) override;
};

class ForStatement : public Statement {
public:
    std::unique_ptr<VariableDeclaration> initializer;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Assignment> increment;
    std::unique_ptr<Block> body;
    void accept(ASTVisitor& visitor) override;
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> value; // nullptr for void return
    void accept(ASTVisitor& visitor) override;
};

class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    void accept(ASTVisitor& visitor) override;
};

class TryStatement : public Statement {
public:
    std::unique_ptr<Block> body;
    std::string errorVar; // empty if no catch
    std::unique_ptr<Block> catchBlock;
    std::unique_ptr<Block> finallyBlock;
    void accept(ASTVisitor& visitor) override;
};

// Function/Method declarations
class Parameter : public ASTNode {
public:
    std::string name;
    std::unique_ptr<TypeNode> type;
    void accept(ASTVisitor& visitor) override;
};

class FunctionDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unique_ptr<TypeNode> returnType; // nullptr for inferred/void
    std::unique_ptr<Block> body;
    void accept(ASTVisitor& visitor) override;
};

// Class declarations
class FieldDeclaration : public ASTNode {
public:
    std::string name;
    std::unique_ptr<TypeNode> type;
    void accept(ASTVisitor& visitor) override;
};

class ConstructorDeclaration : public ASTNode {
public:
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unique_ptr<Block> body;
    void accept(ASTVisitor& visitor) override;
};

class MethodDeclaration : public ASTNode {
public:
    std::string name;
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unique_ptr<TypeNode> returnType;
    std::unique_ptr<Block> body;
    void accept(ASTVisitor& visitor) override;
};

class ClassDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::unique_ptr<FieldDeclaration>> fields;
    std::unique_ptr<ConstructorDeclaration> constructor;
    std::vector<std::unique_ptr<MethodDeclaration>> methods;
    void accept(ASTVisitor& visitor) override;
};

// Root node
class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    void accept(ASTVisitor& visitor) override;
};

// Visitor pattern for traversing the AST
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Expressions
    virtual void visit(NumberLiteral& node) = 0;
    virtual void visit(StringLiteral& node) = 0;
    virtual void visit(BooleanLiteral& node) = 0;
    virtual void visit(NullLiteral& node) = 0;
    virtual void visit(Identifier& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(TernaryExpression& node) = 0;
    virtual void visit(CallExpression& node) = 0;
    virtual void visit(MemberAccess& node) = 0;
    virtual void visit(ArrayAccess& node) = 0;
    virtual void visit(ArrayLiteral& node) = 0;
    virtual void visit(ConstructorCall& node) = 0;
    virtual void visit(TemplateLiteral& node) = 0;
    virtual void visit(ParenthesizedExpression& node) = 0;
    
    // Types
    virtual void visit(PrimitiveType& node) = 0;
    virtual void visit(NamedType& node) = 0;
    virtual void visit(ArrayType& node) = 0;
    virtual void visit(NullableType& node) = 0;
    
    // Statements
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(Assignment& node) = 0;
    virtual void visit(Block& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(TryStatement& node) = 0;
    
    // Declarations
    virtual void visit(Parameter& node) = 0;
    virtual void visit(FunctionDeclaration& node) = 0;
    virtual void visit(FieldDeclaration& node) = 0;
    virtual void visit(ConstructorDeclaration& node) = 0;
    virtual void visit(MethodDeclaration& node) = 0;
    virtual void visit(ClassDeclaration& node) = 0;
    virtual void visit(Program& node) = 0;
};

} // namespace compiler
} // namespace dialos

#endif // DIALOS_AST_H
