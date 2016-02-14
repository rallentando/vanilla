#include "switch.hpp"
#include "const.hpp"

//[[!QWEV]]
#ifdef QTWEBKIT
//[[/!QWEV]]

#include "quickwebviewbase.hpp"

#include <QQmlContext>
#include <QAction>
#include <QVariant>
#include <QDrag>

#include "view.hpp"
#include "webpagebase.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"

#include <memory>

QuickWebViewBase::QuickWebViewBase(TreeBank *parent, QString id, QStringList set)
    : View(parent, id, set)
      //[[QWV]]
    , QQuickWidget(QUrl(QStringLiteral("qrc:/gen/quickwebview.qml")), parent)
      //[[/QWV]]
      //[[QWEV]]
    , QQuickWidget(QUrl(QStringLiteral("qrc:/gen/quickwebengineview.qml")), parent)
      //[[/QWEV]]
{
    Initialize();
    rootContext()->setContextProperty(QStringLiteral("viewInterface"), this);

    m_QmlWebViewBase = rootObject()->childItems().first();

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new WebPageBase(nam, this);
    ApplySpecificSettings(set);

    if(parent) setParent(parent);

    m_ActionTable = QMap<Page::CustomAction, QAction*>();
    m_RequestId = 0;

    connect(m_QmlWebViewBase, SIGNAL(callBackResult(int, QVariant)),
            this,             SIGNAL(CallBackResult(int, QVariant)));

    connect(m_QmlWebViewBase, SIGNAL(viewChanged()),
            this,             SIGNAL(ViewChanged()));
    connect(m_QmlWebViewBase, SIGNAL(scrollChanged(QPointF)),
            this,             SIGNAL(ScrollChanged(QPointF)));
}

QuickWebViewBase::~QuickWebViewBase(){
}

