cmake_minimum_required(VERSION 3.21)

# the version lives in 'VERSION', which the sources include as a header.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VANILLA_VERSION_HEADER)
string(REGEX MATCH "\"([0-9]+\\.[0-9]+\\.[0-9]+)\"" _ "${VANILLA_VERSION_HEADER}")
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "cannot read VANILLA_VERSION out of the 'VERSION' file.")
endif()

project(vanilla VERSION ${CMAKE_MATCH_1} LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Widgets Xml Network
    OpenGL OpenGLWidgets
    WebChannel
    Multimedia MultimediaWidgets
    Qml Quick QuickWidgets
    LinguistTools)

# optional in the same sense as 'qtHaveModule()' in the '.pro'.
find_package(Qt6 6.5 QUIET COMPONENTS PrintSupport WebEngineCore WebEngineWidgets WebEngineQuick)

qt_standard_project_setup(REQUIRES 6.5)

# the '.qrc' files are compiled where they sit, the same way qmake did it.
set(CMAKE_AUTORCC ON)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(VANILLA_HEADERS
    src/app/application.hpp
    src/app/extensionhost.hpp
    src/app/extensioncontroller.hpp
    src/ui/extensionbar.hpp
    src/ui/sidepanels.hpp
    src/view/webengine/webengineextensions.hpp
    src/app/saveflow.hpp
    src/app/shutdownflow.hpp
    src/input/actionmapper.hpp
    src/ui/mainwindow.hpp
    src/app/saver.hpp
    src/core/lightnode.hpp
    src/core/devicescale.hpp
    src/input/jsobject.hpp
    src/core/treeserializer.hpp
    src/core/bookmarkio.hpp
    src/core/settingsio.hpp
    src/core/inputmap.hpp
    src/core/inputmapschema.hpp
    src/core/commandmap.hpp
    src/core/commandframe.hpp
    src/core/certificatepolicy.hpp
    src/core/fileoperation.hpp
    src/core/fileexchange.hpp
    src/core/windowledger.hpp
    src/core/windowframereveal.hpp
    src/core/useragent.hpp
    src/core/cdpshims.hpp
    src/core/extensioncopy.hpp
    src/core/extensionhostwire.hpp
    src/core/extensionmainscripts.hpp
    src/core/extensionuserscripts.hpp
    src/core/extensionui.hpp
    src/core/dnrrules.hpp
    src/core/downloadname.hpp
    src/core/loadending.hpp
    src/core/translationorder.hpp
    src/core/nativehistory.hpp
    src/ui/treebank.hpp
    src/ui/treebar.hpp
    src/ui/toolbar.hpp
    src/ui/notifier.hpp
    src/ui/minimap.hpp
    src/ui/nodepreview.hpp
    src/app/networkcontroller.hpp
    src/core/switch.hpp
    src/core/callback.hpp
    src/core/const.hpp
    src/ui/theme.hpp
    src/core/settingsschema.hpp
    src/app/settingspage.hpp
    src/app/directorypage.hpp
    src/input/keymap.hpp
    src/input/mousemap.hpp
    src/input/spentbuttons.hpp
    src/app/receiver.hpp
    src/app/transmitter.hpp
    src/ui/dialog.hpp
    src/view/view.hpp
    src/view/page.hpp
    src/view/webelement.hpp
    src/view/mediatype.hpp
    src/view/localview.hpp
    src/view/webengine/webenginepage.hpp
    src/view/webengine/webengineview.hpp
    src/view/webengine/quickwebengineview.hpp
    src/view/quicknativewebview.hpp
    src/view/edge/edgewebview.hpp
    src/view/edge/edgewebview_p.hpp
    src/view/edge/edgeeventsubscriptions.hpp
    src/view/edge/edgeunadoptedcontroller.hpp
    src/view/edge/edgewebviewstate.hpp
    src/view/edge/edgemenuitem.hpp
    src/view/edge/edgehostwindow.hpp
    src/gadgets/graphicstableview.hpp
    src/gadgets/gadgets.hpp
    src/gadgets/gadgetsstyle.hpp
    src/gadgets/abstractnodeitem.hpp
    src/gadgets/thumbnail.hpp
    src/gadgets/nodetitle.hpp
    src/gadgets/accessiblewebelement.hpp)

