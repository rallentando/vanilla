#ifndef TOOLBAR_HPP
#define TOOLBAR_HPP

#include "switch.hpp"
#include "const.hpp"

#include "view.hpp"
#include "devicescale.hpp"

#include "receiver.hpp"

#include <QToolBar>

class TreeBank;
class QLineEdit;
class QCompleter;
class QStandardItemModel;
class QPaintEvent;
class QResizeEvent;
class ExtensionBar;
class DownloadBar;

class ToolBar : public QToolBar {
    Q_OBJECT

public:
    ToolBar(TreeBank *tb, QWidget *parent = nullptr);
    ~ToolBar() override;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    static void Initialize();

    static void LoadSettings();
    static void SaveSettings();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void Connect(SharedView view);
    void Disconnect(SharedView view);

    DownloadBar *GetDownloadBar() const { return m_DownloadBar;}

public slots:
    void SetUrl(const QUrl&);
    void SetFinished(bool);

protected:
    void paintEvent(QPaintEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;

    void timerEvent(QTimerEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void enterEvent(QEnterEvent *ev) override;
    void leaveEvent(QEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    bool eventFilter(QObject *watched, QEvent *ev) override;

private:
    void SyncActions(bool loading);

    SharedView m_View;
    TreeBank *m_TreeBank;
    LineEdit *m_LineEdit;
    ExtensionBar *m_ExtensionBar;
    DownloadBar *m_DownloadBar;
    QCompleter *m_Completer;
    QStandardItemModel *m_Model;
    QAction *m_BackAction;
    QAction *m_ForwardAction;
    QAction *m_RewindAction;
    QAction *m_FastForwardAction;
    QAction *m_ReloadAction;
    QAction *m_StopAction;

    void FillCandidates(const QString &text);

    static QStringList m_CommandCandidates;
    static QStringList m_InputUrlHistory;

public:
    static QStringList CommandCandidates();
    static QStringList UrlAncestors(const QString &text, const QUrl &current);
    static QStringList UrlCandidates(const QString &text, const QUrl &current, const QStringList &nearby);

public slots:
    void DisplaySuggest(const QByteArray&);
signals:
    void SuggestRequest(const QUrl&);
};

#endif
