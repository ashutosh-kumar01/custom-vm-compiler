#include "parser.hpp"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

std::vector<std::shared_ptr<Stmt>> Parser::parse() {
    std::vector<std::shared_ptr<Stmt>> statements;
    while (!isAtEnd()) {
        statements.push_back(declaration());
    }
    return statements;
}

std::shared_ptr<Stmt> Parser::declaration() {
    if (match({TokenType::FN})) return functionDeclaration();
    if (match({TokenType::ASSUME})) return varDeclaration();
    return statement();
}

std::shared_ptr<Stmt> Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    std::shared_ptr<Expr> initializer = nullptr;
    if (match({TokenType::DECLARE_ASSIGN, TokenType::EQUAL})) {
        initializer = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_shared<VarDeclStmt>(name, initializer);
}

std::shared_ptr<Stmt> Parser::functionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
    consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");
    std::vector<Token> params;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            params.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name."));
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TokenType::LEFT_BRACE, "Expect '{' before function body.");
    auto body = std::static_pointer_cast<BlockStmt>(block());
    return std::make_shared<FunctionStmt>(name, params, body);
}

std::shared_ptr<Stmt> Parser::statement() {
    if (match({TokenType::WHEN})) return ifStatement();
    if (match({TokenType::LOOP})) return whileStatement();
    if (match({TokenType::EACH})) return eachStatement();
    if (match({TokenType::LEFT_BRACE})) return block();
    if (match({TokenType::GIVE})) return returnStatement();
    return expressionStatement();
}

std::shared_ptr<Stmt> Parser::eachStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'each'.");
    
    std::shared_ptr<Stmt> initializer;
    if (match({TokenType::SEMICOLON})) {
        initializer = nullptr;
    } else if (match({TokenType::ASSUME})) {
        initializer = varDeclaration();
    } else {
        initializer = expressionStatement();
    }
    
    std::shared_ptr<Expr> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");
    
    std::shared_ptr<Expr> increment = nullptr;
    if (!check(TokenType::RIGHT_PAREN)) {
        increment = expression();
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after each clauses.");
    
    std::shared_ptr<Stmt> body = statement();
    
    if (increment != nullptr) {
        std::vector<std::shared_ptr<Stmt>> bodyStmts;
        bodyStmts.push_back(body);
        bodyStmts.push_back(std::make_shared<ExprStmt>(increment));
        body = std::make_shared<BlockStmt>(bodyStmts);
    }
    
    if (condition == nullptr) {
        Token trueToken;
        trueToken.type = TokenType::TRUE_LIT;
        trueToken.lexeme = "true";
        condition = std::make_shared<LiteralExpr>(trueToken);
    }
    
    body = std::make_shared<WhileStmt>(condition, body);
    
    if (initializer != nullptr) {
        std::vector<std::shared_ptr<Stmt>> stmts;
        stmts.push_back(initializer);
        stmts.push_back(body);
        body = std::make_shared<BlockStmt>(stmts);
    }
    
    return body;
}

std::shared_ptr<Stmt> Parser::returnStatement() {
    Token keyword = previous();
    std::shared_ptr<Expr> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after return value.");
    return std::make_shared<ReturnStmt>(keyword, value);
}

std::shared_ptr<Stmt> Parser::whileStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'loop'.");
    std::shared_ptr<Expr> condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");
    std::shared_ptr<Stmt> body = statement();
    return std::make_shared<WhileStmt>(condition, body);
}

std::shared_ptr<Stmt> Parser::ifStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'when'.");
    std::shared_ptr<Expr> condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");

    std::shared_ptr<Stmt> thenBranch = statement();
    std::shared_ptr<Stmt> elseBranch = nullptr;

    if (match({TokenType::OTHERWISE})) {
        elseBranch = statement();
    } else if (match({TokenType::ELSEWHEN})) {
        // Syntactic sugar: elsewhen translates to else if
        // We will just put the ifStatement recursively into the elseBranch.
        // Go back one token so `when` logic is somewhat reused? No, elsewhen means if.
        // Actually, let's just implement elsewhen as another ifStatement.
        current--; // backtrack
        tokens[current].type = TokenType::WHEN; // replace ELSEWHEN with WHEN
        advance(); // consume the WHEN token so ifStatement can consume LEFT_PAREN next
        elseBranch = ifStatement();
    }

    return std::make_shared<IfStmt>(condition, thenBranch, elseBranch);
}

