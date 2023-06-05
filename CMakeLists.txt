cmake_minimum_required(VERSION 3.21)
project(Branch_Predictor_simulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(Branch_Predictor_simulator bp_main.cpp bp_api.h bp.cpp)
