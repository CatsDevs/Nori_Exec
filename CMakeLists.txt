cmake_minimum_required(VERSION 3.20)
project(NoriExec LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(nori_exec src/main.cpp)
target_compile_features(nori_exec PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(nori_exec PRIVATE /W4 /permissive-)
else()
  target_compile_options(nori_exec PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME nori_exec_self_test COMMAND nori_exec --test)
