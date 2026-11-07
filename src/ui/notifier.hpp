#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "switch.hpp"

#include "devicescale.hpp"

#include <QWidget>

class QMenu;
class QTimerEvent;
class QPaintEvent;
class QShowEvent;
class QTimer;

class TreeBank;

class Notifier : public QWidget {
    Q_OBJECT

public:
    enum Position {
        NorthWest,
        NorthEast,
        SouthWest,
        SouthEast,
    } m_Position;

    Notifier(TreeBank *parent = nullptr, bool purge = false);
    ~Notifier() override;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    bool IsPurged() const;
    void Purge();
    void Join();
    void ResizeNotify(QSize size);

public slots:
    void SetStatus(const QString str);
    void SetStatus(const QString str1, const QString str2);
    void ResetStatus();

    void SetLink(const QString url, const QString title, const QString txt);
    void ResetLink();

    void AutoSaveStarted();
    void AutoSaveFailed();
    void AutoSaveFinished(const QString & = QString());

    void SetScroll(QPointF pos);

signals:
    void ScrollRequest(QPointF);

private:
    void MakeOwnedWindow();
    void TakeWindowOwnerIfNeed();

    TreeBank *m_TreeBank;

    bool ShouldBeVisible() const;
    void TextChanged();

    QPoint m_HotSpot;
    QPointF m_ScrollPos;

    bool m_UseLinkText;
    bool m_LowerIsLink;
    QString m_UpperText;
    QString m_LowerText;
    QTimer *m_StatusTimer;
    QTimer *m_LinkTimer;
    friend class tst_notifier;
    bool EmitScrollRequest(QPoint pos);
    QRect ScrollAreaRect() const;
    int ScrollMarkerInset() const;

protected:
    void timerEvent(QTimerEvent *ev) override;
    void paintEvent(QPaintEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void enterEvent(QEnterEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
};

#endif
