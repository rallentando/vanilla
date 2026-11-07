#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QElapsedTimer>
#include <QScopeGuard>
#include <memory>

#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "lightnode.hpp"
#include "testsupport.hpp"

class tst_privatetree : public QObject {
    Q_OBJECT

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        TestSupport::DisableWidgetAnimation();
        TreeBank::Initialize();
    }
    void init();
    void cleanup();

    void aNodeIsPrivateByTheWordsAboveIt();
    void theSweepMarksThePrivateNodesAndOnlyThose();
    void aNodeMovedIntoAPrivateDirectoryIsMarkedAtOnce();
    void theSavedTreeNamesNoFileOfAPrivateNode();
    void aPrivateTabClosesWithoutTheTrash();
    void anOrdinaryTabStillGoesToTheTrash();
    void aMixedDirectoryLeavesItsPrivatePartOutOfTheTrash();
    void aListWithADirectoryAndATabInItClosesEachOnce();
    void theNextCurrentIsNotChosenInADiscardedDirectory();
    void theNextCurrentIsNotChosenInTheTrash();
    void theSweepOfTwentyThousandNodes();
    void aReorderTakingAChildFromElsewhereWalksTheFolderAgain();
    void aMoveWithNoCurrentWalksTheWholeTreeAgain();

private:
    static ViewNode *Tab(ViewNode *parent){
        ViewNode *vn = parent->MakeChild();
        vn->SetHoldView(true);
        vn->SetUrl(QUrl(QStringLiteral("https://example.com/")));
        return vn;
    }
    static ViewNode *Directory(ViewNode *parent, const QString &title){
        ViewNode *vn = parent->MakeChild();
        vn->SetTitle(title);
        return vn;
    }
    static void Settle(){
        QTRY_COMPARE(TreeBank::FreshDeleteCount(), 0);
        TreeBank::DoDelete();
    }

    std::unique_ptr<MainWindow> m_Window;
    TreeBank *m_Bank = nullptr;
};

void tst_privatetree::init(){
    Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
    m_Window.reset(Application::NewWindow(944));
    m_Bank = m_Window->GetTreeBank();
}

void tst_privatetree::cleanup(){
    Settle();
    Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
    Application::RemoveWindow(944);
    TreeBank::Initialize();
    m_Window.reset();
}

void tst_privatetree::aNodeIsPrivateByTheWordsAboveIt(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *inside = Tab(secret);
    ViewNode *open = Directory(secret, QStringLiteral("open;!Private"));
    ViewNode *openTab = Tab(open);
    ViewNode *plain = Tab(root);

    QVERIFY(TreeBank::IsOffTheRecord(secret));
    QVERIFY(TreeBank::IsOffTheRecord(inside));
    QVERIFY(!TreeBank::IsOffTheRecord(open));
    QVERIFY(!TreeBank::IsOffTheRecord(openTab));
    QVERIFY(!TreeBank::IsOffTheRecord(plain));
}

void tst_privatetree::theSweepMarksThePrivateNodesAndOnlyThose(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *inside = Tab(secret);
    ViewNode *child = Tab(inside);
    ViewNode *open = Directory(secret, QStringLiteral("open;!Private"));
    ViewNode *openTab = Tab(open);
    ViewNode *plain = Tab(root);

    TreeBank::SweepOffDisk();
    QVERIFY(inside->IsOffDisk());
    QVERIFY(child->IsOffDisk());
    QVERIFY(!openTab->IsOffDisk());
    QVERIFY(!plain->IsOffDisk());

    secret->SetTitle(QStringLiteral("secret"));
    TreeBank::SweepOffDisk();
    QVERIFY(!inside->IsOffDisk());
    QVERIFY(!child->IsOffDisk());
}

namespace {
    QStringList WalkOn(bool forward){
        ViewNode *iter = forward ? TreeBank::GetViewIterForward() : TreeBank::GetViewIterBackward();
        bool pending = forward ? TreeBank::GetViewIterForwardPending() : TreeBank::GetViewIterBackwardPending();
        QStringList offered;
        TreeBank::WalkToAutoLoad(iter, forward, [&](ViewNode *vn){
            offered << vn->GetTitle();
            return false;
        }, std::numeric_limits<int>::max(), pending);
        return offered;
    }
}

