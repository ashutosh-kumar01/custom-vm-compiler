#ifndef LEXER_HPP
#define LEXER_HPP

#include "token.hpp"
#include <string>
#include <vector>

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> scanTokens();

private:
    std::string source;
    int start = 0;
    int current = 0;
    int line = 1;
    int column = 1;
    int startColumn = 1;

    std::vector<Token> tokens;

    bool isAtEnd() const;
    void scanToken();
    char advance();
    bool match(char expected);
    char peek() const;
    char peekNext() const;
    void string();
    void number();
    void identifier();
    void addToken(TokenType type);
    void addToken(TokenType type, const std::string& text);
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlphaNumeric(char c) const;
};

#endif
