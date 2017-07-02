#ifndef TRIDENTVIEW_HPP
#define TRIDENTVIEW_HPP

#include "switch.hpp"

#ifdef TRIDENTVIEW

#include "view.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"

#include <ActiveQt/QAxWidget>
#include <ActiveQt/qaxtypes.h>

struct IWebBrowser2;
struct IHTMLDocument2;
struct ICustomDoc;
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

    QAxWidget *base() Q_DECL_OVERRIDE;
    Page *page() Q_DECL_OVERRIDE;

    QUrl url() Q_DECL_OVERRIDE;
    QString html() Q_DECL_OVERRIDE;
    TreeBank *parent() Q_DECL_OVERRIDE;
    void setUrl(const QUrl &url) Q_DECL_OVERRIDE;
    void setHtml(const QString &html, const QUrl &url) Q_DECL_OVERRIDE;
    void setParent(TreeBank* tb) Q_DECL_OVERRIDE;

    void Connect(TreeBank *tb) Q_DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) Q_DECL_OVERRIDE;

    bool ForbidToOverlap() Q_DECL_OVERRIDE {
        // rendering issue.
        return true;
    }

    bool CanGoBack() Q_DECL_OVERRIDE;
    bool CanGoForward() Q_DECL_OVERRIDE;

    bool IsRenderable() Q_DECL_OVERRIDE {
        return true;
    }
    void Render(QPainter *painter) Q_DECL_OVERRIDE {
        render(painter);
    }
    void Render(QPainter *painter, const QRegion &clip) Q_DECL_OVERRIDE {
        render(painter, QPoint(), clip);
    }
    QSize GetViewportSize() Q_DECL_OVERRIDE {
        return size();
    }
    void SetViewportSize(QSize size) Q_DECL_OVERRIDE {
        if(!visible()) resize(size);
    }
    void SetSource(const QUrl &url) Q_DECL_OVERRIDE {
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

    QString GetTitle() Q_DECL_OVERRIDE {
        return m_Title;
    }
    QIcon GetIcon() Q_DECL_OVERRIDE {
        return m_Icon;
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        if(QAction *action = Action(a, data)) action->trigger();
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        return page() ? page()->Action(a, data) : 0;
    }
    void TriggerNativeLoadAction(const QUrl &url) Q_DECL_OVERRIDE {
        m_Url = url;
        dynamicCall("Navigate(const QString&)",
                    QString::fromLatin1(QUrl::toPercentEncoding(m_Url.toString(), ":;/?=,+@#$%&")));
    }
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) Q_DECL_OVERRIDE {
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
    void TriggerNativeGoBackAction() Q_DECL_OVERRIDE {
        dynamicCall("GoBack()");
    }
    void TriggerNativeGoForwardAction() Q_DECL_OVERRIDE {
        dynamicCall("GoForward()");
    }
    void TriggerNativeRewindAction() Q_DECL_OVERRIDE;
    void TriggerNativeFastForwardAction() Q_DECL_OVERRIDE;

    void UpKeyEvent() Q_DECL_OVERRIDE;
    void DownKeyEvent() Q_DECL_OVERRIDE;
    void RightKeyEvent() Q_DECL_OVERRIDE;
    void LeftKeyEvent() Q_DECL_OVERRIDE;
    void PageDownKeyEvent() Q_DECL_OVERRIDE;
    void PageUpKeyEvent() Q_DECL_OVERRIDE;
    void HomeKeyEvent() Q_DECL_OVERRIDE;
    void EndKeyEvent() Q_DECL_OVERRIDE;

    void KeyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE { keyPressEvent(ev);}
    void KeyReleaseEvent(QKeyEvent *ev) Q_DECL_OVERRIDE { keyReleaseEvent(ev);}
    void MousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { mousePressEvent(ev);}
    void MouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { mouseReleaseEvent(ev);}
    void MouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { mouseMoveEvent(ev);}
    void MouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { mouseDoubleClickEvent(ev);}
    void WheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE { wheelEvent(ev);}

    QUrl GetBaseUrl() Q_DECL_OVERRIDE;
    QUrl GetCurrentBaseUrl() Q_DECL_OVERRIDE;
    SharedWebElementList FindElements(Page::FindElementsOption option) Q_DECL_OVERRIDE;
    SharedWebElement HitElement(const QPoint&) Q_DECL_OVERRIDE;
    QUrl HitLinkUrl(const QPoint&) Q_DECL_OVERRIDE;
    QUrl HitImageUrl(const QPoint&) Q_DECL_OVERRIDE;
    QString SelectedText() Q_DECL_OVERRIDE;
    QString SelectedHtml() Q_DECL_OVERRIDE;
    QString WholeText() Q_DECL_OVERRIDE;
    QString WholeHtml() Q_DECL_OVERRIDE;
    QRegion SelectionRegion() Q_DECL_OVERRIDE;
    QVariant EvaluateJavaScript(const QString &source) Q_DECL_OVERRIDE;

public slots:
    QSize size() Q_DECL_OVERRIDE { return base()->size();}
    void resize(QSize size) Q_DECL_OVERRIDE {
        if(TreeBank::PurgeView()){
            MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : 0;
            base()->setGeometry(win ? win->geometry() : QRect(QPoint(), size));
        } else {
            base()->setGeometry(QRect(QPoint(), size));
        }
    }
    void show() Q_DECL_OVERRIDE {
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
        if(HistNode *hn = GetHistNode()) hn->SetLastAccessDateToCurrent();

        // view become to stop updating, when only call show method.
        // e.g. in coming back after making other view.
        MainWindow *win = Application::GetCurrentWindow();
        QSize s =
            m_TreeBank ? m_TreeBank->size() :
            win ? win->GetTreeBank()->size() :
            !size().isEmpty() ? size() :
            DEFAULT_WINDOW_SIZE;
        resize(QSize(s.width(), s.height()+1));
        resize(s);

        // set only notifier.
        if(!m_TreeBank || !m_TreeBank->GetNotifier()) return;
        m_TreeBank->GetNotifier()->SetScroll(GetScroll());
    }
    void hide() Q_DECL_OVERRIDE { base()->hide();}
    void raise() Q_DECL_OVERRIDE { base()->raise();}
    void lower() Q_DECL_OVERRIDE { base()->lower();}
    void repaint() Q_DECL_OVERRIDE { base()->repaint();}
    bool visible() Q_DECL_OVERRIDE { return base()->isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) Q_DECL_OVERRIDE {
        QTimer::singleShot(0, this, [this, reason](){ base()->setFocus(reason);});
    }

    void Load()                           Q_DECL_OVERRIDE { View::Load();}
    void Load(const QString &url)         Q_DECL_OVERRIDE { View::Load(url);}
    void Load(const QUrl &url)            Q_DECL_OVERRIDE { View::Load(url);}
    void Load(const QNetworkRequest &req) Q_DECL_OVERRIDE { View::Load(req);}

    void OnBeforeStartingDisplayGadgets() Q_DECL_OVERRIDE { hide();}
    void OnAfterFinishingDisplayGadgets() Q_DECL_OVERRIDE { show();}

    void OnSetViewNode(ViewNode*) Q_DECL_OVERRIDE;
    void OnSetHistNode(HistNode*) Q_DECL_OVERRIDE;
    void OnSetThis(WeakView) Q_DECL_OVERRIDE;
    void OnSetMaster(WeakView) Q_DECL_OVERRIDE;
    void OnSetSlave(WeakView) Q_DECL_OVERRIDE;
    void OnSetJsObject(_View*) Q_DECL_OVERRIDE;
    void OnSetJsObject(_Vanilla*) Q_DECL_OVERRIDE;
    void OnLoadStarted() Q_DECL_OVERRIDE;
    void OnLoadProgress(int) Q_DECL_OVERRIDE;
    void OnLoadFinished(bool) Q_DECL_OVERRIDE;
    void OnTitleChanged(const QString&) Q_DECL_OVERRIDE;
    void OnUrlChanged(const QUrl&) Q_DECL_OVERRIDE;
    void OnViewChanged() Q_DECL_OVERRIDE;
    void OnScrollChanged() Q_DECL_OVERRIDE;

    void EmitScrollChanged() Q_DECL_OVERRIDE;

    QPointF GetScroll() Q_DECL_OVERRIDE;
    void SetScroll(QPointF pos) Q_DECL_OVERRIDE;
    bool SaveScroll() Q_DECL_OVERRIDE;
    bool RestoreScroll() Q_DECL_OVERRIDE;
    bool SaveZoom() Q_DECL_OVERRIDE;
    bool RestoreZoom() Q_DECL_OVERRIDE;
    bool SaveHistory() Q_DECL_OVERRIDE;
    bool RestoreHistory() Q_DECL_OVERRIDE;

    void KeyEvent(QString) {
        // not yet implemented.
    }
    bool SeekText(const QString&, View::FindFlags) {
        // not yet implemented.
        return false;
    }
    void UpdateIcon(const QUrl &iconUrl);
    void DisplayContextMenu(QWidget *parent, SharedWebElement elem, QPoint localPos, QPoint globalPos);

    void Copy() Q_DECL_OVERRIDE;
    void Cut() Q_DECL_OVERRIDE;
    void Paste() Q_DECL_OVERRIDE;
    void Undo() Q_DECL_OVERRIDE;
    void Redo() Q_DECL_OVERRIDE;
    void SelectAll() Q_DECL_OVERRIDE;
    void Unselect() Q_DECL_OVERRIDE;
    void Reload() Q_DECL_OVERRIDE;
    void ReloadAndBypassCache() Q_DECL_OVERRIDE;
    void Stop() Q_DECL_OVERRIDE;
    void StopAndUnselect() Q_DECL_OVERRIDE;
    void Print() Q_DECL_OVERRIDE;
    void Save() Q_DECL_OVERRIDE;
    void ZoomIn() Q_DECL_OVERRIDE;
    void ZoomOut() Q_DECL_OVERRIDE;

    void InspectElement() Q_DECL_OVERRIDE;
    void AddSearchEngine(QPoint pos) Q_DECL_OVERRIDE;
    void AddBookmarklet(QPoint pos) Q_DECL_OVERRIDE;

signals:
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void linkHovered(const QString&, const QString&, const QString&);
    void ViewChanged();
    void ScrollChanged(QPointF);

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
    bool translateKeyEvent(int message, int keycode) const Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent *ev) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *ev) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *ev) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *ev) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    bool focusNextPrevChild(bool next) Q_DECL_OVERRIDE;

private:
    IWebBrowser2 *m_Interface;
    IHTMLDocument2 *m_HtmlDocument;
    ICustomDoc *m_CustomDocument;
    Handler *m_Handler;
    QUrl m_Url;
    QString m_Title;
    QIcon m_Icon;
};

#endif //ifdef TRIDENTVIEW
#endif //ifndef TRIDENTVIEW_HPP
