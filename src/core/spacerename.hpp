#ifndef SPACERENAME_HPP
#define SPACERENAME_HPP

#include "switch.hpp"

#include <QList>
#include <QString>
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QPointer>
#include <QJsonObject>

#include <functional>

namespace SpaceRename {

    enum What {
        WidgetStorage, WidgetCache, QuickStorage, QuickCache, EdgeProfile,
        ExtensionState
    };
    struct Move {
        What what;
        QString from;
        QString to;
        bool operator==(const Move &other) const {
            return what == other.what && from == other.from && to == other.to;
        }
    };

    struct Places {
        QString widgetStorage;
        QString data;
        QString state;
        std::function<QString(const QString &id)> storageName;
        QString edgeProfiles;
        std::function<QString(const QString &profileName)> edgeJournal;
    };

    QList<Move> Candidates(const QString &from, const QString &to, const Places &places);

    QList<Move> Occupied(const QList<Move> &moves);

    struct Cookie {
        QString name;
        QString value;
        QString domain;
        QString path;
        double expires = -1;
        bool session = false;
        bool httpOnly = false;
        bool secure = false;
        int sameSite = 0;
    };
    QList<Cookie> CookiesToAdd(const QList<Cookie> &from, const QList<Cookie> &to);

    void WhenGone(const QList<QPointer<QObject>> &going, int limitMs,
                  std::function<void(bool)> done);

    bool ForgetMissingProfiles(QJsonObject &localState, std::function<bool(const QString&)> exists);

    bool BrowserHoldsFolder(const QString &folder);

    class Mover : public QObject {
    public:
        typedef std::function<bool(const QString &from, const QString &to)> MoveFunction;
        Mover(const QList<Move> &moves, MoveFunction move,
              int retryMs, int limitMs, QObject *parent = nullptr);
        void Start(std::function<void(const QList<Move> &failed)> done);
        static bool DefaultMove(const QString &from, const QString &to);
    private:
        void Step();
        QList<Move> m_Rest;
        QList<Move> m_Failed;
        MoveFunction m_Move;
        int m_RetryMs;
        int m_LimitMs;
        QElapsedTimer m_Trying;
        bool m_Started;
        std::function<void(const QList<Move>&)> m_Done;
    };
}

#endif
