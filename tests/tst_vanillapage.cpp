#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QUrl>
#include <QByteArray>

#include "settingspage.hpp"
#include "directorypage.hpp"
#include "treebank.hpp"

#include "testsupport.hpp"

class tst_vanillapage : public QObject {
    Q_OBJECT

private:
    static QUrl Settings(const QString &path){
        return QUrl(VANILLA_SCHEME + QStringLiteral("://settings") + path);
    }
    static VanillaPageResponse Get(const QUrl &url, const QUrl &initiator = QUrl()){
        return VanillaPage::Answer(url, QByteArrayLiteral("GET"),
                                   QByteArray(), initiator);
    }
    static VanillaPageResponse Post(const QUrl &url, const QByteArray &body,
                                    const QUrl &initiator = QUrl()){
        return VanillaPage::Answer(url, QByteArrayLiteral("POST"), body, initiator);
    }
    static QUrl DirectoryPageUrl(){
        return QUrl(VANILLA_SCHEME + QStringLiteral("://directory/"));
    }

private slots:
    void initTestCase(){ TestSupport::SilenceDebugOutput();}

    void onlyTheTwoPagesAreThisSchemes();
    void anAddressWhichIsNotOneOfThesePagesIsRefused();
    void aPathTheseDoNotServeIsNotFound();
    void everyEndpointTakesOneMethodAndNoOther();
    void anEndpointWhichDoesNotExistIsNotFound();
    void aRequestFromSomewhereElseIsRefused();
    void onlyWhatThePageItselfSentIsRead();
    void theHeadersDecideWhoTheInitiatorIs();
    void aRequestForThePageReachesTheResources();
    void thePageCarriesItsColoursAheadOfItsSheet();
    void theDirectoryNamingTheNativeViewHandsThesePagesToTheEngine();
    void theSettingsTabIsTheSameWhicheverCategoryItShows();
};

void tst_vanillapage::onlyTheTwoPagesAreThisSchemes(){
    QVERIFY(VanillaPage::IsPageUrl(VanillaPage::SettingsUrl()));
    QVERIFY(VanillaPage::IsSettingsUrl(VanillaPage::SettingsUrl()));

    QVERIFY(!VanillaPage::IsSettingsUrl
            (QUrl(VANILLA_SCHEME + QStringLiteral("://elsewhere/"))));
    QVERIFY(!VanillaPage::IsPageUrl(QUrl(QStringLiteral("https://settings/"))));
    QVERIFY(!VanillaPage::IsPageUrl(QUrl()));
}

void tst_vanillapage::anAddressWhichIsNotOneOfThesePagesIsRefused(){
    QCOMPARE(Get(QUrl(QStringLiteral("https://example.com/"))).m_Status, 400);
    QCOMPARE(Get(QUrl(VANILLA_SCHEME + QStringLiteral("://elsewhere/"))).m_Status, 400);
}

void tst_vanillapage::aPathTheseDoNotServeIsNotFound(){
    QCOMPARE(Get(Settings(QStringLiteral("/nothing-here"))).m_Status, 404);
    QCOMPARE(Get(Settings(QStringLiteral("/settings.png"))).m_Status, 404);
}

void tst_vanillapage::everyEndpointTakesOneMethodAndNoOther(){
    QCOMPARE(Post(Settings(QStringLiteral("/api/schema")), QByteArray(),
                  VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/set")),
                 VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/reset")),
                 VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/pick-directory")),
                 VanillaPage::SettingsUrl()).m_Status, 403);

    const VanillaPageResponse picker = Post
        (Settings(QStringLiteral("/api/pick-directory")),
         QByteArrayLiteral("{\"key\":\"network/@Extensions\"}"),
         VanillaPage::SettingsUrl());
    QCOMPARE(picker.m_Status, 200);
    QVERIFY(picker.NeedsUserInteraction());
    QVERIFY(picker.m_UserInteraction ==
            VanillaPageResponse::UserInteraction::PickChromeExtensionDirectory);

    const VanillaPageResponse noPicker = Post
        (Settings(QStringLiteral("/api/pick-directory")),
         QByteArrayLiteral("{\"key\":\"application/@AllowedHosts\"}"),
         VanillaPage::SettingsUrl());
    QCOMPARE(noPicker.m_Status, 200);
    QVERIFY(!noPicker.NeedsUserInteraction());
    QVERIFY(noPicker.m_Body.contains("no directory picker"));
}

void tst_vanillapage::anEndpointWhichDoesNotExistIsNotFound(){
    QCOMPARE(Get(Settings(QStringLiteral("/api/everything")),
                 VanillaPage::SettingsUrl()).m_Status, 404);
    QCOMPARE(Post(Settings(QStringLiteral("/api/everything")),
                  QByteArrayLiteral("{}"), VanillaPage::SettingsUrl()).m_Status, 404);
}

