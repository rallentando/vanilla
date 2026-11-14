#ifndef TOOLBAR_HPP
#define TOOLBAR_HPP

#include "switch.hpp"
#include "const.hpp"

#include "view.hpp"
#include "devicescale.hpp"

#include "receiver.hpp"

#include <QToolBar>
#include <QPointer>

class TreeBank;
class QLineEdit;
class QPaintEvent;
class QResizeEvent;
class ExtensionBar;
class SiteButton;
class SiteInfoPanel;
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

    static QList<QAction*> NavigationActions(TreeBank *tb);
    static void SyncNavigation(TreeBank *tb, View *view, bool loading);

    DownloadBar *GetDownloadBar() const { return m_DownloadBar;}
    SiteButton *GetSiteButton() const { return m_SiteButton;}
    LineEdit *Field() const;

public slots:
    void SetUrl(const QUrl&);
    void SetStarted();
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
    SiteButton *m_SiteButton;
    QPointer<SiteInfoPanel> m_SitePanel;
    ExtensionBar *m_ExtensionBar;
    DownloadBar *m_DownloadBar;

};

#endif
