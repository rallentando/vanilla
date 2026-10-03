#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QRegularExpression>

#include "directorypage.hpp"
#include "view.hpp"
#include <QJsonArray>
#include <QScopeGuard>
#ifdef WEBENGINEVIEW
#  include <QWebEngineProfile>
#  include <QWebEngineSettings>
#  include <QtWebEngineQuick/qtwebenginequickglobal.h>
#endif

using namespace DirectoryPage;

namespace {
    const Token &TokenOf(const char *key){
        foreach(const Token &token, Tokens()){
            if(qstrcmp(token.key, key) == 0) return token;
        }
        Q_ASSERT(false);
        return Tokens().first();
    }
}

class tst_directorypage : public QObject {
    Q_OBJECT

private slots:
    void idIsTheFirstToken();
    void everyDefaultIsResolved();
    void mouseGesturesComeBeforeSuperDrag();

    void takesATitleApart();
    void takesATitleApart_data();

    void composesWhatItTookApart();
    void aRenameOfTheNameAloneKeepsTheTokens();

    void readsTheStateOfAToken();
    void readsTheStateOfAToken_data();

    void theNegationWinsWhenBothArePresent();
    void theAutoLoadRowIsTurnedRound();
    void onlyTheProfileRowIsAboutItsOwnDirectory();

    void writesATokenWithoutDisturbingTheOthers();
    void togglingOffWritesTheNegationWhenThereIsOne();
    void togglingOffRemovesTheTokenWhenThereIsNot();
    void aHandTypedSpellingIsRecognizedAndCanonicalized();

    void acceptsATitleTheRenameDialogWouldAccept();
    void acceptsATitleTheRenameDialogWouldAccept_data();

    void saysWhereAProfileBegins();
    void saysWhereAProfileBegins_data();
    void namesWrittenInTheIdSpelling();

    void everyTokenExplainsItself();

    void namesWhatATokenIsAbout();
    void namesWhatATokenIsAbout_data();

    void inheritsSettingBySetting();
    void inheritsSettingBySetting_data();

    void aDirectoryKeepsBothWordsOfItsOwn();

    void readsOneSettingOutOfTheWholeSet();
    void readsOneSettingOutOfTheWholeSet_data();

    void saysWhichSettingsAnEditChanged();
    void saysWhichSettingsAnEditChanged_data();

    void takesOutOnlyWhatWouldStopTheChange();
    void takesOutOnlyWhatWouldStopTheChange_data();
};

class DirectoryDefaultsHarness : public View {
public:
    static void SetGestures(bool right, bool drag){
        m_EnableMouseGesture = right;
        m_EnableDragGesture = drag;
    }
};

void tst_directorypage::everyDefaultIsResolved(){
    const bool oldRight = View::EnableRightGesture();
    const bool oldDrag = View::EnableDragGesture();
    const auto restoreGestures = qScopeGuard([=](){
        DirectoryDefaultsHarness::SetGestures(oldRight, oldDrag);
    });
#ifdef WEBENGINEVIEW
    auto *settings = QWebEngineProfile::defaultProfile()->settings();
    const bool oldImage = settings->testAttribute(QWebEngineSettings::AutoLoadImages);
    const bool oldScript = settings->testAttribute(QWebEngineSettings::JavascriptEnabled);
    const bool oldPlugins = settings->testAttribute(QWebEngineSettings::PluginsEnabled);
    const auto restoreAttributes = qScopeGuard([=](){
        settings->setAttribute(QWebEngineSettings::AutoLoadImages, oldImage);
        settings->setAttribute(QWebEngineSettings::JavascriptEnabled, oldScript);
        settings->setAttribute(QWebEngineSettings::PluginsEnabled, oldPlugins);
    });
#endif
    for(bool enabled : {false, true}){
        DirectoryDefaultsHarness::SetGestures(enabled, !enabled);
#ifdef WEBENGINEVIEW
        settings->setAttribute(QWebEngineSettings::AutoLoadImages, enabled);
        settings->setAttribute(QWebEngineSettings::JavascriptEnabled, !enabled);
        settings->setAttribute(QWebEngineSettings::PluginsEnabled, enabled);
#endif
        QMap<QString, int> defaults;
        for(const QJsonValue &value : Describe().value("switches").toArray()){
            const auto object = value.toObject();
            const int absence = object.value("absence").toInt(-1);
            QVERIFY2(absence == 0 || absence == 1, qPrintable(object.value("key").toString()));
            defaults[object.value("key").toString()] = absence;
        }
        QCOMPARE(defaults.value("private", -1), 0);
        QCOMPARE(defaults.value("autoload", -1), 0);
        QCOMPARE(defaults.value("rightgesture", -1), int(enabled));
        QCOMPARE(defaults.value("draggesture", -1), int(!enabled));
#ifdef WEBENGINEVIEW
        QCOMPARE(defaults.value("image", -1), int(enabled));
        QCOMPARE(defaults.value("javascript", -1), int(!enabled));
        QCOMPARE(defaults.value("plugins", -1), int(enabled));
#endif
    }
}