void tst_privatetree::aReorderTakingAChildFromElsewhereWalksTheFolderAgain(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *s = Directory(root, QStringLiteral("S"));
    Tab(s)->SetTitle(QStringLiteral("s"));
    ViewNode *x = Directory(s, QStringLiteral("X"));
    Tab(x)->SetTitle(QStringLiteral("x"));
    ViewNode *p = Directory(root, QStringLiteral("P"));
    ViewNode *a = Tab(p);
    a->SetTitle(QStringLiteral("A"));
    ViewNode *b = Tab(p);
    b->SetTitle(QStringLiteral("B"));
    Tab(root)->SetTitle(QStringLiteral("C"));

    TreeBank::SetViewIterForward(nullptr);
    TreeBank::SetViewIterBackward(b);
    QVERIFY(TreeBank::SetChildrenOrder(p, NodeList() << a << b << x));
    const QStringList offered = WalkOn(false);
    QVERIFY2(offered.contains(QStringLiteral("A")), qPrintable(offered.join(QStringLiteral(","))));
    QVERIFY(offered.contains(QStringLiteral("x")));
    QVERIFY(offered.contains(QStringLiteral("s")));
    TreeBank::SetViewIterBackward(nullptr);
}

void tst_privatetree::aMoveWithNoCurrentWalksTheWholeTreeAgain(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *q = Directory(root, QStringLiteral("Q"));
    ViewNode *x = Directory(root, QStringLiteral("X"));
    ViewNode *x1 = Tab(x);
    x1->SetTitle(QStringLiteral("x1"));
    Tab(x)->SetTitle(QStringLiteral("x2"));
    Tab(root)->SetTitle(QStringLiteral("Z"));
    QVERIFY(!m_Bank->GetCurrentViewNode());

    TreeBank::SetViewIterForward(x1);
    TreeBank::SetViewIterBackward(x1);
    QVERIFY(TreeBank::MoveNode(x, q));
    QCOMPARE(TreeBank::GetViewIterForward(), root);
    QVERIFY(TreeBank::GetViewIterForwardPending());
    QVERIFY(!TreeBank::GetViewIterBackward());
    const QStringList offered = WalkOn(true);
    QVERIFY2(offered.contains(QStringLiteral("x2")), qPrintable(offered.join(QStringLiteral(","))));
    QVERIFY(offered.contains(QStringLiteral("Z")));
    TreeBank::SetViewIterForward(nullptr);
}

void tst_privatetree::aNodeMovedIntoAPrivateDirectoryIsMarkedAtOnce(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *plain = Tab(root);
    QVERIFY(!plain->IsOffDisk());
    TreeBank::MoveNode(plain, secret);
    QVERIFY(plain->IsOffDisk());
    TreeBank::MoveNode(plain, root);
    QVERIFY(!plain->IsOffDisk());
}

void tst_privatetree::theSavedTreeNamesNoFileOfAPrivateNode(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *inside = Tab(secret);
    inside->SetTitle(QStringLiteral("inside"));
    ViewNode *plain = Tab(root);
    plain->SetTitle(QStringLiteral("plain"));
    TreeBank::SweepOffDisk();
    QVERIFY(TreeBank::SaveTree(TreeBank::WindowIndexMap()));

    QFile file(Application::StateDirectory() + Application::PrimaryTreeFileName());
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(file.readAll());
    const int at = text.indexOf(QStringLiteral("\"title\": \"inside\""));
    const int next = text.indexOf(QStringLiteral("\"title\": \"plain\""));
    QVERIFY(at >= 0 && next >= 0);
    const QString insideRecord = at < next ? text.mid(at, next - at) : text.mid(at);
    const QString plainRecord = at < next ? text.mid(next) : text.mid(next, at - next);
    QVERIFY(!insideRecord.contains(QStringLiteral("\"thumb\"")));
    QVERIFY(!insideRecord.contains(QStringLiteral("\"history\"")));
    QVERIFY(plainRecord.contains(QStringLiteral("\"thumb\"")));
}

