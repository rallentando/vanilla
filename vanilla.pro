lessThan(QT_MAJOR_VERSION, 5){
    error(please use Qt 5.9 or newer.)
}
equals(QT_MAJOR_VERSION, 5) : lessThan(QT_MINOR_VERSION, 9){
    error(please use Qt 5.9 or newer.)
}

QT += \
    xml network opengl \
    webchannel widgets \
    multimedia multimediawidgets \
    quick quickwidgets qml

exists($(QTDIR)/lib/*Qt*PrintSupport*) {
    QT += printsupport
}

exists($(QTDIR)/lib/*Qt*WebEngine*) {

    DEFINES += WEBENGINEVIEW
    QT += webenginecore webengine webenginewidgets

    greaterThan(QT_MINOR_VERSION, 10){
        RESOURCES += qrc/quickwebengineview5.11.qrc
        OTHER_FILES += view/quickwebengineview5.11.qml
    } else:greaterThan(QT_MINOR_VERSION, 9){
        RESOURCES += qrc/quickwebengineview5.10.qrc
        OTHER_FILES += view/quickwebengineview5.10.qml
    } else {
        RESOURCES += qrc/quickwebengineview5.9.qrc
        OTHER_FILES += view/quickwebengineview5.9.qml
    }
}

exists($(QTDIR)/lib/*Qt*WebKit*) {

    DEFINES += WEBKITVIEW
    QT += webkit webkitwidgets

    RESOURCES += qrc/quickwebkitview.qrc
    OTHER_FILES += view/quickwebkitview.qml
}

winrt | android | ios {

    DEFINES += NATIVEWEBVIEW
    QT += webview

    RESOURCES += qrc/quicknativewebview.qrc
    OTHER_FILES += view/quicknativewebview.qml

} else:win32 {

    DEFINES += TRIDENTVIEW
    QT += winextras axcontainer
}

DEFINES += LOCALVIEW

INCLUDEPATH += . view gadgets

win32 {
    QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CXXFLAGS -= -Zc:strictStrings
    QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CFLAGS -= -Zc:strictStrings
}

CONFIG += qt

PROJECTNAME = vanilla

RESOURCES += qrc/vanilla.qrc

win32 {
    RC_FILE = vanilla.rc
}
mac {
    ICON = vanilla.icns
}

FORMS +=

TARGET = vanilla
TEMPLATE = app

HEADERS += \
    application.hpp \
    actionmapper.hpp \
    mainwindow.hpp \
    saver.hpp \
    node.hpp \
    lightnode.hpp \
    jsobject.hpp \
    treebank.hpp \
    treebar.hpp \
    toolbar.hpp \
    notifier.hpp \
    networkcontroller.hpp \
    switch.hpp \
    callback.hpp \
    const.hpp \
    keymap.hpp \
    mousemap.hpp \
    receiver.hpp \
    transmitter.hpp \
    dialog.hpp \
    view/view.hpp \
    view/page.hpp \
    view/webelement.hpp \
    view/localview.hpp \
    view/webenginepage.hpp \
    view/webengineview.hpp \
    view/quickwebengineview.hpp \
    view/webkitpage.hpp \
    view/webkitview.hpp \
    view/graphicswebkitview.hpp \
    view/quickwebkitview.hpp \
    view/quicknativewebview.hpp \
    view/tridentview.hpp \
    gadgets/graphicstableview.hpp \
    gadgets/gadgets.hpp \
    gadgets/gadgetsstyle.hpp \
    gadgets/abstractnodeitem.hpp \
    gadgets/thumbnail.hpp \
    gadgets/nodetitle.hpp \
    gadgets/accessiblewebelement.hpp

SOURCES += \
    main.cpp \
    application.cpp \
    mainwindow.cpp \
    saver.cpp \
    node.cpp \
    lightnode.cpp \
    treebank.cpp \
    treebar.cpp \
    toolbar.cpp \
    notifier.cpp \
    networkcontroller.cpp \
    receiver.cpp \
    transmitter.cpp \
    dialog.cpp \
    view/view.cpp \
    view/page.cpp \
    view/webelement.cpp \
    view/localview.cpp \
    view/webenginepage.cpp \
    view/webengineview.cpp \
    view/quickwebengineview.cpp \
    view/webkitpage.cpp \
    view/webkitview.cpp \
    view/graphicswebkitview.cpp \
    view/quickwebkitview.cpp \
    view/quicknativewebview.cpp \
    view/tridentview.cpp \
    gadgets/graphicstableview.cpp \
    gadgets/gadgets.cpp \
    gadgets/gadgetsstyle.cpp \
    gadgets/abstractnodeitem.cpp \
    gadgets/thumbnail.cpp \
    gadgets/nodetitle.cpp \
    gadgets/accessiblewebelement.cpp

TRANSLATIONS += \
    translations/vanilla_en.ts \
    translations/vanilla_ja.ts

lupdate_only {

    ## lupdate cannot capture 'tr()' for translations.
    SOURCES = \
        view/quickwebengineview5.9.qml \
        view/quickwebengineview5.10.qml \
        view/quickwebengineview5.11.qml \
        view/quickwebkitview.qml \
        view/quicknativewebview.qml \
        application.hpp \
        actionmapper.hpp \
        mainwindow.hpp \
        saver.hpp \
        node.hpp \
        lightnode.hpp \
        jsobject.hpp \
        treebank.hpp \
        treebar.hpp \
        toolbar.hpp \
        notifier.hpp \
        networkcontroller.hpp \
        switch.hpp \
        callback.hpp \
        const.hpp \
        keymap.hpp \
        mousemap.hpp \
        receiver.hpp \
        transmitter.hpp \
        dialog.hpp \
        view/view.hpp \
        view/page.hpp \
        view/webelement.hpp \
        view/localview.hpp \
        view/webenginepage.hpp \
        view/webengineview.hpp \
        view/quickwebengineview.hpp \
        view/webkitpage.hpp \
        view/webkitview.hpp \
        view/graphicswebkitview.hpp \
        view/quickwebkitview.hpp \
        view/quicknativewebview.hpp \
        gadgets/graphicstableview.hpp \
        gadgets/gadgets.hpp \
        gadgets/gadgetsstyle.hpp \
        gadgets/abstractnodeitem.hpp \
        gadgets/thumbnail.hpp \
        gadgets/nodetitle.hpp \
        gadgets/accessiblewebelement.hpp \
        main.cpp \
        application.cpp \
        mainwindow.cpp \
        saver.cpp \
        node.cpp \
        lightnode.cpp \
        treebank.cpp \
        treebar.cpp \
        toolbar.cpp \
        notifier.cpp \
        networkcontroller.cpp \
        receiver.cpp \
        transmitter.cpp \
        dialog.cpp \
        view/view.cpp \
        view/page.cpp \
        view/webelement.cpp \
        view/localview.cpp \
        view/webenginepage.cpp \
        view/webengineview.cpp \
        view/quickwebengineview.cpp \
        view/webkitpage.cpp \
        view/webkitview.cpp \
        view/graphicswebkitview.cpp \
        view/quickwebkitview.cpp \
        view/quicknativewebview.cpp \
        view/tridentview.hpp \
        view/tridentview.cpp \
        gadgets/graphicstableview.cpp \
        gadgets/gadgets.cpp \
        gadgets/gadgetsstyle.cpp \
        gadgets/abstractnodeitem.cpp \
        gadgets/thumbnail.cpp \
        gadgets/nodetitle.cpp \
        gadgets/accessiblewebelement.cpp
}

mac {
    QMAKE_MAC_SDK = macosx
    LIBS += -framework AppKit

    OBJECTIVE_SOURCES += \
        mainwindowsettings.mm
}
