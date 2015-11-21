#include "switch.hpp"
#include "const.hpp"
#include "keymap.hpp"
#include "mousemap.hpp"

#include "gadgets.hpp"

#include <QtCore>
#include <QGraphicsObject>
#include <QSettings>
#include <QVector>
#include <QAction>
#include <QtWidgets>
#include <QStyle>

#include <functional>

#include "graphicstableview.hpp"

#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "thumbnail.hpp"
#include "nodetitle.hpp"
#include "accessiblewebelement.hpp"
#include "view.hpp"
#ifdef QTWEBKIT
#  include "webview.hpp"
#  include "quickwebview.hpp"
#endif
#include "webengineview.hpp"
#include "quickwebengineview.hpp"

#ifdef USE_LIGHTNODE
#  include "lightnode.hpp"
#else
#  include "node.hpp"
#endif

bool Gadgets::m_EnableMultiStroke = false;
QString Gadgets::m_AccessKeyCustomSequence = QString();
Gadgets::AccessKeyMode Gadgets::m_AccessKeyMode = BothHands;
Gadgets::AccessKeyAction Gadgets::m_AccessKeyAction = OpenMenu;
Gadgets::AccessKeySortOrientation Gadgets::m_AccessKeySortOrientation = Vertical;
Gadgets::AccessKeySelectBlockMethod Gadgets::m_AccessKeySelectBlockMethod = Number;

QMap<QKeySequence, QString> Gadgets::m_ThumbListKeyMap = QMap<QKeySequence, QString>();
QMap<QKeySequence, QString> Gadgets::m_AccessKeyKeyMap = QMap<QKeySequence, QString>();
QMap<QString, QString> Gadgets::m_MouseMap = QMap<QString, QString>();

Gadgets::Gadgets(TreeBank *parent)
    : View(parent)
    , GraphicsTableView(parent)
{
    m_ActionTable = QMap<GadgetsAction, QAction*>();

    m_ViewNodeCollectionType = Flat;
    m_HistNodeCollectionType = Recursive;

    m_AccessKeyCount = 0;
    m_AccessKeyLastBlockIndex = 0;
    m_CurrentAccessKeyBlockIndex = 0;
    m_AccessKeyLabels = QStringList();
    m_AccessKeyCurrentSelection = QString();

    connect(this, &Gadgets::titleChanged, this, &Gadgets::OnTitleChanged);
    connect(this, &Gadgets::urlChanged,   this, &Gadgets::OnUrlChanged);

    bool isNumber = m_AccessKeySelectBlockMethod == Number;
    int size = isNumber ? 10 : AccessKeyBlockSize();
    if(!m_EnableMultiStroke){
        for(int i = 0; i < size; i++){
            m_AccessKeyBlockLabels << new AccessKeyBlockLabel(this, i, isNumber);
        }
    }

    hide();
}

Gadgets::~Gadgets(){
}

TreeBank *Gadgets::parent(){
    return View::m_TreeBank;
}

void Gadgets::setParent(TreeBank *tb){
    View::SetTreeBank(tb);
    GraphicsTableView::SetTreeBank(tb);
    if(base()->scene()) scene()->removeItem(this);
    if(tb) tb->GetScene()->addItem(this);
}

void Gadgets::LoadSettings(){
    GraphicsTableView::LoadSettings();

    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup(QStringLiteral("gadgets"));{
        settings->beginGroup(QStringLiteral("thumblist"));{
            settings->beginGroup(QStringLiteral("keymap"));{
                QStringList keys = settings->allKeys();
                if(keys.isEmpty()){
                    /* default key map. */
                    THUMBLIST_KEYMAP
                } else {
                    if(!m_ThumbListKeyMap.isEmpty()) m_ThumbListKeyMap.clear();
                    foreach(QString key, keys){
                        if(key.isEmpty()) continue;
                        m_ThumbListKeyMap[Application::MakeKeySequence(key)] =
                            settings->value(key, QStringLiteral("NoAction")).value<QString>()
                            // cannot use slashes on QSettings.
                              .replace(QStringLiteral("Backslash"), QStringLiteral("\\"))
                              .replace(QStringLiteral("Slash"), QStringLiteral("/"));
                    }
                }
            }
            settings->endGroup();
            settings->beginGroup(QStringLiteral("mouse"));{
                QStringList keys = settings->allKeys();
                if(keys.isEmpty()){
                    /* default mouse map. */
                    THUMBLIST_MOUSEMAP
                } else {
                    if(!m_MouseMap.isEmpty()) m_MouseMap.clear();
                    foreach(QString key, keys){
                        if(key.isEmpty()) continue;
                        m_MouseMap[key] =
                            settings->value(key, QStringLiteral("NoAction")).value<QString>();
                    }
                }
            }
            settings->endGroup();
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("accesskey"));{
            m_EnableMultiStroke = settings->value(QStringLiteral("@EnableMultiStroke"), false).value<bool>();
            m_AccessKeyCustomSequence =
                settings->value(QStringLiteral("@AccessKeyCustomSequence"), QString()).value<QString>().toUpper()
                // cannot use slashes on QSettings.
                  .replace(QStringLiteral("Backslash"), QStringLiteral("\\"))
                  .replace(QStringLiteral("Slash"), QStringLiteral("/"));

            QString mode = settings->value(QStringLiteral("@AccessKeyMode"), QStringLiteral("BothHands")).value<QString>();
            if     (mode == QStringLiteral("BothHands")) m_AccessKeyMode = BothHands;
            else if(mode == QStringLiteral("LeftHand"))  m_AccessKeyMode = LeftHand;
            else if(mode == QStringLiteral("RightHand")) m_AccessKeyMode = RightHand;
            else if(mode == QStringLiteral("Custom"))    m_AccessKeyMode = Custom;

            QString action = settings->value(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenMenu")).value<QString>();
            if     (action == QStringLiteral("OpenMenu"))                     m_AccessKeyAction = OpenMenu;
            else if(action == QStringLiteral("ClickElement"))                 m_AccessKeyAction = ClickElement;
            else if(action == QStringLiteral("FocusElement"))                 m_AccessKeyAction = FocusElement;
            else if(action == QStringLiteral("HoverElement"))                 m_AccessKeyAction = HoverElement;
            else if(action == QStringLiteral("OpenInNewViewNode"))            m_AccessKeyAction = OpenInNewViewNode;
            else if(action == QStringLiteral("OpenInNewHistNode"))            m_AccessKeyAction = OpenInNewHistNode;
            else if(action == QStringLiteral("OpenInNewDirectory"))           m_AccessKeyAction = OpenInNewDirectory;
            else if(action == QStringLiteral("OpenOnRoot"))                   m_AccessKeyAction = OpenOnRoot;
            else if(action == QStringLiteral("OpenInNewViewNodeBackground"))  m_AccessKeyAction = OpenInNewViewNodeBackground;
            else if(action == QStringLiteral("OpenInNewHistNodeBackground"))  m_AccessKeyAction = OpenInNewHistNodeBackground;
            else if(action == QStringLiteral("OpenInNewDirectoryBackground")) m_AccessKeyAction = OpenInNewDirectoryBackground;
            else if(action == QStringLiteral("OpenOnRootBackground"))         m_AccessKeyAction = OpenOnRootBackground;
            else if(action == QStringLiteral("OpenInNewViewNodeNewWindow"))   m_AccessKeyAction = OpenInNewViewNodeNewWindow;
            else if(action == QStringLiteral("OpenInNewHistNodeNewWindow"))   m_AccessKeyAction = OpenInNewHistNodeNewWindow;
            else if(action == QStringLiteral("OpenInNewDirectoryNewWindow"))  m_AccessKeyAction = OpenInNewDirectoryNewWindow;
            else if(action == QStringLiteral("OpenOnRootNewWindow"))          m_AccessKeyAction = OpenOnRootNewWindow;

            QString orientation = settings->value(QStringLiteral("@AccessKeySortOrientation"), QStringLiteral("Vertical")).value<QString>();
            if     (orientation == QStringLiteral("Vertical"))   m_AccessKeySortOrientation = Vertical;
            else if(orientation == QStringLiteral("Horizontal")) m_AccessKeySortOrientation = Horizontal;

            QString method = settings->value(QStringLiteral("@AccessKeySelectBlockMethod"), QStringLiteral("Number")).value<QString>();
            if     (method == QStringLiteral("Number"))      m_AccessKeySelectBlockMethod = Number;
            else if(method == QStringLiteral("ShiftedChar")) m_AccessKeySelectBlockMethod = ShiftedChar;
            else if(method == QStringLiteral("CtrledChar"))  m_AccessKeySelectBlockMethod = CtrledChar;
            else if(method == QStringLiteral("AltedChar"))   m_AccessKeySelectBlockMethod = AltedChar;
            else if(method == QStringLiteral("MetaChar"))    m_AccessKeySelectBlockMethod = MetaChar;

            settings->beginGroup(QStringLiteral("keymap"));{
                QStringList keys = settings->allKeys();
                if(keys.isEmpty()){
                    /* default key map. */
                    ACCESSKEY_KEYMAP
                } else {
                    if(!m_AccessKeyKeyMap.isEmpty()) m_AccessKeyKeyMap.clear();
                    foreach(QString key, keys){
                        if(key.isEmpty()) continue;
                        m_AccessKeyKeyMap[Application::MakeKeySequence(key)] =
                            settings->value(key, QStringLiteral("NoAction")).value<QString>()
                            // cannot use slashes on QSettings.
                              .replace(QStringLiteral("Backslash"), QStringLiteral("\\"))
                              .replace(QStringLiteral("Slash"), QStringLiteral("/"));
                    }
                }
            }
            settings->endGroup();
        }
        settings->endGroup();
    }
    settings->endGroup();
    Thumbnail::Initialize();
    NodeTitle::Initialize();
    AccessibleWebElement::Initialize();
}

void Gadgets::SaveSettings(){
    GraphicsTableView::SaveSettings();

    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup(QStringLiteral("gadgets"));{
        settings->beginGroup(QStringLiteral("thumblist"));{
            settings->beginGroup(QStringLiteral("keymap"));{
                QList<QKeySequence> seqs = m_ThumbListKeyMap.keys();
                if(seqs.isEmpty()){
                    /* default key map. */
                } else {
                    foreach(QKeySequence seq, seqs){
                        if(!seq.isEmpty() && !seq.toString().isEmpty())
                            settings->setValue(seq.toString()
                                               // cannot use slashes on QSettings.
                                                 .replace(QStringLiteral("\\"), QStringLiteral("Backslash"))
                                                 .replace(QStringLiteral("/"), QStringLiteral("Slash")),
                                               m_ThumbListKeyMap[seq]);
                    }
                }
            }
            settings->endGroup();
            settings->beginGroup(QStringLiteral("mouse"));{
                QStringList buttons = m_MouseMap.keys();
                if(buttons.isEmpty()){
                    /* 'm_MouseMap' will be construct, when next startup. */
                } else {
                    foreach(QString button, buttons){
                        if(!button.isEmpty())
                            settings->setValue(button, m_MouseMap[button]);
                    }
                }
            }
            settings->endGroup();
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("accesskey"));{
            settings->setValue(QStringLiteral("@EnableMultiStroke"), m_EnableMultiStroke);
            settings->setValue(QStringLiteral("@AccessKeyCustomSequence"),
                               m_AccessKeyCustomSequence
                               // cannot use slashes on QSettings.
                                 .replace(QStringLiteral("\\"), QStringLiteral("Backslash"))
                                 .replace(QStringLiteral("/"), QStringLiteral("Slash")));

            if     (m_AccessKeyMode == BothHands) settings->setValue(QStringLiteral("@AccessKeyMode"), QStringLiteral("BothHands"));
            else if(m_AccessKeyMode == LeftHand)  settings->setValue(QStringLiteral("@AccessKeyMode"), QStringLiteral("LeftHand"));
            else if(m_AccessKeyMode == RightHand) settings->setValue(QStringLiteral("@AccessKeyMode"), QStringLiteral("RightHand"));
            else if(m_AccessKeyMode == Custom)    settings->setValue(QStringLiteral("@AccessKeyMode"), QStringLiteral("Custom"));

            if     (m_AccessKeyAction == OpenMenu)                     settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenMenu"));
            else if(m_AccessKeyAction == ClickElement)                 settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("ClickElement"));
            else if(m_AccessKeyAction == FocusElement)                 settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("FocusElement"));
            else if(m_AccessKeyAction == HoverElement)                 settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("HoverElement"));
            else if(m_AccessKeyAction == OpenInNewViewNode)            settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenInNewViewNode"));
            else if(m_AccessKeyAction == OpenInNewHistNode)            settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenInNewHistNode"));
            else if(m_AccessKeyAction == OpenInNewDirectory)           settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenInNewDirectory"));
            else if(m_AccessKeyAction == OpenOnRoot)                   settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenOnRoot"));
            else if(m_AccessKeyAction == OpenInNewViewNodeBackground)  settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenInNewViewNodeBackground"));
            else if(m_AccessKeyAction == OpenInNewHistNodeBackground)  settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenInNewHistNodeBackground"));
            else if(m_AccessKeyAction == OpenInNewDirectoryBackground) settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenInNewDirectoryBackground"));
            else if(m_AccessKeyAction == OpenOnRootBackground)         settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenOnRootBackground"));
            else if(m_AccessKeyAction == OpenInNewViewNodeNewWindow)   settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenInNewViewNodeNewWindow"));
            else if(m_AccessKeyAction == OpenInNewHistNodeNewWindow)   settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenInNewHistNodeNewWindow"));
            else if(m_AccessKeyAction == OpenInNewDirectoryNewWindow)  settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenInNewDirectoryNewWindow"));
            else if(m_AccessKeyAction == OpenOnRootNewWindow)          settings->setValue(QStringLiteral("@AccessKeyAction"), QStringLiteral("OpenOnRootNewWindow"));

            if     (m_AccessKeySortOrientation == Vertical)   settings->setValue(QStringLiteral("@AccessKeySortOrientation"), QStringLiteral("Vertical"));
            else if(m_AccessKeySortOrientation == Horizontal) settings->setValue(QStringLiteral("@AccessKeySortOrientation"), QStringLiteral("Horizontal"));

            if     (m_AccessKeySelectBlockMethod == Number)      settings->setValue(QStringLiteral("@AccessKeySelectBlockMethod"), QStringLiteral("Number"));
            else if(m_AccessKeySelectBlockMethod == ShiftedChar) settings->setValue(QStringLiteral("@AccessKeySelectBlockMethod"), QStringLiteral("ShiftedChar"));
            else if(m_AccessKeySelectBlockMethod == CtrledChar)  settings->setValue(QStringLiteral("@AccessKeySelectBlockMethod"), QStringLiteral("CtrledChar"));
            else if(m_AccessKeySelectBlockMethod == AltedChar)   settings->setValue(QStringLiteral("@AccessKeySelectBlockMethod"), QStringLiteral("AltedChar"));
            else if(m_AccessKeySelectBlockMethod == MetaChar)    settings->setValue(QStringLiteral("@AccessKeySelectBlockMethod"), QStringLiteral("MetaChar"));

            settings->beginGroup(QStringLiteral("keymap"));{
                QList<QKeySequence> seqs = m_AccessKeyKeyMap.keys();
                if(seqs.isEmpty()){
                    /* default key map. */
                } else {
                    foreach(QKeySequence seq, seqs){
                        if(!seq.isEmpty() && !seq.toString().isEmpty())
                            settings->setValue(seq.toString()
                                               // cannot use slashes on QSettings.
                                                 .replace(QStringLiteral("\\"), QStringLiteral("Backslash"))
                                                 .replace(QStringLiteral("/"), QStringLiteral("Slash")),
                                               m_AccessKeyKeyMap[seq]);
                    }
                }
            }
            settings->endGroup();
        }
        settings->endGroup();
    }
    settings->endGroup();
}

