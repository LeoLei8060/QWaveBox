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
set(CORE_SOURCE_DIR src/core)
set(PLAY_SOURCE_DIR src/play)

set(GUI_SOURCES
    src/gui/titlebar.cpp
    src/gui/mainwidget.cpp
    src/gui/videowidget.cpp
    src/gui/playlistwidget.cpp
    src/gui/sdlwidget.cpp
    src/gui/filelistmodel.cpp
    src/gui/filelistview.cpp
)
set(GUI_HEADERS
    src/gui/titlebar.h
    src/gui/mainwidget.h
    src/gui/videowidget.h
    src/gui/playlistwidget.h
    src/gui/sdlwidget.h
    src/gui/filelistmodel.h
    src/gui/filelistview.h
)
set(GUI_FORMS
    src/gui/titlebar.ui
    src/gui/mainwidget.ui
    src/gui/videowidget.ui
    src/gui/playlistwidget.ui
)

set(UTILS_SOURCES
    src/utils/fontmanager.cpp
    src/utils/common.cpp
)
set(UTILS_HEADERS
    src/utils/fontmanager.h
    src/utils/common.h
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

set(PLAY_SOURCES
    src/play/audiodecodethread.cpp
    src/play/avframequeue.cpp
    src/play/avpacketqueue.cpp
    src/play/demuxthread.cpp
    src/play/renderthread.cpp
    src/play/threadbase.cpp
    src/play/threadmanager.cpp
    src/play/videodecodethread.cpp
    src/play/audiorenderthread.cpp
)
set(PLAY_HEADERS
    src/play/audiodecodethread.h
    src/play/avframequeue.h
    src/play/avpacketqueue.h
    src/play/demuxthread.h
    src/play/renderthread.h
    src/play/threadbase.h
    src/play/threadmanager.h
    src/play/videodecodethread.h
    src/play/audiorenderthread.h
    src/play/avsync.h
)

# 项目源文件
set(SOURCES
    ${GUI_SOURCES}
    ${UTILS_SOURCES}
    ${CORE_SOURCES}
    ${PLAY_SOURCES}
    src/main.cpp
)

# 项目头文件
set(HEADERS
    ${GUI_HEADERS}
    ${UTILS_HEADERS}
    ${CORE_HEADERS}
    ${PLAY_HEADERS}
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
    ${CORE_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${PLAY_SOURCE_DIR}
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
