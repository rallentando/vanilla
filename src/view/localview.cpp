#include "const.hpp"
#include "switch.hpp"

#ifdef LOCALVIEW

#include "localview.hpp"

#include "devicescale.hpp"

#include <QNetworkRequest>
#include <QDesktopServices>
#include <QFileSystemModel>
#include <QGuiApplication>
#include <QScreen>
#include <QFileIconProvider>
#include <QIcon>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QtAudio>
#include <QMediaMetaData>
#include <QGraphicsVideoItem>
#include <QtConcurrent/QtConcurrent>
#include <QStyle>
#include <QDir>
#include <QFileInfo>
#include <QClipboard>
#include <QMimeData>
#include <QPointer>

#include "fileoperation.hpp"
#include "mediatype.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "thumbnail.hpp"
#include "nodetitle.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "gadgetsstyle.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "quicknativewebview.hpp"
#include "edgewebview.hpp"
#include "dialog.hpp"

int LocalView::m_MediaVolume = 50;
bool LocalView::m_AutoPlayMedia = true;

LocalView::LocalView(TreeBank *parent, QString id, QStringList set)
    : GraphicsTableView(parent)
    , View(parent)
{
    Initialize();
    setZValue(COVERING_VIEW_CONTENTS_LAYER);

    SetSortPredicate();

    m_ParentNode = new LocalNode();

    class DummyLocalNode : public LocalNode {
        bool IsDummy() const Q_DECL_OVERRIDE { return true;}
    };

    m_DummyLocalNode = new DummyLocalNode();
    m_DummyLocalNode->SetParent(m_ParentNode);

    m_FileOperationRunning = false;
    connect(&m_ReachWatcher, &QFutureWatcher<FileOperation::Reach>::finished,
            this, &LocalView::AskThenDelete, Qt::QueuedConnection);
    connect(&m_WorkWatcher, &QFutureWatcher<QStringList>::finished,
            this, &LocalView::ReportFileOperation);

    m_StillFitsWindow = true;

    m_PixmapItem = new PixmapItem(this);
    m_PixmapItem->setZValue(MULTIMEDIA_LAYER);
    m_PixmapItem->setEnabled(false);
    m_PixmapItem->hide();

    m_VideoItem = new VideoItem(this);
    m_VideoItem->setZValue(MULTIMEDIA_LAYER);
    m_VideoItem->setEnabled(false);
    m_VideoItem->hide();

    m_MediaPlayer = new QMediaPlayer(this);
    m_MediaPlayer->setVideoOutput(m_VideoItem);

    m_AudioOutput = new QAudioOutput(this);
    m_AudioOutput->setVolume(QtAudio::convertVolume(m_MediaVolume / 100.0,
                                                    QtAudio::LogarithmicVolumeScale,
                                                    QtAudio::LinearVolumeScale));
    m_MediaPlayer->setAudioOutput(m_AudioOutput);

    connect(m_MediaPlayer, &QMediaPlayer::metaDataChanged,
            this, &LocalView::OnMediaMetaDataChanged);
    connect(m_MediaPlayer, &QMediaPlayer::errorOccurred,
            this, &LocalView::OnMediaError);

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new Page(this, nam);
    page()->SetView(this);
    ApplySpecificSettings(set);

    if(parent) setParent(parent);

    hide();

}

LocalView::~LocalView(){
    m_ParentNode->Delete();
    m_DummyLocalNode->Delete();
}

QGraphicsObject *LocalView::base(){
    return static_cast<QGraphicsObject*>(this);
}

Page *LocalView::page(){
    return static_cast<Page*>(View::page());
}

TreeBank *LocalView::parent(){
    return View::m_TreeBank;
}

void LocalView::setParent(TreeBank *tb){
    View::SetTreeBank(tb);
    GraphicsTableView::SetTreeBank(tb);
    if(base()->scene()) scene()->removeItem(this);
    if(tb) tb->GetScene()->addItem(this);
}

void LocalView::Load(const QUrl &url){
    TreeBank::AddToUpdateBox(GetThis().lock());

    StopImageCollector();

    QString path = url.toLocalFile();
    bool supported = false;
    if(IsSupported(url)){
        QStringList splited = path.split(QStringLiteral("/"));
        splited.removeLast();
        path = splited.join(QStringLiteral("/"));
        supported = true;
    }
    QUrl directory = QUrl::fromLocalFile(path);

    m_DisplayType = LocalFolderTree;

    foreach(Node *nd, m_ParentNode->GetChildren()){
        nd->Delete();
    }
    m_ParentNode->ClearChildren();
    m_ParentNode->SetUrl(directory);

    RegisterNodes(directory);

    if(!m_ParentNode->HasNoChildren() && supported){
        NodeList children = m_ParentNode->GetChildren();
        for(int i = 0; i < children.length(); i++){
            if(children[i]->GetUrl() == url){
                CollectNodes(children[i]);
                SwapMediaItem(i);
            }
        }
    } else {
        m_DummyLocalNode->SetParent(m_ParentNode);
        CollectNodes(m_DummyLocalNode);
    }

    m_UpDirectoryButton->SetState(GraphicsButton::NotHovered);

    if(url.toLocalFile() != QStringLiteral("/")){
        m_UpDirectoryButton->setEnabled(true);
        m_UpDirectoryButton->setVisible(true);
    } else {
        m_UpDirectoryButton->setEnabled(false);
        m_UpDirectoryButton->setVisible(false);
    }

    emit urlChanged(url);
    emit titleChanged(url.toLocalFile());

    OnLoadFinished(true);

    StartImageCollector(false);
}

void LocalView::Load(const QNetworkRequest &req){
    Load(req.url());
}

void LocalView::Resize(QSizeF size){
    GraphicsTableView::Resize(size);
    Node *nd = m_CurrentNode;
    if(!nd || nd->IsDummy()) return;

    if(!m_StillPixmap.isNull()){
        RelayoutStillPixmap();
        m_PixmapItem->setFocus();
    } else if(
              !m_MediaPlayer->source().isEmpty()
              && IsSupportedVideo(nd->GetUrl())){
        m_VideoItem->setSize(Size());
        m_VideoItem->setOffset(QPointF());
        m_VideoItem->setFocus();
    }
}

QMenu *LocalView::CreateNodeMenu(){
    if(!IsDisplayingNode()) return 0;

    QMenu *menu = new QMenu(GetTreeBank());
    menu->setToolTipsVisible(true);

    menu->addAction(Action(Gadgets::_Deactivate));
    menu->addAction(Action(Gadgets::_Refresh));
    menu->addSeparator();

    if(m_HoveredItemIndex != -1 &&
       m_HoveredItemIndex < m_DisplayThumbnails.length()){

        menu->addAction(Action(Gadgets::_OpenNode));
        menu->addAction(Action(Gadgets::_OpenNodeOnNewWindow));
        menu->addAction(Action(Gadgets::_RenameNode));
        if(GetHoveredNode()->IsDirectory())
            menu->addAction(Action(Gadgets::_DownDirectory));
    }
    if(m_ParentNode &&
       m_ParentNode->GetUrl().toString() != QStringLiteral("file:///")){

        menu->addAction(Action(Gadgets::_UpDirectory));
    }
    if((m_HoveredItemIndex != -1 &&
        m_HoveredItemIndex < m_DisplayThumbnails.length()) ||
       !m_NodesRegister.isEmpty()){

        menu->addAction(Action(Gadgets::_DeleteNode));
        menu->addAction(Action(Gadgets::_DeleteRightNode));
        menu->addAction(Action(Gadgets::_DeleteLeftNode));
        menu->addAction(Action(Gadgets::_DeleteOtherNode));
    }
    menu->addSeparator();
    menu->addMenu(CreateSortMenu(menu));
    return menu;
}

void LocalView::RenderBackground(QPainter *painter){
    if(!GetTreeBank()) return;

    bool nativeMasterBehind = false;
#ifdef NATIVEWEBVIEW
    if(SharedView master = GetMaster().lock())
        nativeMasterBehind = qobject_cast<QuickNativeWebView*>(master->base()) != nullptr;
#endif
#ifdef EDGEWEBVIEW
    if(!nativeMasterBehind)
        if(SharedView master = GetMaster().lock())
            nativeMasterBehind = qobject_cast<EdgeWebView*>(master->base()) != nullptr;
#endif
    if(GetStyle()->StyleName() != QStringLiteral("FlatStyle")
       && !nativeMasterBehind){
        GraphicsTableView::RenderBackground(painter);
        return;
    }

    View *view = 0;

    if(SharedView master = GetMaster().lock()){
        if(master->IsRenderable()){
            view = master.get();
        }
    }

    if(!view && GetTreeBank()->GetCurrentView()){
        if(0);
#ifdef WEBENGINEVIEW
        else if(WebEngineView *w = qobject_cast<WebEngineView*>(GetTreeBank()->GetCurrentView()->base()))
            view = w;
        else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(GetTreeBank()->GetCurrentView()->base()))
            view = w;
#endif
#ifdef NATIVEWEBVIEW
        else if(QuickNativeWebView *w = qobject_cast<QuickNativeWebView*>(GetTreeBank()->GetCurrentView()->base()))
            view = w;
#endif
#ifdef EDGEWEBVIEW
        else if(EdgeWebView *w = qobject_cast<EdgeWebView*>(GetTreeBank()->GetCurrentView()->base()))
            view = w;
#endif
        else;
    }

    if(view){
        if(!view->visible())
            view->SetViewportSize(Size().toSize());

        int width_diff  = Size().width()  - view->GetViewportSize().width();
        int height_diff = Size().height() - view->GetViewportSize().height();

        painter->save();

        if(width_diff != 0 || height_diff != 0)
            painter->translate(width_diff / 2.0, height_diff / 2.0);
        view->Render(painter);

        painter->restore();
    }

    GraphicsTableView::RenderBackground(painter);
}

bool LocalView::IsSupported(QUrl url){
    return MediaType::IsMedia(url.toLocalFile());
}

bool LocalView::IsSupported(QString path){
    return MediaType::IsMedia(path);
}

bool LocalView::IsSupportedImage(QUrl url){
    return MediaType::IsImage(url.toLocalFile());
}

bool LocalView::IsSupportedImage(QString path){
    return MediaType::IsImage(path);
}

bool LocalView::IsSupportedAudio(QUrl url){
    return MediaType::IsAudio(url.toLocalFile());
}

