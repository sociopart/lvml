cmake_minimum_required(VERSION 3.16)
project(lvml VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LVML_BUILD_RUNTIME "Build the LVGL runtime (needs LVGL)" ON)
option(LVML_BUILD_TESTS   "Build parser tests"                   ON)
option(LVML_BUILD_HTML2LVML "Build the HTML/CSS → lvml converter" ON)
option(LVML_BUILD_RENDER    "Build lvml-render (headless preview)" ON)
option(LVML_BUILD_WIN32_HOST "Add CLvmlWin32Window (needs LV_USE_WINDOWS)" ${WIN32})
set(LVML_LV_CONF "${CMAKE_CURRENT_SOURCE_DIR}/examples/lv_conf.h" CACHE FILEPATH "lv_conf.h used when lvml builds LVGL itself")

if(MINGW)
    add_link_options(-static) # no libstdc++/libgcc DLLs next to the tools
endif()
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# ---------- core: parsers, no LVGL ----------
add_library(lvml_core STATIC
    source/TextUtils.cpp
    source/LvmlNode.cpp
    source/LvmlDocument.cpp
    source/LvmlParser.cpp
    source/LvssSelector.cpp
    source/LvssRule.cpp
    source/LvssStyleSheet.cpp
    source/LvssParser.cpp
    source/LvssValue.cpp
)
target_include_directories(lvml_core PUBLIC source)

# ---------- runtime: LVGL objects from documents ----------
if(LVML_BUILD_RUNTIME)
    if(NOT TARGET lvgl)
        set(LV_BUILD_CONF_PATH ${LVML_LV_CONF} CACHE PATH "" FORCE)
        set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
        set(LV_BUILD_SET_CONFIG_OPTS OFF CACHE BOOL "" FORCE)
        add_subdirectory(vendor/lvgl EXCLUDE_FROM_ALL)
        # LVGL 9.5: osal/lv_windows.c calls malloc without <stdlib.h>.
        if(MSVC)
            target_compile_options(lvgl PRIVATE $<$<COMPILE_LANGUAGE:C>:/FIstdlib.h>)
        else()
            target_compile_options(lvgl PRIVATE $<$<COMPILE_LANGUAGE:C>:-include stdlib.h>)
        endif()
    endif()

    add_library(lvml STATIC
        source/LvmlResources.cpp
        source/LvmlDirectoryResources.cpp
        source/LvmlMemoryResources.cpp
        source/LvmlFileSystem.cpp
        source/LvmlFontManager.cpp
        source/LvmlStorage.cpp
        source/LvssProperty.cpp
        source/LvmlStyleSet.cpp
        source/LvmlWidgetFactory.cpp
        source/LvmlEvent.cpp
        source/LvmlScene.cpp
        source/LvmlBuilder.cpp
        source/LvmlContext.cpp
        source/LvmlCursor.cpp
    )
    target_link_libraries(lvml PUBLIC lvml_core lvgl)
    if(WIN32 AND LVML_BUILD_WIN32_HOST)
        target_sources(lvml PRIVATE platform/win32/LvmlWin32Window.cpp)
        target_include_directories(lvml PUBLIC platform/win32)
        target_link_libraries(lvml PUBLIC dwmapi)
    endif()
endif()

# ---------- tests ----------
if(LVML_BUILD_TESTS)
    add_executable(lvml_tests tests/main.cpp tests/ParserTests.cpp)
    target_link_libraries(lvml_tests PRIVATE lvml_core)
    enable_testing()
    add_test(NAME lvml_tests COMMAND lvml_tests)
endif()

# ---------- tools ----------
if(LVML_BUILD_HTML2LVML)
    add_executable(html2lvml
        tools/html2lvml/main.cpp
        tools/html2lvml/HtmlConverter.cpp
        tools/html2lvml/CssConverter.cpp
        tools/html2lvml/ConversionReport.cpp
    )
    target_include_directories(html2lvml PRIVATE tools/html2lvml)
    target_link_libraries(html2lvml PRIVATE lvml_core)
endif()

if(LVML_BUILD_RENDER AND LVML_BUILD_RUNTIME)
    add_executable(lvml-render tools/lvml-render/main.cpp tools/lvml-render/SceneRenderer.cpp)
    target_link_libraries(lvml-render PRIVATE lvml)
endif()
