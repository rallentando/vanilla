#include "switch.hpp"

#include <QtTest>
#include <QFile>
#include <QDir>
#include <QRegularExpression>

#include "settingsschema.hpp"

#include "testsupport.hpp"

class tst_settingsschema : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void everyKeyAppearsOnce();
    void everyItemIsInAKnownCategory();
    void everyChoiceDefaultIsOneOfTheChoices();
    void everyLabelIsPresent();
    void everyDependencyNamesABooleanInTheTable();

    void defaultsAgreeWithTheCodeThatReadsTheKey();
    void noKeyIsShadowedByAnotherOneTheCodeReadsFirst();

    void findOnlyKnowsTheKeysInTheTable();
    void everySettingReachesTheProfilesItIsMeantFor();

    void fromJsonRefusesTheWrongShape();
    void fromJsonRefusesAChoiceThatIsNotOffered();
    void fromJsonAcceptsWhatTheTypeWants();
    void toJsonSettlesTheSpellingOfAnOlderChoice();
    void toJsonSettlesAChoiceThatWasRenamed();
    void roundTripThroughJson();
};

void tst_settingsschema::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_settingsschema::everyKeyAppearsOnce(){
    QSet<QString> seen;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        const QString key = QString::fromLatin1(item.key);
        QVERIFY2(!seen.contains(key), qPrintable(QStringLiteral("duplicate key %1").arg(key)));
        seen.insert(key);
    }
    QVERIFY(!seen.isEmpty());
}

void tst_settingsschema::everyItemIsInAKnownCategory(){
    QSet<QString> categories;
    typedef QPair<QString, QString> Pair;
    foreach(const Pair &pair, SettingsSchema::Categories())
        categories.insert(pair.first);

    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        const QString category = QString::fromLatin1(item.category);
        QVERIFY2(categories.contains(category),
                 qPrintable(QStringLiteral("%1 is in the unknown category %2")
                            .arg(QString::fromLatin1(item.key), category)));
    }
}

void tst_settingsschema::everyChoiceDefaultIsOneOfTheChoices(){
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        if(item.type != SettingsSchema::Choice) continue;

        const QStringList choices =
            QString::fromLatin1(item.choices).split(QLatin1Char('|'));
        QVERIFY2(choices.length() > 1,
                 qPrintable(QStringLiteral("%1 offers no choice")
                            .arg(QString::fromLatin1(item.key))));
        QVERIFY2(choices.contains(QString::fromLatin1(item.fallback)),
                 qPrintable(QStringLiteral("%1 defaults to %2, which it does not offer")
                            .arg(QString::fromLatin1(item.key),
                                 QString::fromLatin1(item.fallback))));
    }

    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        if(item.type == SettingsSchema::Choice) continue;
        QCOMPARE(QString::fromLatin1(item.choices), QString());
    }
}

void tst_settingsschema::everyLabelIsPresent(){
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        QVERIFY2(item.label && item.label[0],
                 qPrintable(QStringLiteral("%1 has no label")
                            .arg(QString::fromLatin1(item.key))));
    }
}

void tst_settingsschema::everyDependencyNamesABooleanInTheTable(){
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        if(!item.appliesWhenKey || !item.appliesWhenKey[0]) continue;

        const QString key = QString::fromLatin1(item.key);
        const QString other = QString::fromLatin1(item.appliesWhenKey);
        QVERIFY2(other != key,
                 qPrintable(QStringLiteral("%1 depends on itself").arg(key)));

        const SettingsSchema::Item *control = SettingsSchema::Find(other);
        QVERIFY2(control,
                 qPrintable(QStringLiteral("%1 depends on %2, which is not in the table")
                            .arg(key, other)));
        QVERIFY2(control->type == SettingsSchema::Bool,
                 qPrintable(QStringLiteral("%1 depends on %2, which is not a boolean")
                            .arg(key, other)));
    }
}

namespace {

    QString TheSources(int *count){
        const QString root = QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR));

        QStringList sources;
        QDirIterator it(root, QStringList() << QStringLiteral("*.cpp") << QStringLiteral("*.hpp"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext()){
            const QString path = it.next();
            if(path.contains(QStringLiteral("/build/")) ||
               path.contains(QStringLiteral("/tests/"))) continue;
            sources << path;
        }
        *count = sources.length();

        QString all;
        foreach(const QString &path, sources){
            QFile file(path);
            if(file.open(QIODevice::ReadOnly))
                all += QString::fromUtf8(file.readAll());
        }
        return all;
    }

    QRegularExpression GuardedRead(const QString &key){
        return QRegularExpression(
            QStringLiteral("^[ \\t]*if\\s*\\(.*\\)\\s*[A-Za-z_][A-Za-z0-9_]*\\s*=[^=].*"
                           "value\\(QStringLiteral\\(\"%1\"\\)")
            .arg(QRegularExpression::escape(key)));
    }
}

