#ifndef SWITCH_HPP
#define SWITCH_HPP

#include <QtCore>

#if defined(Q_OS_WINRT) || defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#  define NATIVEWEBVIEW
#else
#  if defined(Q_OS_WIN)
#    define TRIDENTVIEW
#  endif
#  define WEBENGINEVIEW
#endif

#define LOCALVIEW

// for multi line QStringLiteral
#if defined(Q_CC_MSVC)
#  define VV L
#else
#  define VV ""
#endif

#define QT_ASCII_CAST_WARNINGS

#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII
#define QT_NO_CAST_FROM_BYTEARRAY
#define QT_NO_URL_CAST_FROM_STRING

// 'QObject' for deletion of hist node.
// but that was problem of drawing thumbnail and nodetitle.
// (they had accessed deleted node, when displaying them.)
// so the problem is resolved now,
// by clearing thumbnail and nodetitle before deleting node.
#define USE_LIGHTNODE

//#define USE_ANGLE

//#define USE_WEBCHANNEL

#define PASSWORD_MANAGER

#define FAST_SAVER

#endif //ifndef SWITCH_HPP
