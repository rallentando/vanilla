#include "const.hpp"
#include "switch.hpp"

#include "localview.hpp"

#include <QNetworkRequest>
#include <QDesktopServices>
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QImageReader>
#include <QModelIndex>
#include <QIcon>
#include <QTimer>
#include <QMediaPlayer>
#include <QGraphicsVideoItem>
#include <QtConcurrent/QtConcurrent>
#include <QStyle>

#include "notifier.hpp"
#include "receiver.hpp"
#include "thumbnail.hpp"
#include "nodetitle.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "gadgetsstyle.hpp"
#ifdef QTWEBKIT
#  include "webview.hpp"
#  include "quickwebview.hpp"
#endif
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "dialog.hpp"
#if defined(Q_OS_WIN)
#  include "tridentview.hpp"
#endif

LocalView::LocalView(TreeBank *parent, QString id, QStringList set)
    : View(parent)
    , GraphicsTableView(parent)
{
    Initialize();
    setZValue(COVERING_VIEW_CONTENTS_LAYER);

    m_ParentNode = new LocalNode();

    class DummyLocalNode : public LocalNode{
        bool IsDummy() const DECL_OVERRIDE { return true;}
    };

    m_DummyLocalNode = new DummyLocalNode();
    m_DummyLocalNode->SetParent(m_ParentNode);

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
    // 'LocalNode::SetUrl' sets m_Title.
    m_ParentNode->SetUrl(directory);
    // don't call 'LocalNode::SetTitle' unless renaming.
    //m_ParentNode->SetTitle(path);

    RegisterNodes(directory);

    // 'm_CurrentNode' is displayed pixmap or media.
    // 'CollectNodes' sets 'm_CurrentNode'.
    // 'm_(Hist|View)Node' url is pixmap url or media url or current directory.
    //
    // when not displaying pixmap or media, 'm_CurrentNode' is dummy,
    // and 'm_(Hist|View)Node' url is directory.

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

    StartImageCollector(false);
}

void LocalView::Load(const QNetworkRequest &req){
    Load(req.url());
}

void LocalView::Resize(QSizeF size){
    GraphicsTableView::Resize(size);
    Node *nd = m_CurrentNode;
    if(!nd || nd->IsDummy()) return;

    if(!m_PixmapItem->pixmap().isNull() &&
       !nd->GetImage().isNull()){
        QPixmap pixmap =
            QPixmap::fromImage(nd->GetImage())
            .scaled(Size().toSize(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation);
        QSizeF diff = (Size() - pixmap.size())/2.0;
        m_PixmapItem->setPixmap(pixmap);
        m_PixmapItem->setOffset(diff.width(), diff.height());
        m_PixmapItem->setFocus();
    } else if(m_MediaPlayer->media().isNull() &&
              IsSupportedVideo(nd->GetUrl())){
        m_VideoItem->setSize(Size());
        m_VideoItem->setOffset(QPointF());
        m_VideoItem->setFocus();
    }
}

QMenu *LocalView::CreateNodeMenu(){
    if(!IsDisplayingNode()) return 0;

    QMenu *menu = new QMenu(GetTreeBank());
    menu->setToolTipsVisible(true);

    menu->addAction(Action(Gadgets::Ge_Deactivate));
    menu->addAction(Action(Gadgets::Ge_Refresh));
    menu->addSeparator();

    if(m_HoveredItemIndex != -1 &&
       m_HoveredItemIndex < m_DisplayThumbnails.length()){

        menu->addAction(Action(Gadgets::Ge_OpenNode));
        menu->addAction(Action(Gadgets::Ge_OpenNodeOnNewWindow));
        menu->addAction(Action(Gadgets::Ge_RenameNode));
        if(GetHoveredNode()->IsDirectory())
            menu->addAction(Action(Gadgets::Ge_DownDirectory));
    }
    if(m_ParentNode &&
       m_ParentNode->GetUrl().toString() != QStringLiteral("file:///")){

        menu->addAction(Action(Gadgets::Ge_UpDirectory));
    }
    if((m_HoveredItemIndex != -1 &&
        m_HoveredItemIndex < m_DisplayThumbnails.length()) ||
       !m_NodesRegister.isEmpty()){

        menu->addAction(Action(Gadgets::Ge_DeleteNode));
        menu->addAction(Action(Gadgets::Ge_DeleteRightNode));
        menu->addAction(Action(Gadgets::Ge_DeleteLeftNode));
        menu->addAction(Action(Gadgets::Ge_DeleteOtherNode));
    }
    return menu;
}

void LocalView::RenderBackground(QPainter *painter){
    if(!GetTreeBank()) return;

    if(GetStyle()->StyleName() != QStringLiteral("FlatStyle")
#if defined(Q_OS_WIN)
       && !TreeBank::TridentViewExist()
#endif
       ){
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
#ifdef QTWEBKIT
        if(WebView *w = qobject_cast<WebView*>(GetTreeBank()->GetCurrentView()->base()))
            view = w;
        else if(QuickWebView *w = qobject_cast<QuickWebView*>(GetTreeBank()->GetCurrentView()->base()))
            view = w;
        else
#endif
        if(WebEngineView *w = qobject_cast<WebEngineView*>(GetTreeBank()->GetCurrentView()->base()))
            view = w;
        else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(GetTreeBank()->GetCurrentView()->base()))
            view = w;
#if defined(Q_OS_WIN)
        else if(TridentView *w = qobject_cast<TridentView*>(GetTreeBank()->GetCurrentView()->base()))
            view = w;
#endif
    }

    if(view){
        if(!view->visible())
            // there is an unblockable gap(when difference of window size exists).
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
    QString path = url.toLocalFile();
    return IsSupportedImage(path) || IsSupportedVideo(path);
}

bool LocalView::IsSupported(QString path){
    return IsSupportedImage(path) || IsSupportedVideo(path);
}

bool LocalView::IsSupportedImage(QUrl url){
    QString path = url.toLocalFile();
    return IsSupportedImage(path);
}

bool LocalView::IsSupportedImage(QString path){
    if(path.endsWith(QStringLiteral("#"))) return false;
    QImageReader reader(path);
    return QImageReader::supportedImageFormats().contains(reader.format());
}

bool LocalView::IsSupportedVideo(QUrl url){
    QString path = url.toLocalFile();
    return IsSupportedVideo(path);
}

bool LocalView::IsSupportedVideo(QString path){
    return path.endsWith(QStringLiteral(".mpeg"))
        || path.endsWith(QStringLiteral(".mpg"))
        || path.endsWith(QStringLiteral(".avi"))
        || path.endsWith(QStringLiteral(".wmv"))
        || path.endsWith(QStringLiteral(".flv"))
        || path.endsWith(QStringLiteral(".asf"))
        || path.endsWith(QStringLiteral(".mp4"))

        || path.endsWith(QStringLiteral(".mp3"))
        || path.endsWith(QStringLiteral(".wav"))
        || path.endsWith(QStringLiteral(".ogg"))
        || path.endsWith(QStringLiteral(".wma"));
}

void LocalView::RegisterNodes(const QUrl &url){
    QString path = url.toLocalFile();

    // for windows.
#ifdef Q_OS_WIN
    if(url.toString() == QStringLiteral("file:///")){
        for(char c = 'C'; c <= 'Z'; c++){
            QString rootpath = QString(c) + QStringLiteral(":/");
            QDir rootdir = QDir(rootpath);
            if(rootdir.exists() && rootdir.isReadable()){
                LocalNode *nd = new LocalNode();
                // 'LocalNode::SetUrl' sets m_Title.
                nd->SetUrl(QUrl::fromLocalFile(rootpath));
                // don't call 'LocalNode::SetTitle' unless renaming.
                //nd->SetTitle(rootpath);
                nd->SetParent(m_ParentNode);
                m_ParentNode->AppendChild(nd);
            }
        }
    } else
#endif
    {
        QDir dir = QDir(path);
        QStringList list = dir.entryList(QDir::NoDotAndDotDot|QDir::AllEntries);

        if(LocalNode::m_FileImageCache.maxCost() < list.length())
            LocalNode::m_FileImageCache.setMaxCost(list.length());

        for(int i = 0; i < list.length(); i++){
            QString file = list[i];
            LocalNode *nd = new LocalNode();
            QString filepath = path.endsWith(QStringLiteral("/")) ?
                path + file : path + QStringLiteral("/") + file;
            // 'LocalNode::SetUrl' sets m_Title.
            nd->SetUrl(QUrl::fromLocalFile(filepath));
            // don't call 'LocalNode::SetTitle' unless renaming.
            //nd->SetTitle(file);
            nd->SetParent(m_ParentNode);
            m_ParentNode->AppendChild(nd);
        }
    }
}

void LocalView::Activate(DisplayType type){
    Q_UNUSED(type);
    show();
}

void LocalView::Deactivate(){
    // when using as 'View' object,
    // not wanted to hide, if background is nothing.
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
    // instead of 'Activate' in 'Gadgets'.
    QGraphicsObject::show();
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::ArrowCursor);

    // reset scroll.
    m_HoveredItemIndex = -1;
    m_PrimaryItemIndex = -1;
    if(!m_PixmapItem->pixmap().isNull()){
        m_PixmapItem->setEnabled(true);
        m_PixmapItem->show();
        m_PixmapItem->setFocus();
    } else if(!m_MediaPlayer->media().isNull()){
        m_VideoItem->setEnabled(true);
        m_VideoItem->show();
        m_VideoItem->setFocus();
    }
    if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
    if(HistNode *hn = GetHistNode()) hn->SetLastAccessDateToCurrent();

    RestoreViewState();
    // set only notifier.
    if(!GetTreeBank() || !GetTreeBank()->GetNotifier()) return;
    GetTreeBank()->GetNotifier()->SetScroll(GetScroll());
}

