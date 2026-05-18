#ifndef PARSER_HPP
#define PARSER_HPP

#include "token.hpp"
#include "ast.hpp"
#include <vector>
#include <memory>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::vector<std::shared_ptr<Stmt>> parse();

private:
    std::vector<Token> tokens;
    int current = 0;

    std::shared_ptr<Stmt> statement();
    std::shared_ptr<Stmt> declaration();
    std::shared_ptr<Stmt> varDeclaration();
    std::shared_ptr<Stmt> functionDeclaration();
    std::shared_ptr<Stmt> returnStatement();
    std::shared_ptr<Stmt> ifStatement();
    std::shared_ptr<Stmt> whileStatement();
    std::shared_ptr<Stmt> eachStatement();
    std::shared_ptr<Stmt> block();
    std::shared_ptr<Stmt> expressionStatement();

    std::shared_ptr<Expr> expression();
    std::shared_ptr<Expr> assignment();
    std::shared_ptr<Expr> logicalOr();
    std::shared_ptr<Expr> logicalAnd();
    std::shared_ptr<Expr> equality();
    std::shared_ptr<Expr> comparison();
    std::shared_ptr<Expr> term();
    std::shared_ptr<Expr> factor();
    std::shared_ptr<Expr> unary();
    std::shared_ptr<Expr> call();
    std::shared_ptr<Expr> primary();

    bool match(std::initializer_list<TokenType> types);
    bool check(TokenType type) const;
    Token advance();
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token consume(TokenType type, const std::string& message);
};

#endif