void Gadgets::Activate(DisplayType type){
    GraphicsTableView::Activate(type);
}

void Gadgets::Deactivate(){
    GraphicsTableView::Deactivate();

    QTimer::singleShot(0, this, SLOT(DisableAccessKey()));

    if(GetTreeBank()){
        if(SharedView master = GetMaster().lock()){
            GetTreeBank()->SetCurrent(master);

            if(GetThis().lock() == master->GetSlave().lock())
                master->SetSlave(WeakView());
            SetMaster(WeakView());
        }
    }
}

void Gadgets::DisableAccessKey(){
    if(IsDisplayingAccessKey()){
        foreach(AccessibleWebElement *link, m_AccessibleWebElementCache){
            link->SetElement(NULL);
        }
    }
}

void Gadgets::UpKey(){
    UpKeyEvent();
}

void Gadgets::DownKey(){
    DownKeyEvent();
}

void Gadgets::RightKey(){
    RightKeyEvent();
}

void Gadgets::LeftKey(){
    LeftKeyEvent();
}

void Gadgets::HomeKey(){
    HomeKeyEvent();
}

void Gadgets::EndKey(){
    EndKeyEvent();
}

void Gadgets::PageUpKey(){
    PageUpKeyEvent();
}

void Gadgets::PageDownKey(){
    PageDownKeyEvent();
}

void Gadgets::Connect(TreeBank *tb){
    if(!tb) return;

    if(Receiver *receiver = tb->GetReceiver()){
        connect(receiver, SIGNAL(OpenUrl(QUrl)),
                this, SLOT(OpenInNew(QUrl)));
        connect(receiver, SIGNAL(OpenUrl(QList<QUrl>)),
                this, SLOT(OpenInNew(QList<QUrl>)));
        connect(receiver, SIGNAL(OpenQueryUrl(QString)),
                this, SLOT(OpenInNew(QString)));
        connect(receiver, SIGNAL(SearchWith(QString, QString)),
                this, SLOT(OpenInNew(QString, QString)));
        connect(receiver, SIGNAL(Download(QString, QString)),
                this, SLOT(Download(QString, QString)));

        connect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                this, SLOT(Load(const QString&)));
        connect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                this, SLOT(SeekText(const QString&, View::FindFlags)));
        connect(receiver, SIGNAL(KeyEvent(QString)),
                this, SLOT(KeyEvent(QString)));

        connect(receiver, SIGNAL(TriggerElementAction(Page::CustomAction)),
                this, SLOT(AccessKey_TriggerElementAction(Page::CustomAction)));

        connect(receiver, SIGNAL(Up()),    this, SLOT(ThumbList_MoveToUpperItem()));
        connect(receiver, SIGNAL(Down()),  this, SLOT(ThumbList_MoveToLowerItem()));
        connect(receiver, SIGNAL(Right()), this, SLOT(ThumbList_MoveToRightItem()));
        connect(receiver, SIGNAL(Left()),  this, SLOT(ThumbList_MoveToLeftItem()));
        connect(receiver, SIGNAL(Home()),  this, SLOT(ThumbList_MoveToFirstItem()));
        connect(receiver, SIGNAL(End()),   this, SLOT(ThumbList_MoveToLastItem()));

        connect(receiver, SIGNAL(Deactivate()),                      this, SLOT(Deactivate()));
        connect(receiver, SIGNAL(Refresh()),                         this, SLOT(ThumbList_Refresh()));
        connect(receiver, SIGNAL(RefreshNoScroll()),                 this, SLOT(ThumbList_RefreshNoScroll()));
        connect(receiver, SIGNAL(OpenNode()),                        this, SLOT(ThumbList_OpenNode()));
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
        connect(receiver, SIGNAL(PageUp()),                          this, SLOT(ThumbList_PageUp()));
        connect(receiver, SIGNAL(PageDown()),                        this, SLOT(ThumbList_PageDown()));
        connect(receiver, SIGNAL(ZoomIn()),                          this, SLOT(ThumbList_ZoomIn()));
        connect(receiver, SIGNAL(ZoomOut()),                         this, SLOT(ThumbList_ZoomOut()));
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
        connect(receiver, SIGNAL(SelectAll()),                       this, SLOT(ThumbList_SelectAll()));
        connect(receiver, SIGNAL(ClearSelection()),                  this, SLOT(ThumbList_ClearSelection()));
        connect(receiver, SIGNAL(TransferToUpper()),                 this, SLOT(ThumbList_TransferToUpper()));
        connect(receiver, SIGNAL(TransferToLower()),                 this, SLOT(ThumbList_TransferToLower()));
        connect(receiver, SIGNAL(TransferToRight()),                 this, SLOT(ThumbList_TransferToRight()));
        connect(receiver, SIGNAL(TransferToLeft()),                  this, SLOT(ThumbList_TransferToLeft()));
        connect(receiver, SIGNAL(TransferToPrevPage()),              this, SLOT(ThumbList_TransferToPrevPage()));
        connect(receiver, SIGNAL(TransferToNextPage()),              this, SLOT(ThumbList_TransferToNextPage()));
        connect(receiver, SIGNAL(TransferToFirst()),                 this, SLOT(ThumbList_TransferToFirst()));
        connect(receiver, SIGNAL(TransferToLast()),                  this, SLOT(ThumbList_TransferToLast()));
        connect(receiver, SIGNAL(TransferToUpDirectory()),           this, SLOT(ThumbList_TransferToUpDirectory()));
        connect(receiver, SIGNAL(TransferToDownDirectory()),         this, SLOT(ThumbList_TransferToDownDirectory()));
        connect(receiver, SIGNAL(SwitchNodeCollectionType()),        this, SLOT(ThumbList_SwitchNodeCollectionType()));
        connect(receiver, SIGNAL(SwitchNodeCollectionTypeReverse()), this, SLOT(ThumbList_SwitchNodeCollectionTypeReverse()));
    }
}