void QuickWebViewBase::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);

    if(!page()) return;

    //[[QWV]]
    SetPreference(QWebSettingsBase::AutoLoadImages,                    "AutoLoadImages");
    SetPreference(QWebSettingsBase::CaretBrowsingEnabled,              "CaretBrowsingEnabled");
    SetPreference(QWebSettingsBase::DeveloperExtrasEnabled,            "DeveloperExtrasEnabled");
    SetPreference(QWebSettingsBase::DnsPrefetchEnabled,                "DnsPrefetchEnabled");
    SetPreference(QWebSettingsBase::FrameFlatteningEnabled,            "FrameFlatteningEnabled");
    SetPreference(QWebSettingsBase::JavascriptEnabled,                 "JavascriptEnabled");
    SetPreference(QWebSettingsBase::LocalStorageEnabled,               "LocalStorageEnabled");
    SetPreference(QWebSettingsBase::NotificationsEnabled,              "NotificationsEnabled");
    SetPreference(QWebSettingsBase::OfflineWebApplicationCacheEnabled, "OfflineWebApplicationCacheEnabled");
    SetPreference(QWebSettingsBase::PluginsEnabled,                    "PluginsEnabled");
    SetPreference(QWebSettingsBase::PrivateBrowsingEnabled,            "PrivateBrowsingEnabled");
    SetPreference(QWebSettingsBase::LocalContentCanAccessFileUrls,     "LocalContentCanAccessFileUrls");
    SetPreference(QWebSettingsBase::LocalContentCanAccessRemoteUrls,   "LocalContentCanAccessRemoteUrls");
    SetPreference(QWebSettingsBase::XSSAuditingEnabled,                "XSSAuditingEnabled");
    SetPreference(QWebSettingsBase::WebAudioEnabled,                   "WebAudioEnabled");
    SetPreference(QWebSettingsBase::WebGLEnabled,                      "WebGLEnabled");
  //SetPreference(QWebSettingsBase::ErrorPageEnabled,                  "ErrorPageEnabled");
    //[[/QWV]]
    //[[QWEV]]
    SetPreference(QWebSettingsBase::AutoLoadImages,                    "AutoLoadImages");
  //SetPreference(QWebSettingsBase::CaretBrowsingEnabled,              "CaretBrowsingEnabled");
  //SetPreference(QWebSettingsBase::DeveloperExtrasEnabled,            "DeveloperExtrasEnabled");
  //SetPreference(QWebSettingsBase::DnsPrefetchEnabled,                "DnsPrefetchEnabled");
  //SetPreference(QWebSettingsBase::FrameFlatteningEnabled,            "FrameFlatteningEnabled");
    SetPreference(QWebSettingsBase::JavascriptEnabled,                 "JavascriptEnabled");
    SetPreference(QWebSettingsBase::LocalStorageEnabled,               "LocalStorageEnabled");
  //SetPreference(QWebSettingsBase::NotificationsEnabled,              "NotificationsEnabled");
  //SetPreference(QWebSettingsBase::OfflineWebApplicationCacheEnabled, "OfflineWebApplicationCacheEnabled");
  //SetPreference(QWebSettingsBase::PluginsEnabled,                    "PluginsEnabled");
  //SetPreference(QWebSettingsBase::PrivateBrowsingEnabled,            "PrivateBrowsingEnabled");
    SetPreference(QWebSettingsBase::LocalContentCanAccessFileUrls,     "LocalContentCanAccessFileUrls");
    SetPreference(QWebSettingsBase::LocalContentCanAccessRemoteUrls,   "LocalContentCanAccessRemoteUrls");
    SetPreference(QWebSettingsBase::XSSAuditingEnabled,                "XSSAuditingEnabled");
  //SetPreference(QWebSettingsBase::WebAudioEnabled,                   "WebAudioEnabled");
  //SetPreference(QWebSettingsBase::WebGLEnabled,                      "WebGLEnabled");
    SetPreference(QWebSettingsBase::ErrorPageEnabled,                  "ErrorPageEnabled");
    //[[/QWEV]]

    // for only QuickWebEngineView.
    SetPreference(QWebSettingsBase::HyperlinkAuditingEnabled,          "HyperlinkAuditingEnabled");
    SetPreference(QWebSettingsBase::JavascriptCanAccessClipboard,      "JavascriptCanAccessClipboard");
    SetPreference(QWebSettingsBase::JavascriptCanOpenWindows,          "JavascriptCanOpenWindows");
    SetPreference(QWebSettingsBase::LinksIncludedInFocusChain,         "LinksIncludedInFocusChain");

    SetFontFamily(QWebSettingsBase::StandardFont,  "StandardFont");
    SetFontFamily(QWebSettingsBase::FixedFont,     "FixedFont");
    SetFontFamily(QWebSettingsBase::SerifFont,     "SerifFont");
    SetFontFamily(QWebSettingsBase::SansSerifFont, "SansSerifFont");
    SetFontFamily(QWebSettingsBase::CursiveFont,   "CursiveFont");
    SetFontFamily(QWebSettingsBase::FantasyFont,   "FantasyFont");

    SetFontSize(QWebSettingsBase::MinimumFontSize,        "MinimumFontSize");
    SetFontSize(QWebSettingsBase::MinimumLogicalFontSize, "MinimumLogicalFontSize");
    SetFontSize(QWebSettingsBase::DefaultFontSize,        "DefaultFontSize");
    SetFontSize(QWebSettingsBase::DefaultFixedFontSize,   "DefaultFixedFontSize");

    QMetaObject::invokeMethod(m_QmlWebViewBase, "setUserAgent",
                              Q_ARG(QVariant, QVariant::fromValue(page()->userAgentForUrl(QUrl()))));
    QMetaObject::invokeMethod(m_QmlWebViewBase, "setDefaultTextEncoding",
                              Q_ARG(QVariant, QVariant::fromValue(page()->settings()->defaultTextEncoding())));
}

