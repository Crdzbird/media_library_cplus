cmake_minimum_required(VERSION 3.5)
project(media_library)

set(CMAKE_CXX_STANDARD 17)

include_directories(/opt/homebrew/Cellar/ffmpeg/7.1_3/include)
include_directories(/opt/homebrew/Cellar/jpeg-turbo/3.0.4/include)

link_directories(/opt/homebrew/Cellar/ffmpeg/7.1_3/lib)
link_directories(/opt/homebrew/Cellar/jpeg-turbo/3.0.4/lib)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWSCALE REQUIRED libswscale)

include_directories(${AVFORMAT_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS} ${SWSCALE_INCLUDE_DIRS})
link_directories(${AVFORMAT_LIBRARY_DIRS} ${AVCODEC_LIBRARY_DIRS} ${AVUTIL_LIBRARY_DIRS} ${SWSCALE_LIBRARY_DIRS})

#target_link_libraries(your_executable ${AVFORMAT_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVUTIL_LIBRARIES} ${SWSCALE_LIBRARIES})

add_library(media_library SHARED library.cpp)
