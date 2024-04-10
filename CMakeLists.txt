cmake_minimum_required(VERSION 3.28)
project(media_library)

set(CMAKE_CXX_STANDARD 17)


include_directories(/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/include)

link_directories(/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/lib)

add_library(media_library SHARED library.cpp)
