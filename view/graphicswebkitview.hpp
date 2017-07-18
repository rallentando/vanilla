#ifndef GRAPHICSWEBKITVIEW_HPP
#define GRAPHICSWEBKITVIEW_HPP

#include "switch.hpp"

#ifdef WEBKITVIEW

#include "webkitpage.hpp"
#include "view.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"

#include <stdlib.h>

#include <QGraphicsWebView>
#include <QWebPage>
#include <QWebHistory>

#include <QWebFrame>
#include <QWebElement>

class QKeySequence;

class GraphicsWebKitView : public QGraphicsWebView, public View{
    Q_OBJECT

public:
    GraphicsWebKitView(TreeBank *parent = 0, QString id = "", QStringList set = QStringList());
    ~GraphicsWebKitView();

    QGraphicsWebView *base() Q_DECL_OVERRIDE;
    WebKitPage *page() Q_DECL_OVERRIDE;

    QUrl url() Q_DECL_OVERRIDE;
    QString html() Q_DECL_OVERRIDE;
    TreeBank *parent() Q_DECL_OVERRIDE;
    void setUrl(const QUrl &url) Q_DECL_OVERRIDE;
    void setHtml(const QString &html, const QUrl &url) Q_DECL_OVERRIDE;
    void setParent(TreeBank* tb) Q_DECL_OVERRIDE;

