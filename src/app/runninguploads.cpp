#include "switch.hpp"
#include "const.hpp"

#include "runninguploads.hpp"

#include <QTimer>

RunningUploads *RunningUploads::Instance(){
    static RunningUploads *instance = new RunningUploads(UPLOAD_ROW_DELAY);
    return instance;
}

RunningUploads::RunningUploads(int delay, QObject *parent)
    : QObject(parent)
    , m_Delay(delay)
    , m_NextId(1)
{
}

bool RunningUploads::IsMultipart(const QByteArray &contentType){
    return contentType.trimmed().toLower().startsWith(QByteArrayLiteral("multipart/form-data"));
}

void RunningUploads::Started(const View *key, const QUrl &url, const std::function<void()> &stop){
    foreach(const Running &running, m_Running)
        if(running.key == key) return;
    Running running;
    running.id = m_NextId++;
    running.key = key;
    running.url = url;
    running.stop = stop;
    m_Running << running;

    const int id = running.id;
    QTimer::singleShot(m_Delay, Qt::PreciseTimer, this, [this, id](){
        for(int i = 0; i < m_Running.size(); i++){
            if(m_Running[i].id != id) continue;
            m_Running[i].shown = true;
            emit Changed();
            emit Appeared();
            return;
        }
    });
}

bool RunningUploads::Ended(const View *key){
    for(int i = 0; i < m_Running.size(); i++){
        if(m_Running[i].key != key) continue;
        const bool shown = m_Running[i].shown;
        m_Running.removeAt(i);
        if(shown) emit Changed();
        return true;
    }
    return false;
}

void RunningUploads::Cancel(int id){
    foreach(const Running &running, m_Running){
        if(running.id != id) continue;
        if(running.stop) running.stop();
        return;
    }
}

QList<RunningUploads::Entry> RunningUploads::Shown() const {
    QList<Entry> shown;
    foreach(const Running &running, m_Running){
        if(!running.shown) continue;
        Entry entry;
        entry.id = running.id;
        entry.url = running.url;
        shown << entry;
    }
    return shown;
}
