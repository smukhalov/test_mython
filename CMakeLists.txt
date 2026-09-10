cmake_minimum_required(VERSION 3.25)
project(test_mython)

set(CMAKE_CXX_STANDARD 20)

add_executable(test_mython main.cpp
        comparators.cpp
        comparators.h
        lexer.cpp
        lexer.h
        object.cpp
        object.h
        object_test.cpp
        object_holder.h
        object_holder.cpp
        object_holder_test.cpp
        parse.h
        parse.cpp
        parse_test.cpp
        statement_test.cpp
        statement.h
        statement.cpp
        test_runner.h
)