void tst_settingsschema::defaultsAgreeWithTheCodeThatReadsTheKey(){
    int files = 0;
    const QString all = TheSources(&files);
    QVERIFY2(files > 20, "the sources were not found; check VANILLA_SOURCE_DIR");

    int checked = 0;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        const QString key = QString::fromLatin1(item.key);
        const QString fallback = QString::fromLatin1(item.fallback);

        QRegularExpression pattern(
            QStringLiteral("value\\(QStringLiteral\\(\"%1\"\\)\\s*,\\s*"
                           "(QStringLiteral\\(\"[^\"]*\"\\)"
                           "|QString\\(\\)|QStringList\\(\\)"
                           "|[^),]+)\\)")
            .arg(QRegularExpression::escape(key)));

        QRegularExpressionMatch match = pattern.match(all);
        if(!match.hasMatch()) continue;

        const QString expression = match.captured(1).trimmed();

        QString actual;
        const QRegularExpressionMatch literal =
            QRegularExpression(QStringLiteral("^QStringLiteral\\(\"(.*)\"\\)$")).match(expression);
        if(literal.hasMatch()){
            actual = literal.captured(1);
        } else if(expression == QStringLiteral("QString()") ||
                  expression == QStringLiteral("QStringList()")){
            actual = QString();
        } else if(expression == QStringLiteral("true") ||
                  expression == QStringLiteral("false")){
            actual = expression;
        } else if(QRegularExpression(QStringLiteral("^-?\\d+$")).match(expression).hasMatch()){
            actual = expression;
        } else {
            continue;
        }

        QVERIFY2(actual == fallback,
                 qPrintable(QStringLiteral("%1: the schema says \"%2\", the code reads \"%3\"")
                            .arg(key, fallback, actual)));
        checked++;
    }

    QVERIFY2(checked > 40, qPrintable(QStringLiteral("only %1 defaults were compared").arg(checked)));
}

void tst_settingsschema::noKeyIsShadowedByAnotherOneTheCodeReadsFirst(){
    int files = 0;
    const QString all = TheSources(&files);
    QVERIFY2(files > 20, "the sources were not found; check VANILLA_SOURCE_DIR");

    const QString shape =
        QStringLiteral("    if(m_Example == -1) m_Example = "
                       "s.value(QStringLiteral(\"application/@Example\"), -1).value<int>();");
    QVERIFY2(GuardedRead(QStringLiteral("application/@Example")).match(shape).hasMatch(),
             "the pattern stopped recognising a guarded read; see the comment above");

    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        const QString key = QString::fromLatin1(item.key);

        const QRegularExpression read(
            QStringLiteral("value\\(QStringLiteral\\(\"%1\"\\)")
            .arg(QRegularExpression::escape(key)));
        const QRegularExpression guarded = GuardedRead(key);

        bool any = false;
        bool onItsOwn = false;

        QRegularExpressionMatchIterator reads = read.globalMatch(all);
        while(reads.hasNext()){
            const int at = reads.next().capturedStart();
            const int from = all.lastIndexOf(QLatin1Char('\n'), at) + 1;
            int to = all.indexOf(QLatin1Char('\n'), at);
            if(to < 0) to = all.length();

            any = true;
            if(!guarded.match(all.mid(from, to - from)).hasMatch()) onItsOwn = true;
        }

        QVERIFY2(any,
                 qPrintable(QStringLiteral("the page offers %1, which nothing reads")
                            .arg(key)));
        QVERIFY2(onItsOwn,
                 qPrintable(QStringLiteral("the page offers %1, which is only read where "
                                           "another key came back empty; offer that one instead")
                            .arg(key)));
    }
}

void tst_settingsschema::findOnlyKnowsTheKeysInTheTable(){
    QVERIFY(SettingsSchema::Find(QStringLiteral("application/@ColorScheme")));

    QVERIFY(!SettingsSchema::Find(QString()));
    QVERIFY(!SettingsSchema::Find(QStringLiteral("application/UserAgent_IE")));
    QVERIFY(!SettingsSchema::Find(QStringLiteral("webview/keymap/Ctrl+W")));
    QVERIFY(!SettingsSchema::Find(QStringLiteral("mainwindow/geometry0")));
    QVERIFY(!SettingsSchema::Find(QStringLiteral("application/@ColorSchemeX")));
    QVERIFY(!SettingsSchema::Find(QStringLiteral("application/@ColorSchem")));

    QVERIFY2(!SettingsSchema::Find(QStringLiteral("webview/@EnableScrollGesture")),
             "the scroll gesture row is back on the page; see D-124 before "
             "keeping it, and update this test with what was decided.");
}

