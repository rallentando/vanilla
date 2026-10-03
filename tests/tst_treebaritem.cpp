#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QLayout>
#include <QMainWindow>
#include <QPainter>
#include <QPointer>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyleOptionToolBar>
#include <QToolBar>

#include "lightnode.hpp"
#include "treebar.hpp"

#include "testsupport.hpp"

namespace {

class HandleReferenceToolBar : public QToolBar {
public:
    QRect HandleRect() const {
        QStyleOptionToolBar option;
        initStyleOption(&option);
        return style()->subElementRect(QStyle::SE_ToolBarHandle, &option, this);
    }
};

}

class tst_treebaritem : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void consecutiveFoldsKeepTheLatestTree_data();
    void consecutiveFoldsKeepTheLatestTree();
    void unfoldingRestoresTheWholeVisibleSubtree_data();
    void unfoldingRestoresTheWholeVisibleSubtree();
    void aFoldedDirectoryKeepsOnlyItsPrimaryCurrentPathVisible();
    void unfoldingDoesNotGrowWithWhatTheBarAlreadyShows();
    void foldingNearTheTopCostsWhatFoldingAtTheBottomDoes();
    void foldingMovesAlongTheDirectorysCentreLine_data();
    void foldingMovesAlongTheDirectorysCentreLine();
    void foldingADirectoryWithItsParentSlidesTheRestOnce();
    void foldingLetsGoOfAFocusedChild();
    void aFolderPressCanFinishLayoutButNotDragOrScroll();
    void finishingAnimationsSurvivesLayerDestruction();
    void foldingSurvivesLayerDestruction_data();
    void foldingSurvivesLayerDestruction();
    void nodesCreatedTogetherArePlacedByTheirOwnNeighbours();
    void aNodeCreatedInAFoldedDirectoryStaysHidden();
    void aCurrentNodeBelowTheLastLayerGetsALayerBuilt();

    void anItemLetGoOfItsNodeIsOutOfTheSceneAtOnce();
    void anItemLetGoOfItsNodeIsOutOfTheSceneAtOnce_data();

    void theBarLetsGoOfEveryItemAboutTheNodesItIsGiven();
    void theBarLetsGoOfAnItemWhichIsAlreadyLeaving();
    void theBarLetsGoOfTheItemsBelowADirectory();
    void theBarKeepsTheItemsItWasNotAskedAbout();

    void anItemDetachedFromItsLayerOutlivesItSafely();
    void anItemDetachedFromItsLayerOutlivesItSafely_data();

    void theExitHandlersBelongToTheLayerTheyReachInto();
    void forgettingTheNodeTakesTheExitHandlersWithIt();

    void aSceneWithoutTheItemDrawsNothingOfTheFreedNode();
    void aSceneWithoutTheItemDrawsNothingOfTheFreedNode_data();

    void theHandleUsesTheToolBarsCrossAxisMargin();
    void theMinimumTabHeightKeepsCompactBreathingRoom();
    void anIconIsCentredInAnEvenHeightThumbnailTitleBand();
    void aThumbnailTooThinToShowKeepsEveryControlOnTheTitleBand();

private:
    void Build();
    void TearDown();
    NodeItem *AddItem(Node *nd);
    static void FreeNode(Node *nd);

    TreeBar *m_Bar = nullptr;
    LayerItem *m_Layer = nullptr;
    ViewNode *m_Root = nullptr;
};

void tst_treebaritem::initTestCase(){
    TestSupport::SilenceDebugOutput();
    TestSupport::DisableWidgetAnimation();
}

void tst_treebaritem::consecutiveFoldsKeepTheLatestTree_data(){
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("animated");
    QTest::newRow("small-animated") << 2 << true;
    QTest::newRow("large-animated") << 260 << true;
    QTest::newRow("small-plain") << 2 << false;
    QTest::newRow("large-plain") << 260 << false;
}

void tst_treebaritem::consecutiveFoldsKeepTheLatestTree(){
    QFETCH(int, count);
    QFETCH(bool, animated);
    Build();
    TreeBar::m_EnableAnimation = animated;
    {
        QSignalBlocker blocker(m_Bar);
        m_Bar->setOrientation(Qt::Vertical);
    }
    m_Bar->m_VerticalNodeHeight = 24;
    m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);
    auto *folder = m_Root->MakeChild();
    folder->SetFolded(true);
    for(int i = 0; i < count; ++i) folder->MakeChild()->SetHoldView(true);
    auto *tail = m_Root->MakeChild();
    tail->SetHoldView(true);
    m_Layer->CreateNodeItem(folder, 0, 0, 0, 300);
    m_Layer->CreateNodeItem(tail, 1, 0, 0, 300);
    NodeList changed{folder};
    for(bool folded : {false, true, false}){
        folder->SetFolded(folded);
        m_Bar->OnFoldedChanged(changed);
        if(count == 2) QVERIFY(!m_Layer->IsLocked());
    }
    QVERIFY(m_Layer->FinishAnimations());
    NodeList displayed;
    for(auto *item : m_Layer->GetNodeItems()) displayed.append(item->GetNode());
    NodeList expected{folder};
    expected.append(folder->GetChildren());
    expected.append(tail);
    QCOMPARE(displayed, expected);
    m_Layer->SetFocusedNode(m_Layer->GetNodeItems().at(1));
    folder->SetFolded(true);
    m_Bar->OnFoldedChanged(changed);
    for(auto *item : m_Layer->GetNodeItems()){
        if(item->GetNode()->GetParent() != folder) continue;
        item->SetFocused(false);
        item->setSelected(true);
        item->setSelected(false);
    }
    QVERIFY(m_Layer->FinishAnimations());
    QCOMPARE(m_Layer->GetNodeItems().size(), 2);
    QCOMPARE(m_Layer->GetFocusedNode(), static_cast<NodeItem*>(nullptr));
    QCOMPARE(m_Layer->GetNodeItems().last()->GetNode(), static_cast<Node*>(tail));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(m_Layer->GetNodeItems().size(), 2);
    TearDown();
}

