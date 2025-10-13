#include "parser.h"

namespace dialos {
namespace compiler {

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    
    while (!check(TokenType::END_OF_FILE)) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                program->statements.push_back(std::move(stmt));
            }
        } catch (...) {
            synchronize();
        }
    }
    
    return program;
}

void Parser::advance() {
    current_ = lexer_.nextToken();
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        Token token = current_;
        advance();
        return token;
    }
    
    error(message);
    return current_;
}

void Parser::error(const std::string& message) {
    std::string msg = "Line " + std::to_string(current_.line) + ":" + std::to_string(current_.column) + " - " + message;
    errors_.push_back(msg);
}

void Parser::synchronize() {
    advance();
    
    while (!check(TokenType::END_OF_FILE)) {
        if (current_.type == TokenType::SEMICOLON) {
            advance();
            return;
        }
        
        if (isStatementStart()) {
            return;
        }
        
        advance();
    }
}

bool Parser::isStatementStart() const {
    switch (current_.type) {
        case TokenType::VAR:
        case TokenType::ASSIGN:
        case TokenType::FUNCTION:
        case TokenType::CLASS:
        case TokenType::IF:
        case TokenType::WHILE:
        case TokenType::FOR:
        case TokenType::TRY:
        case TokenType::RETURN:
        case TokenType::LBRACE:
            return true;
        default:
            return false;
    }
}

bool Parser::isTypeKeyword() const {
    switch (current_.type) {
        case TokenType::INT:
        case TokenType::UINT:
        case TokenType::BYTE:
        case TokenType::SHORT:
        case TokenType::FLOAT:
        case TokenType::BOOL:
        case TokenType::STRING_TYPE:
        case TokenType::VOID:
        case TokenType::ANY:
            return true;
        default:
            return false;
    }
}

// Statement parsing
std::unique_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::VAR)) return parseVariableDeclaration();
    if (match(TokenType::ASSIGN)) return parseAssignment();
    if (match(TokenType::FUNCTION)) return parseFunctionDeclaration();
    if (match(TokenType::CLASS)) return parseClassDeclaration();
    if (match(TokenType::IF)) return parseIfStatement();
    if (match(TokenType::WHILE)) return parseWhileStatement();
    if (match(TokenType::FOR)) return parseForStatement();
    if (match(TokenType::TRY)) return parseTryStatement();
    if (match(TokenType::RETURN)) return parseReturnStatement();
    if (check(TokenType::LBRACE)) return parseBlock();
    
    // Expression statement
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression");
    
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->expression = std::move(expr);
    return stmt;
}

std::unique_ptr<VariableDeclaration> Parser::parseVariableDeclaration() {
    auto decl = std::make_unique<VariableDeclaration>();
    
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    decl->name = name.value;
    decl->line = name.line;
    decl->column = name.column;
    
    consume(TokenType::COLON, "Expected ':' after variable name");
    
    decl->initializer = parseExpression();
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    return decl;
}

