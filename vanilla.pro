lessThan(QT_MAJOR_VERSION, 5){
    error(please use Qt 5.5 or newer.)
}
equals(QT_MAJOR_VERSION, 5) : lessThan(QT_MINOR_VERSION, 5){
    error(please use Qt 5.5 or newer.)
}

result = $$system("gosh $$PWD/scripts/GenerateWebView.scm")

equals(result, "success"){
    message("Source Generate Finished.")
} else {
    error("Source Generate Failed.")
}

INCLUDEPATH += . view gen gadgets

QT += \
    core gui xml network opengl webchannel \
    printsupport widgets \
    multimedia multimediawidgets \
    webengine webenginewidgets \
    quick quickwidgets

lessThan(QT_MINOR_VERSION, 6){
    QT += webkit webkitwidgets
}

win32: QT += winextras axcontainer

win32: QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings

CONFIG += qt

PROJECTNAME = vanilla

RC_FILE = vanilla.rc
RESOURCES += vanilla.qrc
FORMS +=

TARGET = vanilla

HEADERS += \
    application.hpp \
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
    view/localview.hpp \
    gen/webpage.hpp \
    gen/webview.hpp \
    gen/graphicswebview.hpp \
    gen/quickwebview.hpp \
    gen/webenginepage.hpp \
    gen/webengineview.hpp \
    gen/quickwebengineview.hpp \
    gadgets/graphicstableview.hpp \
    gadgets/gadgets.hpp \
    gadgets/gadgetsstyle.hpp \
    gadgets/abstractnodeitem.hpp \
    gadgets/thumbnail.hpp \
    gadgets/nodetitle.hpp \
    gadgets/accessiblewebelement.hpp
win32: HEADERS += \
    view/tridentview.hpp

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
    view/localview.cpp \
    gen/webpage.cpp \
    gen/webview.cpp \
    gen/graphicswebview.cpp \
    gen/quickwebview.cpp \
    gen/webenginepage.cpp \
    gen/webengineview.cpp \
    gen/quickwebengineview.cpp \
    gadgets/graphicstableview.cpp \
    gadgets/gadgets.cpp \
    gadgets/gadgetsstyle.cpp \
    gadgets/abstractnodeitem.cpp \
    gadgets/thumbnail.cpp \
    gadgets/nodetitle.cpp \
    gadgets/accessiblewebelement.cpp
win32: SOURCES += \
    view/tridentview.cpp

TRANSLATIONS += \
    translations/vanilla_en.ts \
    translations/vanilla_ja.ts

OTHER_FILES += \
    gen/quickwebview.qml \
    gen/quickwebengineview.qml

lupdate_only {

    ## lupdate cannot capture 'tr()' for translations.
    SOURCES = \
        gen/quickwebview.qml \
        gen/quickwebengineview.qml \
        application.hpp \
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
        view/localview.hpp \
        gen/webpage.hpp \
        gen/webview.hpp \
        gen/graphicswebview.hpp \
        gen/quickwebview.hpp \
        gen/webenginepage.hpp \
        gen/webengineview.hpp \
        gen/quickwebengineview.hpp \
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
        view/localview.cpp \
        gen/webpage.cpp \
        gen/webview.cpp \
        gen/graphicswebview.cpp \
        gen/quickwebview.cpp \
        gen/webenginepage.cpp \
        gen/webengineview.cpp \
        gen/quickwebengineview.cpp \
        gadgets/graphicstableview.cpp \
        gadgets/gadgets.cpp \
        gadgets/gadgetsstyle.cpp \
        gadgets/abstractnodeitem.cpp \
        gadgets/thumbnail.cpp \
        gadgets/nodetitle.cpp \
        gadgets/accessiblewebelement.cpp
    win32: SOURCES += \
        view/tridentview.hpp \
        view/tridentview.cpp
}
