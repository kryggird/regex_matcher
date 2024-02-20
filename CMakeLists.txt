cmake_minimum_required(VERSION 3.12)
project(regex_matcher)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_BUILD_TYPE Debug)

add_executable(regex_matcher src/ast.h src/tests.cpp src/parser.h src/vm.h src/interface.h src/interface.h)