void Gadgets::Disconnect(TreeBank *tb){
    if(!tb) return;

    if(Receiver *receiver = tb->GetReceiver()){
        disconnect(receiver, SIGNAL(OpenUrl(QUrl)),
                   this, SLOT(OpenInNew(QUrl)));
        disconnect(receiver, SIGNAL(OpenUrl(QList<QUrl>)),
                   this, SLOT(OpenInNew(QList<QUrl>)));
        disconnect(receiver, SIGNAL(OpenQueryUrl(QString)),
                   this, SLOT(OpenInNew(QString)));
        disconnect(receiver, SIGNAL(SearchWith(QString, QString)),
                   this, SLOT(OpenInNew(QString, QString)));
        disconnect(receiver, SIGNAL(Download(QString, QString)),
                   this, SLOT(Download(QString, QString)));

        disconnect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                   this, SLOT(Load(const QString&)));
        disconnect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                   this, SLOT(SeekText(const QString&, View::FindFlags)));
        disconnect(receiver, SIGNAL(KeyEvent(QString)),
                   this, SLOT(KeyEvent(QString)));

        disconnect(receiver, SIGNAL(TriggerElementAction(Page::CustomAction)),
                   this, SLOT(AccessKey_TriggerElementAction(Page::CustomAction)));

        disconnect(receiver, SIGNAL(Up()),    this, SLOT(ThumbList_MoveToUpperItem()));
        disconnect(receiver, SIGNAL(Down()),  this, SLOT(ThumbList_MoveToLowerItem()));
        disconnect(receiver, SIGNAL(Right()), this, SLOT(ThumbList_MoveToRightItem()));
        disconnect(receiver, SIGNAL(Left()),  this, SLOT(ThumbList_MoveToLeftItem()));
        disconnect(receiver, SIGNAL(Home()),  this, SLOT(ThumbList_MoveToFirstItem()));
        disconnect(receiver, SIGNAL(End()),   this, SLOT(ThumbList_MoveToLastItem()));

        disconnect(receiver, SIGNAL(Deactivate()),                      this, SLOT(Deactivate()));
        disconnect(receiver, SIGNAL(Refresh()),                         this, SLOT(ThumbList_Refresh()));
        disconnect(receiver, SIGNAL(RefreshNoScroll()),                 this, SLOT(ThumbList_RefreshNoScroll()));
        disconnect(receiver, SIGNAL(OpenNode()),                        this, SLOT(ThumbList_OpenNode()));
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
        disconnect(receiver, SIGNAL(PageUp()),                          this, SLOT(ThumbList_PageUp()));
        disconnect(receiver, SIGNAL(PageDown()),                        this, SLOT(ThumbList_PageDown()));
        disconnect(receiver, SIGNAL(ZoomIn()),                          this, SLOT(ThumbList_ZoomIn()));
        disconnect(receiver, SIGNAL(ZoomOut()),                         this, SLOT(ThumbList_ZoomOut()));
        disconnect(receiver, SIGNAL(MoveToUpperItem()),                 this, SLOT(ThumbList_MoveToUpperItem()));
        disconnect(receiver, SIGNAL(MoveToLowerItem()),                 this, SLOT(ThumbList_MoveToLowerItem()));
        disconnect(receiver, SIGNAL(MoveToRightItem()),                 this, SLOT(ThumbList_MoveToRightItem()));
        disconnect(receiver, SIGNAL(MoveToPrevPage()),                  this, SLOT(ThumbList_MoveToPrevPage()));
        disconnect(receiver, SIGNAL(MoveToNextPage()),                  this, SLOT(ThumbList_MoveToNextPage()));
        disconnect(receiver, SIGNAL(MoveToLeftItem()),                  this, SLOT(ThumbList_MoveToLeftItem()));
        disconnect(receiver, SIGNAL(MoveToFirstItem()),                 this, SLOT(ThumbList_MoveToFirstItem()));
        disconnect(receiver, SIGNAL(MoveToLastItem()),                  this, SLOT(ThumbList_MoveToLastItem()));
        disconnect(receiver, SIGNAL(SelectToUpperItem()),               this, SLOT(ThumbList_SelectToUpperItem()));
        disconnect(receiver, SIGNAL(SelectToLowerItem()),               this, SLOT(ThumbList_SelectToLowerItem()));
        disconnect(receiver, SIGNAL(SelectToRightItem()),               this, SLOT(ThumbList_SelectToRightItem()));
        disconnect(receiver, SIGNAL(SelectToPrevPage()),                this, SLOT(ThumbList_SelectToPrevPage()));
        disconnect(receiver, SIGNAL(SelectToNextPage()),                this, SLOT(ThumbList_SelectToNextPage()));
        disconnect(receiver, SIGNAL(SelectToLeftItem()),                this, SLOT(ThumbList_SelectToLeftItem()));
        disconnect(receiver, SIGNAL(SelectToFirstItem()),               this, SLOT(ThumbList_SelectToFirstItem()));
        disconnect(receiver, SIGNAL(SelectToLastItem()),                this, SLOT(ThumbList_SelectToLastItem()));
        disconnect(receiver, SIGNAL(SelectItem()),                      this, SLOT(ThumbList_SelectItem()));
        disconnect(receiver, SIGNAL(SelectRange()),                     this, SLOT(ThumbList_SelectRange()));
        disconnect(receiver, SIGNAL(SelectAll()),                       this, SLOT(ThumbList_SelectAll()));
        disconnect(receiver, SIGNAL(ClearSelection()),                  this, SLOT(ThumbList_ClearSelection()));
        disconnect(receiver, SIGNAL(TransferToUpper()),                 this, SLOT(ThumbList_TransferToUpper()));
        disconnect(receiver, SIGNAL(TransferToLower()),                 this, SLOT(ThumbList_TransferToLower()));
        disconnect(receiver, SIGNAL(TransferToRight()),                 this, SLOT(ThumbList_TransferToRight()));
        disconnect(receiver, SIGNAL(TransferToLeft()),                  this, SLOT(ThumbList_TransferToLeft()));
        disconnect(receiver, SIGNAL(TransferToPrevPage()),              this, SLOT(ThumbList_TransferToPrevPage()));
        disconnect(receiver, SIGNAL(TransferToNextPage()),              this, SLOT(ThumbList_TransferToNextPage()));
        disconnect(receiver, SIGNAL(TransferToFirst()),                 this, SLOT(ThumbList_TransferToFirst()));
        disconnect(receiver, SIGNAL(TransferToLast()),                  this, SLOT(ThumbList_TransferToLast()));
        disconnect(receiver, SIGNAL(TransferToUpDirectory()),           this, SLOT(ThumbList_TransferToUpDirectory()));
        disconnect(receiver, SIGNAL(TransferToDownDirectory()),         this, SLOT(ThumbList_TransferToDownDirectory()));
        disconnect(receiver, SIGNAL(SwitchNodeCollectionType()),        this, SLOT(ThumbList_SwitchNodeCollectionType()));
        disconnect(receiver, SIGNAL(SwitchNodeCollectionTypeReverse()), this, SLOT(ThumbList_SwitchNodeCollectionTypeReverse()));
    }
}

void Gadgets::OpenInNew(QUrl url){
    if(IsDisplayingViewNode() && m_CurrentNode && m_CurrentNode->GetParent()){

        if(ViewNode *vn = GetHoveredViewNode()){

            GetTreeBank()->OpenInNewViewNode(url, true, vn);
            ThumbList_RefreshNoScroll();
            ThumbList_MoveToRightItem();

        } else {
            GetTreeBank()->OpenOnSuitableNode
                (url, true, m_CurrentNode->GetParent()->ToViewNode());

            ThumbList_RefreshNoScroll();
            ThumbList_MoveToLastItem();
        }
    }
}

void Gadgets::OpenInNew(QList<QUrl> urls){
    if(IsDisplayingViewNode() && m_CurrentNode && m_CurrentNode->GetParent()){

        if(ViewNode *vn = GetHoveredViewNode()){

            GetTreeBank()->OpenInNewViewNode(urls, true, vn);
            ThumbList_RefreshNoScroll();
            ThumbList_MoveToRightItem();

        } else {
            GetTreeBank()->OpenOnSuitableNode
                (urls, true, m_CurrentNode->GetParent()->ToViewNode());

            ThumbList_RefreshNoScroll();
            ThumbList_MoveToLastItem();
        }
    }
}

void Gadgets::OpenInNew(QString query){
    if(IsDisplayingViewNode() && m_CurrentNode && m_CurrentNode->GetParent()){

        if(ViewNode *vn = GetHoveredViewNode()){

            GetTreeBank()->OpenInNewViewNode(Page::CreateQueryUrl(query), true, vn);
            ThumbList_RefreshNoScroll();
            ThumbList_MoveToRightItem();

        } else {
            GetTreeBank()->OpenOnSuitableNode
                (Page::CreateQueryUrl(query), true, m_CurrentNode->GetParent()->ToViewNode());

            ThumbList_RefreshNoScroll();
            ThumbList_MoveToLastItem();
        }
    }
}

void Gadgets::OpenInNew(QString key, QString query){
    if(IsDisplayingViewNode() && m_CurrentNode && m_CurrentNode->GetParent()){

        if(ViewNode *vn = GetHoveredViewNode()){

            GetTreeBank()->OpenInNewViewNode(Page::CreateQueryUrl(query, key), true, vn);
            ThumbList_RefreshNoScroll();
            ThumbList_MoveToRightItem();

        } else {
            GetTreeBank()->OpenOnSuitableNode
                (Page::CreateQueryUrl(query, key), true, m_CurrentNode->GetParent()->ToViewNode());

            ThumbList_RefreshNoScroll();
            ThumbList_MoveToLastItem();
        }
    }
}

void Gadgets::Download(QString, QString){}

