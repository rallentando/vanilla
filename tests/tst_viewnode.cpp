#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QUrl>

#include "lightnode.hpp"

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

    void ancestorsAndDescendants();
    void primaryPath();

    void aTabIsNotADirectory();
    void readableTitleFallsBackToTheUrl();

    void newMakesASiblingTab();
    void cloneCopiesTheSubtree();

    void everyNodeGetsASerialOfItsOwn();
    void aserialIsNeverHandedOutTwice();
};

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

QTEST_MAIN(tst_viewnode)
#include "tst_viewnode.moc"
