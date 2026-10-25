#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QUrl>

#include <QFile>
#include <QRegularExpression>

#include <limits>

#include "lightnode.hpp"
#include "treebank.hpp"

#include "testsupport.hpp"

namespace {

const int Unbounded = std::numeric_limits<int>::max();

ViewNode *MakeFolder(ViewNode *parent, const QString &title){
    ViewNode *vn = parent->MakeChild();
    vn->SetTitle(title);
    return vn;
}

ViewNode *MakeTab(ViewNode *parent, const QString &title, const QString &url = QString()){
    ViewNode *vn = parent->MakeChild();
    vn->SetHoldView(true);
    vn->SetTitle(title);
    if(!url.isEmpty()) vn->SetUrl(QUrl(url));
    return vn;
}

QStringList Titles(const NodeList &list){
    QStringList titles;
    foreach(Node *nd, list) titles << nd->GetTitle();
    return titles;
}

QStringList ChildTitles(Node *nd){
    return Titles(nd->GetChildren());
}

}

class tst_viewnode : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void makeChildAppends();
    void makeChildClampsThePosition();
    void makeChildClampsThePosition_data();

    void makeSiblingLandsNextToTheNode();
    void siblingInsertIndexFollowsThePositionSetting();
    void makeParentWrapsTheNode();

    void nextWalksTheTreeDepthFirst();
    void prevWalksTheTreeBackwards();
    void nextAndPrevStopAtTheEnds();

    void theAutoLoadWalkStopsAtTheFirstTabItLoads();
    void theAutoLoadWalkStepsOverRefusedTabsWithoutGrowingTheStack();
    void theAutoLoadWalkCountsEveryNodeItStepsTo();
    void theAutoLoadWalkResumedTickByTickOffersWhatOneWalkOffers();
    void theAutoLoadWalkStopsOnTheTabItLoadsWithinItsBudget();
    void theAutoLoadTimerStopsOnlyAtTheEndsOrTheLimit();
    void aWalkCutShortGoesOnInTheSameDirection();
    void aNodeLeavingTheTreeIsLetGoOfByBothWalks();

    void ancestorsAndDescendants();
    void primaryPath();

    void aTabIsNotADirectory();
    void readableTitleFallsBackToTheUrl();

    void newMakesASiblingTab();
    void cloneCopiesTheSubtree();

    void everyNodeGetsASerialOfItsOwn();
    void aserialIsNeverHandedOutTwice();
    void aRunsSerialsBeginPastTheLastRuns();

    void theNextCurrentIsAViewUnderTheSameParent();
    void theNextCurrentFallsBackToTheParentsChildrenByLastAccess();
    void theNodeWhichIsLeavingIsNeverTheNextCurrent();
    void aViewAnotherWindowIsShowingIsNoCandidate();
    void aDirectoryStandsForThePrimaryChildBelowIt();
    void aLeafWithNothingBesideItLeavesNobodyToBeCurrent();

    void theDownloadCarrierIsTakenApartInOneOrder();
    void nothingAboutADownloadCarrierGoesThroughTheTrash();
    void quarantineLetsGoOfAllThreePointers();
    void theClosedNodeIsStrippedBeforeTheTrashIsTrimmed();
    void onlyAMoveOfTwoOrMoreWhichStaysInTheTrashIsNoticed_data();
    void onlyAMoveOfTwoOrMoreWhichStaysInTheTrashIsNoticed();
    void theTrashsNodeMenuOffersTheWayBackFirst();
    void theNodeMenuGoesFromTheNodeToLeaving();
    void theTabMenuGoesInTheOverviewsOrder();
    void aDiscardedNodeIsNeverFreedInTheTurnItWasBoxed();
    void aDiscardedNodeIsFreedPastTheBarrier();
    void whileTheTreeIsWalkedTheBarrierOnlyRipens();
    void aStrippedViewLetsGoOfItsNode();
    void treeLoadOnlyConsidersJsonGenerations();
    void theRestoreGivesTheWindowOfTheIdItsNode();

    void overviewSharesTheWebViewAreaAndLeavesCompanionsAlone();
    void showingAnAlreadyVisibleEngineViewDoesNotJiggleItsSize();
    void showingATabStampsItsLastAccessWhicheverEngineShowsIt();
    void aFinishedLoadTakesItsThumbnailWhateverTheBarShows();
    void hidingAFullScreenViewLeavesFullScreenWhicheverEngine();
};

namespace {

std::function<bool(Node*)> Refuse(Node *nd = nullptr){
    return [nd](Node *asked){ return nd && asked == nd;};
}

QString TreeBankSource(){
    QFile file(QStringLiteral(VANILLA_SOURCE_DIR) + QStringLiteral("/ui/treebank.cpp"));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(file.readAll());
}

QString SourceFile(const QString &relative){
    QFile file(QStringLiteral(VANILLA_SOURCE_DIR) + relative);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(file.readAll());
}

QString BodyOf(const QString &source, const QString &signature){
    const int at = source.indexOf(signature);
    if(at == -1) return QString();
    int depth = 0;
    for(int i = source.indexOf(QLatin1Char('{'), at); i < source.length(); i++){
        if(source.at(i) == QLatin1Char('{')) depth++;
        else if(source.at(i) == QLatin1Char('}')){
            depth--;
            if(!depth) return source.mid(at, i - at + 1);
        }
    }
    return QString();
}

}

void tst_viewnode::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_viewnode::makeChildAppends(){
    ViewNode *root = new ViewNode();

    MakeTab(root, QStringLiteral("a"));
    MakeTab(root, QStringLiteral("b"));
    MakeTab(root, QStringLiteral("c"));

    QCOMPARE(ChildTitles(root), QStringList()
             << QStringLiteral("a") << QStringLiteral("b") << QStringLiteral("c"));
    QVERIFY(root->GetChildAt(0)->GetParent() == root);

    delete root;
}

void tst_viewnode::makeChildClampsThePosition_data(){
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("expected");

    QTest::newRow("head")     << 0        << 0;
    QTest::newRow("middle")   << 1        << 1;
    QTest::newRow("tail")     << 2        << 2;
    QTest::newRow("int max")  << INT_MAX  << 2;
    QTest::newRow("past end") << 9        << 2;
    QTest::newRow("negative") << -8       << 0;
}

void tst_viewnode::makeChildClampsThePosition(){
    QFETCH(int, position);
    QFETCH(int, expected);

    ViewNode *root = new ViewNode();
    MakeTab(root, QStringLiteral("a"));
    MakeTab(root, QStringLiteral("b"));

    ViewNode *inserted = root->MakeChild(position);
    inserted->SetTitle(QStringLiteral("new"));

    QCOMPARE(root->ChildrenLength(), 3);
    QCOMPARE(root->ChildrenIndexOf(inserted), expected);

    delete root;
}

void tst_viewnode::makeSiblingLandsNextToTheNode(){
    ViewNode *root = new ViewNode();
    ViewNode *a = MakeTab(root, QStringLiteral("a"));
    MakeTab(root, QStringLiteral("b"));

    ViewNode *young = a->MakeSibling();
    young->SetTitle(QStringLiteral("young"));

    QVERIFY(young->GetParent() == root);
    QCOMPARE(ChildTitles(root), QStringList()
             << QStringLiteral("a") << QStringLiteral("young") << QStringLiteral("b"));

    delete root;
}

struct SiblingPosition : ViewNode {
    static void Use(const QString &name){
        if(name == QStringLiteral("RightEnd"))                    m_AddSiblingViewNodePosition = RightEnd;
        if(name == QStringLiteral("LeftEnd"))                     m_AddSiblingViewNodePosition = LeftEnd;
        if(name == QStringLiteral("RightOfPrimary"))              m_AddSiblingViewNodePosition = RightOfPrimary;
        if(name == QStringLiteral("LeftOfPrimary"))               m_AddSiblingViewNodePosition = LeftOfPrimary;
        if(name == QStringLiteral("TailOfRightUnreadsOfPrimary")) m_AddSiblingViewNodePosition = TailOfRightUnreadsOfPrimary;
        if(name == QStringLiteral("HeadOfLeftUnreadsOfPrimary"))  m_AddSiblingViewNodePosition = HeadOfLeftUnreadsOfPrimary;
    }
};

