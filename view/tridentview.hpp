#ifndef TRIDENTVIEW_HPP
#define TRIDENTVIEW_HPP

#include "switch.hpp"

#include "view.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"

#include <Exdisp.h>
#include <Mshtml.h>
#include <Mshtmhst.h>
#include <Comdef.h>
#include <Shlobj.h>
#include <Tlogstg.h>
#include <ActiveQt/QAxWidget>
#include <ActiveQt/qaxtypes.h>

class Handler;

class TridentView : public QAxWidget, public View{
    Q_OBJECT

public:
    TridentView(TreeBank *parent = 0, QString id = "", QStringList set = QStringList());
    ~TridentView();

    void Clear();

    // register to windows registry.
    static void SetFeatureControl();

    void ResetDocument();

    int GetZoomFactor();
    void SetZoomFactor(int);

    QAxWidget *base() DECL_OVERRIDE {
        return static_cast<QAxWidget*>(this);
    }
    Page *page() DECL_OVERRIDE {
        return static_cast<Page*>(View::page());
    }

    QUrl      url() DECL_OVERRIDE { return m_Url;}
    void   setUrl(const QUrl &url) DECL_OVERRIDE {
        TriggerNativeLoadAction(url);
        emit urlChanged(url);
    }

    QString   html() DECL_OVERRIDE { return WholeHtml();}
    void   setHtml(const QString &html, const QUrl &url) DECL_OVERRIDE;

    TreeBank *parent() DECL_OVERRIDE { return m_TreeBank;}
    void   setParent(TreeBank* tb) DECL_OVERRIDE {
        View::SetTreeBank(tb);
        if(!TreeBank::PurgeView()) base()->setParent(tb);
        if(tb) resize(size());
    }