void LocalView::hide(){
    SaveViewState();

    // instead of 'Deactivate' in 'Gadgets'.

    // needless to reset visibility of child items,
    // because 'LocalView' doesn't use 'AccessKey'.

    QGraphicsObject::hide();
    m_NodesRegister.clear();

    // needless to call 'AfterDisplayGadgets' and 'SetCurrent',
    // when this is used as 'View'
}

void LocalView::ClearCache(){
    LocalNode::m_FileImageCache.clear();
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

            connect(receiver, SIGNAL(Deactivate()),                      this, SLOT(Deactivate()));
            connect(receiver, SIGNAL(Refresh()),                         this, SLOT(ThumbList_Refresh()));
            connect(receiver, SIGNAL(RefreshNoScroll()),                 this, SLOT(ThumbList_RefreshNoScroll()));
            connect(receiver, SIGNAL(OpenNode()),                        this, SLOT(ThumbList_OpenNode()));
            connect(receiver, SIGNAL(OpenNodeOnNewWindow()),             this, SLOT(ThumbList_OpenNodeOnNewWindow()));
            connect(receiver, SIGNAL(DeleteNode()),                      this, SLOT(ThumbList_DeleteNode()));
            connect(receiver, SIGNAL(DeleteRightNode()),                 this, SLOT(ThumbList_DeleteRightNode()));
            connect(receiver, SIGNAL(DeleteLeftNode()),                  this, SLOT(ThumbList_DeleteLeftNode()));
            connect(receiver, SIGNAL(DeleteOtherNode()),                 this, SLOT(ThumbList_DeleteOtherNode()));
            connect(receiver, SIGNAL(PasteNode()),                       this, SLOT(ThumbList_PasteNode()));
            connect(receiver, SIGNAL(RestoreNode()),                     this, SLOT(ThumbList_RestoreNode()));
            connect(receiver, SIGNAL(NewNode()),                         this, SLOT(ThumbList_NewNode()));
            connect(receiver, SIGNAL(CloneNode()),                       this, SLOT(ThumbList_CloneNode()));
            connect(receiver, SIGNAL(UpDirectory()),                     this, SLOT(ThumbList_UpDirectory()));
            connect(receiver, SIGNAL(DownDirectory()),                   this, SLOT(ThumbList_DownDirectory()));
            connect(receiver, SIGNAL(MakeLocalNode()),                   this, SLOT(ThumbList_MakeLocalNode()));
            connect(receiver, SIGNAL(MakeDirectory()),                   this, SLOT(ThumbList_MakeDirectory()));
            connect(receiver, SIGNAL(MakeDirectoryWithSelectedNode()),   this, SLOT(ThumbList_MakeDirectoryWithSelectedNode()));
            connect(receiver, SIGNAL(MakeDirectoryWithSameDomainNode()), this, SLOT(ThumbList_MakeDirectoryWithSameDomainNode()));
            connect(receiver, SIGNAL(RenameNode()),                      this, SLOT(ThumbList_RenameNode()));
            connect(receiver, SIGNAL(CopyNodeUrl()),                     this, SLOT(ThumbList_CopyNodeUrl()));
            connect(receiver, SIGNAL(CopyNodeTitle()),                   this, SLOT(ThumbList_CopyNodeTitle()));
            connect(receiver, SIGNAL(CopyNodeAsLink()),                  this, SLOT(ThumbList_CopyNodeAsLink()));
            connect(receiver, SIGNAL(OpenNodeWithIE()),                  this, SLOT(ThumbList_OpenNodeWithIE()));
            connect(receiver, SIGNAL(OpenNodeWithEdge()),                this, SLOT(ThumbList_OpenNodeWithEdge()));
            connect(receiver, SIGNAL(OpenNodeWithFF()),                  this, SLOT(ThumbList_OpenNodeWithFF()));
            connect(receiver, SIGNAL(OpenNodeWithOpera()),               this, SLOT(ThumbList_OpenNodeWithOpera()));
            connect(receiver, SIGNAL(OpenNodeWithOPR()),                 this, SLOT(ThumbList_OpenNodeWithOPR()));
            connect(receiver, SIGNAL(OpenNodeWithSafari()),              this, SLOT(ThumbList_OpenNodeWithSafari()));
            connect(receiver, SIGNAL(OpenNodeWithChrome()),              this, SLOT(ThumbList_OpenNodeWithChrome()));
            connect(receiver, SIGNAL(OpenNodeWithSleipnir()),            this, SLOT(ThumbList_OpenNodeWithSleipnir()));
            connect(receiver, SIGNAL(OpenNodeWithVivaldi()),             this, SLOT(ThumbList_OpenNodeWithVivaldi()));
            connect(receiver, SIGNAL(OpenNodeWithCustom()),              this, SLOT(ThumbList_OpenNodeWithCustom()));
            connect(receiver, SIGNAL(ToggleTrash()),                     this, SLOT(ThumbList_ToggleTrash()));
            connect(receiver, SIGNAL(ScrollUp()),                        this, SLOT(ThumbList_ScrollUp()));
            connect(receiver, SIGNAL(ScrollDown()),                      this, SLOT(ThumbList_ScrollDown()));
          //connect(receiver, SIGNAL(PageUp()),                          this, SLOT(ThumbList_PageUp()));
          //connect(receiver, SIGNAL(PageDown()),                        this, SLOT(ThumbList_PageDown()));
          //connect(receiver, SIGNAL(ZoomIn()),                          this, SLOT(ThumbList_ZoomIn()));
          //connect(receiver, SIGNAL(ZoomOut()),                         this, SLOT(ThumbList_ZoomOut()));
            connect(receiver, SIGNAL(MoveToUpperItem()),                 this, SLOT(ThumbList_MoveToUpperItem()));
            connect(receiver, SIGNAL(MoveToLowerItem()),                 this, SLOT(ThumbList_MoveToLowerItem()));
            connect(receiver, SIGNAL(MoveToRightItem()),                 this, SLOT(ThumbList_MoveToRightItem()));
            connect(receiver, SIGNAL(MoveToLeftItem()),                  this, SLOT(ThumbList_MoveToLeftItem()));
            connect(receiver, SIGNAL(MoveToPrevPage()),                  this, SLOT(ThumbList_MoveToPrevPage()));
            connect(receiver, SIGNAL(MoveToNextPage()),                  this, SLOT(ThumbList_MoveToNextPage()));
            connect(receiver, SIGNAL(MoveToFirstItem()),                 this, SLOT(ThumbList_MoveToFirstItem()));
            connect(receiver, SIGNAL(MoveToLastItem()),                  this, SLOT(ThumbList_MoveToLastItem()));
            connect(receiver, SIGNAL(SelectToUpperItem()),               this, SLOT(ThumbList_SelectToUpperItem()));
            connect(receiver, SIGNAL(SelectToLowerItem()),               this, SLOT(ThumbList_SelectToLowerItem()));
            connect(receiver, SIGNAL(SelectToRightItem()),               this, SLOT(ThumbList_SelectToRightItem()));
            connect(receiver, SIGNAL(SelectToLeftItem()),                this, SLOT(ThumbList_SelectToLeftItem()));
            connect(receiver, SIGNAL(SelectToPrevPage()),                this, SLOT(ThumbList_SelectToPrevPage()));
            connect(receiver, SIGNAL(SelectToNextPage()),                this, SLOT(ThumbList_SelectToNextPage()));
            connect(receiver, SIGNAL(SelectToFirstItem()),               this, SLOT(ThumbList_SelectToFirstItem()));
            connect(receiver, SIGNAL(SelectToLastItem()),                this, SLOT(ThumbList_SelectToLastItem()));
            connect(receiver, SIGNAL(SelectItem()),                      this, SLOT(ThumbList_SelectItem()));
            connect(receiver, SIGNAL(SelectRange()),                     this, SLOT(ThumbList_SelectRange()));
          //connect(receiver, SIGNAL(SelectAll()),                       this, SLOT(ThumbList_SelectAll()));
            connect(receiver, SIGNAL(ClearSelection()),                  this, SLOT(ThumbList_ClearSelection()));
          //connect(receiver, SIGNAL(TransferToUpper()),                 this, SLOT(ThumbList_TransferToUpper()));
          //connect(receiver, SIGNAL(TransferToLower()),                 this, SLOT(ThumbList_TransferToLower()));
          //connect(receiver, SIGNAL(TransferToRight()),                 this, SLOT(ThumbList_TransferToRight()));
          //connect(receiver, SIGNAL(TransferToLeft()),                  this, SLOT(ThumbList_TransferToLeft()));
          //connect(receiver, SIGNAL(TransferToPrevPage()),              this, SLOT(ThumbList_TransferToPrevPage()));
          //connect(receiver, SIGNAL(TransferToNextPage()),              this, SLOT(ThumbList_TransferToNextPage()));
          //connect(receiver, SIGNAL(TransferToFirst()),                 this, SLOT(ThumbList_TransferToFirst()));
          //connect(receiver, SIGNAL(TransferToLast()),                  this, SLOT(ThumbList_TransferToLast()));
          //connect(receiver, SIGNAL(TransferToUpDirectory()),           this, SLOT(ThumbList_TransferToUpDirectory()));
          //connect(receiver, SIGNAL(TransferToDownDirectory()),         this, SLOT(ThumbList_TransferToDownDirectory()));
            connect(receiver, SIGNAL(SwitchNodeCollectionType()),        this, SLOT(ThumbList_SwitchNodeCollectionType()));
            connect(receiver, SIGNAL(SwitchNodeCollectionTypeReverse()), this, SLOT(ThumbList_SwitchNodeCollectionTypeReverse()));

          //connect(receiver, SIGNAL(Up()),    this, SLOT(ThumbList_MoveToUpperItem()));
          //connect(receiver, SIGNAL(Down()),  this, SLOT(ThumbList_MoveToLowerItem()));
          //connect(receiver, SIGNAL(Right()), this, SLOT(ThumbList_MoveToRightItem()));
          //connect(receiver, SIGNAL(Left()),  this, SLOT(ThumbList_MoveToLeftItem()));
          //connect(receiver, SIGNAL(Home()),  this, SLOT(ThumbList_MoveToFirstItem()));
          //connect(receiver, SIGNAL(End()),   this, SLOT(ThumbList_MoveToLastItem()));
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

            disconnect(receiver, SIGNAL(Deactivate()),                      this, SLOT(Deactivate()));
            disconnect(receiver, SIGNAL(Refresh()),                         this, SLOT(ThumbList_Refresh()));
            disconnect(receiver, SIGNAL(RefreshNoScroll()),                 this, SLOT(ThumbList_RefreshNoScroll()));
            disconnect(receiver, SIGNAL(OpenNode()),                        this, SLOT(ThumbList_OpenNode()));
            disconnect(receiver, SIGNAL(OpenNodeOnNewWindow()),             this, SLOT(ThumbList_OpenNodeOnNewWindow()));
            disconnect(receiver, SIGNAL(DeleteNode()),                      this, SLOT(ThumbList_DeleteNode()));
            disconnect(receiver, SIGNAL(DeleteRightNode()),                 this, SLOT(ThumbList_DeleteRightNode()));
            disconnect(receiver, SIGNAL(DeleteLeftNode()),                  this, SLOT(ThumbList_DeleteLeftNode()));
            disconnect(receiver, SIGNAL(DeleteOtherNode()),                 this, SLOT(ThumbList_DeleteOtherNode()));
            disconnect(receiver, SIGNAL(PasteNode()),                       this, SLOT(ThumbList_PasteNode()));
            disconnect(receiver, SIGNAL(RestoreNode()),                     this, SLOT(ThumbList_RestoreNode()));
            disconnect(receiver, SIGNAL(NewNode()),                         this, SLOT(ThumbList_NewNode()));
            disconnect(receiver, SIGNAL(CloneNode()),                       this, SLOT(ThumbList_CloneNode()));
            disconnect(receiver, SIGNAL(UpDirectory()),                     this, SLOT(ThumbList_UpDirectory()));
            disconnect(receiver, SIGNAL(DownDirectory()),                   this, SLOT(ThumbList_DownDirectory()));
            disconnect(receiver, SIGNAL(MakeLocalNode()),                   this, SLOT(ThumbList_MakeLocalNode()));
            disconnect(receiver, SIGNAL(MakeDirectory()),                   this, SLOT(ThumbList_MakeDirectory()));
            disconnect(receiver, SIGNAL(MakeDirectoryWithSelectedNode()),   this, SLOT(ThumbList_MakeDirectoryWithSelectedNode()));
            disconnect(receiver, SIGNAL(MakeDirectoryWithSameDomainNode()), this, SLOT(ThumbList_MakeDirectoryWithSameDomainNode()));
            disconnect(receiver, SIGNAL(RenameNode()),                      this, SLOT(ThumbList_RenameNode()));
            disconnect(receiver, SIGNAL(CopyNodeUrl()),                     this, SLOT(ThumbList_CopyNodeUrl()));
            disconnect(receiver, SIGNAL(CopyNodeTitle()),                   this, SLOT(ThumbList_CopyNodeTitle()));
            disconnect(receiver, SIGNAL(CopyNodeAsLink()),                  this, SLOT(ThumbList_CopyNodeAsLink()));
            disconnect(receiver, SIGNAL(OpenNodeWithIE()),                  this, SLOT(ThumbList_OpenNodeWithIE()));
            disconnect(receiver, SIGNAL(OpenNodeWithEdge()),                this, SLOT(ThumbList_OpenNodeWithEdge()));
            disconnect(receiver, SIGNAL(OpenNodeWithFF()),                  this, SLOT(ThumbList_OpenNodeWithFF()));
            disconnect(receiver, SIGNAL(OpenNodeWithOpera()),               this, SLOT(ThumbList_OpenNodeWithOpera()));
            disconnect(receiver, SIGNAL(OpenNodeWithOPR()),                 this, SLOT(ThumbList_OpenNodeWithOPR()));
            disconnect(receiver, SIGNAL(OpenNodeWithSafari()),              this, SLOT(ThumbList_OpenNodeWithSafari()));
            disconnect(receiver, SIGNAL(OpenNodeWithChrome()),              this, SLOT(ThumbList_OpenNodeWithChrome()));
            disconnect(receiver, SIGNAL(OpenNodeWithSleipnir()),            this, SLOT(ThumbList_OpenNodeWithSleipnir()));
            disconnect(receiver, SIGNAL(OpenNodeWithVivaldi()),             this, SLOT(ThumbList_OpenNodeWithVivaldi()));
            disconnect(receiver, SIGNAL(OpenNodeWithCustom()),              this, SLOT(ThumbList_OpenNodeWithCustom()));
            disconnect(receiver, SIGNAL(ToggleTrash()),                     this, SLOT(ThumbList_ToggleTrash()));
            disconnect(receiver, SIGNAL(ScrollUp()),                        this, SLOT(ThumbList_ScrollUp()));
            disconnect(receiver, SIGNAL(ScrollDown()),                      this, SLOT(ThumbList_ScrollDown()));
          //disconnect(receiver, SIGNAL(PageUp()),                          this, SLOT(ThumbList_PageUp()));
          //disconnect(receiver, SIGNAL(PageDown()),                        this, SLOT(ThumbList_PageDown()));
          //disconnect(receiver, SIGNAL(ZoomIn()),                          this, SLOT(ThumbList_ZoomIn()));
          //disconnect(receiver, SIGNAL(ZoomOut()),                         this, SLOT(ThumbList_ZoomOut()));
            disconnect(receiver, SIGNAL(MoveToUpperItem()),                 this, SLOT(ThumbList_MoveToUpperItem()));
            disconnect(receiver, SIGNAL(MoveToLowerItem()),                 this, SLOT(ThumbList_MoveToLowerItem()));
            disconnect(receiver, SIGNAL(MoveToRightItem()),                 this, SLOT(ThumbList_MoveToRightItem()));
            disconnect(receiver, SIGNAL(MoveToLeftItem()),                  this, SLOT(ThumbList_MoveToLeftItem()));
            disconnect(receiver, SIGNAL(MoveToPrevPage()),                  this, SLOT(ThumbList_MoveToPrevPage()));
            disconnect(receiver, SIGNAL(MoveToNextPage()),                  this, SLOT(ThumbList_MoveToNextPage()));
            disconnect(receiver, SIGNAL(MoveToFirstItem()),                 this, SLOT(ThumbList_MoveToFirstItem()));
            disconnect(receiver, SIGNAL(MoveToLastItem()),                  this, SLOT(ThumbList_MoveToLastItem()));
            disconnect(receiver, SIGNAL(SelectToUpperItem()),               this, SLOT(ThumbList_SelectToUpperItem()));
            disconnect(receiver, SIGNAL(SelectToLowerItem()),               this, SLOT(ThumbList_SelectToLowerItem()));
            disconnect(receiver, SIGNAL(SelectToRightItem()),               this, SLOT(ThumbList_SelectToRightItem()));
            disconnect(receiver, SIGNAL(SelectToLeftItem()),                this, SLOT(ThumbList_SelectToLeftItem()));
            disconnect(receiver, SIGNAL(SelectToPrevPage()),                this, SLOT(ThumbList_SelectToPrevPage()));
            disconnect(receiver, SIGNAL(SelectToNextPage()),                this, SLOT(ThumbList_SelectToNextPage()));
            disconnect(receiver, SIGNAL(SelectToFirstItem()),               this, SLOT(ThumbList_SelectToFirstItem()));
            disconnect(receiver, SIGNAL(SelectToLastItem()),                this, SLOT(ThumbList_SelectToLastItem()));
            disconnect(receiver, SIGNAL(SelectItem()),                      this, SLOT(ThumbList_SelectItem()));
            disconnect(receiver, SIGNAL(SelectRange()),                     this, SLOT(ThumbList_SelectRange()));
          //disconnect(receiver, SIGNAL(SelectAll()),                       this, SLOT(ThumbList_SelectAll()));
            disconnect(receiver, SIGNAL(ClearSelection()),                  this, SLOT(ThumbList_ClearSelection()));
          //disconnect(receiver, SIGNAL(TransferToUpper()),                 this, SLOT(ThumbList_TransferToUpper()));
          //disconnect(receiver, SIGNAL(TransferToLower()),                 this, SLOT(ThumbList_TransferToLower()));
          //disconnect(receiver, SIGNAL(TransferToRight()),                 this, SLOT(ThumbList_TransferToRight()));
          //disconnect(receiver, SIGNAL(TransferToLeft()),                  this, SLOT(ThumbList_TransferToLeft()));
          //disconnect(receiver, SIGNAL(TransferToPrevPage()),              this, SLOT(ThumbList_TransferToPrevPage()));
          //disconnect(receiver, SIGNAL(TransferToNextPage()),              this, SLOT(ThumbList_TransferToNextPage()));
          //disconnect(receiver, SIGNAL(TransferToFirst()),                 this, SLOT(ThumbList_TransferToFirst()));
          //disconnect(receiver, SIGNAL(TransferToLast()),                  this, SLOT(ThumbList_TransferToLast()));
          //disconnect(receiver, SIGNAL(TransferToUpDirectory()),           this, SLOT(ThumbList_TransferToUpDirectory()));
          //disconnect(receiver, SIGNAL(TransferToDownDirectory()),         this, SLOT(ThumbList_TransferToDownDirectory()));
            disconnect(receiver, SIGNAL(SwitchNodeCollectionType()),        this, SLOT(ThumbList_SwitchNodeCollectionType()));
            disconnect(receiver, SIGNAL(SwitchNodeCollectionTypeReverse()), this, SLOT(ThumbList_SwitchNodeCollectionTypeReverse()));

          //disconnect(receiver, SIGNAL(Up()),    this, SLOT(ThumbList_MoveToUpperItem()));
          //disconnect(receiver, SIGNAL(Down()),  this, SLOT(ThumbList_MoveToLowerItem()));
          //disconnect(receiver, SIGNAL(Right()), this, SLOT(ThumbList_MoveToRightItem()));
          //disconnect(receiver, SIGNAL(Left()),  this, SLOT(ThumbList_MoveToLeftItem()));
          //disconnect(receiver, SIGNAL(Home()),  this, SLOT(ThumbList_MoveToFirstItem()));
          //disconnect(receiver, SIGNAL(End()),   this, SLOT(ThumbList_MoveToLastItem()));
        }
    }

    if(m_VideoItem){
        disconnect(m_VideoItem, SIGNAL(statusBarMessage(const QString&)),
                   this, SIGNAL(statusBarMessage(const QString&)));
    }
}

