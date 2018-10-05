#ifndef REGEX_MATCHER_PARSER_H
#define REGEX_MATCHER_PARSER_H

#include <cctype>
#include <iostream>
#include <optional>
#include <string>

#include "ast.h"

namespace {
    using namespace re::ast;

    std::optional<AtomPointer> parse_regex(std::string::const_iterator &current, std::string::const_iterator end);
    std::optional<AtomPointer> parse_alternation(std::string::const_iterator &current, std::string::const_iterator end);
    std::optional<AtomPointer> parse_concatenation(std::string::const_iterator &current, std::string::const_iterator end);
    std::optional<AtomPointer> parse_repetition(std::string::const_iterator &current, std::string::const_iterator end);
    std::optional<AtomPointer> parse_atom(std::string::const_iterator &current, std::string::const_iterator end);
    std::optional<AtomPointer> parse_raw_character(std::string::const_iterator &current, std::string::const_iterator end);
    std::optional<AtomPointer> parse_character_class(std::string::const_iterator &current, std::string::const_iterator end);

    bool is_reserved_character(char c) {
        return (c == '\\' || c == '|' || c == '(' || c == ')' || c == '*' || c == '+' || c == '?' || c == '.' ||
                c == '^' || c == '$');
    }

    bool consume_constant(char c, std::string::const_iterator &current, std::string::const_iterator end) {
        if (current != end && *current == c) {
            ++current;
            return true;
        } else {
            return false;
        }
    }

    std::optional<AtomPointer> parse_regex(std::string::const_iterator &current, std::string::const_iterator end) {
        auto backup_current = current;
        auto ast = parse_alternation(current, end);
        if (!ast.has_value()) {
            current = backup_current;
            ast = parse_concatenation(current, end);
        }
        return ast;
    }

    std::optional<AtomPointer>
    parse_alternation(std::string::const_iterator &current, std::string::const_iterator end) {
        auto backup_current = current;
        auto lhs = parse_concatenation(current, end);
        if (lhs.has_value() && consume_constant('|', current, end)) {
            auto rhs = parse_regex(current, end);
            if (rhs.has_value()) {
                Atom *unboxed_lhs = *lhs;
                Atom *unboxed_rhs = *rhs;
                return new Alternation{unboxed_lhs, unboxed_rhs};
            }
        }
        current = backup_current;
        return std::nullopt;
    }

    std::optional<AtomPointer>
    parse_concatenation(std::string::const_iterator &current, std::string::const_iterator end) {
        auto ast = parse_repetition(current, end);
        if (ast.has_value()) {
            auto backup_current = current;
            auto tail = parse_concatenation(current, end);
            if (tail.has_value()) {
                Atom *unboxed = *ast;
                unboxed->next = *tail;
            } else {
                current = backup_current;
            }
        }
        return ast;
    }

    std::optional<AtomPointer> parse_repetition(std::string::const_iterator &current, std::string::const_iterator end) {
        auto ast = parse_atom(current, end);
        if (ast.has_value() && consume_constant('?', current, end)) {
            auto unboxed = *ast;
            return new Repetition{RepetitionType::ZeroOrOne, unboxed};
        } else if (ast.has_value() && consume_constant('*', current, end)) {
            auto unboxed = *ast;
            return new Repetition{RepetitionType::ZeroOrMore, unboxed};
        } else if (ast.has_value() && consume_constant('+', current, end)) {
            auto unboxed = *ast;
            return new Repetition{RepetitionType::OneOrMore, unboxed};
        }
        return ast;
    }

    std::optional<AtomPointer> parse_atom(std::string::const_iterator &current, std::string::const_iterator end) {
        if (current != end) {
            if (consume_constant('(', current, end)) {
                auto ast = parse_regex(current, end);
                bool is_terminated = consume_constant(')', current, end);
                return is_terminated ? ast : std::nullopt;
            } else if (consume_constant('\\', current, end)) {
                if (std::isalpha(*current)) {
                    auto ast = parse_character_class(current, end);
                    return ast;
                } else {
                    auto ast = parse_raw_character(current, end);
                    return ast;
                }
            } else if (!is_reserved_character(*current)) {
                auto ast = parse_raw_character(current, end);
                return ast;
            } else if (consume_constant('.', current, end)) {
                return new CharacterClass {CharacterClassType::All, false};
            } else if (consume_constant('^', current, end)) {
                return new Assertion(AssertionType::BeginOfString);
            } else if (consume_constant('$', current, end)) {
                return new Assertion(AssertionType::EndOfString);
            }
        }
        return std::nullopt;
    }

    std::optional<AtomPointer>
    parse_raw_character(std::string::const_iterator &current, std::string::const_iterator end) {
        if (current != end) {
            auto ast = new Character{*current};
            current++;
            return ast;
        } else {
            return std::nullopt;
        }
    }

    std::optional<AtomPointer>
    parse_character_class(std::string::const_iterator &current, std::string::const_iterator end) {
        if (current != end) {
            char c = *current;
            ++current;
            if (c == 'd') {
                return new CharacterClass(CharacterClassType::Digits, false);
            } else if (c == 'D') {
                return new CharacterClass(CharacterClassType::Digits, true);
            } else if (c == 's') {
                return new CharacterClass(CharacterClassType::Whitespace, false);
            } else if (c == 'S') {
                return new CharacterClass(CharacterClassType::Whitespace, true);
            } else if (c == 'w') {
                return new CharacterClass(CharacterClassType::Word, false);
            } else if (c == 'W') {
                return new CharacterClass(CharacterClassType::Word, true);
            } else {
                --current;
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
    }

}

namespace re {
    std::optional<AtomPointer> parse(const std::string& re) {
        auto begin = re.cbegin();
        auto end = re.cend();
        return parse_regex(begin, end);
    }
}

#endif //REGEX_MATCHER_PARSER_H