void tst_treebaritem::unfoldingRestoresTheWholeVisibleSubtree_data(){
    QTest::addColumn<int>("nestedChildren");
    QTest::addColumn<bool>("animated");
    QTest::newRow("small-animated") << 2 << true;
    QTest::newRow("large-animated") << 260 << true;
    QTest::newRow("small-plain") << 2 << false;
    QTest::newRow("large-plain") << 260 << false;
}

void tst_treebaritem::unfoldingRestoresTheWholeVisibleSubtree(){
    QFETCH(int, nestedChildren);
    QFETCH(bool, animated);
    Build();
    TreeBar::m_EnableAnimation = animated;
    { QSignalBlocker blocker(m_Bar); m_Bar->setOrientation(Qt::Vertical); }
    m_Bar->m_VerticalNodeHeight = 24;
    m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);

    ViewNode *outer = m_Root->MakeChild();
    outer->SetFolded(true);
    ViewNode *inner = outer->MakeChild();
    inner->SetFolded(false);
    NodeList nestedTabs;
    for(int i = 0; i < nestedChildren; ++i){
        ViewNode *nestedTab = inner->MakeChild();
        nestedTab->SetHoldView(true);
        nestedTabs.append(nestedTab);
    }
    ViewNode *sibling = outer->MakeChild();
    sibling->SetHoldView(true);
    ViewNode *tail = m_Root->MakeChild();
    tail->SetHoldView(true);

    m_Layer->CreateNodeItem(outer, 0, 0, 0, 300);
    m_Layer->CreateNodeItem(tail, 1, 0, 0, 300);
    NodeList changed{outer};
    outer->SetFolded(false);
    m_Bar->OnFoldedChanged(changed);
    QVERIFY(m_Layer->FinishAnimations());
    QCOMPARE(m_Layer->GetNodeItems().size(), nestedChildren + 4);
    NodeList repeated{outer, inner};
    m_Bar->OnFoldedChanged(repeated);
    QVERIFY(m_Layer->FinishAnimations());

    NodeList displayed;
    QList<int> nests;
    foreach(NodeItem *item, m_Layer->GetNodeItems()){
        displayed.append(item->GetNode());
        nests.append(item->GetNest());
    }
    NodeList expected{outer, inner};
    expected.append(nestedTabs);
    expected.append(sibling);
    expected.append(tail);
    QCOMPARE(displayed, expected);
    QCOMPARE(nests.at(0), 0);
    QCOMPARE(nests.at(1), 1);
    for(int i = 0; i < nestedTabs.size(); ++i)
        QCOMPARE(nests.at(2 + i), 2);
    QCOMPARE(nests.at(2 + nestedTabs.size()), 1);
    QCOMPARE(nests.last(), 0);

    outer->SetFolded(true);
    m_Bar->OnFoldedChanged(repeated);
    QVERIFY(m_Layer->FinishAnimations());
    QCOMPARE(m_Layer->GetNodeItems().size(), 2);
    QCOMPARE(m_Layer->GetNodeItems().first()->GetNode(), static_cast<Node*>(outer));
    QCOMPARE(m_Layer->GetNodeItems().last()->GetNode(), static_cast<Node*>(tail));
    TearDown();
}

void tst_treebaritem::unfoldingDoesNotGrowWithWhatTheBarAlreadyShows(){
    Build();
    TreeBar::m_EnableAnimation = false;
    {
        QSignalBlocker blocker(m_Bar);
        m_Bar->setOrientation(Qt::Vertical);
    }
    m_Bar->m_VerticalNodeHeight = 24;
    m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);
    const int CHILDREN = 2000;
    QList<ViewNode*> dirs;
    for(int d = 0; d < 5; d++){
        auto *dir = m_Root->MakeChild();
        dir->SetFolded(true);
        for(int i = 0; i < CHILDREN; ++i) dir->MakeChild()->SetHoldView(true);
        dirs << dir;
    }
    int index = 0;
    for(Node *nd : m_Root->GetChildren()) m_Layer->CreateNodeItem(nd, index++, 0, 0, 300);
    auto unfold = [&](ViewNode *dir){
        NodeList changed{dir};
        dir->SetFolded(false);
        QElapsedTimer timer;
        timer.start();
        m_Bar->OnFoldedChanged(changed);
        const qint64 elapsed = timer.nsecsElapsed();
        m_Layer->FinishAnimations();
        return elapsed;
    };
    const qint64 shortBar = unfold(dirs[0]);
    unfold(dirs[1]);
    unfold(dirs[2]);
    unfold(dirs[3]);
    QCOMPARE(m_Layer->GetNodeItems().size(), 5 + 4 * CHILDREN);
    const qint64 longBar = unfold(dirs[4]);
    QCOMPARE(m_Layer->GetNodeItems().size(), 5 + 5 * CHILDREN);
    QVERIFY2(longBar < shortBar * 4,
             qPrintable(QStringLiteral("unfolding %1 nodes took %2 ms on a bar of 5 items "
                                       "and %3 ms on one of %4")
                        .arg(CHILDREN).arg(shortBar / 1e6, 0, 'f', 1)
                        .arg(longBar / 1e6, 0, 'f', 1).arg(5 + 4 * CHILDREN)));
    TearDown();
}