void tst_viewnode::siblingInsertIndexFollowsThePositionSetting(){
    struct Reset { ~Reset(){ SiblingPosition::Use(QStringLiteral("RightOfPrimary")); } } reset;
    ViewNode *root = new ViewNode();
    ViewNode *a = MakeTab(root, QStringLiteral("a"));
    ViewNode *b = MakeTab(root, QStringLiteral("b"));
    ViewNode *primary = MakeTab(root, QStringLiteral("primary"));
    ViewNode *c = MakeTab(root, QStringLiteral("c"));
    ViewNode *d = MakeTab(root, QStringLiteral("d"));
    ViewNode *e = MakeTab(root, QStringLiteral("e"));
    auto read   = [](ViewNode *vn){ vn->SetLastAccessDate(vn->GetCreateDate().addSecs(60)); };
    auto unread = [](ViewNode *vn){ vn->SetLastAccessDate(vn->GetCreateDate()); };
    read(a); unread(b); unread(c); read(d); unread(e);
    auto index = [&](const char *position){
        SiblingPosition::Use(QLatin1String(position));
        return primary->SiblingInsertIndex();
    };

    QCOMPARE(index("RightEnd"), 6);
    QCOMPARE(index("LeftEnd"), 0);
    QCOMPARE(index("RightOfPrimary"), 3);
    QCOMPARE(index("LeftOfPrimary"), 2);
    QCOMPARE(index("TailOfRightUnreadsOfPrimary"), 4);
    QCOMPARE(index("HeadOfLeftUnreadsOfPrimary"), 1);
    unread(a); unread(d);
    QCOMPARE(index("TailOfRightUnreadsOfPrimary"), 6);
    QCOMPARE(index("HeadOfLeftUnreadsOfPrimary"), 0);
    ViewNode *young = primary->MakeSibling();
    QCOMPARE(root->ChildrenIndexOf(young), 0);

    delete root;
}

void tst_viewnode::makeParentWrapsTheNode(){
    ViewNode *root = new ViewNode();
    ViewNode *a = MakeTab(root, QStringLiteral("a"));
    MakeTab(root, QStringLiteral("b"));
    root->SetPrimary(a);

    ViewNode *folder = a->MakeParent();
    QVERIFY(folder);

    QVERIFY(a->GetParent() == folder);
    QVERIFY(folder->GetParent() == root);
    QCOMPARE(ChildTitles(folder), QStringList() << QStringLiteral("a"));
    QCOMPARE(root->ChildrenLength(), 2);
    QVERIFY(!root->ChildrenContains(a));

    QVERIFY(root->GetPrimary() != a);

    delete root;
}

void tst_viewnode::nextWalksTheTreeDepthFirst(){
    ViewNode *root = new ViewNode();

    ViewNode *one = MakeFolder(root, QStringLiteral("1"));
    MakeTab(one, QStringLiteral("1-1"));
    ViewNode *oneTwo = MakeFolder(one, QStringLiteral("1-2"));
    MakeTab(oneTwo, QStringLiteral("1-2-1"));
    MakeTab(root, QStringLiteral("2"));

    QStringList walked;
    for(ViewNode *nd = one; nd; nd = nd->Next())
        walked << nd->GetTitle();

    QCOMPARE(walked, QStringList()
             << QStringLiteral("1")
             << QStringLiteral("1-1")
             << QStringLiteral("1-2")
             << QStringLiteral("1-2-1")
             << QStringLiteral("2"));

    delete root;
}

void tst_viewnode::prevWalksTheTreeBackwards(){
    ViewNode *root = new ViewNode();

    ViewNode *one = MakeFolder(root, QStringLiteral("1"));
    MakeTab(one, QStringLiteral("1-1"));
    ViewNode *oneTwo = MakeFolder(one, QStringLiteral("1-2"));
    MakeTab(oneTwo, QStringLiteral("1-2-1"));
    ViewNode *two = MakeTab(root, QStringLiteral("2"));

    QStringList walked;
    for(ViewNode *nd = two; nd && !nd->IsRoot(); nd = nd->Prev())
        walked << nd->GetTitle();

    QCOMPARE(walked, QStringList()
             << QStringLiteral("2")
             << QStringLiteral("1-2-1")
             << QStringLiteral("1-2")
             << QStringLiteral("1-1")
             << QStringLiteral("1"));

    delete root;
}

void tst_viewnode::nextAndPrevStopAtTheEnds(){
    ViewNode *root = new ViewNode();
    ViewNode *only = MakeTab(root, QStringLiteral("only"));

    QVERIFY(!only->Next());
    QVERIFY(only->Prev() == root);
    QVERIFY(!root->Prev());

    delete root;
}

void tst_viewnode::theAutoLoadWalkStopsAtTheFirstTabItLoads(){
    ViewNode *root = new ViewNode();
    ViewNode *one = MakeFolder(root, QStringLiteral("1"));
    ViewNode *start = MakeTab(one, QStringLiteral("1-1"), QStringLiteral("https://a.test/"));
    MakeTab(one, QStringLiteral("1-2"));
    MakeTab(one, QStringLiteral("1-3"), QStringLiteral("https://b.test/"));
    ViewNode *two = MakeTab(root, QStringLiteral("2"), QStringLiteral("https://c.test/"));
    MakeTab(root, QStringLiteral("3"), QStringLiteral("https://d.test/"));

    QStringList offered;
    ViewNode *iter = start;
    TreeBank::WalkToAutoLoad(iter, true, [&](ViewNode *vn){
        offered << vn->GetTitle();
        return vn == two;
    }, Unbounded);
    QCOMPARE(offered, QStringList() << QStringLiteral("1-3") << QStringLiteral("2"));
    QVERIFY(iter == two);

    offered.clear();
    iter = two;
    TreeBank::WalkToAutoLoad(iter, false, [&](ViewNode *vn){
        offered << vn->GetTitle();
        return false;
    }, Unbounded);
    QCOMPARE(offered, QStringList() << QStringLiteral("1-3") << QStringLiteral("1-1"));
    QVERIFY(!iter);

    delete root;
}

void tst_viewnode::theAutoLoadWalkStepsOverRefusedTabsWithoutGrowingTheStack(){
    static const int Folders = 100;
    static const int TabsPerFolder = 1000;

    ViewNode *root = new ViewNode();
    ViewNode *start = MakeTab(root, QStringLiteral("start"), QStringLiteral("https://start.test/"));
    for(int i = 0; i < Folders; i++){
        ViewNode *folder = MakeFolder(root, QString());
        for(int j = 0; j < TabsPerFolder; j++)
            MakeTab(folder, QString(), QStringLiteral("about:blank"));
    }

    int offered = 0;
    ViewNode *iter = start;
    QCOMPARE(TreeBank::WalkToAutoLoad(iter, true, [&](ViewNode *){
        offered++;
        return false;
    }, Unbounded), TreeBank::AutoLoadExhausted);
    QCOMPARE(offered, Folders * TabsPerFolder);
    QVERIFY(!iter);

    offered = 0;
    iter = start;
    int ticks = 0;
    TreeBank::AutoLoadWalk walk;
    do {
        const int before = offered;
        walk = TreeBank::WalkToAutoLoad(iter, true, [&](ViewNode *){
            offered++;
            return false;
        }, AUTO_LOAD_WALK_BUDGET);
        QVERIFY(offered - before <= AUTO_LOAD_WALK_BUDGET);
        ticks++;
        QVERIFY(ticks <= Folders * (TabsPerFolder + 1) / AUTO_LOAD_WALK_BUDGET + 2);
    } while(walk == TreeBank::AutoLoadBudgetSpent);
    QCOMPARE(walk, TreeBank::AutoLoadExhausted);
    QCOMPARE(offered, Folders * TabsPerFolder);
    QVERIFY(ticks > 1);
    QVERIFY(!iter);

    delete root;
}

