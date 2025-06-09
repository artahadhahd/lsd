#ifndef LSD_LEXER_HPP
#define LSD_LEXER_HPP
#include <string>

struct Token {
    enum struct Type {
        End,
        Wtf,
        Lp,
        Rp,
        Ident,
        Num
    } type;
    std::string lexeme;

    Token(Type type, std::string lexeme) : type(type), lexeme(lexeme) {}
    Token() = default;
};

class Lexer {
    size_t max;
    std::string source;
    size_t cursor;
public:
    Lexer(std::string source);
    Token next();
    bool is_done();
};
#endif