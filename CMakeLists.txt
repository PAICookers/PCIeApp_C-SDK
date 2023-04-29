cmake_minimum_required(VERSION 3.12)

project(PCIeApp)
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)
aux_source_directory(./src src)

add_executable(pcie_app main.c ${src})