#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <memory>

#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "lightnode.hpp"
#include "testsupport.hpp"

namespace {
    struct NewChildrenAtTheLeft : ViewNode {
        static void Set(bool left){
            m_AddChildViewNodePosition = left ? LeftEnd : RightEnd;
        }
    };
}

class tst_nodeclipboard : public QObject {
    Q_OBJECT

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        TestSupport::DisableWidgetAnimation();
        TreeBank::Initialize();
    }
    void init();
    void cleanup();

    void nothingPutAsideIsNothingToPaste();
    void aCutMovesOnceAndIsGone();
    void aCopyPastesAsOftenAsAsked();
    void aCopyLandsRightOfTheAnchor();
    void aCutOfTheAnchorStaysWhereItIs();
    void aCutOfTheFirstNodesLandsAtTheStart();
    void aPasteCanLandLeftOfTheAnchor();
    void noNodeLandsInsideItself();
    void whatLandsInsideItselfAloneChangesNothing();
    void aDirectoryAndATabInItCopyOnce();
    void aNodeMovedUnderAnotherAfterwardGoesWithIt();
    void whatIsInTheTrashStaysOutUntilItIsBack();
    void nothingLandsInTheTrash();
    void aThrownAwayNodeIsNotHungBackInTheTree();
    void aFreedNodeLeavesWhatIsPutAside();
    void aCopiedDirectoryKeepsItsOrder();
    void aCopyIntoAPrivateDirectoryIsPrivate();
    void aCopyOutOfAPrivateDirectoryIsNot();
    void aPublicDirectoryBelowAPrivatePlaceIsNot();
    void holdingIsAboutTheNodesNotTheirOrder();

private:
    static ViewNode *Tab(ViewNode *parent, const QString &name = QString()){
        ViewNode *vn = parent->MakeChild();
        vn->SetHoldView(true);
        vn->SetTitle(name);
        vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + name));
        return vn;
    }
    static ViewNode *Directory(ViewNode *parent, const QString &title){
        ViewNode *vn = parent->MakeChild();
        vn->SetTitle(title);
        return vn;
    }
    static QStringList Titles(Node *parent){
        QStringList titles;
        foreach(Node *nd, parent->GetChildren()) titles << nd->GetTitle();
        return titles;
    }
    static void Settle(){
        QTRY_COMPARE(TreeBank::FreshDeleteCount(), 0);
        TreeBank::DoDelete();
    }

    std::unique_ptr<MainWindow> m_Window;
    TreeBank *m_Bank = nullptr;
};

void tst_nodeclipboard::init(){
    Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
    m_Window.reset(Application::NewWindow(945));
    m_Bank = m_Window->GetTreeBank();
    TreeBank::SetClipboardNodes(NodeList(), false);
}

void tst_nodeclipboard::cleanup(){
    TreeBank::SetClipboardNodes(NodeList(), false);
    Settle();
    Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
    Application::RemoveWindow(945);
    TreeBank::Initialize();
    m_Window.reset();
}

void tst_nodeclipboard::nothingPutAsideIsNothingToPaste(){
    ViewNode *root = TreeBank::GetViewRoot();
    Tab(root, QStringLiteral("a"));
    QVERIFY(!TreeBank::HasClipboardNodes());
    QVERIFY(!m_Bank->PasteClipboardNodes(root, nullptr));
    QCOMPARE(Titles(root), QStringList{QStringLiteral("a")});
}

void tst_nodeclipboard::aCutMovesOnceAndIsGone(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *dir = Directory(root, QStringLiteral("dir"));
    ViewNode *a = Tab(root, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << a, true);

    QVERIFY(m_Bank->PasteClipboardNodes(dir, nullptr));
    QCOMPARE(a->GetParent(), static_cast<Node*>(dir));
    QVERIFY(!root->GetChildren().contains(a));
    QVERIFY(!TreeBank::HasClipboardNodes());
    QVERIFY(!m_Bank->PasteClipboardNodes(root, nullptr));
    QCOMPARE(a->GetParent(), static_cast<Node*>(dir));
}