void tst_privatetree::aPrivateTabClosesWithoutTheTrash(){
    ViewNode *secret = Directory(TreeBank::GetViewRoot(), QStringLiteral("secret;Private"));
    ViewNode *inside = Tab(secret);
    const int trashed = TreeBank::GetTrashRoot()->ChildrenLength();

    QVERIFY(m_Bank->DeleteNode(inside));
    QVERIFY(!secret->GetChildren().contains(inside));
    QCOMPARE(TreeBank::GetTrashRoot()->ChildrenLength(), trashed);
}

void tst_privatetree::anOrdinaryTabStillGoesToTheTrash(){
    ViewNode *plain = Tab(TreeBank::GetViewRoot());
    QVERIFY(m_Bank->DeleteNode(plain));
    QCOMPARE(TreeBank::GetTrashRoot()->GetFirstChild(), static_cast<Node*>(plain));
}

void tst_privatetree::aMixedDirectoryLeavesItsPrivatePartOutOfTheTrash(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *inside = Tab(secret);
    ViewNode *open = Directory(secret, QStringLiteral("open;!Private"));
    ViewNode *openTab = Tab(open);
    const int trashed = TreeBank::GetTrashRoot()->ChildrenLength();

    QVERIFY(m_Bank->DeleteNode(secret));
    QCOMPARE(TreeBank::GetTrashRoot()->ChildrenLength(), trashed + 1);
    QCOMPARE(TreeBank::GetTrashRoot()->GetFirstChild(), static_cast<Node*>(secret));
    QVERIFY(!secret->GetChildren().contains(inside));
    QVERIFY(secret->GetChildren().contains(open));
    QVERIFY(open->GetChildren().contains(openTab));
}

void tst_privatetree::aListWithADirectoryAndATabInItClosesEachOnce(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *inside = Tab(secret);
    const int trashed = TreeBank::GetTrashRoot()->ChildrenLength();

    QVERIFY(m_Bank->DeleteNode(NodeList() << secret << inside));
    QVERIFY(!root->GetChildren().contains(secret));
    QCOMPARE(TreeBank::GetTrashRoot()->ChildrenLength(), trashed);
    QCOMPARE(TreeBank::FreshDeleteCount(), 1);
}

void tst_privatetree::theNextCurrentIsNotChosenInADiscardedDirectory(){
    ViewNode *secret = Directory(TreeBank::GetViewRoot(), QStringLiteral("secret;Private"));
    ViewNode *first = Tab(secret);
    ViewNode *second = Tab(secret);
    QVERIFY(m_Bank->DeleteNode(NodeList() << first << secret));
    QVERIFY(m_Bank->GetCurrentViewNode() != second);
    QVERIFY(!second->GetView());
}

void tst_privatetree::theNextCurrentIsNotChosenInTheTrash(){
    ViewNode *plainDir = Directory(TreeBank::GetViewRoot(), QStringLiteral("plain"));
    ViewNode *plainTab = Tab(plainDir);
    ViewNode *secret = Directory(plainDir, QStringLiteral("secret;Private"));
    ViewNode *inside = Tab(secret);
    QVERIFY(m_Bank->DeleteNode(NodeList() << inside << plainDir));
    QCOMPARE(TreeBank::GetTrashRoot()->GetFirstChild(), static_cast<Node*>(plainDir));
    QVERIFY(m_Bank->GetCurrentViewNode() != plainTab);
    QVERIFY(!plainTab->GetView());
}

void tst_privatetree::theSweepOfTwentyThousandNodes(){
    ViewNode *root = TreeBank::GetViewRoot();
    for(int d = 0; d < 200; d++){
        ViewNode *dir = Directory(root, d % 10 ? QStringLiteral("dir") : QStringLiteral("dir;Private"));
        for(int t = 0; t < 100; t++) Tab(dir);
    }
    QElapsedTimer clock;
    clock.start();
    TreeBank::SweepOffDisk();
    qInfo("the sweep of %d nodes: %lld ms", 200 * 101, clock.elapsed());
}

QTEST_MAIN(tst_privatetree)
#include "tst_privatetree.moc"
