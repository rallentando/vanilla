#ifndef NATIVEHISTORY_HPP
#define NATIVEHISTORY_HPP

#include "switch.hpp"

#include <QList>
#include <QUrl>
#include <QByteArray>

class QuickNativeLoadTail {
public:
    QuickNativeLoadTail() : m_Owed(false) {}

    void Started(){ m_Owed = true;}

    bool Take(bool listIsWaiting = false){
        const bool owed = m_Owed;
        m_Owed = false;
        return owed || listIsWaiting;
    }

    bool Owed() const { return m_Owed;}

    void Forget(){ m_Owed = false;}

private:
    bool m_Owed;
};

class NativeHistory {

public:
    enum MoveKind {
        NoMove,
        ImmediateMove,
        LoadMove
    };

    struct Request {
        MoveKind kind;
        int target;
        QUrl url;
    };

    NativeHistory();

    int Count() const { return m_Entries.count();}
    int Index() const { return m_Index;}
    bool IsEmpty() const { return m_Entries.isEmpty();}
    QUrl UrlAt(int index) const;
    QUrl CurrentUrl() const { return UrlAt(m_Index);}
    QList<QUrl> Entries() const { return m_Entries;}

    bool CanGoBack() const { return m_Index > 0;}
    bool CanGoForward() const { return m_Index >= 0 && m_Index < m_Entries.count() - 1;}

    void Visit(const QUrl &url);

    Request RequestBack(bool loading);
    Request RequestForward(bool loading);
    Request RequestRewind(bool loading);
    Request RequestFastForward(bool loading);

    void Release();

    static bool StartsALoad(const QUrl &target, const QUrl &current);

    MoveKind Pending() const { return m_Pending;}
    int PendingTarget() const { return m_PendingTarget;}
    bool Busy(bool loading) const {
        return loading || m_Pending != NoMove;
    }

    QByteArray Serialize() const;
    static bool Deserialize(const QByteArray &data, NativeHistory *out);
    static bool LooksLikeOurs(const QByteArray &data);

    static int MaxEntries() { return 50;}

    static qint32 Marker() { return 0x564E5648;}
    static qint32 Version() { return 1;}

private:
    QList<QUrl> m_Entries;
    int m_Index;
    MoveKind m_Pending;
    int m_PendingTarget;

    Request Start(int target);
};

#endif