    void Connect(TreeBank *tb) DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) DECL_OVERRIDE;

    void ZoomIn() DECL_OVERRIDE;
    void ZoomOut() DECL_OVERRIDE;

    QUrl BaseUrl() DECL_OVERRIDE;

    QUrl CurrentBaseUrl() DECL_OVERRIDE {
        return BaseUrl();
    }

    bool ForbidToOverlap() DECL_OVERRIDE {
        // not yet implemented.
        return false;
    }

    bool CanGoBack() DECL_OVERRIDE;
    bool CanGoForward() DECL_OVERRIDE;

    void Print() DECL_OVERRIDE {
        if(!m_Interface) return;
        m_Interface->ExecWB(OLECMDID_PRINT, OLECMDEXECOPT_PROMPTUSER, 0, 0);
    }
    void AddSearchEngine(QPoint pos) DECL_OVERRIDE {
        // not yet implemented.
        Q_UNUSED(pos);
    }
    void AddBookmarklet(QPoint pos) DECL_OVERRIDE {
        // not yet implemented.
        Q_UNUSED(pos);
    }

    bool IsRenderable() DECL_OVERRIDE {
        return true;
    }
    void Render(QPainter *painter) DECL_OVERRIDE {
        render(painter);
    }
    void Render(QPainter *painter, const QRegion &clip) DECL_OVERRIDE {
        render(painter, QPoint(), clip);
    }
    QSize GetViewportSize() DECL_OVERRIDE {
        return size();
    }
    void SetViewportSize(QSize size) DECL_OVERRIDE {
        if(!visible()) resize(size);
    }
    void SetSource(const QUrl &url) DECL_OVERRIDE {
        // not yet implemented.
        Q_UNUSED(url);
    }
    void SetSource(const QByteArray &html){
        // not yet implemented.
        Q_UNUSED(html);
    }
    void SetSource(const QString &html){
        // not yet implemented.
        Q_UNUSED(html);
    }

    QString GetTitle() DECL_OVERRIDE {
        return m_Title;
    }
    QIcon GetIcon() DECL_OVERRIDE {
        return m_Icon;
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        if(QAction *action = Action(a, data)) action->trigger();
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        return page() ? page()->Action(a, data) : 0;
    }
    void TriggerNativeLoadAction(const QUrl &url) DECL_OVERRIDE {
        m_Url = url;
        dynamicCall("Navigate(const QString&)",
                    QString::fromLatin1(QUrl::toPercentEncoding(m_Url.toString(), ":;/?=,+@#$%&")));
    }
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) DECL_OVERRIDE {
        Q_UNUSED(operation); Q_UNUSED(body);
        m_Url = req.url();
        QByteArray header;
        foreach(QByteArray name, req.rawHeaderList()){
            header += name + ": " + req.rawHeader(name) + "\n";
        }
        dynamicCall("Navigate(const QString&, const QVariant&, const QVariant&, const QVariant&, const QVariant&)",
                    QString::fromLatin1(QUrl::toPercentEncoding(m_Url.toString(), ":;/?=,+@#$%&")),
                    0, "",  QList<QVariant>(), header);
    }
    void TriggerNativeGoBackAction() DECL_OVERRIDE {
        dynamicCall("GoBack()");
    }
    void TriggerNativeGoForwardAction() DECL_OVERRIDE {
        dynamicCall("GoForward()");
    }

    void TriggerNativeReloadAction(){
        dynamicCall("Refresh()");
    }
    void TriggerNativeStopAction(){
        dynamicCall("Stop()");
    }

    void TriggerNativeCutAction(){
        m_Interface->ExecWB(OLECMDID_CUT, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    }
    void TriggerNativePasteAction(){
        m_Interface->ExecWB(OLECMDID_PASTE, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    }
    void TriggerNativeUndoAction(){
        m_Interface->ExecWB(OLECMDID_UNDO, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    }
    void TriggerNativeRedoAction(){
        m_Interface->ExecWB(OLECMDID_REDO, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    }
    void TriggerNativeSelectAllAction(){
        m_Interface->ExecWB(OLECMDID_SELECTALL, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    }

    void UpKeyEvent() DECL_OVERRIDE;
    void DownKeyEvent() DECL_OVERRIDE;
    void RightKeyEvent() DECL_OVERRIDE;
    void LeftKeyEvent() DECL_OVERRIDE;
    void PageDownKeyEvent() DECL_OVERRIDE;
    void PageUpKeyEvent() DECL_OVERRIDE;
    void HomeKeyEvent() DECL_OVERRIDE;
    void EndKeyEvent() DECL_OVERRIDE;

    void KeyPressEvent(QKeyEvent *ev) DECL_OVERRIDE { keyPressEvent(ev);}
    void KeyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE { keyReleaseEvent(ev);}
    void MousePressEvent(QMouseEvent *ev) DECL_OVERRIDE { mousePressEvent(ev);}
    void MouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE { mouseReleaseEvent(ev);}
    void MouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE { mouseMoveEvent(ev);}
    void MouseDoubleClickEvent(QMouseEvent *ev) DECL_OVERRIDE { mouseDoubleClickEvent(ev);}
    void WheelEvent(QWheelEvent *ev) DECL_OVERRIDE {
        QWheelEvent *newev = new QWheelEvent(ev->pos(),
                                             ev->delta()/Application::WheelScrollRate(),
                                             ev->buttons(),
                                             ev->modifiers(),
                                             ev->orientation());
        wheelEvent(newev);
        ev->setAccepted(true);
        delete newev;
    }

    QUrl GetBaseUrl() DECL_OVERRIDE {
        return BaseUrl();
    }
    QUrl GetCurrentBaseUrl() DECL_OVERRIDE {
        return CurrentBaseUrl();
    }
    SharedWebElementList FindElements(Page::FindElementsOption option) DECL_OVERRIDE;
    SharedWebElement HitElement(const QPoint&) DECL_OVERRIDE;
    QUrl HitLinkUrl(const QPoint&) DECL_OVERRIDE;
    QUrl HitImageUrl(const QPoint&) DECL_OVERRIDE;
    QString SelectedText() DECL_OVERRIDE;
    QString SelectedHtml() DECL_OVERRIDE;
    QString WholeText() DECL_OVERRIDE;
    QString WholeHtml() DECL_OVERRIDE;
    QRegion SelectionRegion() DECL_OVERRIDE;
    QVariant EvaluateJavaScript(const QString &source) DECL_OVERRIDE;

public slots:
    QSize size() DECL_OVERRIDE { return base()->size();}
    void resize(QSize size) DECL_OVERRIDE {
        if(TreeBank::PurgeView()){
            MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : 0;
            base()->setGeometry(win ? win->geometry() : QRect(QPoint(), size));
        } else {
            base()->setGeometry(QRect(QPoint(), size));
        }
    }
    void show() DECL_OVERRIDE {
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
        if(HistNode *hn = GetHistNode()) hn->SetLastAccessDateToCurrent();
        // set only notifier.
        if(!m_TreeBank || !m_TreeBank->GetNotifier()) return;
        m_TreeBank->GetNotifier()->SetScroll(GetScroll());
    }
    void hide() DECL_OVERRIDE { base()->hide();}
    void raise() DECL_OVERRIDE { base()->raise();}
    void lower() DECL_OVERRIDE { base()->lower();}
    void repaint() DECL_OVERRIDE { base()->repaint();}
    bool visible() DECL_OVERRIDE { return base()->isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) DECL_OVERRIDE {
        base()->setFocus(reason);
    }

    void Load()                           DECL_OVERRIDE { View::Load();}
    void Load(const QString &url)         DECL_OVERRIDE { View::Load(url);}
    void Load(const QUrl &url)            DECL_OVERRIDE { View::Load(url);}
    void Load(const QNetworkRequest &req) DECL_OVERRIDE { View::Load(req);}

    void OnBeforeStartingDisplayGadgets() DECL_OVERRIDE { hide();}
    void OnAfterFinishingDisplayGadgets() DECL_OVERRIDE { show();}

    void OnSetViewNode(ViewNode*) DECL_OVERRIDE;
    void OnSetHistNode(HistNode*) DECL_OVERRIDE;
    void OnSetThis(WeakView) DECL_OVERRIDE;
    void OnSetMaster(WeakView) DECL_OVERRIDE;
    void OnSetSlave(WeakView) DECL_OVERRIDE;
    void OnSetJsObject(_View*) DECL_OVERRIDE;
    void OnSetJsObject(_Vanilla*) DECL_OVERRIDE;
    void OnLoadStarted() DECL_OVERRIDE;
    void OnLoadProgress(int) DECL_OVERRIDE;
    void OnLoadFinished(bool) DECL_OVERRIDE;
    void OnTitleChanged(const QString&) DECL_OVERRIDE;
    void OnUrlChanged(const QUrl&) DECL_OVERRIDE;
    void OnViewChanged() DECL_OVERRIDE;
    void OnScrollChanged() DECL_OVERRIDE;

    void EmitScrollChanged() DECL_OVERRIDE;
    void EmitScrollChangedIfNeed() DECL_OVERRIDE;

    QPointF GetScroll() DECL_OVERRIDE;
    void SetScroll(QPointF pos) DECL_OVERRIDE;
    bool SaveScroll() DECL_OVERRIDE;
    bool RestoreScroll() DECL_OVERRIDE;
    bool SaveZoom() DECL_OVERRIDE;
    bool RestoreZoom() DECL_OVERRIDE;
    bool SaveHistory() DECL_OVERRIDE;
    bool RestoreHistory() DECL_OVERRIDE;

    void KeyEvent(QString) {
        // not yet implemented.
    }
    bool SeekText(const QString&, View::FindFlags) {
        // not yet implemented.
        return false;
    }
    void UpdateIcon(const QUrl &iconUrl);
    void DisplayContextMenu(QWidget *parent, SharedWebElement elem, QPoint globalPos);

signals:
    void iconChanged();
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void linkHovered(const QString&, const QString&, const QString&);
    void ViewChanged();
    void ScrollChanged(QPointF);
    void ButtonCleared();
    void RenderFinished();

    void titleChanged(const QString&);
    void urlChanged(const QUrl&);
    void loadStarted();
    void loadProgress(int);
    void loadFinished(bool);

    void BeforeNavigate(QString,int,QString,QVariant&,QString,bool&);
    void BeforeNavigate2(IDispatch*,QVariant&,QVariant&,QVariant&,QVariant&,QVariant&,bool&);
    void BeforeScriptExecute(IDispatch*);
    void CommandStateChange(int,bool);
    void DocumentComplete(IDispatch*,QVariant&);
    void DownloadBegin();
    void DownloadComplete();
    void FileDownload(bool,bool&);
    void FrameBeforeNavigate(QString,int,QString,QVariant&,QString,bool&);
    void FrameNavigateComplete(QString);
    void NavigateComplete(QString);
    void NavigateComplete2(IDispatch*,QVariant&);
    void NavigateError(IDispatch*,QVariant&,QVariant&,QVariant&,bool&);
    void NewWindow(QString,int,QString,QVariant&,QString,bool&);
    void NewWindow2(IDispatch**,bool&);
    void NewWindow3(IDispatch**,bool&,uint,QString,QString);
    void PrivacyImpactedStateChange(bool);
    void ProgressChange(int,int);
    void PropertyChange(QString);
    void SetSecureLockIcon(int);
    void StatusTextChange(QString);
    void TitleChange(QString);

    void OnFullScreen(bool);
    void OnMenuBar(bool);
    void OnQuit();
    void OnStatusBar(bool);
    void OnTheaterMode(bool);
    void OnToolBar(bool);
    void OnVisible(bool);

public slots:
    void OnSignal(const QString&, int, void*);
    void OnException(int,QString,QString,QString);
    void OnBeforeNavigate(QString,int,QString,QVariant&,QString,bool&);
    void OnBeforeNavigate2(IDispatch*,QVariant&,QVariant&,QVariant&,QVariant&,QVariant&,bool&);
    void OnBeforeScriptExecute(IDispatch*);
    void OnCommandStateChange(int,bool);
    void OnDocumentComplete(IDispatch*,QVariant&);
    void OnDownloadBegin();
    void OnDownloadComplete();
    void OnFileDownload(bool,bool&);
    void OnFrameBeforeNavigate(QString,int,QString,QVariant&,QString,bool&);
    void OnFrameNavigateComplete(QString);
    void OnNavigateComplete(QString);
    void OnNavigateComplete2(IDispatch*,QVariant&);
    void OnNavigateError(IDispatch*,QVariant&,QVariant&,QVariant&,bool&);
    void OnNewWindow(QString,int,QString,QVariant&,QString,bool&);
    void OnNewWindow2(IDispatch**,bool&);
    void OnNewWindow3(IDispatch**,bool&,uint,QString,QString);
    void OnPrivacyImpactedStateChange(bool);
    void OnProgressChange(int,int);
    void OnPropertyChange(QString);
    void OnSetSecureLockIcon(int);
    void OnStatusTextChange(QString);
    void OnTitleChange(QString);

    void OnOnFullScreen(bool);
    void OnOnMenuBar(bool);
    void OnOnQuit();
    void OnOnStatusBar(bool);
    void OnOnTheaterMode(bool);
    void OnOnToolBar(bool);
    void OnOnVisible(bool);

protected:
    bool translateKeyEvent(int message, int keycode) const DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent *ev) DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *ev) DECL_OVERRIDE;
    void dropEvent(QDropEvent *ev) DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *ev) DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *ev) DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE;
    bool focusNextPrevChild(bool next) DECL_OVERRIDE;

private:
    IWebBrowser2 *m_Interface;
    IHTMLDocument2 *m_HtmlDocument;
    ICustomDoc *m_CustomDocument;
    Handler *m_Handler;
    QUrl m_Url;
    QString m_Title;
    QIcon m_Icon;
};
#endif //ifndef TRIDENTVIEW_HPP
