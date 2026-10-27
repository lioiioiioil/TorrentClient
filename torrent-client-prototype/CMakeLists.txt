cmake_minimum_required(VERSION 3.20)
project(torrent-client-prototype LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CLIENT "Build the network client (downloads CPR during configuration)" ON)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
include(CTest)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

function(enable_project_sanitizers target)
    if(ENABLE_SANITIZERS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        else()
            message(FATAL_ERROR "ENABLE_SANITIZERS requires GCC or Clang")
        endif()
    endif()
endfunction()

if(BUILD_CLIENT)
    include(FetchContent)
    set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use the installed libcurl" FORCE)
    set(CPR_BUILD_TESTS OFF CACHE BOOL "Do not build CPR tests" FORCE)
    FetchContent_Declare(cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 0817715923c9705e68994eb52ef9df3f6845beba)
    FetchContent_MakeAvailable(cpr)
    add_executable(${PROJECT_NAME}
        main.cpp peer_connect.cpp tcp_connect.cpp torrent_tracker.cpp
        torrent_file.cpp bencode.cpp message.cpp byte_tools.cpp
        piece_storage.cpp piece.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto cpr::cpr Threads::Threads)
    enable_project_sanitizers(${PROJECT_NAME})
endif()

if(BUILD_TESTING)
    add_executable(core_tests
        tests/core_tests.cpp bencode.cpp message.cpp byte_tools.cpp
        piece.cpp piece_storage.cpp)
    target_include_directories(core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(core_tests PRIVATE OpenSSL::Crypto Threads::Threads)
    enable_project_sanitizers(core_tests)
    add_test(NAME core_tests COMMAND core_tests)
    set_tests_properties(core_tests PROPERTIES TIMEOUT 30)
endif()

