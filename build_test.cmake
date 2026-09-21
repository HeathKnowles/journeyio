cmake_minimum_required(VERSION 3.20)
project(test_schema LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)
add_executable(test_schema src/test_schema.cpp)
target_link_libraries(test_schema PRIVATE
    "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>"
    "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Parquet::parquet_static,Parquet::parquet_shared>"
)