void tst_directorypage::idIsTheFirstToken(){
    QCOMPARE(QString::fromLatin1(Tokens().first().key), QStringLiteral("id"));
}

void tst_directorypage::mouseGesturesComeBeforeSuperDrag(){
    QStringList keys;
    foreach(const Token &token, Tokens())
        keys << QString::fromLatin1(token.key);

    const int mouse = keys.indexOf(QStringLiteral("rightgesture"));
    const int drag = keys.indexOf(QStringLiteral("draggesture"));
    QVERIFY(mouse != -1);
    QVERIFY(drag != -1);
    QVERIFY(mouse < drag);

    const Token &token = TokenOf("rightgesture");
    QCOMPARE(TokenState(QStringLiteral("work;RightGesture"), token), 1);
    QCOMPARE(TokenState(QStringLiteral("work;!RightGesture"), token), 0);
    QCOMPARE(TokenState(QStringLiteral("work;MouseGesture"), token), 1);
    QCOMPARE(WithTokenState(QStringLiteral("work;MouseGesture"), token, 0),
             QStringLiteral("work;!RightGesture"));

    const QString pattern = QString::fromLatin1(token.pattern);
    const QRegularExpression on(QStringLiteral("^") + pattern + QStringLiteral("$"));
    const QRegularExpression off(QStringLiteral("^!") + pattern + QStringLiteral("$"));
    QVERIFY(on.match(QStringLiteral("MouseGesture")).hasMatch());
    QVERIFY(!off.match(QStringLiteral("MouseGesture")).hasMatch());
    QVERIFY(off.match(QStringLiteral("!MouseGesture")).hasMatch());
}

void tst_directorypage::takesATitleApart_data(){
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QStringList>("tokens");

    QTest::newRow("bare name")
        << "work" << "work" << QStringList();
    QTest::newRow("one token")
        << "work;ID" << "work" << (QStringList() << "ID");
    QTest::newRow("several tokens")
        << "work;ID;Private;!Javascript" << "work"
        << (QStringList() << "ID" << "Private" << "!Javascript");
    QTest::newRow("a token with a space in it")
        << "work;Encoding EUC-JP" << "work"
        << (QStringList() << "Encoding EUC-JP");
    QTest::newRow("an empty run of separators is not a token")
        << "work;;ID" << "work" << (QStringList() << "ID");
    QTest::newRow("the trash root's title")
        << "trash;noload" << "trash" << (QStringList() << "noload");
    QTest::newRow("empty")
        << "" << "" << QStringList();
}

void tst_directorypage::takesATitleApart(){
    QFETCH(QString, title);
    QFETCH(QString, name);
    QFETCH(QStringList, tokens);

    QCOMPARE(TitleName(title), name);
    QCOMPARE(TitleTokens(title), tokens);
}

void tst_directorypage::composesWhatItTookApart(){
    const QString title = QStringLiteral("work;ID;Private;Encoding EUC-JP");
    QCOMPARE(ComposeTitle(TitleName(title), TitleTokens(title)), title);
    QCOMPARE(ComposeTitle(QStringLiteral("work"), QStringList()),
             QStringLiteral("work"));
}

void tst_directorypage::aRenameOfTheNameAloneKeepsTheTokens(){
    const QString before = QStringLiteral("work;ID;!Javascript;Encoding EUC-JP");

    QCOMPARE(TitleName(before), QStringLiteral("work"));
    QCOMPARE(ComposeTitle(QStringLiteral("play"), TitleTokens(before)),
             QStringLiteral("play;ID;!Javascript;Encoding EUC-JP"));
    QCOMPARE(ComposeTitle(QStringLiteral("play"),
                          TitleTokens(QStringLiteral("work"))),
             QStringLiteral("play"));
    QVERIFY(IsValidTitle(ComposeTitle(TitleName(before), TitleTokens(before))));
}

