cmake_minimum_required(VERSION 3.16)
project(QWaveBox VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置自动化处理Qt特性
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# 设置CMake模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 查找Qt包
find_package(Qt5 COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets REQUIRED)

# 查找FFmpeg包
find_package(FFmpeg REQUIRED)
if(FFMPEG_FOUND)
    include_directories(${FFMPEG_INCLUDE_DIRS})
endif()

# 查找SDL2包
find_package(SDL2 REQUIRED)
if(SDL2_image_FOUND)
    include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
endif()

set(GUI_SOURCE_DIR src/gui)
set(UTILS_SOURCE_DIR src/utils)
set(CORE_SOURCCE_DIR src/core)

set(GUI_SOURCES
    src/gui/titlebar.cpp
    src/gui/mainwidget.cpp
    src/gui/videowidget.cpp
    src/gui/playlistwidget.cpp
    src/gui/sdlwidget.cpp
)
set(GUI_HEADERS
    src/gui/titlebar.h
    src/gui/mainwidget.h
    src/gui/videowidget.h
    src/gui/playlistwidget.h
    src/gui/sdlwidget.h
)
set(GUI_FORMS
    src/gui/titlebar.ui
    src/gui/mainwidget.ui
    src/gui/videowidget.ui
    src/gui/playlistwidget.ui
)

set(UTILS_SOURCES
    src/utils/fontmanager.cpp
)
set(UTILS_HEADERS
    src/utils/fontmanager.h
)

set(CORE_SOURCES
    src/core/appcontext.cpp
    src/core/application.cpp
)
set(CORE_HEADERS
    src/core/appcontext.h
    src/core/constants.h
    src/core/application.h
)

# 项目源文件
set(SOURCES
    ${GUI_SOURCES}
    ${UTILS_SOURCES}
    ${CORE_SOURCES}
    src/main.cpp
    src/playlistmanager.cpp
    src/videoplayer.cpp
    src/mediacontrolwidget.cpp
)

# 项目头文件
set(HEADERS
    ${GUI_HEADERS}
    ${UTILS_HEADERS}
    ${CORE_HEADERS}
    src/playlistmanager.h
    src/videoplayer.h
    src/mediacontrolwidget.h
)

# UI文件
set(UI_FILES
    ${GUI_FORMS}
)

# 资源文件
set(RESOURCES
    src/resources.qrc
)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCES}
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GUI_SOURCE_DIR}
    ${UTILS_SOURCE_DIR}
    ${CORE_SOURCCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIR}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    ${SDL2_LIBRARIES}
    ${FFMPEG_LIBRARIES}
)

# 为Windows设置安装规则
if(WIN32)
    install(TARGETS ${PROJECT_NAME} DESTINATION .)
else()
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
endif()