void QuickWebViewBase::Connect(TreeBank *tb){
    View::Connect(tb);

    if(!tb || !page()) return;

    connect(m_QmlWebViewBase, SIGNAL(titleChanged_(const QString&)),
            tb->parent(), SLOT(setWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        connect(this, SIGNAL(statusBarMessage(const QString&)),
                notifier, SLOT(SetStatus(const QString&)));
        connect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                notifier, SLOT(SetStatus(const QString&, const QString&)));
        connect(m_QmlWebViewBase, SIGNAL(statusBarMessage(const QString&)),
                notifier, SLOT(SetStatus(const QString&)));
        connect(m_QmlWebViewBase, SIGNAL(linkHovered_(const QString&, const QString&, const QString&)),
                notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

        connect(this, SIGNAL(ScrollChanged(QPointF)),
                notifier, SLOT(SetScroll(QPointF)));
        connect(notifier, SIGNAL(ScrollRequest(QPointF)),
                this, SLOT(SetScroll(QPointF)));
    }
    if(Receiver *receiver = tb->GetReceiver()){
        connect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                this, SLOT(Load(const QString&)));
        connect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                this, SLOT(SeekText(const QString&, View::FindFlags)));
        connect(receiver, SIGNAL(KeyEvent(QString)),
                this, SLOT(KeyEvent(QString)));

        connect(receiver, SIGNAL(SuggestRequest(const QUrl&)),
                page(), SLOT(DownloadSuggest(const QUrl&)));
        connect(page(), SIGNAL(SuggestResult(const QByteArray&)),
                receiver, SLOT(DisplaySuggest(const QByteArray&)));
    }
}

void QuickWebViewBase::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(!tb || !page()) return;

    disconnect(m_QmlWebViewBase, SIGNAL(titleChanged_(const QString&)),
               tb->parent(), SLOT(setWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        disconnect(this, SIGNAL(statusBarMessage(const QString&)),
                   notifier, SLOT(SetStatus(const QString&)));
        disconnect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                   notifier, SLOT(SetStatus(const QString&, const QString&)));
        disconnect(m_QmlWebViewBase, SIGNAL(statusBarMessage(const QString&)),
                   notifier, SLOT(SetStatus(const QString&)));
        disconnect(m_QmlWebViewBase, SIGNAL(linkHovered_(const QString&, const QString&, const QString&)),
                   notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

        disconnect(this, SIGNAL(ScrollChanged(QPointF)),
                   notifier, SLOT(SetScroll(QPointF)));
        disconnect(notifier, SIGNAL(ScrollRequest(QPointF)),
                   this, SLOT(SetScroll(QPointF)));
    }
    if(Receiver *receiver = tb->GetReceiver()){
        disconnect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                   this, SLOT(Load(const QString&)));
        disconnect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                   this, SLOT(SeekText(const QString&, View::FindFlags)));
        disconnect(receiver, SIGNAL(KeyEvent(QString)),
                   this, SLOT(KeyEvent(QString)));

        disconnect(receiver, SIGNAL(SuggestRequest(const QUrl&)),
                   page(), SLOT(DownloadSuggest(const QUrl&)));
        disconnect(page(), SIGNAL(SuggestResult(const QByteArray&)),
                   receiver, SLOT(DisplaySuggest(const QByteArray&)));
    }
}

