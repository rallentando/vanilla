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

class ToolBar : public QToolBar {
    Q_OBJECT

public:
    ToolBar(TreeBank *tb, QWidget *parent = nullptr);
    ~ToolBar() Q_DECL_OVERRIDE;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    static void Initialize();

    static void LoadSettings();
    static void SaveSettings();

    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    void Connect(SharedView view);
    void Disconnect(SharedView view);

    QMenu *ToolBarMenu();

public slots:
    void SetUrl(const QUrl&);
    void SetFinished(bool);

protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;

    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
    void enterEvent(QEnterEvent *ev) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *watched, QEvent *ev) Q_DECL_OVERRIDE;

private:
    SharedView m_View;
    TreeBank *m_TreeBank;
    LineEdit *m_LineEdit;
    ExtensionBar *m_ExtensionBar;
    QCompleter *m_Completer;
    QStandardItemModel *m_Model;
    QAction *m_BackAction;
    QAction *m_ForwardAction;
    QAction *m_RewindAction;
    QAction *m_FastForwardAction;
    QAction *m_ReloadAction;
    QAction *m_StopAction;

    static QStringList m_CommandCandidates;
    static QStringList m_InputUrlHistory;

public slots:
    void DisplaySuggest(const QByteArray&);
signals:
    void SuggestRequest(const QUrl&);
};

#endif
