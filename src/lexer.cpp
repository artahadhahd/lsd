#include "lexer.hpp"
#include <iostream>

Lexer::Lexer(std::string source) :  max(source.length()), source(source), cursor(0) {
}

bool Lexer::is_done() { return cursor >= max; }

Token Lexer::next() {
    BEGIN:
    if (cursor >= max) {
        return Token{
            Token::Type::End, ""
        };
    }

    if (std::isspace(source[cursor])) {
        while (std::isspace(source[cursor])) {
            cursor++;
        }
        goto BEGIN;
    }

    switch (source[cursor])
    {
    case '(':
        cursor++;
        return Token{Token::Type::Lp, "("};
    case ')':
        cursor++;
        return Token{Token::Type::Rp, ")"};        
    default:
        break;
    }

    if (std::isdigit(source[cursor])) {
        std::string lexeme;
        while (std::isdigit(source[cursor])) {
            lexeme += source[cursor++];
        }
        return Token{Token::Type::Num, lexeme};
    }
    if (std::isalpha(source[cursor])) {
        std::string lexeme;
        while (std::isalpha(source[cursor])) {
            lexeme += source[cursor++];
        }
        return Token{Token::Type::Ident, lexeme};
    }

    return Token{Token::Type::Wtf, std::string{source[cursor++]}};
}