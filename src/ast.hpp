#ifndef AST_HPP
#define AST_HPP

#include "token.hpp"
#include <vector>
#include <memory>
#include <iostream>

struct Expr {
    virtual ~Expr() = default;
    virtual void print(std::string prefix = "", bool isLast = true) const = 0;
};

struct Stmt {
    virtual ~Stmt() = default;
    virtual void print(std::string prefix = "", bool isLast = true) const = 0;
};

struct LiteralExpr : public Expr {
    Token value;
    LiteralExpr(Token value) : value(value) {}
    void print(std::string prefix = "", bool isLast = true) const override { 
        if (value.type == TokenType::NUMBER || value.type == TokenType::FLOAT_NUMBER) {
            std::cout << prefix << (isLast ? "└── " : "├── ") << "Number(" << value.lexeme << ")\n"; 
        } else {
            std::cout << prefix << (isLast ? "└── " : "├── ") << "Literal(" << value.lexeme << ")\n"; 
        }
    }
};

struct VarExpr : public Expr {
    Token name;
    VarExpr(Token name) : name(name) {}
    void print(std::string prefix = "", bool isLast = true) const override { 
        std::cout << prefix << (isLast ? "└── " : "├── ") << "Variable(" << name.lexeme << ")\n"; 
    }
};

struct BinaryExpr : public Expr {
    std::shared_ptr<Expr> left;
    Token op;
    std::shared_ptr<Expr> right;
    BinaryExpr(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right)
        : left(left), op(op), right(right) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "BinaryExpr(" << op.lexeme << ")\n";
        left->print(prefix + (isLast ? "    " : "│   "), false);
        right->print(prefix + (isLast ? "    " : "│   "), true);
    }
};

struct LogicalExpr : public Expr {
    std::shared_ptr<Expr> left;
    Token op;
    std::shared_ptr<Expr> right;
    LogicalExpr(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right)
        : left(left), op(op), right(right) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "LogicalExpr(" << op.lexeme << ")\n";
        left->print(prefix + (isLast ? "    " : "│   "), false);
        right->print(prefix + (isLast ? "    " : "│   "), true);
    }
};

struct CallExpr : public Expr {
    std::shared_ptr<Expr> callee;
    Token paren;
    std::vector<std::shared_ptr<Expr>> arguments;
    CallExpr(std::shared_ptr<Expr> callee, Token paren, std::vector<std::shared_ptr<Expr>> arguments)
        : callee(callee), paren(paren), arguments(arguments) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "CallExpr\n";
        std::string nextPrefix = prefix + (isLast ? "    " : "│   ");
        
        bool hasArgs = !arguments.empty();
        std::cout << nextPrefix << (hasArgs ? "├── " : "└── ") << "Callee:\n";
        callee->print(nextPrefix + (hasArgs ? "│   " : "    "), true);
        
        if (hasArgs) {
            std::cout << nextPrefix << "└── " << "Arguments:\n";
            for (size_t i = 0; i < arguments.size(); ++i) {
                arguments[i]->print(nextPrefix + "    ", i == arguments.size() - 1);
            }
        }
    }
};

struct ExprStmt : public Stmt {
    std::shared_ptr<Expr> expression;
    ExprStmt(std::shared_ptr<Expr> expression) : expression(expression) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "ExprStmt\n";
        expression->print(prefix + (isLast ? "    " : "│   "), true);
    }
};

struct VarDeclStmt : public Stmt {
    Token name;
    std::shared_ptr<Expr> initializer;
    VarDeclStmt(Token name, std::shared_ptr<Expr> initializer) : name(name), initializer(initializer) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "VariableDecl(" << name.lexeme << ")\n";
        if (initializer) {
            initializer->print(prefix + (isLast ? "    " : "│   "), true);
        }
    }
};

struct AssignExpr : public Expr {
    Token name;
    std::shared_ptr<Expr> value;
    AssignExpr(Token name, std::shared_ptr<Expr> value) : name(name), value(value) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "AssignExpr(" << name.lexeme << ")\n";
        value->print(prefix + (isLast ? "    " : "│   "), true);
    }
};