bool LocalView::IsSupportedAudio(QString path){
    return MediaType::IsAudio(path);
}

bool LocalView::IsSupportedVideo(QUrl url){
    return MediaType::IsVideo(url.toLocalFile());
}

bool LocalView::IsSupportedVideo(QString path){
    return MediaType::IsVideo(path);
}

void LocalView::RegisterNodes(const QUrl &url){
    QString path = url.toLocalFile();

#ifdef Q_OS_WIN
    if(url.toString() == QStringLiteral("file:///")){
        for(char c = 'A'; c <= 'Z'; c++){
            QString rootpath = QString(c) + QStringLiteral(":/");
            QDir rootdir = QDir(rootpath);
            if(rootdir.exists() && rootdir.isReadable()){
                LocalNode *nd = new LocalNode();
                nd->SetUrl(QUrl::fromLocalFile(rootpath));
                nd->SetParent(m_ParentNode);
                m_ParentNode->AppendChild(nd);
            }
        }
    } else
#endif
    {
        QDir dir = QDir(path);
        QStringList list = dir.entryList(QDir::NoDotAndDotDot|QDir::AllEntries|QDir::Hidden);

        int cost = qMin(list.length(), MAXIMUM_LOCALVIEW_MAX_FILEIMAGE);
        if(LocalNode::m_FileImageCache.maxCost() < cost)
            LocalNode::m_FileImageCache.setMaxCost(cost);

        for(int i = 0; i < list.length(); i++){
            QString file = list[i];
            LocalNode *nd = new LocalNode();
            QString filepath = path.endsWith(QStringLiteral("/")) ?
                path + file : path + QStringLiteral("/") + file;
            nd->SetUrl(QUrl::fromLocalFile(filepath));
            nd->SetParent(m_ParentNode);
            m_ParentNode->AppendChild(nd);
        }
    }
}

void LocalView::Activate(DisplayType type){
    Q_UNUSED(type)
    show();
}

void LocalView::Deactivate(){
    UpdateThumbnail();
    GraphicsTableView::Deactivate();

    if(GetTreeBank()){
        if(SharedView master = GetMaster().lock()){
            GetTreeBank()->SetCurrent(master);

            if(GetThis().lock() == master->GetSlave().lock())
                master->SetSlave(WeakView());
            SetMaster(WeakView());

            master->OnAfterFinishingDisplayGadgets();
        }
    }
}

void LocalView::show(){
    QGraphicsObject::show();
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::ArrowCursor);

    m_HoveredItemIndex = -1;
    m_PrimaryItemIndex = -1;

    if(EnableFrameRate()){
        StartFrameRateTimer();
    }
    StartAutoUpdateTimer();

    if(!m_StillPixmap.isNull()){
        RelayoutStillPixmap();
        m_PixmapItem->setEnabled(true);
        m_PixmapItem->show();
        m_PixmapItem->setFocus();
    } else if(
              !m_MediaPlayer->source().isEmpty()
              ){
        m_VideoItem->setEnabled(true);
        m_VideoItem->show();
        m_VideoItem->setFocus();
    }
    if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
    if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();

    RestoreViewState();
    if(!GetTreeBank() || !GetTreeBank()->GetNotifier()) return;
    GetTreeBank()->GetNotifier()->SetScroll(GetScroll());
}

void LocalView::hide(){
    SaveViewState();

    QGraphicsObject::hide();
    m_NodesRegister.clear();

}

void LocalView::ClearCache(){
    LocalNode::m_FileImageCache.clear();
}

void LocalView::LoadSettings(){
    Settings &s = Application::GlobalSettings();

    m_MediaVolume   = s.value(QStringLiteral("localview/@MediaVolume"),   50)  .value<int>();
    m_AutoPlayMedia = s.value(QStringLiteral("localview/@AutoPlayMedia"), true).value<bool>();

    m_MediaVolume = qBound(0, m_MediaVolume, 100);
}

void LocalView::Connect(TreeBank *tb){
    View::Connect(tb);

    if(tb){
        connect(this, SIGNAL(titleChanged(const QString&)),
                tb->parent(), SLOT(SetWindowTitle(const QString&)));
        if(Notifier *notifier = tb->GetNotifier()){
            connect(this, SIGNAL(statusBarMessage(const QString&)),
                    notifier, SLOT(SetStatus(const QString&)));
            connect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                    notifier, SLOT(SetStatus(const QString&, const QString&)));
            connect(this, SIGNAL(ItemHovered(const QString&, const QString&, const QString&)),
                    notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

            connect(this, SIGNAL(ScrollChanged(QPointF)),
                    notifier, SLOT(SetScroll(QPointF)));
            connect(notifier, SIGNAL(ScrollRequest(QPointF)),
                    this, SLOT(SetScroll(QPointF)));
        }
        if(Receiver *receiver = tb->GetReceiver()){
            connect(receiver, SIGNAL(Download(QString, QString)),
                    this, SLOT(Download(QString, QString)));
            connect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                    this, SLOT(SeekText(const QString&, View::FindFlags)));
            connect(receiver, SIGNAL(KeyEvent(QString)),
                    this, SLOT(KeyEvent(QString)));
            connect(receiver, SIGNAL(OpenNodeWithCommand(QString)),
                    this, SLOT(ThumbList_OpenNodeWithCommand(QString)));

        }
    }

    if(m_VideoItem){
        connect(m_VideoItem, SIGNAL(statusBarMessage(const QString&)),
                this, SIGNAL(statusBarMessage(const QString&)));
    }
}

void LocalView::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(tb){
        disconnect(this, SIGNAL(titleChanged(const QString&)),
                   tb->parent(), SLOT(SetWindowTitle(const QString&)));
        if(Notifier *notifier = tb->GetNotifier()){
            disconnect(this, SIGNAL(statusBarMessage(const QString&)),
                       notifier, SLOT(SetStatus(const QString&)));
            disconnect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                       notifier, SLOT(SetStatus(const QString&, const QString&)));
            disconnect(this, SIGNAL(ItemHovered(const QString&, const QString&, const QString&)),
                       notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

            disconnect(this, SIGNAL(ScrollChanged(QPointF)),
                       notifier, SLOT(SetScroll(QPointF)));
            disconnect(notifier, SIGNAL(ScrollRequest(QPointF)),
                       this, SLOT(SetScroll(QPointF)));
        }
        if(Receiver *receiver = tb->GetReceiver()){
            disconnect(receiver, SIGNAL(Download(QString, QString)),
                       this, SLOT(Download(QString, QString)));
            disconnect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                       this, SLOT(SeekText(const QString&, View::FindFlags)));
            disconnect(receiver, SIGNAL(KeyEvent(QString)),
                       this, SLOT(KeyEvent(QString)));
            disconnect(receiver, SIGNAL(OpenNodeWithCommand(QString)),
                       this, SLOT(ThumbList_OpenNodeWithCommand(QString)));
        }
    }

    if(m_VideoItem){
        disconnect(m_VideoItem, SIGNAL(statusBarMessage(const QString&)),
                   this, SIGNAL(statusBarMessage(const QString&)));
    }
}

void LocalView::UpdateThumbnail(){
    if(GetViewNode()){
        MainWindow *win = Application::GetCurrentWindow();
        QSize parentsize =
            GetTreeBank() ? GetTreeBank()->size() :
            win ? win->GetTreeBank()->size() :
            !size().isEmpty() ? size() :
            DEFAULT_WINDOW_SIZE;

        QImage image(parentsize, QImage::Format_ARGB32);
        QPainter painter(&image);
        paint(&painter);

        int count = 0;
        foreach(QGraphicsItem *item, childItems()){
            if(item->isVisible()){
                if(item->zValue() > 20.0) break;

                item->paint(&painter, 0);
                count++;
            }
        }

        painter.end();

        if(!count) return;

        parentsize.scale(SAVING_THUMBNAIL_SIZE,
                         Qt::KeepAspectRatioByExpanding);

        int width_diff  = parentsize.width()  - SAVING_THUMBNAIL_SIZE.width();
        int height_diff = parentsize.height() - SAVING_THUMBNAIL_SIZE.height();

        if(width_diff == 0 && height_diff == 0){
            GetViewNode()->SetImage(
                image.
                scaled(SAVING_THUMBNAIL_SIZE,
                       Qt::KeepAspectRatioByExpanding,
                       Qt::SmoothTransformation));
        } else {
            GetViewNode()->SetImage(
                image.
                scaled(parentsize,
                       Qt::KeepAspectRatio,
                       Qt::SmoothTransformation).
                copy(width_diff / 2, height_diff / 2,
                     SAVING_THUMBNAIL_SIZE.width(),
                     SAVING_THUMBNAIL_SIZE.height()));
        }
    }
}

bool LocalView::TriggerKeyEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return false;
    QString str = Gadgets::GetThumbListKeyMap()[seq];
    if(str.isEmpty()) return false;
    TriggerAction(Gadgets::StringToAction(str));
    return true;
}

bool LocalView::TriggerKeyEvent(QString str){
    QKeySequence seq = Application::MakeKeySequence(str);
    if(seq.isEmpty()) return false;
    str = Gadgets::GetThumbListKeyMap()[seq];
    if(str.isEmpty()) return false;
    TriggerAction(Gadgets::StringToAction(str));
    return true;
}

bool LocalView::TriggerAction(QString str, QVariant data){
    Q_UNUSED(data)
    if(Gadgets::IsValidAction(str))
        TriggerAction(Gadgets::StringToAction(str));
    else return false;
    return true;
}

void LocalView::TriggerAction(Gadgets::GadgetsAction a){
    Action(a)->trigger();
}

QAction *LocalView::Action(QString str, QVariant data){
    Q_UNUSED(data)
    if(Gadgets::IsValidAction(str))
        return Action(Gadgets::StringToAction(str));
    return 0;
}