void tst_vanillapage::aRequestFromSomewhereElseIsRefused(){
    QCOMPARE(Get(Settings(QStringLiteral("/api/schema")),
                 QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/set")), QByteArrayLiteral("{}"),
                  QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/pick-directory")),
                  QByteArrayLiteral("{\"key\":\"network/@Extensions\"}"),
                  QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);

    QCOMPARE(Get(Settings(QStringLiteral("/api/schema"))).m_Status, 200);

    const QUrl stranger(QStringLiteral("https://example.com/"));
    const QList<QUrl> documents = QList<QUrl>()
        << Settings(QStringLiteral("/")) << Settings(QStringLiteral("/settings.js"))
        << Settings(QStringLiteral("/settings.css")) << DirectoryPageUrl()
        << QUrl(DirectoryPageUrl().toString() + QStringLiteral("directory.js"));
    foreach(const QUrl &document, documents){
        QCOMPARE(Get(document, stranger).m_Status, 403);
        QCOMPARE(Get(document, QUrl(QStringLiteral("null"))).m_Status, 403);
        QVERIFY2(Get(document).m_Status != 403, qPrintable(document.toString()));
        QVERIFY2(Get(document, Settings(QStringLiteral("/"))).m_Status != 403, qPrintable(document.toString()));
        QVERIFY2(Get(document, DirectoryPageUrl()).m_Status != 403, qPrintable(document.toString()));
    }
}

void tst_vanillapage::onlyWhatThePageItselfSentIsRead(){
    const QByteArray post = QByteArrayLiteral("POST"), get = QByteArrayLiteral("GET");
    const QUrl page = Settings(QStringLiteral("/")), set = Settings(QStringLiteral("/api/set"));
    const QUrl directory = DirectoryPageUrl(), directorySet = QUrl(directory.toString() + QStringLiteral("api/set"));

    QVERIFY(VanillaPage::ReadsBody(set, post, page));
    QVERIFY(VanillaPage::ReadsBody(set, post, Settings(QStringLiteral("/#appearance"))));
    QVERIFY(VanillaPage::ReadsBody(directorySet, post, directory));

    QVERIFY(!VanillaPage::ReadsBody(set, post, QUrl(QStringLiteral("https://evil.example"))));
    QVERIFY(!VanillaPage::ReadsBody(set, post, QUrl(QStringLiteral("null"))));
    QVERIFY(!VanillaPage::ReadsBody(set, post, QUrl(QStringLiteral("form://unnamed"))));
    QVERIFY(!VanillaPage::ReadsBody(directorySet, post, QUrl(QStringLiteral("https://evil.example"))));
    QVERIFY(!VanillaPage::ReadsBody(set, post, QUrl()));
    QVERIFY(!VanillaPage::ReadsBody(set, post, directory));
    QVERIFY(!VanillaPage::ReadsBody(directorySet, post, page));
    QVERIFY(!VanillaPage::ReadsBody(page, post, page));
    QVERIFY(!VanillaPage::ReadsBody(Settings(QStringLiteral("/settings.js")), post, page));
    QVERIFY(!VanillaPage::ReadsBody(set, get, page));
    QVERIFY(!VanillaPage::ReadsBody(QUrl(QStringLiteral("https://settings/api/set")), post, page));

    QCOMPARE(Post(set, QByteArray(), QUrl(QStringLiteral("https://evil.example"))).m_Status, 403);
}

void tst_vanillapage::theHeadersDecideWhoTheInitiatorIs(){

    const QUrl settings = VanillaPage::SettingsUrl();
    const QString foreign = QStringLiteral("https://example.com");

    QCOMPARE(VanillaPage::InitiatorFromHeaders(foreign, QString(), settings),
             QUrl(foreign));
    QCOMPARE(VanillaPage::InitiatorFromHeaders(foreign, settings.toString(),
                                               settings),
             QUrl(foreign));
    QCOMPARE(VanillaPage::InitiatorFromHeaders(QStringLiteral("null"),
                                               QString(), settings),
             QUrl(QStringLiteral("null")));
    QCOMPARE(VanillaPage::InitiatorFromHeaders(QString(), settings.toString(),
                                               QUrl(foreign)),
             settings);
    QCOMPARE(VanillaPage::InitiatorFromHeaders(QString(), QString(), settings),
             settings);
    QCOMPARE(VanillaPage::InitiatorFromHeaders(QString(), QString(),
                                               QUrl(foreign)),
             QUrl());

    const QUrl unnamedForm =
        VanillaPage::InitiatorFromHeaders(QString(), QString(), settings, true);
    QVERIFY(!unnamedForm.isEmpty());
    QVERIFY(!VanillaPage::IsPageUrl(unnamedForm));
    QCOMPARE(VanillaPage::InitiatorFromHeaders(QString(), QString(),
                                               QUrl(foreign), false),
             QUrl());
    QCOMPARE(VanillaPage::InitiatorFromHeaders(QString(), settings.toString(),
                                               settings, true),
             settings);
    QCOMPARE(VanillaPage::InitiatorFromHeaders(QString(), foreign,
                                               QUrl(foreign), true),
             QUrl(foreign));

    QCOMPARE(Post(Settings(QStringLiteral("/api/set")), QByteArrayLiteral("{}"),
                  VanillaPage::InitiatorFromHeaders(foreign, QString(),
                                                    settings)).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/set")), QByteArrayLiteral("{}"),
                  VanillaPage::InitiatorFromHeaders(QStringLiteral("null"),
                                                    QString(),
                                                    settings)).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/set")), QByteArrayLiteral("{}"),
                  unnamedForm).m_Status, 403);
}

