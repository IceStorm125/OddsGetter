cmake_minimum_required(VERSION 3.22)

project(OddsGetter LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include("${CMAKE_CURRENT_BINARY_DIR}/Release/generators/conan_toolchain.cmake")


# Check if the command was successful
if(CONAN_RESULT)
    message(FATAL_ERROR "Failed to run conan install command")
endif()


find_package(Boost REQUIRED)
find_package(Poco REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(CURL REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp )
target_link_libraries(${PROJECT_NAME} boost::boost nlohmann_json::nlohmann_json spdlog::spdlog Poco::Poco CURL::libcurl)  

