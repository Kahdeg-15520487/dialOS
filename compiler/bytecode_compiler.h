/**
 * Bytecode Compiler - Generates bytecode from AST
 */

#ifndef DIALOS_COMPILER_BYTECODE_COMPILER_H
#define DIALOS_COMPILER_BYTECODE_COMPILER_H

#include "ast.h"
#include "bytecode.h"
#include <map>
#include <vector>
#include <string>

namespace dialos {
namespace compiler {

class BytecodeCompiler {
public:
    BytecodeCompiler() : module_(), localCount_(0) {}
    
    // Compile AST to bytecode
    BytecodeModule compile(const Program& program);
    
    // Get error messages
    const std::vector<std::string>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }
    
private:
    BytecodeModule module_;
    std::vector<std::string> errors_;
    
    // Symbol tables
    std::map<std::string, uint8_t> locals_;  // Local variable indices
    uint8_t localCount_;
    
    // Jump patching
    struct JumpPatch {
        size_t position;
        std::string label;
    };
    std::vector<JumpPatch> jumpPatches_;
    std::map<std::string, size_t> labels_;
    
    // Compile statements
    void compileStatement(const Statement& stmt);
    void compileVariableDecl(const VariableDeclaration& decl);
    void compileAssignment(const Assignment& assign);
    void compileFunctionDecl(const FunctionDeclaration& func);
    void compileClassDecl(const ClassDeclaration& cls);
    void compileIfStatement(const IfStatement& ifStmt);
    void compileWhileStatement(const WhileStatement& whileStmt);
    void compileForStatement(const ForStatement& forStmt);
    void compileTryStatement(const TryStatement& tryStmt);
    void compileReturnStatement(const ReturnStatement& ret);
    void compileBlock(const Block& block);
    void compileExpressionStatement(const ExpressionStatement& stmt);
    
    // Compile expressions
    void compileExpression(const Expression& expr);
    void compileBinaryExpression(const BinaryExpression& expr);
    void compileUnaryExpression(const UnaryExpression& expr);
    void compileTernaryExpression(const TernaryExpression& expr);
    void compileCallExpression(const CallExpression& expr);
    void compileMemberAccess(const MemberAccess& expr);
    void compileArrayAccess(const ArrayAccess& expr);
    void compileConstructorCall(const ConstructorCall& expr);
    void compileIdentifier(const Identifier& expr);
    void compileNumberLiteral(const NumberLiteral& expr);
    void compileStringLiteral(const StringLiteral& expr);
    void compileBooleanLiteral(const BooleanLiteral& expr);
    void compileNullLiteral(const NullLiteral& expr);
    void compileArrayLiteral(const ArrayLiteral& expr);
    void compileTemplateLiteral(const TemplateLiteral& expr);
    
    // Helpers
    void error(const std::string& message);
    uint8_t allocateLocal(const std::string& name);
    void emitJump(Opcode jumpOp, const std::string& label);
    void placeLabel(const std::string& label);
    void patchJumps();
};

} // namespace compiler
} // namespace dialos

#endif // DIALOS_COMPILER_BYTECODE_COMPILER_H
