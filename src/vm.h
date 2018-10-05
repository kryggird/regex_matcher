#ifndef REGEX_MATCHER_VM2_H
#define REGEX_MATCHER_VM2_H

#include <bitset>
#include <list>
#include <ostream>
#include <variant>
#include <vector>

#include "ast.h"

namespace {
    template<typename T>
    struct Range {
        using Iter = typename T::const_iterator;

        Range(Iter begin_, Iter counter_, Iter end_) : begin {begin_}, counter{counter_}, end{end_} {};

        Range(const T &inner) : Range(inner.cbegin(), inner.cbegin(), inner.cend()) {};
        Iter begin;
        Iter counter;
        Iter end;

        Range &operator++() {
            ++counter;
            return *this;
        };

        Range operator+(int shift) {
            return Range(begin, counter + shift, end);
        }

        auto *operator&() {
            return std::addressof(*counter);
        }

        auto &operator*() {
            return *counter;
        }

        bool is_start() {
            return counter == begin;
        }

        bool empty() {
            return counter == end;
        }
    };

    class Assertion {
    public:
        Assertion(re::ast::AssertionType assertion_type_): assertion_type {assertion_type_} {};
        bool test(Range<std::string> view) const {
            if (assertion_type == re::ast::AssertionType::BeginOfString) {
                return view.is_start();
            } else if (assertion_type == re::ast::AssertionType::EndOfString) {
                return view.end == view.counter;
            }
        }
        friend std::ostream& operator<<(std::ostream& os, Assertion inst) {
            os << "Assertion(" << (inst.assertion_type == re::ast::AssertionType::EndOfString ? "End" : "Begin") << ")";
            return os;
        };

    private:
        re::ast::AssertionType assertion_type;
    };

    class Character {
    public:
        Character(char c_) : c{c_} {};

        bool match(char other) const { return c == other; };
        friend std::ostream& operator<<(std::ostream& os, Character inst) {
            os << "Character(" << inst.c << ")";
            return os;
        };

    private:
        char c;
    };

    class Bitset {
    public:
        Bitset() = default;

        void set(char c) { mask.set((size_t) c); }
        void flip() { mask.flip(); };
        bool match(char other) const { return mask[(size_t) other]; };
        Bitset operator~() { return Bitset(~mask); };
        Bitset operator|(const Bitset &other) { return Bitset(mask | other.mask); }
        friend std::ostream& operator<<(std::ostream& os, Bitset inst) { os << "Bitset(...)"; return os; };

    private:
        Bitset(std::bitset<255> mask_) : mask{mask_} {};
        std::bitset<255> mask;
    };

    class Split {
    public:
        Split(long lhs_, long rhs_) : lhs{lhs_}, rhs{rhs_} {};

        friend std::ostream& operator<<(std::ostream& os, Split inst) {
            os << "Split(" << inst.lhs << ", " << inst.rhs << ")";
            return os;
        };

        long lhs;
        long rhs;
    };

    class Jump {
    public:
        Jump(long target_) : target{target_} {};

        friend std::ostream& operator<<(std::ostream& os, Jump inst) { os << "Jump(" << inst.target << ")"; return os; };

        long target;
    };

    class Match {
        friend std::ostream& operator<<(std::ostream& os, Match inst) { os << "Match()"; return os; };
    };

    Bitset make_range(char start, char end) {
        Bitset b;
        for (char c = start; c <= end; ++c) {
            b.set(c);
        }
        return b;
    }

    using Instruction = std::variant<Assertion, Character, Bitset, Split, Jump, Match>;
    using re::ast::AtomPointer;

    std::list<Instruction> compile_fragment(AtomPointer root);
    std::list<Instruction> compile_atom(AtomPointer root);

