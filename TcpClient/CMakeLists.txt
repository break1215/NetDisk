cmake_minimum_required(VERSION 3.16)

# 项目名称保持统一大小写 (建议首字母大写)
project(TcpClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 【优化】智能查找 Qt 版本
# 优先尝试 Qt6，如果没有则使用 Qt5
find_package(Qt6 COMPONENTS Widgets Core Network QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Widgets Core Network)
endif()

# 确定主版本号，方便后续使用
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
else()
    set(QT_VERSION_MAJOR 5)
endif()

message(STATUS "Building TcpClient with Qt${QT_VERSION_MAJOR}")

# 3. 【统一】收集所有源文件
# 将 sharefile 和所有的 .qrc 都放在这里，不要分散在 add_executable 中
set(PROJECT_SOURCES
    main.cpp
    tcpclient.cpp tcpclient.h tcpclient.ui
    protocol.h protocol.cpp
    opewidget.h opewidget.cpp
    online.h online.cpp online.ui
    friend.h friend.cpp
    book.h book.cpp
    privatechat.h privatechat.cpp privatechat.ui
    sharefile.h sharefile.cpp       # 移到这里
    config.qrc
    FileType.qrc                    # 确保此文件存在，不存在请删除此行
)

# 4. 创建可执行文件
if(QT_VERSION_MAJOR EQUAL 6)
    qt_add_executable(TcpClient
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_finalize_executable(TcpClient)
else()
    # Qt5 路径
    if(ANDROID)
        add_library(TcpClient SHARED ${PROJECT_SOURCES})
    else()
        add_executable(TcpClient ${PROJECT_SOURCES})
    endif()
endif()

# 5. 链接库
target_link_libraries(TcpClient PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)
