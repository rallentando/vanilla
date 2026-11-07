#ifndef PERMISSIONTABLE_HPP
#define PERMISSIONTABLE_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QUrl>

class PermissionTable {
public:
    enum State { Ask = 0, Allow, Block };

    struct Row {
        QString origin;
        QString kind;
        State state;
    };

    static QString Origin(const QUrl &url);
    static QString Origin(const QString &spelled){ return Origin(QUrl(spelled));}
    static QString OriginFromUserText(const QString &text);

    static QStringList Kinds();
    static bool IsKind(const QString &kind){ return Kinds().contains(kind);}

    static QString StateName(State state);
    static State StateFromName(const QString &name);

    static State Both(State a, State b);

    static PermissionTable FromJson(const QByteArray &json, bool *broken);
    QByteArray ToJson() const;

    State Lookup(const QString &origin, const QString &kind) const;
    bool Set(const QString &origin, const QString &kind, State state);
    bool Remove(const QString &origin, const QString &kind){ return Set(origin, kind, Ask);}
    bool Clear();

    QList<Row> Rows() const;
    bool IsEmpty() const { return m_Rows.isEmpty();}

    QList<Row> Changes(const QList<Row> &held, const QStringList &storable) const;

    QList<Row> Delta(const PermissionTable &before) const;

private:
    QMap<QPair<QString, QString>, State> m_Rows;
};

#endif
