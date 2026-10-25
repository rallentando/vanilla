#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QUrl>
#include <memory>

#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "lightnode.hpp"
#include "settingspage.hpp"
#include "page.hpp"
#include "testsupport.hpp"

class tst_startpage : public QObject {
    Q_OBJECT

private:
    static VanillaPageResponse Get(const QUrl &url, const QUrl &initiator){
        return VanillaPage::Answer(url, QByteArrayLiteral("GET"), QByteArray(), initiator);
    }
    static QUrl Endpoint(const QString &name, const QString &q = QString()){
        QUrl url(VanillaPage::StartUrl().toString() + QStringLiteral("api/") + name);
        if(!q.isEmpty())
            url.setQuery(QStringLiteral("q=") + QString::fromLatin1(QUrl::toPercentEncoding(q)));
        return url;
    }

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        TestSupport::DisableWidgetAnimation();
        TreeBank::Initialize();
    }

    void theFieldGoesToTheWebOrToThesePagesAndSearchesForTheRest();
    void theTabsOfferedAreTheMainTreesPagesByLastLook();
    void aStartPageIsLetGoOfOnlyAsALeafOfTheMainTree();
    void aCardsTabGoesToTheTrashOnlyFromThePage();
    void aTabOfTheTrashGoesBackWhereANewSiblingOfTheStartPageWould();
    void theFirstPageIsTheOpenTabsAloneHoweverFewThereAre();
    void aClickWaitsForTheTreeToBeChangeableAndForgetsATabGone();
    void theHistoryIsEveryTabByTheLastLookInOneOrder();
    void aRowsIconIsTheSitesCachedOneOrNothing();
    void aDiscardTakesTheCheckedTabsOutOfTheTrashForGood();
};