void LocalView::UpdateThumbnail(){
    if(GetHistNode()){
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
                // investigation in progress of segv.
                // i don't want to write this.
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
            GetHistNode()->SetImage(
                image.
                scaled(SAVING_THUMBNAIL_SIZE,
                       Qt::KeepAspectRatioByExpanding,
                       Qt::SmoothTransformation));
        } else {
            GetHistNode()->SetImage(
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
    Q_UNUSED(data);
    if(Gadgets::IsValidAction(str))
        TriggerAction(Gadgets::StringToAction(str));
    else return false;
    return true;
}

void LocalView::TriggerAction(Gadgets::GadgetsAction a){
    Action(a)->trigger();
}

QAction *LocalView::Action(QString str, QVariant data){
    Q_UNUSED(data);
    if(Gadgets::IsValidAction(str))
        return Action(Gadgets::StringToAction(str));
    return 0;
}

QAction *LocalView::Action(Gadgets::GadgetsAction a){
    // forbid many times call of same action.
    static const QList<Gadgets::GadgetsAction> exclude = QList<Gadgets::GadgetsAction>()
        << Gadgets::Ge_NoAction << Gadgets::Ke_End
        << Gadgets::Ke_Up       << Gadgets::Ke_PageUp
        << Gadgets::Ke_Down     << Gadgets::Ke_PageDown << Gadgets::Ge_SelectAll
        << Gadgets::Ke_Right                            << Gadgets::Ge_SwitchWindow
        << Gadgets::Ke_Left                             << Gadgets::Ge_NextWindow
        << Gadgets::Ke_Home                             << Gadgets::Ge_PrevWindow
        << Gadgets::Ge_MoveToUpperItem   << Gadgets::Ge_SelectToLastItem
        << Gadgets::Ge_MoveToLowerItem   << Gadgets::Ge_SelectItem
        << Gadgets::Ge_MoveToRightItem   << Gadgets::Ge_SelectRange
        << Gadgets::Ge_MoveToLeftItem    << Gadgets::Ge_SelectAll
        << Gadgets::Ge_MoveToPrevPage    << Gadgets::Ge_ClearSelection
        << Gadgets::Ge_MoveToNextPage    << Gadgets::Ge_TransferToUpper
        << Gadgets::Ge_MoveToFirstItem   << Gadgets::Ge_TransferToLower
        << Gadgets::Ge_MoveToLastItem    << Gadgets::Ge_TransferToRight
        << Gadgets::Ge_SelectToUpperItem << Gadgets::Ge_TransferToLeft
        << Gadgets::Ge_SelectToLowerItem << Gadgets::Ge_TransferToPrevPage
        << Gadgets::Ge_SelectToRightItem << Gadgets::Ge_TransferToNextPage
        << Gadgets::Ge_SelectToLeftItem  << Gadgets::Ge_TransferToFirst
        << Gadgets::Ge_SelectToPrevPage  << Gadgets::Ge_TransferToLast
        << Gadgets::Ge_SelectToNextPage  << Gadgets::Ge_SwitchNodeCollectionType
        << Gadgets::Ge_SelectToFirstItem << Gadgets::Ge_SwitchNodeCollectionTypeReverse;

    static Gadgets::GadgetsAction previousAction = Gadgets::Ge_NoAction;
    static int sameActionCount = 0;
    if(exclude.contains(a)){
        sameActionCount = 0;
        previousAction = Gadgets::Ge_NoAction;
    } else if(a == previousAction){
        if(++sameActionCount > MAX_SAME_ACTION_COUNT)
            a = Gadgets::Ge_NoAction;
    } else {
        sameActionCount = 0;
        previousAction = a;
    }

    QAction *action = m_ActionTable[a];

    if(action){
        // those action connect to treebank.
        // relation of treebank and view is not one-to-one correspondence.
        // so recreate action.
        switch(a){
        case Gadgets::Ge_Import:
        case Gadgets::Ge_Export:
        case Gadgets::Ge_AboutVanilla:
        case Gadgets::Ge_AboutQt:
        case Gadgets::Ge_Quit:
        //case Gadgets::Ge_ToggleNotifier:
        //case Gadgets::Ge_ToggleReceiver:
        //case Gadgets::Ge_ToggleMenuBar:
        //case Gadgets::Ge_ToggleTreeBar:
        //case Gadgets::Ge_ToggleToolBar:
        case Gadgets::Ge_ToggleFullScreen:
        case Gadgets::Ge_ToggleMaximized:
        case Gadgets::Ge_ToggleMinimized:
        case Gadgets::Ge_ToggleShaded:
        case Gadgets::Ge_ShadeWindow:
        case Gadgets::Ge_UnshadeWindow:
        case Gadgets::Ge_NewWindow:
        case Gadgets::Ge_CloseWindow:
        case Gadgets::Ge_SwitchWindow:
        case Gadgets::Ge_NextWindow:
        case Gadgets::Ge_PrevWindow:
        case Gadgets::Ge_Close:
        case Gadgets::Ge_Restore:
        case Gadgets::Ge_Recreate:
        case Gadgets::Ge_NextView:
        case Gadgets::Ge_PrevView:
        case Gadgets::Ge_BuryView:
        case Gadgets::Ge_DigView:
        case Gadgets::Ge_NewViewNode:
        case Gadgets::Ge_NewHistNode:
        case Gadgets::Ge_CloneViewNode:
        case Gadgets::Ge_CloneHistNode:
        case Gadgets::Ge_DisplayViewTree:
        case Gadgets::Ge_DisplayHistTree:
        case Gadgets::Ge_DisplayAccessKey:
        case Gadgets::Ge_DisplayTrashTree:
        case Gadgets::Ge_OpenTextSeeker:
        case Gadgets::Ge_OpenQueryEditor:
        case Gadgets::Ge_OpenUrlEditor:
        case Gadgets::Ge_OpenCommand:
        case Gadgets::Ge_ReleaseHiddenView:
            delete action;
            m_ActionTable[a] = action = new QAction(this);
            break;
        case Gadgets::Ge_ToggleNotifier:
            action->setChecked(GetTreeBank()->GetNotifier());
            return action;
        case Gadgets::Ge_ToggleReceiver:
            action->setChecked(GetTreeBank()->GetReceiver());
            return action;
        case Gadgets::Ge_ToggleMenuBar:
            action->setChecked(!GetTreeBank()->GetMainWindow()->IsMenuBarEmpty());
            return action;
        case Gadgets::Ge_ToggleTreeBar:
            action->setChecked(GetTreeBank()->GetMainWindow()->GetTreeBar()->isVisible());
            return action;
        case Gadgets::Ge_ToggleToolBar:
            action->setChecked(GetTreeBank()->GetMainWindow()->GetToolBar()->isVisible());
            return action;
        default:
            return action;
        }
    } else {
        m_ActionTable[a] = action = new QAction(this);
    }

    switch(a){
    case Gadgets::Ke_Up:      action->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowUp));       break;
    case Gadgets::Ke_Down:    action->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowDown));     break;
    case Gadgets::Ke_Right:   action->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowRight));    break;
    case Gadgets::Ke_Left:    action->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowLeft));     break;
  //case Gadgets::Ge_Back:    action->setIcon(QIcon(":/resources/menu/back.png"));    break;
  //case Gadgets::Ge_Forward: action->setIcon(QIcon(":/resources/menu/forward.png")); break;
  //case Gadgets::Ge_Reload:  action->setIcon(QIcon(":/resources/menu/reload.png"));  break;
  //case Gadgets::Ge_Stop:    action->setIcon(QIcon(":/resources/menu/stop.png"));    break;
    }

    switch(a){
    case Gadgets::Ge_NoAction: break;

#define DEFINE_ACTION(name, text)                                       \
        case Gadgets::Ke_##name:                                        \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name##Key()));                         \
            break;

        // key events.
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
        case Gadgets::Ge_##name:                                        \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action,        SIGNAL(triggered()),                 \
                    GetTreeBank(), SLOT(name()));                       \
            break;

        // application events.
        DEFINE_ACTION(Import,       tr("Import"));
        DEFINE_ACTION(Export,       tr("Export"));
        DEFINE_ACTION(AboutVanilla, tr("AboutVanilla"));
        DEFINE_ACTION(AboutQt,      tr("AboutQt"));
        DEFINE_ACTION(Quit,         tr("Quit"));

        // window events.
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

        // treebank events.
      //DEFINE_ACTION(Back,               tr("Back"));
      //DEFINE_ACTION(Forward,            tr("Forward"));
      //DEFINE_ACTION(UpDirectory,        tr("UpDirectory"));
        DEFINE_ACTION(Close,              tr("Close"));
        DEFINE_ACTION(Restore,            tr("Restore"));
        DEFINE_ACTION(Recreate,           tr("Recreate"));
        DEFINE_ACTION(NextView,           tr("NextView"));
        DEFINE_ACTION(PrevView,           tr("PrevView"));
        DEFINE_ACTION(BuryView,           tr("BuryView"));
        DEFINE_ACTION(DigView,            tr("DigView"));
      //DEFINE_ACTION(NewViewNode,        tr("NewViewNode"));
      //DEFINE_ACTION(NewHistNode,        tr("NewHistNode"));
      //DEFINE_ACTION(CloneViewNode,      tr("CloneViewNode"));
      //DEFINE_ACTION(CloneHistNode,      tr("CloneHistNode"));
        DEFINE_ACTION(DisplayAccessKey,   tr("DisplayAccessKey"));
        DEFINE_ACTION(DisplayViewTree,    tr("DisplayViewTree"));
        DEFINE_ACTION(DisplayHistTree,    tr("DisplayHistTree"));
        DEFINE_ACTION(DisplayTrashTree,   tr("DisplayTrashTree"));
        DEFINE_ACTION(OpenTextSeeker,     tr("OpenTextSeeker"));
        DEFINE_ACTION(OpenQueryEditor,    tr("OpenQueryEditor"));
        DEFINE_ACTION(OpenUrlEditor,      tr("OpenUrlEditor"));
        DEFINE_ACTION(OpenCommand,        tr("OpenCommand"));
        DEFINE_ACTION(ReleaseHiddenView,  tr("ReleaseHiddenView"));
      //DEFINE_ACTION(Load,               tr("Load"));