QAction *LocalView::Action(Gadgets::GadgetsAction a){
    static const QList<Gadgets::GadgetsAction> exclude = QList<Gadgets::GadgetsAction>()
        << Gadgets::_NoAction << Gadgets::_End
        << Gadgets::_Up       << Gadgets::_PageUp
        << Gadgets::_Down     << Gadgets::_PageDown << Gadgets::_SelectAll
        << Gadgets::_Right                          << Gadgets::_SwitchWindow
        << Gadgets::_Left                           << Gadgets::_NextWindow
        << Gadgets::_Home                           << Gadgets::_PrevWindow
        << Gadgets::_ScrollUp          << Gadgets::_SelectToNextPage
        << Gadgets::_ScrollDown        << Gadgets::_SelectToFirstItem
        << Gadgets::_NextPage          << Gadgets::_SelectToLastItem
        << Gadgets::_PrevPage          << Gadgets::_SelectItem
        << Gadgets::_MoveToUpperItem   << Gadgets::_SelectRange
        << Gadgets::_MoveToLowerItem   << Gadgets::_SelectAll
        << Gadgets::_MoveToRightItem   << Gadgets::_ClearSelection
        << Gadgets::_MoveToLeftItem    << Gadgets::_TransferToUpper
        << Gadgets::_MoveToPrevPage    << Gadgets::_TransferToLower
        << Gadgets::_MoveToNextPage    << Gadgets::_TransferToRight
        << Gadgets::_MoveToFirstItem   << Gadgets::_TransferToLeft
        << Gadgets::_MoveToLastItem    << Gadgets::_TransferToPrevPage
        << Gadgets::_SelectToUpperItem << Gadgets::_TransferToNextPage
        << Gadgets::_SelectToLowerItem << Gadgets::_TransferToFirst
        << Gadgets::_SelectToRightItem << Gadgets::_TransferToLast
        << Gadgets::_SelectToLeftItem  << Gadgets::_SwitchNodeCollectionType
        << Gadgets::_SelectToPrevPage  << Gadgets::_SwitchNodeCollectionTypeReverse;
    static Gadgets::GadgetsAction previousAction = Gadgets::_NoAction;
    static int sameActionCount = 0;
    if(exclude.contains(a)){
        sameActionCount = 0;
        previousAction = Gadgets::_NoAction;
    } else if(a == previousAction){
        if(++sameActionCount > MAX_SAME_ACTION_COUNT)
            a = Gadgets::_NoAction;
    } else {
        sameActionCount = 0;
        previousAction = a;
    }

    QAction *action = m_ActionTable[a];

    if(action){
        switch(a){
        case Gadgets::_Up:
        case Gadgets::_Down:
        case Gadgets::_Right:
        case Gadgets::_Left:
        case Gadgets::_Import:
        case Gadgets::_Export:
        case Gadgets::_AboutVanilla:
        case Gadgets::_AboutQt:
        case Gadgets::_Quit:
        case Gadgets::_ToggleFullScreen:
        case Gadgets::_ToggleMaximized:
        case Gadgets::_ToggleMinimized:
        case Gadgets::_ToggleShaded:
        case Gadgets::_ShadeWindow:
        case Gadgets::_UnshadeWindow:
        case Gadgets::_NewWindow:
        case Gadgets::_CloseWindow:
        case Gadgets::_SwitchWindow:
        case Gadgets::_NextWindow:
        case Gadgets::_PrevWindow:
        case Gadgets::_Close:
        case Gadgets::_Restore:
        case Gadgets::_Recreate:
        case Gadgets::_NextView:
        case Gadgets::_PrevView:
        case Gadgets::_BuryView:
        case Gadgets::_DigView:
        case Gadgets::_FirstView:
        case Gadgets::_SecondView:
        case Gadgets::_ThirdView:
        case Gadgets::_FourthView:
        case Gadgets::_FifthView:
        case Gadgets::_SixthView:
        case Gadgets::_SeventhView:
        case Gadgets::_EighthView:
        case Gadgets::_NinthView:
        case Gadgets::_TenthView:
        case Gadgets::_LastView:
        case Gadgets::_NewViewNode:
        case Gadgets::_CloneViewNode:
        case Gadgets::_DisplayViewTree:
        case Gadgets::_DisplayAccessKey:
        case Gadgets::_DisplayTrashTree:
        case Gadgets::_OpenTextSeeker:
        case Gadgets::_OpenQueryEditor:
        case Gadgets::_OpenUrlEditor:
        case Gadgets::_OpenCommand:
        case Gadgets::_ReleaseHiddenView:
            delete action;
            m_ActionTable[a] = action = new QAction(this);
            break;
        case Gadgets::_ToggleNotifier: action->setChecked(GetTreeBank()->GetNotifier()); return action;
        case Gadgets::_ToggleReceiver: action->setChecked(GetTreeBank()->GetReceiver()); return action;
        case Gadgets::_ToggleMenuBar:  action->setChecked(!GetTreeBank()->GetMainWindow()->IsMenuBarEmpty()); return action;
        case Gadgets::_ToggleTreeBar:  action->setChecked(GetTreeBank()->GetMainWindow()->GetTreeBar()->isVisible()); return action;
        case Gadgets::_ToggleToolBar:  action->setChecked(GetTreeBank()->GetMainWindow()->GetToolBar()->isVisible()); return action;
        default:
            return action;
        }
    } else {
        m_ActionTable[a] = action = new QAction(this);
    }

    switch(a){
    case Gadgets::_Up:          action->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowUp));       break;
    case Gadgets::_Down:        action->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowDown));     break;
    case Gadgets::_Right:       action->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowRight));    break;
    case Gadgets::_Left:        action->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowLeft));     break;
    default: break;
    }

    switch(a){
    case Gadgets::_NoAction: break;

#define DEFINE_ACTION(name, text)                                       \
        case Gadgets::_##name:                                          \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name##Key()));                         \
            break;

        DEFINE_ACTION(Up,       tr("UpKey"));
        DEFINE_ACTION(Down,     tr("DownKey"));
        DEFINE_ACTION(Right,    tr("RightKey"));
        DEFINE_ACTION(Left,     tr("LeftKey"));
        DEFINE_ACTION(Home,     tr("HomeKey"));
        DEFINE_ACTION(End,      tr("EndKey"));
        DEFINE_ACTION(PageUp,   tr("PageUpKey"));
        DEFINE_ACTION(PageDown, tr("PageDownKey"));

#undef  DEFINE_ACTION
#define DEFINE_ACTION(name, text)                                       \
        case Gadgets::_##name:                                          \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action,        SIGNAL(triggered()),                 \
                    GetTreeBank(), SLOT(name()));                       \
            break;

        DEFINE_ACTION(Import,       tr("Import"));
        DEFINE_ACTION(Export,       tr("Export"));
        DEFINE_ACTION(AboutVanilla, tr("AboutVanilla"));
        DEFINE_ACTION(AboutQt,      tr("AboutQt"));
        DEFINE_ACTION(OpenSettings, tr("Settings"));
        DEFINE_ACTION(OpenDirectorySettings, tr("DirectorySettings"));
        DEFINE_ACTION(Quit,         tr("Quit"));

        DEFINE_ACTION(ClearCookies,      tr("ClearCookies"));
        DEFINE_ACTION(ClearHttpCache,    tr("ClearHttpCache"));
        DEFINE_ACTION(ClearVisitedLinks, tr("ClearVisitedLinks"));

        DEFINE_ACTION(ToggleNotifier,   tr("ToggleNotifier"));
        DEFINE_ACTION(ToggleReceiver,   tr("ToggleReceiver"));
        DEFINE_ACTION(ToggleMenuBar,    tr("ToggleMenuBar"));
        DEFINE_ACTION(ToggleTreeBar,    tr("ToggleTreeBar"));
        DEFINE_ACTION(ToggleToolBar,    tr("ToggleToolBar"));
        DEFINE_ACTION(ToggleFullScreen, tr("ToggleFullScreen"));
        DEFINE_ACTION(ToggleMaximized,  tr("ToggleMaximized"));
        DEFINE_ACTION(ToggleMinimized,  tr("ToggleMinimized"));
        DEFINE_ACTION(ToggleShaded,     tr("ToggleShaded"));
        DEFINE_ACTION(ShadeWindow,      tr("ShadeWindow"));
        DEFINE_ACTION(UnshadeWindow,    tr("UnshadeWindow"));
        DEFINE_ACTION(NewWindow,        tr("NewWindow"));
        DEFINE_ACTION(CloseWindow,      tr("CloseWindow"));
        DEFINE_ACTION(SwitchWindow,     tr("SwitchWindow"));
        DEFINE_ACTION(NextWindow,       tr("NextWindow"));
        DEFINE_ACTION(PrevWindow,       tr("PrevWindow"));

        DEFINE_ACTION(Close,              tr("Close"));
        DEFINE_ACTION(Restore,            tr("Restore"));
        DEFINE_ACTION(Recreate,           tr("Recreate"));
        DEFINE_ACTION(NextView,           tr("NextView"));
        DEFINE_ACTION(PrevView,           tr("PrevView"));
        DEFINE_ACTION(BuryView,           tr("BuryView"));
        DEFINE_ACTION(DigView,            tr("DigView"));
        DEFINE_ACTION(FirstView,          tr("FirstView"));
        DEFINE_ACTION(SecondView,         tr("SecondView"));
        DEFINE_ACTION(ThirdView,          tr("ThirdView"));
        DEFINE_ACTION(FourthView,         tr("FourthView"));
        DEFINE_ACTION(FifthView,          tr("FifthView"));
        DEFINE_ACTION(SixthView,          tr("SixthView"));
        DEFINE_ACTION(SeventhView,        tr("SeventhView"));
        DEFINE_ACTION(EighthView,         tr("EighthView"));
        DEFINE_ACTION(NinthView,          tr("NinthView"));
        DEFINE_ACTION(TenthView,          tr("TenthView"));
        DEFINE_ACTION(LastView,           tr("LastView"));
        DEFINE_ACTION(DisplayAccessKey,   tr("DisplayAccessKey"));
        DEFINE_ACTION(DisplayViewTree,    tr("DisplayViewTree"));
        DEFINE_ACTION(DisplayTrashTree,   tr("DisplayTrashTree"));
        DEFINE_ACTION(OpenTextSeeker,     tr("OpenTextSeeker"));
        DEFINE_ACTION(OpenQueryEditor,    tr("OpenQueryEditor"));
        DEFINE_ACTION(OpenUrlEditor,      tr("OpenUrlEditor"));
        DEFINE_ACTION(OpenCommand,        tr("OpenCommand"));
        DEFINE_ACTION(ReleaseHiddenView,  tr("ReleaseHiddenView"));

#undef  DEFINE_ACTION
#define DEFINE_ACTION(name, text)                                       \
        case Gadgets::_##name:                                          \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name()));                              \
            break;

        DEFINE_ACTION(Deactivate, tr("Deactivate"));