void tst_startpage::theFieldGoesToTheWebOrToThesePagesAndSearchesForTheRest(){
    Page::RegisterSearchEngine(QStringLiteral("bing"), SearchEngine()
                               << QStringLiteral("https://www.bing.com/search?q=%1")
                               << QStringLiteral("UTF-8") << QStringLiteral("true"));
    const auto forget = qScopeGuard([]{ Page::RemoveSearchEngine(QStringLiteral("bing")); });
    const QUrl start = VanillaPage::StartUrl();
    auto resolve = [&](const char *typed){
        return QUrl(QJsonDocument::fromJson(Get(Endpoint(QStringLiteral("resolve"),
                                                         QString::fromUtf8(typed)), start).m_Body)
                    .object().value(QStringLiteral("url")).toString());
    };
    const QString searched = QStringLiteral("https://www.bing.com/search?q=");

    QCOMPARE(resolve("https://example.com/path?x=1"), QUrl(QStringLiteral("https://example.com/path?x=1")));
    QCOMPARE(resolve("vanilla://settings/"), VanillaPage::SettingsUrl());
    QCOMPARE(resolve("about:blank"), BLANK_URL);
    QVERIFY(resolve("javascript:alert(1)").toString().startsWith(searched));
    QVERIFY(resolve("file:///C:/x.txt").toString().startsWith(searched));
    QVERIFY(resolve("two words").toString().startsWith(searched + QStringLiteral("two")));
    QVERIFY(resolve("").isEmpty());
    QCOMPARE(Get(Endpoint(QStringLiteral("resolve"), QStringLiteral("x")),
                 QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
}

void tst_startpage::theTabsOfferedAreTheMainTreesPagesByLastLook(){
    ViewNode *root = TreeBank::GetViewRoot();
    const QDateTime now = QDateTime::currentDateTime();
    auto make = [&](const QString &title, const QUrl &url, int minutesAgo){
        ViewNode *vn = root->MakeChild();
        vn->SetHoldView(true);
        vn->SetTitle(title);
        vn->SetUrl(url);
        vn->SetLastAccessDate(now.addSecs(-60 * minutesAgo));
        return vn;
    };
    ViewNode *latest = make(QStringLiteral("Qt docs"), QUrl(QStringLiteral("https://doc.qt.io/qt-6/qurl.html")), 1);
    ViewNode *folder = root->MakeChild();
    folder->SetTitle(QStringLiteral("a folder;edge"));
    ViewNode *inner = folder->MakeChild();
    inner->SetTitle(QStringLiteral("inner"));
    ViewNode *filed = inner->MakeChild();
    filed->SetHoldView(true);
    filed->SetTitle(QStringLiteral("filed"));
    filed->SetUrl(QUrl(QStringLiteral("https://example.org/filed")));
    filed->SetLastAccessDate(now.addSecs(-90));
    make(QString::fromUtf8("日本語 ページ"), QUrl(QStringLiteral("https://ja.wikipedia.org/wiki/%E6%97%A5%E6%9C%AC")), 2);
    make(QStringLiteral("start"), VanillaPage::StartUrl(), 0);
    make(QStringLiteral("blank"), BLANK_URL, 0);
    make(QStringLiteral("settings"), VanillaPage::SettingsUrl(), 0);
    for(int i = 0; i < 13; i++)
        make(QStringLiteral("older %1").arg(i), QUrl(QStringLiteral("https://example.com/%1").arg(i)), 10 + i);
    auto closed = [&](const QString &title, int secondsAgo){
        ViewNode *vn = TreeBank::GetTrashRoot()->MakeChild();
        vn->SetHoldView(true);
        vn->SetTitle(title);
        vn->SetUrl(QUrl(QStringLiteral("https://closed.example/") + title));
        vn->SetLastAccessDate(now.addSecs(-secondsAgo));
        return vn;
    };
    ViewNode *closedLately = closed(QStringLiteral("closed lately"), 0);
    ViewNode *closedLongAgo = closed(QStringLiteral("closed long ago"), 60 * 15 + 30);
    const auto clear = qScopeGuard([&]{ TreeBank::Initialize(); });

    const QUrl start = VanillaPage::StartUrl();
    auto tabs = [&](const char *q){
        return QJsonDocument::fromJson(Get(Endpoint(QStringLiteral("tabs"), QString::fromUtf8(q)), start).m_Body)
            .object().value(QStringLiteral("tabs")).toArray();
    };

    const QJsonArray recent = tabs("");
    QCOMPARE(recent.size(), 12);
    QCOMPARE(recent.at(0).toObject().value(QStringLiteral("node")).toString(),
             QString::number(latest->GetSerial()));
    QCOMPARE(recent.at(0).toObject().value(QStringLiteral("host")).toString(), QStringLiteral("doc.qt.io"));
    QCOMPARE(recent.at(0).toObject().value(QStringLiteral("address")).toString(),
             QStringLiteral("https://doc.qt.io/qt-6/qurl.html"));
    QCOMPARE(recent.at(0).toObject().value(QStringLiteral("path")).toArray().size(), 0);
    QCOMPARE(recent.at(1).toObject().value(QStringLiteral("node")).toString(),
             QString::number(filed->GetSerial()));
    QCOMPARE(recent.at(1).toObject().value(QStringLiteral("path")).toArray(),
             QJsonArray() << QStringLiteral("a folder") << QStringLiteral("inner"));
    QVERIFY(!recent.at(0).toObject().value(QStringLiteral("thumb")).toBool());
    for(const QJsonValue &tab : recent){
        const QString title = tab.toObject().value(QStringLiteral("title")).toString();
        QVERIFY(title != QStringLiteral("start") && title != QStringLiteral("blank"));
        QVERIFY(title != QStringLiteral("settings"));
        QVERIFY(!tab.toObject().value(QStringLiteral("closed")).toBool());
    }

    auto page = [&](int limit){
        QUrl url = Endpoint(QStringLiteral("tabs"));
        url.setQuery(QStringLiteral("q=&limit=%1").arg(limit));
        return QJsonDocument::fromJson(Get(url, start).m_Body).object();
    };
    auto serialAt = [](const QJsonArray &tabs, int i){
        return tabs.at(i).toObject().value(QStringLiteral("node")).toString();
    };
    const QJsonObject first = page(12);
    QVERIFY(first.value(QStringLiteral("more")).toBool());
    QCOMPARE(first.value(QStringLiteral("tabs")).toArray().size(), 12);
    QCOMPARE(page(24).value(QStringLiteral("tabs")).toArray().size(), 12);
    QCOMPARE(page(0).value(QStringLiteral("tabs")).toArray().size(), 12);
    QUrl history = Endpoint(QStringLiteral("tabs"));
    history.setQuery(QStringLiteral("view=history&offset=0&limit=50"));
    const QJsonObject whole = QJsonDocument::fromJson(Get(history, start).m_Body).object();
    const QJsonArray all = whole.value(QStringLiteral("tabs")).toArray();
    QCOMPARE(all.size(), 18);
    QVERIFY(!whole.value(QStringLiteral("more")).toBool());
    QCOMPARE(serialAt(all, 0), QString::number(closedLately->GetSerial()));
    QVERIFY(all.at(0).toObject().value(QStringLiteral("closed")).toBool());
    for(int i = 1; i < 10; i++) QCOMPARE(serialAt(all, i), serialAt(recent, i - 1));
    QCOMPARE(serialAt(all, 10), QString::number(closedLongAgo->GetSerial()));
    QCOMPARE(all.at(11).toObject().value(QStringLiteral("title")).toString(), QStringLiteral("older 6"));
    QVERIFY(!all.at(11).toObject().value(QStringLiteral("closed")).toBool());
    QCOMPARE(tabs("settings").size(), 1);

    QCOMPARE(tabs("\xE6\x97\xA5\xE6\x9C\xAC\xE3\x80\x80\xE3\x83\x9A\xE3\x83\xBC\xE3\x82\xB8").size(), 1);
    QCOMPARE(tabs("wiki \xE6\x97\xA5\xE6\x9C\xAC").size(), 1);
    QCOMPARE(tabs("qurl QT").size(), 1);
    QCOMPARE(tabs("qurl wikipedia").size(), 0);
    QCOMPARE(tabs("nothing-of-the-kind").size(), 0);
    QCOMPARE(tabs("closed").size(), 0);
    QCOMPARE(Get(Endpoint(QStringLiteral("tabs")), QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);

    auto open = [&](quint64 node){
        QUrl url(start.toString() + QStringLiteral("api/open"));
        url.setQuery(QStringLiteral("node=") + QString::number(node));
        return VanillaPage::Answer(url, QByteArrayLiteral("POST"), QByteArray(), start).m_Status;
    };
    QCOMPARE(open(closedLately->GetSerial()), 200);
    QCOMPARE(open(closedLately->GetSerial() + 1000), 400);
    QTest::qWait(20);
}

void tst_startpage::aStartPageIsLetGoOfOnlyAsALeafOfTheMainTree(){
    Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
    std::unique_ptr<MainWindow> window(Application::NewWindow(943));
    const auto unregister = qScopeGuard([&]{
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        Application::RemoveWindow(943);
        TreeBank::Initialize();
    });
    TreeBank *tb = window->GetTreeBank();
    QVERIFY(tb);
    ViewNode *root = TreeBank::GetViewRoot();
    auto tab = [](ViewNode *parent){
        ViewNode *vn = parent->MakeChild();
        vn->SetHoldView(true);
        vn->SetUrl(VanillaPage::StartUrl());
        return vn;
    };
    ViewNode *leaf = tab(root);
    ViewNode *folder = root->MakeChild();
    tab(folder);
    ViewNode *empty = root->MakeChild();
    ViewNode *inTrash = tab(TreeBank::GetTrashRoot());
    const int trashed = TreeBank::GetTrashRoot()->GetChildren().length();

    QVERIFY(!tb->CloseWithoutTrash(nullptr));
    QVERIFY(!tb->CloseWithoutTrash(folder));
    QVERIFY(!tb->CloseWithoutTrash(empty));
    QVERIFY(!tb->CloseWithoutTrash(inTrash));
    QVERIFY(root->GetChildren().contains(folder));
    QVERIFY(root->GetChildren().contains(empty));
    QVERIFY(TreeBank::GetTrashRoot()->GetChildren().contains(inTrash));

    QVERIFY(tb->CloseWithoutTrash(leaf));
    QVERIFY(!root->GetChildren().contains(leaf));
    QCOMPARE(TreeBank::GetTrashRoot()->GetChildren().length(), trashed);
}

void tst_startpage::aCardsTabGoesToTheTrashOnlyFromThePage(){
    Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
    std::unique_ptr<MainWindow> window(Application::NewWindow(944));
    Application::SetCurrentWindow(window.get());
    const auto unregister = qScopeGuard([&]{
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        Application::RemoveWindow(944);
        TreeBank::Initialize();
    });
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *leaf = root->MakeChild();
    leaf->SetHoldView(true);
    leaf->SetUrl(QUrl(QStringLiteral("https://example.com/")));
    const quint64 serial = leaf->GetSerial();
    const int trashed = TreeBank::GetTrashRoot()->GetChildren().length();

    const QUrl start = VanillaPage::StartUrl();
    auto close = [&](quint64 node, const char *method, const QUrl &initiator){
        QUrl url(start.toString() + QStringLiteral("api/close"));
        url.setQuery(QStringLiteral("node=") + QString::number(node));
        return VanillaPage::Answer(url, QByteArray(method), QByteArray(), initiator).m_Status;
    };
    QCOMPARE(close(serial, "GET", start), 403);
    QCOMPARE(close(serial, "POST", QUrl(QStringLiteral("https://example.com/"))), 403);
    QCOMPARE(close(serial, "POST", QUrl()), 403);
    QCOMPARE(close(serial + 1000, "POST", start), 400);
    QTest::qWait(20);
    QVERIFY(root->GetChildren().contains(leaf));
    QCOMPARE(TreeBank::GetTrashRoot()->GetChildren().length(), trashed);

    QCOMPARE(close(serial, "POST", start), 200);
    QVERIFY(root->GetChildren().contains(leaf));
    QTRY_VERIFY_WITH_TIMEOUT(!root->GetChildren().contains(leaf), 1000);
    QCOMPARE(TreeBank::GetTrashRoot()->GetChildren().length(), trashed + 1);
    QCOMPARE(close(serial, "POST", start), 400);
}

void tst_startpage::aTabOfTheTrashGoesBackWhereANewSiblingOfTheStartPageWould(){
    const auto clear = qScopeGuard([&]{ TreeBank::Initialize(); });
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *trash = TreeBank::GetTrashRoot();
    auto tab = [](ViewNode *parent, const QString &title){
        ViewNode *vn = parent->MakeChild();
        vn->SetHoldView(true);
        vn->SetTitle(title);
        vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + title));
        return vn;
    };
    ViewNode *folder = root->MakeChild();
    ViewNode *a = tab(folder, QStringLiteral("a"));
    ViewNode *self = tab(folder, QStringLiteral("start"));
    self->SetUrl(VanillaPage::StartUrl());
    ViewNode *b = tab(folder, QStringLiteral("b"));
    ViewNode *open = tab(root, QStringLiteral("open"));
    ViewNode *closed = tab(trash, QStringLiteral("closed"));
    ViewNode *other = tab(trash, QStringLiteral("other"));

    QVERIFY(!TreeBank::RestoreBeside(nullptr, self));
    QVERIFY(!TreeBank::RestoreBeside(open, self));
    QCOMPARE(root->GetChildren(), NodeList() << folder << open);

    QVERIFY(TreeBank::RestoreBeside(closed, self));
    QCOMPARE(folder->GetChildren(), NodeList() << a << self << closed << b);
    QVERIFY(!TreeBank::IsTrash(closed));
    QCOMPARE(trash->GetChildren(), NodeList() << other);

    QVERIFY(TreeBank::RestoreBeside(other, nullptr));
    QCOMPARE(root->GetChildren(), NodeList() << folder << open << other);
    ViewNode *third = tab(trash, QStringLiteral("third"));
    ViewNode *gone = tab(trash, QStringLiteral("gone"));
    QVERIFY(TreeBank::RestoreBeside(third, gone));
    QCOMPARE(root->GetChildren(), NodeList() << folder << open << other << third);
    QVERIFY(TreeBank::IsTrash(gone));
}

void tst_startpage::theFirstPageIsTheOpenTabsAloneHoweverFewThereAre(){
    const auto clear = qScopeGuard([&]{ TreeBank::Initialize(); });
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *trash = TreeBank::GetTrashRoot();
    const QUrl start = VanillaPage::StartUrl();
    auto tab = [](ViewNode *parent, const QString &name){
        ViewNode *vn = parent->MakeChild();
        vn->SetHoldView(true);
        vn->SetTitle(name);
        vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + name));
        return vn;
    };
    auto page = [&](int limit){
        QUrl url = Endpoint(QStringLiteral("tabs"));
        url.setQuery(QStringLiteral("q=&limit=%1").arg(limit));
        return QJsonDocument::fromJson(Get(url, start).m_Body).object();
    };
    auto count = [](const QJsonObject &answer, bool closed){
        int n = 0;
        for(const QJsonValue &tab : answer.value(QStringLiteral("tabs")).toArray())
            if(tab.toObject().value(QStringLiteral("closed")).toBool() == closed) n++;
        return n;
    };

    auto history = [&](){
        QUrl url = Endpoint(QStringLiteral("tabs"));
        url.setQuery(QStringLiteral("view=history&offset=0&limit=50"));
        return QJsonDocument::fromJson(Get(url, start).m_Body).object();
    };
    QVERIFY(!page(12).value(QStringLiteral("more")).toBool());
    tab(trash, QStringLiteral("closed 0"));
    QCOMPARE(page(12).value(QStringLiteral("tabs")).toArray().size(), 0);
    QVERIFY(page(12).value(QStringLiteral("more")).toBool());
    QCOMPARE(count(history(), true), 1);
    QVERIFY(!history().value(QStringLiteral("more")).toBool());

    tab(root, QStringLiteral("open 0"));
    tab(root, QStringLiteral("open 1"));
    tab(trash, QStringLiteral("closed 1"));
    tab(trash, QStringLiteral("closed 2"));
    const QJsonObject first = page(12);
    QCOMPARE(count(first, false), 2);
    QCOMPARE(count(first, true), 0);
    QVERIFY(first.value(QStringLiteral("more")).toBool());
    QCOMPARE(count(page(24), true), 0);
    const QJsonObject second = history();
    QCOMPARE(count(second, false), 2);
    QCOMPARE(count(second, true), 3);
    QVERIFY(!second.value(QStringLiteral("more")).toBool());
}

