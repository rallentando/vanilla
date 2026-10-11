#ifndef QTLOG_HPP
#define QTLOG_HPP

#include "switch.hpp"

#include <QString>
#include <QtGlobal>

namespace QtLog {

constexpr qint64 RotateLimit = 1024 * 1024;
constexpr qint64 RunLimit = 1024 * 1024;

bool Install(const QString &path);

void Uninstall();

bool Dropped(QtMsgType type, const QString &message);

QString Format(QtMsgType type, const QMessageLogContext &context, const QString &message);

}

#endif
