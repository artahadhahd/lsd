#ifndef LSD_PARSER_HPP
#define LSD_PARSER_HPP

#include "lexer.hpp"

#include <variant>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>
#include <span>

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

using Number = std::variant<int, float>;
using Symbol = std::string;
using Atom = std::variant<Number, Symbol>;

struct Environment;

struct Tree {
    using T = std::variant<std::unique_ptr<Tree>, Atom>;
    std::vector<T> content;

    friend std::ostream& operator<<(std::ostream& os, const Tree& list);
    friend std::ostream& operator<<(std::ostream& os, const Tree::T&);

    using Function = std::function<Tree::T(std::span<Tree::T>, Environment&)>;

    T eval(Environment& env);
};

struct Environment {
    std::unordered_map<std::string, Tree::Function> functions;
    std::unordered_map<std::string, Tree::T> variables;
    std::unordered_map<std::string, std::string> aliases;
    Environment();
    bool has_function(const Symbol&) const;
};


#define LSD_DECL_PRIM(f)            Tree::T primitive_##f(std::span<Tree::T>, Environment&)
#define LSD_DEF_PRIM(f, args, env)  Tree::T primitive_##f(std::span<Tree> args, Environment& env)
namespace lsd {
    LSD_DECL_PRIM(add);
    LSD_DECL_PRIM(sub);
    LSD_DECL_PRIM(print);
    LSD_DECL_PRIM(set); // set variables
    LSD_DECL_PRIM(if); // if statements
    LSD_DECL_PRIM(quote);
    LSD_DECL_PRIM(alias);
}

class Parser {
    Lexer lexer;
    bool failed = false;
public:
    Parser(Lexer lexer);
    bool has_failed();
    bool is_done();
    Tree get_next_list();
private:
    Tree parse_list(bool check_left = true);
    bool expect(Token::Type);
};

#endif