cmake_minimum_required(VERSION 3.10)
project(car_rental_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Add executable
add_executable(car_rental_backend src/main.cpp)

# Include directories
target_include_directories(car_rental_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps
    ${Boost_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(car_rental_backend PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    ${Boost_LIBRARIES}
    -pthread
)
