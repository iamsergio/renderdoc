option(STATIC_QRENDERDOC "Compile qrenderdoc as static" OFF)
option(QRENDERDOC_NO_CXX11_REGEX "Disable C++11 regex in scintilla" OFF)

set(RENDERDOC_SWIG_PACKAGE https://github.com/baldurk/swig/archive/renderdoc-modified-7.zip CACHE STRING "The location where RenderDoc's swig fork source can be found. By default points to the URL on github but can be pointed to a local file.")
set(RENDERDOC_PCRE_PACKAGE https://sourceforge.net/projects/pcre/files/pcre/8.45/pcre-8.45.tar.gz CACHE STRING "The location where the pcre library package can be found. By default points to the URL on sourceforge but can be pointed to a local file.")

if(ENABLE_QRENDERDOC)
    find_package(Qt5 5.6 REQUIRED COMPONENTS Core Gui Widgets Svg Network)
    if(NOT APPLE)
        find_package(Qt5 5.6 REQUIRED COMPONENTS X11Extras)
    endif()
    message(STATUS "Building using Qt ${Qt5_VERSION}")
endif()

set(SWIG_FLAGS "")

if(NOT APPLE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64-bit
        set(SWIG_FLAGS "-DSWIGWORDSIZE64")
    else() # 32-bit
        set(SWIG_FLAGS "-DSWIGWORDSIZE32")
    endif()
endif()

if(APPLE)
    add_custom_command(OUTPUT RenderDoc.icns
        COMMAND echo Creating OS X Icon
        COMMAND mkdir -p RenderDoc.iconset
        COMMAND qlmanage -t -s 1024 -o . ${CMAKE_CURRENT_SOURCE_DIR}/Resources/logo.svg
        COMMAND sips -z 16 16     logo.svg.png --out RenderDoc.iconset/icon_16x16.png
        COMMAND sips -z 32 32     logo.svg.png --out RenderDoc.iconset/icon_16x16@2x.png
        COMMAND sips -z 32 32     logo.svg.png --out RenderDoc.iconset/icon_32x32.png
        COMMAND sips -z 64 64     logo.svg.png --out RenderDoc.iconset/icon_32x32@2x.png
        COMMAND sips -z 128 128   logo.svg.png --out RenderDoc.iconset/icon_128x128.png
        COMMAND sips -z 256 256   logo.svg.png --out RenderDoc.iconset/icon_128x128@2x.png
        COMMAND sips -z 256 256   logo.svg.png --out RenderDoc.iconset/icon_256x256.png
        COMMAND sips -z 512 512   logo.svg.png --out RenderDoc.iconset/icon_256x256@2x.png
        COMMAND sips -z 512 512   logo.svg.png --out RenderDoc.iconset/icon_512x512.png
        COMMAND sips -z 1024 1024 logo.svg.png --out RenderDoc.iconset/icon_512x512@2x.png
        COMMAND iconutil -c icns RenderDoc.iconset
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/logo.svg)
endif()

include(ExternalProject)
include(CheckIncludeFile)

if(ENABLE_PYRENDERDOC)

# Need bison for swig
find_package(BISON)

if(APPLE)
    # macOS 10.14+ ships broken compilers, need to disable CC/CXX inheritance
    # If SWIG configuration fails, try configuring CMake with these vars set to clang and clang++ respectively
    if (NOT DEFINED SWIG_CONFIGURE_CC)
        set(SWIG_CONFIGURE_CC "")
    endif()
    if (NOT DEFINED SWIG_CONFIGURE_CXX)
        set(SWIG_CONFIGURE_CXX "")
    endif()
else()
    set(SWIG_CONFIGURE_CC ${CMAKE_C_COMPILER})
    set(SWIG_CONFIGURE_CXX ${CMAKE_CXX_COMPILER})
endif()

set( GENERATOR_MAKE "$(MAKE)" )
set( GENERATOR_MAKE_PARAMS "" )
if( NOT CMAKE_MAKE_PROGRAM MATCHES "make$" )
    message( "Non make generator detected. Some projects compiled using add_custom_command like swig and QRenderDoc target will still be using make" )
    set( GENERATOR_MAKE "make" )

    include(ProcessorCount)
    ProcessorCount(PROC_COUNT)
    if(NOT PROC_COUNT EQUAL 0)
        message( "Running make with -j${PROC_COUNT} for parallel build" )
        set(GENERATOR_MAKE_PARAMS "-j${PROC_COUNT}")
    else()
        message(WARNING "!!!! Couldn't determine processor count. Make will be run non-parallel !!!!")
    endif()
endif()

if(APPLE AND CMAKE_GENERATOR STREQUAL "Xcode")
    if("${SHELL_CAPTURED_PATH}" STREQUAL "")
        set(SHELL_CAPTURED_PATH $ENV{PATH} CACHE STRING "Shell PATH" FORCE)
    endif()
    message(STATUS "Using custom path for targets custom_swig and QRenderDoc")
    set(SET_SYSTEM_PATH_COMMAND ${CMAKE_COMMAND} -E env PATH=${SHELL_CAPTURED_PATH})
endif()

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.24")
    cmake_policy(SET CMP0135 NEW)
endif()

CHECK_INCLUDE_FILE(pcre.h PCRE_HEADER)

set(PCRE_PREFIX_PATH "")

if(PCRE_HEADER)
    message(STATUS "pcre library is found, using it as-is")

    add_custom_target(local_pcre)
else()
    message(STATUS "pcre header is not found! building pcre locally")
    
    # fetch pcre if missing
    ExternalProject_Add(local_pcre
        # using an URL to a zip directly so we don't clone the history etc
        URL ${RENDERDOC_PCRE_PACKAGE}
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND CC=${SWIG_CONFIGURE_CC} CXX=${SWIG_CONFIGURE_CXX} CFLAGS=-fPIC CXXFLAGS=-fPIC ${SET_SYSTEM_PATH_COMMAND} ./configure --prefix=${CMAKE_BINARY_DIR} --disable-shared > /dev/null
        BUILD_COMMAND ${GENERATOR_MAKE} ${GENERATOR_MAKE_PARAMS} > /dev/null 2>&1
        INSTALL_COMMAND ${GENERATOR_MAKE} install > /dev/null 2>&1)

    set(PCRE_PREFIX_PATH "--with-pcre-prefix=${CMAKE_BINARY_DIR}")
endif()

# Compile our custom SWIG that will do scoped/strong enum classes
ExternalProject_Add(custom_swig
    # using an URL to a zip directly so we don't clone the history etc
    URL ${RENDERDOC_SWIG_PACKAGE}
    DEPENDS local_pcre
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${SET_SYSTEM_PATH_COMMAND} ./autogen.sh > /dev/null 2>&1
    COMMAND CC=${SWIG_CONFIGURE_CC} CXX=${SWIG_CONFIGURE_CXX} CFLAGS=-fPIC CXXFLAGS=-fPIC ${SET_SYSTEM_PATH_COMMAND} ./configure --with-pcre=yes ${PCRE_PREFIX_PATH} --prefix=${CMAKE_BINARY_DIR} > /dev/null
    BUILD_COMMAND ${GENERATOR_MAKE} ${GENERATOR_MAKE_PARAMS} > /dev/null 2>&1
    INSTALL_COMMAND ${GENERATOR_MAKE} install > /dev/null 2>&1)

find_package(Shiboken2 QUIET)
find_package(PySide2 QUIET)

if(ENABLE_UNSUPPORTED_EXPERIMENTAL_POSSIBLY_BROKEN_WAYLAND)
    message(WARNING "!!!! Using the Wayland Qt platform in qrenderdoc is NOT SUPPORTED !!!!")
endif()

option(QRENDERDOC_ENABLE_PYSIDE2 "Enable PySide2 if found" ON)

set(QRENDERDOC_PYSIDE2_AVAILABLE OFF)

if(NOT QRENDERDOC_ENABLE_PYSIDE2)
    message(STATUS "PySide2 integration disabled")
elseif(PySide2_FOUND AND Shiboken2_FOUND AND TARGET Shiboken2::libshiboken AND TARGET PySide2::pyside2)
    message(STATUS "Building with PySide2 ${PySide2_VERSION} from ${PySide2_DIR}")

    if(NOT PYTHONLIBS_VERSION_STRING MATCHES "${SHIBOKEN_PYTHON_VERSION_MAJOR}.${SHIBOKEN_PYTHON_VERSION_MINOR}")
        message(FATAL_ERROR "Shiboken2 that we found was linked against python ${SHIBOKEN_PYTHON_VERSION_MAJOR}.${SHIBOKEN_PYTHON_VERSION_MINOR} but the python library we're linking against is ${PYTHONLIBS_VERSION_STRING}. Ensure these versions are identical or disable PySide2 compilation with QRENDERDOC_ENABLE_PYSIDE2.")
    endif()

    set(QRENDERDOC_PYSIDE2_AVAILABLE ON)

    get_target_property(SHIBOKEN2_INCLUDE_DIRS Shiboken2::libshiboken INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(PYSIDE2_INCLUDE_DIRS PySide2::pyside2 INTERFACE_INCLUDE_DIRECTORIES)
else()
    message(STATUS "PySide2 not found - Qt will not be accessible in python scripting. See https://github.com/baldurk/renderdoc/wiki/PySide2")
endif()

# generate the SWIG interface files
set(swig_interfaces
    Code/pyrenderdoc/renderdoc.i
    Code/pyrenderdoc/qrenderdoc.i)

set(swig_output)

file(GLOB RDOC_REPLAY_FILES ${CMAKE_SOURCE_DIR}/renderdoc/api/replay/*.h)
file(GLOB QRD_INTERFACE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Code/Interface/*.h)

list(SORT RDOC_REPLAY_FILES)
list(SORT QRD_INTERFACE_FILES)

foreach(in ${swig_interfaces})
    get_filename_component(swig_file ${in} NAME_WE)

    add_custom_command(OUTPUT ${swig_file}_python.cxx ${swig_file}.py
            COMMAND ${CMAKE_BINARY_DIR}/bin/swig -v -Wextra -Werror ${SWIG_FLAGS} -O -c++ -python -modern -interface ${swig_file} -modernargs -enumclass -fastunpack -py3 -builtin -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_SOURCE_DIR}/renderdoc/api/replay -outdir ${CMAKE_CURRENT_BINARY_DIR} -o ${CMAKE_CURRENT_BINARY_DIR}/${swig_file}_python.cxx ${CMAKE_CURRENT_SOURCE_DIR}/${in}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${in}
            DEPENDS custom_swig
            DEPENDS ${RDOC_REPLAY_FILES}
            DEPENDS ${QRD_INTERFACE_FILES})

    list(APPEND swig_output ${swig_file}_python.cxx)
endforeach()

add_custom_target(swig-bindings ALL DEPENDS ${swig_output})

else() # not ENABLE_PYRENDERDOC
    set(QRENDERDOC_PYSIDE2_AVAILABLE OFF)
    add_custom_target(swig-bindings)
endif()

if(ENABLE_QRENDERDOC)

    if(ENABLE_PYRENDERDOC)
        set(QRD_PYTHON_SOURCES Code/pyrenderdoc/PythonContext.cpp)
    else()
        set(QRD_PYTHON_SOURCES Code/pyrenderdoc/PythonContext_nopython.cpp)
    endif()

    set(QRD_SOURCES
        Code/qrenderdoc.cpp
        Code/qprocessinfo.cpp
        Code/ReplayManager.cpp
        Code/CaptureContext.cpp
        Code/ScintillaSyntax.cpp
        Code/QRDUtils.cpp
        Code/MiniQtHelper.cpp
        Code/BufferFormatter.cpp
        Code/Resources.cpp
        Code/RGPInterop.cpp
        ${QRD_PYTHON_SOURCES}
        Code/Interface/QRDInterface.cpp
        Code/Interface/Analytics.cpp
        Code/Interface/ShaderProcessingTool.cpp
        Code/Interface/PersistantConfig.cpp
        Code/Interface/RemoteHost.cpp
        Styles/StyleData.cpp
        Styles/RDStyle/RDStyle.cpp
        Styles/RDTweakedNativeStyle/RDTweakedNativeStyle.cpp
        Windows/Dialogs/AboutDialog.cpp
        Windows/Dialogs/CrashDialog.cpp
        Windows/Dialogs/UpdateDialog.cpp
        Windows/MainWindow.cpp
        Windows/EventBrowser.cpp
        Windows/TextureViewer.cpp
        Windows/ShaderViewer.cpp
        Windows/ShaderMessageViewer.cpp
        Windows/DescriptorViewer.cpp
        Widgets/Extended/RDLineEdit.cpp
        Widgets/Extended/RDTextEdit.cpp
        Widgets/Extended/RDLabel.cpp
        Widgets/Extended/RDMenu.cpp
        Widgets/Extended/RDHeaderView.cpp
        Widgets/Extended/RDToolButton.cpp
        Widgets/Extended/RDDoubleSpinBox.cpp
        Widgets/Extended/RDListView.cpp
        Widgets/ComputeDebugSelector.cpp
        Widgets/CustomPaintWidget.cpp
        Widgets/ResourcePreview.cpp
        Widgets/ThumbnailStrip.cpp
        Widgets/ReplayOptionsSelector.cpp
        Widgets/TextureGoto.cpp
        Widgets/RangeHistogram.cpp
        Widgets/AnnotationDisplay.cpp
        Widgets/CollapseGroupBox.cpp
        Windows/Dialogs/TextureSaveDialog.cpp
        Windows/Dialogs/CaptureDialog.cpp
        Windows/Dialogs/LiveCapture.cpp
        Widgets/Extended/RDListWidget.cpp
        Windows/APIInspector.cpp
        Windows/PipelineState/PipelineStateViewer.cpp
        Windows/PipelineState/VulkanPipelineStateViewer.cpp
        Windows/PipelineState/D3D11PipelineStateViewer.cpp
        Windows/PipelineState/D3D12PipelineStateViewer.cpp
        Windows/PipelineState/GLPipelineStateViewer.cpp
        Widgets/Extended/RDTreeView.cpp
        Widgets/Extended/RDTreeWidget.cpp
        Widgets/BufferFormatSpecifier.cpp
        Windows/BufferViewer.cpp
        Widgets/Extended/RDTableView.cpp
        Windows/DebugMessageView.cpp
        Windows/LogView.cpp
        Windows/CommentView.cpp
        Windows/StatisticsViewer.cpp
        Windows/TimelineBar.cpp
        Windows/Dialogs/SettingsDialog.cpp
        Widgets/OrderedListEditor.cpp
        Widgets/MarkerBreadcrumbs.cpp
        Widgets/Extended/RDTableWidget.cpp
        Windows/Dialogs/SuggestRemoteDialog.cpp
        Windows/Dialogs/VirtualFileDialog.cpp
        Windows/Dialogs/RemoteManager.cpp
        Windows/Dialogs/ExtensionManager.cpp
        Windows/PixelHistoryView.cpp
        Widgets/PipelineFlowChart.cpp
        Windows/Dialogs/EnvironmentEditor.cpp
        Widgets/FindReplace.cpp
        Widgets/Extended/RDSplitter.cpp
        Windows/Dialogs/TipsDialog.cpp
        Windows/Dialogs/ConfigEditor.cpp
        Windows/PythonShell.cpp
        Windows/Dialogs/PerformanceCounterSelection.cpp
        Windows/PerformanceCounterViewer.cpp
        Windows/ResourceInspector.cpp
        Windows/Dialogs/AnalyticsConfirmDialog.cpp
        Windows/Dialogs/AnalyticsPromptDialog.cpp
        Windows/Dialogs/AxisMappingDialog.cpp
        Windows/Dialogs/CameraControlsDialog.cpp
        Windows/Dialogs/ProjectionGuessDialog.cpp
        # ToolWindowManager + FlowLayout (bundled)
        3rdparty/toolwindowmanager/ToolWindowManager.cpp
        3rdparty/toolwindowmanager/ToolWindowManagerArea.cpp
        3rdparty/toolwindowmanager/ToolWindowManagerSplitter.cpp
        3rdparty/toolwindowmanager/ToolWindowManagerTabBar.cpp
        3rdparty/toolwindowmanager/ToolWindowManagerWrapper.cpp
        3rdparty/flowlayout/FlowLayout.cpp
    )

    # Headers so AUTOMOC picks them up reliably
    file(GLOB_RECURSE QRD_HEADERS
        Code/*.h
        Styles/*.h
        Windows/*.h
        Widgets/*.h
    )
    list(APPEND QRD_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/toolwindowmanager/ToolWindowManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/toolwindowmanager/ToolWindowManagerArea.h
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/toolwindowmanager/ToolWindowManagerSplitter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/toolwindowmanager/ToolWindowManagerTabBar.h
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/toolwindowmanager/ToolWindowManagerWrapper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/flowlayout/FlowLayout.h
    )

    # Scintilla (bundled). Globbed to match the qmake .pro behavior
    file(GLOB SCINTILLA_LEXLIB_SOURCES  3rdparty/scintilla/lexlib/*.cxx)
    file(GLOB SCINTILLA_LEXERS_SOURCES  3rdparty/scintilla/lexers/*.cxx)
    file(GLOB SCINTILLA_SRC_SOURCES     3rdparty/scintilla/src/*.cxx)
    file(GLOB SCINTILLA_EDIT_SOURCES    3rdparty/scintilla/qt/ScintillaEdit/*.cpp)
    file(GLOB SCINTILLA_EDITBASE_SOURCES 3rdparty/scintilla/qt/ScintillaEditBase/*.cpp)

    set(SCINTILLA_NON_QT_SOURCES
        ${SCINTILLA_LEXLIB_SOURCES}
        ${SCINTILLA_LEXERS_SOURCES}
        ${SCINTILLA_SRC_SOURCES}
    )
    set(SCINTILLA_QT_SOURCES
        ${SCINTILLA_EDIT_SOURCES}
        ${SCINTILLA_EDITBASE_SOURCES}
    )
    file(GLOB SCINTILLA_QT_HEADERS
        3rdparty/scintilla/qt/ScintillaEdit/*.h
        3rdparty/scintilla/qt/ScintillaEditBase/*.h
    )

    # Mac-specific
    if(APPLE)
        list(APPEND QRD_SOURCES Code/AppleUtils.mm)
    endif()

    # SWIG-generated python bindings (produced by the swig-bindings target above)
    if(ENABLE_PYRENDERDOC)
        set(QRD_GENERATED_SOURCES
            ${CMAKE_BINARY_DIR}/qrenderdoc/renderdoc_python.cxx
            ${CMAKE_BINARY_DIR}/qrenderdoc/qrenderdoc_python.cxx
        )
    else()
        set(QRD_GENERATED_SOURCES)
    endif()

    # UI forms (Qt Designer files) - AUTOUIC processes these
    set(QRD_UI_FILES
        Windows/Dialogs/AboutDialog.ui
        Windows/Dialogs/CrashDialog.ui
        Windows/Dialogs/UpdateDialog.ui
        Windows/MainWindow.ui
        Windows/EventBrowser.ui
        Windows/TextureViewer.ui
        Widgets/ResourcePreview.ui
        Widgets/ThumbnailStrip.ui
        Widgets/ReplayOptionsSelector.ui
        Windows/Dialogs/TextureSaveDialog.ui
        Windows/Dialogs/CaptureDialog.ui
        Windows/Dialogs/LiveCapture.ui
        Windows/APIInspector.ui
        Windows/PipelineState/PipelineStateViewer.ui
        Windows/PipelineState/VulkanPipelineStateViewer.ui
        Windows/PipelineState/D3D11PipelineStateViewer.ui
        Windows/PipelineState/D3D12PipelineStateViewer.ui
        Windows/PipelineState/GLPipelineStateViewer.ui
        Widgets/BufferFormatSpecifier.ui
        Widgets/ComputeDebugSelector.ui
        Windows/BufferViewer.ui
        Windows/ShaderViewer.ui
        Windows/ShaderMessageViewer.ui
        Windows/DescriptorViewer.ui
        Windows/DebugMessageView.ui
        Windows/LogView.ui
        Windows/CommentView.ui
        Windows/StatisticsViewer.ui
        Windows/Dialogs/SettingsDialog.ui
        Windows/Dialogs/SuggestRemoteDialog.ui
        Windows/Dialogs/VirtualFileDialog.ui
        Windows/Dialogs/RemoteManager.ui
        Windows/Dialogs/ExtensionManager.ui
        Windows/PixelHistoryView.ui
        Windows/Dialogs/EnvironmentEditor.ui
        Widgets/FindReplace.ui
        Windows/Dialogs/TipsDialog.ui
        Windows/Dialogs/ConfigEditor.ui
        Windows/PythonShell.ui
        Windows/Dialogs/PerformanceCounterSelection.ui
        Windows/PerformanceCounterViewer.ui
        Windows/ResourceInspector.ui
        Windows/Dialogs/AnalyticsConfirmDialog.ui
        Windows/Dialogs/AnalyticsPromptDialog.ui
        Windows/Dialogs/AxisMappingDialog.ui
        Windows/Dialogs/CameraControlsDialog.ui
        Windows/Dialogs/ProjectionGuessDialog.ui
    )

    set(QRD_RESOURCE_FILES Resources/resources.qrc)
    if(APPLE)
        list(APPEND QRD_RESOURCE_FILES Resources/qtconf.qrc)
    endif()

    add_executable(qrenderdoc
        ${QRD_SOURCES}
        ${QRD_HEADERS}
        ${SCINTILLA_NON_QT_SOURCES}
        ${SCINTILLA_QT_SOURCES}
        ${SCINTILLA_QT_HEADERS}
        ${QRD_GENERATED_SOURCES}
        ${QRD_UI_FILES}
        ${QRD_RESOURCE_FILES}
    )

    set_target_properties(qrenderdoc PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Scintilla non-Qt sources don't need MOC scanning
    set_source_files_properties(${SCINTILLA_NON_QT_SOURCES} PROPERTIES
        SKIP_AUTOMOC ON
        SKIP_AUTOUIC ON
    )
    set_source_files_properties(${QRD_GENERATED_SOURCES} PROPERTIES
        GENERATED TRUE
        SKIP_AUTOMOC ON
        SKIP_AUTOUIC ON
    )

    # QRDUtils.cpp redeclares a Qt private class for ABI access; not for moc.
    # qmake ignores it because it's only in SOURCES (qmake mocs HEADERS).
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/Code/QRDUtils.cpp PROPERTIES
        SKIP_AUTOMOC ON
    )

    add_dependencies(qrenderdoc swig-bindings)

    target_include_directories(qrenderdoc PRIVATE
        ${CMAKE_SOURCE_DIR}/renderdoc/api/replay
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/scintilla/include/qt
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/scintilla/include
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/scintilla/src
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/scintilla/lexlib
    )
    if(ENABLE_PYRENDERDOC)
        target_include_directories(qrenderdoc PRIVATE ${PYTHON_INCLUDE_DIR})
    endif()

    if(QRENDERDOC_PYSIDE2_AVAILABLE)
        target_include_directories(qrenderdoc PRIVATE
            ${SHIBOKEN2_INCLUDE_DIRS}
            ${PYSIDE2_INCLUDE_DIRS}
        )
    endif()

    target_compile_definitions(qrenderdoc PRIVATE
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_DEPRECATED_WARNINGS
        SCINTILLA_QT=1
        MAKING_LIBRARY=1
        SCI_LEXER=1
        RENDERDOC_VERSION=${RENDERDOC_VERSION}
    )

    if(APPLE)
        target_compile_definitions(qrenderdoc PRIVATE
            RENDERDOC_PLATFORM_POSIX
            RENDERDOC_PLATFORM_APPLE
        )
    else()
        target_compile_definitions(qrenderdoc PRIVATE
            RENDERDOC_PLATFORM_POSIX
            RENDERDOC_PLATFORM_LINUX
        )
        if(ENABLE_XLIB)
            target_compile_definitions(qrenderdoc PRIVATE RENDERDOC_WINDOWING_XLIB)
        endif()
        if(ENABLE_XCB)
            target_compile_definitions(qrenderdoc PRIVATE RENDERDOC_WINDOWING_XCB)
        endif()
        if(ENABLE_UNSUPPORTED_EXPERIMENTAL_POSSIBLY_BROKEN_WAYLAND)
            target_compile_definitions(qrenderdoc PRIVATE RENDERDOC_WINDOWING_WAYLAND)
        endif()
    endif()

    if(QRENDERDOC_PYSIDE2_AVAILABLE)
        target_compile_definitions(qrenderdoc PRIVATE
            PYSIDE2_ENABLED=1
            PYSIDE2_SYS_PATH=${PYSIDE_PYTHONPATH}
        )
    else()
        target_compile_definitions(qrenderdoc PRIVATE PYSIDE2_ENABLED=0)
    endif()

    if(STATIC_QRENDERDOC)
        target_compile_definitions(qrenderdoc PRIVATE STATIC_QRENDERDOC=1)
    endif()

    if(QRENDERDOC_NO_CXX11_REGEX)
        target_compile_definitions(qrenderdoc PRIVATE NO_CXX11_REGEX=1)
    endif()

    target_compile_options(qrenderdoc PRIVATE -Wno-deprecated-declarations)

    # Mirror the per-compiler warning suppressions that the .pri generation applies
    if(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(qrenderdoc PRIVATE
            -Wno-unknown-warning
            -Wno-cast-function-type
            -Wno-stringop-truncation)
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 14.0)
            target_compile_options(qrenderdoc PRIVATE -Wno-template-id-cdtor)
        endif()
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(qrenderdoc PRIVATE -Wno-shadow -Wno-shorten-64-to-32)
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.9)
            target_compile_options(qrenderdoc PRIVATE -Wno-comma)
        endif()
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19)
            target_compile_options(qrenderdoc PRIVATE -Wno-cast-function-type)
        endif()
    endif()

    target_link_libraries(qrenderdoc PRIVATE
        Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg Qt5::Network
        renderdoc
    )
    if(ENABLE_PYRENDERDOC)
        target_link_libraries(qrenderdoc PRIVATE ${PYTHON_LIBRARY})
    endif()

    if(NOT APPLE)
        target_link_libraries(qrenderdoc PRIVATE Qt5::X11Extras)
    endif()

    if(STATIC_QRENDERDOC)
        target_link_libraries(qrenderdoc PRIVATE util)
        target_link_options(qrenderdoc PRIVATE
            -rdynamic
            "LINKER:--whole-archive"
            ${PYTHON_LIBRARY}
            "LINKER:--no-whole-archive"
        )
    endif()

    if(QRENDERDOC_PYSIDE2_AVAILABLE)
        target_link_libraries(qrenderdoc PRIVATE
            shiboken2${SHIBOKEN_PYTHON_SHARED_LIBRARY_SUFFIX}
        )
    endif()

    if(APPLE)
        target_link_libraries(qrenderdoc PRIVATE
            "-framework Cocoa"
            "-framework QuartzCore"
        )
        set_target_properties(qrenderdoc PROPERTIES
            INSTALL_RPATH "@executable_path/../lib"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    else()
        # Linux: rpath to find librenderdoc.so next to the binary or in ../lib
        set_target_properties(qrenderdoc PROPERTIES
            INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../lib${LIB_SUFFIX}/${LIB_SUBFOLDER_TRAIL_SLASH}"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
        target_link_options(qrenderdoc PRIVATE
            "LINKER:--no-as-needed"
            -rdynamic
        )
    endif()

    add_custom_target(build-qrenderdoc ALL DEPENDS qrenderdoc)

    install(TARGETS qrenderdoc RUNTIME DESTINATION bin)

    # Install supporting files for file associations etc
    install (FILES share/application-x-renderdoc-capture.svg DESTINATION share/icons/hicolor/scalable/mimetypes/)
    install (FILES share/renderdoc-icon-16x16.xpm DESTINATION share/pixmaps/)
    install (FILES share/renderdoc-icon-32x32.xpm DESTINATION share/pixmaps/)
    install (FILES share/magic DESTINATION share/doc/renderdoc)
    install (FILES share/menu DESTINATION share/menu RENAME renderdoc)
    install (FILES share/renderdoc.desktop DESTINATION share/applications)
    install (FILES share/renderdoc.thumbnailer DESTINATION share/thumbnailers)
    install (FILES share/renderdoc-capture.xml DESTINATION share/mime/packages)

    install (CODE "MESSAGE(\"You now need to update some caches.\")")
    install (CODE "MESSAGE(\"e.g.\")")
    install (CODE "MESSAGE(\"sudo update-desktop-database\")")
    install (CODE "MESSAGE(\"sudo update-menus\")")
    install (CODE "MESSAGE(\"sudo update-mime-database /usr/share/mime/\")")
    install (CODE "MESSAGE(\"sudo gtk-update-icon-cache /usr/share/icons/hicolor/\")")
    install (CODE "MESSAGE(\"NB: Your paths may vary.\")")

endif() # if(ENABLE_QRENDERDOC)

# Build python modules - primarily used for constructing documentation
if(ENABLE_PYRENDERDOC AND UNIX)
    add_subdirectory(Code/pyrenderdoc)
endif()