void tst_startpage::aClickWaitsForTheTreeToBeChangeableAndForgetsATabGone(){
    Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
    std::unique_ptr<MainWindow> window(Application::NewWindow(945));
    Application::SetCurrentWindow(window.get());
    const auto unregister = qScopeGuard([&]{
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        Application::RemoveWindow(945);
        TreeBank::Initialize();
    });
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *trash = TreeBank::GetTrashRoot();
    ViewNode *closed = trash->MakeChild();
    closed->SetHoldView(true);
    closed->SetUrl(QUrl(QStringLiteral("https://example.com/closed")));
    const quint64 serial = closed->GetSerial();
    ViewNode *leaf = root->MakeChild();
    leaf->SetHoldView(true);
    leaf->SetUrl(QUrl(QStringLiteral("https://example.com/leaf")));

    const QUrl start = VanillaPage::StartUrl();
    auto post = [&](const QString &endpoint, quint64 node){
        QUrl url(start.toString() + QStringLiteral("api/") + endpoint);
        url.setQuery(QStringLiteral("node=") + QString::number(node));
        return VanillaPage::Answer(url, QByteArrayLiteral("POST"), QByteArray(), start).m_Status;
    };
    auto pending = [&](ViewNode *vn){
        QUrl url = Endpoint(QStringLiteral("tabs"));
        url.setQuery(QStringLiteral("view=history&offset=0&limit=50"));
        const QJsonArray tabs = QJsonDocument::fromJson(Get(url, start).m_Body)
            .object().value(QStringLiteral("tabs")).toArray();
        for(const QJsonValue &tab : tabs)
            if(tab.toObject().value(QStringLiteral("node")).toString() == QString::number(vn->GetSerial()))
                return tab.toObject().value(QStringLiteral("pending")).toBool() ? 1 : 0;
        return -1;
    };
    QCOMPARE(pending(leaf), 0);
    Node *taken = nullptr;
    {
        TreeBank::ChangeScope changing;
        QCOMPARE(post(QStringLiteral("open"), serial), 200);
        QCOMPARE(post(QStringLiteral("close"), leaf->GetSerial()), 200);
        QCOMPARE(post(QStringLiteral("close"), leaf->GetSerial()), 200);
        QTest::qWait(150);
        QVERIFY(TreeBank::IsTrash(closed));
        QCOMPARE(root->GetChildren(), NodeList() << leaf);
        QCOMPARE(pending(leaf), 1);
        const QString page = QString::fromUtf8(Get(start, QUrl()).m_Body);
        QVERIFY(page.contains(QStringLiteral("\"node\":\"%1\",\"path\":[],\"pending\":true").arg(leaf->GetSerial())));
        taken = trash->TakeFirstChild();
    }
    QTRY_VERIFY_WITH_TIMEOUT(TreeBank::IsTrash(leaf), 1000);
    QVERIFY(root->GetChildren().isEmpty());
    QCOMPARE(trash->GetChildren(), NodeList() << leaf);
    QCOMPARE(pending(leaf), 0);
    delete taken;
}