#undef  DEFINE_ACTION
#define DEFINE_ACTION(name, text)                                       \
        case Gadgets::Ge_##name:                                        \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name()));                              \
            break;

        // gadgets events.
        DEFINE_ACTION(Deactivate, tr("Deactivate"));

#undef  DEFINE_ACTION
#define DEFINE_ACTION(name, text)                                       \
        case Gadgets::Ge_##name:                                        \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(ThumbList_##name()));                  \
            break;

        // thumblist events.
        DEFINE_ACTION(Refresh,                         tr("Refresh"));
        DEFINE_ACTION(RefreshNoScroll,                 tr("RefreshNoScroll"));
        DEFINE_ACTION(OpenNode,                        tr("OpenNode"));
        DEFINE_ACTION(OpenNodeOnNewWindow,             tr("OpenNodeOnNewWindow"));
        DEFINE_ACTION(DeleteNode,                      tr("DeleteNode"));
        DEFINE_ACTION(DeleteRightNode,                 tr("DeleteRightNode"));
        DEFINE_ACTION(DeleteLeftNode,                  tr("DeleteLeftNode"));
        DEFINE_ACTION(DeleteOtherNode,                 tr("DeleteOtherNode"));
        DEFINE_ACTION(PasteNode,                       tr("PasteNode"));
        DEFINE_ACTION(RestoreNode,                     tr("RestoreNode"));
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
        DEFINE_ACTION(OpenNodeWithIE,                  tr("OpenNodeWithIE"));
        DEFINE_ACTION(OpenNodeWithEdge,                tr("OpenNodeWithEdge"));
        DEFINE_ACTION(OpenNodeWithFF,                  tr("OpenNodeWithFF"));
        DEFINE_ACTION(OpenNodeWithOpera,               tr("OpenNodeWithOpera"));
        DEFINE_ACTION(OpenNodeWithOPR,                 tr("OpenNodeWithOPR"));
        DEFINE_ACTION(OpenNodeWithSafari,              tr("OpenNodeWithSafari"));
        DEFINE_ACTION(OpenNodeWithChrome,              tr("OpenNodeWithChrome"));
        DEFINE_ACTION(OpenNodeWithSleipnir,            tr("OpenNodeWithSleipnir"));
        DEFINE_ACTION(OpenNodeWithVivaldi,             tr("OpenNodeWithVivaldi"));
        DEFINE_ACTION(OpenNodeWithCustom,              tr("OpenNodeWithCustom"));
        DEFINE_ACTION(ToggleTrash,                     tr("ToggleTrash"));
        DEFINE_ACTION(ScrollUp,                        tr("ScrollUp"));
        DEFINE_ACTION(ScrollDown,                      tr("ScrollDown"));
        DEFINE_ACTION(PageUp,                          tr("PageUp"));
        DEFINE_ACTION(PageDown,                        tr("PageDown"));
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
    }
    switch(a){

    case Gadgets::Ge_ToggleNotifier:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetNotifier());
        action->setText(tr("Notifier"));
        action->setToolTip(tr("Notifier"));
        break;
    case Gadgets::Ge_ToggleReceiver:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetReceiver());
        action->setText(tr("Receiver"));
        action->setToolTip(tr("Receiver"));
        break;
    case Gadgets::Ge_ToggleMenuBar:
        action->setCheckable(true);
        action->setChecked(!GetTreeBank()->GetMainWindow()->IsMenuBarEmpty());
        action->setText(tr("MenuBar"));
        action->setToolTip(tr("MenuBar"));
        break;
    case Gadgets::Ge_ToggleTreeBar:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetMainWindow()->GetTreeBar()->isVisible());
        action->setText(tr("TreeBar"));
        action->setToolTip(tr("TreeBar"));
        break;
    case Gadgets::Ge_ToggleToolBar:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetMainWindow()->GetToolBar()->isVisible());
        action->setText(tr("ToolBar"));
        action->setToolTip(tr("ToolBar"));
        break;

    case Gadgets::Ge_OpenNodeWithIE:
        action->setIcon(Application::BrowserIcon_IE());
        break;
    case Gadgets::Ge_OpenNodeWithEdge:
        action->setIcon(Application::BrowserIcon_Edge());
        break;
    case Gadgets::Ge_OpenNodeWithFF:
        action->setIcon(Application::BrowserIcon_FF());
        break;
    case Gadgets::Ge_OpenNodeWithOpera:
        action->setIcon(Application::BrowserIcon_Opera());
        break;
    case Gadgets::Ge_OpenNodeWithOPR:
        action->setIcon(Application::BrowserIcon_OPR());
        break;
    case Gadgets::Ge_OpenNodeWithSafari:
        action->setIcon(Application::BrowserIcon_Safari());
        break;
    case Gadgets::Ge_OpenNodeWithChrome:
        action->setIcon(Application::BrowserIcon_Chrome());
        break;
    case Gadgets::Ge_OpenNodeWithSleipnir:
        action->setIcon(Application::BrowserIcon_Sleipnir());
        break;
    case Gadgets::Ge_OpenNodeWithVivaldi:
        action->setIcon(Application::BrowserIcon_Vivaldi());
        break;
    case Gadgets::Ge_OpenNodeWithCustom:
        action->setIcon(Application::BrowserIcon_Custom());
        action->setText(tr("OpenNodeWith%1").arg(Application::BrowserPath_Custom().split("/").last().replace(".exe", "")));
        break;
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

