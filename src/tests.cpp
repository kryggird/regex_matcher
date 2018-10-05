#include <iomanip>
#include <iostream>
#include <string>

#include "interface.h"

using namespace re;

void print_helper(const std::string& first, const std::string& second, const std::string& third) {
    std::cout << std::setw(20) << first   << " | "
              << std::setw(20) << second  << " | "
              << std::setw(20) << third << std::endl;
}

template <typename P>
void test_templated(const std::string& re, const std::string& s, bool expected, P predicate) {
    using namespace re;

    bool result = predicate(re, s);
    print_helper("/" + re + "/", "\"" + s + "\"", result == expected ? "Success!" : "Error!");
}

void test_full_match(const std::string &re, const std::string &s, bool expected) {
    test_templated(re, s, expected, full_match);
}

void test_partial_match(const std::string &re, const std::string &s, bool expected) {
    test_templated(re, s, expected, partial_match);
}

void print_usage() {
    std::cout << "regex_matcher [--help | --tests | --match <re> | --bytecode <re> ]" << std::endl;
}

void run_tests() {
    std::cout << "Match from beginning of the string" << std::endl;

    print_helper("/Regex/", "Test string", "Test result");
    test_full_match("\\d\\d", "12", true);
    test_full_match("\\d\\D", "12", false);
    test_full_match("\\w\\W", "a,", true);

    test_full_match("hello( world)?", "hello", true);
    test_full_match("hello( world)?!", "hello!", true);
    test_full_match("hello( world)?", "hello world!", false);

    test_full_match("abc(ff|f)g", "abcfffg", false);
    test_full_match("ab(ff|f)g", "abcfffg", false);
    test_full_match("a*", "aaaa", true);
    test_full_match("a+", "aaaa", true);
    test_full_match("abc(f+|g)e", "abcffffffe", true);
    test_full_match("abc(f+|g)e", "abcge", true);
    test_full_match("abc(f+|g)e", "abcffffffge", false);
    test_full_match("ba*", "baaaa", true);
    test_full_match("a*", "", true);
    //test_match_from_beginning("", "", true);
    //test_match_from_beginning("", "kjc", true);

    std::cout << std::endl << "Match inside string" << std::endl;
    print_helper("/Regex/", "Test string", "Test result");
    test_partial_match("\\d+", "abc 12 sxk", true);
    test_partial_match("\\s+", "abc 12 sxk", true);
    test_partial_match("\\W", "abc_efg", false);
    test_partial_match("\\W", "abc efg", true);
    test_partial_match("..a", "a__a", true);
    test_partial_match(".*", "xyx", true);
    test_partial_match(".+bc", "bc", false);

    test_partial_match(".+b$", "aaaabc", false);
    test_partial_match("^abc$", "abc", true);
    test_partial_match("hello( world)?", "hello world!", true);
}

void match_stdin(const std::string& re) {
    auto maybe_compiled = re::compile_partial(re);

    if (maybe_compiled) {
        auto compiled = *maybe_compiled;
        for (std::string line; std::getline(std::cin, line);) {
            if (re::match(compiled, line)) {
                std::cout << line << std::endl;
            }
        }
    } else {
        std::cerr << "Invalid regex. Aborting." << std::endl;
    }
}

void print_bytecode(const std::string& re) {
    auto maybe_compiled = re::compile_partial(re);

    if (maybe_compiled) {
        auto compiled = *maybe_compiled;
        re::print_bytecode(compiled);
    } else {
        std::cerr << "Invalid regex. Aborting." << std::endl;
    }
}


int main(int argc, char *argv[]) {

    if (argc == 2 && strcmp(argv[1], "--tests") == 0) {
        run_tests();
    } else if (argc == 3 && strcmp(argv[1], "--match") == 0) {
        std::string re {argv[2]};
        match_stdin(re);
    } else if (argc == 3 && strcmp(argv[1], "--bytecode") == 0) {
        std::string re {argv[2]};
        print_bytecode(re);
    } else {
        print_usage();
    }

    return 0;
}