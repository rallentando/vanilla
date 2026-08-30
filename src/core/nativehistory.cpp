#include "nativehistory.hpp"

#include <QDataStream>

NativeHistory::NativeHistory()
    : m_Entries(QList<QUrl>())
    , m_Index(-1)
    , m_Pending(NoMove)
    , m_PendingTarget(-1)
{
}

QUrl NativeHistory::UrlAt(int index) const {
    if(index < 0 || index >= m_Entries.count()) return QUrl();
    return m_Entries[index];
}

void NativeHistory::Visit(const QUrl &url){
    if(url.isEmpty() || !url.isValid()) return;

    if(m_Pending != NoMove && url == UrlAt(m_PendingTarget)){
        m_Index = m_PendingTarget;
        m_Pending = NoMove;
        m_PendingTarget = -1;
        return;
    }

    if(m_Index >= 0 && m_Entries[m_Index] == url) return;

    Release();

    while(m_Entries.count() > m_Index + 1) m_Entries.removeLast();
    m_Entries.append(url);
    while(m_Entries.count() > MaxEntries()) m_Entries.removeFirst();
    m_Index = m_Entries.count() - 1;
}

NativeHistory::Request NativeHistory::Start(int target){
    if(UrlAt(target) == CurrentUrl()){
        m_Index = target;
        return Request{ImmediateMove, target, UrlAt(target)};
    }

    m_Pending = LoadMove;
    m_PendingTarget = target;
    return Request{LoadMove, target, UrlAt(target)};
}

NativeHistory::Request NativeHistory::RequestBack(bool loading){
    if(Busy(loading) || !CanGoBack()) return Request{NoMove, -1, QUrl()};
    return Start(m_Index - 1);
}

NativeHistory::Request NativeHistory::RequestForward(bool loading){
    if(Busy(loading) || !CanGoForward()) return Request{NoMove, -1, QUrl()};
    return Start(m_Index + 1);
}

NativeHistory::Request NativeHistory::RequestRewind(bool loading){
    if(Busy(loading) || !CanGoBack()) return Request{NoMove, -1, QUrl()};
    return Start(0);
}

NativeHistory::Request NativeHistory::RequestFastForward(bool loading){
    if(Busy(loading) || !CanGoForward()) return Request{NoMove, -1, QUrl()};
    return Start(m_Entries.count() - 1);
}

void NativeHistory::Release(){
    m_Pending = NoMove;
    m_PendingTarget = -1;
}

bool NativeHistory::StartsALoad(const QUrl &target, const QUrl &current){
    return target.isValid() && !target.isEmpty() && target != current;
}

QByteArray NativeHistory::Serialize() const {
    if(m_Entries.isEmpty()) return QByteArray();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << Marker() << Version()
           << static_cast<qint32>(m_Index)
           << static_cast<qint32>(m_Entries.count());
    foreach(const QUrl &url, m_Entries) stream << url;
    return data;
}

bool NativeHistory::Deserialize(const QByteArray &data, NativeHistory *out){
    if(!out || data.isEmpty()) return false;

    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_0);

    qint32 marker = 0, version = 0, index = 0, count = 0;
    stream >> marker >> version >> index >> count;
    if(stream.status() != QDataStream::Ok) return false;
    if(marker != Marker() || version != Version()) return false;
    if(count <= 0 || count > MaxEntries()) return false;
    if(index < 0 || index >= count) return false;

    QList<QUrl> entries;
    for(int i = 0; i < count; i++){
        QUrl url;
        stream >> url;
        if(stream.status() != QDataStream::Ok) return false;
        if(url.isEmpty() || !url.isValid()) return false;
        if(!entries.isEmpty() && entries.last() == url) return false;
        entries.append(url);
    }
    if(!stream.atEnd()) return false;

    out->m_Entries = entries;
    out->m_Index = index;
    out->m_Pending = NoMove;
    out->m_PendingTarget = -1;
    return true;
}

bool NativeHistory::LooksLikeOurs(const QByteArray &data){
    NativeHistory history;
    return Deserialize(data, &history);
}