    void Connect(TreeBank *tb) Q_DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) Q_DECL_OVERRIDE;

    bool ForbidToOverlap() Q_DECL_OVERRIDE {
        return (url().toString().toLower().endsWith(QStringLiteral(".swf")) ||
                url().toString().toLower().endsWith(QStringLiteral(".pdf")));
    }

    bool CanGoBack() Q_DECL_OVERRIDE {
        return page() ? page()->history()->canGoBack() : false;
    }
    bool CanGoForward() Q_DECL_OVERRIDE {
        return page() ? page()->history()->canGoForward() : false;
    }

    bool IsRenderable() Q_DECL_OVERRIDE {
        return page() != 0;
    }
    void Render(QPainter *painter) Q_DECL_OVERRIDE {
        if(page()) page()->mainFrame()->render(painter);
    }
    void Render(QPainter *painter, const QRegion &clip) Q_DECL_OVERRIDE {
        if(page()) page()->mainFrame()->render(painter, clip);
    }
    QSize GetViewportSize() Q_DECL_OVERRIDE {
        return page() ? page()->viewportSize() : QSize();
    }
    void SetViewportSize(QSize size) Q_DECL_OVERRIDE {
        if(page()) page()->setViewportSize(size);
    }
    void SetSource(const QUrl &url) Q_DECL_OVERRIDE {
        if(page()) page()->SetSource(url);
    }
    void SetSource(const QByteArray &html) Q_DECL_OVERRIDE {
        if(page()) page()->SetSource(html);
    }
    void SetSource(const QString &html) Q_DECL_OVERRIDE {
        if(page()) page()->SetSource(html);
    }

    QString GetTitle() Q_DECL_OVERRIDE {
        return title();
    }
    QIcon GetIcon() Q_DECL_OVERRIDE {
        return icon();
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        if(page()) page()->TriggerAction(a, data);
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        return page() ? page()->Action(a, data) : 0;
    }

    void TriggerNativeLoadAction(const QUrl &url) Q_DECL_OVERRIDE {
        load(url);
    }
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) Q_DECL_OVERRIDE {
        load(req, operation, body);
    }
    void TriggerNativeGoBackAction() Q_DECL_OVERRIDE {
        if(page()) page()->triggerAction(QWebPage::Back);
    }
    void TriggerNativeGoForwardAction() Q_DECL_OVERRIDE {
        if(page()) page()->triggerAction(QWebPage::Forward);
    }
    void TriggerNativeRewindAction() Q_DECL_OVERRIDE {
        QWebHistory *h = history();
        h->goToItem(h->itemAt(0));
    }
    void TriggerNativeFastForwardAction() Q_DECL_OVERRIDE {
        QWebHistory *h = history();
        h->goToItem(h->itemAt(h->count()-1));
    }

    void UpKeyEvent()       Q_DECL_OVERRIDE { QGraphicsWebView::keyPressEvent(m_UpKey);       EmitScrollChanged();}
    void DownKeyEvent()     Q_DECL_OVERRIDE { QGraphicsWebView::keyPressEvent(m_DownKey);     EmitScrollChanged();}
    void RightKeyEvent()    Q_DECL_OVERRIDE { QGraphicsWebView::keyPressEvent(m_RightKey);    EmitScrollChanged();}
    void LeftKeyEvent()     Q_DECL_OVERRIDE { QGraphicsWebView::keyPressEvent(m_LeftKey);     EmitScrollChanged();}
    void PageDownKeyEvent() Q_DECL_OVERRIDE { QGraphicsWebView::keyPressEvent(m_PageDownKey); EmitScrollChanged();}
    void PageUpKeyEvent()   Q_DECL_OVERRIDE { QGraphicsWebView::keyPressEvent(m_PageUpKey);   EmitScrollChanged();}
    void HomeKeyEvent()     Q_DECL_OVERRIDE { QGraphicsWebView::keyPressEvent(m_HomeKey);     EmitScrollChanged();}
    void EndKeyEvent()      Q_DECL_OVERRIDE { QGraphicsWebView::keyPressEvent(m_EndKey);      EmitScrollChanged();}

    void KeyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE { keyPressEvent(ev);}
    void KeyReleaseEvent(QKeyEvent *ev) Q_DECL_OVERRIDE { keyReleaseEvent(ev);}
    void MousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { m_TreeBank->MousePressEvent(ev);}
    void MouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { m_TreeBank->MouseReleaseEvent(ev);}
    void MouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { m_TreeBank->MouseMoveEvent(ev);}
    void MouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { m_TreeBank->MouseDoubleClickEvent(ev);}
    void WheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE { m_TreeBank->WheelEvent(ev);}

    QUrl GetBaseUrl() Q_DECL_OVERRIDE {
        return page() ? page()->mainFrame()->baseUrl() : QUrl();
    }
    QUrl GetCurrentBaseUrl() Q_DECL_OVERRIDE {
        QUrl url;
        if(page() && page()->currentFrame())
            url = page()->currentFrame()->baseUrl();
        if(!url.isEmpty() && page() && page()->mainFrame())
            url = page()->mainFrame()->baseUrl();
        return url;
    }
    SharedWebElementList FindElements(Page::FindElementsOption option) Q_DECL_OVERRIDE;
    SharedWebElement HitElement(const QPoint &pos) Q_DECL_OVERRIDE;
    QUrl HitLinkUrl(const QPoint &pos) Q_DECL_OVERRIDE {
        return page() ? page()->mainFrame()->hitTestContent(pos).linkUrl() : QUrl();
    }
    QUrl HitImageUrl(const QPoint &pos) Q_DECL_OVERRIDE {
        return page() ? page()->mainFrame()->hitTestContent(pos).imageUrl() : QUrl();
    }
    QString SelectedText() Q_DECL_OVERRIDE {
        return page() ? page()->selectedText() : QString();
    }
    QString SelectedHtml() Q_DECL_OVERRIDE {
        return page() ? page()->selectedHtml() : QString();
    }
    QString WholeText() Q_DECL_OVERRIDE {
        return page() ? page()->mainFrame()->toPlainText() : QString();
    }
    QString WholeHtml() Q_DECL_OVERRIDE {
        return page() ? page()->mainFrame()->toHtml() : QString();
    }
    QRegion SelectionRegion() Q_DECL_OVERRIDE {
        QRegion region;
        QVariant var = EvaluateJavaScript(SelectionRegionJsCode());
        if(!var.isValid() || !var.canConvert(QMetaType::QVariantMap)) return region;
        QVariantMap map = var.toMap();
        QRect viewport = QRect(QPoint(), size());
        foreach(QString key, map.keys()){
            QVariantMap m = map[key].toMap();
            region |= QRect(m["x"].toInt()*zoomFactor(),
                            m["y"].toInt()*zoomFactor(),
                            m["width"].toInt()*zoomFactor(),
                            m["height"].toInt()*zoomFactor()).intersected(viewport);
        }
        return region;
    }
    QVariant EvaluateJavaScript(const QString &code) Q_DECL_OVERRIDE {
        return page() ? page()->mainFrame()->evaluateJavaScript(code) : QVariant();
    }

