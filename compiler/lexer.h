#ifndef DIALOS_LEXER_H
#define DIALOS_LEXER_H

#include <string>
#include <cstddef>

namespace dialos {
namespace compiler {

enum class TokenType {
    // Literals
    NUMBER,
    STRING,
    BOOLEAN,
    NULL_LIT,
    IDENTIFIER,
    
    // Keywords
    VAR,
    ASSIGN,
    FUNCTION,
    CLASS,
    CONSTRUCTOR,
    IF,
    ELSE,
    WHILE,
    FOR,
    TRY,
    CATCH,
    FINALLY,
    RETURN,
    
    // Type keywords
    INT,
    UINT,
    BYTE,
    SHORT,
    FLOAT,
    BOOL,
    STRING_TYPE,
    VOID,
    ANY,
    
    // Boolean literals
    TRUE,
    FALSE,
    
    // Operators
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %
    
    EQUAL,          // =
    NOT_EQUAL,      // !=
    LESS,           // <
    GREATER,        // >
    LESS_EQUAL,     // <=
    GREATER_EQUAL,  // >=
    
    AND,            // and
    OR,             // or
    NOT,            // not
    
    QUESTION,       // ?
    COLON,          // :
    
    // Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    SEMICOLON,      // ;
    COMMA,          // ,
    DOT,            // .
    
    // Template literals
    BACKTICK,       // `
    TEMPLATE_START, // ${
    TEMPLATE_TEXT,  // text content inside template literals
    
    // Special
    END_OF_FILE,
    ERROR,
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token() : type(TokenType::ERROR), line(0), column(0) {}
    Token(TokenType t, const std::string& v, int l, int c) 
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
public:
    Lexer(const std::string& source) 
        : source_(source), pos_(0), line_(1), column_(1), inTemplate_(false) {}
    
    Token nextToken();
    Token peekToken();
    
    const std::string& getSource() const { return source_; }
    int getLine() const { return line_; }
    int getColumn() const { return column_; }
    
private:
    std::string source_;
    size_t pos_;
    int line_;
    int column_;
    Token peeked_;
    bool hasPeeked_ = false;
    bool inTemplate_ = false;  // Track if we're inside a template literal
    int templateDepth_ = 0;    // Track brace nesting depth in template expressions
    
    char current() const;
    char peek(int offset = 1) const;
    void advance();
    void skipWhitespace();
    void skipComment();
    
    Token makeToken(TokenType type, const std::string& value);
    Token scanNumber();
    Token scanString(char quote);
    Token scanIdentifierOrKeyword();
    Token scanOperator();
    Token scanTemplateText();  // Scan text content inside template literals
    
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    bool isAtEnd() const;
    
    TokenType keywordType(const std::string& word) const;
};

} // namespace compiler
} // namespace dialos

#endif // DIALOS_LEXER_H