bool LocalView::ThumbList_Refresh(){
    if(!IsDisplayingNode()) return false;

    Load(m_ParentNode->GetUrl());

    m_NodesRegister.clear();
    return true;
}

bool LocalView::ThumbList_RefreshNoScroll(){
    if(!IsDisplayingNode()) return false;

    // recollect nodes and reset scroll.
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

        // return value is not accurate.
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

    // delete selected nodes or hovered node.

    if(!m_NodesRegister.isEmpty()){

        // delete duplicates.
        m_NodesRegister = m_NodesRegister.toSet().toList();
        DeleteNodes(m_NodesRegister);
        return true;

    } else if(Node *nd = GetHoveredNode()){

        DeleteNode(nd);
        return true;
    }
    return false;
}

bool LocalView::ThumbList_DeleteRightNode(){
    // not yet implemented.
    return false;
}

bool LocalView::ThumbList_DeleteLeftNode(){
    // not yet implemented.
    return false;
}

bool LocalView::ThumbList_DeleteOtherNode(){
    // not yet implemented.
    return false;
}

bool LocalView::ThumbList_PasteNode(){
    // not yet implemented.
    return false;
}

bool LocalView::ThumbList_RestoreNode(){
    // not yet implemented.
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

    // reset scroll.
    // want to scroll to 'PrimaryItem'.
    m_HoveredItemIndex = -1;
    m_PrimaryItemIndex = -1;

    // return value is not accurate.
    QUrl url = m_ParentNode->GetUrl();
    Load(url.resolved(QUrl(url.toString().endsWith(QStringLiteral("/")) ? QStringLiteral("../") : QStringLiteral("./"))));
    return true;
}

