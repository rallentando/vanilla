#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "receiver.hpp"
#include "notifier.hpp"
#include "minimap.hpp"
#include "gadgets.hpp"
#include "commandmap.hpp"
#include "menuschema.hpp"
#include "inputmapschema.hpp"
#include "webelement.hpp"
#include "searchsuggest.hpp"
#include "toolbar.hpp"
#include "nodepreview.hpp"
#include "dialog.hpp"
#include <QClipboard>
#include <QToolButton>
#include <QCompleter>
#include <QHeaderView>
#include <QLineEdit>
#include <QTableView>
#include "settingspage.hpp"
#include "directorypage.hpp"
#include "transmitter.hpp"
#include "testsupport.hpp"

#include <QtTest>
#include <QDataStream>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPointer>
#include <memory>

struct PathElement : public JsWebElement {
    PathElement(const QString &path, const QUrl &link = QUrl(), bool editable = false){
        m_XPath = path;
        m_TagName = QStringLiteral("BUTTON");
        m_IsEditable = editable;
        m_LinkUrl = link;
        m_Rectangle = QRect(1, 1, 10, 10);
    }
};
struct FocusedElement : public PathElement {
    FocusedElement(const QString &path) : PathElement(path, QUrl(), true){ m_IsFocusedField = true; }
};

class tst_receiver : public QObject {
    Q_OBJECT
    static bool connectOnce(QLocalServer *server){
        QSignalSpy accepted(server, &QLocalServer::newConnection);
        QLocalSocket client;
        client.connectToServer(Application::LocalServerName());
        if(!client.waitForConnected(2000)) return false;
        return QTest::qWaitFor([&]{ return accepted.count() == 1; }, 2000);
    }
    static bool commandReaches(Receiver *receiver, const QString &command){
        QSignalSpy keys(receiver, &Receiver::KeyEvent);
        QByteArray frame;
        QDataStream stream(&frame, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
        stream << QStringLiteral("key ") + command;
        QLocalSocket client;
        client.connectToServer(Application::LocalServerName());
        if(!client.waitForConnected(2000)) return false;
        client.write(frame);
        client.waitForBytesWritten(2000);
        return QTest::qWaitFor([&]{ return keys.count() == 1; }, 2000) &&
            keys.first().first().toString() == command;
    }
private slots:
    void onlyTheSameUsersServerIsTold();
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        TestSupport::DisableWidgetAnimation();
        TreeBank::Initialize();
    }

    void aSearchIsOnlyASearchOnceTheSpaceIsTyped_data(){
        QTest::addColumn<QString>("line");
        QTest::addColumn<bool>("seeking");
        QTest::addColumn<QString>("words");
        QTest::newRow("the word alone") << QStringLiteral("search") << false << QString();
        QTest::newRow("the space") << QStringLiteral("search ") << true << QString();
        QTest::newRow("words") << QStringLiteral("search abc") << true << QStringLiteral("abc");
        QTest::newRow("the other name") << QStringLiteral("seek a b") << true << QStringLiteral("a b");
        QTest::newRow("capital, two spaces") << QStringLiteral("Search  x") << true << QStringLiteral("x");
        QTest::newRow("a longer word") << QStringLiteral("searchx y") << false << QString();
        QTest::newRow("another command") << QStringLiteral("set a") << false << QString();
        QTest::newRow("a space first") << QStringLiteral(" search a") << false << QString();
        QTest::newRow("empty") << QString() << false << QString();
    }
    void aSearchIsOnlyASearchOnceTheSpaceIsTyped(){
        QFETCH(QString, line);
        QFETCH(bool, seeking);
        QFETCH(QString, words);
        QString found;
        QCOMPARE(Receiver::SeekWords(line, &found), seeking);
        QCOMPARE(found, words);
    }

    static Receiver::TargetArgs TargetOf(const QUrl &link, const QUrl &image, const QString &selection){
        Receiver::TargetArgs target;
        if(!link.isEmpty() || !image.isEmpty())
            target.element = std::make_shared<JsWebElement>(nullptr, QPoint(5, 5), link, image, false);
        target.selection = selection;
        return target;
    }
    static Receiver::TargetTables DefaultTables(){
        Receiver::TargetTables tables;
        tables.field = MenuSchema::Default(QStringLiteral("TextMenu"));
        tables.link = MenuSchema::Default(QStringLiteral("LinkMenu"));
        tables.image = MenuSchema::Default(QStringLiteral("ImageMenu"));
        tables.selection = MenuSchema::Default(QStringLiteral("SelectionMenu"));
        tables.engines = QStringList() << QStringLiteral("g");
        tables.commands = QStringList() << QStringLiteral("Other");
        return tables;
    }
    static QStringList Described(const Receiver::SuggestRows &rows){
        QStringList list;
        for(const Receiver::SuggestRow &row : rows){
            const QString kind = row.kind == Receiver::SuggestRow::TargetSearch ? QStringLiteral("search")
                : row.kind == Receiver::SuggestRow::TargetCommand ? QStringLiteral("command")
                : QStringLiteral("action");
            list << QStringLiteral("%1:%2:%3").arg(row.serial).arg(kind, row.text);
            if(row.section != Receiver::TargetPartName(static_cast<Receiver::TargetPart>(row.serial))) list << QStringLiteral("bad heading");
        }
        return list;
    }

    void theCardForAnElementOffersWhatItsTablesRunFromWhatIsHeld(){
        const QUrl link(QStringLiteral("https://example.com/a/index.html"));
        const QUrl image(QStringLiteral("https://example.com/b/foo.png?x=1"));
        const Receiver::SuggestRows rows =
            Receiver::TargetRows(TargetOf(link, image, QStringLiteral("some words")), DefaultTables(), QString());
        const QStringList all = Described(rows);
        QVERIFY(!all.contains(QStringLiteral("bad heading")));
        QCOMPARE(all.mid(0, 5), QStringList()
                 << QStringLiteral("0:action:OpenLink") << QStringLiteral("0:action:DownloadLink")
                 << QStringLiteral("0:action:CopyLinkUrl")
                 << QStringLiteral("0:action:OpenLinkWithDefault") << QStringLiteral("0:command:Other"));
        const int imageAt = all.indexOf(QStringLiteral("1:action:OpenImage"));
        QVERIFY(imageAt > 5);
        QCOMPARE(all.mid(imageAt, 6), QStringList()
                 << QStringLiteral("1:action:OpenImage") << QStringLiteral("1:action:DownloadImage")
                 << QStringLiteral("1:action:CopyImage") << QStringLiteral("1:action:CopyImageUrl")
                 << QStringLiteral("1:action:OpenImageWithDefault")
                 << QStringLiteral("1:command:Other"));
        QVERIFY(all.indexOf(QStringLiteral("0:action:CopyLinkHtml")) > 5);
        QVERIFY(all.indexOf(QStringLiteral("0:action:CopyLinkHtml")) < imageAt);
        QVERIFY(all.indexOf(QStringLiteral("1:action:CopyImageHtml")) > imageAt + 5);
        QVERIFY(all.indexOf(QStringLiteral("0:action:LoadLink")) > 4);
        QVERIFY(all.indexOf(QStringLiteral("0:action:LoadLink")) < imageAt);
        QVERIFY2(all.contains(QStringLiteral("0:action:OpenOnRootNewWindow")), qPrintable(all.join(QLatin1Char(' '))));
        QVERIFY(all.contains(QStringLiteral("1:action:OpenImageInNewDirectoryBackground")));
        QCOMPARE(all.count(QStringLiteral("0:command:Other")), 1);
        QCOMPARE(all.count(QStringLiteral("0:action:OpenLink")), 1);
        QVERIFY(all.contains(QStringLiteral("2:action:Copy")));
        QVERIFY(all.contains(QStringLiteral("2:search:g")));
        QVERIFY(all.contains(QStringLiteral("2:action:OpenTextAsUrl")));
        QCOMPARE(all.filter(QRegularExpression(QStringLiteral("^2:"))).length(), 10);
        QVERIFY(all.contains(QStringLiteral("2:action:CopySelectedHtml")));
        QVERIFY(all.contains(QStringLiteral("2:action:SaveAllImage")));
        QCOMPARE(all.filter(QRegularExpression(QStringLiteral("^4:"))), QStringList()
                 << QStringLiteral("4:action:ClickElement") << QStringLiteral("4:action:FocusElement")
                 << QStringLiteral("4:action:HoverElement") << QStringLiteral("4:action:InspectElement"));
        QVERIFY(all.filter(QRegularExpression(QStringLiteral("^6:"))).isEmpty());
        for(const Receiver::SuggestRow &row : rows){
            if(row.kind == Receiver::SuggestRow::TargetAction)
                QVERIFY2(Page::IsValidAction(row.text), qPrintable(row.text));
            QVERIFY(!row.text.startsWith(QStringLiteral("ToggleMedia")));
        }
        for(const Receiver::SuggestRow &row : rows){
            if(row.kind != Receiver::SuggestRow::TargetCommand) continue;
            QCOMPARE(row.key, (row.serial == quint64(Receiver::TargetLink) ? link : image).toString());
        }

        Receiver::TargetTables tables = DefaultTables();
        tables.link = QStringLiteral("Reload,CopySelectedHtml,OpenLink,Separator");
        const QStringList odd = Described(Receiver::TargetRows(TargetOf(link, QUrl(), QString()), tables, QString()));
        QCOMPARE(odd.first(), QStringLiteral("0:action:OpenLink"));
        QVERIFY(!odd.join(QLatin1Char(' ')).contains(QStringLiteral("Reload")));
        QVERIFY(!odd.join(QLatin1Char(' ')).contains(QStringLiteral("CopySelectedHtml")));

        QVERIFY(Described(Receiver::TargetRows(TargetOf(link, QUrl(), QString()), DefaultTables(), QString()))
                .filter(QRegularExpression(QStringLiteral("^[123]:"))).isEmpty());
        QCOMPARE(Described(Receiver::TargetRows(TargetOf(QUrl(), QUrl(), QStringLiteral("w")), DefaultTables(), QString())),
                 QStringList() << QStringLiteral("2:action:Copy") << QStringLiteral("2:action:CopySelectedHtml")
                               << QStringLiteral("2:search:g") << QStringLiteral("2:action:OpenTextSeeker")
                               << QStringLiteral("2:action:OpenAllUrl")
                               << QStringLiteral("2:action:SaveAllUrl") << QStringLiteral("2:action:OpenAllImage")
                               << QStringLiteral("2:action:SaveAllImage") << QStringLiteral("2:action:OpenTextAsUrl")
                               << QStringLiteral("2:action:SaveTextAsUrl"));
        Receiver::TargetArgs player = TargetOf(QUrl(), image, QString());
        player.playable = true;
        Receiver::TargetTables media = DefaultTables();
        media.media = MenuSchema::Default(QStringLiteral("MediaMenu"));
        const QStringList played = Described(Receiver::TargetRows(player, media, QString()));
        QVERIFY(played.filter(QRegularExpression(QStringLiteral("^1:"))).isEmpty());
        QCOMPARE(played.mid(0, 4), QStringList()
                 << QStringLiteral("3:action:ToggleMediaPlayPause") << QStringLiteral("3:action:ToggleMediaMute")
                 << QStringLiteral("3:action:ToggleMediaLoop") << QStringLiteral("3:action:OpenMedia"));
        QVERIFY(played.contains(QStringLiteral("3:action:OpenMediaOnRootBackground")));
        QVERIFY(played.contains(QStringLiteral("3:action:ToggleMediaMute")));
        QVERIFY(played.contains(QStringLiteral("3:action:ToggleMediaControls")));

        Receiver::TargetArgs button;
        button.element = std::make_shared<PathElement>(QStringLiteral("//button[1]"));
        QCOMPARE(Described(Receiver::TargetRows(button, DefaultTables(), QString())), QStringList()
                 << QStringLiteral("4:action:ClickElement") << QStringLiteral("4:action:FocusElement")
                 << QStringLiteral("4:action:HoverElement") << QStringLiteral("4:action:InspectElement"));
        QCOMPARE(Receiver::TargetTag(button), Receiver::TargetPartName(Receiver::TargetElement) + QStringLiteral(" button"));

        Receiver::TargetArgs field;
        field.element = std::make_shared<JsWebElement>(nullptr, QPoint(5, 5), QUrl(), QUrl(), true);
        QCOMPARE(Receiver::TargetTag(field), Receiver::TargetPartName(Receiver::TargetField));
        Receiver::TargetTables fieldTables = DefaultTables();
        fieldTables.field = QStringLiteral("Paste,Separator,Reload,Undo");
        QCOMPARE(Described(Receiver::TargetRows(field, fieldTables, QString())), QStringList()
                 << QStringLiteral("6:action:Paste") << QStringLiteral("6:action:Undo")
                 << QStringLiteral("6:action:Redo") << QStringLiteral("6:action:Cut")
                 << QStringLiteral("6:action:SelectAll"));
        field.selection = QStringLiteral("w");
        const QStringList inField = Described(Receiver::TargetRows(field, DefaultTables(), QString()));
        QCOMPARE(inField.first(), QStringLiteral("6:action:Undo"));
        QVERIFY(inField.contains(QStringLiteral("2:action:Copy")));
        Receiver::TargetArgs found;
        found.element = std::make_shared<PathElement>(QStringLiteral("//input[1]"), QUrl(), true);
        const QStringList foundRows = Described(Receiver::TargetRows(found, DefaultTables(), QString()));
        QCOMPARE(foundRows.first(), QStringLiteral("6:action:Undo"));
        QVERIFY(foundRows.contains(QStringLiteral("4:action:InspectElement")));

        QCOMPARE(Described(Receiver::TargetRows(TargetOf(link, image, QString()), DefaultTables(), QStringLiteral("imageurl"))),
                 QStringList() << QStringLiteral("1:action:CopyImageUrl"));
    }

    void theCardForTabsOffersTheTabsMenuAndTheRest(){
        Receiver::TargetArgs tabs;
        tabs.nodes << 1 << 2;
        Receiver::TargetTables tables = DefaultTables();
        tables.node = MenuSchema::Default(QStringLiteral("TreeBarNodeMenu"));
        const QStringList all = Described(Receiver::TargetRows(tabs, tables, QString()));
        QVERIFY(!all.contains(QStringLiteral("bad heading")));
        QCOMPARE(all.mid(0, 6), QStringList()
                 << QStringLiteral("5:action:OpenNode") << QStringLiteral("5:action:OpenNodeOnNewWindow")
                 << QStringLiteral("5:action:OpenNodeWithDefault") << QStringLiteral("5:command:Other")
                 << QStringLiteral("5:action:RenameNode") << QStringLiteral("5:action:ReloadNode"));
        QVERIFY(all.contains(QStringLiteral("5:action:DeleteRightNode")));
        QVERIFY(all.indexOf(QStringLiteral("5:action:DeleteNode")) > all.indexOf(QStringLiteral("5:action:DeleteRightNode")));
        QVERIFY(all.contains(QStringLiteral("5:action:CopyNodeUrl")));
        QStringList named;
        for(const QString &row : all) named << row.section(QLatin1Char(':'), 2);
        for(const QString &name : { QStringLiteral("TreeBarMenu"), QStringLiteral("RecentlyClosed"),
                                    QStringLiteral("OpenCommand"), QStringLiteral("ToggleTrash"),
                                    QStringLiteral("RestoreNode"), QStringLiteral("SortMenu") })
            QVERIFY2(!named.contains(name), qPrintable(name));
        QCOMPARE(all.count(QStringLiteral("5:command:Other")), 1);

        tabs.trash = true;
        tables.node = MenuSchema::Default(QStringLiteral("TableNodeMenu"));
        QStringList trash = Described(Receiver::TargetRows(tabs, tables, QString()));
        trash.sort();
        QCOMPARE(trash, QStringList()
                 << QStringLiteral("5:action:CopyNodeAsLink") << QStringLiteral("5:action:CopyNodeTitle")
                 << QStringLiteral("5:action:CopyNodeUrl") << QStringLiteral("5:action:DeleteNode")
                 << QStringLiteral("5:action:OpenNode") << QStringLiteral("5:action:OpenNodeWithDefault")
                 << QStringLiteral("5:action:RestoreNode") << QStringLiteral("5:command:Other"));

        const MenuSchema::Table *bar = MenuSchema::Find(QStringLiteral("TreeBarNodeMenu"));
        QVERIFY(MenuSchema::IsKnown(*bar, QStringLiteral("CloneNode")));
        QVERIFY(MenuSchema::IsKnown(*bar, QStringLiteral("TreeBarMenu")));
        QVERIFY(!MenuSchema::IsKnown(*bar, QStringLiteral("Reload")));
        QVERIFY(!MenuSchema::IsKnown(*MenuSchema::Find(QStringLiteral("TableNodeMenu")), QStringLiteral("TreeBarMenu")));
        QVERIFY(!MenuSchema::IsKnown(*MenuSchema::Find(QStringLiteral("LinkMenu")), QStringLiteral("CloneNode")));
        for(const QString &name : bar->defaults.split(QLatin1Char(',')))
            QVERIFY2(MenuSchema::IsKnown(*bar, name), qPrintable(name));
    }

    void anElementFoundAgainHasToBeTheSameOne(){
        const QUrl link(QStringLiteral("https://example.com/a"));
        const QUrl image(QStringLiteral("https://example.com/b.png"));
        QVERIFY(Receiver::TargetHits(QStringLiteral("CopyImage")));
        QVERIFY(Receiver::TargetHits(QStringLiteral("ClickElement")));
        QVERIFY(Receiver::TargetHits(QStringLiteral("FocusElement")));
        QVERIFY(Receiver::TargetHits(QStringLiteral("HoverElement")));
        QVERIFY(!Receiver::TargetHits(QStringLiteral("CopyImageUrl")));
        QVERIFY(!Receiver::TargetHits(QStringLiteral("OpenLink")));

        const PathElement held(QStringLiteral("//a[2]"), link);
        QVERIFY(Receiver::IsSameTarget(held, PathElement(QStringLiteral("//a[2]"))));
        QVERIFY(!Receiver::IsSameTarget(held, PathElement(QStringLiteral("//a[3]"), link)));
        const JsWebElement point(nullptr, QPoint(5, 5), link, image, false);
        QVERIFY(Receiver::IsSameTarget(point, JsWebElement(nullptr, QPoint(9, 9), link, image, false)));
        QVERIFY(!Receiver::IsSameTarget(point, JsWebElement(nullptr, QPoint(9, 9), link, QUrl(), false)));
        QVERIFY(!Receiver::IsSameTarget(point, JsWebElement(nullptr, QPoint(9, 9), QUrl(), image, false)));
        const JsWebElement bare(nullptr, QPoint(5, 5), QUrl(), QUrl(), false);
        QVERIFY(!Receiver::IsSameTarget(bare, bare));
        QVERIFY(!Receiver::IsSameTarget(point, JsWebElement()));

        const PathElement field(QStringLiteral("//input[1]"), QUrl(), true);
        QVERIFY(Receiver::IsSameField(field, PathElement(QStringLiteral("//input[1]"), QUrl(), true)));
        QVERIFY(!Receiver::IsSameField(field, PathElement(QStringLiteral("//input[2]"), QUrl(), true)));
        const JsWebElement menu(nullptr, QPoint(5, 5), QUrl(), QUrl(), true);
        QVERIFY(Receiver::IsSameField(menu, FocusedElement(QStringLiteral("//x-field[1]"))));
        QVERIFY(!Receiver::IsSameField(menu, PathElement(QStringLiteral("//textarea[1]"), QUrl(), true)));
        QVERIFY(!Receiver::IsSameField(menu, PathElement(QStringLiteral("//div[1]"))));
        QVERIFY(!Receiver::IsSameField(menu, JsWebElement()));
    }

