cmake_minimum_required(VERSION 2.8)
project(FrameExtractor)
add_definitions(-Dinline=_inline -Dsnprintf=_snprintf)
include_directories(
${PROJECT_SOURCE_DIR}/
${PROJECT_SOURCE_DIR}/include
)
set (FFMPEG_LIBS
${PROJECT_SOURCE_DIR}/lib/avcodec.lib
${PROJECT_SOURCE_DIR}/lib/avformat.lib
${PROJECT_SOURCE_DIR}/lib/avutil.lib
${PROJECT_SOURCE_DIR}/lib/swscale.lib
)

add_executable(FrameExtractor Encoder.h Encoder.cpp FrameExtractor.cpp)
target_link_libraries(FrameExtractor ${FFMPEG_LIBS})