void tst_nodeclipboard::aCopyPastesAsOftenAsAsked(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *dir = Directory(root, QStringLiteral("dir"));
    ViewNode *a = Tab(root, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << a, false);

    QVERIFY(m_Bank->PasteClipboardNodes(dir, nullptr));
    QVERIFY(m_Bank->PasteClipboardNodes(dir, nullptr));
    QCOMPARE(Titles(dir), (QStringList{QStringLiteral("a"), QStringLiteral("a")}));
    QCOMPARE(a->GetParent(), static_cast<Node*>(root));
    QVERIFY(dir->GetFirstChild() != a);
    QCOMPARE(dir->GetFirstChild()->GetUrl(), a->GetUrl());
    QVERIFY(TreeBank::HasClipboardNodes());
}

void tst_nodeclipboard::aCopyLandsRightOfTheAnchor(){
    ViewNode *dir = Directory(TreeBank::GetViewRoot(), QStringLiteral("dir"));
    ViewNode *a = Tab(dir, QStringLiteral("a"));
    Tab(dir, QStringLiteral("b"));
    ViewNode *c = Tab(dir, QStringLiteral("c"));
    TreeBank::SetClipboardNodes(NodeList() << c, false);

    QVERIFY(m_Bank->PasteClipboardNodes(dir, a));
    QCOMPARE(Titles(dir), (QStringList{QStringLiteral("a"), QStringLiteral("c"),
                                       QStringLiteral("b"), QStringLiteral("c")}));
}

void tst_nodeclipboard::aCutOfTheAnchorStaysWhereItIs(){
    ViewNode *dir = Directory(TreeBank::GetViewRoot(), QStringLiteral("dir"));
    Tab(dir, QStringLiteral("a"));
    ViewNode *b = Tab(dir, QStringLiteral("b"));
    ViewNode *c = Tab(dir, QStringLiteral("c"));
    Tab(dir, QStringLiteral("d"));
    TreeBank::SetClipboardNodes(NodeList() << b << c, true);

    QVERIFY(m_Bank->PasteClipboardNodes(dir, c));
    QCOMPARE(Titles(dir), (QStringList{QStringLiteral("a"), QStringLiteral("b"),
                                       QStringLiteral("c"), QStringLiteral("d")}));
}

void tst_nodeclipboard::aCutOfTheFirstNodesLandsAtTheStart(){
    ViewNode *dir = Directory(TreeBank::GetViewRoot(), QStringLiteral("dir"));
    ViewNode *a = Tab(dir, QStringLiteral("a"));
    Tab(dir, QStringLiteral("b"));
    TreeBank::SetClipboardNodes(NodeList() << a, true);

    QVERIFY(m_Bank->PasteClipboardNodes(dir, a));
    QCOMPARE(Titles(dir), (QStringList{QStringLiteral("a"), QStringLiteral("b")}));
}

void tst_nodeclipboard::aPasteCanLandLeftOfTheAnchor(){
    ViewNode *dir = Directory(TreeBank::GetViewRoot(), QStringLiteral("dir"));
    ViewNode *a = Tab(dir, QStringLiteral("a"));
    ViewNode *b = Tab(dir, QStringLiteral("b"));
    ViewNode *c = Tab(dir, QStringLiteral("c"));
    ViewNode *d = Tab(dir, QStringLiteral("d"));

    TreeBank::SetClipboardNodes(NodeList() << d, true);
    QVERIFY(m_Bank->PasteClipboardNodes(dir, b, true));
    QCOMPARE(Titles(dir), (QStringList{QStringLiteral("a"), QStringLiteral("d"),
                                       QStringLiteral("b"), QStringLiteral("c")}));

    TreeBank::SetClipboardNodes(NodeList() << a << b, true);
    QVERIFY(m_Bank->PasteClipboardNodes(dir, b, true));
    QCOMPARE(Titles(dir), (QStringList{QStringLiteral("d"), QStringLiteral("a"),
                                       QStringLiteral("b"), QStringLiteral("c")}));

    TreeBank::SetClipboardNodes(NodeList() << c, true);
    QVERIFY(m_Bank->PasteClipboardNodes(dir, c, true));
    QCOMPARE(Titles(dir), (QStringList{QStringLiteral("d"), QStringLiteral("a"),
                                       QStringLiteral("b"), QStringLiteral("c")}));
}