void tst_viewnode::theAutoLoadWalkCountsEveryNodeItStepsTo(){
    ViewNode *root = new ViewNode();
    ViewNode *start = MakeTab(root, QStringLiteral("start"), QStringLiteral("https://start.test/"));
    ViewNode *folder = MakeFolder(root, QStringLiteral("folder"));
    ViewNode *blank = MakeTab(folder, QStringLiteral("blank"));
    ViewNode *a = MakeTab(folder, QStringLiteral("a"), QStringLiteral("https://a.test/"));
    MakeTab(root, QStringLiteral("b"), QStringLiteral("https://b.test/"));

    QStringList offered;
    auto refuse = [&](ViewNode *vn){ offered << vn->GetTitle(); return false;};

    ViewNode *iter = start;
    QCOMPARE(TreeBank::WalkToAutoLoad(iter, true, refuse, 2), TreeBank::AutoLoadBudgetSpent);
    QVERIFY(offered.isEmpty());
    QVERIFY(iter == blank);

    QCOMPARE(TreeBank::WalkToAutoLoad(iter, true, refuse, 1), TreeBank::AutoLoadBudgetSpent);
    QCOMPARE(offered, QStringList() << QStringLiteral("a"));
    QVERIFY(iter == a);

    QCOMPARE(TreeBank::WalkToAutoLoad(iter, true, refuse, 0), TreeBank::AutoLoadBudgetSpent);
    QVERIFY(iter == a);

    QCOMPARE(TreeBank::WalkToAutoLoad(iter, true, refuse, 1), TreeBank::AutoLoadBudgetSpent);
    QCOMPARE(TreeBank::WalkToAutoLoad(iter, true, refuse, 1), TreeBank::AutoLoadExhausted);
    QCOMPARE(offered, QStringList() << QStringLiteral("a") << QStringLiteral("b"));
    QVERIFY(!iter);

    delete root;
}

void tst_viewnode::theAutoLoadWalkResumedTickByTickOffersWhatOneWalkOffers(){
    ViewNode *root = new ViewNode();
    QList<ViewNode*> all;
    ViewNode *parent = root;
    for(int i = 0; i < 60; i++){
        ViewNode *vn = nullptr;
        switch(i % 5){
        case 0: vn = MakeFolder(parent, QStringLiteral("f%1").arg(i)); break;
        case 3: vn = MakeTab(parent, QStringLiteral("blank%1").arg(i)); break;
        default: vn = MakeTab(parent, QStringLiteral("t%1").arg(i),
                              QStringLiteral("https://t%1.test/").arg(i)); break;
        }
        all << vn;
        if(i % 10 == 0) parent = vn;
        else if(i % 13 == 0) parent = root;
    }

    for(int pass = 0; pass < 2; pass++){
        const bool forward = pass == 0;
        ViewNode *from = forward ? all.first() : all.last();

        QStringList whole;
        ViewNode *iter = from;
        TreeBank::WalkToAutoLoad(iter, forward, [&](ViewNode *vn){
            whole << vn->GetTitle();
            return false;
        }, Unbounded);
        QVERIFY(!whole.isEmpty());

        for(int budget = 1; budget <= 7; budget++){
            QStringList ticked;
            iter = from;
            TreeBank::AutoLoadWalk walk;
            int ticks = 0;
            do {
                walk = TreeBank::WalkToAutoLoad(iter, forward, [&](ViewNode *vn){
                    ticked << vn->GetTitle();
                    return false;
                }, budget);
                QVERIFY(++ticks < 1000);
            } while(walk == TreeBank::AutoLoadBudgetSpent);
            QCOMPARE(walk, TreeBank::AutoLoadExhausted);
            QCOMPARE(ticked, whole);
            QVERIFY(!iter);
        }
    }

    delete root;
}

void tst_viewnode::theAutoLoadWalkStopsOnTheTabItLoadsWithinItsBudget(){
    ViewNode *root = new ViewNode();
    ViewNode *start = MakeTab(root, QStringLiteral("start"), QStringLiteral("https://start.test/"));
    MakeTab(root, QStringLiteral("a"), QStringLiteral("https://a.test/"));
    ViewNode *b = MakeTab(root, QStringLiteral("b"), QStringLiteral("https://b.test/"));
    MakeTab(root, QStringLiteral("c"), QStringLiteral("https://c.test/"));

    QStringList offered;
    auto takeB = [&](ViewNode *vn){ offered << vn->GetTitle(); return vn == b;};
    ViewNode *iter = start;
    QCOMPARE(TreeBank::WalkToAutoLoad(iter, true, takeB, 100), TreeBank::AutoLoadLoaded);
    QVERIFY(iter == b);
    QCOMPARE(TreeBank::WalkToAutoLoad(iter, true, takeB, 100), TreeBank::AutoLoadExhausted);
    QCOMPARE(offered, QStringList() << QStringLiteral("a") << QStringLiteral("b") << QStringLiteral("c"));

    delete root;
}

void tst_viewnode::theAutoLoadTimerStopsOnlyAtTheEndsOrTheLimit(){
    QVERIFY( TreeBank::AutoLoadStops(false, false, 0, 0));
    QVERIFY( TreeBank::AutoLoadStops(false, false, 0, 10));
    QVERIFY(!TreeBank::AutoLoadStops(true,  false, 3, 10));
    QVERIFY(!TreeBank::AutoLoadStops(false, true,  9, 10));
    QVERIFY( TreeBank::AutoLoadStops(true,  false, 10, 10));
    QVERIFY( TreeBank::AutoLoadStops(true,  true,  11, 10));
    QVERIFY(!TreeBank::AutoLoadStops(true,  false, 0, 0));
    QVERIFY(!TreeBank::AutoLoadStops(true,  true,  500, 0));
    QVERIFY( TreeBank::AutoLoadStops(true,  true,  0, -2));
}

void tst_viewnode::aNodeLeavingTheTreeIsLetGoOfByBothWalks(){
    ViewNode *root = new ViewNode();
    ViewNode *folder = MakeFolder(root, QStringLiteral("folder"));
    ViewNode *refused = MakeTab(folder, QStringLiteral("refused"), QStringLiteral("https://r.test/"));
    ViewNode *other = MakeTab(root, QStringLiteral("other"), QStringLiteral("https://o.test/"));
    QVERIFY(!folder->GetView() && !refused->GetView());

    TreeBank::SetViewIterForward(folder);
    TreeBank::SetViewIterBackward(refused);
    TreeBank::ReleaseAutoLoadIterators(other);
    QVERIFY(TreeBank::GetViewIterForward() == folder);
    QVERIFY(TreeBank::GetViewIterBackward() == refused);

    TreeBank::ReleaseAutoLoadIterators(folder);
    QVERIFY(!TreeBank::GetViewIterForward());
    QVERIFY(TreeBank::GetViewIterBackward() == refused);
    TreeBank::ReleaseAutoLoadIterators(refused);
    QVERIFY(!TreeBank::GetViewIterBackward());

    TreeBank::SetViewIterForward(other);
    TreeBank::SetViewIterBackward(other);
    TreeBank::ReleaseAutoLoadIterators(other);
    QVERIFY(!TreeBank::GetViewIterForward() && !TreeBank::GetViewIterBackward());

    delete root;
}

void tst_viewnode::aWalkCutShortGoesOnInTheSameDirection(){
    QCOMPARE(TreeBank::AutoLoadNextCondition(TreeBank::AutoLoadBudgetSpent, 0), 0);
    QCOMPARE(TreeBank::AutoLoadNextCondition(TreeBank::AutoLoadBudgetSpent, 1), 1);
    QCOMPARE(TreeBank::AutoLoadNextCondition(TreeBank::AutoLoadLoaded, 0), 1);
    QCOMPARE(TreeBank::AutoLoadNextCondition(TreeBank::AutoLoadLoaded, 1), 0);
    QCOMPARE(TreeBank::AutoLoadNextCondition(TreeBank::AutoLoadExhausted, 0), 1);
    QCOMPARE(TreeBank::AutoLoadNextCondition(TreeBank::AutoLoadExhausted, 1), 0);
}

void tst_viewnode::ancestorsAndDescendants(){
    ViewNode *root = new ViewNode();
    ViewNode *folder = MakeFolder(root, QStringLiteral("folder"));
    ViewNode *nested = MakeFolder(folder, QStringLiteral("nested"));
    ViewNode *leaf = MakeTab(nested, QStringLiteral("leaf"));

    QCOMPARE(Titles(leaf->GetAncestors()), QStringList()
             << QStringLiteral("nested") << QStringLiteral("folder") << QString());

    QVERIFY(root->IsAncestorOf(leaf));
    QVERIFY(leaf->IsDescendantOf(root));
    QVERIFY(!leaf->IsAncestorOf(root));

    QVERIFY(folder->IsParentOf(nested));
    QVERIFY(nested->IsChildOf(folder));
    QVERIFY(nested->IsSiblingOf(nested));

    QCOMPARE(root->GetDescendants().length(), 3);
    QVERIFY(root->GetRoot() == root);
    QVERIFY(leaf->GetRoot() == root);

    delete root;
}