//[[QWEV]]
void QuickWebViewBase::ZoomIn(){
    float zoom = PrepareForZoomIn();
    m_QmlWebViewBase->setProperty("devicePixelRatio", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickWebViewBase::ZoomOut(){
    float zoom = PrepareForZoomOut();
    m_QmlWebViewBase->setProperty("devicePixelRatio", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}
//[[/QWEV]]

QAction *QuickWebViewBase::Action(QWebPageBase::WebAction a){
    switch(a){
    case QWebPageBase::Reload:  return Action(Page::We_Reload);
    case QWebPageBase::Stop:    return Action(Page::We_Stop);
    case QWebPageBase::Back:    return Action(Page::We_Back);
    case QWebPageBase::Forward: return Action(Page::We_Forward);
    }
    return page()->Action(a);
}

QAction *QuickWebViewBase::Action(Page::CustomAction a, QVariant data){
    QAction *action = m_ActionTable[a];

    if(action) return action;

    if(a == Page::We_Reload ||
       a == Page::We_Stop ||
       a == Page::We_Back ||
       a == Page::We_Forward){

        m_ActionTable[a] = action = new QAction(this);

        switch(a){
        case Page::We_Reload:
            action->setText(tr("Reload"));
            connect(action, SIGNAL(triggered()), m_QmlWebViewBase, SLOT(reload()));
            break;
        case Page::We_Stop:
            action->setText(tr("Stop"));
            connect(action, SIGNAL(triggered()), m_QmlWebViewBase, SLOT(stop()));
            break;
        case Page::We_Back:
            action->setText(tr("Back"));
            connect(action, SIGNAL(triggered()), m_QmlWebViewBase, SLOT(goBack()));
            break;
        case Page::We_Forward:
            action->setText(tr("Forward"));
            connect(action, SIGNAL(triggered()), m_QmlWebViewBase, SLOT(goForward()));
            break;
        }
        action->setData(data);
        return action;
    }
    return page()->Action(a, data);
}

void QuickWebViewBase::OnViewChanged(){
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void QuickWebViewBase::OnScrollChanged(){
    SaveScroll();
}

void QuickWebViewBase::CallWithScroll(PointFCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toPointF());
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(GetScrollRatioPointJsCode())));
}

void QuickWebViewBase::SetScrollBarState(){
    //[[QWV]]
    m_ScrollBarState = NoScrollBarEnabled;
    //[[/QWV]]
    //[[QWEV]]
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    if(!result.isValid()) return;
                    QVariantList list = result.toList();
                    int hmax = list[0].toInt();
                    int vmax = list[1].toInt();
                    if(hmax < 0) hmax = 0;
                    if(vmax < 0) vmax = 0;
                    if(hmax && vmax) m_ScrollBarState = BothScrollBarEnabled;
                    else if(hmax)    m_ScrollBarState = HorizontalScrollBarEnabled;
                    else if(vmax)    m_ScrollBarState = VerticalScrollBarEnabled;
                    else             m_ScrollBarState = NoScrollBarEnabled;
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(GetScrollBarStateJsCode())));
    //[[/QWEV]]
}

void QuickWebViewBase::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool QuickWebViewBase::SeekText(const QString &str, View::FindFlags opt){
    QMetaObject::invokeMethod(m_QmlWebViewBase, "seekText",
                              Q_ARG(QVariant, QVariant::fromValue(str)),
                              Q_ARG(QVariant, QVariant::fromValue(static_cast<int>(opt))));
    return true;
}

void QuickWebViewBase::hideEvent(QHideEvent *ev){
    SaveViewState();
    QQuickWidget::hideEvent(ev);
}

void QuickWebViewBase::showEvent(QShowEvent *ev){
    QQuickWidget::showEvent(ev);
    RestoreViewState();
}