void Gadgets::SeekText(const QString &text, View::FindFlags flags){
    Q_UNUSED(flags);
    CollectNodes(m_CurrentNode, text);
    ThumbList_MoveToFirstItem();
    // 'ThumbList_MoveToFirstItem' calls 'SetScrollSoft',
    // and 'SetScrollSoft' calls 'SetScroll',
    // but 'SetScroll' doesn't call 'update' when scroll value is not changed.
    Update();
}

void Gadgets::KeyEvent(QString str){
    if(IsDisplayingNode()){
        TriggerKeyEvent(str);
    } else if(IsDisplayingAccessKey()){
        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        QKeySequence seq = Application::MakeKeySequence(str);
        QList<QGraphicsItem*> items = scene()->selectedItems();
        AccessibleWebElement *awe = 0;

        if(items.length() == 1){
            if(QRegExp(QStringLiteral("[eE]sc(?:ape)?")).exactMatch(str)){
                AccessKey_ClearSelection();
            } else if(master->IsRenderable() &&
                      (awe = dynamic_cast<AccessibleWebElement*>(items.first()))){
                AccessKey_TriggerAction(awe, seq);
            }
        } else if(QRegExp(QStringLiteral("[eE]sc(?:ape)?")).exactMatch(str)){

            Deactivate();

        } else if(m_EnableMultiStroke){

            if(IsValidAccessKey(str)){
                m_AccessKeyCurrentSelection += str.toUpper();
            } else if(m_AccessKeyCurrentSelection.length() >= 1){
                int len = m_AccessKeyCurrentSelection.length();
                m_AccessKeyCurrentSelection = m_AccessKeyCurrentSelection.left(len-1);
            } else {
                Deactivate();
                return;
            }
            AccessKey_SieveLabel();

        } else if(QRegExp(QStringLiteral("[uU]p")).exactMatch(str) ||
                  QRegExp(QStringLiteral("[lL]eft")).exactMatch(str) ||
                  QRegExp(QStringLiteral("[pP](?:g|age)?[uU]p")).exactMatch(str) ||
                  QRegExp(QStringLiteral("[bB]ack[tT]ab")).exactMatch(str) ||
                  QRegExp(QStringLiteral("[bB]ack[sS]pace")).exactMatch(str) ||
                  QRegExp(QStringLiteral("[sS]hift\\+[sS]pace")).exactMatch(str)){

            AccessKey_PrevBlock();

        } else if(QRegExp(QStringLiteral("[dD]own")).exactMatch(str) ||
                  QRegExp(QStringLiteral("[rR]ight")).exactMatch(str) ||
                  QRegExp(QStringLiteral("[pP](?:g|age)?[dD](?:ow)?n")).exactMatch(str) ||
                  QRegExp(QStringLiteral("[tT]ab")).exactMatch(str) ||
                  QRegExp(QStringLiteral("[sS]pace")).exactMatch(str)){

            AccessKey_NextBlock();

        } else if(QRegExp(QStringLiteral("[hH]ome")).exactMatch(str)){

            AccessKey_FirstBlock();

        } else if(QRegExp(QStringLiteral("[eE]nd")).exactMatch(str)){

            AccessKey_LastBlock();

        } else if(QRegExp(QStringLiteral("[0-9]+")).exactMatch(str)){

            AccessKey_NthBlock(str.toInt());

        } else if(IsValidAccessKey(str)){

            AccessKey_OpenElement(KeyToIndex(str));
        }
    }
}

void Gadgets::TriggerNativeLoadAction(const QUrl &url){
    if(IsDisplayingViewNode() && m_CurrentNode && m_CurrentNode->GetParent()){
        GetTreeBank()->OpenOnSuitableNode
            (url, true, m_CurrentNode->GetParent()->ToViewNode());
        ThumbList_RefreshNoScroll();
        ThumbList_MoveToLastItem();
    }
}

void Gadgets::TriggerNativeLoadAction(const QNetworkRequest &req,
                                      QNetworkAccessManager::Operation operation,
                                      const QByteArray &body){
    Q_UNUSED(operation); Q_UNUSED(body);
    if(IsDisplayingViewNode() && m_CurrentNode && m_CurrentNode->GetParent()){
        GetTreeBank()->OpenOnSuitableNode
            (req, true, m_CurrentNode->GetParent()->ToViewNode());
        ThumbList_RefreshNoScroll();
        ThumbList_MoveToLastItem();
    }
}

bool Gadgets::IsActive(){
    return isVisible();
}

void Gadgets::SetCurrentView(View *view){
    if(m_DisplayType == AccessKey){
        SetMaster(view->GetThis());
        if(view){
            view->SetSlave(GetThis());
            // for 'paint()'.
            m_CurrentNode = view->GetViewNode();
            CollectAccessibleWebElement(view);
        }
    }
}

static bool WebElementVerticalLessThan(AccessibleWebElement *l1, AccessibleWebElement *l2){
    if(!l1->isVisible()) return false;
    if(!l2->isVisible()) return true;
    QPoint p1 = l1->GetBoundingPos();
    QPoint p2 = l2->GetBoundingPos();
    if(p1.y()/ACCESSKEY_GRID_UNIT_SIZE ==
       p2.y()/ACCESSKEY_GRID_UNIT_SIZE)
        return p1.x() < p2.x();
    else
        return p1.y() < p2.y();
};

static bool WebElementHorizontalLessThan(AccessibleWebElement *l1, AccessibleWebElement *l2){
    if(!l1->isVisible()) return false;
    if(!l2->isVisible()) return true;
    QPoint p1 = l1->GetBoundingPos();
    QPoint p2 = l2->GetBoundingPos();
    if(p1.x()/ACCESSKEY_GRID_UNIT_SIZE ==
       p2.x()/ACCESSKEY_GRID_UNIT_SIZE)
        return p1.y() < p2.y();
    else
        return p1.x() < p2.x();
};

void Gadgets::CreateLabelSequence(){
    static const QStringList bothHandsSeq =
        QStringList()
        << QStringLiteral("K")
        << QStringLiteral("D")
        << QStringLiteral("L")
        << QStringLiteral("S")
        << QStringLiteral("J")
        << QStringLiteral("F")
      //<< QStringLiteral(";")
        << QStringLiteral("A")
        << QStringLiteral("H")
        << QStringLiteral("G")
        << QStringLiteral("I")
        << QStringLiteral("E")
        << QStringLiteral("O")
        << QStringLiteral("W")
        << QStringLiteral("U")
        << QStringLiteral("R")
        << QStringLiteral("P")
        << QStringLiteral("Q")
        << QStringLiteral("Y")
        << QStringLiteral("T")
      //<< QStringLiteral(",")
        << QStringLiteral("C")
      //<< QStringLiteral(".")
        << QStringLiteral("X")
        << QStringLiteral("M")
        << QStringLiteral("V")
      //<< QStringLiteral("/")
        << QStringLiteral("Z")
        << QStringLiteral("N")
        << QStringLiteral("B");

    static const QStringList leftHandSeq =
        QStringList()
        << QStringLiteral("D")
        << QStringLiteral("S")
        << QStringLiteral("F")
        << QStringLiteral("A")
        << QStringLiteral("G")
        << QStringLiteral("E")
        << QStringLiteral("W")
        << QStringLiteral("R")
        << QStringLiteral("Q")
        << QStringLiteral("T")
        << QStringLiteral("C")
        << QStringLiteral("X")
        << QStringLiteral("V")
        << QStringLiteral("Z")
        << QStringLiteral("B");

    static const QStringList rightHandSeq =
        QStringList()
        << QStringLiteral("K")
        << QStringLiteral("L")
        << QStringLiteral("J")
        << QStringLiteral(";")
        << QStringLiteral("H")
        << QStringLiteral("I")
        << QStringLiteral("O")
        << QStringLiteral("U")
        << QStringLiteral("P")
        << QStringLiteral("Y")
        << QStringLiteral(",")
        << QStringLiteral(".")
        << QStringLiteral("M")
        << QStringLiteral("/")
        << QStringLiteral("N");

    static QStringList customSeq;
    if(m_AccessKeyMode == Custom && customSeq.isEmpty()){
        customSeq = m_AccessKeyCustomSequence.split("");
        customSeq.removeAll("");
        if(customSeq.length() < 2)
            customSeq = bothHandsSeq;
    }

    static QStringList seq =
        m_AccessKeyMode == BothHands ? bothHandsSeq :
        m_AccessKeyMode == LeftHand  ? leftHandSeq :
        m_AccessKeyMode == RightHand ? rightHandSeq :
        m_AccessKeyMode == Custom    ? customSeq : QStringList();

    Q_ASSERT(!seq.isEmpty());

    QStringList shorter = seq;
    QStringList longer = QStringList();

    while(m_AccessKeyCount >= shorter.length() + longer.length()){
        if(shorter.isEmpty()){
            shorter = longer;
            longer = QStringList();
        }
        QString head = shorter.takeFirst();
        foreach(QString tail, seq){
            longer << head + tail;
        }
    }
    QStringList tmp = shorter + longer;

    if(tmp.length() / seq.length() >= 1){
        m_AccessKeyLabels = QStringList();
        int width = seq.length();
        int height = tmp.length() / seq.length();
        for(int i = 0; i < width; i++){
            for(int j = 0; j <= height; j++){
                if(j * width + i < m_AccessKeyCount)
                    m_AccessKeyLabels << tmp[j * width + i];
                else break;
            }
        }
    } else {
        m_AccessKeyLabels = tmp;
    }
    m_AccessKeyCurrentSelection = QString();
}

void Gadgets::CollectAccessibleWebElement(View *view){
    m_CurrentAccessKeyBlockIndex = 0;

    foreach(AccessKeyBlockLabel *label, m_AccessKeyBlockLabels){
        label->setRect(QRectF());
    }

    foreach(QGraphicsItem *item, childItems()){
        item->setSelected(false);
        item->setVisible(false);
        item->setEnabled(false);
    }

    view->CallWithFoundElements
        (Page::ForAccessKey,
         [this](SharedWebElementList elems){
            int count = 0;
            foreach(SharedWebElement elem, elems){

                if(elem->IsNull()) continue;

                if(count >= m_AccessibleWebElementCache.length()){
                    m_AccessibleWebElementCache << new AccessibleWebElement(this);
                }
                AccessibleWebElement *link = m_AccessibleWebElementCache[count];
                link->setVisible(true);
                link->setEnabled(true);
                link->SetElement(elem);
                link->SetBoundingPos(elem->Position());

                count++;
            }

            qSort(m_AccessibleWebElementCache.begin(),
                  m_AccessibleWebElementCache.end(),
                  m_AccessKeySortOrientation == Vertical   ? &WebElementVerticalLessThan :
                  m_AccessKeySortOrientation == Horizontal ? &WebElementHorizontalLessThan :
                  &WebElementVerticalLessThan);

            bool isNumber = m_AccessKeySelectBlockMethod == Number;
            int size = isNumber ? 10 : AccessKeyBlockSize();

            for(int i = 0; i < count; i++){
                AccessibleWebElement *link = m_AccessibleWebElementCache[i];

                link->SetIndex(i);

                int index = i / AccessKeyBlockSize();
                if(index >= size || m_EnableMultiStroke) continue;

                AccessKeyBlockLabel *label =  m_AccessKeyBlockLabels[index];
                QRectF rect = link->CharChipRect();

                if(label->isVisible()){
                    label->setRect(boundingRect().intersected(label->rect().united(rect)));
                } else {
                    label->setRect(rect);
                    label->setVisible(true);
                    label->setEnabled(true);
                }
            }
            m_AccessKeyCount = count;
            m_AccessKeyLastBlockIndex = count ? (count - 1) / AccessKeyBlockSize() : 0;
            if(m_EnableMultiStroke) CreateLabelSequence();
        });
}

