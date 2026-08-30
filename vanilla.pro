lessThan(QT_MAJOR_VERSION, 6){
    error(please use Qt 6.)
}

QT += \
    xml network opengl openglwidgets \
    webchannel widgets \
    multimedia multimediawidgets \
    quick quickwidgets qml

## do not use 'exists($(QTDIR)/lib/...)' here.
## '$(QTDIR)' is expanded by the shell, so it silently fails
## when the QTDIR environment variable is not set.
qtHaveModule(printsupport) {
    QT += printsupport
}

qtHaveModule(webenginewidgets) {

    DEFINES += WEBENGINEVIEW
    ## 'webenginequick' provides the QML module which QuickWebEngineView
    ## loads, and 'QtWebEngineQuick::initialize()'.
    QT += webenginecore webenginewidgets webenginequick

    RESOURCES += qrc/quickwebengineview6.qrc
    OTHER_FILES += src/view/webengine/quickwebengineview6.qml
    OTHER_FILES += src/view/webengine/quickwebengineinspector6.qml
}

qtHaveModule(webview) {

    DEFINES += NATIVEWEBVIEW
    QT += webview

    RESOURCES += qrc/quicknativewebview.qrc
    OTHER_FILES += src/view/quicknativewebview.qml
}

## 'EdgeWebView' hosts Edge WebView2 itself rather than through Qt WebView
## (C-7, D-166). Windows only, and only when the vendored SDK is here;
## without it the view is left out and everything else still builds.
win32:exists($$PWD/third_party/webview2/include/WebView2.h) {

    DEFINES += EDGEWEBVIEW
    INCLUDEPATH += $$PWD/third_party/webview2/include
    LIBS += $$PWD/third_party/webview2/x64/WebView2Loader.dll.lib
    ## visual hosting (C-7 e).
    LIBS += -ldcomp -lshlwapi -ldwmapi

    ## the loader shim is what finds the installed Edge runtime, and has to
    ## sit next to the executable.
    edgeloader.files = $$PWD/third_party/webview2/x64/WebView2Loader.dll
    edgeloader.path = $$OUT_PWD
    COPIES += edgeloader
}

DEFINES += LOCALVIEW

# opt-in (mirrors the 'VANILLA_MEDIATIME' option of CMakeLists.txt):
# saving and restoring the playback position of a page's video (A-1).
#DEFINES += MEDIATIME

INCLUDEPATH += . src/core src/app src/input src/ui src/view src/view/edge src/view/webengine src/gadgets

win32 {
    QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CXXFLAGS -= -Zc:strictStrings
    QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CFLAGS -= -Zc:strictStrings

    ## 'windows.h' defines 'min'/'max' as macros and they break
    ## Qt headers such as <QtConcurrent> ('std::max' -> 'std::(...)').
    DEFINES += NOMINMAX
}

CONFIG += qt

PROJECTNAME = vanilla

RESOURCES += qrc/vanilla.qrc

win32 {
    RC_FILE = vanilla.rc
    # 'vanilla.rc' takes the version from <VERSION> at the repository root.
    RC_INCLUDEPATH += $$PWD
}
mac {
    ICON = vanilla.icns
}

FORMS +=

TARGET = vanilla
TEMPLATE = app

HEADERS += \
    src/app/application.hpp \
    src/input/actionmapper.hpp \
    src/ui/mainwindow.hpp \
    src/app/saver.hpp \
    src/core/lightnode.hpp \
    src/core/devicescale.hpp \
    src/input/jsobject.hpp \
    src/core/treeserializer.hpp \
    src/core/bookmarkio.hpp \
    src/core/settingsio.hpp \
    src/core/inputmap.hpp \
    src/core/commandmap.hpp \
    src/core/commandframe.hpp \
    src/core/certificatepolicy.hpp \
    src/core/fileoperation.hpp \
    src/core/fileexchange.hpp \
    src/core/windowledger.hpp \
    src/core/useragent.hpp \
    src/core/downloadname.hpp \
    src/core/nativehistory.hpp \
    src/ui/treebank.hpp \
    src/ui/treebar.hpp \
    src/ui/toolbar.hpp \
    src/ui/notifier.hpp \
    src/ui/minimap.hpp \
    src/ui/nodepreview.hpp \
    src/app/networkcontroller.hpp \
    src/core/switch.hpp \
    src/core/callback.hpp \
    src/core/const.hpp \
    src/ui/theme.hpp \
    src/core/settingsschema.hpp \
    src/app/settingspage.hpp \
    src/app/directorypage.hpp \
    src/input/keymap.hpp \
    src/input/mousemap.hpp \
    src/app/receiver.hpp \
    src/app/transmitter.hpp \
    src/ui/dialog.hpp \
    src/view/view.hpp \
    src/view/page.hpp \
    src/view/webelement.hpp \
    src/view/mediatype.hpp \
    src/view/localview.hpp \
    src/view/webengine/webenginepage.hpp \
    src/view/webengine/webengineview.hpp \
    src/view/webengine/quickwebengineview.hpp \
    src/view/quicknativewebview.hpp \
    src/view/edge/edgewebview.hpp \
    src/view/edge/edgewebview_p.hpp \
    src/view/edge/edgeeventsubscriptions.hpp \
    src/view/edge/edgeunadoptedcontroller.hpp \
    src/view/edge/edgewebviewstate.hpp \
    src/gadgets/graphicstableview.hpp \
    src/gadgets/gadgets.hpp \
    src/gadgets/gadgetsstyle.hpp \
    src/gadgets/abstractnodeitem.hpp \
    src/gadgets/thumbnail.hpp \
    src/gadgets/nodetitle.hpp \
    src/gadgets/accessiblewebelement.hpp

