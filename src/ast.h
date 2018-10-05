#ifndef REGEX_MATCHER_AST_H
#define REGEX_MATCHER_AST_H

namespace re::ast {
    enum class Type {
        Assertion, Character, CharacterClass, Alternation, Repetition
    };
    enum class RepetitionType {
        ZeroOrOne, ZeroOrMore, OneOrMore
    };
    enum class CharacterClassType {
        All, Word, Digits, Whitespace
    };
    enum class AssertionType {
        BeginOfString, EndOfString
    };

    struct Atom;
    using AtomPointer = Atom *;

    struct Atom {
        Type type;
        AtomPointer next;
    };

    struct Character : Atom {
        Character(char c_) : Atom{Type::Character, nullptr}, c{c_} {};
        char c;
    };

    struct CharacterClass : Atom {
        CharacterClass(CharacterClassType type_, bool negate_ = false): Atom{Type::CharacterClass, nullptr},
                                                                        char_class_type {type_}, negate {negate_} {};

        CharacterClassType char_class_type;
        bool negate;
    };

    struct Repetition : Atom {
        Repetition(RepetitionType type_, AtomPointer inner_) : Atom{Type::Repetition, nullptr}, type{type_},
                                                               inner{inner_} {};
        RepetitionType type;
        AtomPointer inner;
    };

    struct Alternation : Atom {
        Alternation(AtomPointer lhs_, AtomPointer rhs_) : Atom{Type::Alternation, nullptr}, lhs{lhs_}, rhs{rhs_} {};
        AtomPointer lhs;
        AtomPointer rhs;
    };

    struct Assertion : Atom {
        Assertion(AssertionType assertion_type_): Atom {Type::Assertion}, assertion_type {assertion_type_} {};
        AssertionType assertion_type;
    };

    // TODO switch to unique_ptr
    void deallocate(AtomPointer p) {
        if (p == nullptr) {
            return;
        } else if (p->type == Type::Character || p->type == Type::CharacterClass) {
            deallocate(p->next);
            delete p;
        } else if (p->type == Type::Repetition) {
            auto casted = (Repetition*) p;
            deallocate(casted->inner);
            deallocate(casted->next);
            delete p;
        } else if (p->type == Type::Alternation) {
            auto casted = (Alternation*) p;
            deallocate(casted->lhs);
            deallocate(casted->rhs);
            deallocate(casted->next);
            delete p;
        }
    }
}
#endif //REGEX_MATCHER_AST_H