std::unique_ptr<Assignment> Parser::parseAssignment() {
    auto assign = std::make_unique<Assignment>();
    assign->line = current_.line;
    assign->column = current_.column;
    
    assign->target = parseExpression();
    assign->value = parseExpression();
    
    consume(TokenType::SEMICOLON, "Expected ';' after assignment");
    
    return assign;
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration() {
    auto func = std::make_unique<FunctionDeclaration>();
    
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    func->name = name.value;
    func->line = name.line;
    func->column = name.column;
    
    // Parameters
    consume(TokenType::LPAREN, "Expected '(' after function name");
    
    if (!check(TokenType::RPAREN)) {
        do {
            func->parameters.push_back(parseParameter());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    
    // Return type (optional)
    if (match(TokenType::COLON)) {
        func->returnType = parseType();
    }
    
    // Body
    func->body = parseBlock();
    
    return func;
}

std::unique_ptr<ClassDeclaration> Parser::parseClassDeclaration() {
    auto cls = std::make_unique<ClassDeclaration>();
    
    Token name = consume(TokenType::IDENTIFIER, "Expected class name");
    cls->name = name.value;
    cls->line = name.line;
    cls->column = name.column;
    
    consume(TokenType::LBRACE, "Expected '{' after class name");
    
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        if (match(TokenType::CONSTRUCTOR)) {
            cls->constructor = parseConstructorDeclaration();
        } else if (check(TokenType::IDENTIFIER)) {
            // Look ahead to determine if it's a field or method
            Token nameToken = current_;
            advance(); // Move past identifier
            
            if (check(TokenType::LPAREN)) {
                // It's a method: name(...) or name(...): type
                cls->methods.push_back(parseMethodDeclaration(nameToken));
            } else if (check(TokenType::COLON)) {
                // It's a field: name: type;
                cls->fields.push_back(parseFieldDeclaration(nameToken));
            } else {
                error("Expected '(' or ':' after identifier in class body");
                synchronize();
            }
        } else {
            error("Expected field, method, or constructor declaration");
            synchronize();
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' after class body");
    
    return cls;
}

std::unique_ptr<IfStatement> Parser::parseIfStatement() {
    auto ifStmt = std::make_unique<IfStatement>();
    ifStmt->line = current_.line;
    ifStmt->column = current_.column;
    
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    ifStmt->condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after if condition");
    
    ifStmt->consequence = parseBlock();
    
    if (match(TokenType::ELSE)) {
        if (check(TokenType::IF)) {
            advance();
            ifStmt->alternative = parseIfStatement();
        } else {
            ifStmt->alternative = parseBlock();
        }
    }
    
    return ifStmt;
}

std::unique_ptr<WhileStatement> Parser::parseWhileStatement() {
    auto whileStmt = std::make_unique<WhileStatement>();
    whileStmt->line = current_.line;
    whileStmt->column = current_.column;
    
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    whileStmt->condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after while condition");
    
    whileStmt->body = parseBlock();
    
    return whileStmt;
}

std::unique_ptr<ForStatement> Parser::parseForStatement() {
    auto forStmt = std::make_unique<ForStatement>();
    forStmt->line = current_.line;
    forStmt->column = current_.column;
    
    consume(TokenType::LPAREN, "Expected '(' after 'for'");
    
    // Initializer (must be var declaration)
    consume(TokenType::VAR, "Expected 'var' in for initializer");
    forStmt->initializer = parseVariableDeclaration();
    
    // Condition
    forStmt->condition = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after for condition");
    
    // Increment (must be assignment)
    consume(TokenType::ASSIGN, "Expected 'assign' in for increment");
    forStmt->increment = parseAssignment();
    
    consume(TokenType::RPAREN, "Expected ')' after for clauses");
    
    forStmt->body = parseBlock();
    
    return forStmt;
}

std::unique_ptr<TryStatement> Parser::parseTryStatement() {
    auto tryStmt = std::make_unique<TryStatement>();
    tryStmt->line = current_.line;
    tryStmt->column = current_.column;
    
    tryStmt->body = parseBlock();
    
    if (match(TokenType::CATCH)) {
        consume(TokenType::LPAREN, "Expected '(' after 'catch'");
        Token errorVar = consume(TokenType::IDENTIFIER, "Expected error variable name");
        tryStmt->errorVar = errorVar.value;
        consume(TokenType::RPAREN, "Expected ')' after error variable");
        
        tryStmt->catchBlock = parseBlock();
    }
    
    if (match(TokenType::FINALLY)) {
        tryStmt->finallyBlock = parseBlock();
    }
    
    return tryStmt;
}

std::unique_ptr<ReturnStatement> Parser::parseReturnStatement() {
    auto ret = std::make_unique<ReturnStatement>();
    ret->line = current_.line;
    ret->column = current_.column;
    
    if (!check(TokenType::SEMICOLON)) {
        ret->value = parseExpression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return statement");
    
    return ret;
}

std::unique_ptr<Block> Parser::parseBlock() {
    auto block = std::make_unique<Block>();
    block->line = current_.line;
    block->column = current_.column;
    
    consume(TokenType::LBRACE, "Expected '{'");
    
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        block->statements.push_back(parseStatement());
    }
    
    consume(TokenType::RBRACE, "Expected '}'");
    
    return block;
}

// Expression parsing (precedence climbing)
std::unique_ptr<Expression> Parser::parseExpression() {
    return parseTernary();
}

std::unique_ptr<Expression> Parser::parseTernary() {
    auto expr = parseLogicalOr();
    
    if (match(TokenType::QUESTION)) {
        auto ternary = std::make_unique<TernaryExpression>();
        ternary->condition = std::move(expr);
        ternary->consequence = parseExpression();
        consume(TokenType::COLON, "Expected ':' in ternary expression");
        ternary->alternative = parseExpression();
        return ternary;
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    
    while (match(TokenType::OR)) {
        auto binary = std::make_unique<BinaryExpression>();
        binary->op = BinaryExpression::Operator::OR;
        binary->left = std::move(left);
        binary->right = parseLogicalAnd();
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd() {
    auto left = parseEquality();
    
    while (match(TokenType::AND)) {
        auto binary = std::make_unique<BinaryExpression>();
        binary->op = BinaryExpression::Operator::AND;
        binary->left = std::move(left);
        binary->right = parseEquality();
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseEquality() {
    auto left = parseComparison();
    
    while (true) {
        BinaryExpression::Operator op;
        
        if (match(TokenType::EQUAL)) {
            op = BinaryExpression::Operator::EQ;
        } else if (match(TokenType::NOT_EQUAL)) {
            op = BinaryExpression::Operator::NE;
        } else {
            break;
        }
        
        auto binary = std::make_unique<BinaryExpression>();
        binary->op = op;
        binary->left = std::move(left);
        binary->right = parseComparison();
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseComparison() {
    auto left = parseAdditive();
    
    while (true) {
        BinaryExpression::Operator op;
        
        if (match(TokenType::LESS)) {
            op = BinaryExpression::Operator::LT;
        } else if (match(TokenType::GREATER)) {
            op = BinaryExpression::Operator::GT;
        } else if (match(TokenType::LESS_EQUAL)) {
            op = BinaryExpression::Operator::LE;
        } else if (match(TokenType::GREATER_EQUAL)) {
            op = BinaryExpression::Operator::GE;
        } else {
            break;
        }
        
        auto binary = std::make_unique<BinaryExpression>();
        binary->op = op;
        binary->left = std::move(left);
        binary->right = parseAdditive();
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    
    while (true) {
        BinaryExpression::Operator op;
        
        if (match(TokenType::PLUS)) {
            op = BinaryExpression::Operator::ADD;
        } else if (match(TokenType::MINUS)) {
            op = BinaryExpression::Operator::SUB;
        } else {
            break;
        }
        
        auto binary = std::make_unique<BinaryExpression>();
        binary->op = op;
        binary->left = std::move(left);
        binary->right = parseMultiplicative();
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseMultiplicative() {
    auto left = parseUnary();
    
    while (true) {
        BinaryExpression::Operator op;
        
        if (match(TokenType::STAR)) {
            op = BinaryExpression::Operator::MUL;
        } else if (match(TokenType::SLASH)) {
            op = BinaryExpression::Operator::DIV;
        } else if (match(TokenType::PERCENT)) {
            op = BinaryExpression::Operator::MOD;
        } else {
            break;
        }
        
        auto binary = std::make_unique<BinaryExpression>();
        binary->op = op;
        binary->left = std::move(left);
        binary->right = parseUnary();
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (match(TokenType::NOT)) {
        auto unary = std::make_unique<UnaryExpression>();
        unary->op = UnaryExpression::Operator::NOT;
        unary->operand = parseUnary();
        return unary;
    }
    
    if (match(TokenType::MINUS)) {
        auto unary = std::make_unique<UnaryExpression>();
        unary->op = UnaryExpression::Operator::NEG;
        unary->operand = parseUnary();
        return unary;
    }
    
    if (match(TokenType::PLUS)) {
        auto unary = std::make_unique<UnaryExpression>();
        unary->op = UnaryExpression::Operator::PLUS;
        unary->operand = parseUnary();
        return unary;
    }
    
    return parsePostfix();
}

std::unique_ptr<Expression> Parser::parsePostfix() {
    auto expr = parsePrimary();
    
    while (true) {
        if (match(TokenType::LPAREN)) {
            // Function call
            std::vector<std::unique_ptr<Expression>> args;
            
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            
            consume(TokenType::RPAREN, "Expected ')' after arguments");
            
            auto call = std::make_unique<CallExpression>();
            call->callee = std::move(expr);
            call->arguments = std::move(args);
            expr = std::move(call);
            
        } else if (match(TokenType::DOT)) {
            // Member access
            Token member = consume(TokenType::IDENTIFIER, "Expected property name after '.'");
            
            auto access = std::make_unique<MemberAccess>();
            access->object = std::move(expr);
            access->property = member.value;
            expr = std::move(access);
            
        } else if (match(TokenType::LBRACKET)) {
            // Array access
            auto index = parseExpression();
            consume(TokenType::RBRACKET, "Expected ']' after array index");
            
            auto access = std::make_unique<ArrayAccess>();
            access->array = std::move(expr);
            access->index = std::move(index);
            expr = std::move(access);
            
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    // Literals
    if (check(TokenType::NUMBER)) {
        auto lit = std::make_unique<NumberLiteral>();
        lit->value = current_.value;
        lit->isFloat = current_.value.find('.') != std::string::npos;
        lit->isHex = current_.value.find("0x") == 0 || current_.value.find("0X") == 0;
        lit->line = current_.line;
        lit->column = current_.column;
        advance();
        return lit;
    }
    
    if (check(TokenType::STRING)) {
        auto lit = std::make_unique<StringLiteral>();
        lit->value = current_.value;
        lit->line = current_.line;
        lit->column = current_.column;
        advance();
        return lit;
    }
    
    if (match(TokenType::TRUE) || match(TokenType::FALSE)) {
        auto lit = std::make_unique<BooleanLiteral>();
        lit->value = (current_.type == TokenType::TRUE);
        lit->line = current_.line;
        lit->column = current_.column;
        return lit;
    }
    
    if (match(TokenType::NULL_LIT)) {
        auto lit = std::make_unique<NullLiteral>();
        lit->line = current_.line;
        lit->column = current_.column;
        return lit;
    }
    
    // Template literal
    if (check(TokenType::BACKTICK)) {
        return parseTemplateLiteral();
    }
    
    // Array literal
    if (check(TokenType::LBRACKET)) {
        return parseArrayLiteral();
    }
    
    // Parenthesized expression
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        
        auto paren = std::make_unique<ParenthesizedExpression>();
        paren->expression = std::move(expr);
        return paren;
    }
    
    // Type constructor: Type(args)
    if (isTypeKeyword()) {
        Token typeToken = current_;
        advance();
        
        if (check(TokenType::LPAREN)) {
            advance();
            
            auto ctor = std::make_unique<ConstructorCall>();
            ctor->typeName = typeToken.value;
            ctor->line = typeToken.line;
            ctor->column = typeToken.column;
            
            if (!check(TokenType::RPAREN)) {
                do {
                    ctor->arguments.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            
            consume(TokenType::RPAREN, "Expected ')' after constructor arguments");
            return ctor;
        } else {
            // It's just a type reference, treat as identifier
            auto id = std::make_unique<Identifier>();
            id->name = typeToken.value;
            id->line = typeToken.line;
            id->column = typeToken.column;
            return id;
        }
    }
    
    // Identifier or constructor call
    if (check(TokenType::IDENTIFIER)) {
        Token id = current_;
        advance();
        
        // Check if it's a constructor call
        if (check(TokenType::LPAREN)) {
            Token peek = lexer_.peekToken();
            // Look ahead to determine if this is a constructor or function call
            // For now, assume uppercase identifiers are types
            if (id.value.length() > 0 && id.value[0] >= 'A' && id.value[0] <= 'Z') {
                advance(); // consume (
                
                auto ctor = std::make_unique<ConstructorCall>();
                ctor->typeName = id.value;
                ctor->line = id.line;
                ctor->column = id.column;
                
                if (!check(TokenType::RPAREN)) {
                    do {
                        ctor->arguments.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }
                
                consume(TokenType::RPAREN, "Expected ')' after constructor arguments");
                return ctor;
            }
        }
        
        // Regular identifier
        auto ident = std::make_unique<Identifier>();
        ident->name = id.value;
        ident->line = id.line;
        ident->column = id.column;
        return ident;
    }
    
    error("Expected expression");
    return std::make_unique<Identifier>(); // Return dummy node
}

std::unique_ptr<Expression> Parser::parseArrayLiteral() {
    auto array = std::make_unique<ArrayLiteral>();
    array->line = current_.line;
    array->column = current_.column;
    
    consume(TokenType::LBRACKET, "Expected '['");
    
    if (!check(TokenType::RBRACKET)) {
        do {
            array->elements.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RBRACKET, "Expected ']'");
    
    return array;
}

std::unique_ptr<Expression> Parser::parseTemplateLiteral() {
    auto tmpl = std::make_unique<TemplateLiteral>();
    tmpl->line = current_.line;
    tmpl->column = current_.column;
    
    consume(TokenType::BACKTICK, "Expected '`'");
    
    // Simple implementation: alternate between strings and expressions
    while (!check(TokenType::BACKTICK) && !check(TokenType::END_OF_FILE)) {
        if (match(TokenType::TEMPLATE_START)) {
            TemplateLiteral::Part part;
            part.type = TemplateLiteral::Part::EXPRESSION;
            part.expression = parseExpression();
            consume(TokenType::RBRACE, "Expected '}' after template expression");
            tmpl->parts.push_back(std::move(part));
        } else {
            // String part - collect until ${ or `
            std::string str = "";
            while (!check(TokenType::TEMPLATE_START) && !check(TokenType::BACKTICK) && !check(TokenType::END_OF_FILE)) {
                str += current_.value;
                advance();
            }
            if (str.length() > 0) {
                TemplateLiteral::Part part;
                part.type = TemplateLiteral::Part::STRING;
                part.stringValue = str;
                tmpl->parts.push_back(std::move(part));
            }
        }
    }
    
    consume(TokenType::BACKTICK, "Expected '`' to close template literal");
    
    return tmpl;
}

// Type parsing
std::unique_ptr<TypeNode> Parser::parseType() {
    std::unique_ptr<TypeNode> type;
    
    if (isTypeKeyword()) {
        auto prim = std::make_unique<PrimitiveType>();
        
        switch (current_.type) {
            case TokenType::INT: prim->kind = PrimitiveType::Kind::INT; break;
            case TokenType::UINT: prim->kind = PrimitiveType::Kind::UINT; break;
            case TokenType::BYTE: prim->kind = PrimitiveType::Kind::BYTE; break;
            case TokenType::SHORT: prim->kind = PrimitiveType::Kind::SHORT; break;
            case TokenType::FLOAT: prim->kind = PrimitiveType::Kind::FLOAT; break;
            case TokenType::BOOL: prim->kind = PrimitiveType::Kind::BOOL; break;
            case TokenType::STRING_TYPE: prim->kind = PrimitiveType::Kind::STRING; break;
            case TokenType::VOID: prim->kind = PrimitiveType::Kind::VOID; break;
            case TokenType::ANY: prim->kind = PrimitiveType::Kind::ANY; break;
            default: break;
        }
        
        advance();
        type = std::move(prim);
    } else if (check(TokenType::IDENTIFIER)) {
        auto named = std::make_unique<NamedType>();
        named->name = current_.value;
        advance();
        type = std::move(named);
    } else {
        error("Expected type");
        return std::make_unique<PrimitiveType>();
    }
    
    // Array type?
    if (match(TokenType::LBRACKET)) {
        consume(TokenType::RBRACKET, "Expected ']' for array type");
        auto arrayType = std::make_unique<ArrayType>();
        arrayType->elementType = std::move(type);
        type = std::move(arrayType);
    }
    
    // Nullable type?
    if (match(TokenType::QUESTION)) {
        auto nullable = std::make_unique<NullableType>();
        nullable->baseType = std::move(type);
        type = std::move(nullable);
    }
    
    return type;
}

std::unique_ptr<Parameter> Parser::parseParameter() {
    auto param = std::make_unique<Parameter>();
    
    Token name = consume(TokenType::IDENTIFIER, "Expected parameter name");
    param->name = name.value;
    param->line = name.line;
    param->column = name.column;
    
    consume(TokenType::COLON, "Expected ':' after parameter name");
    
    param->type = parseType();
    
    return param;
}

// Class member parsing
std::unique_ptr<FieldDeclaration> Parser::parseFieldDeclaration(const Token& nameToken) {
    auto field = std::make_unique<FieldDeclaration>();
    
    field->name = nameToken.value;
    field->line = nameToken.line;
    field->column = nameToken.column;
    
    consume(TokenType::COLON, "Expected ':' after field name");
    
    field->type = parseType();
    
    consume(TokenType::SEMICOLON, "Expected ';' after field declaration");
    
    return field;
}

std::unique_ptr<ConstructorDeclaration> Parser::parseConstructorDeclaration() {
    auto ctor = std::make_unique<ConstructorDeclaration>();
    ctor->line = current_.line;
    ctor->column = current_.column;
    
    consume(TokenType::LPAREN, "Expected '(' after 'constructor'");
    
    if (!check(TokenType::RPAREN)) {
        do {
            ctor->parameters.push_back(parseParameter());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    
    ctor->body = parseBlock();
    
    return ctor;
}

std::unique_ptr<MethodDeclaration> Parser::parseMethodDeclaration(const Token& nameToken) {
    auto method = std::make_unique<MethodDeclaration>();
    
    method->name = nameToken.value;
    method->line = nameToken.line;
    method->column = nameToken.column;
    
    consume(TokenType::LPAREN, "Expected '(' after method name");
    
    if (!check(TokenType::RPAREN)) {
        do {
            method->parameters.push_back(parseParameter());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    
    // Return type (optional)
    if (match(TokenType::COLON)) {
        method->returnType = parseType();
    }
    
    method->body = parseBlock();
    
    return method;
}

} // namespace compiler
} // namespace dialos
