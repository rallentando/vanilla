#ifndef TREESERIALIZER_HPP
#define TREESERIALIZER_HPP

#include "switch.hpp"

#include <QString>
#include <QElapsedTimer>
#include <QDateTime>
#include <QTimeZone>
#include <QHash>

#include <functional>

class ViewNode;
class QTextStream;

namespace TreeSerializer {

class Hooks {

public:
    std::function<int(ViewNode*)> WindowIndexOf;
    std::function<void(ViewNode*, int)> RestoreIntoWindow;
    std::function<bool(ViewNode*)> KeepsSideFiles;
    std::function<void()> Tick;
    int TickIntervalMs = 16;

    int  WindowIndex(ViewNode *nd) const { return WindowIndexOf ? WindowIndexOf(nd) : 0;}
    void Restore(ViewNode *nd, int id) const { if(RestoreIntoWindow) RestoreIntoWindow(nd, id);}
    bool KeepSideFiles(ViewNode *nd) const { return KeepsSideFiles ? KeepsSideFiles(nd) : true;}
    void Tock() const {
        if(!Tick) return;
        if(TickIntervalMs > 0 && m_TickClock.isValid() && m_TickClock.elapsed() < TickIntervalMs) return;
        m_TickClock.start();
        Tick();
    }

private:
    mutable QElapsedTimer m_TickClock;
};

bool ReadJsonFile(const QString &path, ViewNode *root, const Hooks &hooks = Hooks());

bool WriteJsonFile(const QString &path, ViewNode *root, const Hooks &hooks = Hooks());

class StampReader {

public:
    explicit StampReader(const QTimeZone &zone);
    QDateTime Stamp(const QString &str);
    bool PlainDay(const QDate &day);

private:
    struct Day {
        QDateTime midnight;
        bool plain = false;
    };
    const Day &Resolve(const QDate &day);

    QTimeZone m_Zone;
    bool m_Fixed = false;
    bool m_Listable = false;
    QHash<qint64, Day> m_Days;
};

void WriteNode(ViewNode *nd, QTextStream &out, int depth, const Hooks &hooks = Hooks());
QString JsonEscape(QString str);

int ReadNodeCount();
int ReadViewCount();
void ResetReadCounters();

}

#endif