void tst_settingsschema::fromJsonRefusesTheWrongShape(){
    const SettingsSchema::Item *flag =
        SettingsSchema::Find(QStringLiteral("application/@EnableAutoSave"));
    const SettingsSchema::Item *number =
        SettingsSchema::Find(QStringLiteral("application/@AutoSaveInterval"));
    const SettingsSchema::Item *list =
        SettingsSchema::Find(QStringLiteral("application/@AllowedHosts"));
    QVERIFY(flag && number && list);

    QVERIFY(!SettingsSchema::FromJson(*flag, QJsonValue(QStringLiteral("true"))).isValid());
    QVERIFY(!SettingsSchema::FromJson(*flag, QJsonValue(1)).isValid());
    QVERIFY(!SettingsSchema::FromJson(*number, QJsonValue(QStringLiteral("60"))).isValid());
    QVERIFY(!SettingsSchema::FromJson(*number, QJsonValue(true)).isValid());
    QVERIFY(!SettingsSchema::FromJson(*list, QJsonValue(QStringLiteral("a"))).isValid());

    QJsonArray mixed;
    mixed.append(QStringLiteral("ok"));
    mixed.append(7);
    QVERIFY(!SettingsSchema::FromJson(*list, QJsonValue(mixed)).isValid());
}

void tst_settingsschema::fromJsonRefusesAChoiceThatIsNotOffered(){
    const SettingsSchema::Item *item =
        SettingsSchema::Find(QStringLiteral("application/@ColorScheme"));
    QVERIFY(item);

    QCOMPARE(SettingsSchema::FromJson(*item, QJsonValue(QStringLiteral("Dark"))).value<QString>(),
             QStringLiteral("Dark"));
    QVERIFY(!SettingsSchema::FromJson(*item, QJsonValue(QStringLiteral("purple"))).isValid());
    QVERIFY(!SettingsSchema::FromJson(*item, QJsonValue(QStringLiteral("dark"))).isValid());
    QVERIFY(!SettingsSchema::FromJson(*item, QJsonValue(QString())).isValid());
}

void tst_settingsschema::toJsonSettlesTheSpellingOfAnOlderChoice(){
    const SettingsSchema::Item *item =
        SettingsSchema::Find(QStringLiteral("application/@ColorScheme"));
    QVERIFY(item);

    QCOMPARE(SettingsSchema::ToJson(*item, QVariant(QStringLiteral("dark"))).toString(),
             QStringLiteral("Dark"));
    QCOMPARE(SettingsSchema::ToJson(*item, QVariant(QStringLiteral("auto"))).toString(),
             QStringLiteral("Auto"));
    QCOMPARE(SettingsSchema::ToJson(*item, QVariant(QStringLiteral("purple"))).toString(),
             QStringLiteral("purple"));
}

void tst_settingsschema::toJsonSettlesAChoiceThatWasRenamed(){
    const SettingsSchema::Item *item =
        SettingsSchema::Find(QStringLiteral("webview/@SuspendHiddenViews"));
    QVERIFY(item);
    QVERIFY(!QString::fromLatin1(item->choices)
                 .split(QLatin1Char('|')).contains(QStringLiteral("Off")));

    QCOMPARE(SettingsSchema::ToJson(*item, QVariant(QStringLiteral("Off"))).toString(),
             QStringLiteral("Active"));
    QCOMPARE(SettingsSchema::ToJson(*item, QVariant(QStringLiteral("Frozen"))).toString(),
             QStringLiteral("Frozen"));
    QCOMPARE(SettingsSchema::ToJson(*item, QVariant(QStringLiteral("Asleep"))).toString(),
             QStringLiteral("Asleep"));
    QVERIFY(!SettingsSchema::FromJson(*item, QJsonValue(QStringLiteral("Off"))).isValid());
}

void tst_settingsschema::fromJsonAcceptsWhatTheTypeWants(){
    const SettingsSchema::Item *flag =
        SettingsSchema::Find(QStringLiteral("application/@EnableAutoSave"));
    const SettingsSchema::Item *number =
        SettingsSchema::Find(QStringLiteral("application/@AutoSaveInterval"));
    const SettingsSchema::Item *list =
        SettingsSchema::Find(QStringLiteral("application/@AllowedHosts"));

    QCOMPARE(SettingsSchema::FromJson(*flag, QJsonValue(false)).value<bool>(), false);
    QCOMPARE(SettingsSchema::FromJson(*number, QJsonValue(60000)).value<int>(), 60000);

    QJsonArray hosts;
    hosts.append(QStringLiteral("example.com"));
    hosts.append(QStringLiteral("  "));
    hosts.append(QStringLiteral(" spaced.test "));
    const QStringList got =
        SettingsSchema::FromJson(*list, QJsonValue(hosts)).value<QStringList>();
    QCOMPARE(got, QStringList() << QStringLiteral("example.com")
                                << QStringLiteral("spaced.test"));
}