void tst_directorypage::readsTheStateOfAToken_data(){
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("key");
    QTest::addColumn<int>("state");

    QTest::newRow("absent")            << "work"             << "id"         << -1;
    QTest::newRow("canonical")         << "work;ID"          << "id"         <<  1;
    QTest::newRow("long spelling")     << "work;Identifier"  << "id"         <<  1;
    QTest::newRow("lower case")        << "work;id"          << "id"         <<  1;
    QTest::newRow("offtherecord")      << "work;OffTheRecord"<< "private"    <<  1;
    QTest::newRow("negated private")   << "work;!Private"    << "private"    <<  0;
    QTest::newRow("negated")           << "work;!Javascript" << "javascript" <<  0;
    QTest::newRow("short js")          << "work;js"          << "javascript" <<  1;
    QTest::newRow("right gesture")     << "work;RightGesture" << "rightgesture" << 1;
    QTest::newRow("mouse gesture spelling")
        << "work;!MouseGesture" << "rightgesture" << 0;
    QTest::newRow("the id is not an image")
        << "work;ID"    << "image" << -1;
    QTest::newRow("the image is not an id")
        << "work;Image" << "id"    << -1;
    QTest::newRow("noload, as the trash spells it")
        << "trash;noload" << "autoload" << 0;
    QTest::newRow("the negation is auto load on")
        << "work;!noload" << "autoload" << 1;
}

void tst_directorypage::readsTheStateOfAToken(){
    QFETCH(QString, title);
    QFETCH(QString, key);
    QFETCH(int, state);

    QCOMPARE(TokenState(title, TokenOf(key.toLatin1().constData())), state);
}

void tst_directorypage::theNegationWinsWhenBothArePresent(){
    QCOMPARE(TokenState(QStringLiteral("work;Javascript;!Javascript"),
                        TokenOf("javascript")), 0);
    QCOMPARE(TokenState(QStringLiteral("work;!Javascript;Javascript"),
                        TokenOf("javascript")), 0);
}

void tst_directorypage::theAutoLoadRowIsTurnedRound(){
    const Token &autoload = TokenOf("autoload");

    QCOMPARE(WithTokenState(QStringLiteral("work"), autoload, 1),
             QStringLiteral("work;!NoAutoLoad"));
    QCOMPARE(WithTokenState(QStringLiteral("work"), autoload, 0),
             QStringLiteral("work;NoAutoLoad"));

    QCOMPARE(TokenState(QStringLiteral("work;!NoAutoLoad"), autoload),  1);
    QCOMPARE(TokenState(QStringLiteral("work;NoAutoLoad"),  autoload),  0);
    QCOMPARE(TokenState(QStringLiteral("work"),             autoload), -1);

    QCOMPARE(StateIn(TitleTokens(QStringLiteral("work;NoAutoLoad")),
                     QLatin1String(autoload.pattern)), 1);
    QCOMPARE(StateIn(TitleTokens(QStringLiteral("work;!NoAutoLoad")),
                     QLatin1String(autoload.pattern)), 0);

    QCOMPARE(autoload.absence, 0);
    QVERIFY(autoload.inverted);
    foreach(const Token &token, Tokens()){
        if(qstrcmp(token.key, "autoload") == 0) continue;
        QCOMPARE(token.absence, -1);
        QVERIFY(!token.inverted);
    }
}

void tst_directorypage::onlyTheProfileRowIsAboutItsOwnDirectory(){
    QVERIFY(TokenOf("id").ownOnly);
    foreach(const Token &token, Tokens()){
        if(qstrcmp(token.key, "id") == 0) continue;
        QVERIFY2(!token.ownOnly, QByteArray("not own only: ") + token.key);
    }
}

void tst_directorypage::writesATokenWithoutDisturbingTheOthers(){
    const QString before = QStringLiteral("work;Private;Encoding EUC-JP");
    const QString after = WithTokenState(before, TokenOf("id"), 1);
    QCOMPARE(after, QStringLiteral("work;Private;Encoding EUC-JP;ID"));
    QCOMPARE(WithTokenState(after, TokenOf("id"), 0), before);
}