void tst_startpage::theHistoryIsEveryTabByTheLastLookInOneOrder(){
    const auto clear = qScopeGuard([&]{ TreeBank::Initialize(); });
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *trash = TreeBank::GetTrashRoot();
    const QDateTime now = QDateTime(QDate(2026, 10, 21), QTime(12, 0, 0));
    auto tab = [&](ViewNode *parent, const QString &name, int secondsAgo){
        ViewNode *vn = parent->MakeChild();
        vn->SetHoldView(true);
        vn->SetTitle(name);
        vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + name));
        vn->SetLastAccessDate(now.addSecs(-secondsAgo));
        return vn;
    };
    ViewNode *a = tab(root, QStringLiteral("a"), 10);
    ViewNode *b = tab(root, QStringLiteral("b"), 10);
    ViewNode *c = tab(trash, QStringLiteral("c"), 5);
    ViewNode *d = tab(trash, QStringLiteral("d"), 3600 * 30);
    ViewNode *e = tab(root, QStringLiteral("e"), 0);
    tab(root, QStringLiteral("blank"), 0)->SetUrl(BLANK_URL);

    const QUrl start = VanillaPage::StartUrl();
    auto history = [&](int offset, int limit){
        QUrl url = Endpoint(QStringLiteral("tabs"));
        url.setQuery(QStringLiteral("view=history&offset=%1&limit=%2").arg(offset).arg(limit));
        return QJsonDocument::fromJson(Get(url, start).m_Body).object();
    };
    auto serials = [](const QJsonObject &answer){
        QStringList list;
        for(const QJsonValue &tab : answer.value(QStringLiteral("tabs")).toArray())
            list << tab.toObject().value(QStringLiteral("node")).toString();
        return list;
    };
    auto serial = [](ViewNode *vn){ return QString::number(vn->GetSerial()); };

    const QJsonObject all = history(0, 50);
    QCOMPARE(serials(all), QStringList() << serial(e) << serial(c) << serial(a) << serial(b) << serial(d));
    QVERIFY(!all.value(QStringLiteral("more")).toBool());
    const QJsonObject first = all.value(QStringLiteral("tabs")).toArray().at(0).toObject();
    QCOMPARE(first.value(QStringLiteral("seen")).toString(), QStringLiteral("2026-10-21T12:00:00"));
    QVERIFY(!first.value(QStringLiteral("closed")).toBool());
    QVERIFY(all.value(QStringLiteral("tabs")).toArray().at(1).toObject().value(QStringLiteral("closed")).toBool());
    QVERIFY(!first.value(QStringLiteral("icon")).toBool());

    TreeBank::MoveNode(a, trash);
    QCOMPARE(serials(history(0, 50)), QStringList() << serial(e) << serial(c) << serial(a) << serial(b) << serial(d));

    const QJsonObject page = history(1, 2);
    QCOMPARE(serials(page), QStringList() << serial(c) << serial(a));
    QVERIFY(page.value(QStringLiteral("more")).toBool());
    QVERIFY(history(2, 2).value(QStringLiteral("more")).toBool());
    QVERIFY(!history(3, 2).value(QStringLiteral("more")).toBool());
    QCOMPARE(serials(history(4, 2)), QStringList() << serial(d));
    QCOMPARE(serials(history(5, 2)), QStringList());
    QCOMPARE(serials(history(-3, 0)).size(), 1);

    auto found = [&](const char *q){
        QUrl url = Endpoint(QStringLiteral("tabs"));
        url.setQuery(QStringLiteral("view=history&offset=0&limit=50&q=") + QString::fromUtf8(q));
        return serials(QJsonDocument::fromJson(Get(url, start).m_Body).object());
    };
    QCOMPARE(found("example.com"), QStringList() << serial(e) << serial(c) << serial(a) << serial(b) << serial(d));
    QCOMPARE(found("com/d"), QStringList() << serial(d));
    QCOMPARE(found("com/e com/d"), QStringList());

    QVERIFY(QJsonDocument::fromJson(Get(Endpoint(QStringLiteral("tabs")), start).m_Body)
            .object().value(QStringLiteral("more")).toBool());
}