void tst_settingsschema::roundTripThroughJson(){
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        const QVariant fallback = SettingsSchema::Fallback(item);
        const QJsonValue json = SettingsSchema::ToJson(item, fallback);
        const QVariant back = SettingsSchema::FromJson(item, json);

        QVERIFY2(back.isValid(),
                 qPrintable(QStringLiteral("%1: its own default does not survive the trip")
                            .arg(QString::fromLatin1(item.key))));
        QCOMPARE(back.toString(), fallback.toString());
    }
}

void tst_settingsschema::everySettingReachesTheProfilesItIsMeantFor(){
    QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
               QStringLiteral("/app/networkcontroller.cpp"));
    QVERIFY2(file.open(QIODevice::ReadOnly),
             "'networkcontroller.cpp' was not read; check VANILLA_SOURCE_DIR");
    const QString source =
        QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));

    const QStringList setters = QStringList()
        << QStringLiteral("setPersistentCookiesPolicy")
        << QStringLiteral("setUrlRequestInterceptor")
        << QStringLiteral("setSpellCheckLanguages")
        << QStringLiteral("setSpellCheckEnabled")
        << QStringLiteral("setHttpCacheType")
        << QStringLiteral("setHttpCacheMaximumSize")
        << QStringLiteral("setPushServiceEnabled")
        << QStringLiteral("setPersistentPermissionsPolicy");

    foreach(const QString &setter, setters){
        QVERIFY2(source.contains(QStringLiteral("m_Profile->") + setter),
                 qPrintable(QStringLiteral("the widget profile is no longer told '%1'")
                            .arg(setter)));
        QVERIFY2(source.contains(QStringLiteral("profile->") + setter),
                 qPrintable(QStringLiteral("the quick profile is no longer told '%1'")
                            .arg(setter)));
    }

    const int at = source.indexOf
        (QStringLiteral("NetworkController::QuickPrivateProfile"));
    QVERIFY2(at > 0, "'QuickPrivateProfile' was not found");
    const int end = source.indexOf(QStringLiteral("\n}\n"), at);
    QVERIFY2(end > at, "the end of 'QuickPrivateProfile' was not found");
    const QString privateBody = source.mid(at, end - at);

    const QStringList helpers = QStringList()
        << QStringLiteral("ApplyQuickPermissionsPolicy(profile)")
        << QStringLiteral("ApplyQuickBlockRules(profile)")
        << QStringLiteral("ApplyQuickCommonSettings(profile)");
    foreach(const QString &helper, helpers){
        QCOMPARE(source.count(helper), 2);
        QVERIFY2(privateBody.contains(helper),
                 qPrintable(QStringLiteral("the private quick profile no longer calls '%1'")
                            .arg(helper)));
    }

    const int common = source.indexOf
        (QStringLiteral("NetworkController::ApplyQuickCommonSettings"));
    QVERIFY2(common > 0, "'ApplyQuickCommonSettings' was not found");
    const int commonEnd = source.indexOf(QStringLiteral("\n}\n"), common);
    QVERIFY2(commonEnd > common, "the end of 'ApplyQuickCommonSettings' was not found");
    const QString commonBody = source.mid(common, commonEnd - common);

    const QStringList everyProfile = QStringList()
        << QStringLiteral("setSpellCheckLanguages")
        << QStringLiteral("setSpellCheckEnabled")
        << QStringLiteral("setPushServiceEnabled");
    foreach(const QString &setter, everyProfile){
        QVERIFY2(commonBody.contains(setter),
                 qPrintable(QStringLiteral("'%1' is no longer in the helper both "
                                           "quick profiles call").arg(setter)));
    }

    const QStringList diskOnly = QStringList()
        << QStringLiteral("setPersistentCookiesPolicy")
        << QStringLiteral("setHttpCacheType")
        << QStringLiteral("setHttpCacheMaximumSize");
    foreach(const QString &setter, diskOnly){
        QVERIFY2(!privateBody.contains(setter),
                 qPrintable(QStringLiteral("the private quick profile is told '%1', "
                                           "which is about disk").arg(setter)));
    }
}

QTEST_MAIN(tst_settingsschema)
#include "tst_settingsschema.moc"
