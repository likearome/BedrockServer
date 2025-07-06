# 최소 CMake 버전 요구사항
cmake_minimum_required(VERSION 3.16)

# 프로젝트 이름과 C++ 버전 설정
project(JJWSvr VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# "JJWSvr" 라는 이름의 실행 파일을 만듦
add_executable(JJWSvr src/main.cpp)