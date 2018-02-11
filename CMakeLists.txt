cmake_minimum_required(VERSION 3.2)
project(tsl)

set(CMAKE_CXX_STANDARD 14)

set(TSL_INCLUDES include/tsl/tsl.h)

function(make_test ARG_TEST_NAME)
    add_executable(${ARG_TEST_NAME} ${ARGN} ${TSL_INCLUDES})
    target_link_libraries(${ARG_TEST_NAME} ${PROJECT_NAME}_test_common)
    add_test(NAME ${ARG_TEST_NAME} COMMAND ${ARG_TEST_NAME})
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        target_compile_options(${ARG_TEST_NAME} PRIVATE -Wall -Wextra -Wunreachable-code)
    endif ()
endfunction(make_test)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/third_party/catch2.1.2)
add_library(${PROJECT_NAME}_test_common STATIC tests/catch_main.cpp)

enable_testing()
make_test(basic tests/basic.cpp)



