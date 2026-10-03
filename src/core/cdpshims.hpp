#ifndef CDPSHIMS_HPP
#define CDPSHIMS_HPP

#include "switch.hpp"

#include <QString>
#include <QJsonArray>

namespace Cdp {

QString WorkerShim();
QString ContentShim();
QString PageShim();

QString RelayScript();

QString UserScriptPrelude(const QString &extensionId, const QByteArray &secret);

QString WakeScript();
QString MenuChosenScript(const QJsonArray &args);

}

#endif