void tst_startpage::aRowsIconIsTheSitesCachedOneOrNothing(){
    const auto clear = qScopeGuard([&]{ TreeBank::Initialize(); });
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *vn = root->MakeChild();
    vn->SetHoldView(true);
    vn->SetUrl(QUrl(QStringLiteral("https://icon.example/page")));
    const QUrl start = VanillaPage::StartUrl();
    auto icon = [&](quint64 node){
        QUrl url = Endpoint(QStringLiteral("icon"));
        url.setQuery(QStringLiteral("node=") + QString::number(node));
        return Get(url, start);
    };
    QCOMPARE(icon(vn->GetSerial()).m_Status, 404);
    QCOMPARE(icon(vn->GetSerial() + 1000).m_Status, 404);

    QPixmap square(16, 16);
    square.fill(Qt::red);
    Application::RegisterIcon(QStringLiteral("icon.example"), QIcon(square), false);
    const VanillaPageResponse answer = icon(vn->GetSerial());
    QCOMPARE(answer.m_Status, 200);
    QVERIFY(answer.m_Body.startsWith(QByteArrayLiteral("\x89PNG")));
    const QJsonArray tabs = QJsonDocument::fromJson(Get(Endpoint(QStringLiteral("tabs")), start).m_Body)
        .object().value(QStringLiteral("tabs")).toArray();
    QCOMPARE(tabs.size(), 1);
    QVERIFY(tabs.at(0).toObject().value(QStringLiteral("icon")).toBool());
}

