cmake_minimum_required(VERSION 3.15)

if (NOT VERSION)
    set(VERSION 1.0.0)
endif()

# 项目基本信息
project(deepin-scanner
    VERSION ${VERSION}
    DESCRIPTION "Deepin scanner manager"
    LANGUAGES CXX C
)

message(">>> Version: ${PROJECT_VERSION}")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   set(CMAKE_INSTALL_PREFIX /usr)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
# 引入翻译生成
include(translation-generate)

# 查找Qt版本
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
message(">>> Found Qt version: ${QT_VERSION_MAJOR}")
set(QT_DESIRED_VERSION ${QT_VERSION_MAJOR})

# 设置DTK版本
if (QT_VERSION_MAJOR MATCHES 6)
    set(DTK_VERSION_MAJOR 6)
else()
    set(DTK_VERSION_MAJOR "")
endif()
message(">>> Build with DTK: ${DTK_VERSION_MAJOR}")

# 定义必需的Qt组件
set(qt_required_components Core Gui Widgets Concurrent)

# 添加Qt6特有组件
if (QT_DESIRED_VERSION MATCHES 6)
    list(APPEND qt_required_components Core5Compat)
endif()

# 查找Qt和DTK
find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS ${qt_required_components})
# 查找DTK并添加包含路径
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget Gui Core)
include_directories(${Dtk${DTK_VERSION_MAJOR}_INCLUDE_DIRS})

# 定义链接库
set(LINK_LIBS
    Qt${QT_DESIRED_VERSION}::Core
    Qt${QT_DESIRED_VERSION}::Gui
    Qt${QT_DESIRED_VERSION}::Widgets
    Qt${QT_DESIRED_VERSION}::Concurrent
    Dtk${DTK_VERSION_MAJOR}::Widget
    Dtk${DTK_VERSION_MAJOR}::Gui
    Dtk${DTK_VERSION_MAJOR}::Core
)

# # 添加Qt6特有链接库
# if (QT_DESIRED_VERSION MATCHES 6)
#     list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::Core5Compat)
# endif()

# 安全编译选项
add_compile_options(
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
    -fPIC
    -pie
    -Wno-unused-parameter
)

# add_link_options(
#     -pie
#     -z lazy
# )

# 添加子项目
add_subdirectory(src)
# add_subdirectory(tests)

# translation files
TRANSLATION_GENERATE(QM_FILES ${CMAKE_SOURCE_DIR}/translations)
add_custom_target(${PROJECT_NAME}_qm_files DEPENDS ${QM_FILES})
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_qm_files)

# Install translations
install(FILES ${QM_FILES} DESTINATION share/${PROJECT_NAME}/translations)

# Install help manual
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/deepin-scanner DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-manual/manual-assets/application/)

# log viewer config
file(GLOB LOGCONFIG_FILES "${CMAKE_SOURCE_DIR}/assets/logconfig/*.json")
install(FILES ${LOGCONFIG_FILES} DESTINATION share/deepin-log-viewer/deepin-log.conf.d)

# 安装 DConfig 配置文件，1040及以下的默认DTK环境不支持DConfig
set(DCONFIG_APPID org.deepin.scanner)
file(GLOB DCONFIG_FILES "${CMAKE_SOURCE_DIR}/assets/configs/*.json")
if(DEFINED DSG_DATA_DIR)
    if (DTK_VERSION_MAJOR EQUAL "6")
        dtk_add_config_meta_files(APPID ${DCONFIG_APPID} FILES ${DCONFIG_FILES})
    else()
        dconfig_meta_files(APPID ${DCONFIG_APPID} FILES ${DCONFIG_FILES})
    endif()
    message("-- DConfig is supported by DTK")
else()
    install(FILES ${DCONFIG_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dsg/configs/${DCONFIG_APPID})
    message("-- DConfig is not supported by DTK")
endif()