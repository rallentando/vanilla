#ifndef DOWNLOADBAR_HPP
#define DOWNLOADBAR_HPP

#include "switch.hpp"

#include "devicescale.hpp"
#include "downloadhistory.hpp"

#include <QPointer>
#include <QWidget>

class QToolButton;
class QScrollArea;
class QTimer;
class MainWindow;

class DownloadBar : public QWidget {
    Q_OBJECT

public:
    explicit DownloadBar(QWidget *parent);
    ~DownloadBar() override;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    QToolButton *Button() const { return m_Button;}

    static QIcon DownloadIcon();
    void ApplyTheme();

    bool IsNeeded() const { return m_Needed;}
    static qint64 WantedFor(const QList<DownloadHistory::Entry> &entries, const QDateTime &now);
    static const qint64 WantedWhileRunning = -1;
    static QDateTime NewestEnding(const QList<DownloadHistory::Entry> &entries);

public slots:
    void Reconsider();

signals:
    void NeededChanged(bool needed);

private:
    QToolButton *m_Button;
    QTimer *m_Timer;
    QDateTime m_ArmedFor;
    bool m_RanOut;
    bool m_Needed;
};

class DownloadPanel : public QWidget {
    Q_OBJECT

public:
    static void ShowFor(MainWindow *window, bool toggle, bool activate);
    static void Initialize();
    static bool IsOpenFor(MainWindow *window);

    static QString Describe(const DownloadHistory::Entry &entry, bool exists);
    static QString Size(qint64 bytes);
    static bool HoldsAPressedButton(QWidget *rows);
    void ApplyTheme();

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    ~DownloadPanel() override;

protected:
    void paintEvent(QPaintEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    bool eventFilter(QObject *watched, QEvent *ev) override;

private:
    explicit DownloadPanel(MainWindow *window);
    void TellBar();
    void Rebuild();
    void RebuildSoon();
    void Place();
    QWidget *Anchor() const;
    bool IsOutside(const QPoint &global) const;
    void PollButtons();

    QPointer<MainWindow> m_Window;
    QScrollArea *m_Scroll;
    QTimer *m_RebuildTimer;
    QTimer *m_PressPoll;
    bool m_ButtonDown;
    bool m_EscapeDown;
    bool m_Translucent;
};

#endif
