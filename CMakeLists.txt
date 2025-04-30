# root CMakeLists.txt

cmake_minimum_required(VERSION 3.31.6)
project(reverse_proxy VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output binaries to a common directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
include_directories(/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1)
include_directories(/Library/Developer/CommandLineTools/usr/include) 

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-w)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Update jsoncpp library path
find_library(JSONCPP_LIB jsoncpp HINTS /opt/homebrew/opt/jsoncpp/lib)
 set(ZLIB_ROOT /opt/homebrew/opt/zlib)
    set(ZLIB_INCLUDE_DIR ${ZLIB_ROOT}/include)
    set(ZLIB_LIBRARY ${ZLIB_ROOT}/lib/libz.dylib)
find_package(Boost REQUIRED)
find_package(ZLIB REQUIRED)
# Uncomment and find OpenSSL
find_package(OpenSSL REQUIRED)
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found")
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
     /opt/homebrew/opt/jsoncpp/include
     ${Boost_INCLUDE_DIRS}
)

# Create list of source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Find and link zlib
find_package(ZLIB REQUIRED)
if(NOT ZLIB_FOUND)
    message(FATAL_ERROR "zlib not found")
endif()

# Find and link hiredis
find_library(HIREDIS_LIB hiredis HINTS /opt/homebrew/lib REQUIRED)
if(NOT HIREDIS_LIB)
    message(FATAL_ERROR "hiredis library not found")
endif()

# Ensure jsoncpp is linked
find_library(JSONCPP_LIB jsoncpp HINTS /opt/homebrew/opt/jsoncpp/lib REQUIRED)
if(NOT JSONCPP_LIB)
    message(FATAL_ERROR "jsoncpp library not found")
endif()

# Find and link libcurl
find_package(CURL REQUIRED)
if(NOT CURL_FOUND)
    message(FATAL_ERROR "libcurl not found")
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Threads::Threads
    ${JSONCPP_LIB}  # jsoncpp library
    ${HIREDIS_LIB}  # hiredis library
    ZLIB::ZLIB      # zlib library
    OpenSSL::SSL    # OpenSSL SSL library
    OpenSSL::Crypto # OpenSSL Crypto library
    CURL::libcurl   # libcurl library
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Add tests subdirectory if tests exist
# Add tests directory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    enable_testing()
    add_executable(test_config tests/test_config.cpp src/config/Config.cpp)
    target_include_directories(test_config PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(test_config PRIVATE
        Threads::Threads
        ${JSONCPP_LIB}  # jsoncpp library
        ${HIREDIS_LIB}  # hiredis library
        ZLIB::ZLIB      # zlib library
        OpenSSL::SSL    # OpenSSL SSL library
        OpenSSL::Crypto # OpenSSL Crypto library
        CURL::libcurl   # libcurl library
    )
    add_test(NAME ConfigTests COMMAND test_config)
endif()

# Create a package
include(CPack)