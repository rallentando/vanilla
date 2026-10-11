#ifndef PASSWORDPOLICY_HPP
#define PASSWORDPOLICY_HPP

#include "switch.hpp"

#include <QString>
#include <QUrl>

namespace PasswordPolicy {

QString Origin(const QUrl &url);

QString ScriptOrigin(const QUrl &url);

bool IsLoopbackHost(const QString &host);

QString VaultKey(const QString &prefix, const QString &space,
                 const QString &origin, const QString &username);

}

#endif