void QuickWebViewBase::keyPressEvent(QKeyEvent *ev){
    if(!visible()) return;

    // all key events are ignored, if input method is activated.
    // so input method specific keys are accepted.
    if(Application::HasAnyModifier(ev) ||
       // 'HasAnyModifier' ignores ShiftModifier.
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    QQuickWidget::keyPressEvent(ev);

    if(!ev->isAccepted() &&
       !Application::IsOnlyModifier(ev)){

        if(NavigationBySpaceKey() &&
           ev->key() == Qt::Key_Space){

            if(ev->modifiers() & Qt::ShiftModifier){
                GetTreeBank()->Back(GetHistNode());
            } else {
                GetTreeBank()->Forward(GetHistNode());
            }
            ev->setAccepted(true);
            return;
        }

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void QuickWebViewBase::keyReleaseEvent(QKeyEvent *ev){
    if(!visible()) return;

    QQuickWidget::keyReleaseEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");
}

void QuickWebViewBase::resizeEvent(QResizeEvent *ev){
    QQuickWidget::resizeEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebViewBase, "adjustContents");
}

void QuickWebViewBase::contextMenuEvent(QContextMenuEvent *ev){
    ev->setAccepted(true);
}

void QuickWebViewBase::mouseMoveEvent(QMouseEvent *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QQuickWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
        return;
    }
    if(ev->buttons() & Qt::RightButton &&
       !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_RightGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_RightGestureMap[gesture])
              ? Action(Page::StringToAction(m_RightGestureMap[gesture]))->text()
            : m_RightGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
        ev->setAccepted(false);
        return;
    }

    int scrollBarWidth = Application::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    bool horizontal = m_ScrollBarState == BothScrollBarEnabled
        ||            m_ScrollBarState == HorizontalScrollBarEnabled;
    bool vertical   = m_ScrollBarState == BothScrollBarEnabled
        ||            m_ScrollBarState == VerticalScrollBarEnabled;
    QRect touchableRect =
        QRect(QPoint(),
              size() - QSize(vertical   ? scrollBarWidth : 0,
                             horizontal ? scrollBarWidth : 0));

    if(ev->buttons() & Qt::LeftButton &&
       !m_GestureStartedPos.isNull() &&
       touchableRect.contains(m_GestureStartedPos) &&
       (m_ClickedElement &&
        !m_ClickedElement->IsNull() &&
        !m_ClickedElement->IsEditableElement())){

        if(QLineF(ev->pos(), m_GestureStartedPos).length() < 2){
            // gesture not aborted.
            QQuickWidget::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QDrag *drag = new QDrag(this);

        // clear or make directory if need.
        Application::ClearTemporaryDirectory();

        NetworkAccessManager *nam =
            static_cast<NetworkAccessManager*>(page()->networkAccessManager());

        QMimeData *mime = m_HadSelection
            ? CreateMimeDataFromSelection(nam)
            : CreateMimeDataFromElement(nam);

        if(!mime){
            drag->deleteLater();

            // call default behavior.
            GestureAborted();
            QQuickWidget::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QPixmap pixmap = m_HadSelection
            ? CreatePixmapFromSelection()
            : CreatePixmapFromElement();

        //[[WEV]]
        QRect rect = m_HadSelection
            ? m_SelectionRegion.boundingRect()
            : m_ClickedElement->Rectangle().intersected(QRect(QPoint(), size()));
        QPoint pos = ev->pos() - rect.topLeft();
        //[[/WEV]]

        if(pixmap.size().width()  > MAX_DRAGGING_PIXMAP_WIDTH ||
           pixmap.size().height() > MAX_DRAGGING_PIXMAP_HEIGHT){

            pos /= qMax(static_cast<float>(pixmap.size().width()) /
                        static_cast<float>(MAX_DRAGGING_PIXMAP_WIDTH),
                        static_cast<float>(pixmap.size().height()) /
                        static_cast<float>(MAX_DRAGGING_PIXMAP_HEIGHT));
            pixmap = pixmap.scaled(MAX_DRAGGING_PIXMAP_WIDTH,
                                   MAX_DRAGGING_PIXMAP_HEIGHT,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        }

        if(m_HadSelection){
            mime->setImageData(pixmap.toImage());
        } else {
            QPixmap element = m_ClickedElement->Pixmap();
            if(element.isNull())
                mime->setImageData(pixmap.toImage());
            else
                mime->setImageData(element.toImage());
        }
        if(m_EnableDragHackLocal){
            GestureMoved(ev->pos());
        } else {
            GestureAborted();
        }
        m_DragStarted = true;
        drag->setMimeData(mime);
        drag->setPixmap(pixmap);
        drag->setHotSpot(pos);
        drag->exec(Qt::CopyAction | Qt::MoveAction);
        drag->deleteLater();
        ev->setAccepted(true);
    } else {
        // call default behavior.
        GestureAborted();
        QQuickWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
    }
}

void QuickWebViewBase::mousePressEvent(QMouseEvent *ev){
    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(m_MouseMap.contains(mouse)){

        QString str = m_MouseMap[mouse];
        if(!str.isEmpty()){
            if(!View::TriggerAction(str, ev->pos())){
                ev->setAccepted(false);
                return;
            }
            GestureAborted();
            ev->setAccepted(true);
            return;
        }
    }

    GestureStarted(ev->pos());
    QQuickWidget::mousePressEvent(ev);
    ev->setAccepted(true);
}

void QuickWebViewBase::mouseReleaseEvent(QMouseEvent *ev){
    emit statusBarMessage(QString());

    QUrl link = m_ClickedElement ? m_ClickedElement->LinkUrl() : QUrl();

    if(!link.isEmpty() &&
       m_Gesture.isEmpty() &&
       (ev->button() == Qt::LeftButton ||
        ev->button() == Qt::MidButton)){

        QNetworkRequest req(link);
        req.setRawHeader("Referer", url().toEncoded());

        if(Application::keyboardModifiers() & Qt::ShiftModifier ||
           Application::keyboardModifiers() & Qt::ControlModifier ||
           ev->button() == Qt::MidButton){

            GestureAborted();
            m_TreeBank->OpenInNewViewNode(req, Page::Activate(), GetViewNode());
            ev->setAccepted(true);
            return;

        } else if(
            // it's requirements of starting loadhack.
            // loadhack uses new hist node instead of same view's `load()'.
            m_EnableLoadHackLocal
            // url is not empty.
            && !url().isEmpty()
            // link doesn't hold jump command.
            && !link.toEncoded().contains("#")
            // m_ClickedElement doesn't hold javascript function.
            && !m_ClickedElement->IsJsCommandElement()){

            GestureAborted();
            m_TreeBank->OpenInNewHistNode(req, true, GetHistNode());
            ev->setAccepted(true);
            return;
        }
    }

    if(ev->button() == Qt::RightButton){

        if(!m_Gesture.isEmpty()){
            GestureFinished(ev->pos(), ev->button());
        } else if(!m_GestureStartedPos.isNull()){
            SharedWebElement elem = m_ClickedElement;
            GestureAborted(); // resets 'm_ClickedElement'.
            page()->DisplayContextMenu(m_TreeBank, elem, ev->pos(), ev->globalPos());
        }
        ev->setAccepted(true);
        return;
    }
    if(ev->button() == Qt::LeftButton &&
       m_EnableDragHackLocal && !m_GestureStartedPos.isNull()){

        if(!m_Gesture.isEmpty()){
            GestureFinished(ev->pos(), ev->button());
        } else {
            GestureAborted();
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QQuickWidget::mouseReleaseEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");
    ev->setAccepted(true);
}

void QuickWebViewBase::mouseDoubleClickEvent(QMouseEvent *ev){
    QQuickWidget::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void QuickWebViewBase::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QQuickWidget::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void QuickWebViewBase::dragMoveEvent(QDragMoveEvent *ev){
    if(m_EnableDragHackLocal && !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_LeftGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_LeftGestureMap[gesture])
              ? Action(Page::StringToAction(m_LeftGestureMap[gesture]))->text()
            : m_LeftGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
    }
    QQuickWidget::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void QuickWebViewBase::dropEvent(QDropEvent *ev){
    emit statusBarMessage(QString());
    QPoint pos = ev->pos();
    QList<QUrl> urls = ev->mimeData()->urls();
    QObject *source = ev->source();
    QWidget *widget = this;
    QString text;
    if(!ev->mimeData()->text().isEmpty()){
        text = ev->mimeData()->text().replace(QStringLiteral("\""), QStringLiteral("\\\""));
    } else if(!urls.isEmpty()){
        foreach(QUrl u, urls){
            if(text.isEmpty()) text = u.toString();
            else text += QStringLiteral("\n") + u.toString();
        }
    }

    CallWithHitElement(pos, [this, pos, urls, text, source, widget](SharedWebElement elem){

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        elem->SetText(text);
        return;
    }

    if(!m_Gesture.isEmpty() && source == widget){
        GestureFinished(pos, Qt::LeftButton);
        return;
    }

    GestureAborted();

    if(urls.isEmpty() || source == widget){
        // do nothing.
    } else if(qobject_cast<TreeBank*>(source) || dynamic_cast<View*>(source)){
        QList<QUrl> filtered;
        foreach(QUrl u, urls){ if(!u.isLocalFile()) filtered << u;}
        m_TreeBank->OpenInNewViewNode(filtered, true, GetViewNode());
        return;
    } else {
        // foreign drag.
        m_TreeBank->OpenInNewViewNode(urls, true, GetViewNode());
    }

    });

    QQuickWidget::dropEvent(ev);
    ev->setAccepted(true);
}

void QuickWebViewBase::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QQuickWidget::dragLeaveEvent(ev);
}

void QuickWebViewBase::wheelEvent(QWheelEvent *ev){
    if(!visible()) return;

    QString wheel;
    bool up = ev->delta() > 0;

    Application::AddModifiersToString(wheel, ev->modifiers());
    Application::AddMouseButtonsToString(wheel, ev->buttons());
    Application::AddWheelDirectionToString(wheel, up);

    if(m_MouseMap.contains(wheel)){

        QString str = m_MouseMap[wheel];
        if(!str.isEmpty()){
            View::TriggerAction(str, ev->pos());
        }
        ev->setAccepted(true);

    } else {
        QQuickWidget::wheelEvent(ev);
        ev->setAccepted(true);
    }
    QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");
}

void QuickWebViewBase::focusInEvent(QFocusEvent *ev){
    QQuickWidget::focusInEvent(ev);
    OnFocusIn();
}

void QuickWebViewBase::focusOutEvent(QFocusEvent *ev){
    QQuickWidget::focusOutEvent(ev);
    OnFocusOut();
}

bool QuickWebViewBase::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QQuickWidget::focusNextPrevChild(next);
    return false;
}

void QuickWebViewBase::CallWithGotBaseUrl(UrlCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant url){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(url.toUrl());
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(GetBaseUrlJsCode())));
}

void QuickWebViewBase::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    // this implementation is same as baseurl...
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant url){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(url.toUrl());
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(GetBaseUrlJsCode())));
}

