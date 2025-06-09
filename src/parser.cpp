#include "parser.hpp"
#include <iostream>
#include <chrono>

Parser::Parser(Lexer lexer) : lexer(lexer) {}

bool Parser::has_failed() { return failed; }

bool Parser::is_done() { return lexer.is_done(); }

Tree Parser::get_next_list()
{
    Tree list = parse_list();
    return list;
}

Tree Parser::parse_list(bool check_left)
{
    Tree list;
    if (check_left && expect(Token::Type::Lp)) {
        return list;
    }

    Token token;

    while (true) {
        token = lexer.next();
        if (token.type == Token::Type::End) {
            std::cerr << "Unexpected end of stream\n";
            failed = true;
            return Tree{};
        }
        if (token.type == Token::Type::Wtf) {
            std::cerr << "Unrecognized character " << token.lexeme << '\n';
            failed = true;
            return Tree{};
        }
        
        if (token.type == Token::Type::Num) {
            list.content.emplace_back(std::stoi(token.lexeme));
        }
        if (token.type == Token::Type::Ident) {
            list.content.emplace_back(token.lexeme);
        }
        if (token.type == Token::Type::Lp) {
            list.content.emplace_back(std::make_unique<Tree>(parse_list(false)));
        }
        if (token.type == Token::Type::Rp) {
            break;
        }
    }

    return list;
}

bool Parser::expect(Token::Type type)
{
    auto token = lexer.next();
    if (token.type != type) {
        std::cerr << "Error on token " << token.lexeme << ": Expected the token (";
        failed = true;
        return true;
    }
    return false;
}

std::ostream& operator<<(std::ostream& os, const Tree::T& value)
{
    std::visit(overloads {
        [&os](const Atom& atom) {
            std::visit(overloads {
                [&os](Number number) {
                    std::visit(overloads{
                        [&os](const int& n) { os << "int(" << n << ')'; },
                        [&os](const float& f) { os << f; }
                    }, number);
                },
                [&os](Symbol symbol) {
                    os << "Symbol(" << symbol << ')';
                }
            }, atom);
        },
        [&os](const std::unique_ptr<Tree>& list) {
            os << *list;
        }
    }, value);
    return os;
}


std::ostream& operator<<(std::ostream& os, const Tree& list) {
    if (list.content.empty()) {
        os << "nil";
        return os;
    }
    os << '(';
    for (auto const& value : list.content) {
        os << value << ' ';
    }
    os << "\b)";
    return os;
}

static Tree::T getValue(const Tree::T& cmp, Environment& env) {
    return std::visit(overloads {
        [&env](const std::unique_ptr<Tree>& child) -> Tree::T {
            return child->eval(env);
        },
        [&env](const Atom& rest) -> Tree::T {
            return std::visit(overloads {
                [&env](const Symbol& sym) -> Tree::T {
                    Symbol symbol;
                    if (env.aliases.contains(sym)) {
                        symbol = env.aliases.at(sym);
                    } else {
                        symbol = sym;
                    }
                    if (env.variables.contains(symbol)) {
                        return getValue(env.variables.at(symbol), env);
                    } else {
                        throw std::runtime_error("Variable " + sym + " doesn't exist");
                    }
                },
                [&env](const auto& r) -> Tree::T {
                    return r;
                }
            }, rest);
        }
    }, cmp);
}

static bool isNumber(const Tree::T& tree) {
    return std::visit(overloads {
        [](const Atom& atom) -> bool {
            return std::visit(overloads {
                [](const Number& number) -> bool {
                    return true;
                },
                [](const auto& rest) -> bool {
                    return false;
                }
            }, atom);
        },
        [](const auto& rest) -> bool {
            return false;
        }
    }, tree);
}

static uint8_t isTruthy(const Tree::T& tree) {
    return std::visit(overloads {
        [](const Atom& atom) -> uint8_t {
            if (std::holds_alternative<Symbol>(atom)) {
                return 0; // in this case, true is 0 and false is 1
            }
            if (std::get<int>(std::get<Number>(atom)) == 0) {
                return 1;
            } else {
                return 0;
            }
        },
        [](const std::unique_ptr<Tree>& child) -> uint8_t {
            return 1;
        }
    }, tree);
}

namespace lsd {
    Tree::T primitive_add(std::span<Tree::T> args, Environment& env) {
        int sum = 0;
        for (const auto& i : args) {
            Tree::T var = getValue(i, env);
            Atom* atom = std::get_if<Atom>(&var);
            if (atom) {
                std::visit(overloads{
                    [&sum](Number num) -> void {
                        std::visit(overloads{
                            [&sum](int num) -> void {
                                sum += num;
                            },
                            [](float num) -> void {
                                throw std::runtime_error("Floats are not yet supported");
                            }
                        }, num);
                    },
                    [](Symbol sym) -> void {
                        throw std::runtime_error("UNREACHABLE");
                    }
                }, *atom);
            } else {
                throw std::runtime_error("Bad type in argument");
            }
        }

        return sum;
    }