void tst_directorypage::togglingOffWritesTheNegationWhenThereIsOne(){
    QCOMPARE(WithTokenState(QStringLiteral("work;Javascript"),
                            TokenOf("javascript"), 0),
             QStringLiteral("work;!Javascript"));
    QCOMPARE(WithTokenState(QStringLiteral("work;Private"),
                            TokenOf("private"), 0),
             QStringLiteral("work;!Private"));
}

void tst_directorypage::togglingOffRemovesTheTokenWhenThereIsNot(){
    QCOMPARE(WithTokenState(QStringLiteral("work;ID"), TokenOf("id"), 0),
             QStringLiteral("work"));
}

void tst_directorypage::aHandTypedSpellingIsRecognizedAndCanonicalized(){
    const QString before = QStringLiteral("work;identification");
    QCOMPARE(TokenState(before, TokenOf("id")), 1);
    QCOMPARE(WithTokenState(before, TokenOf("id"), 1),
             QStringLiteral("work;ID"));
}

void tst_directorypage::acceptsATitleTheRenameDialogWouldAccept_data(){
    QTest::addColumn<QString>("title");
    QTest::addColumn<bool>("valid");

    QTest::newRow("plain")            << "work"        << true;
    QTest::newRow("with tokens")      << "work;ID"     << true;
    QTest::newRow("empty")            << ""            << true;
    QTest::newRow("tokens without a name")  << ";edge" << true;
    QTest::newRow("a nameless negated id")  << ";!ID"  << true;
    QTest::newRow("a nameless id")          << ";ID"        << false;
    QTest::newRow("a nameless id, spelled long") << ";Identify" << false;
    QTest::newRow("a nameless contradiction")    << ";ID;!ID"  << false;
    QTest::newRow("a slash")          << "a/b"         << false;
    QTest::newRow("a backslash")      << "a\\b"        << false;
    QTest::newRow("a question mark")  << "a?"          << false;
    QTest::newRow("an asterisk")      << "a*"          << false;
    QTest::newRow("angle brackets")   << "<a>"         << false;
    QTest::newRow("a quote")          << "a\"b"        << false;
    QTest::newRow("a colon")          << "a:b"         << false;
    QTest::newRow("a pipe")           << "a|b"         << false;
}

void tst_directorypage::acceptsATitleTheRenameDialogWouldAccept(){
    QFETCH(QString, title);
    QFETCH(bool, valid);

    QCOMPARE(IsValidTitle(title), valid);
}

void tst_directorypage::saysWhereAProfileBegins_data(){
    QTest::addColumn<QString>("title");
    QTest::addColumn<bool>("says");

    QTest::newRow("empty")                 << ""                     << false;
    QTest::newRow("a bare name")           << "work"                 << false;
    QTest::newRow("nameless tokens")       << ";edge"                << false;
    QTest::newRow("the canonical word")    << "work;ID"              << true;
    QTest::newRow("a hand-typed spelling") << "work;identification"  << true;
    QTest::newRow("nameless id")           << ";ID"                  << true;
    QTest::newRow("negated only")          << "work;!ID"             << false;
    QTest::newRow("the contradiction is still a boundary")
        << "work;ID;!ID" << true;
    QTest::newRow("a value with a colon is not an id")
        << "work;Proxy 127.0.0.1:8080" << false;
    QTest::newRow("a directory merely named id") << "id" << false;
}

void tst_directorypage::saysWhereAProfileBegins(){
    QFETCH(QString, title);
    QFETCH(bool, says);

    QCOMPARE(SaysId(title), says);
}

void tst_directorypage::namesWrittenInTheIdSpelling(){
    QVERIFY(NameSpellsId(QStringLiteral("id")));
    QVERIFY(NameSpellsId(QStringLiteral("ID")));
    QVERIFY(NameSpellsId(QStringLiteral("Identify")));
    QVERIFY(NameSpellsId(QStringLiteral("identification")));
    QVERIFY(!NameSpellsId(QStringLiteral("work")));
    QVERIFY(!NameSpellsId(QStringLiteral("ids")));
    QVERIFY(!NameSpellsId(QString()));
}

void tst_directorypage::everyTokenExplainsItself(){
    foreach(const Token &token, Tokens()){
        QVERIFY2(token.label && token.label[0],
                 QByteArray("no label: ") + token.key);
        QVERIFY2(token.hint && token.hint[0],
                 QByteArray("no hint: ") + token.key);
    }
}