set(VANILLA_SOURCES
    src/app/application.cpp
    src/app/extensionhost.cpp
    src/app/extensioncontroller.cpp
    src/ui/extensionbar.cpp
    src/ui/sidepanels.cpp
    src/view/webengine/webengineextensions.cpp
    src/view/edge/edgeextensions.cpp
    src/ui/mainwindow.cpp
    src/app/saver.cpp
    src/core/lightnode.cpp
    src/core/devicescale.cpp
    src/core/treeserializer.cpp
    src/core/bookmarkio.cpp
    src/core/settingsio.cpp
    src/core/inputmap.cpp
    src/core/inputmapschema.cpp
    src/core/commandmap.cpp
    src/core/commandframe.cpp
    src/core/certificatepolicy.cpp
    src/core/fileoperation.cpp
    src/core/fileexchange.cpp
    src/core/useragent.cpp
    src/core/cdpshims.cpp
    src/core/extensioncopy.cpp
    src/core/extensionhostwire.cpp
    src/core/extensionmainscripts.cpp
    src/core/extensionuserscripts.cpp
    src/core/extensionui.cpp
    src/core/dnrrules.cpp
    src/core/downloadname.cpp
    src/core/nativehistory.cpp
    src/ui/treebank.cpp
    src/ui/treebar.cpp
    src/ui/toolbar.cpp
    src/ui/notifier.cpp
    src/ui/minimap.cpp
    src/ui/nodepreview.cpp
    src/app/networkcontroller.cpp
    src/ui/theme.cpp
    src/core/settingsschema.cpp
    src/app/settingspage.cpp
    src/app/directorypage.cpp
    src/app/receiver.cpp
    src/app/transmitter.cpp
    src/ui/dialog.cpp
    src/view/view.cpp
    src/view/page.cpp
    src/view/webelement.cpp
    src/view/mediatype.cpp
    src/view/localview.cpp
    src/view/webengine/webenginepage.cpp
    src/view/webengine/webengineview.cpp
    src/view/webengine/quickwebengineview.cpp
    src/view/quicknativewebview.cpp
    src/view/edge/edgewebview.cpp
    src/view/edge/edgeenvironment.cpp
    src/view/edge/edgewebviewhandlers.cpp
    src/view/edge/edgewebviewinput.cpp
    src/view/edge/edgewebviewdragdrop.cpp
    src/view/edge/edgewebviewinspector.cpp
    src/view/edge/edgewebviewpage.cpp
    src/view/edge/edgewebviewprofile.cpp
    src/view/edge/edgewebviewstate.cpp
    src/view/edge/edgehostwindow.cpp
    src/view/edge/edgeeventsubscriptions.cpp
    src/gadgets/graphicstableview.cpp
    src/gadgets/gadgets.cpp
    src/gadgets/gadgetsstyle.cpp
    src/gadgets/abstractnodeitem.cpp
    src/gadgets/thumbnail.cpp
    src/gadgets/nodetitle.cpp
    src/gadgets/accessiblewebelement.cpp)

# everything but 'main.cpp' lives in a static library, so that the tests under
# 'tests/' can link against the same objects the executable is built from.
# the usage requirements are PUBLIC because the headers carry them: the include
# directories are used by '#include "view.hpp"' and friends, and the feature
# macros ('WEBENGINEVIEW' and so on) decide what the headers declare. a target
# compiled with a different set would see a different class layout.
qt_add_library(vanilla_core STATIC
    ${VANILLA_SOURCES}
    ${VANILLA_HEADERS})

# every '#include' in this project is a bare file name, so each directory
# holding a header is on the path. the repository root is there for 'VERSION',
# which the sources include as a header.
target_include_directories(vanilla_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view/edge
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view/webengine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gadgets)

target_compile_definitions(vanilla_core PUBLIC LOCALVIEW)