    Tree::T primitive_sub(std::span<Tree::T> args, Environment& env) {
        bool fst = true;
        int sum = 0;
        for (const auto& i : args) {
            Tree::T var = getValue(i, env);
            Atom* atom = std::get_if<Atom>(&var);
            if (atom) {
                std::visit(overloads{
                    [&sum, &fst](Number num) -> void {
                        std::visit(overloads{
                            [&sum, &fst](int num) -> void {
                                if (fst) {
                                    sum = num;
                                    fst = false;
                                } else {
                                    sum -= num;
                                }
                            },
                            [](float num) -> void {
                                throw std::runtime_error("Floats are not yet supported");
                            }
                        }, num);
                    },
                    [&env](Symbol sym) -> void {
                        throw std::runtime_error("UNREACHABLE");
                    }
                }, *atom);
            } else {
                throw std::runtime_error("Bad type in argument");
            }
        }

        return sum;
    }

    Tree::T primitive_print(std::span<Tree::T> args, [[maybe_unused]] Environment& env) {
        for (const auto& i : args) {
            std::cout << i << ' ';
        }
        std::cout << '\b';
        return std::make_unique<Tree>();
    }

    Tree::T primitive_set(std::span<Tree::T> args, Environment& env) {
        if (args.size() < 2) {
            throw std::runtime_error("Not enough arguments passed to set");
        }
        if (args.size() > 2) {
            throw std::runtime_error("Too many arguments passed to set");
        }
        auto& var = args.front();
        Atom atom;
        Symbol var_name;
        if (std::holds_alternative<Atom>(var) && std::holds_alternative<Symbol>((atom = std::get<Atom>(var)))) {
            var_name = std::get<Symbol>(atom);
        } else {
            throw std::runtime_error("set expects a symbol as first argument");
        }

        auto& val = args.back();
        env.variables[var_name] = std::move(val);
        return std::make_unique<Tree>();
    }

    Tree::T primitive_if(std::span<Tree::T> args, Environment& env) {
        if (args.size() != 3) {
            throw std::runtime_error(args.size() > 3 ? "Too many arguments passed to if" : "Expected more arguments to if");
        }
        const auto condition = getValue(args.front(), env);
        return std::move(getValue(args[isTruthy(condition) + 1], env));
    }

    Tree::T primitive_quote(std::span<Tree::T> args, [[maybe_unused]] Environment& env) {
        if (args.size() != 1) {
            throw std::runtime_error("quote takes exactly one argument");
        }
        return std::move(args.front());
    }

    // this code sucks.
    Tree::T primitive_alias(std::span<Tree::T> args, [[maybe_unused]] Environment& env) {
        if (args.size() != 2) {
            throw std::runtime_error(args.size() > 2 ? "Too many arguments passed to alias" : "Expected more arguments to alias");
        }
        const auto& fst = args.front();
        const auto& snd = args.back();
        if (!(std::holds_alternative<Atom>(fst) && std::holds_alternative<Atom>(snd))) {
            throw std::runtime_error("Both arguments of alias expect a symbol");
        }
        const auto a1 = std::get<Atom>(fst), a2 = std::get<Atom>(snd);
        if (!(std::holds_alternative<Symbol>(a1) && std::holds_alternative<Symbol>(a2))) {
            throw std::runtime_error("Both arguments of alias expect a symbol");
        }
        auto val = std::get<Symbol>(a1);
        if (!env.variables.contains(val)) {
            throw std::runtime_error("Cannot alias a variable that does not exist");
        }
        auto key = std::get<Symbol>(a2);
        if (env.variables.contains(key)) {
            throw std::runtime_error("Cannot alias as an already existing symbol");
        }
        env.aliases.insert({key, val});
        return std::make_unique<Tree>();
    }
}

Environment::Environment() {
    functions.insert({"add", lsd::primitive_add});
    functions.insert({"sub", lsd::primitive_sub});
    functions.insert({"print", lsd::primitive_print});
    functions.insert({"set", lsd::primitive_set});
    functions.insert({"if", lsd::primitive_if});
    functions.insert({"quote", lsd::primitive_quote});
    functions.insert({"alias", lsd::primitive_alias});

    variables.insert({"true", 1});
    variables.insert({"false", 0});
}

bool Environment::has_function(const Symbol& symbol) const {
    return functions.contains(symbol);
}