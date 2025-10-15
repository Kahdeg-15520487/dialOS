#ifndef DIALOS_PARSER_H
#define DIALOS_PARSER_H

#include "lexer.h"
#include "ast.h"
#include <vector>
#include <memory>

namespace dialos {
namespace compiler {

class Parser {
public:
    Parser(Lexer& lexer) : lexer_(lexer) {
        advance();
    }
    
    std::unique_ptr<Program> parse();
    
    const std::vector<std::string>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }
    
private:
    Lexer& lexer_;
    Token current_;
    std::vector<std::string> errors_;
    
    // Helper methods
    void advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    Token consume(TokenType type, const std::string& message);
    void error(const std::string& message);
    void synchronize();
    
    // Parsing methods - Statements
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<VariableDeclaration> parseVariableDeclaration();
    std::unique_ptr<Assignment> parseAssignment();
    std::unique_ptr<FunctionDeclaration> parseFunctionDeclaration();
    std::unique_ptr<ClassDeclaration> parseClassDeclaration();
    std::unique_ptr<IfStatement> parseIfStatement();
    std::unique_ptr<WhileStatement> parseWhileStatement();
    std::unique_ptr<ForStatement> parseForStatement();
    std::unique_ptr<TryStatement> parseTryStatement();
    std::unique_ptr<ReturnStatement> parseReturnStatement();
    std::unique_ptr<Block> parseBlock();
    
    // Parsing methods - Expressions (by precedence)
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseTernary();
    std::unique_ptr<Expression> parseLogicalOr();
    std::unique_ptr<Expression> parseLogicalAnd();
    std::unique_ptr<Expression> parseEquality();
    std::unique_ptr<Expression> parseComparison();
    std::unique_ptr<Expression> parseAdditive();
    std::unique_ptr<Expression> parseMultiplicative();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parsePostfix();
    std::unique_ptr<Expression> parsePrimary();
    
    // Special expression parsing
    std::unique_ptr<Expression> parseCallArguments(std::unique_ptr<Expression> callee);
    std::unique_ptr<Expression> parseArrayLiteral();
    std::unique_ptr<Expression> parseTemplateLiteral();
    
    // Type parsing
    std::unique_ptr<TypeNode> parseType();
    std::unique_ptr<Parameter> parseParameter();
    
    // Class member parsing
    std::unique_ptr<FieldDeclaration> parseFieldDeclaration(const Token& nameToken);
    std::unique_ptr<ConstructorDeclaration> parseConstructorDeclaration();
    std::unique_ptr<MethodDeclaration> parseMethodDeclaration(const Token& nameToken);
    
    // Helpers for checking statement/expression starts
    bool isStatementStart() const;
    bool isTypeKeyword() const;
    
    // Color constant helpers
    uint32_t getColorConstant(const std::string& name) const;
    std::string toHexString(uint32_t value) const;
};

} // namespace compiler
} // namespace dialos

#endif // DIALOS_PARSER_H