void tst_nodeclipboard::noNodeLandsInsideItself(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *dir = Directory(root, QStringLiteral("dir"));
    ViewNode *below = Directory(dir, QStringLiteral("below"));
    ViewNode *a = Tab(root, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << dir << a, false);

    QVERIFY(m_Bank->PasteClipboardNodes(below, nullptr));
    QCOMPARE(Titles(below), QStringList{QStringLiteral("a")});
    QCOMPARE(dir->GetParent(), static_cast<Node*>(root));
}

void tst_nodeclipboard::whatLandsInsideItselfAloneChangesNothing(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *dir = Directory(root, QStringLiteral("dir"));
    ViewNode *below = Directory(dir, QStringLiteral("below"));
    TreeBank::SetClipboardNodes(NodeList() << dir, true);

    QVERIFY(!m_Bank->PasteClipboardNodes(below, nullptr));
    QVERIFY(!m_Bank->PasteClipboardNodes(dir, nullptr));
    QVERIFY(TreeBank::HasClipboardNodes());
    QCOMPARE(dir->GetParent(), static_cast<Node*>(root));
    QVERIFY(below->HasNoChildren());
}

void tst_nodeclipboard::aDirectoryAndATabInItCopyOnce(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *target = Directory(root, QStringLiteral("target"));
    ViewNode *dir = Directory(root, QStringLiteral("dir"));
    ViewNode *a = Tab(dir, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << dir << a, false);

    QVERIFY(m_Bank->PasteClipboardNodes(target, nullptr));
    QCOMPARE(Titles(target), QStringList{QStringLiteral("dir")});
    QCOMPARE(Titles(target->GetFirstChild()), QStringList{QStringLiteral("a")});
}

void tst_nodeclipboard::aNodeMovedUnderAnotherAfterwardGoesWithIt(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *target = Directory(root, QStringLiteral("target"));
    ViewNode *dir = Directory(root, QStringLiteral("dir"));
    ViewNode *a = Tab(root, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << dir << a, true);
    TreeBank::MoveNode(a, dir);

    QVERIFY(m_Bank->PasteClipboardNodes(target, nullptr));
    QCOMPARE(Titles(target), QStringList{QStringLiteral("dir")});
    QCOMPARE(a->GetParent(), static_cast<Node*>(dir));
}

void tst_nodeclipboard::whatIsInTheTrashStaysOutUntilItIsBack(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *dir = Directory(root, QStringLiteral("dir"));
    ViewNode *a = Tab(root, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << a, false);
    TreeBank::MoveNode(a, TreeBank::GetTrashRoot());

    QVERIFY(!m_Bank->PasteClipboardNodes(dir, nullptr));
    QVERIFY(dir->HasNoChildren());
    QVERIFY(TreeBank::HasClipboardNodes());
    TreeBank::MoveNode(a, root);
    QVERIFY(m_Bank->PasteClipboardNodes(dir, nullptr));
    QCOMPARE(Titles(dir), QStringList{QStringLiteral("a")});
}

void tst_nodeclipboard::nothingLandsInTheTrash(){
    ViewNode *a = Tab(TreeBank::GetViewRoot(), QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << a, false);
    QVERIFY(!m_Bank->PasteClipboardNodes(TreeBank::GetTrashRoot(), nullptr));
    QVERIFY(TreeBank::GetTrashRoot()->HasNoChildren());
}

