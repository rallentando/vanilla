#ifndef SWITCH_HPP
#define SWITCH_HPP

#include <QtCore>

#if defined(Q_OS_MAC)
#  define DECL_OVERRIDE
#else
#  define DECL_OVERRIDE override
#endif

// for multi line QStringLiteral
#if defined(Q_CC_MSVC)
#  define VV L
#else
#  define VV ""
#endif

#if QT_VERSION < 0x050600
#  define QTWEBKIT
#endif

#define QT_ASCII_CAST_WARNINGS

// 'QObject' for deletion of hist node.
// but that was problem of drawing thumbnail and nodetitle.
// (they had accessed deleted node, when displaying them.)
// so the problem is resolved now,
// by clearing thumbnail and nodetitle before deleting node.
#define USE_LIGHTNODE

//#define OUT_OF_THREAD_UPDATE

//#define WEBENGINE_DRAGDROP

//#define WEBENGINEVIEW_DEFAULT

// cannot render QuickWeb(Engine)View without this flag.
// crash when calling QWindow::grabWindow.
#define USE_QQUICKWIDGET

//#define USE_ANGLE

//#define USE_WEBCHANNEL

#define FAST_SAVER

#endif