void tst_vanillapage::aRequestForThePageReachesTheResources(){

    foreach(const QString &path, QStringList()
            << QString() << QStringLiteral("/")
            << QStringLiteral("/settings.css") << QStringLiteral("/settings.js")){
        const VanillaPageResponse response = Get(Settings(path));
        QVERIFY2(response.m_Status != 403, qPrintable(path));
        QVERIFY2(response.m_Body.isEmpty() || response.m_Status == 200,
                 qPrintable(path));
    }

    QVERIFY(VanillaPage::IsPageUrl(DirectoryPageUrl()));
    QVERIFY(!VanillaPage::IsSettingsUrl(DirectoryPageUrl()));
}

void tst_vanillapage::theDirectoryNamingTheNativeViewHandsThesePagesToTheEngine(){
    const QUrl settings = VanillaPage::SettingsUrl();
    const QUrl directory = DirectoryPageUrl();

    foreach(const QString &name, QStringList()
            << QStringLiteral("nw") << QStringLiteral("NWV")
            << QStringLiteral("nativeweb") << QStringLiteral("nativewebview")
            << QStringLiteral("qnw") << QStringLiteral("quicknativewebview")){
        QVERIFY2(TreeBank::NeedsEngineForVanillaPage(settings, QStringList(name)),
                 qPrintable(name));
        QVERIFY2(TreeBank::NeedsEngineForVanillaPage(directory, QStringList(name)),
                 qPrintable(name));
    }

    foreach(const QString &name, QStringList()
            << QStringLiteral("web") << QStringLiteral("quickweb")
            << QStringLiteral("webengine") << QStringLiteral("edge")
            << QStringLiteral("local")){
        QVERIFY2(!TreeBank::NeedsEngineForVanillaPage(settings, QStringList(name)),
                 qPrintable(name));
    }
    QVERIFY(!TreeBank::NeedsEngineForVanillaPage(settings, QStringList()));
    QVERIFY(!TreeBank::NeedsEngineForVanillaPage(
                QUrl(QStringLiteral("https://example.com/")),
                QStringList(QStringLiteral("nativeweb"))));
}

void tst_vanillapage::thePageCarriesItsColoursAheadOfItsSheet(){
    const QByteArray page = QByteArrayLiteral(
        "<!DOCTYPE html>\n<html>\n  <head>\n    <meta charset=\"utf-8\">\n"
        "    <link rel=\"stylesheet\" href=\"/directory.css\">\n  </head>\n"
        "  <body></body>\n</html>\n");
    const QByteArray served = VanillaPage::WithInlinePalette(page);

    const int style = served.indexOf("<style>");
    const int sheet = served.indexOf("<link rel=");
    QVERIFY(style >= 0);
    QVERIFY(sheet >= 0);
    QVERIFY(style < sheet);
    QVERIFY(style > served.indexOf("<head>"));
    QVERIFY(style < served.indexOf("</head>"));
    QVERIFY(served.contains("--bg:"));
    QVERIFY(served.contains("prefers-color-scheme: dark"));
    QVERIFY(served.contains("html { background: var(--bg); }"));
    QCOMPARE(served.count("<link rel="), 1);
    QVERIFY(served.endsWith("</html>\n"));

    const QByteArray bare = QByteArrayLiteral("<html><body></body></html>");
    QCOMPARE(VanillaPage::WithInlinePalette(bare), bare);
}

void tst_vanillapage::theSettingsTabIsTheSameWhicheverCategoryItShows(){
    const QUrl page = VanillaPage::SettingsUrl();
    QUrl network = page; network.setFragment(QStringLiteral("network"));
    QUrl view = page;    view.setFragment(QStringLiteral("view"));
    QVERIFY(VanillaPage::SameDocument(page, network));
    QVERIFY(VanillaPage::SameDocument(network, page));
    QVERIFY(VanillaPage::SameDocument(network, view));
    const QUrl bare(VANILLA_SCHEME + QStringLiteral("://settings"));
    QVERIFY(VanillaPage::SameDocument(page, bare));
    QVERIFY(VanillaPage::SameDocument(bare, network));
    QVERIFY(VanillaPage::SameDocument(bare, QUrl(VANILLA_SCHEME + QStringLiteral("://settings#view"))));
    QVERIFY(!VanillaPage::SameDocument(network, DirectoryPage::PageUrl()));
    QVERIFY(!VanillaPage::SameDocument
            (page, QUrl(VANILLA_SCHEME + QStringLiteral("://settings/api/schema"))));
}
QTEST_MAIN(tst_vanillapage)
#include "tst_vanillapage.moc"
