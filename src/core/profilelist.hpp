#ifndef PROFILELIST_HPP
#define PROFILELIST_HPP

#include "switch.hpp"

#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

#include <functional>

namespace ProfileList {

    enum Kind { Widget, WidgetCache, Quick, QuickCache, Edge };

    struct Folder {
        Kind kind;
        QString storage;
    };

    struct Row {
        QString id;
        QString name;
        bool used = false;
        bool held = false;
        QList<Folder> folders;
    };

    struct Input {
        QMap<Kind, QStringList> present;
        QStringList usedNames;
        QStringList knownNames;
        QSet<QString> heldStorages;
        QStringList heldNames;
        std::function<QString(const QString &id)> storageName;
    };

    bool IsDigest(const QString &name);

    QList<Row> Rows(const Input &input);

    const Row *Find(const QList<Row> &rows, const QString &id);
}

#endif