void tst_treebaritem::foldingNearTheTopCostsWhatFoldingAtTheBottomDoes(){
    qint64 elapsed[2] = {0, 0};
    for(bool top : {true, false}){
        Build();
        TreeBar::m_EnableAnimation = true;
        {
            QSignalBlocker blocker(m_Bar);
            m_Bar->setOrientation(Qt::Vertical);
        }
        m_Bar->m_VerticalNodeHeight = 24;
        m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);
        ViewNode *target = nullptr;
        auto makeTarget = [&]{
            target = m_Root->MakeChild();
            target->SetFolded(true);
            for(int i = 0; i < 100; i++) target->MakeChild()->SetHoldView(true);
        };
        if(top) makeTarget();
        for(int d = 0; d < 5; d++){
            auto *dir = m_Root->MakeChild();
            dir->SetFolded(false);
            for(int i = 0; i < 2000; ++i) dir->MakeChild()->SetHoldView(true);
        }
        if(!top) makeTarget();
        int index = 0;
        std::function<void(Node*, int)> add = [&](Node *nd, int nest){
            m_Layer->CreateNodeItem(nd, index++, 0, nest, 300);
            if(nd->IsDirectory() && !nd->GetFolded())
                for(Node *child : nd->GetChildren()) add(child, nest + 1);
        };
        for(Node *nd : m_Root->GetChildren()) add(nd, 0);
        const int shown = m_Layer->GetNodeItems().size();
        for(bool folded : {false, true}){
            NodeList changed{target};
            target->SetFolded(folded);
            QElapsedTimer timer;
            timer.start();
            m_Bar->OnFoldedChanged(changed);
            if(!folded) elapsed[top ? 0 : 1] = timer.nsecsElapsed();
            int running = 0;
            for(auto *item : m_Layer->GetNodeItems())
                if(item->GetAnimation()->state() == QAbstractAnimation::Running) ++running;
            QVERIFY2(running < 100 + 200, qPrintable(QString::number(running)));
            QVERIFY(m_Layer->FinishAnimations());
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            const auto &items = m_Layer->GetNodeItems();
            QCOMPARE(items.size(), folded ? shown : shown + 100);
            const qreal first = items.first()->GetRect().top();
            for(int i = 0; i < items.size(); i++)
                QCOMPARE(items[i]->GetRect().top(), first + i * 24);
            NodeList expected;
            std::function<void(Node*)> walk = [&](Node *nd){
                expected << nd;
                if(nd->IsDirectory() && !nd->GetFolded())
                    for(Node *child : nd->GetChildren()) walk(child);
            };
            for(Node *nd : m_Root->GetChildren()) walk(nd);
            NodeList displayed;
            for(auto *item : items) displayed << item->GetNode();
            QCOMPARE(displayed, expected);
        }
        TearDown();
    }
    QVERIFY2(elapsed[0] < elapsed[1] * 4 + 20000000,
             qPrintable(QStringLiteral("unfolding a hundred nodes took %1 ms at the top "
                                       "and %2 ms at the bottom")
                        .arg(elapsed[0] / 1e6, 0, 'f', 1).arg(elapsed[1] / 1e6, 0, 'f', 1)));
}

void tst_treebaritem::foldingMovesAlongTheDirectorysCentreLine_data(){
    QTest::addColumn<int>("count");
    QTest::newRow("few") << 2;
    QTest::newRow("many") << 300;
}

void tst_treebaritem::foldingMovesAlongTheDirectorysCentreLine(){
    QFETCH(int, count);
    Build();
    TreeBar::m_EnableAnimation = true;
    {
        QSignalBlocker blocker(m_Bar);
        m_Bar->setOrientation(Qt::Vertical);
    }
    m_Bar->m_VerticalNodeHeight = 24;
    m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);
    auto *head = m_Root->MakeChild();
    head->SetHoldView(true);
    auto *folder = m_Root->MakeChild();
    folder->SetFolded(true);
    for(int i = 0; i < count; ++i) folder->MakeChild()->SetHoldView(true);
    auto *tail = m_Root->MakeChild();
    tail->SetHoldView(true);
    m_Layer->CreateNodeItem(head, 0, 0, 0, 300);
    NodeItem *directory = m_Layer->CreateNodeItem(folder, 1, 0, 0, 300);
    m_Layer->CreateNodeItem(tail, 2, 0, 0, 300);
    const qreal line = directory->GetRect().center().y();
    NodeList changed{folder};

    folder->SetFolded(false);
    m_Bar->OnFoldedChanged(changed);
    int seen = 0;
    for(auto *item : m_Layer->GetNodeItems()){
        if(item->GetNode()->GetParent() != folder) continue;
        auto *animation = item->GetAnimation();
        if(animation->state() != QAbstractAnimation::Running) continue;
        const QRectF start = animation->startValue().toRectF();
        const QRectF end = animation->endValue().toRectF();
        QCOMPARE(start.top(), line);
        QCOMPARE(start.height(), 0.0);
        QCOMPARE(start.left(), end.left());
        QCOMPARE(start.width(), end.width());
        QVERIFY(item->zValue() < directory->zValue());
        ++seen;
    }
    QVERIFY(seen >= qMin(count, 20));
    QVERIFY(m_Layer->FinishAnimations());
    for(auto *item : m_Layer->GetNodeItems())
        QCOMPARE(item->zValue(), directory->zValue());

    folder->SetFolded(true);
    m_Bar->OnFoldedChanged(changed);
    seen = 0;
    for(auto *item : m_Layer->GetNodeItems()){
        if(item->GetNode()->GetParent() != folder) continue;
        auto *animation = item->GetAnimation();
        if(animation->state() != QAbstractAnimation::Running) continue;
        const QRectF start = animation->startValue().toRectF();
        const QRectF end = animation->endValue().toRectF();
        QCOMPARE(end.top(), line);
        QCOMPARE(end.height(), 0.0);
        QCOMPARE(end.left(), start.left());
        QCOMPARE(end.width(), start.width());
        ++seen;
    }
    QVERIFY(seen >= qMin(count, 20));
    QVERIFY(m_Layer->FinishAnimations());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(m_Layer->GetNodeItems().size(), 3);
    TearDown();
}