void Gadgets::OnTitleChanged(const QString &title){
    ChangeNodeTitle(title);
}

void Gadgets::OnUrlChanged(const QUrl &uri){
    ChangeNodeUrl(uri);
}

void Gadgets::UpdateThumbnail(){
    if(m_HistNode){
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
                if(item->zValue() > SPOT_LIGHT_LAYER) break;

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
            m_HistNode->SetImage(
                image.
                scaled(SAVING_THUMBNAIL_SIZE,
                       Qt::KeepAspectRatioByExpanding,
                       Qt::SmoothTransformation));
        } else {
            m_HistNode->SetImage(
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

bool Gadgets::TriggerKeyEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return false;
    QString str = m_ThumbListKeyMap[seq];
    if(str.isEmpty()) return false;

    return TriggerAction(str);
}

bool Gadgets::TriggerKeyEvent(QString str){
    QKeySequence seq = Application::MakeKeySequence(str);
    if(seq.isEmpty()) return false;
    str = m_ThumbListKeyMap[seq]; // sequence => action
    if(str.isEmpty()) return false;

    return TriggerAction(str);
}

QMap<QKeySequence, QString> Gadgets::GetThumbListKeyMap(){
    return m_ThumbListKeyMap;
}

QMap<QKeySequence, QString> Gadgets::GetAccessKeyKeyMap(){
    return m_AccessKeyKeyMap;
}

QMap<QString, QString> Gadgets::GetMouseMap(){
    return m_MouseMap;
}

bool Gadgets::IsValidAccessKey(int k){
    //return Qt::Key_A <= k && k <= Qt::Key_Z
    //    || k == Qt::Key_Semicolon
    //    || k == Qt::Key_Colon
    //    || k == Qt::Key_Comma
    //    || k == Qt::Key_Period
    //    || k == Qt::Key_Slash;

    switch(m_AccessKeyMode){

        // both hands' keys: alphabets.
    case BothHands :
        return Qt::Key_A <= k && k <= Qt::Key_Z;

        // left hand's keys:
        // QWERT
        // ASDFG
        // ZXCVB
    case LeftHand :
        switch(k){
        case Qt::Key_Q :
        case Qt::Key_W :
        case Qt::Key_E :
        case Qt::Key_R :
        case Qt::Key_T :
        case Qt::Key_A :
        case Qt::Key_S :
        case Qt::Key_D :
        case Qt::Key_F :
        case Qt::Key_G :
        case Qt::Key_Z :
        case Qt::Key_X :
        case Qt::Key_C :
        case Qt::Key_V :
        case Qt::Key_B :
            return true;
        }
        break;

        // right hand's keys:
        // YUIOP
        // HJKL;
        // NM,./
    case RightHand :
        switch(k){
        case Qt::Key_Y :
        case Qt::Key_U :
        case Qt::Key_I :
        case Qt::Key_O :
        case Qt::Key_P :
        case Qt::Key_H :
        case Qt::Key_J :
        case Qt::Key_K :
        case Qt::Key_L :
        case Qt::Key_Semicolon :
        case Qt::Key_N :
        case Qt::Key_M :
        case Qt::Key_Comma :
        case Qt::Key_Period :
        case Qt::Key_Slash :
            return true;
        }
        break;

    case Custom :
        return m_AccessKeyCustomSequence.contains(QKeySequence(k).toString());
    }
    return false;
}

bool Gadgets::IsValidAccessKey(QString s){
    //return QRegExp(QStringLiteral("[a-z;,\\./]")).exactMatch(s)
    //    || QRegExp(QStringLiteral("[A-Z:\\+<>\\?]")).exactMatch(s);

    switch(m_AccessKeyMode){

        // both hands' keys: alphabets.
    case BothHands :
        if(QRegExp(QStringLiteral("[a-z]")).exactMatch(s))
            return true;
        if(QRegExp(QStringLiteral("[A-Z]")).exactMatch(s))
            return true;
        break;

        // left hand's keys:
        // QWERT
        // ASDFG
        // ZXCVB
    case LeftHand :
        switch(s.at(0).toLatin1()){
        case 'Q' :
        case 'W' :
        case 'E' :
        case 'R' :
        case 'T' :
        case 'A' :
        case 'S' :
        case 'D' :
        case 'F' :
        case 'G' :
        case 'Z' :
        case 'X' :
        case 'C' :
        case 'V' :
        case 'B' :
        case 'q' :
        case 'w' :
        case 'e' :
        case 'r' :
        case 't' :
        case 'a' :
        case 's' :
        case 'd' :
        case 'f' :
        case 'g' :
        case 'z' :
        case 'x' :
        case 'c' :
        case 'v' :
        case 'b' :
            return true;
        }
        break;

        // right hand's keys:
        // YUIOP
        // HJKL;
        // NM,./
    case RightHand :
        switch(s.at(0).toLatin1()){
        case 'Y' :
        case 'U' :
        case 'I' :
        case 'O' :
        case 'P' :
        case 'H' :
        case 'J' :
        case 'K' :
        case 'L' :
        case ':' :
        case 'N' :
        case 'M' :
        case '<' :
        case '>' :
        case '?' :
        case 'y' :
        case 'u' :
        case 'i' :
        case 'o' :
        case 'p' :
        case 'h' :
        case 'j' :
        case 'k' :
        case 'l' :
        case ';' :
        case 'n' :
        case 'm' :
        case ',' :
        case '.' :
        case '/' :
            return true;
        }
        break;

    case Custom :
        return m_AccessKeyCustomSequence.contains(s);
    }
    return false;
}

int Gadgets::AccessKeyBlockSize(){
    switch(m_AccessKeyMode){
    case BothHands : return 26;
    case LeftHand :  return 15;
    case RightHand : return 15;
    case Custom :    return m_AccessKeyCustomSequence.length();
    }
    return 26;
}

QString Gadgets::IndexToString(int i){
    if(m_EnableMultiStroke){
        if(i < m_AccessKeyLabels.length())
            return m_AccessKeyLabels[i];
        return QString();
    }
    switch(m_AccessKeyMode){

        // both hands' keys: alphabets.
    case BothHands :
        return QString('A' + i % AccessKeyBlockSize());

        // left hand's keys:
        // QWERT
        // ASDFG
        // ZXCVB
    case LeftHand :
        switch(i % AccessKeyBlockSize()){
        case 0  : return QStringLiteral("Q");
        case 1  : return QStringLiteral("W");
        case 2  : return QStringLiteral("E");
        case 3  : return QStringLiteral("R");
        case 4  : return QStringLiteral("T");
        case 5  : return QStringLiteral("A");
        case 6  : return QStringLiteral("S");
        case 7  : return QStringLiteral("D");
        case 8  : return QStringLiteral("F");
        case 9  : return QStringLiteral("G");
        case 10 : return QStringLiteral("Z");
        case 11 : return QStringLiteral("X");
        case 12 : return QStringLiteral("C");
        case 13 : return QStringLiteral("V");
        case 14 : return QStringLiteral("B");
        }
        break;

        // right hand's keys:
        // YUIOP
        // HJKL;
        // NM,./
    case RightHand :
        switch(i % AccessKeyBlockSize()){
        case 0  : return QStringLiteral("Y");
        case 1  : return QStringLiteral("U");
        case 2  : return QStringLiteral("I");
        case 3  : return QStringLiteral("O");
        case 4  : return QStringLiteral("P");
        case 5  : return QStringLiteral("H");
        case 6  : return QStringLiteral("J");
        case 7  : return QStringLiteral("K");
        case 8  : return QStringLiteral("L");
        case 9  : return QStringLiteral(";");
        case 10 : return QStringLiteral("N");
        case 11 : return QStringLiteral("M");
        case 12 : return QStringLiteral(",");
        case 13 : return QStringLiteral(".");
        case 14 : return QStringLiteral("/");
        }
        break;

    case Custom :
        return QString(m_AccessKeyCustomSequence.at(i % AccessKeyBlockSize()));
    }
    return QString();
}

int Gadgets::KeyToIndex(int k){
    if(m_EnableMultiStroke) return -1;
    int n = m_CurrentAccessKeyBlockIndex * AccessKeyBlockSize();
    switch(m_AccessKeyMode){

        // both hands' keys: alphabets.
    case BothHands :
        return n + k - Qt::Key_A;

        // left hand's keys:
        // QWERT
        // ASDFG
        // ZXCVB
    case LeftHand :
        switch(k){
        case Qt::Key_Q :         return n + 0;
        case Qt::Key_W :         return n + 1;
        case Qt::Key_E :         return n + 2;
        case Qt::Key_R :         return n + 3;
        case Qt::Key_T :         return n + 4;
        case Qt::Key_A :         return n + 5;
        case Qt::Key_S :         return n + 6;
        case Qt::Key_D :         return n + 7;
        case Qt::Key_F :         return n + 8;
        case Qt::Key_G :         return n + 9;
        case Qt::Key_Z :         return n + 10;
        case Qt::Key_X :         return n + 11;
        case Qt::Key_C :         return n + 12;
        case Qt::Key_V :         return n + 13;
        case Qt::Key_B :         return n + 14;
        }
        break;

        // right hand's keys:
        // YUIOP
        // HJKL;
        // NM,./
    case RightHand :
        switch(k){
        case Qt::Key_Y :         return n + 0;
        case Qt::Key_U :         return n + 1;
        case Qt::Key_I :         return n + 2;
        case Qt::Key_O :         return n + 3;
        case Qt::Key_P :         return n + 4;
        case Qt::Key_H :         return n + 5;
        case Qt::Key_J :         return n + 6;
        case Qt::Key_K :         return n + 7;
        case Qt::Key_L :         return n + 8;
        case Qt::Key_Semicolon : return n + 9;
        case Qt::Key_N :         return n + 10;
        case Qt::Key_M :         return n + 11;
        case Qt::Key_Comma :     return n + 12;
        case Qt::Key_Period :    return n + 13;
        case Qt::Key_Slash :     return n + 14;
        }
        break;

    case Custom :
        return n + m_AccessKeyCustomSequence.indexOf(QKeySequence(k).toString());
    }
    return -1;
}

int Gadgets::KeyToIndex(QString s){
    if(m_EnableMultiStroke) return -1;
    int n = m_CurrentAccessKeyBlockIndex * AccessKeyBlockSize();
    switch(m_AccessKeyMode){

        // both hands' keys: alphabets.
    case BothHands :
        if(QRegExp(QStringLiteral("[a-z]")).exactMatch(s))
            return n + s.at(0).toLatin1() - 'a';
        if(QRegExp(QStringLiteral("[A-Z]")).exactMatch(s))
            return n + s.at(0).toLatin1() - 'A';
        break;

        // left hand's keys:
        // QWERT
        // ASDFG
        // ZXCVB
    case LeftHand :
        switch(s.at(0).toLatin1()){
        case 'Q' : return n + 0;
        case 'W' : return n + 1;
        case 'E' : return n + 2;
        case 'R' : return n + 3;
        case 'T' : return n + 4;
        case 'A' : return n + 5;
        case 'S' : return n + 6;
        case 'D' : return n + 7;
        case 'F' : return n + 8;
        case 'G' : return n + 9;
        case 'Z' : return n + 10;
        case 'X' : return n + 11;
        case 'C' : return n + 12;
        case 'V' : return n + 13;
        case 'B' : return n + 14;
        case 'q' : return n + 0;
        case 'w' : return n + 1;
        case 'e' : return n + 2;
        case 'r' : return n + 3;
        case 't' : return n + 4;
        case 'a' : return n + 5;
        case 's' : return n + 6;
        case 'd' : return n + 7;
        case 'f' : return n + 8;
        case 'g' : return n + 9;
        case 'z' : return n + 10;
        case 'x' : return n + 11;
        case 'c' : return n + 12;
        case 'v' : return n + 13;
        case 'b' : return n + 14;
        }
        break;

        // right hand's keys:
        // YUIOP
        // HJKL;
        // NM,./
    case RightHand :
        switch(s.at(0).toLatin1()){
        case 'Y' : return n + 0;
        case 'U' : return n + 1;
        case 'I' : return n + 2;
        case 'O' : return n + 3;
        case 'P' : return n + 4;
        case 'H' : return n + 5;
        case 'J' : return n + 6;
        case 'K' : return n + 7;
        case 'L' : return n + 8;
        case ':' : case '+' : return n + 9;
        case 'N' : return n + 10;
        case 'M' : return n + 11;
        case '<' : return n + 12;
        case '>' : return n + 13;
        case '?' : return n + 14;
        case 'y' : return n + 0;
        case 'u' : return n + 1;
        case 'i' : return n + 2;
        case 'o' : return n + 3;
        case 'p' : return n + 4;
        case 'h' : return n + 5;
        case 'j' : return n + 6;
        case 'k' : return n + 7;
        case 'l' : return n + 8;
        case ';' : return n + 9;
        case 'n' : return n + 10;
        case 'm' : return n + 11;
        case ',' : return n + 12;
        case '.' : return n + 13;
        case '/' : return n + 14;
        }
        break;

    case Custom :
        return n + m_AccessKeyCustomSequence.indexOf(s);
    }
    return -1;
}

bool Gadgets::IsCurrentBlock(const AccessibleWebElement *elem) const{
    if(m_EnableMultiStroke) return true;
    foreach(QGraphicsItem *item, scene()->selectedItems()){
        if(AccessibleWebElement *selected = dynamic_cast<AccessibleWebElement*>(item)){
            return selected == elem;
        }
    }
    return elem->GetIndex() / AccessKeyBlockSize() == m_CurrentAccessKeyBlockIndex;
}

QMenu *Gadgets::CreateNodeMenu(){
    if(!IsDisplayingNode()) return 0;

    QMenu *menu = new QMenu(View::m_TreeBank);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(Ge_Deactivate));
    menu->addAction(Action(Ge_Refresh));
    menu->addSeparator();

    if(m_HoveredItemIndex != -1 &&
       m_HoveredItemIndex < m_DisplayThumbnails.length()){

        menu->addAction(Action(Ge_OpenNode));
        menu->addAction(Action(Ge_RenameNode));
        menu->addAction(Action(Ge_CopyNodeUrl));
        menu->addAction(Action(Ge_CloneNode));

        QMenu *m = new QMenu(tr("OpenNodeWithOtherBrowser"));
        if(!Application::BrowserPath_IE().isEmpty())       m->addAction(Action(Ge_OpenNodeWithIE));
        if(!Application::BrowserPath_FF().isEmpty())       m->addAction(Action(Ge_OpenNodeWithFF));
        if(!Application::BrowserPath_Opera().isEmpty())    m->addAction(Action(Ge_OpenNodeWithOpera));
        if(!Application::BrowserPath_OPR().isEmpty())      m->addAction(Action(Ge_OpenNodeWithOPR));
        if(!Application::BrowserPath_Safari().isEmpty())   m->addAction(Action(Ge_OpenNodeWithSafari));
        if(!Application::BrowserPath_Chrome().isEmpty())   m->addAction(Action(Ge_OpenNodeWithChrome));
        if(!Application::BrowserPath_Sleipnir().isEmpty()) m->addAction(Action(Ge_OpenNodeWithSleipnir));
        if(!Application::BrowserPath_Vivaldi().isEmpty())  m->addAction(Action(Ge_OpenNodeWithVivaldi));
        if(!Application::BrowserPath_Custom().isEmpty())   m->addAction(Action(Ge_OpenNodeWithCustom));
        menu->addMenu(m);

        Node *nd = GetHoveredNode();
        if((nd->IsViewNode() && nd->IsDirectory()) ||
           (nd->IsHistNode() && !nd->HasNoChildren())){

            menu->addAction(Action(Ge_DownDirectory));
        }
    }
    if(m_CurrentNode &&
       m_CurrentNode->GetParent() &&
       m_CurrentNode->GetParent()->GetParent()){

        menu->addAction(Action(Ge_UpDirectory));
    }
    if((m_HoveredItemIndex != -1 &&
        m_HoveredItemIndex < m_DisplayThumbnails.length()) ||
       !m_NodesRegister.isEmpty()){

        menu->addAction(Action(Ge_DeleteNode));
        menu->addAction(Action(Ge_DeleteRightNode));
        menu->addAction(Action(Ge_DeleteLeftNode));
        menu->addAction(Action(Ge_DeleteOtherNode));
        if(!m_NodesRegister.isEmpty())
            menu->addAction(Action(Ge_PasteNode));
    }
    menu->addAction(Action(Ge_MakeDirectory));
    menu->addAction(Action(Ge_MakeDirectoryWithSelectedNode));
    menu->addAction(Action(Ge_MakeDirectoryWithSameDomainNode));
    menu->addAction(Action(Ge_RestoreNode));
    menu->addAction(Action(Ge_NewNode));
    menu->addAction(Action(Ge_ToggleTrash));

    return menu;
}

void Gadgets::RenderBackground(QPainter *painter){
    View *view = 0;

    if(m_CurrentNode){
        if((m_CurrentNode->IsHistNode() && m_CurrentNode->ToHistNode() == GetTreeBank()->GetCurrentHistNode()) ||
           (m_CurrentNode->IsViewNode() && m_CurrentNode->ToViewNode() == GetTreeBank()->GetCurrentViewNode())){

            /* do nothing. */

        } else if(m_CurrentNode->GetView() &&
                  m_CurrentNode->GetView()->IsRenderable()){

            view = m_CurrentNode->GetView();

        } else if(SharedView master = GetMaster().lock()){
            if(master->IsRenderable())
                view = master.get();
        }
    } else if(SharedView master = GetMaster().lock()){
        if(master->IsRenderable())
            view = master.get();
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

void Gadgets::keyPressEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return;

    if(IsDisplayingNode()){

        if(Application::HasAnyModifier(ev) ||
           Application::IsFunctionKey(ev) ||
           ev->modifiers() & Qt::ShiftModifier){

            ev->setAccepted(TriggerKeyEvent(ev));
            return;
        }
        QGraphicsObject::keyPressEvent(ev);

        if(!ev->isAccepted() &&
           !Application::IsOnlyModifier(ev)){

            ev->setAccepted(TriggerKeyEvent(ev));
        }
        return;
    }
    if(IsDisplayingAccessKey()){
        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        QKeySequence seq = Application::MakeKeySequence(ev);

        if(m_ThumbListKeyMap.contains(seq)){
            if(m_ThumbListKeyMap[seq] == QStringLiteral("OpenQueryEditor") ||
               m_ThumbListKeyMap[seq] == QStringLiteral("OpenUrlEditor") ||
               m_ThumbListKeyMap[seq] == QStringLiteral("OpenTextSeeker")){

                Deactivate();
                ev->setAccepted(TriggerKeyEvent(ev));
                return;

            } else if(m_ThumbListKeyMap[seq] == QStringLiteral("OpenCommand")){

                ev->setAccepted(TriggerKeyEvent(ev));
                return;
            }
        }

        QList<QGraphicsItem*> items = scene()->selectedItems();
        AccessibleWebElement *awe = 0;

        if(items.length() == 1){
            if(ev->key() == Qt::Key_Escape){
                AccessKey_ClearSelection();
            } else if(master->IsRenderable() &&
                      (awe = dynamic_cast<AccessibleWebElement*>(items.first()))){
                AccessKey_TriggerAction(awe, seq);
            }
        } else if(ev->key() == Qt::Key_Escape){

            Deactivate();

        } else if(m_EnableMultiStroke){

            if(IsValidAccessKey(ev->key())){
                m_AccessKeyCurrentSelection += ev->text().toUpper();
            } else if(m_AccessKeyCurrentSelection.length() >= 1){
                int len = m_AccessKeyCurrentSelection.length();
                m_AccessKeyCurrentSelection = m_AccessKeyCurrentSelection.left(len-1);
            } else {
                Deactivate();
                return;
            }
            AccessKey_SieveLabel();

        } else if(ev->key() == Qt::Key_Up ||
                  ev->key() == Qt::Key_Left ||
                  ev->key() == Qt::Key_PageUp ||
                  ev->key() == Qt::Key_Backtab ||
                  ev->key() == Qt::Key_Backspace ||
                  (ev->key() == Qt::Key_Space &&
                   ev->modifiers() == Qt::ShiftModifier)){

            AccessKey_PrevBlock();

        } else if(ev->key() == Qt::Key_Down ||
                  ev->key() == Qt::Key_Right ||
                  ev->key() == Qt::Key_PageDown ||
                  ev->key() == Qt::Key_Tab ||
                  ev->key() == Qt::Key_Space){

            AccessKey_NextBlock();

        } else if(ev->key() == Qt::Key_Home){

            AccessKey_FirstBlock();

        } else if(ev->key() == Qt::Key_End){

            AccessKey_LastBlock();

        } else if(m_AccessKeySelectBlockMethod == Number &&
                  Qt::Key_0 <= ev->key() && ev->key() <= Qt::Key_9){

            AccessKey_NthBlock(ev->key() - Qt::Key_0);

        } else if(m_AccessKeySelectBlockMethod == ShiftedChar &&
                  ev->modifiers() & Qt::ShiftModifier){

            m_CurrentAccessKeyBlockIndex = 0; // KeyToIndex uses this value.
            AccessKey_NthBlock(KeyToIndex(ev->key()));

        } else if(m_AccessKeySelectBlockMethod == CtrledChar &&
                  ev->modifiers() & Qt::ControlModifier){

            m_CurrentAccessKeyBlockIndex = 0; // KeyToIndex uses this value.
            AccessKey_NthBlock(KeyToIndex(ev->key()));

        } else if(m_AccessKeySelectBlockMethod == AltedChar &&
                  ev->modifiers() & Qt::AltModifier){

            m_CurrentAccessKeyBlockIndex = 0; // KeyToIndex uses this value.
            AccessKey_NthBlock(KeyToIndex(ev->key()));

        } else if(m_AccessKeySelectBlockMethod == MetaChar &&
                  ev->modifiers() & Qt::MetaModifier){

            m_CurrentAccessKeyBlockIndex = 0; // KeyToIndex uses this value.
            AccessKey_NthBlock(KeyToIndex(ev->key()));

        } else if(IsValidAccessKey(ev->key())){

            AccessKey_OpenElement(KeyToIndex(ev->key()));
        }
        ev->setAccepted(true);
    }
}

void Gadgets::keyReleaseEvent(QKeyEvent *ev){
    Q_UNUSED(ev);
    // do nothing.
}

void Gadgets::dragEnterEvent(QGraphicsSceneDragDropEvent *ev){
    GraphicsTableView::dragEnterEvent(ev);
}

void Gadgets::dropEvent(QGraphicsSceneDragDropEvent *ev){
    GraphicsTableView::dropEvent(ev);
}

void Gadgets::dragMoveEvent(QGraphicsSceneDragDropEvent *ev){
    GraphicsTableView::dragMoveEvent(ev);
}

void Gadgets::dragLeaveEvent(QGraphicsSceneDragDropEvent *ev){
    GraphicsTableView::dragLeaveEvent(ev);
}

void Gadgets::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(m_MouseMap.contains(mouse)){

        QString str = m_MouseMap[mouse];
        if(!str.isEmpty()){

            if(!TriggerAction(str)){
                ev->setAccepted(false);
                return;
            }
            ev->setAccepted(true);
            return;
        }
    }

    GraphicsTableView::mousePressEvent(ev);
    ev->setAccepted(true);
}

void Gadgets::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    GraphicsTableView::mouseReleaseEvent(ev);
}