public slots:
    QSize size() Q_DECL_OVERRIDE {
        return base()->size().toSize();
    }
    void resize(QSize size) Q_DECL_OVERRIDE {
        base()->resize(size);
    }
    void show() Q_DECL_OVERRIDE {
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
        if(HistNode *hn = GetHistNode()) hn->SetLastAccessDateToCurrent();

        if(!m_TreeBank || !m_TreeBank->GetNotifier()) return;
        // set only notifier.
        m_TreeBank->GetNotifier()->SetScroll(GetScroll());
    }
    void hide() Q_DECL_OVERRIDE {
        base()->hide();
        // there is no sense to resize because plugins paint to window directly, but...
        if(ForbidToOverlap()) resize(QSize(0,0));
    }

    void raise()   Q_DECL_OVERRIDE { setZValue(VIEW_CONTENTS_LAYER);}
    void lower()   Q_DECL_OVERRIDE { setZValue(HIDDEN_CONTENTS_LAYER);}
    void repaint() Q_DECL_OVERRIDE { update(boundingRect());}

    bool visible() Q_DECL_OVERRIDE { return base()->isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) Q_DECL_OVERRIDE {
        base()->setFocus(reason);
    }

    void Load()                           Q_DECL_OVERRIDE { View::Load();}
    void Load(const QString &url)         Q_DECL_OVERRIDE { View::Load(url);}
    void Load(const QUrl &url)            Q_DECL_OVERRIDE { View::Load(url);}
    void Load(const QNetworkRequest &req) Q_DECL_OVERRIDE { View::Load(req);}

    void OnBeforeStartingDisplayGadgets() Q_DECL_OVERRIDE {}
    void OnAfterFinishingDisplayGadgets() Q_DECL_OVERRIDE {}

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
    void EmitScrollChangedIfNeed();

    void SetScrollBarState() Q_DECL_OVERRIDE;
    QPointF GetScroll() Q_DECL_OVERRIDE;
    void SetScroll(QPointF pos) Q_DECL_OVERRIDE;
    bool SaveScroll() Q_DECL_OVERRIDE;
    bool RestoreScroll() Q_DECL_OVERRIDE;
    bool SaveZoom() Q_DECL_OVERRIDE;
    bool RestoreZoom() Q_DECL_OVERRIDE;
    bool SaveHistory() Q_DECL_OVERRIDE;
    bool RestoreHistory() Q_DECL_OVERRIDE;

    void KeyEvent(QString);
    bool SeekText(const QString&, View::FindFlags);
    void OnIconChanged(const QIcon &icon);

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
    void statusBarMessage2(const QString&, const QString&);
    void ViewChanged();
    void ScrollChanged(QPointF);

protected:
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void resizeEvent(QGraphicsSceneResizeEvent *ev) Q_DECL_OVERRIDE;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent  *ev) Q_DECL_OVERRIDE;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void dropEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void wheelEvent(QGraphicsSceneWheelEvent *ev) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    bool focusNextPrevChild(bool next) Q_DECL_OVERRIDE;
};

#endif //ifdef WEBKITVIEW
#endif //ifndef GRAPHICSWEBKITVIEW_HPP
