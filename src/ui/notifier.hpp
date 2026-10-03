#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "switch.hpp"

#include "devicescale.hpp"

#include <QWidget>
#include <QMap>
#include <QPointer>

class QMenu;
class QTimerEvent;
class QPaintEvent;

class TreeBank;
class DownloadItem;
class UploadItem;

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
    ~Notifier() Q_DECL_OVERRIDE;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    bool IsPurged() const;
    void Purge();
    void Join();
    void ResizeNotify(QSize size);
    void RepaintIfNeed(const QRect &rect);
    void RegisterDownload(DownloadItem *item);
    void RegisterUpload(UploadItem *item);
    void AddDownloadItem(DownloadItem *item);
    void RemoveDownloadItem(DownloadItem *item);
    void AddUploadItem(UploadItem *item);
    void RemoveUploadItem(UploadItem *item);

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
    void SetSaveProgress(QString file, qint64 received, qint64 total);
    void SetOpenProgress(QString file, qint64 sent, qint64 total);

signals:
    void ScrollRequest(QPointF);

private:
    void MakeOwnedWindow();
    void TakeWindowOwnerIfNeed();

    TreeBank *m_TreeBank;

    QPoint m_HotSpot;
    QPointF m_ScrollPos;

    QMap<DownloadItem*, int> m_DownloadItemTable;
    QMap<UploadItem*, int> m_UploadItemTable;

    QPointer<DownloadItem> m_HoveredDownloadItem;
    QPointer<UploadItem> m_HoveredUploadItem;
    enum CancelButtonState {
        NotHovered,
        ItemHovered,
        ButtonHovered,
        ButtonPressed,
    } m_CancelButtonState;

    bool m_UseLinkText;
    QString m_UpperText;
    QString m_LowerText;
    bool EmitScrollRequest(QPoint pos);

protected:
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void enterEvent(QEnterEvent *ev) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
};

#endif