void Gadgets::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    GraphicsTableView::mouseMoveEvent(ev);
}

void Gadgets::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev){
    GraphicsTableView::mouseDoubleClickEvent(ev);
}

void Gadgets::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    GraphicsTableView::hoverEnterEvent(ev);
}

void Gadgets::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    GraphicsTableView::hoverLeaveEvent(ev);
}

void Gadgets::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    GraphicsTableView::hoverMoveEvent(ev);
}

void Gadgets::contextMenuEvent(QGraphicsSceneContextMenuEvent *ev){
    GraphicsTableView::contextMenuEvent(ev);
}

void Gadgets::wheelEvent(QGraphicsSceneWheelEvent *ev){
    if(!IsDisplayingNode()) return;

    QString wheel;
    bool up = ev->delta() > 0;
    bool ignoreStatusBarMessage = true;

    Application::AddModifiersToString(wheel, ev->modifiers());
    Application::AddMouseButtonsToString(wheel, ev->buttons());
    Application::AddWheelDirectionToString(wheel, up);

    if(m_MouseMap.contains(wheel)){

        QString str = m_MouseMap[wheel];
        if(!str.isEmpty()){

            // don't want to overwrite statusBarMessage in this event.
            // because these method may update statusBarMessage.
            if(!TriggerAction(str)){
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

void Gadgets::focusInEvent(QFocusEvent *ev){
    GraphicsTableView::focusInEvent(ev);
    OnFocusIn();
}

void Gadgets::focusOutEvent(QFocusEvent *ev){
    GraphicsTableView::focusOutEvent(ev);
    OnFocusOut();
}

bool Gadgets::TriggerAction(QString str, QVariant data){
    Q_UNUSED(data);
    if(IsValidAction(str))
        TriggerAction(StringToAction(str));
    else return false;
    return true;
}

void Gadgets::TriggerAction(GadgetsAction a){
    Action(a)->trigger();
}

QAction *Gadgets::Action(GadgetsAction a){
    // forbid many times call of same action.
    static const QList<GadgetsAction> exclude = QList<GadgetsAction>()
        << Ge_NoAction << Ke_End
        << Ke_Up       << Ke_PageUp
        << Ke_Down     << Ke_PageDown << Ge_SelectAll
        << Ke_Right                   << Ge_SwitchWindow
        << Ke_Left                    << Ge_NextWindow
        << Ke_Home                    << Ge_PrevWindow
        << Ge_MoveToUpperItem   << Ge_SelectToLastItem
        << Ge_MoveToLowerItem   << Ge_SelectItem
        << Ge_MoveToRightItem   << Ge_SelectRange
        << Ge_MoveToLeftItem    << Ge_SelectAll
        << Ge_MoveToPrevPage    << Ge_ClearSelection
        << Ge_MoveToNextPage    << Ge_TransferToUpper
        << Ge_MoveToFirstItem   << Ge_TransferToLower
        << Ge_MoveToLastItem    << Ge_TransferToRight
        << Ge_SelectToUpperItem << Ge_TransferToLeft
        << Ge_SelectToLowerItem << Ge_TransferToPrevPage
        << Ge_SelectToRightItem << Ge_TransferToNextPage
        << Ge_SelectToLeftItem  << Ge_TransferToFirst
        << Ge_SelectToPrevPage  << Ge_TransferToLast
        << Ge_SelectToNextPage  << Ge_SwitchNodeCollectionType
        << Ge_SelectToFirstItem << Ge_SwitchNodeCollectionTypeReverse;
    static GadgetsAction previousAction = Ge_NoAction;
    static int sameActionCount = 0;
    if(exclude.contains(a)){
        sameActionCount = 0;
        previousAction = Ge_NoAction;
    } else if(a == previousAction){
        if(++sameActionCount > MAX_SAME_ACTION_COUNT)
            a = Ge_NoAction;
    } else {
        sameActionCount = 0;
        previousAction = a;
    }

    QAction *action = m_ActionTable[a];
    if(action) return action;

    m_ActionTable[a] = action = new QAction(this);

    switch(a){
    case Ke_Up:      action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowUp));       break;
    case Ke_Down:    action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowDown));     break;
    case Ke_Right:   action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowRight));    break;
    case Ke_Left:    action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowLeft));     break;
  //case Ge_Back:    action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowBack));     break;
  //case Ge_Forward: action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowForward));  break;
  //case Ge_Reload:  action->setIcon(Application::style()->standardIcon(QStyle::SP_BrowserReload)); break;
  //case Ge_Stop:    action->setIcon(Application::style()->standardIcon(QStyle::SP_BrowserStop));   break;
    }

    switch(a){
    case Ge_NoAction: break;

#define DEFINE_ACTION(name, text)                                       \
        case Ke_##name:                                                 \
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
        case Ge_##name:                                                 \
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
        DEFINE_ACTION(NewViewNode,        tr("NewViewNode"));
        DEFINE_ACTION(NewHistNode,        tr("NewHistNode"));
        DEFINE_ACTION(CloneViewNode,      tr("CloneViewNode"));
        DEFINE_ACTION(CloneHistNode,      tr("CloneHistNode"));
        DEFINE_ACTION(DisplayAccessKey,   tr("DisplayAccessKey"));
        DEFINE_ACTION(DisplayViewTree,    tr("DisplayViewTree"));
        DEFINE_ACTION(DisplayHistTree,    tr("DisplayHistTree"));
        DEFINE_ACTION(DisplayTrashTree,   tr("DisplayTrashTree"));
        DEFINE_ACTION(OpenTextSeeker,     tr("OpenTextSeeker"));
        DEFINE_ACTION(OpenQueryEditor,    tr("OpenQueryEditor"));
        DEFINE_ACTION(OpenUrlEditor,      tr("OpenUrlEditor"));
        DEFINE_ACTION(OpenCommand,        tr("OpenCommand"));
      //DEFINE_ACTION(Load,               tr("Load"));

#undef  DEFINE_ACTION
#define DEFINE_ACTION(name, text)                                       \
        case Ge_##name:                                                 \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name()));                              \
            break;

        // gadgets events.
        DEFINE_ACTION(Deactivate, tr("Deactivate"));

#undef  DEFINE_ACTION
#define DEFINE_ACTION(name, text)                                       \
        case Ge_##name:                                                 \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(ThumbList_##name()));                  \
            break;

        // thumblist events.
        DEFINE_ACTION(Refresh,                         tr("Refresh"));
        DEFINE_ACTION(RefreshNoScroll,                 tr("RefreshNoScroll"));
        DEFINE_ACTION(OpenNode,                        tr("OpenNode"));
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
    case Ge_OpenNodeWithIE:
        action->setIcon(Application::BrowserIcon_IE());
        break;
    case Ge_OpenNodeWithFF:
        action->setIcon(Application::BrowserIcon_FF());
        break;
    case Ge_OpenNodeWithOpera:
        action->setIcon(Application::BrowserIcon_Opera());
        break;
    case Ge_OpenNodeWithOPR:
        action->setIcon(Application::BrowserIcon_OPR());
        break;
    case Ge_OpenNodeWithSafari:
        action->setIcon(Application::BrowserIcon_Safari());
        break;
    case Ge_OpenNodeWithChrome:
        action->setIcon(Application::BrowserIcon_Chrome());
        break;
    case Ge_OpenNodeWithSleipnir:
        action->setIcon(Application::BrowserIcon_Sleipnir());
        break;
    case Ge_OpenNodeWithVivaldi:
        action->setIcon(Application::BrowserIcon_Vivaldi());
        break;
    case Ge_OpenNodeWithCustom:
        action->setIcon(Application::BrowserIcon_Custom());
        action->setText(tr("OpenNodeWith%1").arg(Application::BrowserPath_Custom().split("/").last().replace(".exe", "")));
        break;
    }
    return action;
}