#undef  DEFINE_ACTION
#define DEFINE_ACTION(name, text)                                       \
        case Gadgets::_##name:                                          \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(ThumbList_##name()));                  \
            break;

        DEFINE_ACTION(Refresh,                         tr("Refresh"));
        DEFINE_ACTION(RefreshNoScroll,                 tr("RefreshNoScroll"));
        DEFINE_ACTION(OpenNode,                        tr("OpenNode"));
        DEFINE_ACTION(OpenNodeOnNewWindow,             tr("OpenNodeOnNewWindow"));
        DEFINE_ACTION(DeleteNode,                      tr("DeleteNode"));
        DEFINE_ACTION(DeleteRightNode,                 tr("DeleteRightNode"));
        DEFINE_ACTION(DeleteLeftNode,                  tr("DeleteLeftNode"));
        DEFINE_ACTION(DeleteOtherNode,                 tr("DeleteOtherNode"));
        DEFINE_ACTION(PasteNode,                       tr("PasteNode"));
        DEFINE_ACTION(NewNode,                         tr("NewNode"));
        DEFINE_ACTION(CloneNode,                       tr("CloneNode"));
        DEFINE_ACTION(UpDirectory,                     tr("UpDirectory"));
        DEFINE_ACTION(DownDirectory,                   tr("DownDirectory"));
        DEFINE_ACTION(MakeLocalNode,                   tr("MakeLocalNode"));
        DEFINE_ACTION(MakeDirectory,                   tr("MakeDirectory"));
        DEFINE_ACTION(MakeDirectoryWithSelectedNode,   tr("MakeDirectoryWithSelectedNode"));
        DEFINE_ACTION(MakeDirectoryWithSameDomainNode, tr("MakeDirectoryWithSameDomainNode"));
        DEFINE_ACTION(RenameNode,                      tr("RenameNode"));
        DEFINE_ACTION(CopyNodeUrl,                     tr("CopyNodeUrl"));
        DEFINE_ACTION(CopyNodeTitle,                   tr("CopyNodeTitle"));
        DEFINE_ACTION(CopyNodeAsLink,                  tr("CopyNodeAsLink"));
        DEFINE_ACTION(OpenNodeWithDefault,             tr("OpenNodeWithDefault"));
        DEFINE_ACTION(ToggleTrash,                     tr("ToggleTrash"));
        DEFINE_ACTION(ScrollUp,                        tr("ScrollUp"));
        DEFINE_ACTION(ScrollDown,                      tr("ScrollDown"));
        DEFINE_ACTION(NextPage,                        tr("NextPage"));
        DEFINE_ACTION(PrevPage,                        tr("PrevPage"));
        DEFINE_ACTION(ZoomIn,                          tr("ZoomIn"));
        DEFINE_ACTION(ZoomOut,                         tr("ZoomOut"));
        DEFINE_ACTION(MoveToUpperItem,                 tr("MoveToUpperItem"));
        DEFINE_ACTION(MoveToLowerItem,                 tr("MoveToLowerItem"));
        DEFINE_ACTION(MoveToRightItem,                 tr("MoveToRightItem"));
        DEFINE_ACTION(MoveToLeftItem,                  tr("MoveToLeftItem"));
        DEFINE_ACTION(MoveToPrevPage,                  tr("MoveToPrevPage"));
        DEFINE_ACTION(MoveToNextPage,                  tr("MoveToNextPage"));
        DEFINE_ACTION(MoveToFirstItem,                 tr("MoveToFirstItem"));
        DEFINE_ACTION(MoveToLastItem,                  tr("MoveToLastItem"));
        DEFINE_ACTION(SelectToUpperItem,               tr("SelectToUpperItem"));
        DEFINE_ACTION(SelectToLowerItem,               tr("SelectToLowerItem"));
        DEFINE_ACTION(SelectToRightItem,               tr("SelectToRightItem"));
        DEFINE_ACTION(SelectToLeftItem,                tr("SelectToLeftItem"));
        DEFINE_ACTION(SelectToPrevPage,                tr("SelectToPrevPage"));
        DEFINE_ACTION(SelectToNextPage,                tr("SelectToNextPage"));
        DEFINE_ACTION(SelectToFirstItem,               tr("SelectToFirstItem"));
        DEFINE_ACTION(SelectToLastItem,                tr("SelectToLastItem"));
        DEFINE_ACTION(SelectItem,                      tr("SelectItem"));
        DEFINE_ACTION(SelectRange,                     tr("SelectRange"));
        DEFINE_ACTION(SelectAll,                       tr("SelectAll"));
        DEFINE_ACTION(ClearSelection,                  tr("ClearSelection"));
        DEFINE_ACTION(TransferToUpper,                 tr("TransferToUpper"));
        DEFINE_ACTION(TransferToLower,                 tr("TransferToLower"));
        DEFINE_ACTION(TransferToRight,                 tr("TransferToRight"));
        DEFINE_ACTION(TransferToLeft,                  tr("TransferToLeft"));
        DEFINE_ACTION(TransferToPrevPage,              tr("TransferToPrevPage"));
        DEFINE_ACTION(TransferToNextPage,              tr("TransferToNextPage"));
        DEFINE_ACTION(TransferToFirst,                 tr("TransferToFirst"));
        DEFINE_ACTION(TransferToLast,                  tr("TransferToLast"));
        DEFINE_ACTION(TransferToUpDirectory,           tr("TransferToUpDirectory"));
        DEFINE_ACTION(TransferToDownDirectory,         tr("TransferToDownDirectory"));
        DEFINE_ACTION(SwitchNodeCollectionType,        tr("SwitchNodeCollectionType"));
        DEFINE_ACTION(SwitchNodeCollectionTypeReverse, tr("SwitchNodeCollectionTypeReverse"));

#undef  DEFINE_ACTION

    case Gadgets::_RestoreNode: break;

    default: break;
    }
    switch(a){

    case Gadgets::_ToggleNotifier:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetNotifier());
        action->setText(tr("Notifier"));
        action->setToolTip(tr("Notifier"));
        break;
    case Gadgets::_ToggleReceiver:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetReceiver());
        action->setText(tr("Receiver"));
        action->setToolTip(tr("Receiver"));
        break;
    case Gadgets::_ToggleMenuBar:
        action->setCheckable(true);
        action->setChecked(!GetTreeBank()->GetMainWindow()->IsMenuBarEmpty());
        action->setText(tr("MenuBar"));
        action->setToolTip(tr("MenuBar"));
        break;
    case Gadgets::_ToggleTreeBar:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetMainWindow()->GetTreeBar()->isVisible());
        action->setText(tr("TreeBar"));
        action->setToolTip(tr("TreeBar"));
        break;
    case Gadgets::_ToggleToolBar:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetMainWindow()->GetToolBar()->isVisible());
        action->setText(tr("ToolBar"));
        action->setToolTip(tr("ToolBar"));
        break;

    default: break;
    }
    return action;
}

PixmapItem *LocalView::GetPixmapItem(){
    return m_PixmapItem;
}

VideoItem *LocalView::GetVideoItem(){
    return m_VideoItem;
}

QMediaPlayer *LocalView::GetMediaPlayer(){
    return m_MediaPlayer;
}

QAudioOutput *LocalView::GetAudioOutput(){
    return m_AudioOutput;
}

void LocalView::ChangeMediaVolume(int diff){
    m_MediaVolume = qBound(0, m_MediaVolume + diff, 100);

    m_AudioOutput->setVolume(QtAudio::convertVolume(m_MediaVolume / 100.0,
                                                    QtAudio::LogarithmicVolumeScale,
                                                    QtAudio::LinearVolumeScale));

    Application::GlobalSettings()
        .setValue(QStringLiteral("localview/@MediaVolume"), m_MediaVolume);
}

bool LocalView::ThumbList_Refresh(){
    if(!IsDisplayingNode()) return false;

    Load(m_ParentNode->GetUrl());

    m_NodesRegister.clear();
    return true;
}

bool LocalView::ThumbList_RefreshNoScroll(){
    if(!IsDisplayingNode()) return false;

    int scroll = m_CurrentScroll;
    bool hovered = (m_HoveredItemIndex != -1);

    if(!hovered) m_HoveredItemIndex = scroll;

    Load(m_ParentNode->GetUrl());

    m_NodesRegister.clear();
    return true;
}

bool LocalView::ThumbList_OpenNode(){
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){

        if(ScrollToChangeDirectory() ||
           !ThumbList_DownDirectory()){

            OpenNode(nd);
            return true;
        }
    }
    return false;
}

bool LocalView::ThumbList_OpenNodeOnNewWindow(){
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        MainWindow *win = GetTreeBank()->NewWindow();
        if(win->GetTreeBank()->OpenInNewViewNode(nd->GetUrl(), true, GetViewNode()))
            return true;
    }
    return false;
}

bool LocalView::ThumbList_DeleteNode(){
    if(!IsDisplayingNode()) return false;

    if(!m_NodesRegister.isEmpty()){

        m_NodesRegister = QSet<Node*>(m_NodesRegister.begin(), m_NodesRegister.end()).values();
        DeleteNodes(m_NodesRegister);
        return true;

    } else if(Node *nd = GetHoveredNode()){

        DeleteNode(nd);
        return true;
    }
    return false;
}

bool LocalView::ThumbList_DeleteRightNode(){
    if(!IsDisplayingNode()) return false;

    if(m_HoveredItemIndex == -1 ||
       m_HoveredItemIndex >= m_DisplayThumbnails.length() - 1) return false;

    NodeList list;
    for(int i = m_HoveredItemIndex + 1; i < m_DisplayThumbnails.length(); i++)
        list << m_DisplayThumbnails[i]->GetNode();

    DeleteNodes(list);
    return true;
}

bool LocalView::ThumbList_DeleteLeftNode(){
    if(!IsDisplayingNode()) return false;

    if(m_HoveredItemIndex <= 0 ||
       m_HoveredItemIndex >= m_DisplayThumbnails.length()) return false;

    NodeList list;
    for(int i = 0; i < m_HoveredItemIndex; i++)
        list << m_DisplayThumbnails[i]->GetNode();

    DeleteNodes(list);
    return true;
}

bool LocalView::ThumbList_DeleteOtherNode(){
    if(!IsDisplayingNode()) return false;

    QSet<Node*> keep;

    if(!m_NodesRegister.isEmpty()){
        keep = QSet<Node*>(m_NodesRegister.begin(), m_NodesRegister.end());
    } else if(Node *hovered = GetHoveredNode()){
        keep << hovered;
    } else return false;

    NodeList list;
    for(int i = 0; i < m_DisplayThumbnails.length(); i++){
        Node *nd = m_DisplayThumbnails[i]->GetNode();
        if(!keep.contains(nd)) list << nd;
    }

    if(list.isEmpty()) return false;

    DeleteNodes(list);
    return true;
}

