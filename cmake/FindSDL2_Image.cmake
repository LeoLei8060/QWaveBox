# FindSDL2_image.cmake - 简化版查找SDL2_image库
#
# 该模块通过系统路径查找SDL2_image库（需确保已先找到SDL2）
#
# 定义的变量：
#  SDL2_IMAGE_FOUND        - 如果找到SDL2_image则为true
#  SDL2_IMAGE_INCLUDE_DIRS - SDL2_image头文件目录
#  SDL2_IMAGE_LIBRARIES    - SDL2_image库文件列表
#  SDL2_image::SDL2_image  - SDL2_image目标（如果支持）
#

message(STATUS "正在查找SDL2_image库...")

# 尝试使用CMake的config模式查找SDL2_image
find_package(SDL2_image CONFIG QUIET)
if(SDL2_image_FOUND)
    message(STATUS "使用CMake配置文件找到SDL2_image.")
    return()
endif()

# 设置搜索路径（与SDL2保持一致的VCPKG路径）
set(SDL2_IMAGE_SEARCH_PATHS
    "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}"
    "$ENV{VCPKG_ROOT}/installed/x64-windows"
    "$ENV{VCPKG_ROOT}/installed/x86-windows"
)

# 手动查找头文件（注意SDL_image.h的路径）
find_path(SDL2_IMAGE_INCLUDE_DIR
    NAMES SDL_image.h
    PATHS ${SDL2_IMAGE_SEARCH_PATHS}
    PATH_SUFFIXES SDL2 include/SDL2 include
)

# 手动查找库文件（注意调试版后缀）
find_library(SDL2_IMAGE_LIBRARY
    NAMES SDL2_image SDL2_imaged
    PATHS ${SDL2_IMAGE_SEARCH_PATHS}
    PATH_SUFFIXES lib
)

# 检查库是否找到
if(SDL2_IMAGE_INCLUDE_DIR AND SDL2_IMAGE_LIBRARY)
    set(SDL2_IMAGE_FOUND TRUE)

    # 设置包含目录和库
    set(SDL2_IMAGE_INCLUDE_DIRS ${SDL2_IMAGE_INCLUDE_DIR})
    set(SDL2_IMAGE_LIBRARIES ${SDL2_IMAGE_LIBRARY})

    message(STATUS "SDL2_image库已找到:")
    message(STATUS "  包含目录: ${SDL2_IMAGE_INCLUDE_DIRS}")
    message(STATUS "  库列表: ${SDL2_IMAGE_LIBRARIES}")

    # 创建导入目标并链接SDL2依赖
    if(NOT TARGET SDL2_image::SDL2_image)
        add_library(SDL2_image::SDL2_image UNKNOWN IMPORTED)
        set_target_properties(SDL2_image::SDL2_image PROPERTIES
            IMPORTED_LOCATION "${SDL2_IMAGE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_INCLUDE_DIRS}"
        )
        # 显式链接SDL2（假设SDL2目标已存在）
        if(TARGET SDL2::SDL2)
            target_link_libraries(SDL2_image::SDL2_image INTERFACE SDL2::SDL2)
        endif()
    endif()
else()
    set(SDL2_IMAGE_FOUND FALSE)
    message(WARNING "无法找到SDL2_image库")
endif()

# 处理REQUIRED和QUIET参数
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2_image
    REQUIRED_VARS SDL2_IMAGE_LIBRARY SDL2_IMAGE_INCLUDE_DIR
)
