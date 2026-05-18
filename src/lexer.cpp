#include "lexer.hpp"
#include <unordered_map>
#include <iostream>

static std::unordered_map<std::string, TokenType> keywords = {
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"xor", TokenType::XOR},
    {"not", TokenType::NOT},
    {"when", TokenType::WHEN},
    {"elsewhen", TokenType::ELSEWHEN},
    {"otherwise", TokenType::OTHERWISE},
    {"loop", TokenType::LOOP},
    {"each", TokenType::EACH},
    {"assume", TokenType::ASSUME},
    {"show", TokenType::SHOW},
    {"ask", TokenType::ASK},
    {"give", TokenType::GIVE},
    {"fn", TokenType::FN},
    {"true", TokenType::TRUE_LIT},
    {"false", TokenType::FALSE_LIT}
};

Lexer::Lexer(const std::string& source) : source(source) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        startColumn = column;
        scanToken();
    }
    tokens.emplace_back(TokenType::TOKEN_EOF, "", line, column);
    return tokens;
}

bool Lexer::isAtEnd() const {
    return current >= static_cast<int>(source.length());
}

char Lexer::advance() {
    current++;
    column++;
    return source[current - 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    column++;
    return true;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= static_cast<int>(source.length())) return '\0';
    return source[current + 1];
}

void Lexer::addToken(TokenType type) {
    addToken(type, source.substr(start, current - start));
}

void Lexer::addToken(TokenType type, const std::string& text) {
    tokens.emplace_back(type, text, line, startColumn);
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LEFT_PAREN); break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '{': addToken(TokenType::LEFT_BRACE); break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case '[': addToken(TokenType::LEFT_BRACKET); break;
        case ']': addToken(TokenType::RIGHT_BRACKET); break;
        case ',': addToken(TokenType::COMMA); break;
        case '.': addToken(TokenType::DOT); break;
        case '-': addToken(TokenType::MINUS); break;
        case '+': addToken(TokenType::PLUS); break;
        case ';': addToken(TokenType::SEMICOLON); break;
        case '*': addToken(TokenType::STAR); break;
        case '%': addToken(TokenType::MOD); break;
        case '&': addToken(TokenType::BIT_AND); break;
        case '|': addToken(TokenType::BIT_OR); break;
        case '^': addToken(TokenType::BIT_XOR); break;
        case '~': addToken(TokenType::BIT_NOT); break;
        case ':':
            if (match('=')) {
                addToken(TokenType::DECLARE_ASSIGN);
            } else {
                std::cerr << "Unexpected character ':' at line " << line << ", column " << column << std::endl;
                addToken(TokenType::ERROR);
            }
            break;
        case '!': addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG); break;
        case '=': addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL); break;
        case '<': 
            if (match('=')) addToken(TokenType::LESS_EQUAL);
            else if (match('<')) addToken(TokenType::SHIFT_LEFT);
            else addToken(TokenType::LESS);
            break;
        case '>': 
            if (match('=')) addToken(TokenType::GREATER_EQUAL);
            else if (match('>')) addToken(TokenType::SHIFT_RIGHT);
            else addToken(TokenType::GREATER);
            break;
        case '/':
            if (match('/')) {
                while (peek() != '\n' && !isAtEnd()) advance();
            } else {
                addToken(TokenType::SLASH);
            }
            break;
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            line++;
            column = 1;
            break;
        case '"': string(); break;
        case '\'': {
            char charVal = '\0';
            if (peek() == '\\') {
                advance(); // consume '\'
                char esc = advance();
                switch (esc) {
                    case 'n': charVal = '\n'; break;
                    case 't': charVal = '\t'; break;
                    case 'r': charVal = '\r'; break;
                    case '\\': charVal = '\\'; break;
                    case '\'': charVal = '\''; break;
                    default: charVal = esc; break;
                }
            } else {
                charVal = advance(); // consume char
            }
            if (peek() == '\'') {
                advance(); // consume closing '
                addToken(TokenType::CHAR_LITERAL, std::string(1, charVal));
            } else {
                std::cerr << "Unterminated char literal at line " << line << std::endl;
                addToken(TokenType::ERROR);
            }
            break;
        }
        default:
            if (isDigit(c)) {
                number();
            } else if (isAlpha(c)) {
                identifier();
            } else {
                std::cerr << "Unexpected character '" << c << "' at line " << line << ", column " << column << std::endl;
                addToken(TokenType::ERROR);
            }
            break;
    }
}

void Lexer::string() {
    std::string value = "";
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
            column = 1;
        }
        if (peek() == '\\' && peekNext() != '\0') {
            advance(); // Consume '\'
            char esc = advance();
            switch (esc) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                default: value += esc; break;
            }
        } else {
            value += advance();
        }
    }
    if (isAtEnd()) {
        std::cerr << "Unterminated string at line " << line << std::endl;
        addToken(TokenType::ERROR);
        return;
    }
    advance(); // closing "
    addToken(TokenType::STRING, value);
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

void Lexer::number() {
    bool isFloat = false;
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
        isFloat = true;
        advance(); // Consume the '.'
        while (isDigit(peek())) advance();
    }

    if (isFloat) {
        addToken(TokenType::FLOAT_NUMBER, source.substr(start, current - start));
    } else {
        addToken(TokenType::NUMBER, source.substr(start, current - start));
    }
}

void Lexer::identifier() {
    while (isAlphaNumeric(peek())) advance();
    std::string text = source.substr(start, current - start);
    TokenType type = TokenType::IDENTIFIER;
    if (keywords.find(text) != keywords.end()) {
        type = keywords[text];
    }
    addToken(type, text);
}