bool LocalView::ThumbList_PasteNode(){
    if(!IsDisplayingNode() || !m_ParentNode) return false;

    if(m_FileOperationRunning){
        emit statusBarMessage(tr("Another file operation is still running."));
        return false;
    }

    const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
    if(!mime || !mime->hasUrls()) return false;

    const QString directory = m_ParentNode->GetUrl().toLocalFile();
    if(directory.isEmpty()) return false;

    QStringList sources;
    foreach(const QUrl &url, mime->urls()){
        if(url.isLocalFile()) sources << url.toLocalFile();
    }

    if(sources.isEmpty()) return false;

    StartFileOperation
        (tr("Pasting..."),
         [directory, sources](){
             QStringList failed;
             foreach(const QString &source, sources){
                 const QString name = FileOperation::UniqueName
                     (directory, QFileInfo(source).fileName());
                 if(name.isEmpty()){
                     failed << source;
                     continue;
                 }
                 FileOperation::Copy
                     (source, directory + QStringLiteral("/") + name, &failed);
             }
             return failed;
         },
         tr("Could not paste."),
         tr("These could not be copied here:\n%1"));

    return true;
}

bool LocalView::ThumbList_RestoreNode(){
    return false;
}

bool LocalView::ThumbList_NewNode(){
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        NewNode(nd);
        Load(m_ParentNode->GetUrl());
        return true;
    }
    return false;
}

bool LocalView::ThumbList_CloneNode(){
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        CloneNode(nd);
        Load(m_ParentNode->GetUrl());
        return true;
    }
    return false;
}

bool LocalView::ThumbList_UpDirectory(){
    if(!IsDisplayingNode()) return false;

    m_HoveredItemIndex = -1;
    m_PrimaryItemIndex = -1;

    QUrl url = m_ParentNode->GetUrl();
    Load(url.resolved(QUrl(url.toString().endsWith(QStringLiteral("/")) ? QStringLiteral("../") : QStringLiteral("./"))));
    return true;
}

bool LocalView::ThumbList_DownDirectory(){
    if(!IsDisplayingNode()) return false;

    Node *nd = GetHoveredNode();
    if(!nd || !nd->IsDirectory()) return false;

    m_HoveredItemIndex = -1;
    m_PrimaryItemIndex = -1;

    Load(nd->GetUrl());
    return true;
}

bool LocalView::ThumbList_RenameNode(){
    return GraphicsTableView::ThumbList_RenameNode();
}

bool LocalView::ThumbList_CopyNodeUrl(){
    return GraphicsTableView::ThumbList_CopyNodeUrl();
}

bool LocalView::ThumbList_CopyNodeTitle(){
    return GraphicsTableView::ThumbList_CopyNodeTitle();
}

bool LocalView::ThumbList_CopyNodeAsLink(){
    return GraphicsTableView::ThumbList_CopyNodeAsLink();
}

bool LocalView::ThumbList_OpenNodeWithDefault(){
    return GraphicsTableView::ThumbList_OpenNodeWithDefault();
}

bool LocalView::ThumbList_OpenNodeWithCommand(QString name){
    return GraphicsTableView::ThumbList_OpenNodeWithCommand(name);
}

bool LocalView::ThumbList_MakeLocalNode(){
    return false;
}

bool LocalView::ThumbList_MakeDirectory(){
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        MakeDirectory(nd);

        Load(m_ParentNode->GetUrl());
        return true;
    }
    return false;
}

bool LocalView::ThumbList_MakeDirectoryWithSelectedNode(){
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        MakeDirectory(nd);

        Load(m_ParentNode->GetUrl());
        return true;
    }
    return false;
}

bool LocalView::ThumbList_MakeDirectoryWithSameDomainNode(){
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        MakeDirectory(nd);

        Load(m_ParentNode->GetUrl());
        return true;
    }
    return false;
}

bool LocalView::ThumbList_ToggleTrash(){
    return false;
}

bool LocalView::ThumbList_ApplyChildrenOrder(DisplayArea area, QPointF basepos){
    Q_UNUSED(area) Q_UNUSED(basepos)
    ThumbList_RefreshNoScroll();
    return false;
}

void LocalView::StartImageCollector(bool reverse){
    m_CollectingFuture = reverse
        ? QtConcurrent::run(&LocalView::LoadImageRequestReverse, this,
                            m_CurrentScroll)
        : QtConcurrent::run(&LocalView::LoadImageRequest, this,
                            m_CurrentScroll);
}

void LocalView::StopImageCollector(){
    if(m_CollectingFuture.isRunning())
        m_CollectingFuture.cancel();
}

void LocalView::RestartImageCollector(){
    StopImageCollector();
    StartImageCollector();
}

void LocalView::RaiseMaxCostIfNeed(){
    if (LocalNode::m_FileImageCache.maxCost()<(2 * m_CurrentThumbnailColumnCount * m_CurrentThumbnailLineCount))
        LocalNode::m_FileImageCache.setMaxCost(2 * m_CurrentThumbnailColumnCount * m_CurrentThumbnailLineCount);
}

bool LocalView::SelectMediaItem(int index, std::function<void()> defaultAction){
    bool result = false;
    TreeBank::AddToUpdateBox(GetThis().lock());

    if(!m_PixmapItem->pixmap().isNull() ||
       !m_MediaPlayer->source().isEmpty()
       ){
        int min = 0;
        int max = m_DisplayThumbnails.length() - 1;
        if(index < min) index = min;
        if(index > max) index = max;
        m_ScrollIndicator->setSelected(false);
        GraphicsTableView::SetScroll(index);
        SwapMediaItem(index);
        m_TargetScroll = index;
        result = true;
    } else {
        defaultAction();
    }
    return result;
}

bool LocalView::ThumbList_ScrollUp(){
    return SelectMediaItem(m_TargetScroll - 1,
                           [this](){ GraphicsTableView::ThumbList_ScrollUp();});
}

bool LocalView::ThumbList_ScrollDown(){
    return SelectMediaItem(m_TargetScroll + 1,
                           [this](){ GraphicsTableView::ThumbList_ScrollDown();});
}

bool LocalView::ThumbList_NextPage(){
    return ThumbList_MoveToNextPage();
}

bool LocalView::ThumbList_PrevPage(){
    return ThumbList_MoveToPrevPage();
}

bool LocalView::ThumbList_MoveToUpperItem(){
    return SelectMediaItem(m_TargetScroll - m_CurrentThumbnailColumnCount,
                           [this](){ GraphicsTableView::ThumbList_MoveToUpperItem();});
}

bool LocalView::ThumbList_MoveToLowerItem(){
    return SelectMediaItem(m_TargetScroll + m_CurrentThumbnailColumnCount,
                           [this](){ GraphicsTableView::ThumbList_MoveToLowerItem();});
}

bool LocalView::ThumbList_MoveToRightItem(){
    return SelectMediaItem(m_TargetScroll + 1,
                           [this](){ GraphicsTableView::ThumbList_MoveToRightItem();});
}

bool LocalView::ThumbList_MoveToLeftItem(){
    return SelectMediaItem(m_TargetScroll - 1,
                           [this](){ GraphicsTableView::ThumbList_MoveToLeftItem();});
}

bool LocalView::ThumbList_MoveToPrevPage(){
    return SelectMediaItem(m_TargetScroll - 1,
                           [this](){ GraphicsTableView::ThumbList_MoveToPrevPage();});
}

bool LocalView::ThumbList_MoveToNextPage(){
    return SelectMediaItem(m_TargetScroll + 1,
                           [this](){ GraphicsTableView::ThumbList_MoveToNextPage();});
}

bool LocalView::ThumbList_MoveToFirstItem(){
    return SelectMediaItem(0,
                           [this](){ GraphicsTableView::ThumbList_MoveToFirstItem();});
}

bool LocalView::ThumbList_MoveToLastItem(){
    return SelectMediaItem(m_DisplayThumbnails.length()-1,
                           [this](){ GraphicsTableView::ThumbList_MoveToLastItem();});
}

bool LocalView::ThumbList_SelectToUpperItem(){
    return GraphicsTableView::ThumbList_SelectToUpperItem();
}

bool LocalView::ThumbList_SelectToLowerItem(){
    return GraphicsTableView::ThumbList_SelectToLowerItem();
}

bool LocalView::ThumbList_SelectToRightItem(){
    return GraphicsTableView::ThumbList_SelectToRightItem();
}

bool LocalView::ThumbList_SelectToLeftItem(){
    return GraphicsTableView::ThumbList_SelectToLeftItem();
}

bool LocalView::ThumbList_SelectToPrevPage(){
    return GraphicsTableView::ThumbList_SelectToPrevPage();
}

bool LocalView::ThumbList_SelectToNextPage(){
    return GraphicsTableView::ThumbList_SelectToNextPage();
}

bool LocalView::ThumbList_SelectToFirstItem(){
    return GraphicsTableView::ThumbList_SelectToFirstItem();
}

bool LocalView::ThumbList_SelectToLastItem(){
    return GraphicsTableView::ThumbList_SelectToLastItem();
}

bool LocalView::ThumbList_SelectItem(){
    return GraphicsTableView::ThumbList_SelectItem();
}

bool LocalView::ThumbList_SelectRange(){
    return GraphicsTableView::ThumbList_SelectRange();
}

bool LocalView::ThumbList_SelectAll(){
    return GraphicsTableView::ThumbList_SelectAll();
}

bool LocalView::ThumbList_ClearSelection(){
    return GraphicsTableView::ThumbList_ClearSelection();
}

bool LocalView::ThumbList_TransferToUpper(){
    return false;
}

bool LocalView::ThumbList_TransferToLower(){
    return false;
}

bool LocalView::ThumbList_TransferToRight(){
    return false;
}

bool LocalView::ThumbList_TransferToLeft(){
    return false;
}

bool LocalView::ThumbList_TransferToPrevPage(){
    return false;
}

bool LocalView::ThumbList_TransferToNextPage(){
    return false;
}

bool LocalView::ThumbList_TransferToFirst(){
    return false;
}

bool LocalView::ThumbList_TransferToLast(){
    return false;
}

bool LocalView::ThumbList_TransferToUpDirectory(){
    return false;
}