SOURCES += \
    src/app/main.cpp \
    src/app/application.cpp \
    src/ui/mainwindow.cpp \
    src/app/saver.cpp \
    src/core/lightnode.cpp \
    src/core/devicescale.cpp \
    src/core/treeserializer.cpp \
    src/core/bookmarkio.cpp \
    src/core/settingsio.cpp \
    src/core/inputmap.cpp \
    src/core/commandmap.cpp \
    src/core/commandframe.cpp \
    src/core/certificatepolicy.cpp \
    src/core/fileoperation.cpp \
    src/core/fileexchange.cpp \
    src/core/useragent.cpp \
    src/core/downloadname.cpp \
    src/core/nativehistory.cpp \
    src/ui/treebank.cpp \
    src/ui/treebar.cpp \
    src/ui/toolbar.cpp \
    src/ui/notifier.cpp \
    src/ui/minimap.cpp \
    src/ui/nodepreview.cpp \
    src/app/networkcontroller.cpp \
    src/ui/theme.cpp \
    src/core/settingsschema.cpp \
    src/app/settingspage.cpp \
    src/app/directorypage.cpp \
    src/app/receiver.cpp \
    src/app/transmitter.cpp \
    src/ui/dialog.cpp \
    src/view/view.cpp \
    src/view/page.cpp \
    src/view/webelement.cpp \
    src/view/mediatype.cpp \
    src/view/localview.cpp \
    src/view/webengine/webenginepage.cpp \
    src/view/webengine/webengineview.cpp \
    src/view/webengine/quickwebengineview.cpp \
    src/view/quicknativewebview.cpp \
    src/view/edge/edgewebview.cpp \
    src/view/edge/edgeenvironment.cpp \
    src/view/edge/edgewebviewhandlers.cpp \
    src/view/edge/edgewebviewinput.cpp \
    src/view/edge/edgewebviewdragdrop.cpp \
    src/view/edge/edgewebviewinspector.cpp \
    src/view/edge/edgewebviewpage.cpp \
    src/view/edge/edgewebviewprofile.cpp \
    src/view/edge/edgewebviewstate.cpp \
    src/view/edge/edgeeventsubscriptions.cpp \
    src/gadgets/graphicstableview.cpp \
    src/gadgets/gadgets.cpp \
    src/gadgets/gadgetsstyle.cpp \
    src/gadgets/abstractnodeitem.cpp \
    src/gadgets/thumbnail.cpp \
    src/gadgets/nodetitle.cpp \
    src/gadgets/accessiblewebelement.cpp

TRANSLATIONS += \
    translations/vanilla_en.ts \
    translations/vanilla_ja.ts