    void theCardForTabsRunsOnWhatIsStillWhereItWas(){
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        std::unique_ptr<MainWindow> window(Application::NewWindow(974));
        Application::SetCurrentWindow(window.get());
        auto forget = qScopeGuard([](){
            Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
            Application::RemoveWindow(974);
        });
        window->show();
        Receiver *r = window->GetTreeBank()->GetReceiver();
        using Row = Receiver::SuggestRow;
        const NodeList tree = TreeBank::GetViewRoot()->GetChildren();
        const NodeList trashed = TreeBank::GetTrashRoot()->GetChildren();
        auto drop = qScopeGuard([tree, trashed](){
            TreeBank::SetClipboardNodes(NodeList(), false);
            for(ViewNode *root : { TreeBank::GetViewRoot(), TreeBank::GetTrashRoot() }){
                const NodeList kept = root == TreeBank::GetViewRoot() ? tree : trashed;
                foreach(Node *nd, root->GetChildren()){
                    if(kept.contains(nd)) continue;
                    root->RemoveChild(nd);
                    nd->Delete();
                }
            }
        });
        const auto tab = [](ViewNode *parent, const QString &url){
            ViewNode *vn = parent->MakeChild();
            vn->SetHoldView(true);
            vn->SetUrl(QUrl(url));
            return vn;
        };
        ViewNode *a = tab(TreeBank::GetViewRoot(), QStringLiteral("https://example.invalid/zqa"));
        ViewNode *b = tab(TreeBank::GetViewRoot(), QStringLiteral("https://example.invalid/zqb"));
        ViewNode *closed = tab(TreeBank::GetTrashRoot(), QStringLiteral("https://example.invalid/zqclosed"));
        ViewNode *folder = TreeBank::GetViewRoot()->MakeChild();
        folder->SetTitle(QStringLiteral("zqFolder"));
        const quint64 folderSerial = folder->GetSerial();
        const auto run = [r](const Receiver::TargetArgs &target, const QString &name){
            r->OpenTarget(target);
            for(const Row &row : r->m_Rows){
                if(row.kind == Row::TargetAction && row.text == name){
                    r->RunTargetRow(row);
                    return true;
                }
            }
            return false;
        };

        Receiver::TargetArgs held;
        held.nodes << a->GetSerial() << closed->GetSerial() << b->GetSerial() << folderSerial;
        QApplication::clipboard()->setText(QStringLiteral("before"));
        QVERIFY(run(held, QStringLiteral("CopyNodeUrl")));
        QVERIFY(!r->isVisible());
        QTRY_COMPARE(QApplication::clipboard()->text(),
                     QStringLiteral("https://example.invalid/zqa\nhttps://example.invalid/zqb"));

        Receiver::TargetArgs one;
        one.nodes << b->GetSerial();
        QApplication::clipboard()->setText(QStringLiteral("before"));
        QVERIFY(run(one, QStringLiteral("CopyNodeUrl")));
        r->OpenTarget(one);
        QTest::qWait(100);
        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("before"));
        r->Dismiss();

        a->SetTitle(QStringLiteral("zqA"));
        b->SetTitle(QStringLiteral("zqB"));
        Receiver::TargetArgs two;
        two.nodes << a->GetSerial() << b->GetSerial() << folderSerial;
        QVERIFY(run(two, QStringLiteral("CopyNodeTitle")));
        QTRY_COMPARE(QApplication::clipboard()->text(), QStringLiteral("zqA\nzqB\nzqFolder"));
        QVERIFY(run(two, QStringLiteral("CopyNodeAsLink")));
        QTRY_COMPARE(QApplication::clipboard()->text(),
                     QStringLiteral("<a href=\"https://example.invalid/zqa\">zqA</a>\n"
                                    "<a href=\"https://example.invalid/zqb\">zqB</a>\n"));
        Receiver::TargetArgs onlyA;
        onlyA.nodes << a->GetSerial();
        const int before = TreeBank::GetViewRoot()->GetChildren().length();
        TreeBank::SetClipboardNodes(NodeList(), false);
        QVERIFY(run(onlyA, QStringLiteral("CopyNode")));
        QTRY_VERIFY(TreeBank::HasClipboardNodes());
        QVERIFY(TreeBank::ClipboardHolds(NodeList() << a));
        Receiver::TargetArgs onlyB;
        onlyB.nodes << b->GetSerial();
        QVERIFY(run(onlyB, QStringLiteral("PasteNode")));
        QTRY_COMPARE(TreeBank::GetViewRoot()->GetChildren().length(), before + 1);
        const NodeList after = TreeBank::GetViewRoot()->GetChildren();
        QCOMPARE(after[after.indexOf(b) + 1]->GetUrl(), QUrl(QStringLiteral("https://example.invalid/zqa")));
        QVERIFY(after[after.indexOf(b) + 1] != a);
        Receiver::TargetArgs folderOnly;
        folderOnly.nodes << folderSerial;
        TreeBank::SetClipboardNodes(NodeList(), false);
        QVERIFY(run(onlyA, QStringLiteral("CutNode")));
        QTRY_VERIFY(TreeBank::ClipboardHolds(NodeList() << a));
        QVERIFY(run(folderOnly, QStringLiteral("PasteNode")));
        QTRY_COMPARE(TreeBank::GetViewRoot()->GetChildren().indexOf(a),
                     TreeBank::GetViewRoot()->GetChildren().indexOf(folder) + 1);
        QCOMPARE(TreeBank::GetViewRoot()->GetChildren().length(), before + 1);

        Receiver::TargetArgs directory;
        directory.nodes << folderSerial;
        QCOMPARE(Receiver::HeldNode(folderSerial), folder);
        QVERIFY(r->NodeTag(directory).contains(QStringLiteral("zqFolder")));
        const int children = TreeBank::GetViewRoot()->GetChildren().length();
        QVERIFY(run(directory, QStringLiteral("MakeDirectory")));
        QTRY_COMPARE(TreeBank::GetViewRoot()->GetChildren().length(), children + 1);

