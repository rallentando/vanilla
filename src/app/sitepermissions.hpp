#ifndef SITEPERMISSIONS_HPP
#define SITEPERMISSIONS_HPP

#include "switch.hpp"

#include <QString>
#include <QUrl>

#include "permissiontable.hpp"

class SitePermissions {
public:
    static PermissionTable::State Lookup(const QUrl &origin, const QString &kind);

    static void Remember(const QUrl &origin, const QStringList &kinds,
                         PermissionTable::State state, bool offTheRecord);

    static bool Set(const QString &origin, const QString &kind, PermissionTable::State state);
    static bool Clear();

    static const PermissionTable &Table();

private:
    static void Changed(const PermissionTable &before);
};

#endif