void tst_viewnode::primaryPath(){
    ViewNode *root = new ViewNode();
    ViewNode *folder = MakeFolder(root, QStringLiteral("folder"));
    ViewNode *nested = MakeFolder(folder, QStringLiteral("nested"));
    ViewNode *leaf = MakeTab(nested, QStringLiteral("leaf"));
    MakeTab(nested, QStringLiteral("other"));

    leaf->ResetPrimaryPath();

    QVERIFY(leaf->IsPrimaryOfParent());
    QVERIFY(nested->IsPrimaryOfParent());
    QVERIFY(folder->IsPrimaryOfParent());
    QVERIFY(root->GetPrimary() == folder);

    delete root;
}

void tst_viewnode::aTabIsNotADirectory(){
    ViewNode *root = new ViewNode();
    ViewNode *tab = MakeTab(root, QStringLiteral("tab"), QStringLiteral("https://example.com/"));
    ViewNode *folder = MakeFolder(root, QStringLiteral("folder"));

    QVERIFY(tab->HoldsView());
    QVERIFY(!tab->IsDirectory());
    QVERIFY(!tab->TitleEditable());

    QVERIFY(!folder->HoldsView());
    QVERIFY(folder->IsDirectory());
    QVERIFY(folder->TitleEditable());

    QVERIFY(root->IsRoot());
    QVERIFY(!folder->IsRoot());

    delete root;
}

void tst_viewnode::readableTitleFallsBackToTheUrl(){
    ViewNode *root = new ViewNode();

    ViewNode *titled = MakeTab(root, QStringLiteral("a title"), QStringLiteral("https://example.com/"));
    QCOMPARE(titled->ReadableTitle(), QStringLiteral("a title"));

    ViewNode *untitled = MakeTab(root, QString(), QStringLiteral("https://example.com/"));
    QCOMPARE(untitled->ReadableTitle(), QStringLiteral("https://example.com/"));

    ViewNode *blank = MakeTab(root, QString());
    QCOMPARE(blank->ReadableTitle(), QStringLiteral("No Title"));

    ViewNode *folder = MakeFolder(root, QString());
    QCOMPARE(folder->ReadableTitle(), QStringLiteral("Directory"));

    ViewNode *configured = MakeFolder(root, QStringLiteral("name;id;noload"));
    QCOMPARE(configured->ReadableTitle(), QStringLiteral("name"));

    delete root;
}

void tst_viewnode::newMakesASiblingTab(){
    ViewNode *root = new ViewNode();
    ViewNode *a = MakeTab(root, QStringLiteral("a"));

    ViewNode *fresh = a->New();
    QVERIFY(fresh);
    QVERIFY(fresh->GetParent() == root);
    QVERIFY(fresh->HoldsView());
    QCOMPARE(fresh->GetUrl(), START_URL);

    delete root;
}

void tst_viewnode::cloneCopiesTheSubtree(){
    ViewNode *root = new ViewNode();
    ViewNode *folder = MakeFolder(root, QStringLiteral("folder"));
    ViewNode *tab = MakeTab(folder, QStringLiteral("tab"), QStringLiteral("https://example.com/"));
    tab->SetScrollX(3);
    tab->SetScrollY(4);
    tab->SetZoom(1.5f);
#ifdef MEDIATIME
    tab->SetMediaTime(78.5f);
#endif
    folder->SetPrimary(tab);
    folder->SetFolded(false);

    ViewNode *clone = folder->Clone();
    QVERIFY(clone);
    QVERIFY(clone != folder);
    QVERIFY(clone->GetParent() == root);
    QCOMPARE(clone->GetTitle(), QStringLiteral("folder"));
    QVERIFY(!clone->GetFolded());
    QCOMPARE(clone->ChildrenLength(), 1);

    ViewNode *clonedTab = clone->GetChildAt(0)->ToViewNode();
    QVERIFY(clonedTab);
    QVERIFY(clonedTab != tab);
    QCOMPARE(clonedTab->GetUrl(), QUrl(QStringLiteral("https://example.com/")));
    QCOMPARE(clonedTab->GetScrollX(), 3);
    QCOMPARE(clonedTab->GetScrollY(), 4);
    QCOMPARE(clonedTab->GetZoom(), 1.5f);
#ifdef MEDIATIME
    QCOMPARE(clonedTab->GetMediaTime(), 78.5f);
#endif
    QVERIFY(clone->GetPrimary() == clonedTab);

    delete root;
}

void tst_viewnode::everyNodeGetsASerialOfItsOwn(){
    ViewNode *root = new ViewNode();
    ViewNode *a = MakeTab(root, QStringLiteral("a"));
    ViewNode *b = MakeTab(root, QStringLiteral("b"));

    QVERIFY(root->GetSerial() != 0);
    QVERIFY(a->GetSerial() != b->GetSerial());
    QVERIFY(a->GetSerial() != root->GetSerial());

    delete root;
}

void tst_viewnode::aserialIsNeverHandedOutTwice(){
    ViewNode *root = new ViewNode();

    ViewNode *first = MakeTab(root, QStringLiteral("first"));
    const quint64 serial = first->GetSerial();
    const quintptr address = reinterpret_cast<quintptr>(first);

    root->RemoveChild(first);
    first->Delete();

    bool addressWasReused = false;
    for(int i = 0; i < 64; i++){
        ViewNode *next = MakeTab(root, QStringLiteral("next"));
        QVERIFY(next->GetSerial() != serial);
        QVERIFY(next->GetSerial() > serial);
        if(reinterpret_cast<quintptr>(next) == address) addressWasReused = true;
    }

    delete root;
}

void tst_viewnode::theNextCurrentIsAViewUnderTheSameParent(){
    ViewNode *root = new ViewNode();
    ViewNode *dir = MakeFolder(root, QStringLiteral("dir"));
    ViewNode *leaving = MakeTab(dir, QStringLiteral("leaving"));
    ViewNode *beside = MakeTab(dir, QStringLiteral("beside"));
    ViewNode *elsewhere = MakeTab(root, QStringLiteral("elsewhere"));

    const NodeList views = NodeList() << elsewhere << leaving << beside;

    QCOMPARE(TreeBank::ChooseCurrentAfterDelete(views, dir, leaving, Refuse()),
             static_cast<Node*>(beside));

    delete root;
}

void tst_viewnode::theNextCurrentFallsBackToTheParentsChildrenByLastAccess(){
    ViewNode *root = new ViewNode();
    ViewNode *dir = MakeFolder(root, QStringLiteral("dir"));
    ViewNode *leaving = MakeTab(dir, QStringLiteral("leaving"));
    ViewNode *older = MakeTab(dir, QStringLiteral("older"));
    ViewNode *newer = MakeTab(dir, QStringLiteral("newer"));

    older->SetLastAccessDate(QDateTime::currentDateTime().addSecs(-60));
    newer->SetLastAccessDate(QDateTime::currentDateTime());

    QCOMPARE(TreeBank::ChooseCurrentAfterDelete(NodeList(), dir, leaving, Refuse()),
             static_cast<Node*>(newer));

    older->SetLastAccessDate(QDateTime::currentDateTime().addSecs(60));
    QCOMPARE(TreeBank::ChooseCurrentAfterDelete(NodeList(), dir, leaving, Refuse()),
             static_cast<Node*>(older));

    delete root;
}

void tst_viewnode::theNodeWhichIsLeavingIsNeverTheNextCurrent(){
    ViewNode *root = new ViewNode();
    ViewNode *dir = MakeFolder(root, QStringLiteral("dir"));
    ViewNode *leaving = MakeTab(dir, QStringLiteral("leaving"));

    QCOMPARE(TreeBank::ChooseCurrentAfterDelete(NodeList() << leaving, dir, leaving, Refuse()),
             static_cast<Node*>(nullptr));

    delete root;
}

void tst_viewnode::aViewAnotherWindowIsShowingIsNoCandidate(){
    ViewNode *root = new ViewNode();
    ViewNode *dir = MakeFolder(root, QStringLiteral("dir"));
    ViewNode *leaving = MakeTab(dir, QStringLiteral("leaving"));
    ViewNode *theirs = MakeTab(root, QStringLiteral("theirs"));

    QCOMPARE(TreeBank::ChooseCurrentAfterDelete(NodeList() << theirs, dir, leaving,
                                                Refuse(theirs)),
             static_cast<Node*>(nullptr));

    QCOMPARE(TreeBank::ChooseCurrentAfterDelete(NodeList() << theirs, dir, leaving,
                                                Refuse()),
             static_cast<Node*>(theirs));

    delete root;
}