# saving and restoring the playback position of a page's video (A-1). on by
# default since D-063 追記 (2026-09-01): the cost -- a float on every view
# node and a field in the session file -- was accepted.
option(VANILLA_MEDIATIME "save and restore the playback position of videos" ON)
if(VANILLA_MEDIATIME)
    target_compile_definitions(vanilla_core PUBLIC MEDIATIME)
endif()

target_link_libraries(vanilla_core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Xml Qt6::Network
    Qt6::OpenGL Qt6::OpenGLWidgets
    Qt6::WebChannel
    Qt6::Multimedia Qt6::MultimediaWidgets
    Qt6::Qml Qt6::Quick Qt6::QuickWidgets)

# a border on full screen OpenGL windows, so other windows show over them
# ('QWindowsApplication::setHasBorderInFullScreenDefault', application.cpp).
if(WIN32)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
    target_link_libraries(vanilla_core PUBLIC Qt6::GuiPrivate)
endif()

if(TARGET Qt6::PrintSupport)
    target_link_libraries(vanilla_core PUBLIC Qt6::PrintSupport)
endif()

# 'WebEngineQuick' provides the QML module which QuickWebEngineView loads,
# and 'QtWebEngineQuick::initialize()'.
if(TARGET Qt6::WebEngineWidgets)
    target_compile_definitions(vanilla_core PUBLIC WEBENGINEVIEW)
    target_link_libraries(vanilla_core PUBLIC
        Qt6::WebEngineCore Qt6::WebEngineWidgets Qt6::WebEngineQuick)
else()
    message(WARNING "Qt WebEngine was not found. building without the default view.")
endif()

# 'WebView' is the platform's own web view -- the system one on Android and
# iOS, and Qt WebEngine underneath on the desktop. It used to be built only
# for the two mobile targets, which meant the desktop had no second engine to
# fall back on; it is built wherever the module is installed now (D-062).
find_package(Qt6 6.5 QUIET COMPONENTS WebView)
if(TARGET Qt6::WebView)
    target_compile_definitions(vanilla_core PUBLIC NATIVEWEBVIEW)
    target_link_libraries(vanilla_core PUBLIC Qt6::WebView)
else()
    message(STATUS "Qt WebView was not found. building without QuickNativeWebView.")
endif()

# 'EdgeWebView' hosts Edge WebView2 itself rather than through Qt WebView, so
# that the host side callbacks exist at all (C-7, D-166). Windows and x64 only,
# and only when the vendored SDK is here: without it the view is left out and
# everything else still builds.
set(VANILLA_WEBVIEW2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/webview2")
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND
   EXISTS "${VANILLA_WEBVIEW2_DIR}/include/WebView2.h" AND
   EXISTS "${VANILLA_WEBVIEW2_DIR}/x64/WebView2Loader.dll.lib")

    target_compile_definitions(vanilla_core PUBLIC EDGEWEBVIEW)
    target_include_directories(vanilla_core PUBLIC "${VANILLA_WEBVIEW2_DIR}/include")
    target_link_libraries(vanilla_core PUBLIC
        "${VANILLA_WEBVIEW2_DIR}/x64/WebView2Loader.dll.lib")

    set(VANILLA_WEBVIEW2_LOADER "${VANILLA_WEBVIEW2_DIR}/x64/WebView2Loader.dll")

    # visual hosting composes the page into a DirectComposition tree of
    # this side's own (C-7 e). 'shlwapi' comes with it for the stream the
    # backend writes its own screenshot into.
    # 'dwmapi' for the floating inspector dock's title bar colour (A-26).
    target_link_libraries(vanilla_core PUBLIC dcomp shlwapi dwmapi)
else()
    message(STATUS "the WebView2 SDK was not found. building without EdgeWebView.")
endif()

if(WIN32)
    # 'windows.h' defines 'min'/'max' as macros and they break Qt headers
    # such as <QtConcurrent> ('std::max' -> 'std::(...)').
    # PUBLIC: the headers of this project include 'windows.h' too.
    target_compile_definitions(vanilla_core PUBLIC NOMINMAX)
endif()

if(APPLE)
    target_sources(vanilla_core PRIVATE src/ui/mainwindowsettings.mm)
    target_link_libraries(vanilla_core PUBLIC "-framework AppKit")