    std::list<Instruction> compile_atom(AtomPointer root) {
        if (root->type == re::ast::Type::Character) {
            auto atom = (re::ast::Character *) root;
            return {Character(atom->c)};
        } else if (root->type == re::ast::Type::CharacterClass) {
            auto atom = (re::ast::CharacterClass *) root;
            Bitset inst;

            if (atom->char_class_type == re::ast::CharacterClassType::Digits) {
                inst = make_range('0', '9');
            } else if (atom->char_class_type == re::ast::CharacterClassType::Word) {
                inst = make_range('a', 'z') | make_range('A', 'Z') | make_range('_', '_');
            } else if (atom->char_class_type == re::ast::CharacterClassType::Whitespace) {
                inst = make_range(' ', ' ') | make_range('\t', '\t') | make_range('\n', '\n');
            } else if (atom->char_class_type == re::ast::CharacterClassType::All) {
                inst = ~make_range('\n', '\n');
            }
            inst = atom->negate ? ~inst : inst;

            return {inst};
        } else if (root->type == re::ast::Type::Alternation) {
            auto atom = (re::ast::Alternation *) root;
            std::list<Instruction> code;

            auto lhs = compile_fragment(atom->lhs);
            auto rhs = compile_fragment(atom->rhs);

            code.emplace_back(Split(1, lhs.size() + 2));
            code.splice(code.end(), lhs);
            code.emplace_back(Jump(rhs.size() + 1));
            code.splice(code.end(), rhs);

            return code;
        } else if (root->type == re::ast::Type::Assertion) {
            auto atom = (re::ast::Assertion*) root;
            return {Assertion {atom->assertion_type}};
        } else if (root->type == re::ast::Type::Repetition) {
            auto atom = (re::ast::Repetition *) root;
            auto code = std::list<Instruction>{};

            if (atom->type == re::ast::RepetitionType::ZeroOrOne) {
                auto inner = compile_fragment(atom->inner);
                code.emplace_back(Split(1, inner.size() + 1));
                code.splice(code.end(), inner);
            } else if (atom->type == re::ast::RepetitionType::ZeroOrMore) {
                auto inner = compile_fragment(atom->inner);
                auto inner_size = inner.size();

                code.emplace_back(Split(1, inner_size + 2));
                code.splice(code.end(), inner);
                code.emplace_back(Jump(-inner_size - 1));

            } else if (atom->type == re::ast::RepetitionType::OneOrMore) {
                auto inner = compile_fragment(atom->inner);
                auto inner_size = inner.size();

                code.splice(code.end(), inner);
                code.emplace_back(Split(-inner_size, 1));
            } else {
                throw std::runtime_error("Compilation error. Unknown repetition type");
            }
            return code;
        }
    }

    std::list<Instruction> compile_fragment(AtomPointer root) {
        std::list<Instruction> code{};
        for (; root; root = root->next) {
            auto fragment = compile_atom(root);
            code.splice(code.end(), fragment);
        }
        return code;
    }

    bool match_fragment(Range<std::vector<Instruction>> program_counter, Range<std::string> data_counter) {
        while (!program_counter.empty()) {
            if (auto inst = std::get_if<Character>(&program_counter)) {
                if (!data_counter.empty() && inst->match(*data_counter)) {
                    ++program_counter;
                    ++data_counter;
                } else {
                    return false;
                }
            } else if (auto inst = std::get_if<Bitset>(&program_counter)) {
                if (!data_counter.empty() && inst->match(*data_counter)) {
                    ++program_counter;
                    ++data_counter;
                } else {
                    return false;
                }
            } else if (auto inst = std::get_if<Split>(&program_counter)) {
                bool result = match_fragment(program_counter + inst->lhs, data_counter);
                if (!result) {
                    result = match_fragment(program_counter + inst->rhs, data_counter);
                }
                return result;
            } else if (auto inst = std::get_if<Assertion>(&program_counter)) {
                if (inst->test(data_counter)) {
                    ++program_counter;
                } else {
                    return false;
                }
            } else if (auto inst = std::get_if<Jump>(&program_counter)) {
                program_counter = program_counter + inst->target;
            } else if (auto inst = std::get_if<Match>(&program_counter)) {
                return true;
            } else {
                throw std::runtime_error("Invalid instruction!");
            }
        }
        return false;
    }
}

namespace re {
    void print_bytecode(const std::vector<Instruction>& compiled) {
        for (auto& inst: compiled) {
            std::visit([](auto x) {std::cout << x << std::endl; } , inst);
        }
    }

    std::optional<std::vector<Instruction>> compile_partial(const std::string& re) {
        auto maybe_ast = parse(re);
        if (maybe_ast) {
            auto ast = *maybe_ast;

            auto compiled = compile_fragment(ast);

            // push_front works in reverse order
            compiled.push_front(Jump {-2});
            compiled.push_front(~make_range('\n', '\n'));
            compiled.push_front(Split {3, 1});

            compiled.push_back(Match {});

            deallocate(ast);

            return {{std::begin(compiled), std::end(compiled)}};
        } else {
            return std::nullopt;
        }
    }

    std::optional<std::vector<Instruction>> compile_full(const std::string& re) {
        auto maybe_ast = parse(re);
        if (maybe_ast) {
            auto ast = *maybe_ast;

            auto compiled = compile_fragment(ast);
            compiled.push_back(::Assertion {re::ast::AssertionType::EndOfString});
            compiled.push_back(Match {});

            deallocate(ast);

            return {{std::begin(compiled), std::end(compiled)}};
        } else {
            return std::nullopt;
        }
    }

    bool match(const std::vector<Instruction>& re, const std::string& s) {
        auto re_range = Range(re);
        auto s_range = Range(s);

        return match_fragment(re_range, s_range);
    }

    bool full_match(const std::string& re, const std::string& s) {
        auto maybe_compiled = compile_full(re);
        if (maybe_compiled) {
            auto compiled = *maybe_compiled;
            return match(compiled, s);
        } else {
            return false;
        }
    }

    bool partial_match(const std::string& re, const std::string& s) {
        auto maybe_compiled = compile_partial(re);
        if (maybe_compiled) {
            auto compiled = *maybe_compiled;
            return match(compiled, s);
        } else {
            return false;
        }
    }
}
#endif //REGEX_MATCHER_VM2_H