void Gadgets::KeyPressEvent(QKeyEvent *ev){
    keyPressEvent(ev);
}

void Gadgets::KeyReleaseEvent(QKeyEvent *ev){
    keyReleaseEvent(ev);
}

void Gadgets::MousePressEvent(QMouseEvent *ev){
    View::m_TreeBank->MousePressEvent(ev);
}

void Gadgets::MouseReleaseEvent(QMouseEvent *ev){
    View::m_TreeBank->MouseReleaseEvent(ev);
}

void Gadgets::MouseMoveEvent(QMouseEvent *ev){
    View::m_TreeBank->MouseMoveEvent(ev);
}

void Gadgets::MouseDoubleClickEvent(QMouseEvent *ev){
    View::m_TreeBank->MouseDoubleClickEvent(ev);
}

void Gadgets::WheelEvent(QWheelEvent *ev){
    View::m_TreeBank->WheelEvent(ev);
}

void Gadgets::AccessKey_TriggerElementAction(Page::CustomAction action){
    SharedView master = GetMaster().lock();
    if(!master) return;
    QList<QGraphicsItem*> items = scene()->selectedItems();
    if(items.length() != 1) return;
    AccessibleWebElement *awe = dynamic_cast<AccessibleWebElement*>(items.first());
    if(!awe) return;
    SharedWebElement elem = awe->GetElement();
    if(!elem) return;
    Deactivate();
    elem->SetFocus();
    master->TriggerAction(action, QVariant::fromValue(elem));
}

