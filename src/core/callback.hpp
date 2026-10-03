#ifndef CALLBACK_HPP
#define CALLBACK_HPP

#include <QUrl>
#include <QString>
#include <QUrl>
#include <QString>
#include <QVariant>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QRect>
#include <QRectF>
#include <QRegion>

#include <functional>

typedef std::function<void(bool)> BoolCallBack;
typedef std::function<void(const QUrl&)> UrlCallBack;
typedef std::function<void(const QString&)> StringCallBack;
typedef std::function<void(const QVariant&)> VariantCallBack;
typedef std::function<void(const QPoint&)> PointCallBack;
typedef std::function<void(const QPointF&)> PointFCallBack;
typedef std::function<void(const QSize&)> SizeCallBack;
typedef std::function<void(const QSizeF&)> SizeFCallBack;
typedef std::function<void(const QRect&)> RectCallBack;
typedef std::function<void(const QRectF&)> RectFCallBack;
typedef std::function<void(const QRegion&)> RegionCallBack;
typedef std::function<void()> VoidCallBack;

class CompletionBarrier {
public:
    explicit CompletionBarrier(VoidCallBack finished)
        : m_Remaining(1), m_Finished(false), m_OnFinished(finished) {}

    void Add(){
        if(!m_Finished) ++m_Remaining;
    }

    void Complete(){
        if(m_Finished) return;
        if(--m_Remaining == 0) Finish();
    }

    void Seal(){ Complete();}
    void Expire(){ Finish();}
    bool IsFinished() const { return m_Finished;}

private:
    void Finish(){
        if(m_Finished) return;
        m_Finished = true;
        VoidCallBack finished = m_OnFinished;
        m_OnFinished = VoidCallBack();
        if(finished) finished();
    }

    int m_Remaining;
    bool m_Finished;
    VoidCallBack m_OnFinished;
};

#endif
