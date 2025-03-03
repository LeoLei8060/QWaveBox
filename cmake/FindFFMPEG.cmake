# FindFFMPEG.cmake - 简化版查找FFmpeg库
# 
# 该模块主要通过VCPKG或系统路径查找FFmpeg库
#
# 定义的变量：
#  FFMPEG_FOUND        - 如果找到FFmpeg则为true
#  FFMPEG_INCLUDE_DIRS - FFmpeg头文件目录
#  FFMPEG_LIBRARIES    - FFmpeg库文件列表
#

message(STATUS "正在查找FFmpeg库...")

# 尝试通过多种方式查找FFmpeg
set(FFMPEG_SEARCH_PATHS
    # VCPKG路径
    "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}"
    
    # 常见系统路径
    "/usr/local"
    "/usr"
    "C:/Program Files/FFmpeg"
    "C:/FFmpeg"
)

# 如果未定义目标平台，尝试自动确定
if(NOT DEFINED VCPKG_TARGET_TRIPLET AND DEFINED ENV{VCPKG_ROOT})
    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(VCPKG_TARGET_TRIPLET "x64-windows")
        else()
            set(VCPKG_TARGET_TRIPLET "x86-windows")
        endif()
        list(PREPEND FFMPEG_SEARCH_PATHS "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
    endif()
endif()

# 查找FFmpeg头文件目录
find_path(FFMPEG_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    PATHS ${FFMPEG_SEARCH_PATHS}
    PATH_SUFFIXES include
)

# 查找必要的FFmpeg库
find_library(AVCODEC_LIBRARY 
    NAMES avcodec avcodec-59 avcodec-60
    PATHS ${FFMPEG_SEARCH_PATHS}
    PATH_SUFFIXES lib
)
    
find_library(AVFORMAT_LIBRARY 
    NAMES avformat avformat-59 avformat-60
    PATHS ${FFMPEG_SEARCH_PATHS}
    PATH_SUFFIXES lib
)
    
find_library(AVUTIL_LIBRARY 
    NAMES avutil avutil-57 avutil-58
    PATHS ${FFMPEG_SEARCH_PATHS}
    PATH_SUFFIXES lib
)
    
find_library(SWSCALE_LIBRARY 
    NAMES swscale swscale-6 swscale-7
    PATHS ${FFMPEG_SEARCH_PATHS}
    PATH_SUFFIXES lib
)
    
find_library(SWRESAMPLE_LIBRARY 
    NAMES swresample swresample-4 swresample-5
    PATHS ${FFMPEG_SEARCH_PATHS}
    PATH_SUFFIXES lib
)

# 检查所有库是否找到
if(FFMPEG_INCLUDE_DIR AND AVCODEC_LIBRARY AND AVFORMAT_LIBRARY AND AVUTIL_LIBRARY AND SWSCALE_LIBRARY AND SWRESAMPLE_LIBRARY)
    set(FFMPEG_FOUND TRUE)
    
    # 设置包含目录和库
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
    set(FFMPEG_LIBRARIES
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${SWRESAMPLE_LIBRARY})
    
    message(STATUS "FFmpeg库已找到:")
    message(STATUS "  包含目录: ${FFMPEG_INCLUDE_DIRS}")
    message(STATUS "  库列表: ${FFMPEG_LIBRARIES}")
else()
    set(FFMPEG_FOUND FALSE)
    message(WARNING "无法找到部分或全部FFmpeg库")
endif()

# 处理REQUIRED和QUIET参数
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFMPEG
    REQUIRED_VARS FFMPEG_INCLUDE_DIR AVCODEC_LIBRARY AVFORMAT_LIBRARY AVUTIL_LIBRARY SWSCALE_LIBRARY SWRESAMPLE_LIBRARY
)