        r->OpenCommand();
        r->RunRowAction(Row{ Row::Trash, Row::OpenTab, QString(), QString(), QString(), closed->GetSerial() },
                        Receiver::RowAction::OpenCard);
        QCOMPARE(r->m_Mode, Receiver::Target);
        QVERIFY(r->m_Target.trash);
        QCOMPARE(r->m_Target.nodes, QList<quint64>() << closed->GetSerial());
        r->Dismiss();
    }

    void theSelectionsHtmlIsTakenForItsOwnCard(){
        MainWindow window(975, QPoint(100, 100));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenTarget(TargetOf(QUrl(), QUrl(), QStringLiteral("w")));
        const quint64 request = r->m_TargetRequest;
        r->TakeSelectionHtml(request + 1, QStringLiteral("<b>other</b>"));
        QVERIFY(r->m_Target.selectionHtml.isEmpty());
        r->TakeSelectionHtml(request, QStringLiteral("<b>w</b>"));
        QCOMPARE(r->m_Target.selectionHtml, QStringLiteral("<b>w</b>"));
        r->Dismiss();
    }

    void nothingHeldIsThePlainBarAndTheAccessKeysOpenTheCard(){
        MainWindow window(976, QPoint(100, 100));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenTarget(Receiver::TargetArgs());
        QCOMPARE(r->m_Mode, Receiver::Command);
        r->Dismiss();
        const InputMap::KeyMap m_AccessKeyKeyMap =
            InputMapSchema::DefaultKeyMap(QStringLiteral("gadgets/accesskey/keymap"));
        QCOMPARE(m_AccessKeyKeyMap.value(QKeySequence(Qt::Key_G)), QStringLiteral("OpenCommand"));
        QCOMPARE(m_AccessKeyKeyMap.value(QKeySequence(Qt::Key_H)), QStringLiteral("OpenCommand"));
    }

    void theTagSaysWhatIsHeld(){
        const QUrl link(QStringLiteral("https://example.com/a/index.html"));
        const QUrl image(QStringLiteral("https://example.com/b/foo.png?x=1"));
        QCOMPARE(Receiver::TargetTag(TargetOf(link, QUrl(), QString())),
                 Receiver::TargetPartName(Receiver::TargetLink) + QStringLiteral(" index.html"));
        QCOMPARE(Receiver::TargetTag(TargetOf(link, image, QString())),
                 Receiver::TargetPartName(Receiver::TargetImage) + QStringLiteral(" foo.png"));
        const QString tag = Receiver::TargetTag(TargetOf(QUrl(), QUrl(), QString(40, QLatin1Char('a'))));
        QVERIFY(tag.contains(QString(25, QLatin1Char('a')) + QStringLiteral("...")));
        QVERIFY(!tag.contains(QString(26, QLatin1Char('a'))));
    }

    void aTargetIsRunOnlyInItsTabAsTheCurrentAndInItsLoad(){
        int a = 0, b = 0;
        const View *held = reinterpret_cast<const View*>(&a);
        const View *other = reinterpret_cast<const View*>(&b);
        QVERIFY(Receiver::IsLiveTarget(held, held, held, 7, 7));
        QVERIFY(!Receiver::IsLiveTarget(held, other, held, 7, 7));
        QVERIFY(!Receiver::IsLiveTarget(held, nullptr, held, 7, 7));
        QVERIFY(!Receiver::IsLiveTarget(held, held, other, 7, 7));
        QVERIFY(!Receiver::IsLiveTarget(held, held, held, 7, 8));
        QVERIFY(!Receiver::IsLiveTarget(nullptr, nullptr, nullptr, 7, 7));
    }

    void theCardForAnElementRunsNothingOnceItsViewHasGone(){
        MainWindow window(958, QPoint(100, 100));
        window.show();
        TreeBank *tb = window.GetTreeBank();
        Receiver *r = tb->GetReceiver();
        Receiver::TargetArgs target = TargetOf(QUrl(QStringLiteral("https://example.com/x")), QUrl(), QStringLiteral("held words"));
        r->OpenTarget(target);
        QVERIFY(r->isVisible());
        QCOMPARE(r->m_Mode, Receiver::Target);
        QVERIFY(!r->m_LineEdit->Tag().isEmpty());
        QVERIFY(!r->ShowsSourceBar());
        QVERIFY(!r->m_Rows.isEmpty());
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        r->m_LineEdit->setText(QStringLiteral("search abc"));
        QVERIFY(r->m_Rows.isEmpty());
        QVERIFY(!r->IsSeeking());
        r->SuitableAction();
        QVERIFY(r->isVisible());
        r->m_LineEdit->setText(QString());
        QVERIFY(!r->m_LineEdit->Tag().isEmpty());
        QCOMPARE(r->m_Mode, Receiver::Target);

        QApplication::clipboard()->setText(QStringLiteral("before"));
        r->m_LineEdit->setText(QStringLiteral("copy"));
        r->m_CurrentSuggestIndex = -1;
        for(int i = 0; i < r->m_Rows.length(); i++){
            if(r->m_Rows[i].serial == quint64(Receiver::TargetSelection) && r->m_Rows[i].text == QStringLiteral("Copy"))
                r->m_CurrentSuggestIndex = i;
        }
        QVERIFY(r->m_CurrentSuggestIndex != -1);
        const quint64 request = r->m_TargetRequest;
        r->SuitableAction();
        QVERIFY(!r->isVisible());
        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("before"));
        QVERIFY(r->m_TargetRequest != request);
        QTRY_VERIFY(!r->m_Target.element);

        r->OpenCommand();
        r->OpenTargetWhenFocused(target, nullptr);
        QTest::qWait(700);
        QCOMPARE(r->m_Mode, Receiver::Command);
        QVERIFY(r->isVisible());
        r->Dismiss();

        r->OpenTarget(target);
        r->OpenCommand();
        QVERIFY(!r->m_Target.element);
        QVERIFY(r->m_LineEdit->Tag().isEmpty());
        QVERIFY(r->ShowsSourceBar());
        r->Dismiss();
    }

    void theMenusOfAnElementOpenTheCardAndTheOthersDoNot(){
        for(const MenuSchema::Table &table : MenuSchema::Tables()){
            const QString name = QString::fromLatin1(table.name);
            QVERIFY2(!table.defaults.split(QLatin1Char(',')).contains(QStringLiteral("OpenCommand")),
                     qPrintable(name));
            QVERIFY2(!MenuSchema::IsKnown(table, QStringLiteral("OpenCommand")), qPrintable(name));
            const QJsonArray groups = MenuSchema::DescribeTable(SettingsIO::Map(), table)[QStringLiteral("groups")].toArray();
            for(const QJsonValue &group : groups)
                for(const QJsonValue &action : group.toObject()[QStringLiteral("actions")].toArray())
                    QVERIFY2(action.toObject()[QStringLiteral("name")].toString() != QStringLiteral("OpenCommand"),
                             qPrintable(name));
        }
        QCOMPARE(CommandMap::Resolve(QStringLiteral("opentargetcommand")), CommandMap::NotACommand);

        for(const char *name : { "TreeBarNodeMenu", "TableNodeMenu" })
            for(const QString &icon : { QStringLiteral("NewNode"), QStringLiteral("CloneNode"),
                                        QStringLiteral("DeleteNode"), QStringLiteral("RestoreNode") })
                QVERIFY(!MenuSchema::Default(QString::fromLatin1(name)).split(QLatin1Char(',')).contains(icon));
        for(const char *name : { "LinkMenu", "ImageMenu", "MediaMenu" })
            QVERIFY2(!MenuSchema::Default(QString::fromLatin1(name)).contains(QStringLiteral("Html")), name);
        const QString before = QStringLiteral("LinkUrl,OpenLink,DownloadLink,CopyLinkUrl,CopyLinkHtml,OpenLinkWithOtherBrowser,"
                                              "OpenTargetCommand");
        const MenuSchema::Table *link = MenuSchema::Find(QStringLiteral("LinkMenu"));
        const QString kept = QString(before).replace(QStringLiteral("OpenTargetCommand"), QStringLiteral("OpenCommand"));
        QJsonArray names;
        for(const QString &name : kept.split(QLatin1Char(','))) names << name;
        SettingsIO::Map settings;
        QVERIFY(!MenuSchema::Write(settings, *link, names).isEmpty());
        settings.insert(QStringLiteral("webview/menu/LinkMenu"), kept);
        QVERIFY(MenuSchema::Write(settings, *link, names).isEmpty());
        QCOMPARE(MenuSchema::Current(settings, *link), kept.split(QLatin1Char(',')));
    }

    void theSpaceMovesTheCardAndTakingItAwayMovesItBack(){
        MainWindow window(951, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        TreeBank *tb = window.GetTreeBank();
        Receiver *r = tb->GetReceiver();
        r->OpenCommand();
        QSignalSpy seeks(r, &Receiver::SeekText);

        r->m_LineEdit->setText(QStringLiteral("search"));
        const QRect middle = r->geometry();
        QVERIFY(qAbs(middle.center().x() - tb->rect().center().x()) <= 1);
        QCOMPARE(r->CurrentPlacement(), Receiver::Center);
        QCOMPARE(seeks.count(), 0);

        r->m_LineEdit->setText(QStringLiteral("search abc"));
        QCOMPARE(r->CurrentPlacement(), Receiver::TopRight);
        QCOMPARE(r->geometry().right() + 1, tb->width() - r->ScaleByDevice(RECEIVER_MARGIN));
        QCOMPARE(r->geometry().top(), r->ScaleByDevice(RECEIVER_MARGIN));
        QCOMPARE(r->m_LineEdit->text(), QStringLiteral("search abc"));
        QCOMPARE(seeks.count(), 1);
        QCOMPARE(seeks.takeLast().first().toString(), QStringLiteral("abc"));

        r->m_LineEdit->setText(QStringLiteral("search"));
        QCOMPARE(r->geometry(), middle);
        QCOMPARE(seeks.count(), 1);
        QVERIFY(seeks.takeLast().first().toString().isEmpty());
        QVERIFY(r->isVisible());
    }

    void enterOnASearchInTheFieldStaysAndThePipeDoesNotChange(){
        MainWindow window(952, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("search abc"));
        QSignalSpy seeks(r, &Receiver::SeekText);
        r->SuitableAction();
        QVERIFY(r->isVisible());
        QCOMPARE(seeks.count(), 1);
        QCOMPARE(seeks.first().at(0).toString(), QStringLiteral("abc"));
        QVERIFY(seeks.first().at(1).value<View::FindFlags>() & View::WrapsAroundDocument);
        r->OnAborted();
        QVERIFY(!r->isVisible());
        QVERIFY(seeks.last().at(0).toString().isEmpty());

        seeks.clear();
        r->ReceiveCommand(QStringLiteral("search def"));
        QVERIFY(!r->isVisible());
        QCOMPARE(seeks.count(), 1);
        QCOMPARE(seeks.first().at(0).toString(), QStringLiteral("def"));
    }

    void leavingATabTakesTheHighlightAndTheCard(){
        MainWindow window(953, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("search abc"));
        QSignalSpy seeks(r, &Receiver::SeekText);
        r->Dismiss();
        QVERIFY(!r->isVisible());
        QCOMPARE(seeks.count(), 1);
        QVERIFY(seeks.first().at(0).toString().isEmpty());

        seeks.clear();
        r->OpenCommand();
        seeks.clear();
        r->Dismiss();
        QCOMPARE(seeks.count(), 0);
    }

    void theWebSearchAsksForSuggestionsAndTakesOne(){
        Settings &s = Application::GlobalSettings();
        s.setValue(QStringLiteral("application/@EnableGoogleSuggest"), true);
        Application::LoadGlobalSettings();
        MainWindow window(953, QPoint(100, 100));
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        QObject::disconnect(r, &Receiver::SuggestRequest, nullptr, nullptr);
        QObject::disconnect(r, &Receiver::OpenQueryUrl, nullptr, nullptr);
        QSignalSpy asked(r, &Receiver::SuggestRequest);
        QSignalSpy searched(r, &Receiver::OpenQueryUrl);
        const QByteArray answer = QStringLiteral(
            "<?xml version=\"1.0\"?><toplevel><CompleteSuggestion><suggestion data=\"テスト\"/>"
            "</CompleteSuggestion><CompleteSuggestion><suggestion data=\"tea\"/></CompleteSuggestion></toplevel>").toUtf8();

        r->OpenQueryEditor();
        r->m_LineEdit->setFocus();
        QVERIFY(asked.isEmpty());
        r->m_LineEdit->setText(QStringLiteral("te"));
        QCOMPARE(asked.count(), 1);
        const QUrlQuery query(asked.takeFirst().first().toUrl());
        QCOMPARE(query.queryItemValue(QStringLiteral("q")), QStringLiteral("te"));
        QCOMPARE(query.queryItemValue(QStringLiteral("oe")), QStringLiteral("utf-8"));

        r->DisplaySuggest(answer);
        QCOMPARE(r->m_SuggestStrings, QStringList() << QStringLiteral("テスト") << QStringLiteral("tea"));
        QVERIFY(!r->ShowsSourceBar());

        r->m_LineEdit->setText(QString());
        QVERIFY(asked.isEmpty());
        QVERIFY(r->m_SuggestStrings.isEmpty());
        r->m_LineEdit->setText(QStringLiteral("  "));
        QVERIFY(asked.isEmpty());

        r->m_LineEdit->setText(QStringLiteral("te"));
        r->DisplaySuggest(answer);
        r->SelectNextSuggest();
        r->SuitableAction();
        QCOMPARE(searched.takeFirst().first().toString(), QStringLiteral("テスト"));
        QVERIFY(!r->isVisible());

        r->OpenQueryEditor();
        r->m_LineEdit->setFocus();
        r->m_LineEdit->setText(QStringLiteral("te"));
        r->DisplaySuggest(answer);
        r->SuitableAction();
        QCOMPARE(searched.takeFirst().first().toString(), QStringLiteral("te"));

        r->OpenQueryEditor();
        r->m_LineEdit->setFocus();
        r->m_LineEdit->setText(QStringLiteral("te"));
        r->DisplaySuggest(answer);
        QTest::mouseClick(r, Qt::LeftButton, Qt::NoModifier, r->SuggestRect(1).center());
        QCOMPARE(searched.takeFirst().first().toString(), QStringLiteral("tea"));
        QVERIFY(!r->isVisible());

        s.setValue(QStringLiteral("application/@EnableGoogleSuggest"), false);
        Application::LoadGlobalSettings();
        r->OpenQueryEditor();
        asked.clear();
        r->m_LineEdit->setText(QStringLiteral("te"));
        QVERIFY(asked.isEmpty());
    }

    void everyServiceIsAskedAndAnswersInItsOwnForm(){
        const QUrl google = SearchSuggest::Url(QStringLiteral("Google"), QStringLiteral("c++ a&b"));
        QCOMPARE(QUrlQuery(google).queryItemValue(QStringLiteral("q"), QUrl::FullyDecoded), QStringLiteral("c++ a&b"));
        QVERIFY(google.toString(QUrl::FullyEncoded).contains(QStringLiteral("c%2B%2B")));
        QCOMPARE(QUrlQuery(google).queryItemValue(QStringLiteral("oe")), QStringLiteral("utf-8"));
        QCOMPARE(SearchSuggest::Url(QStringLiteral("Bing"), QStringLiteral("a")).host(), QStringLiteral("www.bing.com"));
        QCOMPARE(QUrlQuery(SearchSuggest::Url(QStringLiteral("Bing"), QStringLiteral("a"))).queryItemValue(QStringLiteral("query")), QStringLiteral("a"));
        const QUrl duck = SearchSuggest::Url(QStringLiteral("DuckDuckGo"), QStringLiteral("a"));
        QCOMPARE(duck.host(), QStringLiteral("duckduckgo.com"));
        QCOMPARE(QUrlQuery(duck).queryItemValue(QStringLiteral("type")), QStringLiteral("list"));

        const QStringList both = QStringList() << QStringLiteral("\u30C6\u30B9\u30C8") << QStringLiteral("tea");
        QCOMPARE(SearchSuggest::Parse(QStringLiteral("[\"te\",[\"\u30C6\u30B9\u30C8\",\"tea\"]]").toUtf8()), both);
        QCOMPARE(SearchSuggest::Parse(QStringLiteral("<toplevel><CompleteSuggestion><suggestion data=\"\u30C6\u30B9\u30C8\"/>"
                                                     "</CompleteSuggestion><CompleteSuggestion><suggestion data=\"tea\"/>"
                                                     "</CompleteSuggestion></toplevel>").toUtf8()), both);
        QVERIFY(SearchSuggest::Parse("not an answer").isEmpty());
        QVERIFY(SearchSuggest::Parse("[\"te\"]").isEmpty());
    }

    void aHintTakesWhatTheTitleLeaves(){
        QCOMPARE(Receiver::HintWidth(600, 100, 200, 10), 200);
        QCOMPARE(Receiver::HintWidth(600, 100, 450, 10), 450);
        QCOMPARE(Receiver::HintWidth(600, 100, 550, 10), 490);
        QCOMPARE(Receiver::HintWidth(600, 500, 550, 10), 300);
    }

    void inputsPastWhatIsShownAreCounted(){
        const QList<int> widths{ 10, 20, 30 };
        QCOMPARE(Receiver::InputsWidth(widths, 3, 4, 8), 68);
        QCOMPARE(Receiver::InputsWidth(widths, 1, 4, 8), 22);
        QCOMPARE(Receiver::InputsWidth(widths, 0, 4, 8), 8);
    }

    void theCardsOpenFromTheLine(){
        MainWindow window(949, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        QCOMPARE(CommandMap::Resolve(QStringLiteral("OpenTextSeeker")), CommandMap::Signal);
        QVERIFY(CommandMap::AllCommands().contains(QStringLiteral("OpenQueryEditor")));
        QVERIFY(!r->CommandLabel(QStringLiteral("Reconfigure")).isEmpty());
        QVERIFY(r->CommandLabel(QStringLiteral("Up")).isEmpty());
        QVERIFY(r->CommandLabel(QStringLiteral("OpenSettings")).isEmpty());
        r->DispatchAction(QStringLiteral("OpenQueryEditor"));
        QCOMPARE(r->m_Mode, Receiver::Query);
        QCOMPARE(r->m_LineEdit->placeholderText(), Receiver::tr("Input Query."));
        r->DispatchAction(QStringLiteral("OpenUrlEditor"));
        QCOMPARE(r->m_Mode, Receiver::UrlEdit);
        QCOMPARE(r->m_LineEdit->placeholderText(), Receiver::tr("Enter a URL."));
        r->DispatchAction(QStringLiteral("OpenTextSeeker"));
        QCOMPARE(r->m_Mode, Receiver::Search);
        QCOMPARE(r->m_LineEdit->placeholderText(), Receiver::tr("Incremental Search"));
    }

    void theToolBarsAddressOpensTheLineOverIt(){
        MainWindow window(951, QPoint(0, 0));
        window.resize(760, 560);
        ToolBar *bar = window.findChild<ToolBar*>();
        QVERIFY(bar);
        window.show();
        bar->show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        LineEdit *field = bar->Field();
        QVERIFY(field->isVisible());
        QVERIFY(field->isReadOnly());
        QCOMPARE(field->focusPolicy(), Qt::NoFocus);
        QVERIFY(!field->completer());

        Receiver *r = window.GetTreeBank()->GetReceiver();
        QTest::mouseClick(field, Qt::LeftButton, Qt::NoModifier, QPoint(5, field->height() / 2));
        QVERIFY(r->isVisible());
        QVERIFY(r->IsAnchored());
        QCOMPARE(r->m_Anchor.data(), static_cast<QWidget*>(field));
        r->hide();
        QTest::mouseClick(field, Qt::RightButton, Qt::NoModifier, QPoint(5, field->height() / 2));
        QVERIFY(!r->isVisible());
    }

    void theWayUpFromTheTab(){
        const QUrl page(QStringLiteral("https://a.com/b/c?x=1#f"));
        const QString whole = page.toString();
        const QStringList up = QStringList() << QStringLiteral("https://a.com/b/c")
                                             << QStringLiteral("https://a.com/b/")
                                             << QStringLiteral("https://a.com/");
        QCOMPARE(Receiver::UrlAncestors(whole, page), up);
        QCOMPARE(Receiver::UrlAncestors(QStringLiteral("https://a.com/b/"), page),
                 QStringList() << QStringLiteral("https://a.com/b/c") << QStringLiteral("https://a.com/"));
        QCOMPARE(Receiver::UrlAncestors(QStringLiteral("HTTPS"), page), up);
        QVERIFY(Receiver::UrlAncestors(QStringLiteral("reload"), page).isEmpty());
        QVERIFY(Receiver::UrlAncestors(QStringLiteral("https://a.com/"), QUrl(QStringLiteral("https://a.com/"))).isEmpty());
        QVERIFY(Receiver::UrlAncestors(QStringLiteral("https://a.com"), QUrl(QStringLiteral("https://a.com"))).isEmpty());
        QCOMPARE(Receiver::UrlAncestors(QStringLiteral("https://a.com/b/d/"), QUrl(QStringLiteral("https://a.com/b/d/"))),
                 QStringList() << QStringLiteral("https://a.com/b/") << QStringLiteral("https://a.com/"));
        QVERIFY(Receiver::UrlAncestors(QStringLiteral("about:blank"), QUrl(QStringLiteral("about:blank"))).isEmpty());
    }

    void theWebOffersTheWayUpUnderWhatIsTyped(){
        MainWindow window(969, QPoint(100, 100));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        const QUrl page(QStringLiteral("https://a.com/b/c?x=1#f"));
        const auto titles = [](const Receiver::SuggestRows &rows){
            QStringList list;
            for(const Receiver::SuggestRow &row : rows) list << row.title;
            return list;
        };

        Receiver::SuggestRows rows = r->WebRows(page.toString(), page);
        QCOMPARE(titles(rows), QStringList() << page.toString()
                                             << QStringLiteral("https://a.com/b/c")
                                             << QStringLiteral("https://a.com/b/")
                                             << QStringLiteral("https://a.com/")
                                             << page.toString());
        QCOMPARE(rows.at(0).kind, Receiver::SuggestRow::OpenAddress);
        QCOMPARE(rows.at(2).kind, Receiver::SuggestRow::OpenAddress);
        QCOMPARE(rows.at(2).text, QStringLiteral("https://a.com/b/"));
        QCOMPARE(rows.last().kind, Receiver::SuggestRow::SearchWeb);

        rows = r->WebRows(QStringLiteral("htt"), page);
        QCOMPARE(rows.first().kind, Receiver::SuggestRow::SearchWeb);
        QCOMPARE(rows.length(), 4);
        QCOMPARE(rows.at(1).kind, Receiver::SuggestRow::OpenAddress);

        QCOMPARE(r->WebRows(QStringLiteral("reload"), page).length(), 1);
        QCOMPARE(r->WebRows(page.toString(), QUrl()).length(), 2);
        QCOMPARE(r->WebRows(QStringLiteral("about:blank"), QUrl(QStringLiteral("about:blank"))).length(), 2);
    }

    void overTheFieldItIsTheAddressBar(){
        MainWindow window(958, QPoint(0, 0));
        window.resize(760, 560);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        TreeBank *bank = window.GetTreeBank();
        Receiver *r = bank->GetReceiver();
        QObject::disconnect(r, nullptr, bank, nullptr);
        QSignalSpy urls(r, QOverload<QList<QUrl>>::of(&Receiver::OpenUrl));
        QSignalSpy queries(r, &Receiver::OpenQueryUrl);
        const bool joined = !r->IsPurged();

        QWidget *bar = new QWidget(&window);
        bar->setGeometry(0, 0, 760, 560);
        bar->show();
        QLineEdit *field = new QLineEdit(bar);
        field->setGeometry(100, 40, 300, 26);
        field->show();
        QToolButton *beside = new QToolButton(bar);
        beside->setGeometry(20, 40, 26, 26);
        beside->show();
        const auto open = [&](){
            r->OpenAtField(field);
            QVERIFY(r->isVisible());
            QVERIFY(r->IsAnchored());
            QCOMPARE(r->CurrentPlacement(), Receiver::Anchored);
        };

        open();
        QVERIFY(r->IsPurged());
        const QRect line(r->m_LineEdit->mapToGlobal(QPoint(0, 0)), r->m_LineEdit->size());
        const QRect at(field->mapToGlobal(QPoint(0, 0)), field->size());
        QCOMPARE(line.topLeft(), at.topLeft());
        QCOMPARE(line.height(), at.height());
        QVERIFY(r->width() >= at.width());
        QVERIFY(!r->m_LineEdit->IsPill());
        QCOMPARE(r->m_ActiveTab, int(Receiver::SuggestRow::Web) + 1);
        QCOMPARE(r->m_CurrentSuggestIndex, -1);

        r->m_LineEdit->setText(QStringLiteral("zzq qqz"));
        QVERIFY(!r->m_Rows.isEmpty());
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        r->m_WebSuggests = QStringList() << QStringLiteral("zzq qqz answered");
        r->RefillKeepingChoice();
        QCOMPARE(r->m_Rows.length(), 2);
        QCOMPARE(r->m_CurrentSuggestIndex, -1);

        r->hide();
        field->move(100, field->y() + r->screen()->availableGeometry().top() + 2
                         - field->mapToGlobal(QPoint(0, 0)).y());
        open();
        {
            const QRect topLine(r->m_LineEdit->mapToGlobal(QPoint(0, 0)), r->m_LineEdit->size());
            const QRect topAt(field->mapToGlobal(QPoint(0, 0)), field->size());
            QVERIFY(topAt.top() - r->screen()->availableGeometry().top() < r->ScaleByDevice(RECEIVER_PADDING));
            QCOMPARE(topLine.topLeft(), topAt.topLeft());
            QVERIFY(r->geometry().top() >= r->screen()->availableGeometry().top());
            QVERIFY(!r->m_Rows.isEmpty());
            QVERIFY(r->SuggestRect(0).top() > r->m_LineEdit->geometry().bottom());
        }
        r->hide();
        field->move(100, 40);
        {
            struct FirstShow : QObject {
                int height = -1;
                bool eventFilter(QObject *o, QEvent *e) override {
                    if(e->type() == QEvent::Show && height < 0)
                        height = static_cast<QWidget*>(o)->height();
                    return false;
                }
            } first;
            r->installEventFilter(&first);
            open();
            r->removeEventFilter(&first);
            QVERIFY(first.height > 0);
            QCOMPARE(first.height, r->height());
        }
        r->m_LineEdit->setText(QStringLiteral("zzq qqz"));

        field->move(140, 40);
        QCOMPARE(r->m_LineEdit->mapToGlobal(QPoint(0, 0)).x(), field->mapToGlobal(QPoint(0, 0)).x());

        r->SuitableAction();
        QVERIFY(!r->isVisible());
        QVERIFY(!r->IsAnchored());
        QCOMPARE(queries.count(), 0);
        QCOMPARE(urls.count(), 1);

        open();
        r->m_LineEdit->setText(QStringLiteral("example.invalid"));
        QCOMPARE(r->m_Rows.first().kind, Receiver::SuggestRow::OpenAddress);
        r->SelectNextSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->SuitableAction();
        QVERIFY(!r->isVisible());
        QCOMPARE(urls.count(), 1);

        QTRY_COMPARE(r->IsPurged(), !joined);

        open();
        r->OpenCommand();
        QVERIFY(!r->IsAnchored());
        QCOMPARE(r->CurrentPlacement(), Receiver::Center);
        r->hide();

        open();
        field->hide();
        QVERIFY(!r->isVisible());
        field->show();
        open();
        window.move(window.pos() + QPoint(30, 0));
        QTRY_VERIFY(!r->isVisible());
        QVERIFY(!r->IsAnchored());

        field->move(100, window.height() - 40);
        open();
        {
            const QRect bottomLine(r->m_LineEdit->mapToGlobal(QPoint(0, 0)), r->m_LineEdit->size());
            const QRect bottomAt(field->mapToGlobal(QPoint(0, 0)), field->size());
            QCOMPARE(bottomLine.topLeft(), bottomAt.topLeft());
            QCOMPARE(bottomLine.height(), bottomAt.height());
            QVERIFY(!r->m_Rows.isEmpty());
            QVERIFY(r->SuggestRect(0).bottom() < r->m_LineEdit->y());
            QVERIFY(r->SourceBarRect(0).bottom() < r->m_LineEdit->y());
            QVERIFY(r->SuggestRect(0).bottom() < r->SourceBarRect(0).top());
            const int tabs = r->mapToGlobal(r->SourceBarRect(0).topLeft()).y();
            r->m_LineEdit->setText(QStringLiteral("zzq"));
            QCOMPARE(r->mapToGlobal(r->SourceBarRect(0).topLeft()).y(), tabs);
            r->m_LineEdit->setText(r->CurrentAddress().toString());
            QVERIFY(r->geometry().top() >= bank->mapToGlobal(QPoint(0, 0)).y());
        }
        r->m_LineEdit->setText(QStringLiteral("zzq qqz"));
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        r->SuitableAction();
        QCOMPARE(queries.count(), 0);
        QCOMPARE(urls.count(), 2);
        field->move(100, 40);

        window.move(0, 300);
        QCoreApplication::processEvents();
        open();
        r->m_LineEdit->setText(QStringLiteral("zzq"));
        r->m_WebSuggests.clear();
        for(int i = 0; i < 40; i++) r->m_WebSuggests << QStringLiteral("zzq %1").arg(i);
        r->RefillKeepingChoice();
        QVERIFY(r->geometry().bottom() <= window.screen()->availableGeometry().bottom());
        r->hide();
        window.move(0, 0);
        QCoreApplication::processEvents();

        r->OpenAtField(nullptr);
        QVERIFY(!r->IsAnchored());
        QCOMPARE(r->CurrentPlacement(), Receiver::Center);
        QCOMPARE(r->m_ActiveTab, int(Receiver::SuggestRow::Web) + 1);
        QVERIFY(r->m_LineEdit->IsPill());
        r->hide();
    }

    void theTabOfTheWebAsksAndOffersWhatIsAnswered(){
        Settings &s = Application::GlobalSettings();
        s.setValue(QStringLiteral("application/@EnableGoogleSuggest"), true);
        s.setValue(QStringLiteral("application/@SuggestEngine"), QStringLiteral("Bing"));
        Application::LoadGlobalSettings();
        MainWindow window(952, QPoint(100, 100));
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        QObject::disconnect(r, &Receiver::SuggestRequest, nullptr, nullptr);
        QObject::disconnect(r, &Receiver::OpenQueryUrl, nullptr, nullptr);
        QSignalSpy asked(r, &Receiver::SuggestRequest);
        QSignalSpy searched(r, &Receiver::OpenQueryUrl);
        const QByteArray answer = "[\"te\",[\"te\",\"tea\",\"tee\"]]";
        const int web = Receiver::SuggestRow::Web + 1;

        r->OpenCommand();
        r->m_LineEdit->setFocus();
        r->m_LineEdit->setText(QStringLiteral("te"));
        QVERIFY(asked.isEmpty());
        r->DisplaySuggest(answer);
        QVERIFY(!r->m_SuggestStrings.contains(QStringLiteral("tea")));

        r->SelectTab(web);
        QCOMPARE(asked.count(), 1);
        QCOMPARE(asked.takeFirst().first().toUrl().host(), QStringLiteral("www.bing.com"));
        QCOMPARE(r->m_SuggestStrings, QStringList() << QStringLiteral("te"));
        r->DisplaySuggest(answer);
        QCOMPARE(r->m_SuggestStrings, QStringList() << QStringLiteral("te") << QStringLiteral("tea") << QStringLiteral("tee"));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        r->SelectNextSuggest();
        r->DisplaySuggest("[\"te\",[\"tax\",\"tea\"]]");
        QCOMPARE(r->m_SuggestStrings[r->m_CurrentSuggestIndex], QStringLiteral("tea"));
        r->SuitableAction();
        QCOMPARE(searched.takeFirst().first().toString(), QStringLiteral("tea"));
        QVERIFY(!r->isVisible());

        r->OpenCommand();
        r->m_LineEdit->setFocus();
        r->SelectTab(web);
        QVERIFY(asked.isEmpty());
        r->m_LineEdit->setText(QStringLiteral("te"));
        r->DisplaySuggest(answer);
        asked.clear();
        r->m_LineEdit->setText(QStringLiteral("https://example.com/"));
        QVERIFY(asked.isEmpty());
        QVERIFY(!r->m_SuggestStrings.contains(QStringLiteral("tea")));
        r->DisplaySuggest(answer);
        QVERIFY(!r->m_SuggestStrings.contains(QStringLiteral("tea")));
        r->m_LineEdit->setText(QStringLiteral("te"));
        r->DisplaySuggest(answer);
        asked.clear();
        r->m_LineEdit->setText(QString());
        QVERIFY(asked.isEmpty());
        r->m_LineEdit->setText(QStringLiteral("x"));
        QVERIFY(!r->m_SuggestStrings.contains(QStringLiteral("tea")));

        r->m_LineEdit->setText(QStringLiteral("te"));
        r->DisplaySuggest(answer);
        r->SelectTab(0);
        QVERIFY(!r->m_SuggestStrings.contains(QStringLiteral("tea")));
        r->DisplaySuggest(answer);
        QVERIFY(!r->m_SuggestStrings.contains(QStringLiteral("tea")));

        s.setValue(QStringLiteral("application/@EnableGoogleSuggest"), false);
        s.remove(QStringLiteral("application/@SuggestEngine"));
        Application::LoadGlobalSettings();
        asked.clear();
        r->SelectTab(web);
        QVERIFY(asked.isEmpty());
    }

    void aSearchHasNoListAndALateAnswerIsNotShown(){
        MainWindow window(954, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->SetSuggest(QStringList() << QStringLiteral("a") << QStringLiteral("b"));
        r->SelectNextSuggest();
        r->m_LineEdit->setText(QStringLiteral("search a"));
        QVERIFY(r->m_SuggestStrings.isEmpty());
        QCOMPARE(r->m_CurrentSuggestIndex, -1);

        r->m_LineEdit->setFocus();
        r->DisplaySuggest("<toplevel><suggestion data=\"x\"/></toplevel>");
        QVERIFY(r->m_SuggestStrings.isEmpty());
    }

    void theFirstRowIsUnderTheFieldAndTheArrowsGoDownIt(){
        MainWindow window(955, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->SetSuggest(QStringList() << QStringLiteral("a") << QStringLiteral("b") << QStringLiteral("c"));
        QCOMPARE(r->VisibleSuggestCount(), 3);
        QVERIFY(r->SuggestRect(0).top() >= r->m_LineEdit->geometry().bottom());
        QVERIFY(r->SuggestRect(1).top() > r->SuggestRect(0).top());
        QCOMPARE(r->SuggestAt(r->SuggestRect(1).center()), 1);
        QCOMPARE(r->SuggestAt(r->m_LineEdit->geometry().center()), -1);
        QVERIFY(r->rect().contains(r->SuggestRect(2)));

        r->SelectNextSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->SelectNextSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, 1);
        r->SelectPrevSuggest();
        r->SelectPrevSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, 2);
        r->SelectNextSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        QStringList many;
        for(int i = 0; i < 200; i++) many << QString::number(i);
        r->SetSuggest(many);
        QVERIFY(r->VisibleSuggestCount() < 200);
        QVERIFY(r->geometry().bottom() < window.GetTreeBank()->height());

        r->SetSuggest(QStringList() << QStringLiteral("a") << QStringLiteral("b") << QStringLiteral("c"));
        r->SelectPrevSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, 2);
        for(int h = window.GetTreeBank()->height(); h > 0 && r->VisibleSuggestCount() == 3; h--)
            r->ResizeNotify(QSize(1000, h));
        QVERIFY(r->VisibleSuggestCount() < 3);
        QCOMPARE(r->m_CurrentSuggestIndex, 2);
        QVERIFY(r->m_SuggestScroll > 0);
        QVERIFY(r->m_CurrentSuggestIndex < r->m_SuggestScroll + r->VisibleSuggestCount());
    }

    void theCardDoesNotHangOnTheNotifier(){
        MainWindow window(956, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        TreeBank *tb = window.GetTreeBank();
        Receiver *r = tb->GetReceiver();
        r->OpenCommand();
        const QRect with = r->geometry();
        tb->GetNotifier()->SetStatus(QStringLiteral("status"));
        QVERIFY(tb->GetNotifier()->isVisible());
        r->ResizeNotify(tb->size());
        QCOMPARE(r->geometry(), with);
    }

    void aCompositionIsReadWhereItIsTyped(){
        MainWindow window(957, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("ab"));
        r->m_LineEdit->setCursorPosition(1);
        QInputMethodEvent composing(QStringLiteral("x"), {});
        QCoreApplication::sendEvent(r->m_LineEdit, &composing);
        QCOMPARE(r->m_LineString, QStringLiteral("axb"));
        QInputMethodEvent off;
        QCoreApplication::sendEvent(r->m_LineEdit, &off);
        QCOMPARE(r->m_LineString, QStringLiteral("ab"));
    }

    void committingACompositionDoesNotSeekAgain(){
        MainWindow window(958, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        const QString word = QStringLiteral("11月");
        for(int pass = 0; pass < 2; pass++){
            const QString prefix = pass ? QStringLiteral("search ") : QString();
            if(pass) r->OpenCommand(); else r->OpenTextSeeker();
            r->m_LineEdit->setText(prefix);
            QSignalSpy seeks(r, &Receiver::SeekText);
            QInputMethodEvent composing(word, {});
            QCoreApplication::sendEvent(r->m_LineEdit, &composing);
            QCOMPARE(seeks.count(), 1);
            QCOMPARE(seeks.last().first().toString(), word);
            QInputMethodEvent commit;
            commit.setCommitString(word);
            QCoreApplication::sendEvent(r->m_LineEdit, &commit);
            QCOMPARE(r->m_LineEdit->text(), prefix + word);
            QCOMPARE(seeks.count(), 1);
            r->OnAborted();
        }
    }

    void aMiniMapPutUpTakesTheWordsBeingLookedFor(){
        MainWindow window(961, QPoint(100, 100));
        window.show();
        TreeBank *tb = window.GetTreeBank();
        if(tb->GetMiniMap()) tb->ToggleMiniMap();
        Receiver *r = tb->GetReceiver();
        r->OpenTextSeeker();
        r->m_LineEdit->setText(QStringLiteral("abc"));
        QCOMPARE(r->SoughtWords(), QStringLiteral("abc"));
        tb->ToggleMiniMap();
        QVERIFY(tb->GetMiniMap());
        QCOMPARE(tb->GetMiniMap()->m_SeekTerm, QStringLiteral("abc"));
        r->OnAborted();
        QVERIFY(tb->GetMiniMap()->m_SeekTerm.isEmpty());
        tb->DisplayViewTree();
        QVERIFY(tb->IsDisplayingTableView());
        r->OpenTextSeeker();
        r->m_LineEdit->setText(QStringLiteral("abc"));
        QVERIFY(tb->GetMiniMap()->m_SeekTerm.isEmpty());
        r->SuitableAction();
        QVERIFY(tb->GetMiniMap()->m_SeekTerm.isEmpty());
        tb->GetGadgets()->Deactivate();
        tb->ToggleMiniMap();
        r->OnAborted();
        Application::GlobalSettings().remove(QStringLiteral("mainwindow/minimap961"));
    }

    void theCountShowsWhileSeekingAndGoesWithTheWords(){
        MainWindow window(960, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("reload"));
        r->SetFindCount(1, 9);
        QVERIFY(r->m_LineEdit->Count().isEmpty());
        r->m_LineEdit->setText(QStringLiteral("search abc"));
        r->SetFindCount(1, 190);
        QCOMPARE(r->m_LineEdit->Count(), QStringLiteral("1 / 190"));
        r->m_LineEdit->setText(QStringLiteral("search"));
        QVERIFY(r->m_LineEdit->Count().isEmpty());
        r->m_LineEdit->setText(QStringLiteral("search abc"));
        r->SetFindCount(0, 0);
        QCOMPARE(r->m_LineEdit->Count(), QStringLiteral("0 / 0"));
        r->OnAborted();
        QVERIFY(r->m_LineEdit->Count().isEmpty());
    }

    void theCloseButtonClosesTheCard(){
        MainWindow window(960, QPoint(100, 100));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        LineEdit *field = r->m_LineEdit;

        r->OpenCommand();
        QVERIFY(r->isVisible());
        QVERIFY(!field->CloseRect().isEmpty());
        QVERIFY(field->rect().contains(field->CloseRect()));
        field->setText(QStringLiteral("abc"));
        QTest::mousePress(field, Qt::LeftButton, Qt::NoModifier, QPoint(5, field->height() / 2));
        QTest::mouseRelease(field, Qt::LeftButton, Qt::NoModifier, field->CloseRect().center());
        QVERIFY(r->isVisible());
        QCOMPARE(field->text(), QStringLiteral("abc"));
        QTest::mouseClick(field, Qt::LeftButton, Qt::NoModifier, field->CloseRect().center());
        QVERIFY(!r->isVisible());

        r->OpenTextSeeker();
        field->setText(QStringLiteral("abc"));
        r->SetFindCount(1, 9);
        QCOMPARE(field->Count(), QStringLiteral("1 / 9"));
        QVERIFY(field->textMargins().right() >= field->width() - field->CloseRect().left());
        QSignalSpy seek(r, &Receiver::SeekText);
        QTest::mouseClick(field, Qt::LeftButton, Qt::NoModifier, field->CloseRect().center());
        QVERIFY(!r->isVisible());
        QCOMPARE(seek.count(), 1);
        QVERIFY(seek.at(0).at(0).toString().isEmpty());

        QWidget *bar = new QWidget(&window);
        bar->setGeometry(0, 0, 760, 560);
        bar->show();
        QLineEdit *address = new QLineEdit(bar);
        address->setGeometry(100, 40, 300, 26);
        address->show();
        r->OpenAtField(address);
        QCOMPARE(r->CurrentPlacement(), Receiver::Anchored);
        QVERIFY(field->rect().contains(field->CloseRect()));
        QTest::mouseClick(field, Qt::LeftButton, Qt::NoModifier, field->CloseRect().center());
        QVERIFY(!r->isVisible());
        QVERIFY(!r->IsAnchored());

        LineEdit plain;
        plain.resize(200, 30);
        QVERIFY(plain.CloseRect().isEmpty());
        QCOMPARE(plain.textMargins().right(), 0);
    }

    void aSearchOverTheAddressBarStaysAndCounts(){
        MainWindow window(991, QPoint(0, 0));
        window.resize(760, 560);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        TreeBank *bank = window.GetTreeBank();
        Receiver *r = bank->GetReceiver();
        QObject::disconnect(r, nullptr, bank, nullptr);
        QSignalSpy urls(r, QOverload<QList<QUrl>>::of(&Receiver::OpenUrl));
        QSignalSpy seeks(r, &Receiver::SeekText);
        QWidget *bar = new QWidget(&window);
        bar->setGeometry(0, 0, 760, 560);
        bar->show();
        QLineEdit *field = new QLineEdit(bar);
        field->setGeometry(100, 40, 300, 26);
        field->show();

        r->OpenAtField(field);
        r->m_LineEdit->setText(QStringLiteral("search abc"));
        QVERIFY(r->IsSeeking());
        QCOMPARE(r->CurrentPlacement(), Receiver::Anchored);
        QVERIFY(r->m_Rows.isEmpty());
        QVERIFY(r->m_WebSuggests.isEmpty());
        QCOMPARE(seeks.last().first().toString(), QStringLiteral("abc"));
        QCOMPARE(seeks.last().at(1).value<View::FindFlags>(), View::FindFlags(View::HighlightAllOccurrences));

        r->SuitableAction();
        QVERIFY(r->isVisible());
        QVERIFY(r->IsAnchored());
        QCOMPARE(urls.count(), 0);
        QCOMPARE(seeks.last().first().toString(), QStringLiteral("abc"));
        QCOMPARE(seeks.last().at(1).value<View::FindFlags>(), View::FindFlags(View::WrapsAroundDocument));
        r->SetFindCount(2, 7);
        QCOMPARE(r->m_LineEdit->Count(), QStringLiteral("2 / 7"));

        r->DismissFocusLeft();
        QVERIFY(!r->isVisible());
        QVERIFY(seeks.last().first().toString().isEmpty());
        QVERIFY(r->m_LineEdit->Count().isEmpty());

        r->OpenAtField(field);
        r->m_LineEdit->setText(QStringLiteral("zzq qqz"));
        QVERIFY(!r->IsSeeking());
        r->SuitableAction();
        QVERIFY(!r->isVisible());
        QCOMPARE(urls.count(), 1);

        r->OpenCommand();
        r->SelectTab(int(Receiver::SuggestRow::Commands) + 1);
        r->m_LineEdit->setText(QStringLiteral("search abc"));
        QVERIFY(r->IsSeeking());
        QCOMPARE(r->CurrentPlacement(), Receiver::TopRight);
        QVERIFY(r->m_Rows.isEmpty());
        QCOMPARE(seeks.last().first().toString(), QStringLiteral("abc"));
        r->SuitableAction();
        QVERIFY(r->isVisible());
        QCOMPARE(seeks.last().at(1).value<View::FindFlags>(), View::FindFlags(View::WrapsAroundDocument));
        r->SetFindCount(1, 3);
        QCOMPARE(r->m_LineEdit->Count(), QStringLiteral("1 / 3"));
        r->OnAborted();
        QVERIFY(seeks.last().first().toString().isEmpty());
        r->OpenCommand();
        r->SelectTab(int(Receiver::SuggestRow::Web) + 1);
        r->m_LineEdit->setText(QStringLiteral("search abc"));
        QVERIFY(!r->IsSeeking());
        r->hide();
    }

    void onlyTheMiddleCardGoesWhenTheKeyboardGoesElsewhere(){
        QVERIFY(Receiver::ShouldCloseOnFocusLeft(true, false, false, true));
        QVERIFY(!Receiver::ShouldCloseOnFocusLeft(false, false, false, true));
        QVERIFY(!Receiver::ShouldCloseOnFocusLeft(true, true, false, true));
        QVERIFY(!Receiver::ShouldCloseOnFocusLeft(true, false, true, true));
        QVERIFY(!Receiver::ShouldCloseOnFocusLeft(true, false, false, false));
    }

    void aWordBeingTypedIsOfferedTheCommands(){
        MainWindow window(959, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();

        r->m_LineEdit->setText(QStringLiteral("toggletr"));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Commands);
        QCOMPARE(r->m_SuggestStrings.first(), QStringLiteral("ToggleTreeBar"));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        r->m_LineEdit->setText(QStringLiteral("reload"));
        QVERIFY(r->m_SuggestStrings.contains(QStringLiteral("Reload")));
        QCOMPARE(r->m_CurrentSuggestIndex, -1);

        r->m_LineEdit->setText(QStringLiteral("reload now"));
        QVERIFY(r->m_SuggestStrings.isEmpty());
        QVERIFY(r->m_Rows.isEmpty());

        r->m_LineEdit->setText(QStringLiteral("unset "));
        QVERIFY(r->ShowsSourceBar());
    }

    void withNothingTypedEverySourceHasSomethingToSay(){
        MainWindow window(962, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        QVERIFY(r->HasHeading(0));
        QSet<int> sources;
        for(const Receiver::SuggestRow &row : r->m_Rows) sources << row.source;
        QVERIFY(sources.contains(Receiver::SuggestRow::Commands));
        QVERIFY(sources.contains(Receiver::SuggestRow::Settings));
        QVERIFY(sources.contains(Receiver::SuggestRow::Web));
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        foreach(const QString &name, CommandMap::FirstCommands(false)){
            QVERIFY(r->m_SuggestStrings.contains(name));
            QCOMPARE(CommandMap::Resolve(name), CommandMap::Signal);
        }

        const QStringList history = std::exchange(Receiver::m_History, QStringList());
        QStringList first;
        for(const Receiver::SuggestRow &row : r->CommandRows(QString(), 3)) first << row.text;
        Receiver::m_History = history;
        QCOMPARE(first, QStringList() << QStringLiteral("OpenSettings")
                                      << QStringLiteral("NewViewNode") << QStringLiteral("DisplayViewTree"));
        const QStringList overList = CommandMap::FirstCommands(true);
        QCOMPARE(overList.last(), QStringLiteral("Deactivate"));
        QVERIFY(!overList.contains(QStringLiteral("DisplayViewTree")));
        foreach(const QString &name, overList)
            QVERIFY(Receiver::AnswersHere(name, true, true));
    }

    void whatIsTypedCanBeSearchedForAndAnAddressOpened(){
        MainWindow window(963, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        QObject::disconnect(r, nullptr, window.GetTreeBank(), nullptr);
        QSignalSpy urls(r, QOverload<QList<QUrl>>::of(&Receiver::OpenUrl));
        QSignalSpy queries(r, &Receiver::OpenQueryUrl);

        r->m_LineEdit->setText(QStringLiteral("zzq qqz"));
        QCOMPARE(r->m_Rows.first().kind, Receiver::SuggestRow::SearchWeb);
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->SuitableAction();
        QVERIFY(!r->isVisible());
        QCOMPARE(queries.count(), 1);
        QCOMPARE(queries.first().first().toString(), QStringLiteral("zzq qqz"));

        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("example.com"));
        QCOMPARE(r->m_Rows.first().kind, Receiver::SuggestRow::OpenAddress);
        QCOMPARE(r->m_Rows.at(1).kind, Receiver::SuggestRow::SearchWeb);
        QVERIFY(r->HasHeading(0));
        QVERIFY(!r->HasHeading(1));
        r->SuitableAction();
        QCOMPARE(urls.count(), 1);
    }

    void theSourcesShareTheRows(){
        const auto rows = [](Receiver::SuggestRow::Source source, int count){
            Receiver::SuggestRows list;
            for(int i = 0; i < count; i++)
                list << Receiver::SuggestRow{ source, Receiver::SuggestRow::RunCommand, QString(), QString(), QString(), 0 };
            return list;
        };
        const auto c = rows(Receiver::SuggestRow::Commands, 20), t = rows(Receiver::SuggestRow::Tabs, 20),
            w = rows(Receiver::SuggestRow::Web, 1), none = Receiver::SuggestRows();

        Receiver::SuggestRows all = Receiver::ComposeRows({ c, t, w }, 9);
        QCOMPARE(all.length(), 9);
        QCOMPARE(all.first().source, Receiver::SuggestRow::Commands);
        QCOMPARE(all.at(4).source, Receiver::SuggestRow::Tabs);
        QCOMPARE(all.last().source, Receiver::SuggestRow::Web);

        QCOMPARE(Receiver::ComposeRows({ w, c }, 3).first().source, Receiver::SuggestRow::Web);
        QCOMPARE(Receiver::ComposeRows({ w, none }, 5).length(), 1);
        QVERIFY(Receiver::ComposeRows({ none, none }, 5).isEmpty());
    }

    void aTallerWindowOffersMore(){
        MainWindow window(968, QPoint(100, 100));
        window.resize(1000, 400);
        window.show();
        TreeBank *tb = window.GetTreeBank();
        Receiver *r = tb->GetReceiver();
        r->OpenCommand();
        const int few = r->VisibleSuggestCount();
        QVERIFY(r->geometry().bottom() < tb->height());

        window.resize(1000, 1000);
        r->OpenCommand();
        QVERIFY(r->VisibleSuggestCount() > few);
        QCOMPARE(r->geometry().top(), tb->height() / 12);
        QVERIFY(tb->height() - r->geometry().bottom() - 1 >= tb->height() / 12);
        QCOMPARE(r->VisibleSuggestCount(), r->m_SuggestStrings.length());
    }

    void theFirstWordNarrowsToASource_data(){
        QTest::addColumn<QString>("line");
        QTest::addColumn<int>("source");
        QTest::addColumn<QString>("rest");
        QTest::newRow("tab") << QStringLiteral("tab foo") << int(Receiver::SuggestRow::Tabs) << QStringLiteral("foo");
        QTest::newRow("tabs") << QStringLiteral("Tabs a b") << int(Receiver::SuggestRow::Tabs) << QStringLiteral("a b");
        QTest::newRow("trash") << QStringLiteral("trash ") << int(Receiver::SuggestRow::Trash) << QString();
        QTest::newRow("settings") << QStringLiteral("settings x") << int(Receiver::SuggestRow::Settings) << QStringLiteral("x");
        QTest::newRow("commands") << QStringLiteral("command y") << int(Receiver::SuggestRow::Commands) << QStringLiteral("y");
        QTest::newRow("com") << QStringLiteral("com y") << int(Receiver::SuggestRow::Commands) << QStringLiteral("y");
        QTest::newRow("set") << QStringLiteral("set x") << int(Receiver::SuggestRow::Settings) << QStringLiteral("x");
        QTest::newRow("setting") << QStringLiteral("Setting x") << int(Receiver::SuggestRow::Settings) << QStringLiteral("x");
        QTest::newRow("web") << QStringLiteral("web w") << int(Receiver::SuggestRow::Web) << QStringLiteral("w");
        QTest::newRow("tra") << QStringLiteral("tra w") << int(Receiver::SuggestRow::Trash) << QStringLiteral("w");
    }
    void theFirstWordNarrowsToASource(){
        QFETCH(QString, line);
        QFETCH(int, source);
        QFETCH(QString, rest);
        Receiver::SuggestRow::Source found;
        QString words;
        QVERIFY(Receiver::SourceWord(line, &found, &words));
        QCOMPARE(int(found), source);
        QCOMPARE(words, rest);
    }

    void aSourceWordIsTheBeginningOfAName(){
        QVERIFY(Receiver::SourceWord(QStringLiteral("tab"), nullptr, nullptr));
        QVERIFY(!Receiver::SourceWord(QStringLiteral("tabx y"), nullptr, nullptr));
        bool unset = false;
        Receiver::SuggestRow::Source found = Receiver::SuggestRow::SourceCount;
        QVERIFY(Receiver::SourceWord(QStringLiteral("unset x"), &found, nullptr, &unset));
        QCOMPARE(found, Receiver::SuggestRow::Settings);
        QVERIFY(unset);
        QVERIFY(Receiver::SourceWord(QStringLiteral("set x"), nullptr, nullptr, &unset));
        QVERIFY(!unset);
        QVERIFY(!Receiver::SourceWord(QStringLiteral("we are"), nullptr, nullptr));
        QVERIFY(!Receiver::SourceWord(QStringLiteral("hi there"), nullptr, nullptr));
    }

    void aSearchEngineOfThatNameIsStillAsked(){
        const QString name = QStringLiteral("com");
        const bool had = Page::GetSearchEngineMap().contains(name);
        const SearchEngine before = Page::GetSearchEngine(name);
        Page::RegisterSearchEngine(name, SearchEngine() << QStringLiteral("https://example.invalid/?q=%1"));
        QVERIFY(!Receiver::SourceWord(QStringLiteral("com y"), nullptr, nullptr));
        Page::RemoveSearchEngine(name);
        if(had) Page::RegisterSearchEngine(name, before);
    }

    void theNamesTheUserKeepsAreOfferedAsCommands(){
        MainWindow window(974, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        typedef Receiver::SuggestRow Row;
        const auto find = [r](const QString &text, Row *found){
            for(const Row &row : r->m_Rows){
                if(row.source == Row::Commands && row.text == text){
                    if(found) *found = row;
                    return true;
                }
            }
            return false;
        };

        const QString engine = QStringLiteral("zqEngine");
        const QString bookmarklet = QStringLiteral("zqBookmarklet");
        Page::RegisterSearchEngine(engine, SearchEngine() << QStringLiteral("https://www.zqhost.example/?q=%1"));
        Page::RegisterBookmarklet(bookmarklet, Bookmarklet() << QStringLiteral("javascript:void(0)"));
        const QString longer = engine + QStringLiteral("2");
        Page::RegisterSearchEngine(longer, SearchEngine() << QStringLiteral("https://a.example/?q=%1"));
        Page::RegisterSearchEngine(QStringLiteral("reload"), SearchEngine() << QStringLiteral("https://a.example/?q=%1"));
        Page::RegisterSearchEngine(QStringLiteral("zq split"), SearchEngine() << QStringLiteral("https://a.example/?q=%1"));

        r->m_LineEdit->setText(engine);
        Row row;
        QVERIFY(find(engine + QLatin1Char(' '), &row));
        QCOMPARE(row.kind, Row::FillField);
        QVERIFY2(row.hint.contains(QStringLiteral("zqhost.example")), qPrintable(row.hint));
        QVERIFY(!row.hint.contains(QStringLiteral("www.")));
        QCOMPARE(r->m_CurrentSuggestIndex, -1);

        r->m_LineEdit->setText(QStringLiteral("zqeng"));
        QVERIFY(find(engine + QLatin1Char(' '), nullptr));
        r->m_LineEdit->setText(QStringLiteral("zqhost"));
        QVERIFY(find(engine + QLatin1Char(' '), &row));
        r->TakeRow(row);
        QCOMPARE(r->m_LineEdit->text(), engine + QLatin1Char(' '));

        r->m_LineEdit->setText(engine + QLatin1Char(' '));
        QVERIFY(find(engine + QLatin1Char(' '), &row));
        QCOMPARE(row.kind, Row::FillField);
        QVERIFY(row.hint.contains(QStringLiteral("zqhost.example")));
        QCOMPARE(r->m_CurrentSuggestIndex, -1);

        const QString line = engine + QStringLiteral(" foo bar");
        r->m_LineEdit->setText(line);
        QVERIFY(find(line, &row));
        QCOMPARE(row.kind, Row::RunCommand);
        QCOMPARE(row.title, engine + QLatin1Char(' '));
        QVERIFY(row.hint.contains(QStringLiteral("zqhost.example")));
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        int commands = 0;
        for(const Row &each : r->m_Rows) if(each.source == Row::Commands) commands++;
        QCOMPARE(commands, 1);

        r->m_LineEdit->setText(QStringLiteral("zqbook"));
        QVERIFY(find(bookmarklet, &row));
        QCOMPARE(row.kind, Row::RunCommand);
        QCOMPARE(row.title, bookmarklet);
        QVERIFY(!row.hint.isEmpty());

        r->m_LineEdit->setText(QStringLiteral("reload"));
        QVERIFY(!find(QStringLiteral("reload "), nullptr));
        r->m_LineEdit->setText(QStringLiteral("zq"));
        QVERIFY(!find(QStringLiteral("zq split "), nullptr));

        r->m_LineEdit->setText(QString());
        r->SelectTab(Row::Commands + 1);
        QVERIFY(find(engine + QLatin1Char(' '), nullptr));
        QVERIFY(find(bookmarklet, nullptr));
        r->SelectTab(0);

        Page::RemoveSearchEngine(engine);
        Page::RemoveSearchEngine(longer);
        Page::RemoveBookmarklet(bookmarklet);
        Page::RemoveSearchEngine(QStringLiteral("reload"));
        Page::RemoveSearchEngine(QStringLiteral("zq split"));
    }

    void aCommandStaysOfferedWhileItsArgumentIsTyped(){
        MainWindow window(975, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        QObject::disconnect(r, nullptr, window.GetTreeBank(), nullptr);
        QSignalSpy urls(r, QOverload<QList<QUrl>>::of(&Receiver::OpenUrl));
        typedef Receiver::SuggestRow Row;

        r->m_LineEdit->setText(QStringLiteral("Open "));
        QCOMPARE(r->m_Rows.first().source, Row::Commands);
        QCOMPARE(r->m_Rows.first().title, QStringLiteral("Open "));
        QCOMPARE(r->m_Rows.first().kind, Row::FillField);
        QCOMPARE(r->m_CurrentSuggestIndex, -1);

        r->TakeRow(r->m_Rows.first());
        QVERIFY(r->isVisible());
        QCOMPARE(r->m_LineEdit->text(), QStringLiteral("open "));

        const QString line = QStringLiteral("open https://example.invalid/");
        r->m_LineEdit->setText(line);
        QCOMPARE(r->m_Rows.first().title, QStringLiteral("Open "));
        QCOMPARE(r->m_Rows.first().text, line);
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        r->TakeRow(r->m_Rows.first());
        QVERIFY(!r->isVisible());
        QCOMPARE(urls.count(), 1);
    }

    void setAndSettingsAreOneWord(){
        MainWindow window(973, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        Settings &s = Application::GlobalSettings();
        const QString key = QStringLiteral("webview/bookmarklet/tst_receiver");

        s.setValue(key, QStringLiteral("0"));
        foreach(const QString &word, QStringList() << QStringLiteral("set") << QStringLiteral("settings")){
            r->OpenCommand();
            r->m_LineEdit->setText(word + QStringLiteral(" webview/bookmarklet/tst_"));
            QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Settings);
            QCOMPARE(r->m_Rows.first().title, key);
            QCOMPARE(r->m_Rows.first().text, QStringLiteral("set %1 0").arg(key));
            QCOMPARE(r->m_CurrentSuggestIndex, 0);
        }

        Receiver::m_History.clear();
        foreach(const QString &word, QStringList() << QStringLiteral("set") << QStringLiteral("settings")){
            r->OpenCommand();
            r->m_LineEdit->setText(word + QStringLiteral(" %1 a b").arg(key));
            QCOMPARE(r->m_Rows.length(), 1);
            QCOMPARE(r->m_Rows.first().text, QStringLiteral("set %1 a b").arg(key));
            QCOMPARE(r->m_CurrentSuggestIndex, 0);
            QVERIFY(r->ShowsSourceBar());
            r->SuitableAction();
            QVERIFY(!r->isVisible());
            QCOMPARE(s.value(key).toString(), QStringLiteral("a b"));
            s.setValue(key, QStringLiteral("0"));
        }
        QCOMPARE(Receiver::m_History, QStringList() << QStringLiteral("Set"));
        Receiver::m_History.clear();
        s.remove(key);

        s.setValue(key, QStringLiteral("0"));
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("set webview/bookmarklet/tst_"));
        r->SuitableAction();
        QCOMPARE(r->m_LineEdit->text(), QStringLiteral("set %1 0").arg(key));
        QVERIFY(r->isVisible());
        s.remove(key);

        QCOMPARE(CommandMap::Resolve(QStringLiteral("settings")), CommandMap::Signal);
    }

    void unsetIsSetsOtherHalf(){
        MainWindow window(992, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        Settings &s = Application::GlobalSettings();
        const QString scheme = QStringLiteral("application/@ColorScheme");
        const QString panels = QStringLiteral("application/@EnableTranslucentPanels");
        const QVariant had = s.value(scheme);
        const bool hadPanels = s.contains(panels);
        const QVariant panelsWere = s.value(panels);
        auto back = qScopeGuard([&](){
            if(had.isValid()) s.setValue(scheme, had); else s.remove(scheme);
            if(hadPanels) s.setValue(panels, panelsWere); else s.remove(panels);
            Receiver::m_History.clear();
        });
        s.setValue(scheme, QStringLiteral("Dark"));
        s.remove(panels);

        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("unset "));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Settings);
        QStringList keys;
        for(const Receiver::SuggestRow &row : r->m_Rows){
            if(row.source != Receiver::SuggestRow::Settings) continue;
            QVERIFY(row.kind != Receiver::SuggestRow::EditBinding);
            QVERIFY(s.contains(row.key));
            QCOMPARE(row.text, QStringLiteral("unset %1").arg(row.key));
            keys << row.key;
        }
        QVERIFY(keys.contains(scheme));
        QVERIFY(!keys.contains(panels));
        r->m_LineEdit->setText(QStringLiteral("set "));
        QCOMPARE(r->m_Rows.first().text.left(4), QStringLiteral("set "));

        r->m_LineEdit->setText(QStringLiteral("unset colorscheme"));
        QCOMPARE(r->m_Rows.first().key, scheme);
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->SuitableAction();
        QCOMPARE(r->m_LineEdit->text(), QStringLiteral("unset %1").arg(scheme));
        QVERIFY(r->m_Rows.isEmpty());
        r->SuitableAction();
        QVERIFY(!r->isVisible());
        QVERIFY(!s.contains(scheme));
        QCOMPARE(Receiver::m_History, QStringList() << QStringLiteral("Unset"));

        s.setValue(scheme, QStringLiteral("Dark"));
        r->OpenCommand();
        r->SelectTab(Receiver::SuggestRow::Commands + 1);
        r->m_LineEdit->setText(QStringLiteral("unset colorscheme"));
        QVERIFY(!r->m_Rows.isEmpty());
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("unset %1").arg(scheme));
        r->m_LineEdit->setText(QStringLiteral("set colorscheme"));
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("set %1 Dark").arg(scheme));
        r->m_LineEdit->setText(QStringLiteral("set %1 Light").arg(scheme));
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("set %1 Light").arg(scheme));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->m_LineEdit->setText(QStringLiteral("unset"));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Commands);
        r->SelectTab(0);
    }

    void aWordBringsItsSectionFirstAndLeavesTheRest(){
        MainWindow window(974, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("com re"));
        QCOMPARE(r->m_ActiveTab, 0);
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Commands);
        QVERIFY(r->m_SuggestStrings.contains(QStringLiteral("Reload")));
        QVERIFY(r->HasHeading(0));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        QCOMPARE(r->m_Rows.last().kind, Receiver::SuggestRow::SearchWeb);
        QCOMPARE(r->m_Rows.last().text, QStringLiteral("com re"));

        r->m_LineEdit->setText(QStringLiteral("web foo"));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Web);
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("foo"));

        r->m_LineEdit->setText(QStringLiteral("tabs zzzzqqqq"));
        QCOMPARE(r->m_Rows.first().kind, Receiver::SuggestRow::SearchWeb);
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("tabs zzzzqqqq"));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        r->m_LineEdit->setText(QStringLiteral("set"));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Settings);
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
    }

    void aTabChosenHasNoHeadingAndItsWordsAreOnlyWords(){
        MainWindow window(975, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->SelectTab(Receiver::SuggestRow::Commands + 1);
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Commands);
        QVERIFY(!r->HasHeading(0));
        QCOMPARE(r->SuggestRect(0).top(), r->SourceBarRect(0).bottom() + 1);
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        r->m_LineEdit->setText(QStringLiteral("tab"));
        QCOMPARE(r->m_ActiveTab, Receiver::SuggestRow::Commands + 1);
        r->m_LineEdit->setText(QStringLiteral("set x"));
        QCOMPARE(r->m_ActiveTab, Receiver::SuggestRow::Commands + 1);

        r->m_LineEdit->setText(QStringLiteral("search abc"));
        QCOMPARE(r->CurrentPlacement(), Receiver::TopRight);
        QCOMPARE(r->m_ActiveTab, Receiver::SuggestRow::Commands + 1);

        QSignalSpy urls(r, QOverload<QList<QUrl>>::of(&Receiver::OpenUrl));
        r->m_LineEdit->setText(QStringLiteral("zzzzqqqq"));
        QVERIFY(r->m_Rows.isEmpty());
        r->SuitableAction();
        QVERIFY(r->isVisible());
        QCOMPARE(urls.count(), 0);

        r->OpenCommand();
        QCOMPARE(r->m_ActiveTab, 0);
        QVERIFY(r->HasHeading(0));
    }

    void settingsAreFoundByEveryWord(){
        MainWindow window(976, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        QVERIFY(!r->SettingRows(QStringLiteral("receiver purge")).isEmpty());
        QVERIFY(!r->SettingRows(QStringLiteral("purge receiver")).isEmpty());
        QCOMPARE(r->SettingRows(QStringLiteral("purge receiver")).first().text.split(QLatin1Char(' ')).at(1),
                 QStringLiteral("application/@PurgeReceiver"));
        QVERIFY(r->SettingRows(QStringLiteral("receiver zzzzqqqq")).isEmpty());

        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("set purge receiver"));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Settings);
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("purge receiver"));
        bool found = false;
        for(const Receiver::SuggestRow &row : r->m_Rows) found = found || row.source == Receiver::SuggestRow::Settings;
        QVERIFY(found);
    }

    void theBarLeavesTheLineAlone(){
        MainWindow window(970, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("foo"));
        QVERIFY(r->ShowsSourceBar());
        const int tabs = Receiver::SuggestRow::Tabs + 1;
        QCOMPARE(r->SourceBarAt(r->SourceBarRect(tabs).center()), tabs);
        QVERIFY(r->SuggestRect(0).top() >= r->SourceBarRect(0).bottom());

        r->SelectTab(tabs);
        QCOMPARE(r->m_ActiveTab, tabs);
        QCOMPARE(r->m_LineEdit->text(), QStringLiteral("foo"));
        r->SelectTab(0);
        QCOMPARE(r->m_LineEdit->text(), QStringLiteral("foo"));
        QCOMPARE(r->m_ActiveTab, 0);
    }

    void theCardsKeysAreInTheTable(){
        QVERIFY(InputMapSchema::TypesIntoTheField(QKeySequence(QStringLiteral("A"))));
        QVERIFY(InputMapSchema::TypesIntoTheField(QKeySequence(QStringLiteral("Shift+A"))));
        QVERIFY(!InputMapSchema::TypesIntoTheField(QKeySequence(QStringLiteral("Ctrl+A"))));
        QVERIFY(!InputMapSchema::TypesIntoTheField(QKeySequence(QStringLiteral("Tab"))));
        QVERIFY(!InputMapSchema::TypesIntoTheField(QKeySequence(QStringLiteral("F2"))));
        const InputMapSchema::Table *table = InputMapSchema::Find(QStringLiteral("receiver/keymap"));
        QVERIFY(table);
        SettingsIO::Map written;
        const auto entry = [](const QString &input){
            QJsonObject object;
            object[QStringLiteral("input")] = input;
            object[QStringLiteral("action")] = QStringLiteral("NextCommandLineRow");
            return QJsonArray{ object };
        };
        QVERIFY(!InputMapSchema::Write(written, *table, entry(QStringLiteral("A"))).isEmpty());
        QVERIFY(InputMapSchema::Write(written, *table, entry(QStringLiteral("Ctrl+N"))).isEmpty());

        MainWindow window(944, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        Settings &s = Application::GlobalSettings();
        const auto press = [r](int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier){
            QKeyEvent ev(QEvent::KeyPress, key, modifiers);
            return r->HandleMappedKey(&ev);
        };

        r->OpenCommand();
        r->m_LineEdit->setFocus();
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        QVERIFY(press(Qt::Key_Down));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        QVERIFY(press(Qt::Key_Down));
        QCOMPARE(r->m_CurrentSuggestIndex, 1);
        QVERIFY(press(Qt::Key_Up));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        QVERIFY(!press(Qt::Key_PageDown));
        r->SelectTab(Receiver::SuggestRow::Commands + 1);
        r->m_CurrentSuggestIndex = 0;
        QVERIFY(press(Qt::Key_PageDown));
        QVERIFY(r->m_CurrentSuggestIndex > 1);
        const int down = r->m_CurrentSuggestIndex;
        QVERIFY(press(Qt::Key_PageUp));
        QVERIFY(r->m_CurrentSuggestIndex < down);
        QVERIFY(press(Qt::Key_Escape));
        QVERIFY(!r->isVisible());

        r->OpenQueryEditor();
        QVERIFY(!press(Qt::Key_Tab));
        QVERIFY(press(Qt::Key_Escape));
        QVERIFY(!r->isVisible());

        s.setValue(QStringLiteral("receiver/keymap/Ctrl+N"), QStringLiteral("NextCommandLineRow"));
        s.setValue(QStringLiteral("receiver/keymap/Ctrl+G"), QStringLiteral("CloseCommandLine"));
        s.setValue(QStringLiteral("receiver/keymap/A"), QStringLiteral("CloseCommandLine"));
        auto back = qScopeGuard([&s](){
            s.remove(QStringLiteral("receiver/keymap/Ctrl+N"));
            s.remove(QStringLiteral("receiver/keymap/Ctrl+G"));
            s.remove(QStringLiteral("receiver/keymap/A"));
        });
        r->OpenQueryEditor();
        QVERIFY(press(Qt::Key_G, Qt::ControlModifier));
        QVERIFY(!r->isVisible());
        r->OpenCommand();
        QVERIFY(!press(Qt::Key_A));
        QVERIFY(r->isVisible());
        QVERIFY(press(Qt::Key_N, Qt::ControlModifier));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        QVERIFY(!press(Qt::Key_Down));
        QVERIFY(press(Qt::Key_G, Qt::ControlModifier));
        QVERIFY(!r->isVisible());
    }

    void theReleaseOfASpentRightButtonOpensNoRowMenu(){
        MainWindow window(943, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        const QPoint at = r->SuggestRect(0).center();
        bool opened = false;
        const auto click = [&](){
            opened = false;
            QTimer::singleShot(0, [&opened](){
                if(QWidget *popup = QApplication::activePopupWidget()){
                    opened = true;
                    popup->close();
                }
            });
            QContextMenuEvent ev(QContextMenuEvent::Mouse, at, r->mapToGlobal(at));
            QCoreApplication::sendEvent(r, &ev);
            QCoreApplication::processEvents();
        };
        View::TakeRightButtonConsumed();

        View::ConsumeRightButton();
        click();
        QVERIFY(!opened);
        QVERIFY(!View::TakeRightButtonConsumed());
        click();
        QVERIFY(opened);

        View::ConsumeRightButton();
        QTest::mousePress(r, Qt::RightButton, Qt::NoModifier, at);
        QTest::mouseRelease(r, Qt::RightButton, Qt::NoModifier, at);
        click();
        QVERIFY(opened);
    }

    void aRowHasItsOperations(){
        using Row = Receiver::SuggestRow;
        using Action = Receiver::RowAction;
        const auto of = [](Row::Source source, Row::Kind kind, const QString &text){
            return Receiver::RowActions(Row{ source, kind, QString(), QString(), text, 0 });
        };
        const auto list = [](std::initializer_list<Action> actions){ return QVector<Action>(actions); };
        QCOMPARE(of(Row::Tabs, Row::OpenTab, QString()), list({ Action::Take, Action::CloseTab, Action::CopyAddress, Action::OpenCard }));
        QCOMPARE(of(Row::Trash, Row::OpenTab, QString()), list({ Action::Take, Action::DiscardTab, Action::CopyAddress, Action::OpenCard }));
        QCOMPARE(of(Row::Commands, Row::RunCommand, QStringLiteral("Reload")), list({ Action::Take, Action::ChangeAssignment }));
        QCOMPARE(of(Row::Commands, Row::FillField, QStringLiteral("open ")), list({ Action::Take }));
        const auto ofSetting = [](const QString &key){
            return Receiver::RowActions(Row{ Row::Settings, Row::FillField, QString(), QString(),
                                             QStringLiteral("set ") + key, 0, QIcon(), QString(), key });
        };
        QCOMPARE(ofSetting(QStringLiteral("application/@ColorScheme")),
                 list({ Action::Take, Action::OpenInSettings, Action::ResetSetting }));
        QCOMPARE(ofSetting(QStringLiteral("zq/unlisted")), list({ Action::Take, Action::ResetSetting }));
        QCOMPARE(of(Row::Settings, Row::EditBinding, QStringLiteral("ctrl+t")), list({ Action::Take }));
        QCOMPARE(of(Row::Web, Row::SearchWeb, QStringLiteral("a")), list({ Action::Take, Action::CopyWords }));
        QCOMPARE(of(Row::Web, Row::FillField, QStringLiteral("query ")), list({ Action::Take }));
        for(Action action : of(Row::Tabs, Row::OpenTab, QString()))
            QVERIFY(!Receiver::RowActionLabel(Row{ Row::Tabs, Row::OpenTab }, action).isEmpty());
    }

    void tabOpensTheOperationsOfTheRowChosen(){
        MainWindow window(948, QPoint(100, 100));
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setFocus();
        QStringList seen;
        const auto pressTab = [&](){
            seen.clear();
            QTimer::singleShot(0, [&seen](){
                if(QWidget *popup = QApplication::activePopupWidget()){
                    for(QAction *action : popup->actions()) seen << action->text();
                    popup->close();
                }
            });
            QTest::keyClick(r->m_LineEdit, Qt::Key_Tab);
        };
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        pressTab();
        QVERIFY(!seen.isEmpty());
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        const int settings = [r](){
            for(int i = 0; i < r->m_Rows.length(); i++)
                if(r->m_Rows[i].source == Receiver::SuggestRow::Settings) return i;
            return -1;
        }();
        QVERIFY(settings > 0);
        const QStringList top = seen;
        r->m_CurrentSuggestIndex = settings;
        pressTab();
        QVERIFY(!seen.isEmpty());
        QVERIFY(seen != top);
        QCOMPARE(r->m_CurrentSuggestIndex, settings);
        QVERIFY(r->isVisible());

        r->OpenQueryEditor();
        QVERIFY(!r->HandleTabKey());
    }

    void resettingASettingTakesTheWholeKey(){
        MainWindow window(946, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        Settings &s = Application::GlobalSettings();
        s.setValue(QStringLiteral("webview/bookmarklet/zqreset foo bar"), 1);
        s.setValue(QStringLiteral("webview/bookmarklet/zqreset foo"), 2);
        auto gone = qScopeGuard([&s](){ s.remove(QStringLiteral("webview/bookmarklet/zqreset foo")); });
        Receiver::SuggestRow row;
        for(const Receiver::SuggestRow &one : r->SettingRows(QStringLiteral("webview/bookmarklet/zqreset ")))
            if(one.key == QStringLiteral("webview/bookmarklet/zqreset foo bar")) row = one;
        QCOMPARE(row.key, QStringLiteral("webview/bookmarklet/zqreset foo bar"));
        r->RunRowAction(row, Receiver::RowAction::ResetSetting);
        QVERIFY(!s.contains(QStringLiteral("webview/bookmarklet/zqreset foo bar")));
        QVERIFY(s.contains(QStringLiteral("webview/bookmarklet/zqreset foo")));
    }

    void theSettingsPageIsOpenedOnATurnOfItsOwn(){
        MainWindow window(945, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        const int tabs = TreeBank::GetViewRoot()->GetChildren().length();
        using Row = Receiver::SuggestRow;
        r->RunRowAction(Row{ Row::Commands, Row::RunCommand, QString(), QString(), QStringLiteral("Reload"), 0 },
                        Receiver::RowAction::ChangeAssignment);
        QVERIFY(!r->isVisible());
        QCOMPARE(TreeBank::GetViewRoot()->GetChildren().length(), tabs);
        QTest::qWait(20);
        QCOMPARE(TreeBank::GetViewRoot()->GetChildren().length(), tabs);
    }

    void closingARowLeavesTheCardAndOnlyWhatIsOffToTheSide(){
        Application::GlobalSettings().setValue(QStringLiteral("application/@EnableAutoLoad"), false);
        Application::LoadGlobalSettings();
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        std::unique_ptr<MainWindow> window(Application::NewWindow(947));
        Application::SetCurrentWindow(window.get());
        auto forget = qScopeGuard([](){
            Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
            Application::RemoveWindow(947);
        });
        window->show();
        window->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(window.get()));
        TreeBank *bank = window->GetTreeBank();
        Receiver *r = bank->GetReceiver();
        ViewNode *parent = bank->GetCurrentViewNode() ? bank->GetCurrentViewNode()->GetParent()->ToViewNode()
                                                      : TreeBank::GetViewRoot();
        ViewNode *tab = parent->MakeChild();
        tab->SetHoldView(true);
        tab->SetTitle(QStringLiteral("zqOffToTheSide"));
        tab->SetUrl(QUrl(QStringLiteral("https://example.invalid/side")));
        const quint64 serial = tab->GetSerial();
        using Row = Receiver::SuggestRow;

        r->OpenCommand();
        r->m_LineEdit->setFocus();
        r->m_LineEdit->setText(QStringLiteral("zqOffToTheSide"));
        Row row;
        for(const Row &one : r->m_Rows) if(one.source == Row::Tabs) row = one;
        QCOMPARE(row.serial, serial);

        r->RunRowAction(Row{ Row::Web, Row::SearchWeb, QString(), QString(), QStringLiteral("zqwords"), 0 },
                        Receiver::RowAction::CopyWords);
        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("zqwords"));
        QVERIFY(!r->isVisible());

        r->OpenCommand();
        r->RunRowAction(row, Receiver::RowAction::CopyAddress);
        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("https://example.invalid/side"));

        r->OpenCommand();
        r->m_LineEdit->setFocus();
        r->m_LineEdit->setText(QStringLiteral("zqOffToTheSide"));
        r->RunRowAction(row, Receiver::RowAction::CloseTab);
        const auto closed = [serial](){
            ViewNode *now = VanillaPage::TabOfSerial(serial);
            return !now || TreeBank::IsTrash(now);
        };
        QTRY_VERIFY(closed());
        QVERIFY(r->isVisible());
        for(const Row &one : r->m_Rows) QVERIFY(one.source != Row::Tabs);

        r->RunRowAction(row, Receiver::RowAction::CloseTab);
        r->RunRowAction(Row{ Row::Tabs, Row::OpenTab, QString(), QString(), QString(), quint64(-1) },
                        Receiver::RowAction::CloseTab);
        QTest::qWait(50);
        QVERIFY(closed());
    }

    void theTrashIsASourceAndATabOfItIsFoundBySerial(){
        MainWindow window(972, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        QVERIFY(TreeBank::GetTrashRoot());
        ViewNode *closed = TreeBank::GetTrashRoot()->MakeChild();
        closed->SetHoldView(true);
        closed->SetTitle(QStringLiteral("zqTrashedTab"));
        closed->SetUrl(QUrl(QStringLiteral("https://example.invalid/zq")));

        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("zqTrashedTab"));
        QVERIFY(!r->m_Rows.isEmpty());
        bool found = false;
        for(const Receiver::SuggestRow &row : r->m_Rows){
            if(row.source != Receiver::SuggestRow::Trash) continue;
            found = true;
            QCOMPARE(row.kind, Receiver::SuggestRow::OpenTab);
            QCOMPARE(row.title, QStringLiteral("zqTrashedTab"));
            QCOMPARE(VanillaPage::TabOfSerial(row.serial), closed);
        }
        QVERIFY(found);

        r->m_LineEdit->setText(QStringLiteral("trash zqTrashedTab"));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Trash);
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
    }

    void theCommandsRunLastComeFirst(){
        Receiver::m_History.clear();
        Receiver::Remember(QStringLiteral("reload"));
        Receiver::Remember(QStringLiteral("set a b"));
        Receiver::Remember(QStringLiteral("reload"));
        Receiver::Remember(QStringLiteral("not a command at all"));
        QCOMPARE(Receiver::m_History, QStringList() << QStringLiteral("Reload") << QStringLiteral("Set"));
        Receiver::m_History.clear();

        MainWindow window(971, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        QSignalSpy keys(r, &Receiver::KeyEvent);
        r->OpenCommand();
        const auto firstOf = [r](const QString &typed){
            r->m_LineEdit->setText(QString());
            r->m_LineEdit->setText(typed);
            return r->m_Rows.first().text;
        };
        const QString untouched = firstOf(QStringLiteral("zoom"));
        const QString other = untouched == QStringLiteral("ZoomIn") ? QStringLiteral("ZoomOut") : QStringLiteral("ZoomIn");

        Receiver::Remember(other);
        QCOMPARE(firstOf(QStringLiteral("zoom")), other);
        r->OpenCommand();
        QCOMPARE(r->m_Rows.first().text, other);
        r->m_LineEdit->setText(other);
        int commands = 0;
        for(const Receiver::SuggestRow &row : r->m_Rows)
            if(row.source == Receiver::SuggestRow::Commands && row.text == other) commands++;
        QCOMPARE(commands, 1);

        const QList<InputMapSchema::Binding> real = r->m_Bindings;
        r->m_Bindings << InputMapSchema::Binding{ QStringLiteral("Zoom"), QStringLiteral("Zoom"), QStringLiteral("Reload"),
                                                  QString(), QString(), InputMapSchema::PageActions };
        QCOMPARE(firstOf(QStringLiteral("zoom")), QStringLiteral("Reload"));
        r->m_Bindings = real;

        const QStringList all = CommandMap::AllCommands();
        Receiver::Remember(all.last());
        QCOMPARE(r->CommandRows(QString(), 1).first().text, all.last());
        QCOMPARE(r->CommandRows(all.last().left(1), 1).first().text, all.last());

        Receiver::Remember(QStringLiteral("NextWindow"));
        QCOMPARE(r->CommandRows(QStringLiteral("next"), 1).first().text, QStringLiteral("NextView"));
        QCOMPARE(r->CommandRows(QStringLiteral("1st"), 1).first().text, QStringLiteral("FirstView"));
        QCOMPARE(r->CommandRows(QStringLiteral("2nd"), 1).first().text, QStringLiteral("SecondView"));
        r->SelectTab(Receiver::SuggestRow::Commands + 1);
        QCOMPARE(firstOf(QStringLiteral("1st")), QStringLiteral("FirstView"));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->SelectTab(0);

        Receiver::m_History.clear();
        r->m_LineEdit->setText(QStringLiteral("key a"));
        r->m_CurrentSuggestIndex = -1;
        r->SuitableAction();
        QCOMPARE(keys.count(), 1);
        QCOMPARE(Receiver::m_History, QStringList() << QStringLiteral("Key"));
        r->ReceiveCommand(QStringLiteral("reload"));
        QCOMPARE(Receiver::m_History, QStringList() << QStringLiteral("Key"));

        r->OpenCommand();
        r->TakeRow(Receiver::SuggestRow{ Receiver::SuggestRow::Commands, Receiver::SuggestRow::RunAction,
                                         QStringLiteral("zzzzqqqq"), QString(), QStringLiteral("zzzzqqqq"), 0 });
        QCOMPARE(Receiver::m_History.first(), QStringLiteral("zzzzqqqq"));

        Receiver::m_History.clear();
        for(int i = 0; i < RECEIVER_HISTORY_LENGTH + 5; i++)
            Receiver::RememberAction(QString::number(i));
        QCOMPARE(Receiver::m_History.length(), RECEIVER_HISTORY_LENGTH);
        Receiver::m_History.clear();
    }

    void theKeysGoRoundTheSources(){
        MainWindow window(964, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        const auto press = [r](int key, Qt::KeyboardModifiers modifiers){
            QKeyEvent ev(QEvent::KeyPress, key, modifiers);
            return r->HandleMappedKey(&ev);
        };

        const int count = Receiver::SuggestRow::SourceCount + 1;
        for(int tab = count - 1; tab >= 0; tab--){
            r->SelectTab(tab);
            const QString expected = tab == 0 ? Receiver::tr("All")
                : Receiver::SourceName(static_cast<Receiver::SuggestRow::Source>(tab - 1));
            QCOMPARE(r->m_LineEdit->placeholderText(), expected);
        }
        r->OpenCommand();
        QCOMPARE(r->m_LineEdit->placeholderText(), Receiver::tr("All"));
        QCOMPARE(r->m_ActiveTab, 0);
        QVERIFY(press(Qt::Key_Tab, Qt::ControlModifier));
        QCOMPARE(r->m_ActiveTab, 1);
        QVERIFY(press(Qt::Key_Tab, Qt::ControlModifier));
        QCOMPARE(r->m_ActiveTab, 2);

        QVERIFY(press(Qt::Key_Backtab, Qt::ControlModifier | Qt::ShiftModifier));
        QVERIFY(press(Qt::Key_Backtab, Qt::ControlModifier | Qt::ShiftModifier));
        QCOMPARE(r->m_ActiveTab, 0);
        QVERIFY(press(Qt::Key_Backtab, Qt::ControlModifier | Qt::ShiftModifier));
        QCOMPARE(r->m_ActiveTab, count - 1);
        QVERIFY(press(Qt::Key_Tab, Qt::ControlModifier));
        QCOMPARE(r->m_ActiveTab, 0);

        QVERIFY(!press(Qt::Key_A, Qt::NoModifier));

        Settings &s = Application::GlobalSettings();
        s.setValue(QStringLiteral("receiver/keymap/Ctrl+J"), QStringLiteral("NextCommandLineTab"));
        r->OpenCommand();
        QVERIFY(!press(Qt::Key_Tab, Qt::ControlModifier));
        QVERIFY(press(Qt::Key_J, Qt::ControlModifier));
        QCOMPARE(r->m_ActiveTab, 1);
        s.remove(QStringLiteral("receiver/keymap/Ctrl+J"));
        r->OpenCommand();

        for(const Receiver::SuggestRow &row : r->CommandRows(QStringLiteral("ctrl tab"), 100))
            QVERIFY(!row.text.contains(QStringLiteral("CommandLineTab")));
        bool offered = false;
        for(const Receiver::SuggestRow &row : r->SettingRows(QStringLiteral("ctrl tab")))
            offered = offered || row.kind == Receiver::SuggestRow::EditBinding;
        QVERIFY(offered);

        r->SelectTab(0);
        r->m_LineEdit->setText(QStringLiteral("search abc"));
        QVERIFY(!press(Qt::Key_Tab, Qt::ControlModifier));
    }

    void aTabGoneByTheTimeItIsTakenIsNothing(){
        MainWindow window(965, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->TakeRow(Receiver::SuggestRow{ Receiver::SuggestRow::Tabs, Receiver::SuggestRow::OpenTab,
                                         QStringLiteral("gone"), QString(), QString(), quint64(-1) });
        QCoreApplication::processEvents();
        QVERIFY(!r->isVisible());
    }

    void anotherModeHasNoSourcesLeftOver(){
        MainWindow window(966, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        QVERIFY(!r->m_Rows.isEmpty());
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::ControlModifier);
        foreach(const int mode, QList<int>() << 0 << 1 << 2){
            r->OpenCommand();
            if(mode == 0) r->OpenQueryEditor();
            if(mode == 1) r->OpenUrlEditor();
            if(mode == 2) r->OpenTextSeeker();
            QVERIFY(r->m_Rows.isEmpty());
            QVERIFY(r->m_SuggestStrings.isEmpty());
            QVERIFY(!r->HandleMappedKey(&key));
        }
    }

    void aSettingTakesOnlyWhatItCanBe(){
        MainWindow window(968, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        Settings &s = Application::GlobalSettings();
        const QString style = QStringLiteral("gadgets/@Style");
        const QString panels = QStringLiteral("application/@EnableTranslucentPanels");
        const QString saves = QStringLiteral("application/@AutoSaveInterval");
        const QStringList keys{ style, panels, saves, QStringLiteral("nothing/@At") };
        QMap<QString, QVariant> had;
        for(const QString &key : keys) if(s.contains(key)) had[key] = s.value(key);
        auto back = qScopeGuard([&](){
            for(const QString &key : keys){
                if(had.contains(key)) s.setValue(key, had[key]); else s.remove(key);
            }
            window.GetTreeBank()->Reconfigure();
            Receiver::m_History.clear();
        });
        s.remove(style);
        const auto dialogs = [](){
            QCoreApplication::processEvents();
            int count = 0;
            for(QWidget *widget : QApplication::allWidgets()){
                ModelessDialog *dialog = qobject_cast<ModelessDialog*>(widget);
                if(!dialog || !dialog->isVisible()) continue;
                dialog->Discard();
                count++;
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QCoreApplication::processEvents();
            return count;
        };

        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("set %1 ").arg(style));
        QCOMPARE(r->m_Rows.length(), 3);
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("set %1 PlainStyle").arg(style));
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        r->SelectTab(Receiver::SuggestRow::Commands + 1);
        r->m_LineEdit->setText(QStringLiteral("set %1 ").arg(style));
        QCOMPARE(r->m_Rows.length(), 3);
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        r->SelectTab(0);

        r->m_LineEdit->setText(QStringLiteral("set %1 Flat").arg(style));
        QCOMPARE(r->m_Rows.length(), 1);
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("set %1 FlatStyle").arg(style));
        QCOMPARE(r->m_Rows.first().kind, Receiver::SuggestRow::RunCommand);
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->SuitableAction();
        QVERIFY(!r->isVisible());
        QCOMPARE(s.value(style).toString(), QStringLiteral("FlatStyle"));

        const QString notAValue = Receiver::tr("Not a value it takes");
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("set %1 Square").arg(style));
        QVERIFY(r->m_Rows.isEmpty());
        QCOMPARE(r->m_LineEdit->Count(), notAValue);
        r->SuitableAction();
        QVERIFY(r->isVisible());
        QCOMPARE(s.value(style).toString(), QStringLiteral("FlatStyle"));
        QCOMPARE(dialogs(), 0);
        r->m_LineEdit->setText(QStringLiteral("set %1 Gl").arg(style));
        QCOMPARE(r->m_LineEdit->Count(), QString());
        r->SelectTab(Receiver::SuggestRow::Commands + 1);
        r->m_LineEdit->setText(QStringLiteral("set %1 Square").arg(style));
        QVERIFY(r->m_Rows.isEmpty());
        QCOMPARE(r->m_LineEdit->Count(), notAValue);
        r->SuitableAction();
        QVERIFY(r->isVisible());
        QCOMPARE(s.value(style).toString(), QStringLiteral("FlatStyle"));
        QCOMPARE(dialogs(), 0);
        r->SelectTab(0);

        r->m_LineEdit->setText(QStringLiteral("set %1 tr").arg(panels));
        QCOMPARE(r->m_Rows.length(), 1);
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("set %1 true").arg(panels));

        r->m_LineEdit->setText(QStringLiteral("set %1 60000").arg(saves));
        QCOMPARE(r->m_Rows.length(), 1);
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->m_LineEdit->setText(QStringLiteral("set %1 often").arg(saves));
        QVERIFY(r->m_Rows.isEmpty());
        QCOMPARE(r->m_LineEdit->Count(), notAValue);
        r->SuitableAction();
        QVERIFY(r->isVisible());
        QCOMPARE(s.value(saves), had.value(saves));
        QCOMPARE(dialogs(), 0);

        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("set nothing/@At 1"));
        QVERIFY(r->m_Rows.isEmpty());
        QCOMPARE(r->m_LineEdit->Count(), Receiver::tr("No such setting"));
        r->SuitableAction();
        QVERIFY(r->isVisible());
        QVERIFY(!s.contains(QStringLiteral("nothing/@At")));
        QCOMPARE(dialogs(), 0);
        r->hide();

        QVERIFY(Receiver::SettingRefusal(style, QStringLiteral("FlatStyle")).isEmpty());
        QVERIFY(Receiver::SettingRefusal(style, QStringLiteral("Square")).contains(QStringLiteral("GlassStyle")));
        QVERIFY(Receiver::SettingRefusal(QStringLiteral("nothing/@At"), QStringLiteral("1")).contains(QStringLiteral("nothing/@At")));

        r->ReceiveCommand(QStringLiteral("set %1 Flat").arg(style));
        QCOMPARE(s.value(style).toString(), QStringLiteral("FlatStyle"));
        r->ReceiveCommand(QStringLiteral("set nothing/@At 1"));
        QVERIFY(!s.contains(QStringLiteral("nothing/@At")));
        QCOMPARE(dialogs(), 2);
        r->ReceiveCommand(QStringLiteral("set %1 glassstyle").arg(style));
        QCOMPARE(s.value(style).toString(), QStringLiteral("GlassStyle"));
        r->ReceiveCommand(QStringLiteral("set %1 TRUE").arg(panels));
        QCOMPARE(s.value(panels), QVariant(true));
        QCOMPARE(dialogs(), 0);

        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
    }

    void aSettingOfferedKeepsItsValueAndAListIsNotOffered(){
        MainWindow window(967, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        const QString key = QStringLiteral("application/@ColorScheme");
        Application::GlobalSettings().remove(key);
        const Receiver::SuggestRows rows = r->SettingRows(QStringLiteral("application/@ColorScheme"));
        QCOMPARE(rows.length(), 1);
        QCOMPARE(rows.first().text, QStringLiteral("set %1 Auto").arg(key));

        QVERIFY(r->SettingRows(QStringLiteral("SpellCheckLanguages")).isEmpty());
    }

    void aCommandIsFoundByTwoWordsInTheField(){
        MainWindow window(977, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("toggle tree"));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Commands);
        QCOMPARE(r->m_SuggestStrings.first(), QStringLiteral("ToggleTreeBar"));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
    }

    void theChosenTabRowIsPreviewed(){
        MainWindow window(982, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        Receiver *r = window.GetTreeBank()->GetReceiver();
        NodePreview *preview = NodePreview::Instance();
        r->OpenCommand();
        preview->HoldUntilPointerMoves(QPoint(0, 0));

        typedef Receiver::SuggestRow Row;
        r->m_Rows = Receiver::SuggestRows()
            << Row{ Row::Commands, Row::RunCommand, QStringLiteral("a command"), QString(), QString(), 0, QIcon() }
            << Row{ Row::Tabs, Row::OpenTab, QStringLiteral("a tab"), QStringLiteral("example.com"), QString(), 0, QIcon() }
            << Row{ Row::Trash, Row::OpenTab, QStringLiteral("a closed tab"), QStringLiteral("example.org"), QString(), 0, QIcon() };
        r->m_SuggestStrings.clear();
        for(const Row &row : std::as_const(r->m_Rows)) r->m_SuggestStrings << row.title;
        r->m_CurrentSuggestIndex = -1;
        r->ResizeNotify(window.GetTreeBank()->size());
        QVERIFY(r->VisibleSuggestCount() >= 3);
        const auto rowRect = [r](int index){
            const QRect local = r->SuggestRect(index);
            return QRect(r->mapToGlobal(local.topLeft()), local.size());
        };

        r->SelectNextSuggest();
        QVERIFY(!preview->IsAbout(QStringLiteral("a command"), rowRect(0)));
        r->SelectNextSuggest();
        QVERIFY(preview->IsAbout(QStringLiteral("a tab"), rowRect(1)));
        r->SelectNextSuggest();
        QVERIFY(preview->IsAbout(QStringLiteral("a closed tab"), rowRect(2)));

        r->SelectNextSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        QVERIFY(!preview->IsAbout(QStringLiteral("a closed tab"), rowRect(2)));

        r->SelectPrevSuggest();
        QVERIFY(preview->IsAbout(QStringLiteral("a closed tab"), rowRect(2)));
        r->hide();
        QVERIFY(!preview->IsAbout(QStringLiteral("a closed tab"), rowRect(2)));
    }

    void aTabChosenScrollsWhenThereIsMoreThanFits(){
        MainWindow window(981, QPoint(100, 100));
        window.resize(1000, 500);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();

        r->SelectPrevSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, r->VisibleSuggestCount() - 1);
        QCOMPARE(r->m_SuggestScroll, 0);

        const int commands = Receiver::SuggestRow::Commands + 1;
        r->SelectTab(commands);
        const int total = r->m_SuggestStrings.length();
        const int shown = r->VisibleSuggestCount();
        QVERIFY2(total > shown, "the commands should be more than fit");
        const auto chosenIsShown = [r](){
            return r->m_CurrentSuggestIndex >= r->m_SuggestScroll &&
                r->m_CurrentSuggestIndex < r->m_SuggestScroll + r->VisibleSuggestCount() &&
                r->rect().contains(r->SuggestRect(r->m_CurrentSuggestIndex));
        };

        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        for(int i = 0; i < shown; i++) r->SelectNextSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, shown);
        QCOMPARE(r->m_SuggestScroll, 1);
        QVERIFY(chosenIsShown());
        QCOMPARE(r->SuggestAt(r->SuggestRect(shown).center()), shown);

        r->m_CurrentSuggestIndex = 0;
        r->m_SuggestScroll = 0;
        r->SelectPrevSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, total - 1);
        QVERIFY(chosenIsShown());
        r->SelectNextSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        QCOMPARE(r->m_SuggestScroll, 0);

        QKeyEvent down(QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier);
        QVERIFY(r->HandleMappedKey(&down));
        QCOMPARE(r->m_CurrentSuggestIndex, qMax(shown - 1, 1));
        QVERIFY(chosenIsShown());
        QKeyEvent up(QEvent::KeyPress, Qt::Key_PageUp, Qt::NoModifier);
        QVERIFY(r->HandleMappedKey(&up));
        QVERIFY(r->HandleMappedKey(&up));
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        const QPoint at = r->SuggestRect(0).center();
        QWheelEvent wheel(at, r->mapToGlobal(at), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
                          Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(r, &wheel);
        QCOMPARE(r->m_SuggestScroll, int(RECEIVER_WHEEL_ROWS));
        QVERIFY(chosenIsShown());

        for(int i = 0; i < total; i++) QCoreApplication::sendEvent(r, &wheel);
        QCOMPARE(r->m_SuggestScroll, total - r->VisibleSuggestCount());
        QVERIFY(chosenIsShown());

        QCOMPARE(r->m_Rows[r->m_CurrentSuggestIndex].title, r->m_SuggestStrings[r->m_CurrentSuggestIndex]);

        QVERIFY(r->m_SuggestScroll > 0);
        r->m_LineEdit->setText(QStringLiteral("toggle"));
        QCOMPARE(r->m_SuggestScroll, 0);
        QCOMPARE(r->m_CurrentSuggestIndex, 0);

        r->SelectTab(0);
        QCOMPARE(r->m_SuggestScroll, 0);
        QStringList many;
        for(int i = 0; i < 200; i++) many << QString::number(i);
        r->SetSuggest(many);
        QVERIFY(r->VisibleSuggestCount() < many.length());
        QCoreApplication::sendEvent(r, &wheel);
        QCOMPARE(r->m_SuggestScroll, int(RECEIVER_WHEEL_ROWS));
        r->SelectPrevSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, many.length() - 1);
        QVERIFY(chosenIsShown());
        const int strip = r->width() - ScaleByDevice(RECEIVER_PADDING) / 2;
        const int first = r->SuggestRect(r->m_SuggestScroll).top();
        r->m_SuggestScroll = 0;
        QTest::mousePress(r, Qt::LeftButton, Qt::NoModifier,
                          QPoint(strip, r->SuggestRect(r->VisibleSuggestCount() - 1).bottom()));
        QCOMPARE(r->m_SuggestScroll, r->LastSuggestScroll());
        QMouseEvent drag(QEvent::MouseMove, QPointF(strip, first), r->mapToGlobal(QPointF(strip, first)),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(r, &drag);
        QCOMPARE(r->m_SuggestScroll, 0);
        QTest::mouseRelease(r, Qt::LeftButton, Qt::NoModifier, QPoint(strip, first));
        QVERIFY(!r->m_DraggingScroll);

        r->m_LineEdit->setText(QString());
        QVERIFY(!r->m_Rows.isEmpty());
        QCoreApplication::sendEvent(r, &wheel);
        QCOMPARE(r->m_SuggestScroll, 0);
        QKeyEvent other(QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier);
        QVERIFY(!r->HandleMappedKey(&other));
    }

    void theSettingsTabWithNothingTypedIsDivided(){
        MainWindow window(985, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        const int settings = Receiver::SuggestRow::Settings + 1;
        r->SelectTab(settings);

        QVERIFY(r->HasHeading(0));
        QVERIFY(!r->m_Rows.first().section.isEmpty());
        QSet<QString> sections;
        int headings = 0;
        for(int i = 0; i < r->m_Rows.length(); i++){
            if(!r->HasHeading(i)) continue;
            headings++;
            QVERIFY2(!sections.contains(r->m_Rows[i].section), qPrintable(r->m_Rows[i].section));
            sections << r->m_Rows[i].section;
        }
        QVERIFY(headings > 5);
        QVERIFY(sections.contains(QCoreApplication::translate("InputMapSchema", "Keys on a page")));
        QVERIFY(sections.contains(QCoreApplication::translate("SettingsSchema", "Appearance")));

        for(const Receiver::SuggestRow &row : r->m_Rows){
            if(row.kind == Receiver::SuggestRow::EditBinding) QVERIFY(!row.hint.contains(row.section));
        }

        const int total = r->m_Rows.length();
        for(int i = 0; i < total; i++){
            r->SelectNextSuggest();
            const QRect row = r->SuggestRect(r->m_CurrentSuggestIndex);
            QVERIFY2(r->rect().contains(row), qPrintable(QString::number(r->m_CurrentSuggestIndex)));
            QVERIFY(r->SuggestAt(row.center()) == r->m_CurrentSuggestIndex);
        }
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->SelectPrevSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, total - 1);
        QCOMPARE(r->m_SuggestScroll, r->LastSuggestScroll());
        QCOMPARE(r->m_SuggestScroll + r->VisibleSuggestCount(), total);

        r->m_LineEdit->setText(QStringLiteral("f5"));
        QVERIFY(!r->m_Rows.isEmpty());
        for(int i = 0; i < r->m_Rows.length(); i++) QVERIFY(!r->HasHeading(i));
        QVERIFY(r->m_Rows.first().hint.contains(QCoreApplication::translate("InputMapSchema", "Keys on a page")));

        r->m_LineEdit->setText(QString());
        r->SelectTab(Receiver::SuggestRow::Commands + 1);
        QVERIFY(!r->HasHeading(0));
        r->SelectTab(0);
        for(const Receiver::SuggestRow &row : r->m_Rows) QVERIFY(row.section.isEmpty());
    }

    void whatRunsMayOpenTheCardAgain(){
        MainWindow window(986, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->TakeRow(Receiver::SuggestRow{ Receiver::SuggestRow::Commands, Receiver::SuggestRow::RunAction,
                                         QString(), QString(), QStringLiteral("OpenTextSeeker"), 0 });
        QVERIFY(r->isVisible());
        QCOMPARE(r->m_Mode, Receiver::Search);
        r->OnAborted();
        r->OpenCommand();
        r->TakeRow(Receiver::SuggestRow{ Receiver::SuggestRow::Commands, Receiver::SuggestRow::RunCommand,
                                         QString(), QString(), QStringLiteral("OpenCommand"), 0 });
        QVERIFY(r->isVisible());
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("opencommand"));
        r->m_CurrentSuggestIndex = -1;
        r->SuitableAction();
        QVERIFY(r->isVisible());
    }

    void onlyWhatCanRunHereIsOffered(){
        MainWindow window(987, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        QVERIFY(!window.GetTreeBank()->IsDisplayingTableView());
        const auto names = [r](const QString &typed){
            QStringList list;
            for(const Receiver::SuggestRow &row : r->CommandRows(typed, 10000)) list << row.text;
            return list;
        };
        QVERIFY(!names(QStringLiteral("MoveToLeftItem")).contains(QStringLiteral("MoveToLeftItem")));
        QVERIFY(!names(QString()).contains(QStringLiteral("MoveToLeftItem")));
        QVERIFY(names(QString()).contains(QStringLiteral("Reload")));
        QVERIFY(names(QString()).contains(QStringLiteral("NewViewNode")));

        const auto answers = [](const char *action, bool list, bool table){
            return Receiver::AnswersHere(QString::fromLatin1(action), list, table);
        };
        QVERIFY(!answers("Reload", true, true));
        QVERIFY(!answers("Back", true, true));
        QVERIFY(answers("NewViewNode", true, true));
        QVERIFY(answers("OpenSettings", true, true));
        QVERIFY(answers("MoveToLeftItem", true, true));
        QVERIFY(answers("query", true, true));
        QVERIFY(answers("Reload", false, true));
        QVERIFY(answers("MoveToLeftItem", false, true));
        QVERIFY(answers("Reload", false, false));
        QVERIFY(!answers("MoveToLeftItem", false, false));
        QVERIFY(answers("query", false, false));
    }

    void aPressTakesTheRowUnderItWithTheLeftButton(){
        MainWindow window(988, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("sear"));
        QCOMPARE(r->m_SuggestStrings.first(), QStringLiteral("Search "));
        QVERIFY(r->m_SuggestStrings.length() > 1);

        const QPoint first = r->SuggestRect(0).center();
        r->m_CurrentSuggestIndex = 1;
        QTest::mouseClick(r, Qt::RightButton, Qt::NoModifier, first);
        QCOMPARE(r->m_LineEdit->text(), QStringLiteral("sear"));
        QCOMPARE(r->m_CurrentSuggestIndex, 1);
        QTest::mouseClick(r, Qt::LeftButton, Qt::NoModifier, first);
        QCOMPARE(r->m_LineEdit->text(), QStringLiteral("search "));
    }

    void anotherWindowSizeSharesTheRowsAgain(){
        MainWindow window(989, QPoint(100, 100));
        window.resize(1000, 400);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        const int few = r->m_Rows.length();
        window.resize(1000, 900);
        QCoreApplication::processEvents();
        QVERIFY(r->m_Rows.length() > few);
        window.resize(1000, 400);
        QCoreApplication::processEvents();
        QCOMPARE(r->m_Rows.length(), few);
    }

    void withNoRoomForARowNoneCanBeChosen(){
        MainWindow window(990, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        TreeBank *tb = window.GetTreeBank();
        Receiver *r = tb->GetReceiver();
        r->OpenCommand();
        r->SelectTab(Receiver::SuggestRow::Commands + 1);
        QCOMPARE(r->m_CurrentSuggestIndex, 0);
        r->ResizeNotify(QSize(1000, 10));
        QCOMPARE(r->VisibleSuggestCount(), 0);
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        r->SelectNextSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        r->SelectPrevSuggest();
        QCOMPARE(r->m_CurrentSuggestIndex, -1);
        r->ResizeNotify(tb->size());
        QVERIFY(r->VisibleSuggestCount() > 0);
    }

    void aWindowMadeSmallerKeepsTheChosenRowShown(){
        MainWindow window(982, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        TreeBank *tb = window.GetTreeBank();
        Receiver *r = tb->GetReceiver();
        r->OpenCommand();
        r->SelectTab(Receiver::SuggestRow::Commands + 1);
        for(int i = 0; i < 5; i++) r->SelectNextSuggest();
        const int chosen = r->m_CurrentSuggestIndex;
        r->ResizeNotify(QSize(1000, 300));
        QCOMPARE(r->m_CurrentSuggestIndex, chosen);
        QVERIFY(r->VisibleSuggestCount() > 0);
        QVERIFY(chosen >= r->m_SuggestScroll);
        QVERIFY(chosen < r->m_SuggestScroll + r->VisibleSuggestCount());
    }

    void anInputTypedFindsTheCommandOnIt(){
        MainWindow window(978, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        foreach(const QString &typed, QStringList() << QStringLiteral("ctrl+t") << QStringLiteral("t ctrl")){
            r->m_LineEdit->setText(typed);
            QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Commands);
            QCOMPARE(r->m_Rows.first().text, QStringLiteral("NewViewNode"));
            QCOMPARE(r->m_CurrentSuggestIndex, 0);
        }
        r->m_LineEdit->setText(QStringLiteral("ctrl+t"));
        QVERIFY(!r->m_SuggestStrings.contains(QStringLiteral("NewNode")));

        r->m_LineEdit->setText(QStringLiteral("ctrl+f"));
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("OpenTextSeeker"));
        QCOMPARE(r->m_Rows.first().kind, Receiver::SuggestRow::RunCommand);

        Settings &s = Application::GlobalSettings();
        s.setValue(QStringLiteral("receiver/keymap/Ctrl+J"), QStringLiteral("NextCommandLineTab"));
        s.setValue(QStringLiteral("webview/keymap/Ctrl+J"), QStringLiteral("AddBookmarklet"));
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("ctrl+j"));
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("AddBookmarklet"));
        QCOMPARE(r->m_Rows.first().kind, Receiver::SuggestRow::RunAction);
        s.remove(QStringLiteral("receiver/keymap/Ctrl+J"));
        s.remove(QStringLiteral("webview/keymap/Ctrl+J"));
        r->OpenCommand();

        r->m_LineEdit->setText(QStringLiteral("ctrl+shift"));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Commands);

        const QList<InputMapSchema::Binding> real = r->m_Bindings;
        r->m_Bindings << InputMapSchema::Binding{ QStringLiteral("Re"), QStringLiteral("Re"), QStringLiteral("ZoomIn"),
                                                  QString(), QString(), InputMapSchema::PageActions };
        r->m_LineEdit->setText(QStringLiteral("re"));
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("ZoomIn"));
        r->m_Bindings = real;

        Receiver::m_History.clear();
        Receiver::Remember(QStringLiteral("zoomin"));
        r->m_LineEdit->setText(QStringLiteral("f5"));
        QCOMPARE(r->m_Rows.first().source, Receiver::SuggestRow::Commands);
        QCOMPARE(r->m_Rows.first().text, QStringLiteral("Reload"));
        Receiver::m_History.clear();

        r->m_LineEdit->setText(QStringLiteral("reload"));
        QCOMPARE(r->m_CurrentSuggestIndex, -1);

        r->m_LineEdit->setText(QStringLiteral("reload now"));
        QVERIFY(r->m_Rows.isEmpty());
        r->m_LineEdit->setText(QStringLiteral("open set"));
        for(const Receiver::SuggestRow &row : r->m_Rows){
            if(row.source == Receiver::SuggestRow::Commands) QCOMPARE(row.text, QStringLiteral("open set"));
        }
    }

    void aBindingIsChangedFromTheSettings(){
        MainWindow window(984, QPoint(100, 100));
        window.resize(1000, 700);
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        const QString page = QCoreApplication::translate("InputMapSchema", "Keys on a page");

        foreach(const QString &typed, QStringList() << QStringLiteral("Ctrl+T") << QStringLiteral("NewViewNode")
                                                    << page + QStringLiteral(" Ctrl+T")){
            const Receiver::SuggestRows rows = r->SettingRows(typed);
            QVERIFY2(!rows.isEmpty(), qPrintable(typed));
            bool found = false;
            for(const Receiver::SuggestRow &row : rows){
                if(row.kind != Receiver::SuggestRow::EditBinding) continue;
                QCOMPARE(row.source, Receiver::SuggestRow::Settings);
                if(row.text == QStringLiteral("Ctrl+T ") + page) found = true;
            }
            QVERIFY2(found, qPrintable(typed));
        }

        const QList<InputMapSchema::Binding> real = r->m_Bindings;
        r->m_Bindings << InputMapSchema::Binding{ QStringLiteral("Scheme"), QStringLiteral("Scheme"), QStringLiteral("ZoomIn"),
                                                  QString(), QString(), InputMapSchema::PageActions };
        QVERIFY(r->SettingRows(QStringLiteral("scheme")).length() >= 2);
        QCOMPARE(r->SettingRows(QStringLiteral("scheme")).first().kind, Receiver::SuggestRow::EditBinding);
        r->m_Bindings = real;

        const Receiver::SuggestRows all = r->SettingRows(QString());
        QCOMPARE(all.first().kind, Receiver::SuggestRow::FillField);
        QCOMPARE(all.last().kind, Receiver::SuggestRow::EditBinding);

        int tables = 0;
        for(const Receiver::SuggestRow &row : r->SettingRows(QStringLiteral("f5")))
            if(row.kind == Receiver::SuggestRow::EditBinding && row.title == QStringLiteral("F5")) tables++;
        QVERIFY(tables >= 2);
    }

    void aCommandSaysEverythingItIsOn(){
        MainWindow window(979, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        QVERIFY(r->InputsOf(QStringLiteral("NewViewNode")).contains(QStringLiteral("Ctrl+T")));
        const QStringList reload = r->InputsOf(QStringLiteral("Reload"));
        QVERIFY(reload.length() >= 2);
        QVERIFY(r->InputsOf(QStringLiteral("zzzzqqqq")).isEmpty());

        QVERIFY(!r->m_Bindings.isEmpty());
        bool gadgets = false;
        for(const InputMapSchema::Binding &binding : r->m_Bindings){
            if(binding.vocabulary == InputMapSchema::GadgetsActions) gadgets = true;
            else QVERIFY(!gadgets);
        }
        QVERIFY(gadgets);
        const QList<InputMapSchema::Binding> real = r->m_Bindings;
        for(int i = 0; i < 7; i++){
            r->m_Bindings << InputMapSchema::Binding{ QString::number(i), QString::number(i), QStringLiteral("zzzz"),
                                                      QString(), QString(), InputMapSchema::PageActions };
        }
        QCOMPARE(r->InputsOf(QStringLiteral("zzzz")).length(), 7);
        r->m_Bindings << InputMapSchema::Binding{ QStringLiteral(","), QStringLiteral(","), QStringLiteral("zzzy"),
                                                  QString(), QString(), InputMapSchema::PageActions }
                      << InputMapSchema::Binding{ QStringLiteral("Alt+Right"), QStringLiteral("Alt+Right"), QStringLiteral("zzzy"),
                                                  QString(), QString(), InputMapSchema::PageActions };
        QCOMPARE(r->InputsOf(QStringLiteral("zzzy")), QStringList({ QStringLiteral(","), QStringLiteral("Alt+Right") }));
        r->m_Bindings = real;

        r->m_LineEdit->setText(QStringLiteral("newviewnode"));
        bool hinted = false;
        for(const Receiver::SuggestRow &row : r->m_Rows)
            if(row.source == Receiver::SuggestRow::Commands && row.inputs.contains(QStringLiteral("Ctrl+T")) &&
               !row.hint.contains(QStringLiteral("Ctrl+T"))) hinted = true;
        QVERIFY(hinted);
    }

    void anInputTakenRunsItsAction(){
        MainWindow window(980, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->TakeRow(Receiver::SuggestRow{ Receiver::SuggestRow::Commands, Receiver::SuggestRow::RunAction,
                                         QStringLiteral("x"), QString(), QStringLiteral("zzzzqqqq"), 0 });
        QVERIFY(!r->isVisible());
    }

    void takingACommandWhichNeedsMoreLeavesItInTheField(){
        MainWindow window(960, QPoint(100, 100));
        window.show();
        Receiver *r = window.GetTreeBank()->GetReceiver();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("sear"));
        QCOMPARE(r->m_SuggestStrings.first(), QStringLiteral("Search "));
        r->SuitableAction();
        QVERIFY(r->isVisible());
        QCOMPARE(r->m_LineEdit->text(), QStringLiteral("search "));
        QCOMPARE(r->CurrentPlacement(), Receiver::TopRight);
        QVERIFY(r->m_SuggestStrings.isEmpty());
    }

    void takingABareCommandRunsItAndCloses(){
        MainWindow window(961, QPoint(100, 100));
        window.show();
        TreeBank *tb = window.GetTreeBank();
        Receiver *r = tb->GetReceiver();
        const bool had = tb->GetMiniMap();
        r->OpenCommand();
        r->m_LineEdit->setText(QStringLiteral("toggleminim"));
        QCOMPARE(r->m_SuggestStrings.first(), QStringLiteral("ToggleMiniMap"));
        r->SuitableAction();
        QVERIFY(!r->isVisible());
        QCOMPARE(static_cast<bool>(tb->GetMiniMap()), !had);
    }

    void serverIsReleasedWithTheLastReceiverAndMadeAgain(){
        QPointer<QLocalServer> first;
        {
            MainWindow window(931, QPoint(100, 100));
            first = Receiver::m_LocalServer;
            QVERIFY(first);
            QVERIFY(first->isListening());
        }
        QVERIFY(!first.isNull());
        QVERIFY(!first->isListening());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(first.isNull());
        QVERIFY(!Receiver::m_LocalServer);

        MainWindow again(932, QPoint(100, 100));
        const QPointer<QLocalServer> second = Receiver::m_LocalServer;
        QVERIFY(second);
        QVERIFY(second->isListening());
    }

    void aWindowMadeBeforeTheDeferredDeleteGetsItsOwnServer(){
        QPointer<QLocalServer> old;
        {
            MainWindow window(933, QPoint(100, 100));
            old = Receiver::m_LocalServer;
            QVERIFY(old);
        }
        MainWindow next(934, QPoint(100, 100));
        const QPointer<QLocalServer> fresh = Receiver::m_LocalServer;
        QVERIFY(fresh);
        QVERIFY(fresh != old);
        QVERIFY(!old.isNull() && !old->isListening());
        QVERIFY(fresh->isListening());
        QVERIFY(connectOnce(fresh));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(old.isNull());
        QVERIFY(!fresh.isNull() && fresh->isListening());
        QVERIFY(connectOnce(fresh));
    }

    void twoUnregisteredWindowsReleaseTheServerOnce(){
        auto first = std::make_unique<MainWindow>(935, QPoint(100, 100));
        auto second = std::make_unique<MainWindow>(936, QPoint(100, 100));
        const QPointer<QLocalServer> server = Receiver::m_LocalServer;
        QVERIFY(server);
        first.reset();
        QVERIFY(!Receiver::m_LocalServer);
        second.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(server.isNull());
        QVERIFY(!Receiver::m_LocalServer);
    }

    void anOlderReceiverDoesNotReleaseTheCurrentServer(){
        auto first = std::make_unique<MainWindow>(939, QPoint(100, 100));
        auto second = std::make_unique<MainWindow>(940, QPoint(100, 100));
        const QPointer<QLocalServer> old = Receiver::m_LocalServer;
        QVERIFY(old);
        first.reset();
        QVERIFY(!Receiver::m_LocalServer);
        MainWindow third(941, QPoint(100, 100));
        const QPointer<QLocalServer> fresh = Receiver::m_LocalServer;
        QVERIFY(fresh && fresh != old);
        second.reset();
        QCOMPARE(Receiver::m_LocalServer, fresh.data());
        QVERIFY(fresh->isListening());
        QVERIFY(connectOnce(fresh));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(old.isNull());
        QVERIFY(!fresh.isNull() && fresh->isListening());
    }

    void aReceiverDeletedByItsOwnCommandStopsTakingConnections(){
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        std::unique_ptr<MainWindow> window(Application::NewWindow(942));
        std::unique_ptr<MainWindow> other(Application::NewWindow(943));
        const auto unregister = qScopeGuard([&]{
            Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
            Application::RemoveWindow(942);
            Application::RemoveWindow(943);
        });
        Application::SetCurrentWindow(942);
        Receiver *receiver = window->GetTreeBank()->GetReceiver();
        QVERIFY(receiver);
        const QPointer<QLocalServer> server = Receiver::m_LocalServer;
        QVERIFY(server);
        QObject::disconnect(server, &QLocalServer::newConnection, nullptr, nullptr);
        QSignalSpy accepted(server, &QLocalServer::newConnection);
        const QPointer<Receiver> alive(receiver);
        connect(receiver, &Receiver::KeyEvent, receiver, [&](){
            Application::SetCurrentWindow(943);
            Application::RemoveWindow(942);
            window.reset();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        QLocalSocket first, second;
        for(QLocalSocket *client : {&first, &second}){
            QByteArray frame;
            QDataStream stream(&frame, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
            stream << QStringLiteral("key F5");
            client->connectToServer(Application::LocalServerName());
            QVERIFY(client->waitForConnected(2000));
            client->write(frame);
            client->waitForBytesWritten(2000);
        }
        QVERIFY(QTest::qWaitFor([&]{ return accepted.count() == 2; }, 2000));
        QTest::qWait(100);
        receiver->ForeignCommandReceived();
        QVERIFY(alive.isNull());
        QVERIFY(server && server->isListening());
        QVERIFY(server->hasPendingConnections());
        delete server->nextPendingConnection();
        QVERIFY(!server->hasPendingConnections());
    }

    void registeredWindowsKeepTheServerUntilTheLastOneGoes(){
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        std::unique_ptr<MainWindow> first(Application::NewWindow(937));
        std::unique_ptr<MainWindow> second(Application::NewWindow(938));
        const auto unregister = qScopeGuard([&]{
            Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
            Application::RemoveWindow(937);
            Application::RemoveWindow(938);
        });
        const QPointer<QLocalServer> server = Receiver::m_LocalServer;
        QVERIFY(server);
        QVERIFY(server->isListening());

        Application::RemoveWindow(937);
        first.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!server.isNull());
        QCOMPARE(Receiver::m_LocalServer, server.data());
        QVERIFY(server->isListening());
        Application::SetCurrentWindow(938);
        QVERIFY(commandReaches(second->GetTreeBank()->GetReceiver(), QStringLiteral("F5")));

        Application::RemoveWindow(938);
        second.reset();
        QVERIFY(!Receiver::m_LocalServer);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(server.isNull());
    }
};

void tst_receiver::onlyTheSameUsersServerIsTold(){
    QLocalServer server;
    const QString name = QStringLiteral("vanilla_tst_receiver_same_user_%1").arg(QCoreApplication::applicationPid());
    QLocalServer::removeServer(name);
    QVERIFY(server.listen(name));
    QLocalSocket client;
    QVERIFY(!Transmitter::ServerIsSameUser(&client));
    QVERIFY(!Transmitter::ServerIsSameUser(nullptr));
    client.connectToServer(name);
    QVERIFY(client.waitForConnected(2000));
    QVERIFY(Transmitter::ServerIsSameUser(&client));
    client.abort();
    QVERIFY(!Transmitter::ServerIsSameUser(&client));
}

int main(int argc, char **argv){
    Application app(argc, argv);
    tst_receiver test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_receiver.moc"
