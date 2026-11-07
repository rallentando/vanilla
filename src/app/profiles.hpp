#ifndef PROFILES_HPP
#define PROFILES_HPP

#include "switch.hpp"

#include <QJsonObject>
#include <QString>

#include "profilelist.hpp"

namespace Profiles {

    QList<ProfileList::Row> Rows();
    QString PathOf(const ProfileList::Folder &folder);

    QJsonObject Describe();

    QString Remove(const QString &id);
    QString RemoveAll();
    bool Removing();
}

#endif