void tst_startpage::aDiscardTakesTheCheckedTabsOutOfTheTrashForGood(){
    Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
    std::unique_ptr<MainWindow> window(Application::NewWindow(946));
    Application::SetCurrentWindow(window.get());
    const auto unregister = qScopeGuard([&]{
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        Application::RemoveWindow(946);
        TreeBank::Initialize();
    });
    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *trash = TreeBank::GetTrashRoot();
    auto tab = [](ViewNode *parent, const QString &name){
        ViewNode *vn = parent->MakeChild();
        vn->SetHoldView(true);
        vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + name));
        return vn;
    };
    ViewNode *open = tab(root, QStringLiteral("open"));
    ViewNode *folder = trash->MakeChild();
    folder->SetTitle(QStringLiteral("folder"));
    ViewNode *inner = tab(folder, QStringLiteral("inner"));
    ViewNode *inner2 = tab(folder, QStringLiteral("inner 2"));
    ViewNode *loose = tab(trash, QStringLiteral("loose"));
    ViewNode *gone = tab(trash, QStringLiteral("gone"));

    const QUrl start = VanillaPage::StartUrl();
    auto discard = [&](const QString &nodes, const char *method = "POST", const QUrl &initiator = VanillaPage::StartUrl()){
        QUrl url(start.toString() + QStringLiteral("api/discard"));
        url.setQuery(QStringLiteral("nodes=") + QString(nodes).replace(QLatin1Char(','), QStringLiteral("%2C")));
        return VanillaPage::Answer(url, QByteArray(method), QByteArray(), initiator).m_Status;
    };
    auto serial = [](ViewNode *vn){ return QString::number(vn->GetSerial()); };
    auto pending = [&](ViewNode *vn){
        QUrl url = Endpoint(QStringLiteral("tabs"));
        url.setQuery(QStringLiteral("view=history&offset=0&limit=50"));
        const QJsonArray tabs = QJsonDocument::fromJson(Get(url, start).m_Body)
            .object().value(QStringLiteral("tabs")).toArray();
        for(const QJsonValue &tab : tabs)
            if(tab.toObject().value(QStringLiteral("node")).toString() == serial(vn))
                return tab.toObject().value(QStringLiteral("pending")).toBool() ? 1 : 0;
        return -1;
    };
    int askedAbout = -1;
    bool answer = true;
    TreeBank::SetDiscardConfirmer([&](int count){ askedAbout = count; return answer; });
    const auto unhook = qScopeGuard([]{ TreeBank::SetDiscardConfirmer(nullptr); });

    QCOMPARE(discard(serial(loose), "GET"), 403);
    QCOMPARE(discard(serial(loose), "POST", QUrl(QStringLiteral("https://example.com/"))), 403);
    QCOMPARE(discard(serial(loose), "POST", QUrl()), 403);
    QCOMPARE(discard(QString()), 400);
    QCOMPARE(discard(serial(loose) + QStringLiteral(",") + serial(open)), 400);
    QCOMPARE(discard(serial(loose) + QStringLiteral(",") + serial(loose) + QStringLiteral("0")), 400);
    QTest::qWait(50);
    QCOMPARE(trash->GetChildren(), NodeList() << folder << loose << gone);
    QCOMPARE(pending(loose), 0);

    Node *taken = nullptr;
    {
        TreeBank::ChangeScope changing;
        QCOMPARE(discard(serial(inner) + QStringLiteral(",") + serial(loose) + QStringLiteral(",") + serial(loose) + QStringLiteral(",") + serial(gone)), 200);
        QTest::qWait(150);
        QCOMPARE(trash->GetChildren(), NodeList() << folder << loose << gone);
        QCOMPARE(pending(inner), 1);
        QCOMPARE(pending(loose), 1);
        QCOMPARE(pending(inner2), 0);
        taken = trash->TakeLastChild();
    }
    QTRY_VERIFY_WITH_TIMEOUT(!trash->GetChildren().contains(loose), 1000);
    QCOMPARE(askedAbout, 3);
    QCOMPARE(trash->GetChildren(), NodeList() << folder);
    QCOMPARE(folder->GetChildren(), NodeList() << inner2);
    QCOMPARE(root->GetChildren(), NodeList() << open);
    QCOMPARE(pending(inner2), 0);
    QCOMPARE(pending(loose), -1);

    answer = false;
    askedAbout = -1;
    QCOMPARE(discard(serial(inner2)), 200);
    QTRY_COMPARE_WITH_TIMEOUT(askedAbout, 1, 1000);
    QTest::qWait(100);
    QCOMPARE(folder->GetChildren(), NodeList() << inner2);
    QCOMPARE(pending(inner2), 0);

    answer = true;
    QCOMPARE(discard(serial(inner2)), 200);
    QTRY_VERIFY_WITH_TIMEOUT(trash->GetChildren().isEmpty(), 1000);
    delete taken;
}

int main(int argc, char **argv){
    Application app(argc, argv);
    tst_startpage test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_startpage.moc"