void tst_treebaritem::foldingADirectoryWithItsParentSlidesTheRestOnce(){
    Build();
    TreeBar::m_EnableAnimation = true;
    {
        QSignalBlocker blocker(m_Bar);
        m_Bar->setOrientation(Qt::Vertical);
    }
    m_Bar->m_VerticalNodeHeight = 24;
    m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);
    auto *outer = m_Root->MakeChild();
    outer->SetFolded(false);
    auto *inner = outer->MakeChild();
    inner->SetFolded(false);
    auto *child = inner->MakeChild();
    child->SetHoldView(true);
    auto *tail = m_Root->MakeChild();
    tail->SetHoldView(true);
    m_Layer->CreateNodeItem(outer, 0, 0, 0, 300);
    m_Layer->CreateNodeItem(inner, 1, 0, 1, 300);
    m_Layer->CreateNodeItem(child, 2, 0, 2, 300);
    m_Layer->CreateNodeItem(tail, 3, 0, 0, 300);
    outer->SetFolded(true);
    inner->SetFolded(true);
    NodeList changed{outer, inner};
    m_Bar->OnFoldedChanged(changed);
    QVERIFY(m_Layer->FinishAnimations());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    const auto &items = m_Layer->GetNodeItems();
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0]->GetNode(), static_cast<Node*>(outer));
    QCOMPARE(items[1]->GetNode(), static_cast<Node*>(tail));
    QCOMPARE(items[1]->GetRect().top(), items[0]->GetRect().top() + 24);
    TearDown();
}

void tst_treebaritem::foldingLetsGoOfAFocusedChild(){
    Build();
    TreeBar::m_EnableAnimation = true;
    {
        QSignalBlocker blocker(m_Bar);
        m_Bar->setOrientation(Qt::Vertical);
    }
    m_Bar->m_VerticalNodeHeight = 24;
    m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);
    auto *folder = m_Root->MakeChild();
    folder->SetFolded(false);
    QList<Node*> children;
    for(int i = 0; i < 3; i++){
        auto *child = folder->MakeChild();
        child->SetHoldView(true);
        children << child;
    }
    auto *tail = m_Root->MakeChild();
    tail->SetHoldView(true);
    m_Layer->CreateNodeItem(folder, 0, 0, 0, 300);
    NodeItem *focused = nullptr;
    for(int i = 0; i < 3; i++){
        NodeItem *item = m_Layer->CreateNodeItem(children[i], i + 1, 0, 1, 300);
        if(i == 1) focused = item;
    }
    m_Layer->CreateNodeItem(tail, 4, 0, 0, 300);
    m_Layer->SetFocusedNode(focused);
    folder->SetFolded(true);
    NodeList changed{folder};
    m_Bar->OnFoldedChanged(changed);
    QVERIFY(m_Layer->GetFocusedNode() != focused);
    QVERIFY(m_Layer->FinishAnimations());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    const auto &items = m_Layer->GetNodeItems();
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0]->GetNode(), static_cast<Node*>(folder));
    QCOMPARE(items[1]->GetNode(), static_cast<Node*>(tail));
    QCOMPARE(items[1]->GetRect().top(), items[0]->GetRect().top() + 24);
    TearDown();
}

void tst_treebaritem::aFoldedDirectoryKeepsOnlyItsPrimaryCurrentPathVisible(){
    ViewNode root;
    ViewNode *folder = root.MakeChild();
    ViewNode *primary = folder->MakeChild();
    ViewNode *hidden = folder->MakeChild();
    primary->SetHoldView(true);
    hidden->SetHoldView(true);
    folder->SetFolded(true);
    folder->SetPrimary(primary);

    const NodeList currentPath{folder, primary};
    const QList<TreeBar::VisibleNode> visible =
        TreeBar::VisibleSubtree(folder, 0, currentPath);
    QCOMPARE(visible.size(), 2);
    QCOMPARE(visible[0], qMakePair(static_cast<Node*>(folder), 0));
    QCOMPARE(visible[1], qMakePair(static_cast<Node*>(primary), 1));
}