void tst_directorypage::namesWhatATokenIsAbout_data(){
    QTest::addColumn<QString>("token");
    QTest::addColumn<QString>("subject");

    QTest::newRow("canonical")     << "Javascript"  << "javascript";
    QTest::newRow("short")         << "js"          << "javascript";
    QTest::newRow("negated")       << "!JavaScript" << "javascript";
    QTest::newRow("the other name")<< "OffTheRecord"<< "private";
    QTest::newRow("noload short")  << "nl"          << "autoload";
    QTest::newRow("right gesture") << "RightGesture" << "rightgesture";
    QTest::newRow("mouse gesture spelling")
        << "!MouseGesture" << "rightgesture";

    QTest::newRow("a proxy")       << "Proxy 127.0.0.1:8080" << "proxy";
    QTest::newRow("another proxy") << "Proxy example:1"      << "proxy";
    QTest::newRow("a user agent")  << "UserAgent Chrome"     << "useragent";
    QTest::newRow("an encoding")   << "Encoding EUC-JP"      << "encoding";

    QTest::newRow("a view kind")       << "WebEngine" << "viewkind";
    QTest::newRow("another view kind") << "Local"     << "viewkind";
    QTest::newRow("a short view kind") << "nwv"       << "viewkind";
    QTest::newRow("the edge view kind") << "Edge"        << "viewkind";
    QTest::newRow("its long spelling")  << "EdgeWebView" << "viewkind";
    QTest::newRow("the shortest edge")  << "e"           << "viewkind";
    QTest::newRow("a short edge")       << "ev"          << "viewkind";

    QTest::newRow("webgl")         << "WebGL"     << "webgl";
    QTest::newRow("inspector")     << "Inspector" << "inspector";

    QTest::newRow("unknown")       << "Frobnicate" << "frobnicate";
    QTest::newRow("unknown negated") << "!Frobnicate" << "frobnicate";
}

void tst_directorypage::namesWhatATokenIsAbout(){
    QFETCH(QString, token);
    QFETCH(QString, subject);

    QCOMPARE(TokenSubject(token), subject);
}

void tst_directorypage::inheritsSettingBySetting_data(){
    QTest::addColumn<QStringList>("titles");
    QTest::addColumn<QStringList>("tokens");

    QTest::newRow("nothing said anywhere")
        << (QStringList() << "inner" << "outer")
        << QStringList();
    QTest::newRow("only the far one speaks")
        << (QStringList() << "inner" << "outer;!NoAutoLoad")
        << (QStringList() << "!NoAutoLoad");
    QTest::newRow("the near one has the last word on what it mentions")
        << (QStringList() << "inner;NoAutoLoad" << "outer;!NoAutoLoad")
        << (QStringList() << "NoAutoLoad");
    QTest::newRow("a word about something else costs nothing")
        << (QStringList() << "inner;Private" << "outer;!NoAutoLoad")
        << (QStringList() << "Private" << "!NoAutoLoad");
    QTest::newRow("spelling variations still shadow")
        << (QStringList() << "inner;!js" << "outer;Javascript")
        << (QStringList() << "!js");
    QTest::newRow("one kind of view shadows another")
        << (QStringList() << "inner;Local" << "outer;WebEngine")
        << (QStringList() << "Local");
    QTest::newRow("the edge view shadows another kind")
        << (QStringList() << "inner;Edge" << "outer;WebEngine")
        << (QStringList() << "Edge");
    QTest::newRow("the shortest edge still shadows")
        << (QStringList() << "inner;e" << "outer;WebEngine")
        << (QStringList() << "e");
    QTest::newRow("three deep, each about its own")
        << (QStringList() << "a;Private" << "b;!Image" << "c;ID")
        << (QStringList() << "Private" << "!Image" << "ID");
}

void tst_directorypage::inheritsSettingBySetting(){
    QFETCH(QStringList, titles);
    QFETCH(QStringList, tokens);

    QCOMPARE(InheritTokens(titles), tokens);
}

void tst_directorypage::aDirectoryKeepsBothWordsOfItsOwn(){
    const QStringList titles = QStringList()
        << "inner;Javascript;!Javascript" << "outer;js";
    QCOMPARE(InheritTokens(titles),
             QStringList() << "Javascript" << "!Javascript");
}

