#include "lexer.h"

namespace dialos {
namespace compiler {

Token Lexer::nextToken() {
    if (hasPeeked_) {
        hasPeeked_ = false;
        return peeked_;
    }
    
    // Skip whitespace only when not in template mode
    if (!inTemplate_) {
        skipWhitespace();
    }
    
    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE, "");
    }
    
    char c = current();
    
    // Template literal handling
    if (c == '`') {
        advance();
        inTemplate_ = !inTemplate_;  // Toggle template mode
        return makeToken(TokenType::BACKTICK, "`");
    }
    
    // When inside template, handle special cases
    if (inTemplate_) {
        if (c == '$' && peek() == '{') {
            advance(); advance();
            templateDepth_ = 1;  // Start tracking brace depth
            inTemplate_ = false;  // Exit template mode for expression
            return makeToken(TokenType::TEMPLATE_START, "${");
        }
        // Scan template text content (preserves spaces)
        return scanTemplateText();
    }
    
    skipWhitespace();  // Skip whitespace for normal tokens
    
    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE, "");
    }
    
    c = current();
    
    // Numbers
    if (isDigit(c)) {
        return scanNumber();
    }
    
    // Identifiers and keywords
    if (isAlpha(c)) {
        return scanIdentifierOrKeyword();
    }
    
    // Strings
    if (c == '"' || c == '\'') {
        return scanString(c);
    }
    
    // Single character tokens
    switch (c) {
        case '(': advance(); return makeToken(TokenType::LPAREN, "(");
        case ')': advance(); return makeToken(TokenType::RPAREN, ")");
        case '{': 
            advance(); 
            if (templateDepth_ > 0) templateDepth_++;
            return makeToken(TokenType::LBRACE, "{");
        case '}': 
            advance(); 
            if (templateDepth_ > 0) {
                templateDepth_--;
                if (templateDepth_ == 0) {
                    inTemplate_ = true;  // Re-enter template mode after expression
                }
            }
            return makeToken(TokenType::RBRACE, "}");
        case '[': advance(); return makeToken(TokenType::LBRACKET, "[");
        case ']': advance(); return makeToken(TokenType::RBRACKET, "]");
        case ';': advance(); return makeToken(TokenType::SEMICOLON, ";");
        case ',': advance(); return makeToken(TokenType::COMMA, ",");
        case '.': advance(); return makeToken(TokenType::DOT, ".");
        case '?': advance(); return makeToken(TokenType::QUESTION, "?");
        case ':': advance(); return makeToken(TokenType::COLON, ":");
        case '%': advance(); return makeToken(TokenType::PERCENT, "%");
    }
    
    // Operators (potentially multi-char)
    return scanOperator();
}

Token Lexer::peekToken() {
    if (!hasPeeked_) {
        peeked_ = nextToken();
        hasPeeked_ = true;
    }
    return peeked_;
}

char Lexer::current() const {
    if (isAtEnd()) return '\0';
    return source_[pos_];
}

char Lexer::peek(int offset) const {
    if (pos_ + offset >= source_.length()) return '\0';
    return source_[pos_ + offset];
}

void Lexer::advance() {
    if (!isAtEnd()) {
        if (current() == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        pos_++;
    }
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek() == '/') {
            skipComment();
        } else if (c == '/' && peek() == '*') {
            // Block comment
            advance(); // /
            advance(); // *
            while (!isAtEnd() && !(current() == '*' && peek() == '/')) {
                advance();
            }
            if (!isAtEnd()) {
                advance(); // *
                advance(); // /
            }
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    // Skip until end of line
    while (!isAtEnd() && current() != '\n') {
        advance();
    }
}

Token Lexer::makeToken(TokenType type, const std::string& value) {
    return Token(type, value, line_, column_);
}

Token Lexer::scanNumber() {
    int startLine = line_;
    int startCol = column_;
    std::string value = "";
    
    // Check for hex
    if (current() == '0' && (peek() == 'x' || peek() == 'X')) {
        value += current(); advance();
        value += current(); advance();
        while (!isAtEnd() && (isDigit(current()) || 
               (current() >= 'a' && current() <= 'f') ||
               (current() >= 'A' && current() <= 'F'))) {
            value += current();
            advance();
        }
        return Token(TokenType::NUMBER, value, startLine, startCol);
    }
    
    // Regular number
    while (!isAtEnd() && isDigit(current())) {
        value += current();
        advance();
    }
    
    // Check for decimal point
    if (!isAtEnd() && current() == '.' && isDigit(peek())) {
        value += current();
        advance();
        while (!isAtEnd() && isDigit(current())) {
            value += current();
            advance();
        }
    }
    
    return Token(TokenType::NUMBER, value, startLine, startCol);
}

Token Lexer::scanString(char quote) {
    int startLine = line_;
    int startCol = column_;
    std::string value = "";
    
    advance(); // Skip opening quote
    
    while (!isAtEnd() && current() != quote) {
        if (current() == '\\') {
            advance();
            if (!isAtEnd()) {
                char escaped = current();
                switch (escaped) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case 'r': value += '\r'; break;
                    case '\\': value += '\\'; break;
                    case '"': value += '"'; break;
                    case '\'': value += '\''; break;
                    default: value += escaped; break;
                }
                advance();
            }
        } else {
            value += current();
            advance();
        }
    }
    
    if (isAtEnd()) {
        return Token(TokenType::ERROR, "Unterminated string", startLine, startCol);
    }
    
    advance(); // Skip closing quote
    return Token(TokenType::STRING, value, startLine, startCol);
}