void tst_treebaritem::aFolderPressCanFinishLayoutButNotDragOrScroll(){
    Build();
    TreeBar::m_EnableAnimation = true;
    {
        QSignalBlocker blocker(m_Bar);
        m_Bar->setOrientation(Qt::Vertical);
    }
    m_Bar->m_VerticalNodeHeight = 24;
    m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);
    NodeItem *folder = AddItem(m_Root->MakeChild());
    folder->SetRect(QRectF(0, 0, 300, 24));
    NodeItem *other = AddItem(m_Root->MakeChild());
    auto press = [&]{
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
        event.setButton(Qt::LeftButton);
        event.setButtons(Qt::LeftButton);
        event.setPos(QPointF(50, 12));
        folder->mousePressEvent(&event);
        return event.isAccepted();
    };
    auto animate = [&]{
        other->OnCreated(QRectF(0, 48, 300, 24), QRectF(0, 24, 300, 1));
        m_Layer->LockWhileAnimating();
    };
    animate();
    QVERIFY(press());
    QVERIFY(!m_Layer->IsLocked());
    QVERIFY(!other->IsLocked());
    animate();
    const bool closeEnabled = TreeBar::m_EnableCloseButton;
    TreeBar::m_EnableCloseButton = true;
    QGraphicsSceneMouseEvent control(QEvent::GraphicsSceneMousePress);
    control.setButton(Qt::LeftButton);
    control.setButtons(Qt::LeftButton);
    control.setPos(folder->CloseButtonRect().center());
    folder->mousePressEvent(&control);
    TreeBar::m_EnableCloseButton = closeEnabled;
    QVERIFY(!control.isAccepted());
    {
        QSignalBlocker blocker(m_Bar);
        m_Bar->setOrientation(Qt::Horizontal);
        QVERIFY(!press());
        m_Bar->setOrientation(Qt::Vertical);
    }
    QVERIFY(m_Layer->FinishAnimations());
    m_Layer->LockWhileAnimating();
    QVERIFY(!press());
    QVERIFY(m_Layer->FinishAnimations());

    animate();
    other->setZValue(30.0);
    other->SetFocused(true);
    QVERIFY(!m_Layer->FinishAnimations());
    other->setSelected(true);
    other->setSelected(false);
    bool dragCommitted = false;
    connect(other->GetAnimation(), &QPropertyAnimation::finished, this, [&]{ dragCommitted = true; });
    QVERIFY(!m_Layer->FinishAnimations());
    QVERIFY(!dragCommitted);
    QVERIFY(!press());
    other->GetAnimation()->stop();
    other->setZValue(10.0);
    QVERIFY(m_Layer->FinishAnimations());

    folder->OnCreated(QRectF(0, 120, 300, 24), QRectF(0, 0, 300, 24));
    QVERIFY(!press());
    folder->OnDeleted(QRectF(0, 120, 300, 0));
    QVERIFY(!press());
    TearDown();
}

void tst_treebaritem::finishingAnimationsSurvivesLayerDestruction(){
    Build();
    TreeBar::m_EnableAnimation = true;
    NodeItem *item = AddItem(m_Root->MakeChild());
    item->OnCreated(QRectF(0, 0, 100, 24), QRectF(0, 0, 100, 1));
    QPointer<LayerItem> layer = m_Layer;
    connect(item->GetAnimation(), &QPropertyAnimation::finished, this, [&]{ delete m_Layer; });
    QVERIFY(!m_Layer->FinishAnimations());
    QVERIFY(layer.isNull());
    TearDown();
}

void tst_treebaritem::foldingSurvivesLayerDestruction_data(){
    QTest::addColumn<bool>("rebuild");
    QTest::newRow("deleted") << false;
    QTest::newRow("rebuilt") << true;
}

void tst_treebaritem::foldingSurvivesLayerDestruction(){
    QFETCH(bool, rebuild);
    Build();
    TreeBar::m_EnableAnimation = true;
    { QSignalBlocker blocker(m_Bar); m_Bar->setOrientation(Qt::Vertical); }
    NodeItem *item = AddItem(m_Root->MakeChild());
    item->OnCreated(QRectF(0, 0, 100, 24), QRectF(0, 0, 100, 1));
    auto *second = new LayerItem(nullptr, m_Bar, m_Root, nullptr, nullptr);
    m_Bar->m_Scene->addItem(second);
    m_Bar->m_LayerList.append(second);
    QPointer<LayerItem> replacement;
    QPointer<LayerItem> layer = m_Layer;
    connect(item->GetAnimation(), &QPropertyAnimation::finished, this, [&]{
        delete second;
        delete m_Layer;
        if(rebuild){
            replacement = new LayerItem(nullptr, m_Bar, m_Root, nullptr, nullptr);
            m_Bar->m_Scene->addItem(replacement);
            m_Bar->m_LayerList.append(replacement);
        }
    });
    NodeList changed{m_Root};
    m_Bar->OnFoldedChanged(changed);
    QVERIFY(layer.isNull());
    QCOMPARE(m_Bar->m_LayerList.size(), rebuild ? 1 : 0);
    if(rebuild) QCOMPARE(m_Bar->m_LayerList.first(), replacement.data());
    TearDown();
}

void tst_treebaritem::nodesCreatedTogetherArePlacedByTheirOwnNeighbours(){
    Build();
    TreeBar::m_EnableAnimation = false;
    { QSignalBlocker blocker(m_Bar); m_Bar->setOrientation(Qt::Vertical); }
    m_Bar->m_VerticalNodeHeight = 24;
    m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);

    ViewNode *first = m_Root->MakeChild();
    ViewNode *between = m_Root->MakeChild();
    ViewNode *second = m_Root->MakeChild();
    ViewNode *last = m_Root->MakeChild();
    for(ViewNode *nd : {first, between, second, last}) nd->SetHoldView(true);
    m_Layer->CreateNodeItem(first, 0, 0, 0, 300);
    m_Layer->CreateNodeItem(second, 1, 0, 0, 300);

    NodeList created{between, last};
    m_Bar->OnNodeCreated(created);

    NodeList displayed;
    for(NodeItem *item : m_Layer->GetNodeItems()) displayed.append(item->GetNode());
    QCOMPARE(displayed, (NodeList{first, between, second, last}));
    TearDown();
}

