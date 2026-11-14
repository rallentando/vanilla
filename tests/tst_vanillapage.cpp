#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QUrl>
#include <QByteArray>
#include <QFile>

#include "application.hpp"
#include "settingspage.hpp"
#include "directorypage.hpp"
#include "treebank.hpp"
#include "permissiontable.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "lightnode.hpp"
#include <QJsonDocument>
#include <QJsonObject>

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
    void aKeyBeingRecordedIsSaidToTheBackends(){
        const QUrl url(QStringLiteral("vanilla://settings/api/input-recording"));
        const QUrl initiator = VanillaPage::SettingsUrl();
        QVERIFY(!VanillaPage::IsRecordingKey());
        QCOMPARE(VanillaPage::Answer(url, QByteArrayLiteral("GET"), QByteArray(), initiator).m_Status, 403);
        QVERIFY(!VanillaPage::IsRecordingKey());
        VanillaPage::Answer(url, QByteArrayLiteral("POST"), "{\"on\":true}", initiator);
        QVERIFY(VanillaPage::IsRecordingKey());
        VanillaPage::Answer(url, QByteArrayLiteral("POST"), "{\"on\":false}", initiator);
        QVERIFY(!VanillaPage::IsRecordingKey());
    }

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
    void theKindOfViewTheSettingsTabKeepsIsTheDirectorys();
    void thePageCarriesItsSchemaAheadOfItsScript();
    void thePageCarriesItsSheetAndItsScriptInIt();
};