void Gadgets::AccessKey_OpenElement(int index){
    if(index < 0 || index >= m_AccessibleWebElementCache.length())
        return;

    AccessibleWebElement *awe = m_AccessibleWebElementCache[index];
    // need to capture before 'Deactivate'.
    SharedWebElement elem = awe->GetElement();

    if(!elem) return;

    if(elem->GetAction() == WebElement::Focus){

        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        // 'Deactivate' clears 'm_Master' and 'm_Slave'.
        Deactivate();
        elem->SetFocus();
        master->TriggerAction(Page::We_FocusElement, QVariant::fromValue(elem));

    } else if(elem->GetAction() == WebElement::Click){

        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        // 'Deactivate' clears 'm_Master' and 'm_Slave'.
        Deactivate();
        elem->SetFocus();
        master->TriggerAction(Page::We_ClickElement, QVariant::fromValue(elem));

    } else if(elem->GetAction() == WebElement::Hover){

        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        // 'Deactivate' clears 'm_Master' and 'm_Slave'.
        Deactivate();
        elem->SetFocus();
        master->TriggerAction(Page::We_HoverElement, QVariant::fromValue(elem));

    } else if(m_AccessKeyAction == OpenMenu){
        QList<QRectF> list;

        list << awe->boundingRect();

        awe->setSelected(true);

        list << awe->boundingRect();

        foreach(QRectF rect, list){
            Update(rect);
        }
        foreach(AccessibleWebElement *e, m_AccessibleWebElementCache){
            e->UpdateMinimal();
        }
    } else {
        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        QVariant var = QVariant::fromValue(elem);
        // 'Deactivate' clears 'm_Master' and 'm_Slave'.
        Deactivate();
        switch(m_AccessKeyAction){
        case ClickElement:                 master->TriggerAction(Page::We_ClickElement, var); break;
        case FocusElement:                 master->TriggerAction(Page::We_FocusElement, var); break;
        case HoverElement:                 master->TriggerAction(Page::We_HoverElement, var); break;
        case OpenInNewViewNode:            master->TriggerAction(Page::We_OpenInNewViewNode, var); break;
        case OpenInNewHistNode:            master->TriggerAction(Page::We_OpenInNewHistNode, var); break;
        case OpenInNewDirectory:           master->TriggerAction(Page::We_OpenInNewDirectory, var); break;
        case OpenOnRoot:                   master->TriggerAction(Page::We_OpenOnRoot, var); break;
        case OpenInNewViewNodeBackground:  master->TriggerAction(Page::We_OpenInNewViewNodeBackground, var); break;
        case OpenInNewHistNodeBackground:  master->TriggerAction(Page::We_OpenInNewHistNodeBackground, var); break;
        case OpenInNewDirectoryBackground: master->TriggerAction(Page::We_OpenInNewDirectoryBackground, var); break;
        case OpenOnRootBackground:         master->TriggerAction(Page::We_OpenOnRootBackground, var); break;
        case OpenInNewViewNodeNewWindow:   master->TriggerAction(Page::We_OpenInNewViewNodeNewWindow, var); break;
        case OpenInNewHistNodeNewWindow:   master->TriggerAction(Page::We_OpenInNewHistNodeNewWindow, var); break;
        case OpenInNewDirectoryNewWindow:  master->TriggerAction(Page::We_OpenInNewDirectoryNewWindow, var); break;
        case OpenOnRootNewWindow:          master->TriggerAction(Page::We_OpenOnRootNewWindow, var); break;
        }
    }
}

void Gadgets::AccessKey_ClearSelection(){
    QList<QRectF> list;

    foreach(QGraphicsItem *item, scene()->selectedItems()){
        list << item->boundingRect();

        item->setSelected(false);

        list << item->boundingRect();
    }

    foreach(QRectF rect, list){
        Update(rect);
    }

    foreach(AccessibleWebElement *elem, m_AccessibleWebElementCache){
        elem->UpdateMinimal();
    }

    if(m_EnableMultiStroke){
        m_AccessKeyCurrentSelection = QString();
        AccessKey_SieveLabel();
    }
}

void Gadgets::AccessKey_TriggerAction(AccessibleWebElement *awe, QKeySequence seq){
    // need to capture before 'Deactivate'.
    SharedWebElement elem = awe->GetElement();

    if(!elem) return;

    if(m_AccessKeyKeyMap.contains(seq)){
        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        // 'Deactivate' clears 'm_Master' and 'm_Slave'.
        Deactivate();
        elem->SetFocus();
        master->TriggerAction(Page::StringToAction(m_AccessKeyKeyMap[seq]),
                              QVariant::fromValue(elem));
    }
}

void Gadgets::AccessKey_NextBlock(){
    AccessKey_ClearSelection();

    m_CurrentAccessKeyBlockIndex += 1;
    if(m_CurrentAccessKeyBlockIndex > m_AccessKeyLastBlockIndex)
        m_CurrentAccessKeyBlockIndex = m_AccessKeyLastBlockIndex;

    foreach(AccessibleWebElement *elem, m_AccessibleWebElementCache){
        if(GetStyle()->UseGraphicsItemUpdate())
            elem->update();
        else
            elem->UpdateMinimal();
    }
}

void Gadgets::AccessKey_PrevBlock(){
    AccessKey_ClearSelection();

    m_CurrentAccessKeyBlockIndex -= 1;
    if(m_CurrentAccessKeyBlockIndex < 0)
        m_CurrentAccessKeyBlockIndex = 0;

    foreach(AccessibleWebElement *elem, m_AccessibleWebElementCache){
        if(GetStyle()->UseGraphicsItemUpdate())
            elem->update();
        else
            elem->UpdateMinimal();
    }
}

void Gadgets::AccessKey_FirstBlock(){
    AccessKey_ClearSelection();

    m_CurrentAccessKeyBlockIndex = 0;

    foreach(AccessibleWebElement *elem, m_AccessibleWebElementCache){
        if(GetStyle()->UseGraphicsItemUpdate())
            elem->update();
        else
            elem->UpdateMinimal();
    }
}

void Gadgets::AccessKey_LastBlock(){
    AccessKey_ClearSelection();

    m_CurrentAccessKeyBlockIndex = m_AccessKeyLastBlockIndex;

    foreach(AccessibleWebElement *elem, m_AccessibleWebElementCache){
        if(GetStyle()->UseGraphicsItemUpdate())
            elem->update();
        else
            elem->UpdateMinimal();
    }
}

void Gadgets::AccessKey_NthBlock(int n){
    AccessKey_ClearSelection();

    if(0 <= n && n <= m_AccessKeyLastBlockIndex)
        m_CurrentAccessKeyBlockIndex = n;

    foreach(AccessibleWebElement *elem, m_AccessibleWebElementCache){
        if(GetStyle()->UseGraphicsItemUpdate())
            elem->update();
        else
            elem->UpdateMinimal();
    }
}

void Gadgets::AccessKey_SieveLabel(){
    for(int i = 0; i < m_AccessKeyCount; i++){

        if(m_AccessKeyLabels[i].startsWith(m_AccessKeyCurrentSelection))
            m_AccessibleWebElementCache[i]->show();
        else
            m_AccessibleWebElementCache[i]->hide();

        if(m_AccessKeyLabels[i] == m_AccessKeyCurrentSelection)
            AccessKey_OpenElement(i);
    }
    emit statusBarMessage(tr("Current selection: %1").arg(m_AccessKeyCurrentSelection));
}

AccessKeyBlockLabel::AccessKeyBlockLabel(QGraphicsItem *parent, int index, bool isNumber)
    : QGraphicsRectItem(parent)
{
    m_Index = index;
    m_IsNumber = isNumber;
}

AccessKeyBlockLabel::~AccessKeyBlockLabel(){
}

void AccessKeyBlockLabel::paint(QPainter *painter,
                                const QStyleOptionGraphicsItem *item, QWidget *widget){
    Q_UNUSED(item); Q_UNUSED(widget);

    painter->setBrush(Qt::NoBrush);
    painter->setFont(ACCESSKEY_CHAR_CHIP_L_FONT);

    painter->setPen(QColor(255,255,255,200));
    painter->drawText(boundingRect(), Qt::AlignCenter,
                      m_IsNumber ?
                        QStringLiteral("%1").arg(m_Index) :
                        QStringLiteral("%1").arg(static_cast<Gadgets*>(parentObject())->IndexToString(m_Index)));

    painter->setPen(QColor(255,255,255,100));
    painter->drawRect(boundingRect());
}