void QuickWebViewBase::CallWithFoundElements(Page::FindElementsOption option,
                                             WebElementListCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant var){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    QVariantList list = var.toMap().values();
                    SharedWebElementList result;

                    MainWindow *win = Application::GetCurrentWindow();
                    QSize s =
                        m_TreeBank ? m_TreeBank->size() :
                        win ? win->GetTreeBank()->size() :
                        !size().isEmpty() ? size() :
                        DEFAULT_WINDOW_SIZE;
                    QRect viewport = QRect(QPoint(), s);

                    for(int i = 0; i < list.length(); i++){
                        std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
                        *e = JsWebElement(this, list[i]);
                        if(!viewport.intersects(e->Rectangle()))
                            e->SetRectangle(QRect());
                        result << e;
                    }
                    callBack(result);
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(FindElementsJsCode(option))));
}

void QuickWebViewBase::CallWithHitElement(const QPoint &pos,
                                          WebElementCallBack callBack){
    if(pos.isNull()){
        callBack(SharedWebElement());
        return;
    }
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant var){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
                    *e = JsWebElement(this, var);
                    callBack(e);
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(HitElementJsCode(pos / m_HistNode->GetZoom()))));
}

void QuickWebViewBase::CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()){
        callBack(QUrl());
        return;
    }
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant url){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(url.toUrl());
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(HitLinkUrlJsCode(pos / m_HistNode->GetZoom()))));
}