void tst_treebaritem::aNodeCreatedInAFoldedDirectoryStaysHidden(){
    Build();
    TreeBar::m_EnableAnimation = false;
    { QSignalBlocker blocker(m_Bar); m_Bar->setOrientation(Qt::Vertical); }
    m_Bar->m_VerticalNodeHeight = 24;
    m_Bar->m_Scene->setSceneRect(0, 0, 320, 640);

    ViewNode *folder = m_Root->MakeChild();
    folder->SetFolded(true);
    ViewNode *inner = folder->MakeChild();
    inner->MakeChild()->SetHoldView(true);
    ViewNode *created = folder->MakeChild();
    created->SetHoldView(true);
    ViewNode *tail = m_Root->MakeChild();
    tail->SetHoldView(true);
    m_Layer->CreateNodeItem(folder, 0, 0, 0, 300);
    m_Layer->CreateNodeItem(tail, 1, 0, 0, 300);

    NodeList nds{created};
    m_Bar->OnNodeCreated(nds);

    NodeList displayed;
    for(NodeItem *item : m_Layer->GetNodeItems()) displayed.append(item->GetNode());
    QCOMPARE(displayed, (NodeList{folder, tail}));
    TearDown();
}

void tst_treebaritem::aCurrentNodeBelowTheLastLayerGetsALayerBuilt(){
    Build();
    m_Bar->m_HorizontalNodeWidth = 150;
    m_Bar->m_Scene->setSceneRect(0, 0, 640, 40);

    ViewNode *folder = m_Root->MakeChild();
    ViewNode *child = folder->MakeChild();
    child->SetHoldView(true);
    m_Layer->CreateNodeItem(folder, 0, 0, 0, 24);
    m_Layer->SetNode(folder);
    QCOMPARE(m_Bar->m_LayerList.size(), 1);

    m_Bar->OnCurrentChanged(child);

    QCOMPARE(m_Bar->m_LayerList.size(), 2);
    QCOMPARE(m_Bar->m_LayerList.at(0)->GetNode(), static_cast<Node*>(folder));
    QCOMPARE(m_Bar->m_LayerList.at(1)->GetNode(), static_cast<Node*>(child));
    QCOMPARE(m_Bar->m_LayerList.at(1)->GetNodeItems().size(), 1);
    TearDown();
}

void tst_treebaritem::Build(){
    m_Bar = new TreeBar(nullptr, nullptr);
    m_Root = new ViewNode();

    m_Layer = new LayerItem(nullptr, m_Bar, m_Root, nullptr, nullptr);
    m_Bar->m_Scene->addItem(m_Layer);
    m_Bar->m_LayerList << m_Layer;
}