void tst_directorypage::readsOneSettingOutOfTheWholeSet_data(){
    QTest::addColumn<QStringList>("set");
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<int>("state");

    const QString js = QStringLiteral("[jJ](?:ava)?[sS](?:cript)?");

    QTest::newRow("not mentioned")
        << (QStringList() << "Private") << js << -1;
    QTest::newRow("in force")
        << (QStringList() << "Javascript") << js << 1;
    QTest::newRow("turned off")
        << (QStringList() << "!js") << js << 0;
    QTest::newRow("the negation wins")
        << (QStringList() << "!Javascript" << "Javascript") << js << 0;
    QTest::newRow("and in the other order too")
        << (QStringList() << "Javascript" << "!Javascript") << js << 0;
    QTest::newRow("words about other things are stepped over")
        << (QStringList() << "Private" << "!Image" << "js") << js << 1;
    QTest::newRow("either spelling answers")
        << (QStringList() << "!DragHack")
        << QStringLiteral("[dD](?:rag)?[hH](?:ack)?|[dD](?:rag)?[gG](?:esture)?")
        << 0;
}

void tst_directorypage::readsOneSettingOutOfTheWholeSet(){
    QFETCH(QStringList, set);
    QFETCH(QString, pattern);
    QFETCH(int, state);

    QCOMPARE(StateIn(set, pattern), state);
}

void tst_directorypage::saysWhichSettingsAnEditChanged_data(){
    QTest::addColumn<QString>("before");
    QTest::addColumn<QString>("after");
    QTest::addColumn<QStringList>("changed");

    QTest::newRow("nothing changed")
        << "work;Private" << "work;Private" << QStringList();
    QTest::newRow("only the name changed")
        << "work;Private" << "home;Private" << QStringList();
    QTest::newRow("a setting turned on")
        << "work" << "work;!Javascript" << (QStringList() << "!Javascript");
    QTest::newRow("a setting turned round")
        << "work;Javascript" << "work;!Javascript"
        << (QStringList() << "!Javascript");
    QTest::newRow("a setting let go")
        << "work;!Javascript" << "work" << QStringList();
    QTest::newRow("one changed, one left alone")
        << "work;Private;Javascript" << "work;Private;!Javascript"
        << (QStringList() << "!Javascript");
    QTest::newRow("respelled")
        << "work;js" << "work;Javascript" << QStringList();
    QTest::newRow("a value changed")
        << "work;Proxy a:1" << "work;Proxy b:2"
        << (QStringList() << "Proxy b:2");
}

void tst_directorypage::saysWhichSettingsAnEditChanged(){
    QFETCH(QString, before);
    QFETCH(QString, after);
    QFETCH(QStringList, changed);

    QCOMPARE(ChangedWords(before, after), changed);
}

void tst_directorypage::takesOutOnlyWhatWouldStopTheChange_data(){
    QTest::addColumn<QString>("title");
    QTest::addColumn<QStringList>("changed");
    QTest::addColumn<QString>("after");

    const QStringList jsOff = QStringList() << "!Javascript";

    QTest::newRow("says nothing about it")
        << "inner" << jsOff << "inner";
    QTest::newRow("says something else entirely")
        << "inner;Private" << jsOff << "inner;Private";
    QTest::newRow("says the same thing")
        << "inner;!Javascript" << jsOff << "inner;!Javascript";
    QTest::newRow("says it in another spelling")
        << "inner;!js" << jsOff << "inner;!js";

    QTest::newRow("says the opposite")
        << "inner;Javascript" << jsOff << "inner";
    QTest::newRow("keeps everything else it says")
        << "inner;Private;Javascript;Image" << jsOff << "inner;Private;Image";
    QTest::newRow("another word for a setting answered by a word")
        << "inner;Local" << (QStringList() << "WebEngine") << "inner";
}

void tst_directorypage::takesOutOnlyWhatWouldStopTheChange(){
    QFETCH(QString, title);
    QFETCH(QStringList, changed);
    QFETCH(QString, after);

    QCOMPARE(TitleFollowing(title, changed), after);
}

#ifdef WEBENGINEVIEW
int main(int argc, char **argv){
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    tst_directorypage test;
    return QTest::qExec(&test, argc, argv);
}
#else
QTEST_MAIN(tst_directorypage)
#endif
#include "tst_directorypage.moc"