struct BlockStmt : public Stmt {
    std::vector<std::shared_ptr<Stmt>> statements;
    BlockStmt(std::vector<std::shared_ptr<Stmt>> statements) : statements(statements) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "Block\n";
        std::string nextPrefix = prefix + (isLast ? "    " : "│   ");
        for (size_t i = 0; i < statements.size(); ++i) {
            statements[i]->print(nextPrefix, i == statements.size() - 1);
        }
    }
};

struct IfStmt : public Stmt {
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> thenBranch;
    std::shared_ptr<Stmt> elseBranch; // Can be null
    IfStmt(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> thenBranch, std::shared_ptr<Stmt> elseBranch)
        : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "IfStmt\n";
        std::string nextPrefix = prefix + (isLast ? "    " : "│   ");
        
        std::cout << nextPrefix << "├── " << "Condition:\n";
        condition->print(nextPrefix + "│   ", true);
        
        std::cout << nextPrefix << (elseBranch ? "├── " : "└── ") << "ThenBranch:\n";
        thenBranch->print(nextPrefix + (elseBranch ? "│   " : "    "), true);
        
        if (elseBranch) {
            std::cout << nextPrefix << "└── " << "ElseBranch:\n";
            elseBranch->print(nextPrefix + "    ", true);
        }
    }
};

struct WhileStmt : public Stmt {
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> body;
    WhileStmt(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> body) : condition(condition), body(body) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "WhileStmt\n";
        std::string nextPrefix = prefix + (isLast ? "    " : "│   ");
        
        std::cout << nextPrefix << "├── " << "Condition:\n";
        condition->print(nextPrefix + "│   ", true);
        
        std::cout << nextPrefix << "└── " << "Body:\n";
        body->print(nextPrefix + "    ", true);
    }
};

struct FunctionStmt : public Stmt {
    Token name;
    std::vector<Token> params;
    std::shared_ptr<BlockStmt> body;
    FunctionStmt(Token n, std::vector<Token> p, std::shared_ptr<BlockStmt> b) 
        : name(n), params(p), body(b) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "FunctionDecl(" << name.lexeme << ")\n";
        body->print(prefix + (isLast ? "    " : "│   "), true);
    }
};

struct ReturnStmt : public Stmt {
    Token keyword;
    std::shared_ptr<Expr> value;
    ReturnStmt(Token k, std::shared_ptr<Expr> v) : keyword(k), value(v) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "Return\n";
        if (value) {
            value->print(prefix + (isLast ? "    " : "│   "), true);
        }
    }
};

struct SubscriptExpr : public Expr {
    std::shared_ptr<Expr> object;
    Token bracket;
    std::shared_ptr<Expr> index;
    SubscriptExpr(std::shared_ptr<Expr> object, Token bracket, std::shared_ptr<Expr> index)
        : object(object), bracket(bracket), index(index) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "SubscriptExpr\n";
        std::string nextPrefix = prefix + (isLast ? "    " : "│   ");
        std::cout << nextPrefix << "├── " << "Object:\n";
        object->print(nextPrefix + "│   ", true);
        std::cout << nextPrefix << "└── " << "Index:\n";
        index->print(nextPrefix + "    ", true);
    }
};

struct IndexAssignExpr : public Expr {
    std::shared_ptr<Expr> object;
    Token bracket;
    std::shared_ptr<Expr> index;
    std::shared_ptr<Expr> value;
    IndexAssignExpr(std::shared_ptr<Expr> object, Token bracket, std::shared_ptr<Expr> index, std::shared_ptr<Expr> value)
        : object(object), bracket(bracket), index(index), value(value) {}
    void print(std::string prefix = "", bool isLast = true) const override {
        std::cout << prefix << (isLast ? "└── " : "├── ") << "IndexAssignExpr\n";
        std::string nextPrefix = prefix + (isLast ? "    " : "│   ");
        std::cout << nextPrefix << "├── " << "Object:\n";
        object->print(nextPrefix + "│   ", true);
        std::cout << nextPrefix << "├── " << "Index:\n";
        index->print(nextPrefix + "│   ", true);
        std::cout << nextPrefix << "└── " << "Value:\n";
        value->print(nextPrefix + "    ", true);
    }
};

#endif