Token Lexer::scanIdentifierOrKeyword() {
    int startLine = line_;
    int startCol = column_;
    std::string value = "";
    
    while (!isAtEnd() && isAlphaNumeric(current())) {
        value += current();
        advance();
    }
    
    TokenType type = keywordType(value);
    return Token(type, value, startLine, startCol);
}

Token Lexer::scanOperator() {
    int startLine = line_;
    int startCol = column_;
    char c = current();
    
    // Check two-character operators first
    if (c == '!' && peek() == '=') {
        advance(); advance();
        return Token(TokenType::NOT_EQUAL, "!=", startLine, startCol);
    }
    if (c == '<' && peek() == '=') {
        advance(); advance();
        return Token(TokenType::LESS_EQUAL, "<=", startLine, startCol);
    }
    if (c == '>' && peek() == '=') {
        advance(); advance();
        return Token(TokenType::GREATER_EQUAL, ">=", startLine, startCol);
    }
    if (c == '$' && peek() == '{') {
        advance(); advance();
        return Token(TokenType::TEMPLATE_START, "${", startLine, startCol);
    }
    
    // Single character operators
    switch (c) {
        case '+': advance(); return Token(TokenType::PLUS, "+", startLine, startCol);
        case '-': advance(); return Token(TokenType::MINUS, "-", startLine, startCol);
        case '*': advance(); return Token(TokenType::STAR, "*", startLine, startCol);
        case '/': advance(); return Token(TokenType::SLASH, "/", startLine, startCol);
        case '=': advance(); return Token(TokenType::EQUAL, "=", startLine, startCol);
        case '<': advance(); return Token(TokenType::LESS, "<", startLine, startCol);
        case '>': advance(); return Token(TokenType::GREATER, ">", startLine, startCol);
    }
    
    advance();
    return Token(TokenType::ERROR, std::string("Unexpected character: ") + c, startLine, startCol);
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.length();
}

TokenType Lexer::keywordType(const std::string& word) const {
    // Keywords
    if (word == "var") return TokenType::VAR;
    if (word == "assign") return TokenType::ASSIGN;
    if (word == "function") return TokenType::FUNCTION;
    if (word == "class") return TokenType::CLASS;
    if (word == "constructor") return TokenType::CONSTRUCTOR;
    if (word == "if") return TokenType::IF;
    if (word == "else") return TokenType::ELSE;
    if (word == "while") return TokenType::WHILE;
    if (word == "for") return TokenType::FOR;
    if (word == "try") return TokenType::TRY;
    if (word == "catch") return TokenType::CATCH;
    if (word == "finally") return TokenType::FINALLY;
    if (word == "return") return TokenType::RETURN;
    
    // Type keywords
    if (word == "int") return TokenType::INT;
    if (word == "uint") return TokenType::UINT;
    if (word == "byte") return TokenType::BYTE;
    if (word == "short") return TokenType::SHORT;
    if (word == "float") return TokenType::FLOAT;
    if (word == "bool") return TokenType::BOOL;
    if (word == "string") return TokenType::STRING_TYPE;
    if (word == "void") return TokenType::VOID;
    if (word == "any") return TokenType::ANY;
    
    // Boolean literals
    if (word == "true") return TokenType::TRUE;
    if (word == "false") return TokenType::FALSE;
    
    // Logical operators
    if (word == "and") return TokenType::AND;
    if (word == "or") return TokenType::OR;
    if (word == "not") return TokenType::NOT;
    
    // Null
    if (word == "null") return TokenType::NULL_LIT;
    
    return TokenType::IDENTIFIER;
}

Token Lexer::scanTemplateText() {
    int startLine = line_;
    int startCol = column_;
    std::string text = "";
    
    // Collect characters until we hit ${ or `
    while (!isAtEnd() && current() != '`' && !(current() == '$' && peek() == '{')) {
        char c = current();
        if (c == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        text += c;
        pos_++;
    }
    
    // Return the collected text as a TEMPLATE_TEXT token
    return Token(TokenType::TEMPLATE_TEXT, text, startLine, startCol);
}

} // namespace compiler
} // namespace dialos
