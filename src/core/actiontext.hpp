#ifndef ACTIONTEXT_HPP
#define ACTIONTEXT_HPP

#include "switch.hpp"

#include <QString>
#include <QStringList>

namespace ActionText {

QString Source(const QString &name);

QString Label(const QString &name);

QString MenuLabel(const QString &name);

QStringList Names();

}

#endif
