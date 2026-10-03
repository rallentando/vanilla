#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QUrl>

#include <QFile>
#include <QRegularExpression>

#include "lightnode.hpp"
#include "treebank.hpp"

#include "testsupport.hpp"

namespace {

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
    void makeParentWrapsTheNode();

    void nextWalksTheTreeDepthFirst();
    void prevWalksTheTreeBackwards();
    void nextAndPrevStopAtTheEnds();

    void theAutoLoadWalkStopsAtTheFirstTabItLoads();
    void theAutoLoadWalkStepsOverRefusedTabsWithoutGrowingTheStack();

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
    void treeLoadOnlyConsidersJsonGenerations();

    void overviewSharesTheWebViewAreaAndLeavesCompanionsAlone();
    void showingAnAlreadyVisibleEngineViewDoesNotJiggleItsSize();
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
    });
    QCOMPARE(offered, QStringList() << QStringLiteral("1-3") << QStringLiteral("2"));
    QVERIFY(iter == two);

    offered.clear();
    iter = two;
    TreeBank::WalkToAutoLoad(iter, false, [&](ViewNode *vn){
        offered << vn->GetTitle();
        return false;
    });
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
    TreeBank::WalkToAutoLoad(iter, true, [&](ViewNode *){
        offered++;
        return false;
    });
    QCOMPARE(offered, Folders * TabsPerFolder);
    QVERIFY(!iter);

    delete root;
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
    QCOMPARE(fresh->GetUrl(), BLANK_URL);

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
        << QStringLiteral("vn->Delete()")
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
    QVERIFY(body.contains(QStringLiteral("SetViewIterForward(nullptr)")));
    QVERIFY(body.contains(QStringLiteral("SetViewIterBackward(nullptr)")));
}

void tst_viewnode::treeLoadOnlyConsidersJsonGenerations(){
    const QString source = TreeBankSource();
    QVERIFY2(!source.isEmpty(), "treebank.cpp was not read; check VANILLA_SOURCE_DIR");

    const QString body = BodyOf(source, QStringLiteral("void TreeBank::LoadTree"));
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

QTEST_MAIN(tst_viewnode)
#include "tst_viewnode.moc"