endif()

# the '.qrc' files stay on the executable. a resource compiled into a static
# library registers itself from a static initializer, which the linker is free
# to drop when nothing else in that object is referenced.
qt_add_executable(vanilla WIN32 MACOSX_BUNDLE
    src/app/main.cpp
    qrc/vanilla.qrc)

target_link_libraries(vanilla PRIVATE vanilla_core)

if(TARGET Qt6::WebEngineWidgets)
    target_sources(vanilla PRIVATE qrc/quickwebengineview6.qrc)
endif()

if(TARGET Qt6::WebView)
    target_sources(vanilla PRIVATE qrc/quicknativewebview.qrc)
endif()

if(WIN32)
    # the '.pro' had to strip '-Zc:strictStrings', which the qmake mkspec adds.
    # nothing adds it here, so there is nothing to strip.
    target_sources(vanilla PRIVATE vanilla.rc)
endif()

if(APPLE)
    set(VANILLA_ICON "${CMAKE_CURRENT_SOURCE_DIR}/vanilla.icns")
    target_sources(vanilla PRIVATE ${VANILLA_ICON})
    set_source_files_properties(${VANILLA_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(vanilla PROPERTIES
        MACOSX_BUNDLE_ICON_FILE vanilla.icns
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
endif()

# translations.
#
# the '.qm' files are not embedded as resources: 'Application::BootApplication'
# loads them from a 'translations' directory next to the executable, together
# with Qt's own 'qt_*.qm' and 'qtbase_*.qm'.
#
# with no 'SOURCE_TARGETS', lupdate reads every target of this project, so the
# sources moved into 'vanilla_core' are still scanned. 'tests/' opts out with
# 'QT_EXCLUDE_FROM_TRANSLATION'.
set(VANILLA_QM_DIR "${CMAKE_CURRENT_BINARY_DIR}/translations")

#
# '-no-obsolete' drops the entries whose source string no longer exists. they
# are inert -- lrelease ignores them -- but they pile up (182 of them by the
# time this was added), and a '.ts' one can read is a '.ts' one can check.
# it also means a string lupdate fails to find is dropped rather than kept as
# obsolete, so after renaming or re-wording anything, look at the diff.
qt_add_translations(vanilla
    TS_FILES
        translations/vanilla_en.ts
        translations/vanilla_ja.ts
    SOURCES
        src/view/webengine/quickwebengineview6.qml
        src/view/quicknativewebview.qml
    LUPDATE_OPTIONS -no-obsolete
    QM_OUTPUT_DIRECTORY ${VANILLA_QM_DIR}
    QM_FILES_OUTPUT_VARIABLE VANILLA_QM_FILES)

# 'custom_*.ts' is written by hand (it translates strings coming from Qt, not
# from this project), so it is released but never handed to lupdate.
qt_add_lrelease(
    TS_FILES
        translations/custom_en.ts
        translations/custom_ja.ts
    QM_OUTPUT_DIRECTORY ${VANILLA_QM_DIR}
    QM_FILES_OUTPUT_VARIABLE VANILLA_CUSTOM_QM_FILES)

list(APPEND VANILLA_QM_FILES ${VANILLA_CUSTOM_QM_FILES})

# put them next to the executable, so that running from the build directory
# shows the translated UI.
add_custom_command(TARGET vanilla POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:vanilla>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${VANILLA_QM_FILES} "$<TARGET_FILE_DIR:vanilla>/translations"
    VERBATIM)

# 'WebView2Loader.dll' is the shim which finds the installed Edge runtime. it
# has to be beside the executable, in the build tree as well as in an install.
if(VANILLA_WEBVIEW2_LOADER)
    add_custom_command(TARGET vanilla POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VANILLA_WEBVIEW2_LOADER}" "$<TARGET_FILE_DIR:vanilla>"
        VERBATIM)

endif()

# tests.
#
# 'ctest --test-dir <builddir>' runs them. they link 'vanilla_core', so they
# exercise the very objects the executable is built from.
option(VANILLA_BUILD_TESTS "build the unit tests" ON)

if(VANILLA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# packaging.
#
# 'cmake --install <builddir> --prefix <somewhere>' produces a directory which
# runs on a machine with no Qt installed. everything is laid out flat next to
# the executable, because 'Application::BaseDirectory()' keeps 'data/' there.
if(APPLE)
    set(VANILLA_INSTALL_RUNTIME_DIR .)
    set(VANILLA_INSTALL_DATA_DIR vanilla.app/Contents/MacOS)
else()
    set(VANILLA_INSTALL_RUNTIME_DIR .)
    set(VANILLA_INSTALL_DATA_DIR .)
endif()

install(TARGETS vanilla
    BUNDLE  DESTINATION ${VANILLA_INSTALL_RUNTIME_DIR}
    RUNTIME DESTINATION ${VANILLA_INSTALL_RUNTIME_DIR})

install(FILES ${VANILLA_QM_FILES}
    DESTINATION ${VANILLA_INSTALL_DATA_DIR}/translations)

# the loader shim goes beside the executable, as it does in the build
# tree. without it the Edge view cannot start (C-7).
if(VANILLA_WEBVIEW2_LOADER)
    install(FILES "${VANILLA_WEBVIEW2_LOADER}"
        DESTINATION ${VANILLA_INSTALL_RUNTIME_DIR})
endif()

# the licences of what the package carries (D-241 gate 2). vanilla's own
# goes beside the executable.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION ${VANILLA_INSTALL_RUNTIME_DIR})

# the loader is Microsoft's binary, and its licence asks for the copyright
# notice to travel with it. a build which ships the loader without the
# notices does not configure -- the two cannot come apart by accident.
if(VANILLA_WEBVIEW2_LOADER)
    foreach(notice LICENSE.txt NOTICE.txt)
        if(NOT EXISTS "${VANILLA_WEBVIEW2_DIR}/${notice}")
            message(FATAL_ERROR
                "'${VANILLA_WEBVIEW2_DIR}/${notice}' is missing. "
                "'WebView2Loader.dll' may not ship without the SDK's notices.")
        endif()
    endforeach()

    install(FILES "${VANILLA_WEBVIEW2_DIR}/LICENSE.txt"
                  "${VANILLA_WEBVIEW2_DIR}/NOTICE.txt"
        DESTINATION ${VANILLA_INSTALL_RUNTIME_DIR}/licenses/webview2)
endif()

# the Qt libraries and Qt WebEngine travel with the package under the LGPLv3,
# which asks for its own text to travel with them. our own NOTICE names what
# ships, under which licence, and where its source is.
set(VANILLA_QT_LICENSE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/qt")

install(FILES "${VANILLA_QT_LICENSE_DIR}/NOTICE.txt"
              "${VANILLA_QT_LICENSE_DIR}/LGPL-3.0-only.txt"
              "${VANILLA_QT_LICENSE_DIR}/GPL-3.0-only.txt"
              "${VANILLA_QT_LICENSE_DIR}/LICENSE.Chromium"
    DESTINATION ${VANILLA_INSTALL_RUNTIME_DIR}/licenses/qt)

# the notice carries a version number, so it can go stale without anything
# else changing. a build against another Qt would ship a notice which does
# not describe what it carries.
file(READ "${VANILLA_QT_LICENSE_DIR}/NOTICE.txt" VANILLA_QT_NOTICE)
string(REGEX MATCH "Qt ([0-9]+\.[0-9]+\.[0-9]+)" _ "${VANILLA_QT_NOTICE}")
if(NOT CMAKE_MATCH_1 STREQUAL Qt6_VERSION)
    message(WARNING
        "'third_party/qt/NOTICE.txt' names Qt ${CMAKE_MATCH_1}, but this builds "
        "against Qt ${Qt6_VERSION}. update the notice and 'third_party/qt/README.md' "
        "before shipping this build.")
endif()

if(NOT APPLE)
    # the deployment defaults follow 'GNUInstallDirs' and would put the Qt
    # libraries in 'bin' while the executable sits at the top. keep everything
    # flat instead: the executable has to find the libraries next to itself,
    # and 'Application::BaseDirectory()' keeps 'data/' there as well.
    install(CODE "
        set(QT_DEPLOY_BIN_DIR \".\")
        set(QT_DEPLOY_LIBEXEC_DIR \".\")
    ")
endif()

# windeployqt / macdeployqt bring the Qt libraries, the plugins, the QML
# modules and the WebEngine resources. the '--qmldir' is what lets the tool
# see the imports of 'src/view/webengine/quickwebengineview6.qml', which lives in a resource
# rather than in a QML module.
set(VANILLA_DEPLOY_OPTIONS --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/src/view")

if(WIN32)
    # FFmpeg is not redistributed (D-241). '--no-ffmpeg' leaves its
    # libraries out, and the multimedia plugin which needs them has to go
    # with them -- what is left plays video through Windows Media.
    # A package that stays on this machine may keep them (D-241 addendum 6):
    # with the FFmpeg backend present, Qt Multimedia prefers it and
    # 'LocalView' plays what Windows Media cannot. OFF is the default so
    # that a distribution build cannot ship FFmpeg by forgetting a flag.
    option(VANILLA_PACKAGE_FFMPEG
        "Ship Qt Multimedia's FFmpeg backend in the package (local packages only; a distributed package excludes it, D-241)"
        OFF)
    if(NOT VANILLA_PACKAGE_FFMPEG)
        list(APPEND VANILLA_DEPLOY_OPTIONS
            --no-ffmpeg --exclude-plugins ffmpegmediaplugin)
    endif()

    # 'vc_redist.x64.exe' belongs in the package (D-241). the tool deploys
    # it by default, so this only says so out loud -- it does NOT make a
    # shell without 'VCINSTALLDIR' complain: measured on Qt 6.11.1
    # (2026-08-27), a bare shell says nothing either way. the package has
    # to be made in a vcvars shell and the file's presence checked.
    list(APPEND VANILLA_DEPLOY_OPTIONS --compiler-runtime)
endif()

qt_generate_deploy_app_script(
    TARGET vanilla
    OUTPUT_SCRIPT VANILLA_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS ${VANILLA_DEPLOY_OPTIONS})
install(SCRIPT ${VANILLA_DEPLOY_SCRIPT})

# windeployqt follows optional imports and copies several standalone QML
# modules which neither our three QML files nor WebEngine's control delegates
# require. Windows style imports Fusion and its Menu uses Effects, so those
# transitive dependencies stay in the package (D-297).
set(VANILLA_UNUSED_QML_MODULES
    QtQml/XmlListModel
    QtQuick/Controls/FluentWinUI3
    QtQuick/Controls/Imagine
    QtQuick/Controls/Material
    QtQuick/Controls/Universal
    QtQuick/Dialogs/quickimpl/qml/+Fusion
    QtQuick/Dialogs/quickimpl/qml/+Imagine
    QtQuick/Dialogs/quickimpl/qml/+Material
    QtQuick/Dialogs/quickimpl/qml/+Universal
    QtQuick/LocalStorage
    QtQuick/Particles
    QtQuick/Pdf
    QtQuick/Shapes
    QtQuick/tooling
    QtQuick/VectorImage)
foreach(module IN LISTS VANILLA_UNUSED_QML_MODULES)
    install(CODE "file(REMOVE_RECURSE \"\${CMAKE_INSTALL_PREFIX}/qml/${module}\")")
endforeach()

set(VANILLA_UNUSED_QML_LIBRARIES
    Qt6QuickControls2FluentWinUI3StyleImpl
    Qt6QuickControls2Imagine
    Qt6QuickControls2ImagineStyleImpl
    Qt6QuickControls2Material
    Qt6QuickControls2MaterialStyleImpl
    Qt6QuickControls2Universal
    Qt6QuickControls2UniversalStyleImpl
    Qt6QuickShapes)
foreach(library IN LISTS VANILLA_UNUSED_QML_LIBRARIES)
    install(CODE "file(REMOVE \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_SHARED_LIBRARY_PREFIX}${library}${CMAKE_SHARED_LIBRARY_SUFFIX}\")")
endforeach()