void tst_viewnode::aDirectoryStandsForThePrimaryChildBelowIt(){
    ViewNode *root = new ViewNode();
    ViewNode *dir = MakeFolder(root, QStringLiteral("dir"));
    ViewNode *leaving = MakeTab(dir, QStringLiteral("leaving"));

    ViewNode *other = MakeFolder(root, QStringLiteral("other"));
    ViewNode *first = MakeTab(other, QStringLiteral("first"));
    ViewNode *primary = MakeTab(other, QStringLiteral("primary"));

    QCOMPARE(TreeBank::ChooseCurrentAfterDelete(NodeList() << other, dir, leaving, Refuse()),
             static_cast<Node*>(first));

    other->SetPrimary(primary);
    QCOMPARE(TreeBank::ChooseCurrentAfterDelete(NodeList() << other, dir, leaving, Refuse()),
             static_cast<Node*>(primary));

    delete root;
}

void tst_viewnode::aLeafWithNothingBesideItLeavesNobodyToBeCurrent(){
    ViewNode *root = new ViewNode();
    ViewNode *dir = MakeFolder(root, QStringLiteral("dir"));
    ViewNode *leaving = MakeTab(dir, QStringLiteral("leaving"));

    MakeFolder(root, QStringLiteral("empty"));

    QCOMPARE(TreeBank::ChooseCurrentAfterDelete(NodeList() << leaving, dir, leaving, Refuse()),
             static_cast<Node*>(nullptr));

    delete root;
}