bool LocalView::ThumbList_TransferToDownDirectory(){
    return false;
}

bool LocalView::ThumbList_ZoomIn(){
    bool result = GraphicsTableView::ThumbList_ZoomIn();
    GetViewNode()->SetZoom(m_CurrentThumbnailZoomFactor);
    return result;
}

bool LocalView::ThumbList_ZoomOut(){
    bool result = GraphicsTableView::ThumbList_ZoomOut();
    GetViewNode()->SetZoom(m_CurrentThumbnailZoomFactor);
    if(!result) return false;

    RaiseMaxCostIfNeed();
    RestartImageCollector();

    return result;
}

void LocalView::OpenNode(Node *nd){
    if(IsSupported(nd->GetUrl())){
        m_ScrollIndicator->setSelected(false);
        GraphicsTableView::SetScroll(m_HoveredItemIndex);
        SwapMediaItem(m_HoveredItemIndex);
    } else if(nd->IsDirectory()){
        GetTreeBank()->OpenInNewViewNode(nd->GetUrl(), true, m_ViewNode);
    } else {
        QDesktopServices::openUrl(nd->GetUrl());
    }
}

void LocalView::OpenNodes(NodeList list){
    for(int i = 0; i < list.length(); i++){
        Node *nd = list[i];
        if(nd->IsDirectory()){
            GetTreeBank()->OpenInNewViewNode(nd->GetUrl(), i == list.length()-1, m_ViewNode);
        } else {
            QDesktopServices::openUrl(nd->GetUrl());
        }
    }
}

void LocalView::DeleteNode(Node *nd){
    DeleteNodes(NodeList() << nd);
}

void LocalView::DeleteNodes(NodeList list){
    if(m_FileOperationRunning){
        emit statusBarMessage(tr("Another file operation is still running."));
        return;
    }

    QStringList paths;
    foreach(Node *nd, list){
        if(!nd || nd->IsRoot()) continue;
        const QString path = nd->GetUrl().toLocalFile();
        if(!path.isEmpty()) paths << path;
    }
    if(paths.isEmpty()) return;

    m_PendingPaths = paths;
    m_PendingDirectory = m_ParentNode->GetUrl().toLocalFile();
    m_FileOperationRunning = true;

    emit statusBarMessage(tr("Looking at what would be deleted..."));
    m_ReachWatcher.setFuture
        (QtConcurrent::run(&FileOperation::ReachOf, paths));
}

void LocalView::AskThenDelete(){
    const FileOperation::Reach reach = m_ReachWatcher.result();
    emit statusBarMessage(QString());

    if(reach.files == 0 && reach.directories == 0){
        EndFileOperation();
        return;
    }

    const QString question = reach.directories == 0
        ? tr("Are you sure you want to delete these?\n"
             "Files: %1\n"
             "In: %2")
          .arg(reach.files).arg(m_PendingDirectory)
        : tr("Are you sure you want to delete these?\n"
             "Directories: %1 (everything inside them is deleted as well)\n"
             "Files: %2\n"
             "In: %3")
          .arg(reach.directories).arg(reach.files).arg(m_PendingDirectory);

    ModalDialog *dialog = new ModalDialog();
    dialog->SetTitle(tr("Delete Files."));
    dialog->SetCaption(question);
    dialog->SetButtons(Dialog::Yes | Dialog::No);

    connect(this, &QObject::destroyed, dialog, &ModalDialog::Aborted);

    QPointer<LocalView> self(this);
    const bool ok = dialog->Execute() &&
        dialog->ClickedButton() == Dialog::Yes;
    if(!self) return;

    if(!ok){
        EndFileOperation();
        return;
    }

    if(!m_ParentNode ||
       m_ParentNode->GetUrl().toLocalFile() != m_PendingDirectory){
        EndFileOperation();
        ModelessDialog::Information
            (tr("Nothing was deleted."),
             tr("The listing moved to another directory "
                "while the question was open."), this);
        return;
    }

    const QStringList paths = m_PendingPaths;
    StartFileOperation
        (tr("Deleting..."),
         [paths](){
             QStringList failed;
             foreach(const QString &path, paths)
                 FileOperation::Remove(path, &failed);
             return failed;
         },
         tr("Could not delete."),
         tr("These could not be deleted:\n%1"));
}

void LocalView::StartFileOperation(const QString &status,
                                   std::function<QStringList()> work,
                                   const QString &failureTitle,
                                   const QString &failureCaption){
    m_FailureTitle = failureTitle;
    m_FailureCaption = failureCaption;
    m_FileOperationRunning = true;

    emit statusBarMessage(status);
    m_WorkWatcher.setFuture(QtConcurrent::run(work));
}

void LocalView::ReportFileOperation(){
    const QStringList failed = m_WorkWatcher.result();

    EndFileOperation();

    if(m_ParentNode) Load(m_ParentNode->GetUrl());

    if(failed.isEmpty()) return;

    static const int shown = 20;
    QStringList lines = failed.mid(0, shown);
    if(failed.length() > shown)
        lines << tr("...and %1 more.").arg(failed.length() - shown);

    ModelessDialog::Information
        (m_FailureTitle, m_FailureCaption.arg(lines.join(QStringLiteral("\n"))), this);
}

void LocalView::EndFileOperation(){
    m_FileOperationRunning = false;
    m_PendingPaths.clear();
    m_PendingDirectory = QString();
    emit statusBarMessage(QString());
}

void LocalView::NewNode(Node *ln){
    if(ln->IsRoot()) return;
    bool ok;
    QString parent = ln->GetParent()->GetUrl().toLocalFile();
    QString name = ModalDialog::GetText
        (tr("Input file name."),
         tr("Input new file name."),
         QString(), &ok);
    if(!ok) return;
    if(name.isEmpty()){
        ModelessDialog::Information
            (tr("Invalid file name."),
             tr("Cannot make file with such name."), this);
        return;
    }

    QFile file(parent + QStringLiteral("/") + name);
    file.open(QIODevice::WriteOnly);
    file.close();
}

void LocalView::CloneNode(Node *ln){
    if(ln->IsRoot()) return;
    bool ok;
    QString parent = ln->GetParent()->GetUrl().toLocalFile();
    QString base = ln->GetUrl().toLocalFile().split(QStringLiteral("/")).last();
    QString name = ModalDialog::GetText
        (tr("Input file name."),
         tr("Input clone file name."),
         base, &ok);
    if(!ok) return;
    if(name.isEmpty() || name == base){
        ModelessDialog::Information
            (tr("Invalid file name."),
             tr("Cannot make file with such name."), this);
        return;
    }

    QFile file(parent + QStringLiteral("/") + base);
    file.copy(parent + QStringLiteral("/") + name);
}

void LocalView::MakeDirectory(Node *ln){
    if(ln->IsRoot()) return;
    bool ok;
    QString parent = ln->GetParent()->GetUrl().toLocalFile();
    QString name = ModalDialog::GetText
        (tr("Input directory name."),
         tr("Input new directory name."),
         QString(), &ok);
    if(name.isEmpty()){
        ModelessDialog::Information
            (tr("Invalid directory name."),
             tr("Cannot make directory with such name."), this);
        return;
    }

    QDir(parent).mkdir(name);
}

void LocalView::LoadImageRequest(int scope){
    int breath = m_CurrentThumbnailColumnCount * m_CurrentThumbnailLineCount;
    int min = qMax(scope - static_cast<int>(breath*0.5), 0);
    int max = qMin(scope + static_cast<int>(breath*1.5), m_DisplayThumbnails.length());

    for(int i = min; i < max; i++){
        if(i < m_DisplayThumbnails.length()
           && m_DisplayThumbnails[i]->GetNode()->GetImage().isNull()){

            LoadImageToCache(m_DisplayThumbnails[i]->GetNode());
        }
    }
}

void LocalView::LoadImageRequestReverse(int scope){
    int breath = m_CurrentThumbnailColumnCount * m_CurrentThumbnailLineCount;
    int min = qMax(scope - static_cast<int>(breath*0.5), 0);
    int max = qMin(scope + static_cast<int>(breath*1.5), m_DisplayThumbnails.length());

    for(int i = max; i > min; i--){
        if(i < m_DisplayThumbnails.length()
           && m_DisplayThumbnails[i]->GetNode()->GetImage().isNull()){

            LoadImageToCache(m_DisplayThumbnails[i]->GetNode());
        }
    }
}

void LocalView::LoadImageToCache(Node *nd){
    if(nd->GetUrl().isEmpty() || !nd->GetImage().isNull() ||
       !m_ParentNode->ChildrenContains(nd))
        return;
    LoadImageToCache(nd->GetUrl().toLocalFile());
}

