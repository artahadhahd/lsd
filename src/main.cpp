#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <variant>
#include <memory>
#include <vector>
#include <type_traits>
#include <unordered_map>
#include <functional>
#include <span>
#include <chrono>

#include "lexer.hpp"
#include "parser.hpp"

using namespace std::string_literals;

std::string readFileContent(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        return "";
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

Tree::T Tree::eval(Environment& env) {
    if (content.empty()) {
        return std::move(std::make_unique<Tree>());
    }
    Atom* atom;
    if (!(atom = std::get_if<Atom>(&content.at(0)))) {
        std::cerr << "Not a function: " << content.at(0) << std::endl;
        return std::make_unique<Tree>();
    }

    Symbol* symbol;
    if (!(symbol = std::get_if<Symbol>(atom))) {
        std::cerr << "Not a function: " << content.at(0) << std::endl;
        return std::make_unique<Tree>();
    }

    std::span<Tree::T> args(content.begin() + 1, content.end());

    if (env.has_function(*symbol)) {
        return env.functions.at(*symbol)(args, env);
    } else {
        throw std::runtime_error("Function " + *symbol + " not defined");
    }

    return Tree::T(*atom);
}


struct Repl {
    Repl() = delete;
    static void loop() {
        while (true) {
            std::cout << "(lsd) ";
            std::string in;
            std::getline(std::cin, in);

            if (in == "quit" || in == "exit") {
                break;
            }

            Environment env;

            auto start = std::chrono::high_resolution_clock::now();
            decltype(start) end;
            std::chrono::nanoseconds duration;
            Lexer lexer(in);
            Parser parser(lexer);
            Tree::T result;

            try {
                while (!parser.is_done()) {
                    result = parser.get_next_list().eval(env);
                    end = std::chrono::high_resolution_clock::now();
                    duration = std::chrono::duration_cast<decltype(duration)>(end - start);
                }
                std::cout << result;
            } catch(std::runtime_error& e) {
                std::cerr << e.what();
            }

            std::cout << std::endl;
            std::cout << "Evaluation time: " << duration.count() << " ns\n";
        }
    }
};



int main(int argc, char* argv[])
{
    if (argc == 1) {
        Repl::loop();
        return EXIT_SUCCESS;
    }
    auto file = readFileContent(argv[1]);

    Lexer lexer(file);
    Parser parser(lexer);
    Environment env;

    while (!parser.is_done()) {
        parser.get_next_list().eval(env);
    }
}