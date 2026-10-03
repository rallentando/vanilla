#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "keymap.hpp"
#include "mousemap.hpp"

#include "gadgets.hpp"
#include "inputmap.hpp"

#include <QtCore>
#include <QGraphicsObject>
#include <QSettings>
#include <QVector>
#include <QAction>
#include <QtWidgets>
#include <QStyle>
#include <QNetworkRequest>
#include <QLatin1Char>

#include <functional>

#include "graphicstableview.hpp"
#include "gadgetsstyle.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "treebank.hpp"
#include "directorypage.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "thumbnail.hpp"
#include "nodetitle.hpp"
#include "accessiblewebelement.hpp"
#include "view.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "quicknativewebview.hpp"
#include "edgewebview.hpp"

#include "lightnode.hpp"

bool Gadgets::m_EnableMultiStroke = true;
QString Gadgets::m_AccessKeyCustomSequence = QString();
Gadgets::AccessKeyMode Gadgets::m_AccessKeyMode = BothHands;
Gadgets::AccessKeyAction Gadgets::m_AccessKeyAction = OpenMenu;
Gadgets::AccessKeySortOrientation Gadgets::m_AccessKeySortOrientation = Vertical;
Gadgets::AccessKeySelectBlockMethod Gadgets::m_AccessKeySelectBlockMethod = Number;

QMap<QKeySequence, QString> Gadgets::m_ThumbListKeyMap = QMap<QKeySequence, QString>();
QMap<QKeySequence, QString> Gadgets::m_AccessKeyKeyMap = QMap<QKeySequence, QString>();
QMap<QString, QString> Gadgets::m_MouseMap = QMap<QString, QString>();

