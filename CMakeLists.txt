cmake_minimum_required(VERSION 3.12)
project(alex)

set(CMAKE_CXX_STANDARD 14)

# Define the macro ‘DEBUG' in the debug mode
if(CMAKE_BUILD_TYPE STREQUAL Debug)        
    ADD_DEFINITIONS(-DDEBUG)               
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "/O2 /arch:AVX2 /W1 /EHsc")
    message(1)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_CXX_FLAGS "-O3 -xHost")
    message(2)
else()
    # clang and gcc
    set(CMAKE_CXX_FLAGS "-march=native -Wall -Wextra")
    message(3)
endif()


include_directories(src/core)
include_directories(attack)

#header files for ortools lib
include_directories(/home/ubuntu/data/or-tools/include)
link_directories(/home/ubuntu/data/or-tools/lib)



#add_executable(whitebox src/benchmark/whitebox.cpp)
#add_executable(noattack src/benchmark/noattack.cpp)
#add_executable(blackbox src/benchmark/blackbox.cpp)
#add_executable(epoch src/benchmark/epoch.cpp)
#add_executable(lis src/benchmark/lis.cpp)
#add_executable(example src/benchmark/example.cpp)
#add_executable(lis_comp_root src/benchmark/lis_comp_root.cpp)
#add_executable(benchmark src/benchmark/main.cpp)
#add_executable(ratt src/benchmark/root_attack.cpp)
add_executable(perf_aca_whitebox src/benchmark/perf_aca_whitebox.cpp)
#add_executable(perf_aca_graybox src/benchmark/perf_aca_graybox.cpp)
#add_executable(perf_aca_blackbox src/benchmark/perf_aca_blackbox.cpp)
#add_executable(space_aca_whitbox src/benchmark/space_aca_whitbox.cpp)
#add_executable(space_aca_blackbox src/benchmark/space_aca_blackbox.cpp)


#target_link_libraries(benchmark -lortools)
#target_link_libraries(whitebox -lortools)
#target_link_libraries(noattack -lortools)
#target_link_libraries(blackbox -lortools)
#target_link_libraries(epoch -lortools)
#target_link_libraries(lis -lortools)
#target_link_libraries(example -lortools)
#target_link_libraries(lis_comp_root -lortools)
#target_link_libraries(epoch -lortools)
#target_link_libraries(blackbox -lortools)
#target_link_libraries(lis_setting -lortools)
#target_link_libraries(ratt -lortools)
target_link_libraries(perf_aca_whitebox -lortools)
#target_link_libraries(perf_aca_graybox -lortools)
#target_link_libraries(perf_aca_blackbox -lortools)
#target_link_libraries(space_aca_whitbox -lortools)
#target_link_libraries(space_aca_blackbox -lortools)
#target_link_libraries(budget -lortools)


set(DOCTEST_DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/doctest)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(DOWNLOAD
    https://raw.githubusercontent.com/onqtam/doctest/2.4.6/doctest/doctest.h
    ${DOCTEST_DOWNLOAD_DIR}/doctest.h
    EXPECTED_HASH SHA1=40728d2bed7f074e80cb095844820114e7d16ce0
)

add_executable(test_alex test/unittest_main.cpp)
target_link_libraries(test_alex -lortools)
target_include_directories(test_alex PRIVATE ${DOCTEST_DOWNLOAD_DIR})

enable_testing()
add_test(test_alex test_alex)
