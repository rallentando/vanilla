#ifndef NODEPREVIEW_HPP
#define NODEPREVIEW_HPP

#include "switch.hpp"

#include <QWidget>
#include <QImage>
#include <QString>
#include <QRect>
#include <QPoint>
#include <QTimer>
#include <QElapsedTimer>

class QPropertyAnimation;

class NodePreview : public QWidget {
    Q_OBJECT

public:
    static NodePreview *Instance();

    void Request(const QImage &image, const QString &title,
                 const QString &host,
                 const QRect &itemRect, const QRect &barRect, Qt::Edge side,
                 const QPoint &pointer);
    void LeaveItem();
    void Dismiss();
    void HoldUntilPointerMoves(const QPoint &pointer);
    bool IsAbout(const QString &title, const QRect &itemRect) const;

    static QRect Place(const QRect &itemRect, const QRect &barRect, Qt::Edge side,
                       const QSize &size, const QRect &screen, int gap);

    static QSize SizeFor(bool hasImage, bool hasHost);

protected:
    void paintEvent(QPaintEvent *ev) override;

private:
    NodePreview();

    void Appear();

    QImage m_Image;
    QString m_Title;
    QString m_Host;
    QRect m_ItemRect;
    QRect m_BarRect;
    Qt::Edge m_Side = Qt::BottomEdge;
    QTimer m_Timer;
    QPropertyAnimation *m_Slide;
    bool m_Held = false;
    QPoint m_HeldAt;
    QElapsedTimer m_SinceShown;
};

#endif