Gadgets::Gadgets(TreeBank *parent)
    : GraphicsTableView(parent)
    , View(parent)
{
    m_ActionTable = QMap<GadgetsAction, QAction*>();

    m_ViewNodeCollectionType = DefaultNodeCollectionType();

    m_BackdropOwner = nullptr;
    m_BackdropRatio = 0.0;

    m_AccessKeyCount = 0;
    m_AccessKeyLastBlockIndex = 0;
    m_CurrentAccessKeyBlockIndex = 0;
    m_AccessKeyLabels = QStringList();
    m_AccessKeyCurrentSelection = QString();

    connect(this, &Gadgets::titleChanged, this, &Gadgets::OnTitleChanged);
    connect(this, &Gadgets::urlChanged,   this, &Gadgets::OnUrlChanged);

    PrepareAccessKeyBlockLabels();

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

    Settings &s = Application::GlobalSettings();

    {
        THUMBLIST_KEYMAP
        InputMap::LoadKeyMap(s, QStringLiteral("gadgets/thumblist/keymap"), m_ThumbListKeyMap, InputMap::Hooks(IsValidAction));
    }
    {
        THUMBLIST_MOUSEMAP
        InputMap::LoadGestureMap(s, QStringLiteral("gadgets/thumblist/mouse"), m_MouseMap, InputMap::Hooks(IsValidAction));
    }

    m_EnableMultiStroke = s.value(QStringLiteral("gadgets/accesskey/@EnableMultiStroke"), true).value<bool>();
    m_AccessKeyCustomSequence =
        InputMap::UnescapeKeyName(s.value(QStringLiteral("gadgets/accesskey/@AccessKeyCustomSequence"), QString()).value<QString>().toUpper());

    QString mode = s.value(QStringLiteral("gadgets/accesskey/@AccessKeyMode"), QStringLiteral("BothHands")).value<QString>();
    if     (mode == QStringLiteral("BothHands")) m_AccessKeyMode = BothHands;
    else if(mode == QStringLiteral("LeftHand"))  m_AccessKeyMode = LeftHand;
    else if(mode == QStringLiteral("RightHand")) m_AccessKeyMode = RightHand;
    else if(mode == QStringLiteral("Custom"))    m_AccessKeyMode = Custom;

    if(m_AccessKeyMode == Custom && m_AccessKeyCustomSequence.isEmpty())
        m_AccessKeyMode = BothHands;

    QString action = s.value(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenMenu")).value<QString>();
    if     (action == QStringLiteral("OpenMenu"))                     m_AccessKeyAction = OpenMenu;
    else if(action == QStringLiteral("ClickElement"))                 m_AccessKeyAction = ClickElement;
    else if(action == QStringLiteral("FocusElement"))                 m_AccessKeyAction = FocusElement;
    else if(action == QStringLiteral("HoverElement"))                 m_AccessKeyAction = HoverElement;
    else if(action == QStringLiteral("OpenInNewViewNode"))            m_AccessKeyAction = OpenInNewViewNode;
    else if(action == QStringLiteral("OpenInNewDirectory"))           m_AccessKeyAction = OpenInNewDirectory;
    else if(action == QStringLiteral("OpenOnRoot"))                   m_AccessKeyAction = OpenOnRoot;
    else if(action == QStringLiteral("OpenInNewViewNodeBackground"))  m_AccessKeyAction = OpenInNewViewNodeBackground;
    else if(action == QStringLiteral("OpenInNewDirectoryBackground")) m_AccessKeyAction = OpenInNewDirectoryBackground;
    else if(action == QStringLiteral("OpenOnRootBackground"))         m_AccessKeyAction = OpenOnRootBackground;
    else if(action == QStringLiteral("OpenInNewViewNodeNewWindow"))   m_AccessKeyAction = OpenInNewViewNodeNewWindow;
    else if(action == QStringLiteral("OpenInNewDirectoryNewWindow"))  m_AccessKeyAction = OpenInNewDirectoryNewWindow;
    else if(action == QStringLiteral("OpenOnRootNewWindow"))          m_AccessKeyAction = OpenOnRootNewWindow;

    QString orientation = s.value(QStringLiteral("gadgets/accesskey/@AccessKeySortOrientation"), QStringLiteral("Vertical")).value<QString>();
    if     (orientation == QStringLiteral("Vertical"))   m_AccessKeySortOrientation = Vertical;
    else if(orientation == QStringLiteral("Horizontal")) m_AccessKeySortOrientation = Horizontal;

    QString method = s.value(QStringLiteral("gadgets/accesskey/@AccessKeySelectBlockMethod"), QStringLiteral("Number")).value<QString>();
    if     (method == QStringLiteral("Number"))      m_AccessKeySelectBlockMethod = Number;
    else if(method == QStringLiteral("ShiftedChar")) m_AccessKeySelectBlockMethod = ShiftedChar;
    else if(method == QStringLiteral("CtrledChar"))  m_AccessKeySelectBlockMethod = CtrledChar;
    else if(method == QStringLiteral("AltedChar"))   m_AccessKeySelectBlockMethod = AltedChar;
    else if(method == QStringLiteral("MetaChar"))    m_AccessKeySelectBlockMethod = MetaChar;

    {
        ACCESSKEY_KEYMAP
        InputMap::LoadKeyMap(s, QStringLiteral("gadgets/accesskey/keymap"), m_AccessKeyKeyMap, InputMap::Hooks(Page::IsValidAction));
    }
}

void Gadgets::SaveSettings(){
    GraphicsTableView::SaveSettings();

    Settings &s = Application::GlobalSettings();

    InputMap::SaveKeyMap(s, QStringLiteral("gadgets/thumblist/keymap"), m_ThumbListKeyMap);

    InputMap::SaveGestureMap(s, QStringLiteral("gadgets/thumblist/mouse"), m_MouseMap);

    s.setValue(QStringLiteral("gadgets/accesskey/@EnableMultiStroke"), m_EnableMultiStroke);
    s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyCustomSequence"),
                InputMap::EscapeKeyName(m_AccessKeyCustomSequence));

    if     (m_AccessKeyMode == BothHands) s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyMode"), QStringLiteral("BothHands"));
    else if(m_AccessKeyMode == LeftHand)  s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyMode"), QStringLiteral("LeftHand"));
    else if(m_AccessKeyMode == RightHand) s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyMode"), QStringLiteral("RightHand"));
    else if(m_AccessKeyMode == Custom)    s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyMode"), QStringLiteral("Custom"));

    if     (m_AccessKeyAction == OpenMenu)                     s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenMenu"));
    else if(m_AccessKeyAction == ClickElement)                 s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("ClickElement"));
    else if(m_AccessKeyAction == FocusElement)                 s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("FocusElement"));
    else if(m_AccessKeyAction == HoverElement)                 s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("HoverElement"));
    else if(m_AccessKeyAction == OpenInNewViewNode)            s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenInNewViewNode"));
    else if(m_AccessKeyAction == OpenInNewDirectory)           s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenInNewDirectory"));
    else if(m_AccessKeyAction == OpenOnRoot)                   s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenOnRoot"));
    else if(m_AccessKeyAction == OpenInNewViewNodeBackground)  s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenInNewViewNodeBackground"));
    else if(m_AccessKeyAction == OpenInNewDirectoryBackground) s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenInNewDirectoryBackground"));
    else if(m_AccessKeyAction == OpenOnRootBackground)         s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenOnRootBackground"));
    else if(m_AccessKeyAction == OpenInNewViewNodeNewWindow)   s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenInNewViewNodeNewWindow"));
    else if(m_AccessKeyAction == OpenInNewDirectoryNewWindow)  s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenInNewDirectoryNewWindow"));
    else if(m_AccessKeyAction == OpenOnRootNewWindow)          s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenOnRootNewWindow"));

    if     (m_AccessKeySortOrientation == Vertical)   s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeySortOrientation"), QStringLiteral("Vertical"));
    else if(m_AccessKeySortOrientation == Horizontal) s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeySortOrientation"), QStringLiteral("Horizontal"));

    if     (m_AccessKeySelectBlockMethod == Number)      s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeySelectBlockMethod"), QStringLiteral("Number"));
    else if(m_AccessKeySelectBlockMethod == ShiftedChar) s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeySelectBlockMethod"), QStringLiteral("ShiftedChar"));
    else if(m_AccessKeySelectBlockMethod == CtrledChar)  s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeySelectBlockMethod"), QStringLiteral("CtrledChar"));
    else if(m_AccessKeySelectBlockMethod == AltedChar)   s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeySelectBlockMethod"), QStringLiteral("AltedChar"));
    else if(m_AccessKeySelectBlockMethod == MetaChar)    s.setValue(QStringLiteral("gadgets/accesskey/@AccessKeySelectBlockMethod"), QStringLiteral("MetaChar"));

    InputMap::SaveKeyMap(s, QStringLiteral("gadgets/accesskey/keymap"), m_AccessKeyKeyMap);
}