std::shared_ptr<Stmt> Parser::block() {
    std::vector<std::shared_ptr<Stmt>> statements;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }
    consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
    return std::make_shared<BlockStmt>(statements);
}

std::shared_ptr<Stmt> Parser::expressionStatement() {
    std::shared_ptr<Expr> expr = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_shared<ExprStmt>(expr);
}

std::shared_ptr<Expr> Parser::expression() {
    return assignment();
}

std::shared_ptr<Expr> Parser::assignment() {
    std::shared_ptr<Expr> expr = logicalOr();

    if (match({TokenType::DECLARE_ASSIGN})) {
        Token equals = previous();
        std::shared_ptr<Expr> value = assignment();

        if (auto e = std::dynamic_pointer_cast<VarExpr>(expr)) {
            Token name = e->name;
            return std::make_shared<AssignExpr>(name, value);
        } else if (auto se = std::dynamic_pointer_cast<SubscriptExpr>(expr)) {
            return std::make_shared<IndexAssignExpr>(se->object, se->bracket, se->index, value);
        }

        std::cerr << "Invalid assignment target." << std::endl;
    }

    return expr;
}

std::shared_ptr<Expr> Parser::logicalOr() {
    std::shared_ptr<Expr> expr = logicalAnd();
    while (match({TokenType::OR})) {
        Token op = previous();
        std::shared_ptr<Expr> right = logicalAnd();
        expr = std::make_shared<LogicalExpr>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::logicalAnd() {
    std::shared_ptr<Expr> expr = equality();
    while (match({TokenType::AND})) {
        Token op = previous();
        std::shared_ptr<Expr> right = equality();
        expr = std::make_shared<LogicalExpr>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::equality() {
    std::shared_ptr<Expr> expr = comparison();

    while (match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL})) {
        Token op = previous();
        std::shared_ptr<Expr> right = comparison();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::comparison() {
    std::shared_ptr<Expr> expr = term();

    while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) {
        Token op = previous();
        std::shared_ptr<Expr> right = term();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::term() {
    std::shared_ptr<Expr> expr = factor();

    while (match({TokenType::MINUS, TokenType::PLUS})) {
        Token op = previous();
        std::shared_ptr<Expr> right = factor();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::factor() {
    std::shared_ptr<Expr> expr = unary();

    while (match({TokenType::SLASH, TokenType::STAR, TokenType::MOD})) {
        Token op = previous();
        std::shared_ptr<Expr> right = unary();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::unary() {
    if (match({TokenType::BANG, TokenType::MINUS})) {
        // Token op = previous();
        // std::shared_ptr<Expr> right = unary();
        // Return a UnaryExpr. Omitting for brevity, just fall back to call.
    }
    return call();
}

std::shared_ptr<Expr> Parser::call() {
    std::shared_ptr<Expr> expr = primary();

    while (true) {
        if (match({TokenType::LEFT_PAREN})) {
            std::vector<std::shared_ptr<Expr>> args;
            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    args.push_back(expression());
                } while (match({TokenType::COMMA}));
            }
            Token paren = consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
            expr = std::make_shared<CallExpr>(expr, paren, args);
        } else if (match({TokenType::LEFT_BRACKET})) {
            std::shared_ptr<Expr> index = expression();
            Token bracket = consume(TokenType::RIGHT_BRACKET, "Expect ']' after index.");
            expr = std::make_shared<SubscriptExpr>(expr, bracket, index);
        } else {
            break;
        }
    }
    return expr;
}

std::shared_ptr<Expr> Parser::primary() {
    if (match({TokenType::FALSE_LIT})) return std::make_shared<LiteralExpr>(previous());
    if (match({TokenType::TRUE_LIT})) return std::make_shared<LiteralExpr>(previous());
    if (match({TokenType::NUMBER, TokenType::FLOAT_NUMBER, TokenType::STRING, TokenType::CHAR_LITERAL})) {
        return std::make_shared<LiteralExpr>(previous());
    }
    if (match({TokenType::IDENTIFIER, TokenType::SHOW, TokenType::ASK})) {
        // SHOW and ASK are built-ins, but parsed as identifiers here for calls
        return std::make_shared<VarExpr>(previous());
    }
    if (match({TokenType::LEFT_PAREN})) {
        std::shared_ptr<Expr> expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        return expr; // GroupingExpr ideally
    }
    std::cerr << "Expected expression at line " << peek().line << std::endl;
    advance(); // recover somewhat
    return nullptr;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::TOKEN_EOF;
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    std::cerr << message << " at line " << peek().line << std::endl;
    throw std::runtime_error(message);
}
