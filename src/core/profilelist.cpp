#include "switch.hpp"

#include "profilelist.hpp"

#include <QRegularExpression>

namespace ProfileList {

bool IsDigest(const QString &name){
    static const QRegularExpression digest(QStringLiteral("\\A[0-9a-f]{32}\\z"));
    return digest.match(name).hasMatch();
}

QList<Row> Rows(const Input &input){
    QMap<QString, QList<Kind>> at;
    for(auto it = input.present.cbegin(); it != input.present.cend(); ++it)
        foreach(const QString &storage, it.value())
            if(IsDigest(storage) && !at[storage].contains(it.key())) at[storage] << it.key();

    QSet<QString> protectedStorages;
    foreach(const QString &name, input.usedNames){
        protectedStorages << input.storageName(name)
                          << input.storageName(QStringLiteral("quick:") + name);
    }

    QSet<QString> heldStorages = input.heldStorages;
    foreach(const QString &name, input.heldNames){
        heldStorages << input.storageName(name)
                     << input.storageName(QStringLiteral("quick:") + name);
    }

    QList<Row> rows;
    QSet<QString> claimed;
    QStringList names = input.usedNames;
    foreach(const QString &name, input.knownNames) if(!names.contains(name)) names << name;
    for(const QString &name : std::as_const(names)){
        const QString widget = input.storageName(name);
        const QString quick = input.storageName(QStringLiteral("quick:") + name);
        Row row;
        row.id = widget;
        row.name = name;
        for(Kind kind : {Widget, WidgetCache, Edge})
            if(at.value(widget).contains(kind)) row.folders << Folder{kind, widget};
        for(Kind kind : {Quick, QuickCache})
            if(at.value(quick).contains(kind == Quick ? Quick : WidgetCache))
                row.folders << Folder{kind, quick};
        if(row.folders.isEmpty()) continue;
        claimed << widget << quick;
        rows << row;
    }
    QStringList rest = at.keys();
    rest.sort();
    for(const QString &storage : std::as_const(rest)){
        if(claimed.contains(storage)) continue;
        Row row;
        row.id = storage;
        foreach(Kind kind, at.value(storage)) row.folders << Folder{kind, storage};
        rows << row;
    }
    for(Row &row : rows){
        row.used = input.usedNames.contains(row.name);
        foreach(const Folder &folder, row.folders){
            if(protectedStorages.contains(folder.storage)) row.used = true;
            if(heldStorages.contains(folder.storage)) row.held = true;
        }
    }
    return rows;
}

const Row *Find(const QList<Row> &rows, const QString &id){
    if(!IsDigest(id)) return nullptr;
    for(const Row &row : rows) if(row.id == id) return &row;
    return nullptr;
}

}