bool LocalView::ThumbList_DownDirectory(){
    if(!IsDisplayingNode()) return false;

    Node *nd = GetHoveredNode();
    if(!nd || !nd->IsDirectory()) return false;

    // reset scroll.
    // want to scroll to 'PrimaryItem'.
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

bool LocalView::ThumbList_OpenNodeWithIE(){
    return GraphicsTableView::ThumbList_OpenNodeWithIE();
}

bool LocalView::ThumbList_OpenNodeWithEdge(){
    return GraphicsTableView::ThumbList_OpenNodeWithEdge();
}

bool LocalView::ThumbList_OpenNodeWithFF(){
    return GraphicsTableView::ThumbList_OpenNodeWithFF();
}

bool LocalView::ThumbList_OpenNodeWithOpera(){
    return GraphicsTableView::ThumbList_OpenNodeWithOpera();
}

bool LocalView::ThumbList_OpenNodeWithOPR(){
    return GraphicsTableView::ThumbList_OpenNodeWithOPR();
}

bool LocalView::ThumbList_OpenNodeWithSafari(){
    return GraphicsTableView::ThumbList_OpenNodeWithSafari();
}

bool LocalView::ThumbList_OpenNodeWithChrome(){
    return GraphicsTableView::ThumbList_OpenNodeWithChrome();
}

bool LocalView::ThumbList_OpenNodeWithSleipnir(){
    return GraphicsTableView::ThumbList_OpenNodeWithSleipnir();
}

bool LocalView::ThumbList_OpenNodeWithVivaldi(){
    return GraphicsTableView::ThumbList_OpenNodeWithVivaldi();
}

bool LocalView::ThumbList_OpenNodeWithCustom(){
    return GraphicsTableView::ThumbList_OpenNodeWithCustom();
}

bool LocalView::ThumbList_MakeLocalNode(){
    // do nothing.
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
    // not yet implemented.
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        MakeDirectory(nd);

        Load(m_ParentNode->GetUrl());
        return true;
    }
    return false;
}