void LocalView::LoadImageToCache(const QString &path){

    if(LocalNode::m_FileImageCache.object(path)) return;

    if(LocalNode::m_FileImageCache.count()     > LocalNode::m_FileImageCache.maxCost() ||
       LocalNode::m_FileImageCache.totalCost() > LocalNode::m_FileImageCache.maxCost()){

        LocalNode::m_FileImageCache.setMaxCost(LocalNode::m_FileImageCache.totalCost());
    }
    if(LocalNode::m_FileImageCache.count()     <= LocalNode::m_FileImageCache.maxCost() &&
       LocalNode::m_FileImageCache.totalCost() <= LocalNode::m_FileImageCache.maxCost()){

        LocalNode::m_DiskAccessMutex.lock();

        if(IsSupportedImage(path)){
            QImage image = QImage(path);
            if(!image.isNull()){
                image = image.scaled(SAVING_THUMBNAIL_SIZE,
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
                LocalNode::m_FileImageCache.insert(path, new QImage(image));
                LocalNode::m_DiskAccessMutex.unlock();
                return;
            }
        }

        QSize size = QSize(ScaleByDevice(48), ScaleByDevice(48));
        QIcon icon = QFileIconProvider().icon(QFileInfo(path));
        if(!icon.isNull()){
            QImage *image = new QImage(icon.pixmap(size).toImage());
            LocalNode::m_FileImageCache.insert(path, image);
        } else {
        }
        LocalNode::m_DiskAccessMutex.unlock();
    }
}

void LocalView::StopPlayback(){
    m_MediaPlayer->stop();
    m_MediaPlayer->setSource(QUrl());
    m_VideoItem->setEnabled(false);
    m_VideoItem->hide();
}

void LocalView::ShowStillPixmap(const QPixmap &pixmap, bool fitToWindow){
    m_StillPixmap = pixmap;
    m_StillFitsWindow = fitToWindow;
    RelayoutStillPixmap();
    m_PixmapItem->setEnabled(true);
    m_PixmapItem->show();
    m_PixmapItem->setFocus();
}

void LocalView::RelayoutStillPixmap(){
    if(m_StillPixmap.isNull()) return;

    const QSize size = Size().toSize();

    if(size.width() <= 0 || size.height() <= 0){
        m_PixmapItem->setPixmap(m_StillPixmap);
        m_PixmapItem->setOffset(QPointF());
        return;
    }

    const bool shrinkOnly =
        !m_StillFitsWindow &&
        m_StillPixmap.width() <= size.width() &&
        m_StillPixmap.height() <= size.height();

    QPixmap scaled = shrinkOnly
        ? m_StillPixmap
        : m_StillPixmap.scaled(size,
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);
    QSizeF diff = (Size() - scaled.size())/2.0;
    m_PixmapItem->setPixmap(scaled);
    m_PixmapItem->setOffset(diff.width(), diff.height());
}

QPixmap LocalView::FileIconPixmap(const QString &path){
    QIcon icon = QFileIconProvider().icon(QFileInfo(path));
    if(icon.isNull()) return QPixmap();
    const int side = DeviceScale::Primary(256);
    return icon.pixmap(QSize(side, side));
}

void LocalView::SwapMediaItem(int index){
    if(m_ParentNode->HasNoChildren()) return;

    if(index == -1){
        m_StillPixmap = QPixmap();
        m_PixmapItem->setPixmap(QPixmap());
        m_PixmapItem->setOffset(QPointF());
        m_PixmapItem->setEnabled(false);
        m_PixmapItem->hide();

        StopPlayback();

        m_CurrentNode = m_ParentNode->GetFirstChild();
        QString path = m_ParentNode->GetUrl().toLocalFile();
        emit urlChanged(QUrl::fromLocalFile(path));
        emit titleChanged(path);
    } else {
        if(index >= m_DisplayThumbnails.length())
            index = m_DisplayThumbnails.length() - 1;

        m_TargetScroll = index;

        Node *nd = m_DisplayThumbnails[index]->GetNode();
        m_CurrentNode = nd;
        emit urlChanged(nd->GetUrl());
        emit titleChanged(nd->GetUrl().toLocalFile());

        QString path = nd->GetUrl().toLocalFile();

        switch(MediaType::KindOfPath(path)){

        case MediaType::Video: {
            if(!m_PixmapItem->pixmap().isNull()){
                m_StillPixmap = QPixmap();
                m_PixmapItem->setPixmap(QPixmap());
                m_PixmapItem->setOffset(QPointF());
                m_PixmapItem->setEnabled(false);
                m_PixmapItem->hide();
            }

            m_MediaPlayer->setSource(nd->GetUrl());
            if(m_AutoPlayMedia) m_MediaPlayer->play();
            m_VideoItem->setSize(Size());
            m_VideoItem->setOffset(QPointF());
            m_VideoItem->setEnabled(true);
            m_VideoItem->show();
            m_VideoItem->setFocus();
            break;
        }
        case MediaType::Audio: {
            m_VideoItem->setEnabled(false);
            m_VideoItem->hide();

            m_MediaPlayer->setSource(nd->GetUrl());
            if(m_AutoPlayMedia) m_MediaPlayer->play();

            QPixmap icon = FileIconPixmap(path);
            if(!icon.isNull()) ShowStillPixmap(icon, false);
            break;
        }
        case MediaType::Image: {
            if(!m_MediaPlayer->source().isEmpty()) StopPlayback();

            QPixmap pixmap = QPixmap(path);
            if(pixmap.isNull()) return;
            ShowStillPixmap(pixmap, true);
            break;
        }
        case MediaType::NotMedia: {
            if(!m_MediaPlayer->source().isEmpty()) StopPlayback();

            QPixmap icon = FileIconPixmap(path);
            if(icon.isNull()) return;
            ShowStillPixmap(icon, false);
            break;
        }
        }
    }
}

void LocalView::OnMediaMetaDataChanged(){
    if(m_MediaPlayer->source().isEmpty()) return;
    if(!m_CurrentNode || m_CurrentNode->IsDummy()) return;
    if(!IsSupportedAudio(m_CurrentNode->GetUrl())) return;

    QVariant art = m_MediaPlayer->metaData().value(QMediaMetaData::CoverArtImage);
    if(!art.isValid())
        art = m_MediaPlayer->metaData().value(QMediaMetaData::ThumbnailImage);
    if(!art.isValid()) return;

    QImage image = art.value<QImage>();
    if(image.isNull()) return;

    ShowStillPixmap(QPixmap::fromImage(image), true);
}

void LocalView::OnMediaError(){
    if(m_MediaPlayer->error() == QMediaPlayer::NoError) return;

    emit statusBarMessage(tr("Cannot play this file.") +
                          QStringLiteral(" ") + m_MediaPlayer->errorString());
    qWarning() << "cannot play" << m_MediaPlayer->source()
               << ":" << m_MediaPlayer->errorString();
}

void LocalView::OnSetViewNode(ViewNode*){}

void LocalView::OnSetThis(WeakView){}

void LocalView::OnSetMaster(WeakView){}

void LocalView::OnSetSlave(WeakView){}

void LocalView::OnSetJsObject(_View*){}

void LocalView::OnSetJsObject(_Vanilla*){}

void LocalView::OnLoadStarted(){
    View::OnLoadStarted();
}

void LocalView::OnLoadProgress(int progress){
    View::OnLoadProgress(progress);
}

void LocalView::OnLoadFinished(bool ok){
    View::OnLoadFinished(ok);
}

void LocalView::OnTitleChanged(const QString &title){
    ChangeNodeTitle(title);
}

void LocalView::OnUrlChanged(const QUrl &uri){
    ChangeNodeUrl(uri);
}

void LocalView::OnViewChanged(){
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void LocalView::OnScrollChanged(){
    RestartImageCollector();
    SaveScroll();
}

void LocalView::EmitScrollChanged(){
    emit ScrollChanged(GetScroll());
}

QPointF LocalView::GetScroll(){
    return QPointF(0.5,
                   static_cast<double>(m_CurrentScroll) /
                   static_cast<double>(m_DisplayThumbnails.length()));
}

void LocalView::SetScroll(QPointF pos){
    GraphicsTableView::SetScroll(pos);
}

bool LocalView::SaveScroll(){
    if(!GetViewNode()) return false;
    GetViewNode()->SetScrollX(0);
    GetViewNode()->SetScrollY(m_CurrentScroll);
    return true;
}

bool LocalView::RestoreScroll(){
    if(!GetViewNode()) return false;
    GraphicsTableView::SetScroll(GetViewNode()->GetScrollY());
    return true;
}

bool LocalView::SaveZoom(){
    if(!GetViewNode()) return false;
    GetViewNode()->SetZoom(m_CurrentThumbnailZoomFactor);
    return true;
}

bool LocalView::RestoreZoom(){
    if(!GetViewNode()) return false;

    m_CurrentThumbnailZoomFactor = GetViewNode()->GetZoom();
    RelocateContents();
    RelocateScrollBar();

    RaiseMaxCostIfNeed();
    RestartImageCollector();

    return true;
}

bool LocalView::SaveHistory(){ return false;}

bool LocalView::RestoreHistory(){ return false;}

void LocalView::Download(QString, QString){}

void LocalView::SeekText(const QString &text, View::FindFlags flags){
    Q_UNUSED(flags)
    if(!m_CurrentNode) return;
    CollectNodes(m_CurrentNode, text);
    ThumbList_MoveToFirstItem();
    RestartImageCollector();
}

void LocalView::KeyEvent(QString str){
    TriggerKeyEvent(str);
}

void LocalView::UpKey(){
    UpKeyEvent();
}

void LocalView::DownKey(){
    DownKeyEvent();
}

void LocalView::RightKey(){
    RightKeyEvent();
}

void LocalView::LeftKey(){
    LeftKeyEvent();
}

void LocalView::HomeKey(){
    HomeKeyEvent();
}

void LocalView::EndKey(){
    EndKeyEvent();
}

void LocalView::PageUpKey(){
    PageUpKeyEvent();
}

void LocalView::PageDownKey(){
    PageDownKeyEvent();
}

void LocalView::keyPressEvent(QKeyEvent *ev){
    if(Application::HasAnyModifier(ev) ||
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    QGraphicsObject::keyPressEvent(ev);

    if(!ev->isAccepted() &&
       !Application::IsOnlyModifier(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void LocalView::keyReleaseEvent(QKeyEvent *ev){
    Q_UNUSED(ev)
}

void LocalView::dragEnterEvent(QGraphicsSceneDragDropEvent *ev){
    GraphicsTableView::dragEnterEvent(ev);
}

void LocalView::dropEvent(QGraphicsSceneDragDropEvent *ev){
    GraphicsTableView::dropEvent(ev);
}

void LocalView::dragMoveEvent(QGraphicsSceneDragDropEvent *ev){
    GraphicsTableView::dragMoveEvent(ev);
}

void LocalView::dragLeaveEvent(QGraphicsSceneDragDropEvent *ev){
    GraphicsTableView::dragLeaveEvent(ev);
}

void LocalView::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    Application::SetCurrentWindow(GetTreeBank()->GetMainWindow());

    if(m_EnableRightGestureLocal &&
       ev->buttons() & Qt::RightButton &&
       !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos().toPoint());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_RightGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_RightGestureMap[gesture])
              ? Action(Page::StringToAction(m_RightGestureMap[gesture]))->text()
            : m_RightGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
        ev->setAccepted(true);
    } else {
        GraphicsTableView::mouseMoveEvent(ev);
    }
}

void LocalView::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    if(!m_PixmapItem->pixmap().isNull() ||
       !m_MediaPlayer->source().isEmpty())
    {
        SwapMediaItem(-1);
    }

    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(Gadgets::GetMouseMap().contains(mouse)){

        QString str = Gadgets::GetMouseMap()[mouse];
        if(!str.isEmpty()){

            if(!TriggerAction(str, ev->pos().toPoint())){
                ev->setAccepted(false);
                return;
            }
            GestureAborted();
            ev->setAccepted(true);
            return;
        }
    }

    GestureStarted(ev->pos().toPoint());
    GraphicsTableView::mousePressEvent(ev);
    ev->setAccepted(true);
}

void LocalView::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    emit statusBarMessage(QString());
    if(!m_Gesture.isEmpty()){

        GestureFinished(ev->pos().toPoint(), ev->button());
        ev->setAccepted(true);

    } else {
        GraphicsTableView::mouseReleaseEvent(ev);
    }
    m_GestureStartedPos = QPoint();
}

void LocalView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev){
    GraphicsTableView::mouseDoubleClickEvent(ev);
}

