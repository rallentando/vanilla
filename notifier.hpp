#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "switch.hpp"

#include <QWidget>
#include <QMap>

class QMenu;
class QTimerEvent;
class QPaintEvent;

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
class QWinTaskbarButton;
class QWinTaskbarProgress;
#endif

class TreeBank;
class DownloadItem;
class UploadItem;

class Notifier : public QWidget {
    Q_OBJECT

public:
    enum Position{
        NorthWest,
        NorthEast,
        SouthWest,
        SouthEast,
    } m_Position;

    Notifier(TreeBank *parent = 0, bool purge = false);
    ~Notifier();
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
    TreeBank *m_TreeBank;

    QPoint m_HotSpot;
    QPointF m_ScrollPos;

    QMap<DownloadItem*, int> m_DownloadItemTable;
    QMap<UploadItem*, int> m_UploadItemTable;
    DownloadItem *m_HoveredDownloadItem;
    UploadItem *m_HoveredUploadItem;
    enum CancelButtonState {
        NotHovered,
        ItemHovered,
        ButtonHovered,
        ButtonPressed,
    } m_CancelButtonState;

    bool m_UseLinkText;
    QString m_UpperText;
    QString m_LowerText;
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    QWinTaskbarButton *m_Button;
    QWinTaskbarProgress *m_Progress;
    void UpdateTaskbarProgress();
#endif
    bool EmitScrollRequest(QPoint pos);

protected:
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *ev) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
};

#endif //ifndef NOTIFIER_H