void Gadgets::ApplyNodeCollectionTypeSetting(){
    m_ViewNodeCollectionType = DefaultNodeCollectionType();
    if(isVisible() && IsDisplayingViewNode()) ThumbList_Refresh();
}

void Gadgets::Activate(DisplayType type){
    GraphicsTableView::Activate(type);
}

void Gadgets::Deactivate(){
    GraphicsTableView::Deactivate();

    m_Backdrop = QPixmap();
    m_BackdropOwner = nullptr;
    m_BackdropRatio = 0.0;

    QTimer::singleShot(0, this, &Gadgets::DisableAccessKey);

    if(GetTreeBank()){
        if(SharedView master = GetMaster().lock()){
            GetTreeBank()->SetCurrent(master);

            if(GetThis().lock() == master->GetSlave().lock())
                master->SetSlave(WeakView());
            SetMaster(WeakView());
        }
    }
}

void Gadgets::Dismiss(){
    GraphicsTableView::Deactivate();

    QTimer::singleShot(0, this, &Gadgets::DisableAccessKey);

    if(SharedView master = GetMaster().lock()){
        if(GetThis().lock() == master->GetSlave().lock())
            master->SetSlave(WeakView());
    }
    SetMaster(WeakView());
}

void Gadgets::DisableAccessKey(){
    if(IsDisplayingAccessKey()){
        foreach(AccessibleWebElement *link, m_AccessibleWebElementCache){
            link->SetElement(nullptr);
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
        connect(receiver, SIGNAL(OpenNodeWithCommand(QString)),
                this, SLOT(ThumbList_OpenNodeWithCommand(QString)));

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
        disconnect(receiver, SIGNAL(OpenNodeWithCommand(QString)),
                   this, SLOT(ThumbList_OpenNodeWithCommand(QString)));
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
    Q_UNUSED(flags)
    CollectNodes(m_CurrentNode, text);
    ThumbList_MoveToFirstItem();
    update();
}

void Gadgets::KeyEvent(QString str){
    if(IsDisplayingNode()){
        TriggerKeyEvent(str);
    } else if(IsDisplayingAccessKey()){
        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        QKeySequence seq = Application::MakeKeySequence(str);
        QList<QGraphicsItem*> items = scene()->selectedItems();
        AccessibleWebElement *awe = nullptr;

        if(items.length() == 1){
            if(Application::ExactMatch(QStringLiteral("[eE]sc(?:ape)?"), str)){
                AccessKey_ClearSelection();
            } else if(master->IsRenderable() &&
                      (awe = dynamic_cast<AccessibleWebElement*>(items.first()))){
                AccessKey_TriggerAction(awe, seq);
            }
        } else if(Application::ExactMatch(QStringLiteral("[eE]sc(?:ape)?"), str)){

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

        } else if(Application::ExactMatch(QStringLiteral("[uU]p"), str) ||
                  Application::ExactMatch(QStringLiteral("[lL]eft"), str) ||
                  Application::ExactMatch(QStringLiteral("[pP](?:g|age)?[uU]p"), str) ||
                  Application::ExactMatch(QStringLiteral("[bB]ack[tT]ab"), str) ||
                  Application::ExactMatch(QStringLiteral("[bB]ack[sS]pace"), str) ||
                  Application::ExactMatch(QStringLiteral("[sS]hift\\+[sS]pace"), str)){

            AccessKey_PrevBlock();

        } else if(Application::ExactMatch(QStringLiteral("[dD]own"), str) ||
                  Application::ExactMatch(QStringLiteral("[rR]ight"), str) ||
                  Application::ExactMatch(QStringLiteral("[pP](?:g|age)?[dD](?:ow)?n"), str) ||
                  Application::ExactMatch(QStringLiteral("[tT]ab"), str) ||
                  Application::ExactMatch(QStringLiteral("[sS]pace"), str)){

            AccessKey_NextBlock();

        } else if(Application::ExactMatch(QStringLiteral("[hH]ome"), str)){

            AccessKey_FirstBlock();

        } else if(Application::ExactMatch(QStringLiteral("[eE]nd"), str)){

            AccessKey_LastBlock();

        } else if(Application::ExactMatch(QStringLiteral("[0-9]+"), str)){

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
    Q_UNUSED(operation) Q_UNUSED(body)
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

bool Gadgets::IsDisplaying(DisplayType type){
    if(!IsActive()) return false;
    return m_DisplayType == type;
}

void Gadgets::SetCurrentView(View *view){
    if(m_DisplayType == AccessKey){
        SetMaster(view->GetThis());
        if(view){
            view->SetSlave(GetThis());
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
        << QStringLiteral("C")
        << QStringLiteral("X")
        << QStringLiteral("M")
        << QStringLiteral("V")
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

    QStringList customSeq;
    if(m_AccessKeyMode == Custom){
        customSeq = m_AccessKeyCustomSequence.split(QString());
        customSeq.removeAll(QString());
        if(customSeq.length() < 2)
            customSeq = bothHandsSeq;
    }

    QStringList seq =
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

void Gadgets::PrepareAccessKeyBlockLabels(){
    bool isNumber = m_AccessKeySelectBlockMethod == Number;
    int size = m_EnableMultiStroke ? 0 : isNumber ? 10 : AccessKeyBlockSize();

    if(m_AccessKeyBlockLabels.length() == size &&
       (size == 0 || m_AccessKeyBlockLabels.first()->IsNumber() == isNumber))
        return;

    while(!m_AccessKeyBlockLabels.isEmpty())
        delete m_AccessKeyBlockLabels.takeLast();

    for(int i = 0; i < size; i++){
        m_AccessKeyBlockLabels << new AccessKeyBlockLabel(this, i, isNumber);
    }
}

void Gadgets::CollectAccessibleWebElement(View *view){
    m_CurrentAccessKeyBlockIndex = 0;

    PrepareAccessKeyBlockLabels();

    foreach(AccessKeyBlockLabel *label, m_AccessKeyBlockLabels){
        label->setRect(QRectF());
    }

    foreach(QGraphicsItem *item, childItems()){
        item->setSelected(false);
        item->setVisible(false);
        item->setEnabled(false);
    }

    view->CallWithFoundElements
        (Page::ForAccessKey, [this](SharedWebElementList elems){
            if(m_DisplayType != AccessKey) return;
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

            std::sort(m_AccessibleWebElementCache.begin(),
                      m_AccessibleWebElementCache.end(),
                      m_AccessKeySortOrientation == Vertical   ? &WebElementVerticalLessThan :
                      m_AccessKeySortOrientation == Horizontal ? &WebElementHorizontalLessThan :
                      &WebElementVerticalLessThan);

            for(int i = 0; i < count; i++){
                AccessibleWebElement *link = m_AccessibleWebElementCache[i];

                link->SetIndex(i);

                int index = i / AccessKeyBlockSize();
                if(index >= m_AccessKeyBlockLabels.length()) continue;

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
    if(m_ViewNode){
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
                if(item->zValue() > SPOT_LIGHT_LAYER) break;

                item->paint(&painter, nullptr);
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
            m_ViewNode->SetImage(
                image.
                scaled(SAVING_THUMBNAIL_SIZE,
                       Qt::KeepAspectRatioByExpanding,
                       Qt::SmoothTransformation));
        } else {
            m_ViewNode->SetImage(
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

    if(!TriggerAction(str)){
        return false;
    }
    return true;
}

bool Gadgets::TriggerKeyEvent(QString str){
    QKeySequence seq = Application::MakeKeySequence(str);
    if(seq.isEmpty()) return false;
    str = m_ThumbListKeyMap[seq];
    if(str.isEmpty()) return false;

    if(!TriggerAction(str)){
        return false;
    }
    return true;
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

    switch(m_AccessKeyMode){

    case BothHands :
        return Qt::Key_A <= k && k <= Qt::Key_Z;

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

    switch(m_AccessKeyMode){

    case BothHands :
        if(Application::ExactMatch(QStringLiteral("[a-z]"), s))
            return true;
        if(Application::ExactMatch(QStringLiteral("[A-Z]"), s))
            return true;
        break;

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

QString Gadgets::IndexToString(int i) const {
    if(m_EnableMultiStroke){
        if(i < m_AccessKeyLabels.length())
            return m_AccessKeyLabels[i];
        return QString();
    }
    switch(m_AccessKeyMode){

    case BothHands :
        return QString(QLatin1Char('A' + i % AccessKeyBlockSize()));

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

int Gadgets::KeyToIndex(int k) const {
    if(m_EnableMultiStroke) return -1;
    int n = m_CurrentAccessKeyBlockIndex * AccessKeyBlockSize();
    switch(m_AccessKeyMode){

    case BothHands :
        return n + k - Qt::Key_A;

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

int Gadgets::KeyToIndex(QString s) const {
    if(m_EnableMultiStroke) return -1;
    int n = m_CurrentAccessKeyBlockIndex * AccessKeyBlockSize();
    switch(m_AccessKeyMode){

    case BothHands :
        if(Application::ExactMatch(QStringLiteral("[a-z]"), s))
            return n + s.at(0).toLatin1() - 'a';
        if(Application::ExactMatch(QStringLiteral("[A-Z]"), s))
            return n + s.at(0).toLatin1() - 'A';
        break;

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

bool Gadgets::IsCurrentBlock(const AccessibleWebElement *elem) const {
    if(m_EnableMultiStroke) return true;
    foreach(QGraphicsItem *item, scene()->selectedItems()){
        if(AccessibleWebElement *selected = dynamic_cast<AccessibleWebElement*>(item)){
            return selected == elem;
        }
    }
    return elem->GetIndex() / AccessKeyBlockSize() == m_CurrentAccessKeyBlockIndex;
}

QMenu *Gadgets::CreateNodeMenu(){
    if(!IsDisplayingNode()) return nullptr;

    QMenu *menu = new QMenu(GetTreeBank());
    menu->setToolTipsVisible(true);

    menu->addAction(Action(_Deactivate));
    menu->addAction(Action(_Refresh));
    menu->addSeparator();

    if(m_HoveredItemIndex != -1 &&
       m_HoveredItemIndex < m_DisplayThumbnails.length()){

        menu->addAction(Action(_OpenNode));
        menu->addAction(Action(_OpenNodeOnNewWindow));
        menu->addAction(Action(_RenameNode));
        menu->addAction(Action(_CopyNodeUrl));
        menu->addAction(Action(_CloneNode));

        QMenu *m = new QMenu(tr("OpenNodeWithOtherBrowser"));
        m->addAction(Action(_OpenNodeWithDefault));
        const QList<QPair<QString, QString> > commands = Application::ExternalCommands();
        for(int i = 0; i < commands.length(); i++){
            QAction *action = m->addAction(commands[i].first, this,
                                           SLOT(ThumbList_OpenNodeWithCommand()));
            action->setProperty(EXTERNAL_COMMAND_PROPERTY, commands[i].first);
        }
        menu->addMenu(m);

        Node *nd = GetHoveredNode();
        if(nd->IsViewNode() && nd->IsDirectory()){

            menu->addAction(Action(_DownDirectory));

            TreeBank *tb = GetTreeBank();
            ViewNode *vn = nd->ToViewNode();
            menu->addMenu(DirectoryPage::CreateSettingsMenu
                          (vn,
                           [this, tb, vn](){
                               Deactivate();
                               tb->OpenDirectorySettings(vn);
                           },
                           menu));
        }
    }
    if(m_CurrentNode &&
       m_CurrentNode->GetParent() &&
       m_CurrentNode->GetParent()->GetParent()){

        menu->addAction(Action(_UpDirectory));
    }
    if((m_HoveredItemIndex != -1 &&
        m_HoveredItemIndex < m_DisplayThumbnails.length()) ||
       !m_NodesRegister.isEmpty()){

        menu->addAction(Action(_DeleteNode));
        menu->addAction(Action(_DeleteRightNode));
        menu->addAction(Action(_DeleteLeftNode));
        menu->addAction(Action(_DeleteOtherNode));
        if(!m_NodesRegister.isEmpty())
            menu->addAction(Action(_PasteNode));
    }
    menu->addAction(Action(_MakeDirectory));
    menu->addAction(Action(_MakeDirectoryWithSelectedNode));
    menu->addAction(Action(_MakeDirectoryWithSameDomainNode));
    menu->addAction(Action(_RestoreNode));
    menu->addAction(Action(_NewNode));
    menu->addAction(Action(_ToggleTrash));

    menu->addSeparator();
    menu->addMenu(CreateSortMenu(menu));

    if(IsDisplayingViewNode()){
        menu->addSeparator();
        menu->addMenu(CreateNodeCollectionTypeMenu(menu));
    }

    return menu;
}

void Gadgets::RenderBackground(QPainter *painter){
    bool nativeViewBehind = false;
#ifdef NATIVEWEBVIEW
    if(GetTreeBank()->GetCurrentView())
        nativeViewBehind = qobject_cast<QuickNativeWebView*>
            (GetTreeBank()->GetCurrentView()->base()) != nullptr;
#endif
#ifdef EDGEWEBVIEW
    if(!nativeViewBehind && GetTreeBank()->GetCurrentView())
        nativeViewBehind = qobject_cast<EdgeWebView*>
            (GetTreeBank()->GetCurrentView()->base()) != nullptr;
#endif

    View *view = nullptr;

    if(m_CurrentNode){
        if(m_CurrentNode->GetView() &&
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

    if(!nativeViewBehind &&
       (!view || view == GetTreeBank()->GetCurrentView().get())){
        GraphicsTableView::RenderBackground(painter);
        return;
    }

    if(view){
        if(!view->visible())
            view->SetViewportSize(Size().toSize());

        const QSize size = Size().toSize();
        const QSize viewport = view->GetViewportSize();

        const qreal ratio = painter->device()
            ? painter->device()->devicePixelRatioF() : 1.0;

        if(m_Backdrop.isNull() || m_BackdropOwner != view ||
           m_BackdropSize != size || m_BackdropViewport != viewport ||
           m_BackdropRatio != ratio){

            m_Backdrop = QPixmap(QSize(static_cast<int>(size.width()  * ratio),
                                       static_cast<int>(size.height() * ratio)));
            m_Backdrop.setDevicePixelRatio(ratio);
            m_Backdrop.fill(Qt::transparent);

            QPainter into(&m_Backdrop);
            const bool inPlace = view == GetTreeBank()->GetCurrentView().get();
            const int width_diff  = size.width()  - viewport.width();
            const int height_diff = size.height() - viewport.height();
            if(!inPlace && (width_diff != 0 || height_diff != 0))
                into.translate(width_diff / 2.0, height_diff / 2.0);
            view->Render(&into);
            into.end();

            m_BackdropOwner = view;
            m_BackdropSize = size;
            m_BackdropViewport = viewport;
            m_BackdropRatio = ratio;
        }

        painter->drawPixmap(QPointF(), m_Backdrop);
    }

    GraphicsTableView::RenderBackground(painter);
}

void Gadgets::keyPressEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return;

    if(IsDisplayingNode()){

        if(Application::HasAnyModifier(ev) ||
           Application::IsFunctionKey(ev) ||
           Application::HasShiftModifier(ev)){

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
        AccessibleWebElement *awe = nullptr;

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
                   Application::HasShiftModifier(ev))){

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
                  Application::HasShiftModifier(ev)){

            m_CurrentAccessKeyBlockIndex = 0;
            AccessKey_NthBlock(KeyToIndex(ev->key()));

        } else if(m_AccessKeySelectBlockMethod == CtrledChar &&
                  Application::HasCtrlModifier(ev)){

            m_CurrentAccessKeyBlockIndex = 0;
            AccessKey_NthBlock(KeyToIndex(ev->key()));

        } else if(m_AccessKeySelectBlockMethod == AltedChar &&
                  Application::HasAltModifier(ev)){

            m_CurrentAccessKeyBlockIndex = 0;
            AccessKey_NthBlock(KeyToIndex(ev->key()));

        } else if(m_AccessKeySelectBlockMethod == MetaChar &&
                  Application::HasMetaModifier(ev)){

            m_CurrentAccessKeyBlockIndex = 0;
            AccessKey_NthBlock(KeyToIndex(ev->key()));

        } else if(IsValidAccessKey(ev->key())){

            AccessKey_OpenElement(KeyToIndex(ev->key()));
        }
        ev->setAccepted(true);
    }
}

void Gadgets::keyReleaseEvent(QKeyEvent *ev){
    Q_UNUSED(ev)
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

    bool up = ev->delta() > 0;
    bool ignoreStatusBarMessage = true;

    if(GetTreeBank()->GetView()->MouseEventSource() != Qt::MouseEventSynthesizedBySystem){
        QString wheel;

        Application::AddModifiersToString(wheel, ev->modifiers());
        Application::AddMouseButtonsToString(wheel, ev->buttons());
        Application::AddWheelDirectionToString(wheel, up);

        if(m_MouseMap.contains(wheel)){

            QString str = m_MouseMap[wheel];
            if(!str.isEmpty()){

                if(!TriggerAction(str)){
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

void Gadgets::focusInEvent(QFocusEvent *ev){
    GraphicsTableView::focusInEvent(ev);
    OnFocusIn();
}

void Gadgets::focusOutEvent(QFocusEvent *ev){
    GraphicsTableView::focusOutEvent(ev);
    OnFocusOut();
}

bool Gadgets::TriggerAction(QString str, QVariant data){
    Q_UNUSED(data)
    if(IsValidAction(str))
        TriggerAction(StringToAction(str));
    else return false;
    return true;
}

void Gadgets::TriggerAction(GadgetsAction a){
    Action(a)->trigger();
}

QAction *Gadgets::Action(GadgetsAction a){
    static const QList<GadgetsAction> exclude = QList<GadgetsAction>()
        << _NoAction << _End
        << _Up       << _PageUp
        << _Down     << _PageDown << _SelectAll
        << _Right                 << _SwitchWindow
        << _Left                  << _NextWindow
        << _Home                  << _PrevWindow
        << _ScrollUp          << _SelectToNextPage
        << _ScrollDown        << _SelectToFirstItem
        << _NextPage          << _SelectToLastItem
        << _PrevPage          << _SelectItem
        << _MoveToUpperItem   << _SelectRange
        << _MoveToLowerItem   << _SelectAll
        << _MoveToRightItem   << _ClearSelection
        << _MoveToLeftItem    << _TransferToUpper
        << _MoveToPrevPage    << _TransferToLower
        << _MoveToNextPage    << _TransferToRight
        << _MoveToFirstItem   << _TransferToLeft
        << _MoveToLastItem    << _TransferToPrevPage
        << _SelectToUpperItem << _TransferToNextPage
        << _SelectToLowerItem << _TransferToFirst
        << _SelectToRightItem << _TransferToLast
        << _SelectToLeftItem  << _SwitchNodeCollectionType
        << _SelectToPrevPage  << _SwitchNodeCollectionTypeReverse;
    static GadgetsAction previousAction = _NoAction;
    static int sameActionCount = 0;
    if(exclude.contains(a)){
        sameActionCount = 0;
        previousAction = _NoAction;
    } else if(a == previousAction){
        if(++sameActionCount > MAX_SAME_ACTION_COUNT)
            a = _NoAction;
    } else {
        sameActionCount = 0;
        previousAction = a;
    }

    QAction *action = m_ActionTable[a];
    bool create = false;
    if(!action){
        create = true;
        m_ActionTable[a] = action = new QAction(this);
    }

    switch(a){
    case _Up:          action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowUp));       break;
    case _Down:        action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowDown));     break;
    case _Right:       action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowRight));    break;
    case _Left:        action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowLeft));     break;
    default: break;
    }

    if(!create){
        switch(a){
        case _ToggleNotifier: action->setChecked(GetTreeBank()->GetNotifier()); break;
        case _ToggleReceiver: action->setChecked(GetTreeBank()->GetReceiver()); break;
        case _ToggleMenuBar:  action->setChecked(!GetTreeBank()->GetMainWindow()->IsMenuBarEmpty()); break;
        case _ToggleTreeBar:  action->setChecked(GetTreeBank()->GetMainWindow()->GetTreeBar()->isVisible()); break;
        case _ToggleToolBar:  action->setChecked(GetTreeBank()->GetMainWindow()->GetToolBar()->isVisible()); break;
        default: break;
        }
        return action;
    }

    switch(a){
    case _NoAction: break;

#define DEFINE_ACTION(name, text)                                       \
        case _##name:                                                   \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name##Key()));                         \
            break

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
        case _##name:                                                   \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action,        SIGNAL(triggered()),                 \
                    GetTreeBank(), SLOT(name()));                       \
            break

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
        DEFINE_ACTION(NewViewNode,        tr("NewViewNode"));
        DEFINE_ACTION(CloneViewNode,      tr("CloneViewNode"));
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
        case _##name:                                                   \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name()));                              \
            break

        DEFINE_ACTION(Deactivate, tr("Deactivate"));

#undef  DEFINE_ACTION
#define DEFINE_ACTION(name, text)                                       \
        case _##name:                                                   \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(ThumbList_##name()));                  \
            break

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
    }
    switch(a){

    case _ToggleNotifier:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetNotifier());
        action->setText(tr("Notifier"));
        action->setToolTip(tr("Notifier"));
        break;
    case _ToggleReceiver:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetReceiver());
        action->setText(tr("Receiver"));
        action->setToolTip(tr("Receiver"));
        break;
    case _ToggleMenuBar:
        action->setCheckable(true);
        action->setChecked(!GetTreeBank()->GetMainWindow()->IsMenuBarEmpty());
        action->setText(tr("MenuBar"));
        action->setToolTip(tr("MenuBar"));
        break;
    case _ToggleTreeBar:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetMainWindow()->GetTreeBar()->isVisible());
        action->setText(tr("TreeBar"));
        action->setToolTip(tr("TreeBar"));
        break;
    case _ToggleToolBar:
        action->setCheckable(true);
        action->setChecked(GetTreeBank()->GetMainWindow()->GetToolBar()->isVisible());
        action->setText(tr("ToolBar"));
        action->setToolTip(tr("ToolBar"));
        break;

    default: break;
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
    GetTreeBank()->MousePressEvent(ev);
}

void Gadgets::MouseReleaseEvent(QMouseEvent *ev){
    GetTreeBank()->MouseReleaseEvent(ev);
}

void Gadgets::MouseMoveEvent(QMouseEvent *ev){
    GetTreeBank()->MouseMoveEvent(ev);
}

void Gadgets::MouseDoubleClickEvent(QMouseEvent *ev){
    GetTreeBank()->MouseDoubleClickEvent(ev);
}

void Gadgets::WheelEvent(QWheelEvent *ev){
    GetTreeBank()->WheelEvent(ev);
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
    SharedWebElement elem = awe->GetElement();

    if(!elem) return;

    if(elem->GetAction() == WebElement::Focus){

        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        Deactivate();
        elem->SetFocus();
        master->TriggerAction(Page::_FocusElement, QVariant::fromValue(elem));

    } else if(elem->GetAction() == WebElement::Click){

        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        Deactivate();
        elem->SetFocus();
        master->TriggerAction(Page::_ClickElement, QVariant::fromValue(elem));

    } else if(elem->GetAction() == WebElement::Hover){

        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        Deactivate();
        elem->SetFocus();
        master->TriggerAction(Page::_HoverElement, QVariant::fromValue(elem));

    } else if(m_AccessKeyAction == OpenMenu){
        QList<QRectF> list;

        list << awe->boundingRect();

        awe->setSelected(true);

        list << awe->boundingRect();

        foreach(QRectF rect, list){
            update(rect);
        }
        foreach(AccessibleWebElement *e, m_AccessibleWebElementCache){
            e->UpdateMinimal();
        }
    } else {
        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

        QVariant var = QVariant::fromValue(elem);
        Deactivate();
        switch(m_AccessKeyAction){
        case ClickElement:                 master->TriggerAction(Page::_ClickElement, var); break;
        case FocusElement:                 master->TriggerAction(Page::_FocusElement, var); break;
        case HoverElement:                 master->TriggerAction(Page::_HoverElement, var); break;
        case OpenInNewViewNode:            master->TriggerAction(Page::_OpenInNewViewNode, var); break;
        case OpenInNewDirectory:           master->TriggerAction(Page::_OpenInNewDirectory, var); break;
        case OpenOnRoot:                   master->TriggerAction(Page::_OpenOnRoot, var); break;
        case OpenInNewViewNodeBackground:  master->TriggerAction(Page::_OpenInNewViewNodeBackground, var); break;
        case OpenInNewDirectoryBackground: master->TriggerAction(Page::_OpenInNewDirectoryBackground, var); break;
        case OpenOnRootBackground:         master->TriggerAction(Page::_OpenOnRootBackground, var); break;
        case OpenInNewViewNodeNewWindow:   master->TriggerAction(Page::_OpenInNewViewNodeNewWindow, var); break;
        case OpenInNewDirectoryNewWindow:  master->TriggerAction(Page::_OpenInNewDirectoryNewWindow, var); break;
        case OpenOnRootNewWindow:          master->TriggerAction(Page::_OpenOnRootNewWindow, var); break;
        default: break;
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
        update(rect);
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
    SharedWebElement elem = awe->GetElement();

    if(!elem) return;

    if(m_AccessKeyKeyMap.contains(seq)){
        SharedView master = GetMaster().lock();

        Q_ASSERT(master);

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
    Q_UNUSED(item) Q_UNUSED(widget)

    painter->setBrush(Qt::NoBrush);
    painter->setFont(AccessKeyChipLFont());

    painter->setPen(Theme::Pen(Theme::GadgetsAccessKeyLabelText));
    painter->drawText(boundingRect(), Qt::AlignCenter,
                      m_IsNumber ?
                        QStringLiteral("%1").arg(m_Index) :
                        QStringLiteral("%1").arg(static_cast<Gadgets*>(parentObject())->IndexToString(m_Index)));

    painter->setPen(Theme::Pen(Theme::GadgetsAccessKeyLabelBorder));
    painter->drawRect(boundingRect());
}