void tst_treebaritem::TearDown(){
    m_Bar->m_LayerList.clear();
    m_Layer = nullptr;
    delete m_Bar;
    m_Bar = nullptr;
    m_Layer = nullptr;
    delete m_Root;
    m_Root = nullptr;
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void tst_treebaritem::FreeNode(Node *nd){
    if(Node *parent = nd->GetParent()) parent->RemoveChild(nd);
    nd->Delete();
}

NodeItem *tst_treebaritem::AddItem(Node *nd){
    NodeItem *item = new NodeItem(nullptr, m_Bar, nd, m_Layer);
    m_Layer->GetNodeItems() << item;
    item->SetRect(QRectF(0, 0, 100, 24));
    return item;
}

void tst_treebaritem::anItemLetGoOfItsNodeIsOutOfTheSceneAtOnce(){
    QFETCH(bool, animated);

    Build();
    TreeBar::m_EnableAnimation = animated;

    ViewNode *leaf = m_Root->MakeChild();
    leaf->SetHoldView(true);
    NodeItem *item = AddItem(leaf);
    QPointer<NodeItem> watched = item;

    QRectF rect = item->GetRect();
    rect.setWidth(0);
    item->OnDeleted(rect);

    QVERIFY(!watched.isNull());
    QCOMPARE(item->GetNode(), static_cast<Node*>(leaf));
    QVERIFY(m_Bar->m_Scene->items().contains(item));

    item->ForgetNode();

    QCOMPARE(item->GetNode(), static_cast<Node*>(nullptr));
    QVERIFY(!m_Bar->m_Scene->items().contains(item));
    QVERIFY(!m_Layer->GetNodeItems().contains(item));
    QVERIFY(!item->isVisible());
    QVERIFY(item->GetAnimation()->state() != QAbstractAnimation::Running);

    FreeNode(leaf);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(watched.isNull());

    TearDown();
}

void tst_treebaritem::anItemLetGoOfItsNodeIsOutOfTheSceneAtOnce_data(){
    QTest::addColumn<bool>("animated");
    QTest::newRow("animated") << true;
    QTest::newRow("plain") << false;
}

void tst_treebaritem::theBarLetsGoOfEveryItemAboutTheNodesItIsGiven(){
    Build();
    TreeBar::m_EnableAnimation = true;

    ViewNode *going = m_Root->MakeChild();
    going->SetHoldView(true);
    ViewNode *staying = m_Root->MakeChild();
    staying->SetHoldView(true);

    NodeItem *goingItem = AddItem(going);
    NodeItem *stayingItem = AddItem(staying);

    NodeList nds = NodeList() << going;
    m_Bar->ForgetNodes(nds);

    QCOMPARE(goingItem->GetNode(), static_cast<Node*>(nullptr));
    QCOMPARE(stayingItem->GetNode(), static_cast<Node*>(staying));
    QVERIFY(!m_Bar->m_Scene->items().contains(goingItem));
    QVERIFY(m_Bar->m_Scene->items().contains(stayingItem));

    FreeNode(going);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    TearDown();
}

void tst_treebaritem::theBarLetsGoOfAnItemWhichIsAlreadyLeaving(){
    Build();
    TreeBar::m_EnableAnimation = true;

    ViewNode *leaf = m_Root->MakeChild();
    leaf->SetHoldView(true);
    NodeItem *item = AddItem(leaf);
    QPointer<NodeItem> watched = item;

    QRectF rect = item->GetRect();
    rect.setWidth(0);
    item->OnDeleted(rect);
    m_Layer->RemoveFromNodeItems(item);
    QVERIFY(!m_Layer->GetNodeItems().contains(item));
    QVERIFY(m_Bar->m_Scene->items().contains(item));

    NodeList nds = NodeList() << leaf;
    m_Bar->ForgetNodes(nds);

    QCOMPARE(item->GetNode(), static_cast<Node*>(nullptr));
    QVERIFY(!m_Bar->m_Scene->items().contains(item));

    FreeNode(leaf);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(watched.isNull());
    TearDown();
}

void tst_treebaritem::theBarLetsGoOfTheItemsBelowADirectory(){
    Build();
    TreeBar::m_EnableAnimation = false;

    ViewNode *dir = m_Root->MakeChild();
    ViewNode *child = dir->MakeChild();
    child->SetHoldView(true);

    NodeItem *item = AddItem(child);

    NodeList nds = NodeList() << dir;
    m_Bar->ForgetNodes(nds);

    QCOMPARE(item->GetNode(), static_cast<Node*>(nullptr));

    FreeNode(dir);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    TearDown();
}

void tst_treebaritem::theBarKeepsTheItemsItWasNotAskedAbout(){
    Build();
    TreeBar::m_EnableAnimation = false;

    ViewNode *one = m_Root->MakeChild();
    one->SetHoldView(true);
    ViewNode *two = m_Root->MakeChild();
    two->SetHoldView(true);

    NodeItem *first = AddItem(one);
    NodeItem *second = AddItem(two);

    NodeList nds = NodeList();
    m_Bar->ForgetNodes(nds);

    QCOMPARE(first->GetNode(), static_cast<Node*>(one));
    QCOMPARE(second->GetNode(), static_cast<Node*>(two));
    QCOMPARE(m_Layer->GetNodeItems().length(), 2);

    TearDown();
}

void tst_treebaritem::theExitHandlersBelongToTheLayerTheyReachInto(){
    Build();

    TreeBar::m_EnableAnimation = false;
    ViewNode *plain = m_Root->MakeChild();
    plain->SetHoldView(true);
    NodeItem *plainItem = AddItem(plain);
    QRectF rect = plainItem->GetRect();
    rect.setWidth(0);
    plainItem->OnDeleted(rect);

    QVERIFY2(QObject::disconnect(plainItem, &QObject::destroyed, m_Layer, nullptr),
             "the item's exit handler is not the layer's");

    TreeBar::m_EnableAnimation = true;
    ViewNode *animated = m_Root->MakeChild();
    animated->SetHoldView(true);
    NodeItem *animatedItem = AddItem(animated);
    QRectF other = animatedItem->GetRect();
    other.setWidth(0);
    animatedItem->OnDeleted(other);

    QVERIFY2(QObject::disconnect(animatedItem->GetAnimation(),
                                 &QPropertyAnimation::finished, m_Layer, nullptr),
             "the animation's exit handler is not the layer's");

    TearDown();
}

void tst_treebaritem::forgettingTheNodeTakesTheExitHandlersWithIt(){
    Build();
    TreeBar::m_EnableAnimation = false;

    ViewNode *leaf = m_Root->MakeChild();
    leaf->SetHoldView(true);
    NodeItem *item = AddItem(leaf);

    QRectF rect = item->GetRect();
    rect.setWidth(0);
    item->OnDeleted(rect);
    QVERIFY(QObject::disconnect(item, &QObject::destroyed, m_Layer, nullptr));

    item->OnDeleted(rect);
    item->ForgetNode();

    QVERIFY2(!QObject::disconnect(item, &QObject::destroyed, nullptr, nullptr),
             "a forgotten item still calls out on its own destruction");
    QVERIFY2(!QObject::disconnect(item->GetAnimation(),
                                  &QPropertyAnimation::finished, nullptr, nullptr),
             "a forgotten item's animation still calls out");

    FreeNode(leaf);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    TearDown();
}

void tst_treebaritem::anItemDetachedFromItsLayerOutlivesItSafely(){
    QFETCH(bool, animated);

    Build();
    TreeBar::m_EnableAnimation = animated;

    ViewNode *leaf = m_Root->MakeChild();
    leaf->SetHoldView(true);
    NodeItem *item = AddItem(leaf);
    QPointer<NodeItem> watched = item;

    QRectF rect = item->GetRect();
    rect.setWidth(0);
    item->OnDeleted(rect);

    NodeList nds = NodeList() << leaf;
    m_Bar->ForgetNodes(nds);
    FreeNode(leaf);

    QVERIFY(!watched.isNull());
    m_Bar->m_LayerList.removeAll(m_Layer);
    delete m_Layer;
    m_Layer = nullptr;

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(watched.isNull());

    TearDown();
}

void tst_treebaritem::anItemDetachedFromItsLayerOutlivesItSafely_data(){
    QTest::addColumn<bool>("animated");
    QTest::newRow("animated") << true;
    QTest::newRow("plain") << false;
}

void tst_treebaritem::aSceneWithoutTheItemDrawsNothingOfTheFreedNode(){
    QFETCH(bool, animated);

    Build();
    TreeBar::m_EnableAnimation = animated;

    ViewNode *leaf = m_Root->MakeChild();
    leaf->SetHoldView(true);
    leaf->SetTitle(QStringLiteral("a tab which is going"));
    NodeItem *item = AddItem(leaf);

    QRectF rect = item->GetRect();
    rect.setWidth(0);
    item->OnDeleted(rect);

    NodeList nds = NodeList() << leaf;
    m_Bar->ForgetNodes(nds);

    FreeNode(leaf);

    QImage image(200, 60, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        m_Bar->m_Scene->render(&painter);
    }

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    TearDown();
}

void tst_treebaritem::aSceneWithoutTheItemDrawsNothingOfTheFreedNode_data(){
    QTest::addColumn<bool>("animated");
    QTest::newRow("animated") << true;
    QTest::newRow("plain") << false;
}

void tst_treebaritem::theHandleUsesTheToolBarsCrossAxisMargin(){
    const QList<Qt::Orientation> orientations =
        { Qt::Horizontal, Qt::Vertical };
    foreach(Qt::Orientation orientation, orientations){
        QMainWindow barWindow;
        QMainWindow referenceWindow;
        TreeBar *bar = new TreeBar(nullptr, &barWindow);
        HandleReferenceToolBar *reference = new HandleReferenceToolBar();
        reference->setStyleSheet(QStringLiteral("QToolBar{ padding: %1px; spacing: %2px;}")
                                 .arg(bar->ScaleByDevice(TOOL_BAR_PADDING))
                                 .arg(bar->ScaleByDevice(TOOL_BAR_ICON_SPACING)));

        const Qt::ToolBarArea area = orientation == Qt::Horizontal
            ? Qt::TopToolBarArea : Qt::LeftToolBarArea;
        barWindow.addToolBar(area, bar);
        referenceWindow.addToolBar(area, reference);
        barWindow.resize(500, 500);
        referenceWindow.resize(500, 500);
        barWindow.layout()->activate();
        referenceWindow.layout()->activate();
        const QSize size = orientation == Qt::Horizontal
            ? QSize(500, 80) : QSize(80, 500);
        bar->resize(size);
        reference->resize(size);
        bar->ensurePolished();
        reference->ensurePolished();

        QStyleOptionToolBar option;
        bar->initStyleOption(&option);
        const QRect raw = bar->style()->subElementRect
            (QStyle::SE_ToolBarHandle, &option, bar);
        QCOMPARE(bar->HandlePaintRect(raw), reference->HandleRect());
    }
}

void tst_treebaritem::theMinimumTabHeightKeepsCompactBreathingRoom(){
    Build();
    m_Bar->resize(200, m_Bar->MinHeight());

    QCOMPARE(m_Bar->GetHorizontalNodeHeight(),
             m_Bar->ScaleByDevice(TREE_BAR_TAB_MINIMUM_HEIGHT));
    QVERIFY(TREE_BAR_TAB_MINIMUM_HEIGHT > 24);

    TearDown();
}

void tst_treebaritem::anIconIsCentredInAnEvenHeightThumbnailTitleBand(){
    const QRect titleBand(6, 22, 120, 20);
    const QSize favicon(16, 16);
    const QRect icon = NodeItem::CenteredIconRect(titleBand, favicon);

    QCOMPARE(icon, QRect(6, 24, 16, 16));
    QCOMPARE(icon.top() - titleBand.top(), titleBand.bottom() - icon.bottom());
}

void tst_treebaritem::aThumbnailTooThinToShowKeepsEveryControlOnTheTitleBand(){
    Build();
    ViewNode *leaf = m_Root->MakeChild();
    NodeItem *item = AddItem(leaf);

    const int tabHeight =
        m_Bar->ScaleByDevice(3) * 2 + m_Bar->ScaleByDevice(22)
        + m_Bar->ScaleByDevice(TREE_BAR_TAB_MINIMUM_IMAGE_HEIGHT) - 1;
    item->SetRect(QRectF(0, 0, 220, tabHeight));
    const QRectF bound = item->GetRect();
    const QRectF thumbnail = item->ThumbnailRect(bound);
    QVERIFY(thumbnail.height()
            < m_Bar->ScaleByDevice(TREE_BAR_TAB_MINIMUM_IMAGE_HEIGHT));

    const QRect titleBand = item->TitleBandRect(bound);
    const QSize faviconSize = m_Bar->ScaleByDevice(QSize(16, 16));
    const QList<QRect> icons = {
        NodeItem::CenteredIconRect(titleBand, faviconSize),
        item->CloseIconRect(),
        item->CloneIconRect(),
        item->SoundIconRect()
    };
    foreach(const QRect &icon, icons)
        QCOMPARE(icon.center().y(), icons.first().center().y());

    const bool closeWasEnabled = TreeBar::m_EnableCloseButton;
    const bool cloneWasEnabled = TreeBar::m_EnableCloneButton;
    TreeBar::m_EnableCloseButton = true;
    TreeBar::m_EnableCloneButton = true;
    const QRectF closeButton = item->CloseButtonRect();
    const QRectF cloneButton = item->CloneButtonRect();
    TreeBar::m_EnableCloseButton = closeWasEnabled;
    TreeBar::m_EnableCloneButton = cloneWasEnabled;
    QCOMPARE(closeButton.center().y(), QRectF(icons.first()).center().y());
    QCOMPARE(cloneButton.center().y(), QRectF(icons.first()).center().y());

    const QRectF textRect = item->TitleTextRect(bound, titleBand);
    QCOMPARE(textRect.top(), qreal(titleBand.top() - m_Bar->ScaleByDevice(1)));

    TearDown();
}

QTEST_MAIN(tst_treebaritem)
#include "tst_treebaritem.moc"
