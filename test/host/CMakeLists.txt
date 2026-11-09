# Host-side unit tests for components/puck_core (no ESP-IDF needed).
#   cmake -S test/host -B build-host -DCOVERAGE=ON && cmake --build build-host
#   ctest --test-dir build-host --output-on-failure
# or simply: tools/test.sh
cmake_minimum_required(VERSION 3.16)
project(openpuck_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(COVERAGE "Instrument puck_core for coverage" OFF)
option(SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" ON)

include(FetchContent)
FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(unity)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../components/puck_core puck_core)

set(WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror)
target_compile_options(puck_core PRIVATE ${WARNINGS})
if(COVERAGE)
    target_compile_options(puck_core PRIVATE --coverage -O0 -g)
    target_link_options(puck_core PUBLIC --coverage)
endif()
if(SANITIZE)
    set(SAN -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(puck_core PRIVATE ${SAN})
    target_link_options(puck_core PUBLIC ${SAN})
endif()

enable_testing()
foreach(name game save_policy state_codec touch_input timer history settings battery color number)
    add_executable(test_${name} test_${name}.c)
    target_link_libraries(test_${name} PRIVATE puck_core unity)
    target_compile_options(test_${name} PRIVATE -Wall -Wextra -Werror)
    if(SANITIZE)
        target_compile_options(test_${name} PRIVATE ${SAN})
    endif()
    add_test(NAME ${name} COMMAND test_${name})
endforeach()
