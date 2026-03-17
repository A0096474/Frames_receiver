cmake_minimum_required(VERSION 3.16)
project(frames_receiver_cpp)

set(CMAKE_CXX_STANDARD 17)

# Use bundled single-header nlohmann JSON for the POC (third_party/json.hpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Ensure a reasonable _WIN32_WINNT is defined for Boost/Asio on Windows
add_definitions(-D_WIN32_WINNT=0x0601)

# Optional: build a lightweight WebSocket POC using Boost.Beast
find_package(Boost COMPONENTS system REQUIRED)

# User may provide libdatachannel via vcpkg or system install. If found, build the
# full `receiver` target; otherwise skip it and only build `receiver_ws`.
find_path(LIBDATACHANNEL_INCLUDE_DIR rtc/rtc.hpp)
find_library(LIBDATACHANNEL_LIB rtc)

if(LIBDATACHANNEL_INCLUDE_DIR AND LIBDATACHANNEL_LIB)
  include_directories(${LIBDATACHANNEL_INCLUDE_DIR})
  add_executable(receiver main.cpp)
  target_link_libraries(receiver PRIVATE ${LIBDATACHANNEL_LIB})
else()
  message(STATUS "libdatachannel not found — skipping 'receiver' target. Only building POC 'receiver_ws'.")
endif()

# Build a standalone WebSocket receiver POC (does not require libdatachannel)
add_executable(ws_receiver ws_main.cpp)
target_link_libraries(ws_receiver PRIVATE Boost::system)
