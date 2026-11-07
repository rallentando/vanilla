#ifndef CDPSHIMS_HPP
#define CDPSHIMS_HPP

#include "switch.hpp"

#include <QString>
#include <QStringList>
#include <QJsonArray>

namespace Cdp {

QString WorkerShim();
QString WorkerShim(const QStringList &permissions, bool askedAlarms = false);
QString ContentShim();
QString PageShim(bool askedAlarms = false);

QString RelayScript();

QString UserScriptPrelude(const QString &extensionId, const QByteArray &secret);

QString WakeScript();
QString MenuChosenScript(const QJsonArray &args);

}

#endif
