cmake_minimum_required(VERSION 3.15)

# project(deepin-scanner)

add_subdirectory(libofd)

# --- Find SANE and PNG using pkg-config (more robust) ---
# Only on non-Windows systems
if(UNIX AND NOT APPLE)
    # Find pkg-config itself
    find_package(PkgConfig REQUIRED)

    # Use pkg-config to find SANE (usually provided as 'sane-backends')
    pkg_check_modules(SANE REQUIRED sane-backends)

    if(SANE_FOUND)
        message(STATUS "Found SANE: TRUE (via pkg-config)")
        include_directories(${SANE_INCLUDE_DIRS}) # Add include directories
    else()
        message(FATAL_ERROR "SANE library (sane-backends) not found by pkg-config. Is libsane-dev installed?")
    endif()

    # Use pkg-config to find PNG (usually provided as 'libpng')
    pkg_check_modules(PNG REQUIRED libpng)

    if(PNG_FOUND)
        message(STATUS "Found PNG: TRUE (via pkg-config)")
        include_directories(${PNG_INCLUDE_DIRS}) # Add include directories
    else()
        message(FATAL_ERROR "PNG library (libpng) not found by pkg-config. Is libpng-dev installed?")
    endif()
endif()

# 收集所有源文件
file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/device/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/device/*.h"
)

# 定义可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
)

# 启用自动moc处理
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
    APP_VERSION="${VERSION}"
)

# 包含目录
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 设置包含 Qt 头文件的路径
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LINK_LIBS}
    ofdcpp
)

# --- 链接 SANE 和 PNG (仅在非 Windows 系统) ---
if(UNIX AND NOT APPLE)
    # Use variables set by pkg_check_modules
    if(SANE_FOUND AND PNG_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${SANE_LIBRARIES} ${PNG_LIBRARIES})
        message(STATUS "Linking with SANE and PNG libraries (found via pkg-config).")
    endif()

    # No need for else here, pkg_check_modules already handles REQUIRED errors
endif()

# 添加资源文件
if(QT_DESIRED_VERSION MATCHES 6)
    qt_add_resources(qrc_RESOURCES
        ${CMAKE_SOURCE_DIR}/resources/resources.qrc
    )
else()
    qt5_add_resources(qrc_RESOURCES
        ${CMAKE_SOURCE_DIR}/resources/resources.qrc
    )
endif()

target_sources(${PROJECT_NAME} PRIVATE ${qrc_RESOURCES})

# 安装目标
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装桌面文件和图标
install(FILES
    ${CMAKE_SOURCE_DIR}/resources/deepin-scanner.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES
    ${CMAKE_SOURCE_DIR}/resources/icons/deepin/builtin/icons/deepin-scanner.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

# # 安装帮助文件
# install(DIRECTORY
# ${CMAKE_SOURCE_DIR}/assets/
# DESTINATION ${CMAKE_INSTALL_DATADIR}/deepin-manual/manual-assets/application
# )