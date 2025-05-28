cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(GameLib STATIC
	src/model.h 
	src/model.cpp 
	src/loot_generator.h 
	src/loot_generator.cpp 
	src/tagged.h
	src/collision_detector.h
	src/collision_detector.cpp
)

target_link_libraries(GameLib PUBLIC CONAN_PKG::boost Threads::Threads)

add_executable(game_server
	src/ticker.h
	src/model_serialization.h 
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/extra_data.h
        src/extra_data.cpp
	src/db.h
	src/db.cpp
	src/stat_saver_impl.h
)

add_executable(game_server_tests
    tests/tests.cpp
)

target_link_libraries(game_server PRIVATE GameLib CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 GameLib)