void QuickWebViewBase::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()){
        callBack(QUrl());
        return;
    }
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant url){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(url.toUrl());
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(HitImageUrlJsCode(pos / m_HistNode->GetZoom()))));
}

void QuickWebViewBase::CallWithSelectedText(StringCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toString());
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(SelectedTextJsCode())));
}

void QuickWebViewBase::CallWithSelectedHtml(StringCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toString());
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(SelectedHtmlJsCode())));
}

void QuickWebViewBase::CallWithWholeText(StringCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toString());
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(WholeTextJsCode())));
}

void QuickWebViewBase::CallWithWholeHtml(StringCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toString());
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(WholeHtmlJsCode())));
}

void QuickWebViewBase::CallWithSelectionRegion(RegionCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant var){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    QRegion region;
                    if(!var.isValid() || !var.canConvert(QMetaType::QVariantMap)){
                        callBack(region);
                        return;
                    }
                    QVariantMap map = var.toMap();
                    QRect viewport = QRect(QPoint(), size());
                    foreach(QString key, map.keys()){
                        QVariantMap m = map[key].toMap();
                        region |= QRect(m["x"].toInt(),
                                        m["y"].toInt(),
                                        m["width"].toInt(),
                                        m["height"].toInt()).intersected(viewport);
                    }
                    callBack(region);
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(SelectionRegionJsCode())));
}

void QuickWebViewBase::CallWithEvaluatedJavaScriptResult(const QString &code,
                                                         VariantCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebViewBase::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result);
                });

    QMetaObject::invokeMethod(m_QmlWebViewBase, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(code)));
}

//[[!QWEV]]
#endif
//[[/!QWEV]]