void tst_vanillapage::onlyTheTwoPagesAreThisSchemes(){
    QVERIFY(VanillaPage::IsPageUrl(VanillaPage::SettingsUrl()));
    QVERIFY(VanillaPage::IsSettingsUrl(VanillaPage::SettingsUrl()));

    QVERIFY(!VanillaPage::IsSettingsUrl
            (QUrl(VANILLA_SCHEME + QStringLiteral("://elsewhere/"))));
    QVERIFY(!VanillaPage::IsPageUrl(QUrl(QStringLiteral("https://settings/"))));
    QVERIFY(!VanillaPage::IsPageUrl(QUrl()));

    QVERIFY(VanillaPage::IsTranslucentPage(VanillaPage::StartUrl()));
    QVERIFY(VanillaPage::IsTranslucentPage(VanillaPage::SettingsUrl()));
    QVERIFY(VanillaPage::IsTranslucentPage(QUrl(VANILLA_SCHEME + QStringLiteral("://settings/#appearance"))));
    QVERIFY(!VanillaPage::IsTranslucentPage(DirectoryPage::PageUrl()));
    QVERIFY(!VanillaPage::IsTranslucentPage(QUrl(QStringLiteral("https://example.com/"))));
    QVERIFY(!VanillaPage::IsTranslucentPage(QUrl()));

    const QJsonObject schema = QJsonDocument::fromJson(
        Get(Settings(QStringLiteral("/api/schema"))).m_Body).object();
    QVERIFY(schema.value(QStringLiteral("translucent")).isBool());
    QCOMPARE(schema.value(QStringLiteral("translucent")).toBool(), Application::EnableTransparentWindow());

    const QString dir = QStringLiteral(VANILLA_SOURCE_DIR) + QStringLiteral("/../resources/settings/");
    QFile html(dir + QStringLiteral("settings.html")), sheet(dir + QStringLiteral("settings.css"));
    QVERIFY(html.open(QIODevice::ReadOnly));
    QVERIFY(sheet.open(QIODevice::ReadOnly));
    const QByteArray page = VanillaPage::WithInlineResources(
        VanillaPage::WithInlinePalette(html.readAll()), sheet.readAll(), QByteArray());
    const int ground = page.indexOf("html { background: var(--bg); }");
    const int tint   = page.indexOf("html.translucent { background: color-mix(in srgb, var(--bg) 50%, transparent); }");
    QVERIFY(ground >= 0);
    QVERIFY(tint > ground);
    QVERIFY(page.contains("html.translucent body { background: transparent; }"));

    QFile script(dir + QStringLiteral("settings.js"));
    QVERIFY(script.open(QIODevice::ReadOnly));
    QVERIFY(script.readAll().contains("if (schema.translucent) document.documentElement.classList.add('translucent');"));
    const QString src = QStringLiteral(VANILLA_SOURCE_DIR);
    foreach(const QString &view, QStringList()
            << QStringLiteral("/view/edge/edgewebviewprofile.cpp")
            << QStringLiteral("/view/webengine/webengineview.cpp")
            << QStringLiteral("/view/webengine/quickwebengineview.cpp")){
        QFile file(src + view);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(view));
        const QByteArray code = file.readAll();
        QVERIFY2(code.contains("Application::EnableTransparentWindow() &&") &&
                 code.contains("VanillaPage::IsTranslucentPage("), qPrintable(view));
        QVERIFY2(!code.contains("VanillaPage::IsStartUrl("), qPrintable(view));
    }
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

    const VanillaPageResponse folder = Post
        (Settings(QStringLiteral("/api/pick-directory")),
         QByteArrayLiteral("{\"key\":\"application/@FileSaveDirectory\"}"),
         VanillaPage::SettingsUrl());
    QCOMPARE(folder.m_Status, 200);
    QVERIFY(folder.m_UserInteraction ==
            VanillaPageResponse::UserInteraction::Deferred);
    QVERIFY(folder.m_Deferred);

    const VanillaPageResponse noPicker = Post
        (Settings(QStringLiteral("/api/pick-directory")),
         QByteArrayLiteral("{\"key\":\"application/@AllowedHosts\"}"),
         VanillaPage::SettingsUrl());
    QCOMPARE(noPicker.m_Status, 200);
    QVERIFY(!noPicker.NeedsUserInteraction());
    QVERIFY(noPicker.m_Body.contains("no directory picker"));

    const VanillaPageResponse colour = Post
        (Settings(QStringLiteral("/api/pick-color")),
         QByteArrayLiteral("{\"key\":\"application/@WindowEdgeColor\"}"),
         VanillaPage::SettingsUrl());
    QCOMPARE(colour.m_Status, 200);
    QVERIFY(colour.m_UserInteraction ==
            VanillaPageResponse::UserInteraction::Deferred);
    QVERIFY(colour.m_Deferred);
    const VanillaPageResponse noColour = Post
        (Settings(QStringLiteral("/api/pick-color")),
         QByteArrayLiteral("{\"key\":\"application/@FileSaveDirectory\"}"),
         VanillaPage::SettingsUrl());
    QCOMPARE(noColour.m_Status, 200);
    QVERIFY(!noColour.NeedsUserInteraction());
    QVERIFY(noColour.m_Body.contains("no colour picker"));
    QCOMPARE(Get(Settings(QStringLiteral("/api/pick-color")),
                 VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/pick-color")),
                  QByteArrayLiteral("{\"key\":\"application/@WindowEdgeColor\"}"),
                  QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);

    QCOMPARE(Post(Settings(QStringLiteral("/api/passwords")), QByteArray(),
                  VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/passwords-remove")),
                 VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/passwords-remove-all")),
                 VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/passwords-remove-all"))).m_Status, 403);

    const VanillaPageResponse list = Get(Settings(QStringLiteral("/api/passwords")),
                                         VanillaPage::SettingsUrl());
    QCOMPARE(list.m_Status, 200);
    QVERIFY(list.m_Body.contains("\"entries\""));
    QVERIFY(!list.m_Body.contains("\"password\""));

    const VanillaPageResponse remove = Post
        (Settings(QStringLiteral("/api/passwords-remove")),
         QByteArrayLiteral("{\"space\":\"s\",\"origin\":\"https://a.example:443\",\"username\":\"u\"}"),
         VanillaPage::SettingsUrl());
    QCOMPARE(remove.m_Status, 200);
    QVERIFY(remove.m_UserInteraction == VanillaPageResponse::UserInteraction::Deferred);
    QCOMPARE(Get(Settings(QStringLiteral("/api/passwords-never-remove")),
                 VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/passwords-never-remove")),
                  QByteArrayLiteral("{\"space\":\"s\",\"origin\":\"https://a.example\"}"),
                  QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QVERIFY(Post(Settings(QStringLiteral("/api/passwords-never-remove")),
                 QByteArrayLiteral("{\"space\":\"s\",\"origin\":\"https://a.example\"}"),
                 VanillaPage::SettingsUrl()).m_UserInteraction ==
            VanillaPageResponse::UserInteraction::Deferred);
    QVERIFY(Post(Settings(QStringLiteral("/api/passwords-never-remove")),
                 QByteArrayLiteral("{\"space\":\"s\",\"origin\":\"\"}"),
                 VanillaPage::SettingsUrl()).m_Body.contains("\"error\""));
    const VanillaPageResponse removeAll = Post
        (Settings(QStringLiteral("/api/passwords-remove-all")), QByteArrayLiteral("{}"),
         VanillaPage::SettingsUrl());
    QVERIFY(removeAll.m_UserInteraction == VanillaPageResponse::UserInteraction::Deferred);

    QCOMPARE(Post(Settings(QStringLiteral("/api/permissions")), QByteArray(),
                  VanillaPage::SettingsUrl()).m_Status, 403);
    for(const char *endpoint : {"/api/permissions-set", "/api/permissions-remove",
                                "/api/permissions-remove-all"}){
        QCOMPARE(Get(Settings(QString::fromLatin1(endpoint)),
                     VanillaPage::SettingsUrl()).m_Status, 403);
        QCOMPARE(Post(Settings(QString::fromLatin1(endpoint)), QByteArrayLiteral("{}"),
                      QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    }
    const VanillaPageResponse permissions = Get(Settings(QStringLiteral("/api/permissions")),
                                                VanillaPage::SettingsUrl());
    QCOMPARE(permissions.m_Status, 200);
    QVERIFY(permissions.m_Body.contains("\"entries\""));
    QVERIFY(permissions.m_Body.contains("\"notifications\""));
    QVERIFY(permissions.m_Body.contains("\"kinds\":[\"notifications\",\"geolocation\",\"microphone\",\"camera\""));
    {
        const QJsonArray kinds = QJsonDocument::fromJson(permissions.m_Body).object()
            .value(QStringLiteral("kinds")).toArray();
        const int edge = kinds.toVariantList().indexOf(QStringLiteral("clipboard-read"));
        const int engine = kinds.toVariantList().indexOf(QStringLiteral("clipboard"));
        QVERIFY(edge != -1 && engine != -1);
        QCOMPARE(kinds.size(), PermissionTable::Kinds().size());
#ifdef EDGEWEBVIEW
        QVERIFY(edge < engine);
#else
        QVERIFY(engine < edge);
#endif
    }
    const VanillaPageResponse setOne = Post
        (Settings(QStringLiteral("/api/permissions-set")),
         QByteArrayLiteral("{\"origin\":\"https://a.example\",\"kind\":\"camera\",\"state\":\"block\"}"),
         VanillaPage::SettingsUrl());
    QCOMPARE(setOne.m_Status, 200);
    QVERIFY(setOne.m_UserInteraction == VanillaPageResponse::UserInteraction::Deferred);
    for(const QByteArray &wrong : {
            QByteArrayLiteral("{\"origin\":\"https://a.example/path\",\"kind\":\"camera\",\"state\":\"block\"}"),
            QByteArrayLiteral("{\"origin\":\"https://a.example\",\"kind\":\"no-such\",\"state\":\"block\"}"),
            QByteArrayLiteral("{\"origin\":\"https://a.example\",\"kind\":\"camera\",\"state\":\"ask\"}"),
            QByteArrayLiteral("{\"origin\":\"\",\"kind\":\"camera\",\"state\":\"block\"}")}){
        const VanillaPageResponse refused = Post
            (Settings(QStringLiteral("/api/permissions-set")), wrong, VanillaPage::SettingsUrl());
        QVERIFY(!refused.NeedsUserInteraction());
        QVERIFY(refused.m_Body.contains("\"error\""));
    }
    QVERIFY(Post(Settings(QStringLiteral("/api/permissions-set")),
                 QByteArrayLiteral("{\"site\":\"A.example/x\",\"kind\":\"camera\",\"state\":\"allow\"}"),
                 VanillaPage::SettingsUrl()).m_UserInteraction ==
            VanillaPageResponse::UserInteraction::Deferred);
    for(const QByteArray &wrong : {
            QByteArrayLiteral("{\"site\":\"ftp://a.example\",\"kind\":\"camera\",\"state\":\"allow\"}"),
            QByteArrayLiteral("{\"site\":\"\",\"kind\":\"camera\",\"state\":\"allow\"}"),
            QByteArrayLiteral("{\"site\":\"a.example\",\"kind\":\"no-such\",\"state\":\"allow\"}"),
            QByteArrayLiteral("{\"site\":\"a.example\",\"origin\":\"https://a.example\",\"kind\":\"camera\",\"state\":\"allow\"}")}){
        const VanillaPageResponse refused = Post
            (Settings(QStringLiteral("/api/permissions-set")), wrong, VanillaPage::SettingsUrl());
        QVERIFY2(!refused.NeedsUserInteraction(), wrong.constData());
        QVERIFY(refused.m_Body.contains("\"error\""));
    }
    QVERIFY(Post(Settings(QStringLiteral("/api/permissions-remove")),
                 QByteArrayLiteral("{\"site\":\"a.example\",\"kind\":\"camera\"}"),
                 VanillaPage::SettingsUrl()).m_Body.contains("\"error\""));
    QVERIFY(Post(Settings(QStringLiteral("/api/permissions-remove")),
                 QByteArrayLiteral("{\"origin\":\"https://a.example\",\"kind\":\"camera\"}"),
                 VanillaPage::SettingsUrl()).m_UserInteraction ==
            VanillaPageResponse::UserInteraction::Deferred);
    QVERIFY(Post(Settings(QStringLiteral("/api/permissions-remove")),
                 QByteArrayLiteral("{\"origin\":\"https://a.example\",\"kind\":\"future\"}"),
                 VanillaPage::SettingsUrl()).m_UserInteraction ==
            VanillaPageResponse::UserInteraction::Deferred);
    QVERIFY(Post(Settings(QStringLiteral("/api/permissions-remove")),
                 QByteArrayLiteral("{\"origin\":\"\",\"kind\":\"camera\"}"),
                 VanillaPage::SettingsUrl()).m_Body.contains("\"error\""));
    QVERIFY(Post(Settings(QStringLiteral("/api/permissions-remove-all")), QByteArrayLiteral("{}"),
                 VanillaPage::SettingsUrl()).m_UserInteraction ==
            VanillaPageResponse::UserInteraction::Deferred);

    QCOMPARE(Post(Settings(QStringLiteral("/api/downloads")), QByteArray(),
                  VanillaPage::SettingsUrl()).m_Status, 403);
    for(const char *endpoint : {"/api/downloads-open", "/api/downloads-show",
                                "/api/downloads-cancel", "/api/downloads-remove",
                                "/api/downloads-clear"}){
        QCOMPARE(Get(Settings(QString::fromLatin1(endpoint)),
                     VanillaPage::SettingsUrl()).m_Status, 403);
        QCOMPARE(Get(Settings(QString::fromLatin1(endpoint))).m_Status, 403);
        const VanillaPageResponse post = Post(Settings(QString::fromLatin1(endpoint)),
                                              QByteArrayLiteral("{\"id\":1}"),
                                              VanillaPage::SettingsUrl());
        QCOMPARE(post.m_Status, 200);
        QVERIFY(post.m_UserInteraction == VanillaPageResponse::UserInteraction::Deferred);
    }
    QCOMPARE(Get(Settings(QStringLiteral("/api/downloads-source")),
                 VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/downloads-source")), QByteArrayLiteral("{\"id\":1}"),
                  QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    {
        const VanillaPageResponse source = Post(Settings(QStringLiteral("/api/downloads-source")),
                                                QByteArrayLiteral("{\"id\":987654}"),
                                                VanillaPage::SettingsUrl());
        QCOMPARE(source.m_Status, 200);
        QVERIFY(!source.NeedsUserInteraction());
        QVERIFY(source.m_Body.contains("\"ok\":false"));
    }
    const VanillaPageResponse downloads = Get(Settings(QStringLiteral("/api/downloads")),
                                              VanillaPage::SettingsUrl());
    QCOMPARE(downloads.m_Status, 200);
    QVERIFY(downloads.m_Body.contains("\"entries\""));

    QCOMPARE(Post(Settings(QStringLiteral("/api/directory")), QByteArray(),
                  VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/directory-set")),
                 VanillaPage::SettingsUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/directory-set"))).m_Status, 403);
    const VanillaPageResponse directorySet = Post
        (Settings(QStringLiteral("/api/directory-set")),
         QByteArrayLiteral("{\"node\":\"1\",\"name\":\"x\",\"tokens\":[]}"),
         VanillaPage::SettingsUrl());
    QCOMPARE(directorySet.m_Status, 200);
    QVERIFY(directorySet.m_UserInteraction == VanillaPageResponse::UserInteraction::Deferred);

    const VanillaPageResponse old = Get(DirectoryPageUrl());
    QCOMPARE(old.m_Status, 200);
    QVERIFY(old.m_Body.contains("vanilla://settings/#directory"));
    QVERIFY(!old.m_Body.contains("<script"));
    QCOMPARE(Get(QUrl(DirectoryPageUrl().toString() + QStringLiteral("api/tree")),
                 DirectoryPageUrl()).m_Status, 404);
    QCOMPARE(Post(QUrl(DirectoryPageUrl().toString() + QStringLiteral("api/set")),
                  QByteArrayLiteral("{}"), DirectoryPageUrl()).m_Status, 404);

    TreeBank::Initialize();
    ViewNode *a = TreeBank::GetViewRoot()->MakeChild();
    a->SetTitle(QStringLiteral("a"));
    ViewNode *b = TreeBank::GetViewRoot()->MakeChild();
    b->SetTitle(QStringLiteral("b"));
    const QString idOfB = DirectoryPage::Describe(b).value(QStringLiteral("current")).toString();
    DirectoryPage::SetPendingSubject(a);
    const QJsonObject asked = QJsonDocument::fromJson
        (Get(Settings(QStringLiteral("/api/directory?node=") + idOfB),
             VanillaPage::SettingsUrl()).m_Body).object();
    QCOMPARE(asked.value(QStringLiteral("current")).toString(), idOfB);
    QVERIFY(!asked.value(QStringLiteral("fallback")).toBool());
    QVERIFY(!DirectoryPage::TakePendingSubject());

    const QJsonObject gone = QJsonDocument::fromJson
        (Get(Settings(QStringLiteral("/api/directory?node=ffffffff")),
             VanillaPage::SettingsUrl()).m_Body).object();
    QVERIFY(gone.value(QStringLiteral("fallback")).toBool());

    DirectoryPage::SetPendingSubject(a);
    const QString idOfA = DirectoryPage::Describe(a).value(QStringLiteral("current")).toString();
    QCOMPARE(QJsonDocument::fromJson(Get(Settings(QStringLiteral("/api/directory")),
                                         VanillaPage::SettingsUrl()).m_Body)
             .object().value(QStringLiteral("current")).toString(), idOfA);
    QVERIFY(QJsonDocument::fromJson(Get(Settings(QStringLiteral("/api/directory")),
                                        VanillaPage::SettingsUrl()).m_Body)
            .object().value(QStringLiteral("current")).toString() != idOfA);
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
    QCOMPARE(Get(Settings(QStringLiteral("/api/passwords")),
                 QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/passwords-remove")), QByteArrayLiteral("{}"),
                  QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/passwords-remove-all")), QByteArrayLiteral("{}"),
                  QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/passwords-remove-all")), QByteArrayLiteral("{}"),
                  DirectoryPage::PageUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/downloads")),
                 QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/downloads-open")), QByteArrayLiteral("{\"id\":1}"),
                  QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/downloads-open")), QByteArrayLiteral("{\"id\":1}"),
                  DirectoryPage::PageUrl()).m_Status, 403);
    QCOMPARE(Get(Settings(QStringLiteral("/api/directory")),
                 QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/directory-set")), QByteArrayLiteral("{}"),
                  QUrl(QStringLiteral("https://example.com/"))).m_Status, 403);
    QCOMPARE(Post(Settings(QStringLiteral("/api/directory-set")), QByteArrayLiteral("{}"),
                  DirectoryPage::PageUrl()).m_Status, 403);

    QCOMPARE(Get(Settings(QStringLiteral("/api/schema"))).m_Status, 200);

    const QUrl stranger(QStringLiteral("https://example.com/"));
    const QList<QUrl> documents = QList<QUrl>()
        << Settings(QStringLiteral("/")) << Settings(QStringLiteral("/settings.js"))
        << Settings(QStringLiteral("/settings.css")) << DirectoryPageUrl()
        << QUrl(DirectoryPageUrl().toString() + QStringLiteral("directory.js"));
    for(const QUrl &document : documents){
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
    QVERIFY(!VanillaPage::ReadsBody(directorySet, post, directory));
    QVERIFY(VanillaPage::ReadsBody(Settings(QStringLiteral("/api/directory-set")), post, page));

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

void tst_vanillapage::theKindOfViewTheSettingsTabKeepsIsTheDirectorys(){
    const QUrl settings = VanillaPage::SettingsUrl();
    const QUrl ordinary(QStringLiteral("https://example.com/"));

#ifdef WEBENGINEVIEW
    foreach(const QString &name, QStringList()
            << QStringLiteral("w") << QStringLiteral("web") << QStringLiteral("WV")
            << QStringLiteral("gw") << QStringLiteral("graphicswebview")
            << QStringLiteral("we") << QStringLiteral("webengine") << QStringLiteral("WEV")
            << QStringLiteral("gwe") << QStringLiteral("graphicswebengineview")){
        QCOMPARE(TreeBank::ViewKindFor(settings, QStringList(name)), TreeBank::WebEngineKind);
        QCOMPARE(TreeBank::ViewKindFor(ordinary, QStringList(name)), TreeBank::WebEngineKind);
    }
    foreach(const QString &name, QStringList()
            << QStringLiteral("qw") << QStringLiteral("quickweb")
            << QStringLiteral("qwe") << QStringLiteral("quickwebengineview")){
        QCOMPARE(TreeBank::ViewKindFor(settings, QStringList(name)), TreeBank::QuickWebEngineKind);
        QCOMPARE(TreeBank::ViewKindFor(ordinary, QStringList(name)), TreeBank::QuickWebEngineKind);
    }
    QCOMPARE(TreeBank::ViewKindFor(settings, QStringList() << QStringLiteral("id") << QStringLiteral("qw") << QStringLiteral("we")),
             TreeBank::QuickWebEngineKind);
#endif
#if defined(NATIVEWEBVIEW) && defined(WEBENGINEVIEW)
    foreach(const QString &name, QStringList() << QStringLiteral("nwv") << QStringLiteral("quicknativewebview")){
        QCOMPARE(TreeBank::ViewKindFor(settings, QStringList(name)), TreeBank::WebEngineKind);
        QCOMPARE(TreeBank::ViewKindFor(ordinary, QStringList(name)), TreeBank::QuickNativeKind);
    }
#endif
#ifdef EDGEWEBVIEW
    foreach(const QString &name, QStringList()
            << QStringLiteral("e") << QStringLiteral("edge") << QStringLiteral("edgeweb")
            << QStringLiteral("edgewebview") << QStringLiteral("EWV")){
        QCOMPARE(TreeBank::ViewKindFor(settings, QStringList(name)), TreeBank::EdgeKind);
    }
#endif
    const TreeBank::ViewKind fallback = TreeBank::ViewKindFor(settings, QStringList());
    QVERIFY(fallback != TreeBank::NoView);
    QCOMPARE(TreeBank::ViewKindFor(ordinary, QStringList(QStringLiteral("id"))), fallback);
#if defined(EDGEWEBVIEW)
    QVERIFY(fallback == TreeBank::EdgeKind || fallback == TreeBank::WebEngineKind);
#elif defined(WEBENGINEVIEW)
    QCOMPARE(fallback, TreeBank::WebEngineKind);
#endif
    QCOMPARE(TreeBank::KindOf(nullptr), TreeBank::NoView);
}

void tst_vanillapage::thePageCarriesItsSchemaAheadOfItsScript(){
    const QByteArray page = QByteArrayLiteral(
        "<!DOCTYPE html>\n<html>\n  <head>\n    <meta charset=\"utf-8\">\n  </head>\n"
        "  <body>\n    <script src=\"/settings.js\"></script>\n  </body>\n</html>\n");
    const QByteArray schema = QByteArrayLiteral(
        "{\"items\":[{\"label\":\"</script><b>x\"},{\"hint\":\"<!-- y\"}]}");
    const QByteArray served = VanillaPage::WithEmbeddedSchema(page, schema);

    const QByteArray opener = QByteArrayLiteral("<script id=\"schema\" type=\"application/json\">");
    const int block  = served.indexOf(opener);
    const int script = served.indexOf("<script src=\"/settings.js\">");
    QVERIFY(block >= 0);
    QVERIFY(script >= 0);
    QVERIFY(block < script);

    const int open = block + opener.size();
    const int close = served.indexOf("</script>", open);
    QVERIFY(close >= 0);
    QVERIFY(close < script);
    const QByteArray body = served.mid(open, close - open);
    QVERIFY(!body.contains('<'));

    const QJsonDocument parsed = QJsonDocument::fromJson(body);
    QVERIFY(!parsed.isNull());
    QCOMPARE(parsed.object().value(QStringLiteral("items")).toArray().at(0).toObject()
             .value(QStringLiteral("label")).toString(), QStringLiteral("</script><b>x"));
    QCOMPARE(parsed.object().value(QStringLiteral("items")).toArray().at(1).toObject()
             .value(QStringLiteral("hint")).toString(), QStringLiteral("<!-- y"));

    const QByteArray bare = QByteArrayLiteral("<html><body></body></html>");
    QCOMPARE(VanillaPage::WithEmbeddedSchema(bare, schema), bare);
}

void tst_vanillapage::thePageCarriesItsSheetAndItsScriptInIt(){
    const QByteArray page = QByteArrayLiteral(
        "<html><head><link rel=\"stylesheet\" href=\"/settings.css\"></head>"
        "<body><script id=\"schema\" type=\"application/json\">{}</script>\n"
        "    <script src=\"/settings.js\"></script></body></html>");
    const QByteArray css = QByteArrayLiteral("body { color: red; }\n");
    const QByteArray js  = QByteArrayLiteral("const x = '<b>';\n");
    const QByteArray served = VanillaPage::WithInlineResources(page, css, js);

    QVERIFY(!served.contains("<link rel="));
    QVERIFY(!served.contains("<script src="));
    QVERIFY(served.contains("<style>\n" + css + "</style>"));
    QVERIFY(served.contains("<script>\n" + js + "</script>"));
    QVERIFY(served.indexOf("<script id=\"schema\"") < served.indexOf("<script>\n" + js));

    const QByteArray unsafe = QByteArrayLiteral("const s = '</script>';\n");
    const QByteArray partly = VanillaPage::WithInlineResources(page, css, unsafe);
    QVERIFY(partly.contains("<script src=\"/settings.js\"></script>"));
    QVERIFY(!partly.contains(unsafe));
    QVERIFY(partly.contains("<style>\n" + css + "</style>"));

    const QByteArray bare = QByteArrayLiteral("<html><body></body></html>");
    QCOMPARE(VanillaPage::WithInlineResources(bare, css, js), bare);
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