void tst_nodeclipboard::aThrownAwayNodeIsNotHungBackInTheTree(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *dir = Directory(root, QStringLiteral("dir"));
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *a = Tab(secret, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << a, true);

    QVERIFY(m_Bank->DeleteNode(a));
    QVERIFY(!m_Bank->PasteClipboardNodes(dir, nullptr));
    QVERIFY(dir->HasNoChildren());
}

void tst_nodeclipboard::aFreedNodeLeavesWhatIsPutAside(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *dir = Directory(root, QStringLiteral("dir;Private"));
    ViewNode *a = Tab(dir, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << a, false);

    QVERIFY(m_Bank->DeleteNode(dir));
    Settle();
    QVERIFY(!TreeBank::HasClipboardNodes());
}

void tst_nodeclipboard::aCopiedDirectoryKeepsItsOrder(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *target = Directory(root, QStringLiteral("target"));
    ViewNode *dir = Directory(root, QStringLiteral("dir"));
    Tab(dir, QStringLiteral("a"));
    Tab(dir, QStringLiteral("b"));
    TreeBank::SetClipboardNodes(NodeList() << dir, false);

    NewChildrenAtTheLeft::Set(true);
    const bool pasted = m_Bank->PasteClipboardNodes(target, nullptr);
    NewChildrenAtTheLeft::Set(false);

    QVERIFY(pasted);
    QCOMPARE(Titles(target->GetFirstChild()),
             (QStringList{QStringLiteral("a"), QStringLiteral("b")}));
}

void tst_nodeclipboard::aCopyIntoAPrivateDirectoryIsPrivate(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *a = Tab(root, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << a, false);

    QVERIFY(m_Bank->PasteClipboardNodes(secret, nullptr));
    QVERIFY(secret->GetFirstChild()->ToViewNode()->IsOffDisk());
    QVERIFY(!a->IsOffDisk());
}

void tst_nodeclipboard::aCopyOutOfAPrivateDirectoryIsNot(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *target = Directory(root, QStringLiteral("target"));
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *a = Tab(secret, QStringLiteral("a"));
    TreeBank::SetClipboardNodes(NodeList() << a, false);

    QVERIFY(m_Bank->PasteClipboardNodes(target, nullptr));
    QVERIFY(a->IsOffDisk());
    QVERIFY(!target->GetFirstChild()->ToViewNode()->IsOffDisk());
}

void tst_nodeclipboard::aPublicDirectoryBelowAPrivatePlaceIsNot(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *secret = Directory(root, QStringLiteral("secret;Private"));
    ViewNode *open = Directory(root, QStringLiteral("open;!Private"));
    Tab(open, QStringLiteral("a"));
    ViewNode *plain = Tab(root, QStringLiteral("plain"));
    TreeBank::SetClipboardNodes(NodeList() << plain << open, false);

    QVERIFY(m_Bank->PasteClipboardNodes(secret, nullptr));
    const QList<Node*> copies = secret->GetChildren();
    QCOMPARE(copies.length(), 2);
    QVERIFY(copies[0]->ToViewNode()->IsOffDisk());
    QVERIFY(!copies[1]->GetFirstChild()->ToViewNode()->IsOffDisk());
}

void tst_nodeclipboard::holdingIsAboutTheNodesNotTheirOrder(){
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *a = Tab(root, QStringLiteral("a"));
    ViewNode *b = Tab(root, QStringLiteral("b"));
    TreeBank::SetClipboardNodes(NodeList() << a << b, true);
    QVERIFY(TreeBank::ClipboardHolds(NodeList() << b << a));
    QVERIFY(!TreeBank::ClipboardHolds(NodeList() << a));
}

QTEST_MAIN(tst_nodeclipboard)
#include "tst_nodeclipboard.moc"
