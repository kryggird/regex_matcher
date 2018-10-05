# Regex matching

Toy header only regex matching library. Uses a backtrackng virtual machine internally. Still a WIP.
Heavily inspired by [Russ Cox articles on re2](https://swtch.com/~rsc/regexp/).

## Compiling and example program

This library has no dependencies. You will need only CMake and a C++17 compatible compiler.
To compile an example application, go to the repository folder, and run

```bash
mkdir build
cd build
cmake ../CMakeLists.txt
```

## Api

All the functions are exposed in `interface.h`.
To match a regex to the entirety of a string, use the `full_match` function:
```c++
bool does_match = full_match("hello( world)?!", "hello!");
bool does_not_match = full_match("hello", "hello world!");
assert(does_match);
assert(!does_not_match);
```

To match in the middle of a string, use `partial_match`:
```c++
bool does_match = partial_match("hello", "hello world!");
assert(does_match);
```

Regular expressions can be compiled and reused using `compile_full` and `compile_partial`.
A compiled regex can be matched to a string with `match`:
```c++
auto re = std::string {"hello"};
auto s = std::string {"hello world!"};

auto compiled = compile_partial(re, s);
assert(match(compiled, s) == partial_match(re, s));
```

## Example application usage

The example application provides grep-like functionality:

```console
$ printf 'hello\nworld'
hello
world
$ printf 'hello\nworld' | ./regex_matcher --match 'wa?'
world
```

## Todo

 - Support bracketed character classes
 - Return match groups
 - Thompson algorithm (rather than backtracking)
 - Unicode support 