void LocalView::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    GraphicsTableView::hoverEnterEvent(ev);
}

void LocalView::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    GraphicsTableView::hoverLeaveEvent(ev);
}

void LocalView::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    GraphicsTableView::hoverMoveEvent(ev);
}

void LocalView::contextMenuEvent(QGraphicsSceneContextMenuEvent *ev){
    GraphicsTableView::contextMenuEvent(ev);
}

void LocalView::wheelEvent(QGraphicsSceneWheelEvent *ev){
    if(!IsDisplayingNode()) return;

    bool up = ev->delta() > 0;
    bool ignoreStatusBarMessage = true;

    if(!m_PixmapItem->pixmap().isNull() ||
       !m_MediaPlayer->source().isEmpty()
       ){

        if(up) ThumbList_ScrollUp();
        else   ThumbList_ScrollDown();
        ev->setAccepted(true);
        return;
    }

    if(GetTreeBank()->GetView()->MouseEventSource() != Qt::MouseEventSynthesizedBySystem){
        QString wheel;

        Application::AddModifiersToString(wheel, ev->modifiers());
        Application::AddMouseButtonsToString(wheel, ev->buttons());
        Application::AddWheelDirectionToString(wheel, up);

        if(Gadgets::GetMouseMap().contains(wheel)){

            QString str = Gadgets::GetMouseMap()[wheel];
            if(!str.isEmpty()){

                GestureAborted();

                if(!TriggerAction(str, ev->pos().toPoint())){
                    ev->setAccepted(false);
                    return;
                }
            }
            UpdateInPlaceNotifier(ev->pos(), ev->scenePos(), ignoreStatusBarMessage);
            ev->setAccepted(true);
            return;

        } else if(ScrollToChangeDirectory() &&
                  ThumbnailAreaRect().contains(ev->pos())){

            if(up) ThumbList_UpDirectory();
            else   ThumbList_DownDirectory();

            UpdateInPlaceNotifier(ev->pos(), ev->scenePos(), ignoreStatusBarMessage);
            ev->setAccepted(true);
            return;
        }
    }
    ignoreStatusBarMessage = false;
    Scroll(-ev->delta() * m_CurrentThumbnailColumnCount / 120.0);

    UpdateInPlaceNotifier(ev->pos(), ev->scenePos(), ignoreStatusBarMessage);
    ev->setAccepted(true);
}

void LocalView::focusInEvent(QFocusEvent *ev){
    GraphicsTableView::focusInEvent(ev);
    OnFocusIn();
}

void LocalView::focusOutEvent(QFocusEvent *ev){
    GraphicsTableView::focusOutEvent(ev);
    OnFocusOut();
}

void LocalView::KeyPressEvent(QKeyEvent *ev){
    keyPressEvent(ev);
}

void LocalView::KeyReleaseEvent(QKeyEvent *ev){
    keyReleaseEvent(ev);
}

void LocalView::MousePressEvent(QMouseEvent *ev){
    GetTreeBank()->MousePressEvent(ev);
}

void LocalView::MouseReleaseEvent(QMouseEvent *ev){
    GetTreeBank()->MouseReleaseEvent(ev);
}

void LocalView::MouseMoveEvent(QMouseEvent *ev){
    GetTreeBank()->MouseMoveEvent(ev);
}

void LocalView::MouseDoubleClickEvent(QMouseEvent *ev){
    GetTreeBank()->MouseDoubleClickEvent(ev);
}

void LocalView::WheelEvent(QWheelEvent *ev){
    GetTreeBank()->WheelEvent(ev);
}

PixmapItem::PixmapItem(LocalView *parent)
    : QGraphicsPixmapItem(parent)
{
    m_LocalView = parent;
    setFlag(QGraphicsItem::ItemIsFocusable);
}

PixmapItem::~PixmapItem(){}

void PixmapItem::keyPressEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(!seq.isEmpty()){

        if(!m_LocalView->GetMediaPlayer()->source().isEmpty() &&
           m_LocalView->GetVideoItem()->HandleMediaKey(ev)){

        } else if(Application::HasAnyModifier(ev)){
            m_LocalView->TriggerKeyEvent(ev);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Up ||
                  ev->key() == Qt::Key_Left ||
                  ev->key() == Qt::Key_PageUp ||
                  ev->key() == Qt::Key_Backtab ||
                  ev->key() == Qt::Key_Backspace ||
                  (ev->key() == Qt::Key_Space &&
                   Application::HasShiftModifier(ev))){

            m_LocalView->ThumbList_ScrollUp();

        } else if(ev->key() == Qt::Key_Down ||
                  ev->key() == Qt::Key_Right ||
                  ev->key() == Qt::Key_PageDown ||
                  ev->key() == Qt::Key_Tab ||
                  ev->key() == Qt::Key_Space){

            m_LocalView->ThumbList_ScrollDown();

        } else if(ev->key() == Qt::Key_Home){

            m_LocalView->ThumbList_MoveToFirstItem();

        } else if(ev->key() == Qt::Key_End){

            m_LocalView->ThumbList_MoveToLastItem();

        } else {
            QGraphicsPixmapItem::keyPressEvent(ev);

            if(!ev->isAccepted() &&
               !Application::IsOnlyModifier(ev) &&
               Application::HasNoModifier(ev)){

                m_LocalView->TriggerKeyEvent(ev);
                ev->setAccepted(true);
            }
        }
    }
}

void PixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    m_LocalView->SwapMediaItem(-1);
    ev->setAccepted(true);
}

void PixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsPixmapItem::mouseReleaseEvent(ev);
}

void PixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsPixmapItem::mouseMoveEvent(ev);
}

VideoItem::VideoItem(LocalView *parent)
    : QGraphicsVideoItem(parent)
{
    m_LocalView = parent;
    setFlag(QGraphicsItem::ItemIsFocusable);
}

VideoItem::~VideoItem(){}

void VideoItem::Play(){
    emit statusBarMessage(tr("play"));
    m_LocalView->GetMediaPlayer()->play();
}

void VideoItem::Pause(){
    emit statusBarMessage(tr("pause"));
    m_LocalView->GetMediaPlayer()->pause();
}

void VideoItem::Stop(){
    emit statusBarMessage(tr("stop"));
    m_LocalView->GetMediaPlayer()->stop();
}

void VideoItem::VolumeUp(){
    emit statusBarMessage(tr("volume up"));
    m_LocalView->ChangeMediaVolume(10);
}

void VideoItem::VolumeDown(){
    emit statusBarMessage(tr("volume down"));
    m_LocalView->ChangeMediaVolume(-10);
}

void VideoItem::SetPositionRelative(qint64 diff){
    QMediaPlayer *player = m_LocalView->GetMediaPlayer();
    player->setPosition(player->position() + diff);
}

bool VideoItem::HandleMediaKey(QKeyEvent *ev){

    if(ev->key() == Qt::Key_Up){

        VolumeUp();

    } else if(ev->key() == Qt::Key_Down){

        VolumeDown();

    } else if(ev->key() == Qt::Key_Left &&
              Application::HasShiftModifier(ev) &&
              Application::HasCtrlModifier(ev)){

        emit statusBarMessage(tr("10 minutes back"));
        SetPositionRelative(-600000);

    } else if(ev->key() == Qt::Key_Right &&
              Application::HasShiftModifier(ev) &&
              Application::HasCtrlModifier(ev)){

        emit statusBarMessage(tr("10 minutes forward"));
        SetPositionRelative(600000);

    } else if(ev->key() == Qt::Key_Left &&
              Application::HasShiftModifier(ev)){

        emit statusBarMessage(tr("5 minutes back"));
        SetPositionRelative(-300000);

    } else if(ev->key() == Qt::Key_Right &&
              Application::HasShiftModifier(ev)){

        emit statusBarMessage(tr("5 minutes forward"));
        SetPositionRelative(300000);

    } else if(ev->key() == Qt::Key_Left &&
              Application::HasCtrlModifier(ev)){

        emit statusBarMessage(tr("1 minute back"));
        SetPositionRelative(-60000);

    } else if(ev->key() == Qt::Key_Right &&
              Application::HasCtrlModifier(ev)){

        emit statusBarMessage(tr("1 minute forward"));
        SetPositionRelative(60000);

    } else if(ev->key() == Qt::Key_Left){

        emit statusBarMessage(tr("10 seconds back"));
        SetPositionRelative(-10000);

    } else if(ev->key() == Qt::Key_Right){

        emit statusBarMessage(tr("10 seconds forward"));
        SetPositionRelative(10000);

    } else if(ev->key() == Qt::Key_Space){

        if(m_LocalView->GetMediaPlayer()->playbackState()
           == QMediaPlayer::PlayingState)
            Pause();
        else
            Play();

    } else {
        return false;
    }

    ev->setAccepted(true);
    return true;
}

void VideoItem::keyPressEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(!seq.isEmpty()){

        if(HandleMediaKey(ev)){

        } else if(ev->key() == Qt::Key_PageDown){

            m_LocalView->ThumbList_MoveToRightItem();
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_PageUp){

            m_LocalView->ThumbList_MoveToLeftItem();
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Home){

            m_LocalView->ThumbList_MoveToFirstItem();
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_End){

            m_LocalView->ThumbList_MoveToLastItem();
            ev->setAccepted(true);

        } else if(Application::HasAnyModifier(ev)){

            m_LocalView->TriggerKeyEvent(ev);
            ev->setAccepted(true);

        } else {
            QGraphicsVideoItem::keyPressEvent(ev);

            if(!ev->isAccepted() &&
               !Application::IsOnlyModifier(ev) &&
               Application::HasNoModifier(ev)){

                m_LocalView->TriggerKeyEvent(ev);
                ev->setAccepted(true);
            }
        }
    }
}

void VideoItem::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    m_LocalView->SwapMediaItem(-1);
    ev->setAccepted(true);
}

void VideoItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsVideoItem::mouseReleaseEvent(ev);
}

void VideoItem::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsVideoItem::mouseMoveEvent(ev);
}

#endif