lupdate_only {

    ## lupdate cannot capture 'tr()' for translations.
    SOURCES = \
        src/view/webengine/quickwebengineview6.qml \
        src/view/quicknativewebview.qml \
        src/app/application.hpp \
        src/input/actionmapper.hpp \
        src/ui/mainwindow.hpp \
        src/app/saver.hpp \
        src/core/lightnode.hpp \
        src/core/devicescale.hpp \
        src/input/jsobject.hpp \
        src/core/treeserializer.hpp \
        src/core/bookmarkio.hpp \
        src/core/settingsio.hpp \
        src/core/inputmap.hpp \
        src/core/commandmap.hpp \
        src/core/commandframe.hpp \
        src/core/certificatepolicy.hpp \
        src/core/fileoperation.hpp \
        src/core/fileexchange.hpp \
        src/core/windowledger.hpp \
        src/core/useragent.hpp \
        src/core/downloadname.hpp \
        src/core/nativehistory.hpp \
        src/ui/treebank.hpp \
        src/ui/treebar.hpp \
        src/ui/toolbar.hpp \
        src/ui/notifier.hpp \
        src/ui/minimap.hpp \
        src/ui/nodepreview.hpp \
        src/app/networkcontroller.hpp \
        src/core/switch.hpp \
        src/core/callback.hpp \
        src/core/const.hpp \
        src/ui/theme.hpp \
        src/core/settingsschema.hpp \
        src/app/settingspage.hpp \
        src/app/directorypage.hpp \
        src/input/keymap.hpp \
        src/input/mousemap.hpp \
        src/app/receiver.hpp \
        src/app/transmitter.hpp \
        src/ui/dialog.hpp \
        src/view/view.hpp \
        src/view/page.hpp \
        src/view/webelement.hpp \
        src/view/mediatype.hpp \
        src/view/localview.hpp \
        src/view/webengine/webenginepage.hpp \
        src/view/webengine/webengineview.hpp \
        src/view/webengine/quickwebengineview.hpp \
        src/view/quicknativewebview.hpp \
        src/view/edge/edgewebview.hpp \
        src/view/edge/edgewebview_p.hpp \
        src/view/edge/edgeeventsubscriptions.hpp \
        src/view/edge/edgeunadoptedcontroller.hpp \
        src/view/edge/edgewebviewstate.hpp \
        src/gadgets/graphicstableview.hpp \
        src/gadgets/gadgets.hpp \
        src/gadgets/gadgetsstyle.hpp \
        src/gadgets/abstractnodeitem.hpp \
        src/gadgets/thumbnail.hpp \
        src/gadgets/nodetitle.hpp \
        src/gadgets/accessiblewebelement.hpp \
        src/app/main.cpp \
        src/app/application.cpp \
        src/ui/mainwindow.cpp \
        src/app/saver.cpp \
        src/core/lightnode.cpp \
        src/core/devicescale.cpp \
        src/core/treeserializer.cpp \
        src/core/bookmarkio.cpp \
        src/core/settingsio.cpp \
        src/core/inputmap.cpp \
        src/core/commandmap.cpp \
        src/core/commandframe.cpp \
        src/core/certificatepolicy.cpp \
        src/core/fileoperation.cpp \
        src/core/fileexchange.cpp \
        src/core/useragent.cpp \
        src/core/downloadname.cpp \
        src/core/nativehistory.cpp \
        src/ui/treebank.cpp \
        src/ui/treebar.cpp \
        src/ui/toolbar.cpp \
        src/ui/notifier.cpp \
        src/ui/minimap.cpp \
        src/ui/nodepreview.cpp \
        src/app/networkcontroller.cpp \
        src/ui/theme.cpp \
        src/core/settingsschema.cpp \
        src/app/settingspage.cpp \
        src/app/directorypage.cpp \
        src/app/receiver.cpp \
        src/app/transmitter.cpp \
        src/ui/dialog.cpp \
        src/view/view.cpp \
        src/view/page.cpp \
        src/view/webelement.cpp \
        src/view/mediatype.cpp \
        src/view/localview.cpp \
        src/view/webengine/webenginepage.cpp \
        src/view/webengine/webengineview.cpp \
        src/view/webengine/quickwebengineview.cpp \
        src/view/quicknativewebview.cpp \
        src/view/edge/edgewebview.cpp \
        src/view/edge/edgeenvironment.cpp \
        src/view/edge/edgewebviewhandlers.cpp \
        src/view/edge/edgewebviewinput.cpp \
        src/view/edge/edgewebviewdragdrop.cpp \
        src/view/edge/edgewebviewinspector.cpp \
        src/view/edge/edgewebviewpage.cpp \
        src/view/edge/edgewebviewprofile.cpp \
        src/view/edge/edgewebviewstate.cpp \
        src/view/edge/edgeeventsubscriptions.cpp \
        src/gadgets/graphicstableview.cpp \
        src/gadgets/gadgets.cpp \
        src/gadgets/gadgetsstyle.cpp \
        src/gadgets/abstractnodeitem.cpp \
        src/gadgets/thumbnail.cpp \
        src/gadgets/nodetitle.cpp \
        src/gadgets/accessiblewebelement.cpp
}

mac {
    QMAKE_MAC_SDK = macosx
    LIBS += -framework AppKit

    OBJECTIVE_SOURCES += \
        src/ui/mainwindowsettings.mm
}
