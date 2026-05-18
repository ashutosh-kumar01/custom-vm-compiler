#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

enum class TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR, MOD,
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT,

    // One or two character tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL, DECLARE_ASSIGN, // :=
    GREATER, GREATER_EQUAL, SHIFT_RIGHT,
    LESS, LESS_EQUAL, SHIFT_LEFT,

    // Literals
    IDENTIFIER, STRING, NUMBER, FLOAT_NUMBER, CHAR_LITERAL,

    // Keywords
    AND, OR, XOR, NOT,
    WHEN, ELSEWHEN, OTHERWISE,
    LOOP, EACH, ASSUME,
    SHOW, ASK, GIVE, FN,
    TRUE_LIT, FALSE_LIT,
    
    // Built-ins (optional, can be identifiers, but let's keep them as identifiers)

    TOKEN_EOF, ERROR
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token() : type(TokenType::TOKEN_EOF), lexeme(""), line(0), column(0) {}
    Token(TokenType type, std::string lexeme, int line, int column)
        : type(type), lexeme(lexeme), line(line), column(column) {}
};

#endif