bool LocalView::ThumbList_MakeDirectoryWithSameDomainNode(){
    // not yet implemented.
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        MakeDirectory(nd);

        Load(m_ParentNode->GetUrl());
        return true;
    }
    return false;
}

bool LocalView::ThumbList_ToggleTrash(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_ApplyChildrenOrder(DisplayArea area, QPointF basepos){
    Q_UNUSED(area); Q_UNUSED(basepos);
    // do nothing.
    ThumbList_RefreshNoScroll();
    return false;
}

void LocalView::StartImageCollector(bool reverse){
    m_CollectingFuture = reverse
        ? QtConcurrent::run(this, &LocalView::LoadImageRequestReverse,
                            m_CurrentScroll)
        : QtConcurrent::run(this, &LocalView::LoadImageRequest,
                            m_CurrentScroll);
    for(int i = 100; i < 5000; i <<= 2)
        QTimer::singleShot(i, this, SLOT(update()));
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
       !m_MediaPlayer->media().isNull()){
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

bool LocalView::ThumbList_PageUp(){
    return ThumbList_MoveToPrevPage();
}

bool LocalView::ThumbList_PageDown(){
    return ThumbList_MoveToNextPage();
}

bool LocalView::ThumbList_MoveToUpperItem(){
    return SelectMediaItem(m_TargetScroll + m_CurrentThumbnailColumnCount,
                           [this](){ GraphicsTableView::ThumbList_MoveToUpperItem();});
}

bool LocalView::ThumbList_MoveToLowerItem(){
    return SelectMediaItem(m_TargetScroll - m_CurrentThumbnailColumnCount,
                           [this](){ GraphicsTableView::ThumbList_MoveToLowerItem();});
}

bool LocalView::ThumbList_MoveToRightItem(){
    return SelectMediaItem(m_TargetScroll - 1,
                           [this](){ GraphicsTableView::ThumbList_MoveToRightItem();});
}

bool LocalView::ThumbList_MoveToLeftItem(){
    return SelectMediaItem(m_TargetScroll + 1,
                           [this](){ GraphicsTableView::ThumbList_MoveToLeftItem();});
}

bool LocalView::ThumbList_MoveToPrevPage(){
    return SelectMediaItem(m_TargetScroll - 1,
                           [this](){ GraphicsTableView::ThumbList_PageUp();});
}

bool LocalView::ThumbList_MoveToNextPage(){
    return SelectMediaItem(m_TargetScroll + 1,
                           [this](){ GraphicsTableView::ThumbList_PageDown();});
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
    // do nothing.
    return false;
}

bool LocalView::ThumbList_TransferToLower(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_TransferToRight(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_TransferToLeft(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_TransferToPrevPage(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_TransferToNextPage(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_TransferToFirst(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_TransferToLast(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_TransferToUpDirectory(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_TransferToDownDirectory(){
    // do nothing.
    return false;
}

bool LocalView::ThumbList_ZoomIn(){
    bool result = GraphicsTableView::ThumbList_ZoomIn();
    GetHistNode()->SetZoom(m_CurrentThumbnailZoomFactor);
    return result;
}

bool LocalView::ThumbList_ZoomOut(){
    bool result = GraphicsTableView::ThumbList_ZoomOut();
    GetHistNode()->SetZoom(m_CurrentThumbnailZoomFactor);
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
    if(nd->IsRoot()) return;
    BoolCallBack callBack = [this, nd](bool ok){
        if(!ok) return;
        QString path = nd->GetUrl().toLocalFile();
        QFile file(path);
        file.remove();
        Load(m_ParentNode->GetUrl());
    };
    ModelessDialog::Question
        (tr("Delete File."),
         tr("Are you sure you want to delete this file?"),
         callBack, this);
}

void LocalView::DeleteNodes(NodeList list){
    BoolCallBack callBack = [this, list](bool ok){
        if(!ok) return;
        foreach(Node *nd, list){
            if(nd->IsRoot()) continue;
            QString path = nd->GetUrl().toLocalFile();
            QFile file(path);
            file.remove();
        }
        Load(m_ParentNode->GetUrl());
    };
    ModelessDialog::Question
        (tr("Delete Files."),
         tr("Are you sure you want to delete these files?"),
         callBack, this);
}

void LocalView::NewNode(Node *ln){
    // put file at neighbor of argument.
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
    // put file at neighbor of argument.
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
    // put file at neighbor of argument.
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

// don't call directly from GUI thread!
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

        // why not display correctly?
        //QSize size = QSize(m_CurrentThumbnailHeight, m_CurrentThumbnailHeight);
        QSize size = QSize(48, 48);
        QIcon icon = QFileIconProvider().icon(QFileInfo(path));
        if(!icon.isNull()){
            QImage *image = new QImage(icon.pixmap(size).toImage());
            LocalNode::m_FileImageCache.insert(path, image);
        }
        LocalNode::m_DiskAccessMutex.unlock();
    }
}

void LocalView::SwapMediaItem(int index){
    if(m_ParentNode->HasNoChildren()) return;

    if(index == -1){
        m_PixmapItem->setPixmap(QPixmap());
        m_PixmapItem->setOffset(QPointF());
        m_PixmapItem->setEnabled(false);
        m_PixmapItem->hide();

        m_MediaPlayer->stop();
        m_MediaPlayer->setMedia(QMediaContent());
        m_VideoItem->setEnabled(false);
        m_VideoItem->hide();

        m_CurrentNode = m_ParentNode->GetFirstChild();
        QString path = m_ParentNode->GetUrl().toLocalFile();
        emit urlChanged(QUrl::fromLocalFile(path));
        emit titleChanged(path);
    } else {
        if(index >= m_DisplayThumbnails.length())
            index = m_DisplayThumbnails.length() - 1;

        Node *nd = m_DisplayThumbnails[index]->GetNode();
        m_CurrentNode = nd;
        emit urlChanged(nd->GetUrl());
        emit titleChanged(nd->GetUrl().toLocalFile());

        if(IsSupportedVideo(nd->GetUrl())){
            if(!m_PixmapItem->pixmap().isNull()){
                m_PixmapItem->setPixmap(QPixmap());
                m_PixmapItem->setOffset(QPointF());
                m_PixmapItem->setEnabled(false);
                m_PixmapItem->hide();
            }

            m_MediaPlayer->setMedia(nd->GetUrl());
            m_MediaPlayer->play();
            m_VideoItem->setSize(Size());
            m_VideoItem->setOffset(QPointF());
            m_VideoItem->setEnabled(true);
            m_VideoItem->show();
            m_VideoItem->setFocus();
        } else if(IsSupportedImage(nd->GetUrl())){
            if(m_MediaPlayer->media().isNull()){
                m_MediaPlayer->stop();
                m_MediaPlayer->setMedia(QMediaContent());
                m_VideoItem->setEnabled(false);
                m_VideoItem->hide();
            }

            QString path = nd->GetUrl().toLocalFile();
            QPixmap pixmap = QPixmap(path);
            if(pixmap.isNull()) return;
            pixmap = pixmap.scaled(Size().toSize(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
            QSizeF diff = (Size() - pixmap.size())/2.0;
            m_PixmapItem->setPixmap(pixmap);
            m_PixmapItem->setOffset(diff.width(), diff.height());
            m_PixmapItem->setEnabled(true);
            m_PixmapItem->show();
            m_PixmapItem->setFocus();
        } else {
            if(m_MediaPlayer->media().isNull()){
                m_MediaPlayer->stop();
                m_MediaPlayer->setMedia(QMediaContent());
                m_VideoItem->setEnabled(false);
                m_VideoItem->hide();
            }

            QString path = nd->GetUrl().toLocalFile();
            // why not display correctly?
            //QSize size = QSize(m_CurrentThumbnailHeight, m_CurrentThumbnailHeight);
            QSize size = QSize(48, 48);
            QIcon icon = QFileIconProvider().icon(QFileInfo(path));
            if(icon.isNull()) return;
            QPixmap pixmap = icon.pixmap(size)
                .scaled(Size().toSize(),
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation);
            QSizeF diff = (Size() - pixmap.size())/2.0;
            m_PixmapItem->setPixmap(pixmap);
            m_PixmapItem->setOffset(diff.width(), diff.height());
            m_PixmapItem->setEnabled(true);
            m_PixmapItem->show();
            m_PixmapItem->setFocus();
        }
    }
}

void LocalView::OnSetViewNode(ViewNode*){}

void LocalView::OnSetHistNode(HistNode*){}

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

void LocalView::EmitScrollChangedIfNeed(){
    if(GetHistNode()->GetScrollY() != m_CurrentScroll)
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
    if(!GetHistNode()) return false;
    GetHistNode()->SetScrollX(0);
    GetHistNode()->SetScrollY(m_CurrentScroll);
    return true;
}

bool LocalView::RestoreScroll(){
    if(!GetHistNode()) return false;
    GraphicsTableView::SetScroll(GetHistNode()->GetScrollY());
    return true;
}

bool LocalView::SaveZoom(){
    if(!GetHistNode()) return false;
    GetHistNode()->SetZoom(m_CurrentThumbnailZoomFactor);
    return true;
}

bool LocalView::RestoreZoom(){
    if(!GetHistNode()) return false;

    m_CurrentThumbnailZoomFactor = GetHistNode()->GetZoom();
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
    Q_UNUSED(flags);
    if(!m_CurrentNode) return;
    CollectNodes(m_CurrentNode, text);
    ThumbList_MoveToFirstItem();
    // 'ThumbList_MoveToFirstItem' calls 'SetScrollToItem',
    // and 'SetScrollToItem' calls 'SetScroll',
    // but 'SetScroll' doesn't call 'update' when scroll value is not changed.
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
    // all key events are ignored, if input method is activated.
    // so input method specific keys are accepted.
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
    Q_UNUSED(ev);
    // do nothing.
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

    if(ev->buttons() & Qt::RightButton &&
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
       !m_MediaPlayer->media().isNull()){
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

    QString wheel;
    bool up = ev->delta() > 0;
    bool ignoreStatusBarMessage = true;

    if(!m_PixmapItem->pixmap().isNull() ||
       !m_MediaPlayer->media().isNull()){

        if(up) ThumbList_ScrollUp();
        else   ThumbList_ScrollDown();
        ev->setAccepted(true);
        return;
    }

    Application::AddModifiersToString(wheel, ev->modifiers());
    Application::AddMouseButtonsToString(wheel, ev->buttons());
    Application::AddWheelDirectionToString(wheel, up);

    if(Gadgets::GetMouseMap().contains(wheel)){

        QString str = Gadgets::GetMouseMap()[wheel];
        if(!str.isEmpty()){

            GestureAborted();

            // don't want to overwrite statusBarMessage in this event.
            // because these method may update statusBarMessage.
            if(!TriggerAction(str, ev->pos().toPoint())){
                ev->setAccepted(false);
                return;
            }
        }

    } else if(ScrollToChangeDirectory() &&
              ThumbnailAreaRect().contains(ev->pos())){

        // don't want to overwrite statusBarMessage in this event.
        // because these method updates statusBarMessage.
        if(up) ThumbList_UpDirectory();
        else   ThumbList_DownDirectory();

    } else {

        ignoreStatusBarMessage = false;
        if(up) ThumbList_ScrollUp();
        else   ThumbList_ScrollDown();
    }

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

        // 'HasAnyModifier' ignores ShiftModifier.
        if(Application::HasAnyModifier(ev)){
            m_LocalView->TriggerKeyEvent(ev);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Up ||
                  ev->key() == Qt::Key_Left ||
                  ev->key() == Qt::Key_PageUp ||
                  ev->key() == Qt::Key_Backtab ||
                  ev->key() == Qt::Key_Backspace ||
                  (ev->key() == Qt::Key_Space &&
                   ev->modifiers() == Qt::ShiftModifier)){

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
    QMediaPlayer *player = m_LocalView->GetMediaPlayer();
    player->setVolume(player->volume() + 10);
}

void VideoItem::VolumeDown(){
    emit statusBarMessage(tr("volume down"));
    QMediaPlayer *player = m_LocalView->GetMediaPlayer();
    player->setVolume(player->volume() - 10);
}

void VideoItem::SetPositionRelative(qint64 diff){
    QMediaPlayer *player = m_LocalView->GetMediaPlayer();
    player->setPosition(player->position() + diff);
}

void VideoItem::keyPressEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(!seq.isEmpty()){

        if(ev->key() == Qt::Key_Up){

            VolumeUp();
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Down){

            VolumeDown();
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Left &&
                  ev->modifiers() & Qt::ShiftModifier &&
                  ev->modifiers() & Qt::ControlModifier){

            emit statusBarMessage(tr("10 minutes back"));
            SetPositionRelative(-600000);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Right &&
                  ev->modifiers() & Qt::ShiftModifier &&
                  ev->modifiers() & Qt::ControlModifier){

            emit statusBarMessage(tr("10 minutes forward"));
            SetPositionRelative(600000);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Left &&
                  ev->modifiers() == Qt::ShiftModifier){

            emit statusBarMessage(tr("5 minutes back"));
            SetPositionRelative(-300000);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Right &&
                  ev->modifiers() == Qt::ShiftModifier){

            emit statusBarMessage(tr("5 minutes forward"));
            SetPositionRelative(300000);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Left &&
                  ev->modifiers() == Qt::ControlModifier){

            emit statusBarMessage(tr("1 minute back"));
            SetPositionRelative(-60000);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Right &&
                  ev->modifiers() == Qt::ControlModifier){

            emit statusBarMessage(tr("1 minute forward"));
            SetPositionRelative(60000);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Left){

            emit statusBarMessage(tr("10 seconds back"));
            SetPositionRelative(-10000);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Right){

            emit statusBarMessage(tr("10 seconds forward"));
            SetPositionRelative(10000);
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_PageDown){

            m_LocalView->ThumbList_MoveToRightItem();
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_PageUp){

            m_LocalView->ThumbList_MoveToRightItem();
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Home){

            m_LocalView->ThumbList_MoveToFirstItem();
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_End){

            m_LocalView->ThumbList_MoveToLastItem();
            ev->setAccepted(true);

        } else if(ev->key() == Qt::Key_Space){

            Pause();
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