void tst_viewnode::theDownloadCarrierIsTakenApartInOneOrder(){
    const QString source = TreeBankSource();
    QVERIFY2(!source.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");

    const QString body =
        BodyOf(source, QStringLiteral("SharedView TreeBank::ExtractDownloadCarrier"));
    QVERIFY(!body.isEmpty());

    const QStringList steps = QStringList()
        << QStringLiteral("GetThis().lock()")
        << QStringLiteral("QuarantineViewNode(vn)")
        << QStringLiteral("DislinkView(vn)")
        << QStringLiteral("held->Orphan()")
        << QStringLiteral("DisownNode(vn)")
        << QStringLiteral("EmitNodeDeleted(deleted)")
        << QStringLiteral("ForgetNodeItems(deleted)")
        << QStringLiteral("Discard(vn, prevparent)")
        << QStringLiteral("SetCurrent(next)");

    int at = -1;
    foreach(const QString &step, steps){
        const int found = body.indexOf(step);
        QVERIFY2(found != -1, qPrintable(QStringLiteral("missing step: ") + step));
        QVERIFY2(found > at, qPrintable(QStringLiteral("out of order: ") + step));
        at = found;
    }

    QVERIFY(body.indexOf(QStringLiteral("FindCurrentAfterDelete")) <
            body.indexOf(QStringLiteral("QuarantineViewNode(vn)")));

    QVERIFY(body.contains(QStringLiteral("HasNoChildren()")));
    QVERIFY(!body.contains(QStringLiteral("->Delete()")));
}

void tst_viewnode::nothingAboutADownloadCarrierGoesThroughTheTrash(){
    const QString source = TreeBankSource();
    QVERIFY2(!source.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");

    const QString body =
        BodyOf(source, QStringLiteral("SharedView TreeBank::ExtractDownloadCarrier"));
    QVERIFY(!body.isEmpty());

    QVERIFY(!body.contains(QStringLiteral("MoveToTrash")));
    QVERIFY(!body.contains(QStringLiteral("m_TrashRoot")));
}

void tst_viewnode::quarantineLetsGoOfAllThreePointers(){
    const QString source = TreeBankSource();
    QVERIFY2(!source.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");

    const QString body =
        BodyOf(source, QStringLiteral("void TreeBank::QuarantineViewNode"));
    QVERIFY(!body.isEmpty());

    QVERIFY(body.contains(QStringLiteral("SetCurrentViewNode(nullptr)")));
    const int release = body.indexOf(QStringLiteral("ReleaseAutoLoadIterators(vn)"));
    QVERIFY(release >= 0);
    QVERIFY(release < body.indexOf(QStringLiteral("if(!tb) return;")));
}

void tst_viewnode::theClosedNodeIsStrippedBeforeTheTrashIsTrimmed(){
    const QString source = TreeBankSource();
    QVERIFY2(!source.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");

    const QString body = BodyOf(source, QStringLiteral("bool TreeBank::MoveToTrash"));
    QVERIFY(!body.isEmpty());

    const int strip = body.indexOf(QStringLiteral("StripSubTree(vn);"));
    const int trim = body.indexOf(QStringLiteral("while(m_TrashRoot->ChildrenLength()"));
    QVERIFY(strip >= 0 && trim >= 0);
    QVERIFY(strip < trim);
    QCOMPARE(body.count(QStringLiteral("StripSubTree(vn);")), 1);
    QVERIFY(body.contains(QStringLiteral("qMax(0, m_MaxTrashEntryCount)")));
    QVERIFY(body.contains(QStringLiteral("ChildrenLength() > keep")));

    QVERIFY(body.indexOf(QStringLiteral("DiscardFromTrash(m_TrashRoot->GetLastChild());")) > trim);
    const QString steps = BodyOf(source, QStringLiteral("bool TreeBank::DiscardFromTrash"));
    QVERIFY(!steps.isEmpty());
    const int disown = steps.indexOf(QStringLiteral("DisownNode(nd);"));
    const int stripped = steps.indexOf(QStringLiteral("StripSubTree(nd);"));
    const int forget = steps.indexOf(QStringLiteral("ForgetNodeItems(forgotten);"));
    const int discard = steps.indexOf(QStringLiteral("Discard(nd, parent);"));
    QVERIFY(disown >= 0 && disown < stripped && stripped < forget && forget < discard);
    QVERIFY(!steps.contains(QStringLiteral("->Delete()")));
    QVERIFY(!body.contains(QStringLiteral("->Delete()")));
}

void tst_viewnode::onlyAMoveOfTwoOrMoreWhichStaysInTheTrashIsNoticed_data(){
    QTest::addColumn<int>("moved");
    QTest::addColumn<int>("keep");
    QTest::addColumn<int>("kept");
    QTest::addColumn<bool>("noticed");

    QTest::newRow("one")              << 1 << 25 <<  1 << false;
    QTest::newRow("two")              << 2 << 25 <<  2 << true;
    QTest::newRow("more than kept")   << 5 <<  3 <<  3 << true;
    QTest::newRow("only one kept")    << 3 <<  1 <<  1 << true;
    QTest::newRow("trash of none")    << 3 <<  0 <<  0 << false;
    QTest::newRow("below none")       << 3 << -1 <<  0 << false;
}

void tst_viewnode::onlyAMoveOfTwoOrMoreWhichStaysInTheTrashIsNoticed(){
    QFETCH(int, moved);
    QFETCH(int, keep);
    QFETCH(int, kept);
    QFETCH(bool, noticed);

    QCOMPARE(TreeBank::KeptInTrash(moved, keep), kept);
    QCOMPARE(TreeBank::NoticesTrash(moved, keep), noticed);

    const QString body = BodyOf(TreeBankSource(),
                                QStringLiteral("bool TreeBank::DeleteNode(NodeList list)"));
    QVERIFY(!body.isEmpty());
    QCOMPARE(body.count(QStringLiteral("NoticeTrash(moved)")), 1);
    QVERIFY(body.indexOf(QStringLiteral("NoticeTrash(moved)"))
            > body.indexOf(QStringLiteral("if(!moved) return false;")));
}

void tst_viewnode::theTrashsNodeMenuOffersTheWayBackFirst(){
    const QString menu = BodyOf(SourceFile(QStringLiteral("/gadgets/gadgets.cpp")),
                                QStringLiteral("QMenu *Gadgets::CreateNodeMenu()"));
    QVERIFY(!menu.isEmpty());

    const int trashAt = menu.indexOf(QStringLiteral("if(trash && (hovered || selected)){"));
    const int firstAt = menu.indexOf(QStringLiteral("menu->addAction("));
    const int treeAt = menu.indexOf(QStringLiteral("if(!trash){"));
    const int endAt = menu.indexOf(QStringLiteral("menu->addAction(toggleTrash);"), treeAt);
    QVERIFY(trashAt >= 0 && treeAt > trashAt && endAt > treeAt);
    QVERIFY(firstAt > trashAt);
    const QString inTrash = menu.mid(trashAt, menu.indexOf(QLatin1Char('}'), trashAt) - trashAt);
    const int elseAt = menu.indexOf(QStringLiteral("} else if(hovered || selected){"), treeAt);
    QVERIFY(elseAt > treeAt && elseAt < endAt);
    const QString inTree = menu.mid(treeAt, elseAt - treeAt);
    const QString inTrashDeletes = menu.mid(elseAt, menu.indexOf(QLatin1Char('}'), elseAt + 1) - elseAt);
    QVERIFY(inTrashDeletes.contains(QStringLiteral("menu->addAction(deleting);")));
    QCOMPARE(inTrashDeletes.count(QStringLiteral("addAction(")), 1);

    QVERIFY(inTrash.contains(QStringLiteral("menu->addAction(restore);")));
    QCOMPARE(menu.indexOf(QStringLiteral("menu->addAction(restore);")),
             menu.indexOf(QStringLiteral("menu->addAction("), trashAt));
    for(const QString &name : { QStringLiteral("_DeleteRightNode"),
                                QStringLiteral("_DeleteLeftNode"), QStringLiteral("_DeleteOtherNode"),
                                QStringLiteral("_NewNode"), QStringLiteral("_MakeDirectory"),
                                QStringLiteral("_PasteNode"), QStringLiteral("_CloneNode")}){
        QVERIFY2(!inTrash.contains(name), qPrintable(name));
        if(name != QStringLiteral("_CloneNode"))
            QCOMPARE(menu.count(name + QLatin1Char(')')), inTree.count(name + QLatin1Char(')')));
    }
    QVERIFY(menu.contains(QStringLiteral("if(!trash) menu->addAction(Action(_CloneNode));")));

    const int guard = inTree.indexOf(QStringLiteral("if(!TreeBank::GetTrashRoot()->HasNoChildren())"));
    QVERIFY(guard >= 0);
    QVERIFY(inTree.indexOf(QStringLiteral("addAction(restore)"), guard) > guard);

    QCOMPARE(menu.count(QStringLiteral("Action(_DeleteNode)")), 1);
    QVERIFY(menu.contains(QStringLiteral("!trash    ? tr(\"DeleteNode\")")));
    QVERIFY(inTree.contains(QStringLiteral("menu->addAction(deleting);")));
    QVERIFY(!inTrash.contains(QStringLiteral("deleting")));

    const QString restore = BodyOf(SourceFile(QStringLiteral("/gadgets/graphicstableview.cpp")),
                                   QStringLiteral("bool GraphicsTableView::ThumbList_RestoreNode()"));
    QVERIFY(!restore.isEmpty());
    QVERIFY(restore.contains(QStringLiteral("TreeBank::IsTrash(nd) && nd->IsViewNode()")));
    QVERIFY(restore.indexOf(QStringLiteral("if(!TreeBank::IsTrash(m_CurrentNode)){"))
            > restore.indexOf(QStringLiteral("m_TreeBank->Restore(nd->ToViewNode(), nullptr);")));

    const QString deleting = BodyOf(SourceFile(QStringLiteral("/gadgets/graphicstableview.cpp")),
                                    QStringLiteral("bool GraphicsTableView::DeleteNodes(NodeList list)"));
    QVERIFY(!deleting.isEmpty());
    const int refused = deleting.indexOf(QStringLiteral("if(m_DisplayType == TrashTree){"));
    QVERIFY(refused >= 0);
    QVERIFY(refused < deleting.indexOf(QStringLiteral("TreeBank::DiscardFromTrashAsked(")));
    QVERIFY(deleting.indexOf(QStringLiteral("TreeBank::DiscardFromTrashAsked("))
            < deleting.indexOf(QStringLiteral("setVisible(false)")));
    const int failed = deleting.indexOf(QStringLiteral("if(!success){"));
    QVERIFY(failed >= 0);
    const int remade = deleting.indexOf(QStringLiteral("ThumbList_RefreshNoScroll();"), failed);
    QVERIFY(remade > failed);
    QVERIFY(remade < deleting.indexOf(QStringLiteral("return false;"), failed));
}

void tst_viewnode::theNodeMenuGoesFromTheNodeToLeaving(){
    const QString menu = BodyOf(SourceFile(QStringLiteral("/gadgets/gadgets.cpp")),
                                QStringLiteral("QMenu *Gadgets::CreateNodeMenu()"));
    QVERIFY(!menu.isEmpty());

    const QStringList order = {
        QStringLiteral("Action(_OpenNode)"),
        QStringLiteral("Action(_OpenNodeOnNewWindow)"),
        QStringLiteral("tr(\"OpenNodeWithOtherBrowser\")"),
        QStringLiteral("Action(_RenameNode)"),
        QStringLiteral("Action(_CopyNodeUrl)"),
        QStringLiteral("Action(_CloneNode)"),
        QStringLiteral("DirectoryPage::CreateSettingsMenu"),
        QStringLiteral("Action(_NewNode)"),
        QStringLiteral("Action(_MakeDirectory)"),
        QStringLiteral("Action(_PasteNode)"),
        QStringLiteral("Action(_MakeDirectoryWithSelectedNode)"),
        QStringLiteral("Action(_MakeDirectoryWithSameDomainNode)"),
        QStringLiteral("if(hovered || selected)\n            menu->addAction(deleting);"),
        QStringLiteral("tr(\"DeleteNodesTogether\")"),
        QStringLiteral("if(!TreeBank::GetTrashRoot()->HasNoChildren())\n            menu->addAction(restore);"),
        QStringLiteral("} else if(hovered || selected){"),
        QStringLiteral("Action(_UpDirectory)"),
        QStringLiteral("Action(_DownDirectory)"),
        QStringLiteral("menu->addAction(toggleTrash);"),
        QStringLiteral("CreateSortMenu(menu)"),
        QStringLiteral("CreateNodeCollectionTypeMenu(menu)"),
        QStringLiteral("Action(_Refresh)"),
        QStringLiteral("menu->addAction(deactivate);"),
    };
    int last = -1;
    for(const QString &item : order){
        const int at = menu.indexOf(item);
        QVERIFY2(at > last, qPrintable(item));
        QCOMPARE(menu.count(item), 1);
        last = at;
    }

    const int together = menu.indexOf(QStringLiteral("tr(\"DeleteNodesTogether\")"));
    const int added = menu.indexOf(QStringLiteral("menu->addMenu(m);"), together);
    for(const QString &item : { QStringLiteral("m->addAction(Action(_DeleteRightNode));"),
                                QStringLiteral("m->addAction(Action(_DeleteLeftNode));"),
                                QStringLiteral("m->addAction(Action(_DeleteOtherNode));") }){
        const int at = menu.indexOf(item);
        QVERIFY2(at > together && at < added, qPrintable(item));
    }

    QVERIFY(menu.contains(QStringLiteral("deactivate->setText(tr(\"CloseViewTree\"));")));
    QVERIFY(menu.contains(QStringLiteral("toggleTrash->setText(trash ? tr(\"BackToViewTree\") : tr(\"ShowTrash\"));")));
}

void tst_viewnode::theTabMenuGoesInTheOverviewsOrder(){
    const QString source = SourceFile(QStringLiteral("/ui/treebar.cpp"));
    const QString menu = BodyOf(source, QStringLiteral("QMenu *NodeItem::NodeMenu()"));
    QVERIFY(!menu.isEmpty());

    const QStringList order = {
        QStringLiteral("tr(\"OpenViewNode\")"),
        QStringLiteral("tr(\"OpenViewNodeOnNewWindow\")"),
        QStringLiteral("tr(\"OpenViewNodeWithOtherBrowser\")"),
        QStringLiteral("tr(\"RenameViewNode\")"),
        QStringLiteral("tr(\"ReloadViewNode\")"),
        QStringLiteral("tr(\"CloneViewNode\")"),
        QStringLiteral("tr(\"NewViewNode\")"),
        QStringLiteral("tr(\"MakeDirectory\")"),
        QStringLiteral("tr(\"MakeDirectoryWithSelectedNode\")"),
        QStringLiteral("tr(\"MakeDirectoryWithSameDomainNode\")"),
        QStringLiteral("tr(\"DeleteViewNode\")"),
        QStringLiteral("tr(\"DeleteViewNodesTogether\")"),
        QStringLiteral("tr(\"RecentlyClosedViewNode\")"),
        QStringLiteral("m_TreeBar->TreeBarMenu()"),
        QStringLiteral("tb->ShowKeys(menu);"),
    };
    int last = -1;
    for(const QString &item : order){
        const int at = menu.indexOf(item);
        QVERIFY2(at > last, qPrintable(item));
        QCOMPARE(menu.count(item), 1);
        last = at;
    }

    for(const QString &item : { QStringLiteral("together->addAction(deleteRightNode);"),
                                QStringLiteral("together->addAction(deleteLeftNode);"),
                                QStringLiteral("together->addAction(deleteOtherNode);") })
        QCOMPARE(menu.count(item), 1);

    QVERIFY(menu.contains(QStringLiteral("if(current) action->setObjectName(name);")));
    QCOMPARE(menu.count(QStringLiteral("setObjectName(")), 1);

    const int recent = menu.indexOf(QStringLiteral("AddTrashEntries(recent, tb,"));
    QVERIFY(recent >= 0);
    const int into = menu.indexOf(QStringLiteral("tb->MoveNode(t, vn);"), recent);
    const int next = menu.indexOf(QStringLiteral("tb->MoveNode(t, vn->GetParent(), vn->Index() + 1);"), recent);
    QVERIFY(into > recent && next > into);
    QVERIFY(menu.indexOf(QStringLiteral("if(vn->IsDirectory())"), recent) < into);

    const QString layer = BodyOf(source, QStringLiteral("QMenu *LayerItem::MakeNodeMenu()"));
    QVERIFY(layer.contains(QStringLiteral("AddTrashEntries(menu, tb,")));
}

namespace {
    int g_Freed = 0;
    class CountedNode : public ViewNode {
    public:
        ~CountedNode() Q_DECL_OVERRIDE { g_Freed++;}
    };
    CountedNode *MakeCounted(ViewNode *parent){
        CountedNode *nd = new CountedNode();
        nd->SetParent(parent);
        parent->AppendChild(nd);
        return nd;
    }
    void TakeOut(Node *nd){
        nd->GetParent()->RemoveChild(nd);
        nd->SetParent(nullptr);
    }
}

void tst_viewnode::aDiscardedNodeIsNeverFreedInTheTurnItWasBoxed(){
    ViewNode *root = new ViewNode();
    CountedNode *leaving = MakeCounted(root);
    MakeCounted(leaving);
    TakeOut(leaving);
    g_Freed = 0;

    TreeBank::Discard(leaving, root);
    QCOMPARE(TreeBank::FreshDeleteCount(), 1);
    TreeBank::DoDelete();
    QCOMPARE(g_Freed, 0);
    QCOMPARE(TreeBank::FreshDeleteCount(), 1);

    TreeBank::Discard(leaving, root);
    QCOMPARE(TreeBank::FreshDeleteCount(), 1);

    TreeBank::PassDeleteBarrier(true);
    QCOMPARE(g_Freed, 2);
    QCOMPARE(TreeBank::FreshDeleteCount(), 0);
    QCOMPARE(TreeBank::RipeDeleteCount(), 0);
    delete root;
}

void tst_viewnode::aDiscardedNodeIsFreedPastTheBarrier(){
    ViewNode *root = new ViewNode();
    CountedNode *leaving = MakeCounted(root);
    TakeOut(leaving);
    g_Freed = 0;

    TreeBank::Discard(leaving, root);
    QCOMPARE(g_Freed, 0);
    QTRY_COMPARE(g_Freed, 1);
    QCOMPARE(TreeBank::FreshDeleteCount(), 0);
    QCOMPARE(TreeBank::RipeDeleteCount(), 0);
    delete root;
}

void tst_viewnode::whileTheTreeIsWalkedTheBarrierOnlyRipens(){
    ViewNode *root = new ViewNode();
    CountedNode *leaving = MakeCounted(root);
    TakeOut(leaving);
    g_Freed = 0;

    TreeBank::Discard(leaving, root);
    TreeBank::PassDeleteBarrier(false);
    QCOMPARE(g_Freed, 0);
    QCOMPARE(TreeBank::FreshDeleteCount(), 0);
    QCOMPARE(TreeBank::RipeDeleteCount(), 1);

    TreeBank::DoDelete();
    QCOMPARE(g_Freed, 1);
    QCOMPARE(TreeBank::RipeDeleteCount(), 0);

    QCoreApplication::processEvents();
    QCOMPARE(g_Freed, 1);
    delete root;
}

void tst_viewnode::aStrippedViewLetsGoOfItsNode(){
    const QString source = TreeBankSource();
    QVERIFY2(!source.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");
    const QString body = BodyOf(source, QStringLiteral("void TreeBank::StripSubTree"));
    QVERIFY(!body.isEmpty());
    const int held = body.indexOf(QStringLiteral("GetThis().lock()"));
    const int dislink = body.indexOf(QStringLiteral("DislinkView(vn);"));
    const int forget = body.indexOf(QStringLiteral("held->ForgetViewNode(vn);"));
    QVERIFY(held >= 0 && dislink > held && forget > dislink);
}

void tst_viewnode::theRestoreGivesTheWindowOfTheIdItsNode(){
    const QString source = TreeBankSource();
    QVERIFY2(!source.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");

    const QString body = BodyOf(source, QStringLiteral("MainWindow *TreeBank::RestoreIntoWindow"));
    QVERIFY(!body.isEmpty());
    QVERIFY(body.contains(QStringLiteral("Application::Window(id)")));
    QVERIFY(body.contains(QStringLiteral("SetCurrent(nd)")));
    QVERIFY(body.indexOf(QStringLiteral("LoadWithLink(nd)")) < body.indexOf(QStringLiteral("SetCurrent(nd)")));
}

void tst_viewnode::treeLoadOnlyConsidersJsonGenerations(){
    const QString source = TreeBankSource();
    QVERIFY2(!source.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");

    const QString body = BodyOf(source, QStringLiteral("TreeBank::Restoration TreeBank::LoadTree"));
    QVERIFY(!body.isEmpty());
    QVERIFY(body.contains(QStringLiteral("backup.endsWith(filename)")));
    QVERIFY(!body.contains(QStringLiteral("Legacy")));
    QVERIFY(!body.contains(QStringLiteral(".xml")));
}

void tst_viewnode::overviewSharesTheWebViewAreaAndLeavesCompanionsAlone(){
    const QString tree = TreeBankSource();
    QVERIFY2(!tree.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");

    const QString area =
        BodyOf(tree, QStringLiteral("void TreeBank::ResizeViewArea"));
    const QString before =
        BodyOf(tree, QStringLiteral("void TreeBank::BeforeStartingDisplayGadgets"));
    const QString after =
        BodyOf(tree, QStringLiteral("void TreeBank::AfterFinishingDisplayGadgets"));
    const QString resize =
        BodyOf(tree, QStringLiteral("void TreeBank::resizeEvent"));
    const QString shelve =
        BodyOf(tree, QStringLiteral("void TreeBank::SetMiniMapShelved"));
    const QString setCurrent =
        BodyOf(tree, QStringLiteral("bool TreeBank::SetCurrent(Node *nd)"));
    QVERIFY(!area.isEmpty());
    QVERIFY(!before.isEmpty());
    QVERIFY(!after.isEmpty());
    QVERIFY(!resize.isEmpty());
    QVERIFY(!shelve.isEmpty());
    QVERIFY(!setCurrent.isEmpty());

    QVERIFY(area.contains(QStringLiteral("m_View->setGeometry(QRect(QPoint(), size))")));
    QVERIFY(area.contains(QStringLiteral("m_View->setSceneRect(QRect(QPoint(), size))")));
    QVERIFY(area.contains(QStringLiteral("m_Gadgets->ResizeNotify(size)")));
    QVERIFY(resize.contains(QStringLiteral("const QSize viewSize = ViewSize()")));
    QVERIFY(resize.contains(QStringLiteral("ResizeViewArea(viewSize)")));
    QVERIFY(resize.contains(QStringLiteral("m_CurrentView->resize(viewSize)")));
    QVERIFY(shelve.contains(QStringLiteral("ResizeViewArea(viewSize)")));
    const int setMiniMap = setCurrent.indexOf(QStringLiteral("m_MiniMap->SetView"));
    const int resizeArea = setCurrent.indexOf(QStringLiteral("ResizeViewArea(viewSize)"));
    QVERIFY(setMiniMap != -1);
    QVERIFY2(resizeArea > setMiniMap,
             "a tab switch sizes the overview before the minimap chooses ViewSize");

    QVERIFY2(!before.contains(QStringLiteral("SuspendInspectorPane")),
             "opening the overview still moves the inspector");
    QVERIFY2(!after.contains(QStringLiteral("SuspendInspectorPane")),
             "closing the overview still moves the inspector");
    QVERIFY2(!before.contains(QStringLiteral("m_MiniMap->hide()")),
             "opening the overview still hides the minimap");
    QVERIFY2(!after.contains(QStringLiteral("m_MiniMap->show()")),
             "closing the overview still shows the minimap again");
    const int restack = before.indexOf(QStringLiteral("RestackChildWidgets(OverviewUp)"));
    const int repaint = before.indexOf(QStringLiteral("m_View->viewport()->repaint()"));
    QVERIFY(restack != -1);
    QVERIFY2(repaint > restack,
             "a native page can disappear before the raised overview is painted");
}

void tst_viewnode::showingAnAlreadyVisibleEngineViewDoesNotJiggleItsSize(){
    const QStringList files = QStringList()
        << QStringLiteral("/view/webengine/webengineview.hpp")
        << QStringLiteral("/view/webengine/quickwebengineview.hpp");

    foreach(const QString &file, files){
        const QString source = SourceFile(file);
        QVERIFY2(!source.isEmpty(), qPrintable(file + QStringLiteral(" was not read")));
        const QString body = BodyOf(source, QStringLiteral("void show() Q_DECL_OVERRIDE"));
        QVERIFY2(!body.isEmpty(), qPrintable(file + QStringLiteral(" has no show body")));

        const int visible = body.indexOf(QStringLiteral("const bool wasVisible = base()->isVisible()"));
        const int guard = body.indexOf(QStringLiteral("if(!wasVisible)"));
        const int jiggle = body.indexOf(QStringLiteral("s.height()+1"));
        QVERIFY2(visible != -1, qPrintable(file + QStringLiteral(" does not remember visibility")));
        QVERIFY2(guard > visible, qPrintable(file + QStringLiteral(" does not guard the refresh")));
        QVERIFY2(jiggle > guard, qPrintable(file + QStringLiteral(" refreshes before the guard")));
    }
}

void tst_viewnode::aRunsSerialsBeginPastTheLastRuns(){
    const quint64 span = Node::SERIAL_SPAN, ceiling = Node::SERIAL_CEILING;
    QCOMPARE(Node::SerialStart(QByteArray()), quint64(1));
    QCOMPARE(Node::SerialStart("x"), quint64(1));
    QCOMPARE(Node::SerialStart("0"), quint64(1));
    QCOMPARE(Node::SerialStart("-5"), quint64(1));
    QCOMPARE(Node::SerialStart(QByteArray::number(ceiling)), quint64(1));
    QCOMPARE(Node::SerialStart(QByteArray::number(ceiling - 1)), ceiling - 1);
    QCOMPARE(Node::SerialStart("12345\n"), quint64(12345));
    QCOMPARE(Node::SerialAfter(1, 1), 1 + span);
    QCOMPARE(Node::SerialAfter(1, 10), 1 + span);
    QCOMPARE(Node::SerialAfter(100, 100 + span + 7), 100 + span + 7);
    QCOMPARE(Node::SerialAfter(ceiling - span, ceiling - span), quint64(1));
    QCOMPARE(Node::SerialAfter(ceiling - span - 1, ceiling - span - 1), ceiling - 1);
    QCOMPARE(Node::SerialAfter(1, ceiling + 5), quint64(1));
    QVERIFY(Node::SerialStart(QByteArray::number(Node::SerialAfter(ceiling - 1, ceiling - 1))) < ceiling);
}

void tst_viewnode::showingATabStampsItsLastAccessWhicheverEngineShowsIt(){
    const QString tree = TreeBankSource();
    QVERIFY2(!tree.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");

    const QString setCurrent =
        BodyOf(tree, QStringLiteral("bool TreeBank::SetCurrent(Node *nd)"));
    QVERIFY(!setCurrent.isEmpty());

    const int current = setCurrent.indexOf(QStringLiteral("m_CurrentViewNode = nd->ToViewNode()"));
    const int stamp = setCurrent.indexOf(QStringLiteral("m_CurrentViewNode->SetLastAccessDateToCurrent()"));
    const int load = setCurrent.indexOf(QStringLiteral("LoadWithLink(m_CurrentViewNode)"));
    QVERIFY(current != -1);
    QVERIFY2(stamp != -1, "showing a tab no longer stamps its last access");
    QVERIFY2(stamp > current, "the stamp lands on the tab being left");
    QVERIFY2(load == -1 || stamp < load, "the stamp waits for the engine, which Edge never answers with a focus-in");

    ViewNode *root = new ViewNode();
    ViewNode *dir = root->MakeChild();
    ViewNode *tab = dir->MakeChild();
    const QDateTime old = QDateTime::currentDateTime().addSecs(-3600);
    dir->SetLastAccessDate(old);
    tab->SetLastAccessDate(old);
    tab->SetLastAccessDateToCurrent();
    QVERIFY(tab->GetLastAccessDate() > old);
    QVERIFY(dir->GetLastAccessDate() > old);
    delete root;
}

void tst_viewnode::aFinishedLoadTakesItsThumbnailWhateverTheBarShows(){
    struct Place { QString file; QString signature; };
    const QList<Place> places = QList<Place>()
        << Place{ QStringLiteral("/view/webengine/webengineview.cpp"),
                  QStringLiteral("void WebEngineView::OnLoadFinished(bool ok)") }
        << Place{ QStringLiteral("/view/webengine/quickwebengineview.cpp"),
                  QStringLiteral("void QuickWebEngineView::OnLoadFinished(bool ok)") }
        << Place{ QStringLiteral("/view/quicknativewebview.cpp"),
                  QStringLiteral("void QuickNativeWebView::OnLoadFinished(bool ok)") }
        << Place{ QStringLiteral("/view/edge/edgewebviewhandlers.cpp"),
                  QStringLiteral("add_NavigationCompleted") };

    foreach(const Place &place, places){
        const QString source = SourceFile(place.file);
        QVERIFY2(!source.isEmpty(), qPrintable(place.file + QStringLiteral(" was not read")));
        const QString body = BodyOf(source, place.signature);
        QVERIFY2(!body.isEmpty(), qPrintable(place.file + QStringLiteral(" has no body for ") + place.signature));

        const int taken = body.indexOf(QStringLiteral("UpdateThumbnail()"));
        QVERIFY2(taken != -1, qPrintable(place.file + QStringLiteral(" takes no thumbnail at the end of a load")));
        QVERIFY2(body.lastIndexOf(QStringLiteral("visible() && m_TreeBank"), taken) != -1,
                 qPrintable(place.file + QStringLiteral(" takes the thumbnail without asking whether it is on screen")));
        QVERIFY2(!body.contains(QStringLiteral("GetTreeBar()->isVisible()")),
                 qPrintable(place.file + QStringLiteral(" still waits for the tab bar to be visible")));
    }
}

void tst_viewnode::hidingAFullScreenViewLeavesFullScreenWhicheverEngine(){
    struct Place { QString file; QString signature; };
    const QList<Place> places = QList<Place>()
        << Place{ QStringLiteral("/view/webengine/webengineview.cpp"),
                  QStringLiteral("void WebEngineView::hideEvent(QHideEvent *ev)") }
        << Place{ QStringLiteral("/view/webengine/quickwebengineview.cpp"),
                  QStringLiteral("void QuickWebEngineView::hideEvent(QHideEvent *ev)") }
        << Place{ QStringLiteral("/view/quicknativewebview.cpp"),
                  QStringLiteral("void QuickNativeWebView::hideEvent(QHideEvent *ev)") }
        << Place{ QStringLiteral("/view/edge/edgewebview.cpp"),
                  QStringLiteral("void EdgeWebView::hideEvent(QHideEvent *ev)") };

    foreach(const Place &place, places){
        const QString source = SourceFile(place.file);
        QVERIFY2(!source.isEmpty(), qPrintable(place.file + QStringLiteral(" was not read")));
        const QString body = BodyOf(source, place.signature);
        QVERIFY2(!body.isEmpty(), qPrintable(place.file + QStringLiteral(" has no hideEvent body")));
        QVERIFY2(body.contains(QStringLiteral("if(GetDisplayObscured()) ExitFullScreen();")),
                 qPrintable(place.file + QStringLiteral(" hides without leaving full screen")));
    }
}

QTEST_MAIN(tst_viewnode)
#include "tst_viewnode.moc"
