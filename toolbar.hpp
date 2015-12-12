#ifndef TOOLBAR_HPP
#define TOOLBAR_HPP

#include "switch.hpp"
#include "const.hpp"

#include "view.hpp"

#include "receiver.hpp"

#include <QToolBar>

class TreeBank;
class QLineEdit;
class QCompleter;
class QStringListModel;
class QPaintEvent;
class QResizeEvent;

class ToolBar : public QToolBar{
    Q_OBJECT

public:
    ToolBar(TreeBank *tb, QWidget *parent = 0);
    ~ToolBar();

    static void Initialize();

    static void LoadSettings();
    static void SaveSettings();

    QSize sizeHint() const DECL_OVERRIDE;
    QSize minimumSizeHint() const DECL_OVERRIDE;

    void Connect(SharedView view);
    void Disconnect(SharedView view);

    QMenu *ToolBarMenu();

public slots:
    void SetUrl(const QUrl&);
    void SetFinished(bool);

protected:
    void paintEvent(QPaintEvent *ev) DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) DECL_OVERRIDE;

    void timerEvent(QTimerEvent *ev) DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) DECL_OVERRIDE;
    void enterEvent(QEvent *ev) DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;

private:
    SharedView m_View;
    TreeBank *m_TreeBank;
    LineEdit *m_LineEdit;
    QCompleter *m_Completer;
    QStringListModel *m_Model;
    QAction *m_BackAction;
    QAction *m_ForwardAction;
    QAction *m_ReloadAction;
    QAction *m_StopAction;

    static QStringList m_CommandCandidates;
};

#endif
