# FindSDL2.cmake - 简化版查找SDL2库
# 
# 该模块主要通过系统路径查找SDL2库
#
# 定义的变量：
#  SDL2_FOUND        - 如果找到SDL2则为true
#  SDL2_INCLUDE_DIRS - SDL2头文件目录
#  SDL2_LIBRARIES    - SDL2库文件列表
#  SDL2::SDL2        - SDL2目标（如果支持）
#

message(STATUS "正在查找SDL2库...")

# 尝试使用CMake的config模式查找SDL2
find_package(SDL2 CONFIG QUIET)
if(SDL2_FOUND)
    message(STATUS "使用CMake配置文件找到SDL2.")
    return()
endif()

# 设置搜索路径
set(SDL2_SEARCH_PATHS
    # VCPKG路径
    "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}"
    "$ENV{VCPKG_ROOT}/installed/x64-windows"
    "$ENV{VCPKG_ROOT}/installed/x86-windows"
)

# 通过find_path和find_library手动查找
find_path(SDL2_INCLUDE_DIR
    NAMES SDL.h
    PATHS ${SDL2_SEARCH_PATHS}
    PATH_SUFFIXES SDL2 include/SDL2 include
)

find_library(SDL2_LIBRARY
    NAMES SDL2 SDL2d
    PATHS ${SDL2_SEARCH_PATHS}
    PATH_SUFFIXES lib
)

# 检查库是否找到
if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
    set(SDL2_FOUND TRUE)
    
    # 设置包含目录和库
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
    set(SDL2_LIBRARIES ${SDL2_LIBRARY})
    
    message(STATUS "SDL2库已找到:")
    message(STATUS "  包含目录: ${SDL2_INCLUDE_DIRS}")
    message(STATUS "  库列表: ${SDL2_LIBRARIES}")
    
    # 创建导入目标
    if(NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 UNKNOWN IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            IMPORTED_LOCATION "${SDL2_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}")
    endif()
else()
    set(SDL2_FOUND FALSE)
    message(WARNING "无法找到SDL2库")
endif()

# 处理REQUIRED和QUIET参数
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2
    REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR
)
