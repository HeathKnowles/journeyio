cmake_minimum_required(VERSION 3.20)
project(journeyio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)
find_package(httplib REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(flatbuffers REQUIRED)

find_program(FLATC_EXECUTABLE
    NAMES flatc
    HINTS "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/tools/flatbuffers"
    REQUIRED
)

set(FBS_SCHEMA ${CMAKE_CURRENT_SOURCE_DIR}/schemas/api.fbs)
set(FBS_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_generated.h)

add_custom_command(
    OUTPUT ${FBS_OUTPUT}
    COMMAND ${FLATC_EXECUTABLE} --cpp --scoped-enums -o ${CMAKE_CURRENT_BINARY_DIR} ${FBS_SCHEMA}
    DEPENDS ${FBS_SCHEMA}
    COMMENT "Generating FlatBuffers C++ headers from api.fbs"
)

add_executable(journeyio
    src/main.cpp
    src/data_loader.cpp
    src/api.cpp
    src/flatbuf_serializer.cpp
    ${FBS_OUTPUT}
)

add_custom_target(flatbuffers_gen DEPENDS ${FBS_OUTPUT})
add_dependencies(journeyio flatbuffers_gen)

target_link_libraries(journeyio PRIVATE
    "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>"
    "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Parquet::parquet_static,Parquet::parquet_shared>"
    httplib::httplib
    nlohmann_json::nlohmann_json
    flatbuffers::flatbuffers
)

target_include_directories(journeyio PRIVATE
    src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${FLATBUFFERS_INCLUDE_DIR}
)
