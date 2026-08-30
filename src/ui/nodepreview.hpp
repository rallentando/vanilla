#ifndef NODEPREVIEW_HPP
#define NODEPREVIEW_HPP

#include "switch.hpp"

#include <QWidget>
#include <QImage>
#include <QString>
#include <QRect>
#include <QTimer>

class NodePreview : public QWidget {
    Q_OBJECT

public:
    static NodePreview *Instance();

    void Request(const QImage &image, const QString &title,
                 const QRect &itemRect, bool isDirectory);
    void Dismiss();

    static QRect Place(const QRect &itemRect, const QSize &size, const QRect &screen, int gap);

    static QSize SizeForImageArea(const QSize &imageArea);

protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;

private:
    NodePreview();

    void Appear();

    QImage m_Image;
    QString m_Title;
    QRect m_ItemRect;
    bool m_IsDirectory = false;
    QTimer m_Timer;
};

#endif
