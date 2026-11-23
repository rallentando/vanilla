#include "switch.hpp"

#include <QtTest>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QSet>
#include <QJsonArray>
#include <QRegularExpression>

#include "settingsschema.hpp"
#include "useragent.hpp"
#include "graphicstableview.hpp"
#include "application.hpp"

#include "testsupport.hpp"

class tst_settingsschema : public QObject {
    Q_OBJECT

private slots:
    void retiredCollectionFallsBackOnlyOnRead();
    void initTestCase();

    void everyKeyAppearsOnce();
    void everyItemIsInAKnownCategory();
    void everyCategoryHasARowAndTheTableKeepsTheirOrder();
    void theOnlyHiddenCategoryIsTheOtherSettings();
    void theRunsOfCategoriesBeginWhereTheListSays();
    void everyGroupStartsARowOnceAndInOrder();
    void everyChoiceDefaultIsOneOfTheChoices();
    void everyChoiceHasALabelAndEveryLabelNamesAChoice();
    void everyLabelIsPresent();
    void everyDependencyNamesARowAndAValueItOffers();
    void aDependencyIsDescribedInTheShapeOfTheRowItNames();
    void chromeExtensionsOfferADirectoryPicker();
    void theDownloadFoldersOfferAPickerAndWhatEmptyComesTo();

    void defaultsAgreeWithTheCodeThatReadsTheKey();
    void noKeyIsShadowedByAnotherOneTheCodeReadsFirst();
    void everyGraphicsApiChoiceMeansSomethingToQt();
    void theWidgetsCompositeOnTheGraphicsApiOrNotAtAll();
    void theMainWindowIsAnRhiWindowOnlyWhenAskedFor();
    void chromiumSwitchesKeepQuotedSpaces();

    void findOnlyKnowsTheKeysInTheTable();
    void theUserAgentRowsAreTheTemplates();
    void everySettingReachesTheProfilesItIsMeantFor();
    void everyPreferenceRowIsAppliedByTheView();
    void everyAttributeTheViewAppliesReachesThePage();
    void aTabListItemMadeWithTheWindowAsksForARestart();
    void unknownKeysAreDroppedAndKnownOnesKept();
    void aValueTheSettingDoesNotTakeIsDroppedOrPutRight();
    void fromTextTakesOnlyWhatTheSettingCanBe();
    void everyKeyTheSourcesSpellIsKnown();

    void fromJsonRefusesTheWrongShape();
    void fromJsonRefusesAChoiceThatIsNotOffered();
    void fromJsonAcceptsWhatTheTypeWants();
    void theGestureChoiceIsTheNumberTheViewReads();
    void toJsonSettlesTheSpellingOfAnOlderChoice();
    void toJsonSettlesAChoiceThatWasRenamed();
    void roundTripThroughJson();
};

void tst_settingsschema::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_settingsschema::retiredCollectionFallsBackOnlyOnRead(){
    const auto *item = SettingsSchema::Find(QStringLiteral("gadgets/thumblist/@NodeCollectionType"));
    QVERIFY(item);
    QCOMPARE(QString::fromLatin1(item->choices), QStringLiteral("Flat|Recursive|Foldable"));
    for(const QString &value : { QStringLiteral("Straight"), QStringLiteral("invalid") }){
        QCOMPARE(GraphicsTableView::NodeCollectionTypeFromName(value), GraphicsTableView::Flat);
        QCOMPARE(SettingsSchema::ToJson(*item, value).toString(), QStringLiteral("Flat"));
        QVERIFY(!SettingsSchema::FromJson(*item, QJsonValue(value)).isValid());
    }
    for(const QString &value : { QStringLiteral("Flat"), QStringLiteral("Recursive"),
                                QStringLiteral("Foldable") }){
        QCOMPARE(SettingsSchema::ToJson(*item, value).toString(), value);
        QCOMPARE(SettingsSchema::FromJson(*item, QJsonValue(value)).toString(), value);
    }
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

void tst_settingsschema::everyCategoryHasARowAndTheTableKeepsTheirOrder(){
    QStringList order;
    typedef QPair<QString, QString> Pair;
    foreach(const Pair &pair, SettingsSchema::Categories())
        order << pair.first;

    QStringList seen;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        const QString category = QString::fromLatin1(item.category);
        if(seen.isEmpty() || seen.last() != category){
            QVERIFY2(!seen.contains(category),
                     qPrintable(QStringLiteral("%1 is apart from the rest of %2")
                                .arg(QString::fromLatin1(item.key), category)));
            seen << category;
        }
    }
    QStringList expected = order;
    expected.removeOne(QStringLiteral("passwords"));
    expected.removeOne(QStringLiteral("permissions"));
    expected.removeOne(QStringLiteral("profiles"));
    expected.removeOne(QStringLiteral("directory"));
    expected.removeOne(QStringLiteral("menu"));
    QCOMPARE(seen, expected);
}

void tst_settingsschema::theOnlyHiddenCategoryIsTheOtherSettings(){
    const QJsonArray categories =
        SettingsSchema::Describe()[QStringLiteral("categories")].toArray();
    QStringList hidden;
    QStringList all;
    for(const QJsonValue &value : categories){
        const QJsonObject category = value.toObject();
        all << category[QStringLiteral("name")].toString();
        if(category[QStringLiteral("hidden")].toBool()) hidden << all.last();
    }
    QCOMPARE(hidden, QStringList() << QStringLiteral("hidden"));
    QCOMPARE(all.indexOf(QStringLiteral("menu")), all.indexOf(QStringLiteral("input")) + 1);
    QCOMPARE(all.last(), QStringLiteral("hidden"));
}

void tst_settingsschema::theRunsOfCategoriesBeginWhereTheListSays(){
    const QJsonArray categories =
        SettingsSchema::Describe()[QStringLiteral("categories")].toArray();
    QStringList marked;
    for(int i = 0; i < categories.size(); i++){
        const QJsonObject category = categories[i].toObject();
        if(category[QStringLiteral("divider")].toBool()){
            QVERIFY2(i > 0, "the first category is marked");
            marked << category[QStringLiteral("name")].toString();
        }
    }
    QCOMPARE(marked, SettingsSchema::CategoriesBeginningARun());
}

void tst_settingsschema::everyGroupStartsARowOnceAndInOrder(){
    QStringList keys;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items())
        keys << QString::fromLatin1(item.key);

    int last = -1;
    foreach(const SettingsSchema::Group &group, SettingsSchema::Groups()){
        const QString key = QString::fromLatin1(group.key);
        const int at = keys.indexOf(key);
        QVERIFY2(at >= 0,
                 qPrintable(QStringLiteral("heading '%1' names no row (%2)")
                            .arg(QString::fromLatin1(group.label), key)));
        QVERIFY2(at > last,
                 qPrintable(QStringLiteral("heading '%1' is out of the table's order (%2)")
                            .arg(QString::fromLatin1(group.label), key)));
        last = at;
        QVERIFY2(QString::fromLatin1(group.label).length() > 0,
                 qPrintable(QStringLiteral("heading over %1 has no words").arg(key)));
    }
    QVERIFY(!SettingsSchema::Groups().isEmpty());
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

void tst_settingsschema::everyChoiceHasALabelAndEveryLabelNamesAChoice(){
    QSet<QString> offered;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        if(item.type != SettingsSchema::Choice) continue;
        foreach(const QString &choice,
                QString::fromLatin1(item.choices).split(QLatin1Char('|')))
            offered.insert(QString::fromLatin1(item.key) + QLatin1Char('|') + choice);
    }
    QVERIFY(offered.size() > 20);

    QSet<QString> labelled;
    foreach(const SettingsSchema::ChoiceLabel &line, SettingsSchema::ChoiceLabels()){
        const QString id = QString::fromLatin1(line.key) + QLatin1Char('|') +
                           QString::fromLatin1(line.choice);
        QVERIFY2(offered.contains(id),
                 qPrintable(QStringLiteral("%1 is labelled, but no row offers it").arg(id)));
        QVERIFY2(!labelled.contains(id),
                 qPrintable(QStringLiteral("%1 is labelled twice").arg(id)));
        QVERIFY2(line.label && line.label[0],
                 qPrintable(QStringLiteral("%1 has an empty label").arg(id)));
        labelled.insert(id);
    }
    foreach(const QString &id, offered)
        QVERIFY2(labelled.contains(id),
                 qPrintable(QStringLiteral("%1 has no label; add one to 'ChoiceLabels'").arg(id)));

    const QJsonArray described =
        SettingsSchema::Describe()[QStringLiteral("items")].toArray();
    int rows = 0;
    for(const QJsonValue &value : described){
        const QJsonObject object = value.toObject();
        if(object[QStringLiteral("type")].toString() != QStringLiteral("choice")) continue;
        const QJsonArray choices = object[QStringLiteral("choices")].toArray();
        const QJsonArray labels  = object[QStringLiteral("choiceLabels")].toArray();
        QCOMPARE(labels.size(), choices.size());
        for(int i = 0; i < labels.size(); i++){
            QVERIFY(!labels[i].toString().isEmpty());
        }
        rows++;
    }
    QVERIFY(rows > 10);
}

void tst_settingsschema::everyLabelIsPresent(){
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        QVERIFY2(item.label && item.label[0],
                 qPrintable(QStringLiteral("%1 has no label")
                            .arg(QString::fromLatin1(item.key))));
    }
}

void tst_settingsschema::everyDependencyNamesARowAndAValueItOffers(){
    int clausesSeen = 0;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        const QString key = QString::fromLatin1(item.key);
        typedef QPair<QString, QString> Clause;
        foreach(const Clause &clause, SettingsSchema::AppliesWhenClauses(item)){
            clausesSeen++;
            const QString other = clause.first, value = clause.second;
            QVERIFY2(other != key,
                     qPrintable(QStringLiteral("%1 depends on itself").arg(key)));

            const SettingsSchema::Item *control = SettingsSchema::Find(other);
            QVERIFY2(control,
                     qPrintable(QStringLiteral("%1 depends on %2, which is not in the table")
                                .arg(key, other)));
            if(control->type == SettingsSchema::Bool){
                QVERIFY2(value == QStringLiteral("true") || value == QStringLiteral("false"),
                         qPrintable(QStringLiteral("%1 depends on %2 being '%3', which is not a boolean")
                                    .arg(key, other, value)));
            } else if(control->type == SettingsSchema::Choice){
                const QStringList choices =
                    QString::fromLatin1(control->choices).split(QLatin1Char('|'));
                QVERIFY2(choices.contains(value),
                         qPrintable(QStringLiteral("%1 depends on %2 being '%3', which it does not offer")
                                    .arg(key, other, value)));
            } else {
                QFAIL(qPrintable(QStringLiteral("%1 depends on %2, which is neither a boolean nor a choice")
                                 .arg(key, other)));
            }
        }
    }
    QVERIFY2(clausesSeen >= 16,
             qPrintable(QStringLiteral("only %1 clauses were found").arg(clausesSeen)));
}

void tst_settingsschema::aDependencyIsDescribedInTheShapeOfTheRowItNames(){
    const QJsonArray described =
        SettingsSchema::Describe()[QStringLiteral("items")].toArray();
    QJsonArray clauses;
    for(const QJsonValue &value : described){
        const QJsonObject object = value.toObject();
        if(object[QStringLiteral("key")].toString() !=
           QStringLiteral("application/@EnableTransparentWindow")) continue;
        clauses = object[QStringLiteral("appliesWhen")].toArray();
    }
    QCOMPARE(clauses.count(), 2);

    const QJsonObject rhi = clauses[0].toObject();
    QCOMPARE(rhi[QStringLiteral("key")].toString(), QStringLiteral("application/@EnableMainWindowRhi"));
    QVERIFY(rhi[QStringLiteral("value")].isBool());
    QCOMPARE(rhi[QStringLiteral("value")].toBool(), false);
    QVERIFY(!rhi.contains(QStringLiteral("label")));

    const QJsonObject api = clauses[1].toObject();
    QCOMPARE(api[QStringLiteral("key")].toString(), QStringLiteral("application/@GraphicsApi"));
    QCOMPARE(api[QStringLiteral("value")].toString(), QStringLiteral("OpenGL"));
    QCOMPARE(api[QStringLiteral("label")].toString(),
             QCoreApplication::translate("SettingsSchema", "OpenGL"));

    for(const QJsonValue &value : described){
        const QJsonObject object = value.toObject();
        const QString key = object[QStringLiteral("key")].toString();
        if(key == QStringLiteral("gadgets/thumblist/@RightClickToRenameNode")){
            const QJsonArray glass = object[QStringLiteral("appliesWhen")].toArray();
            QCOMPARE(glass.count(), 1);
            QCOMPARE(glass[0].toObject()[QStringLiteral("value")].toString(), QStringLiteral("GlassStyle"));
            QCOMPARE(glass[0].toObject()[QStringLiteral("label")].toString(),
                     QCoreApplication::translate("SettingsSchema", "Glass"));
        }
        if(key == QStringLiteral("application/@ColorScheme"))
            QVERIFY(!object.contains(QStringLiteral("appliesWhen")));
        if(key == QStringLiteral("application/@ExternalCommands") ||
           key == QStringLiteral("localview/@MediaVolume") ||
           key == QStringLiteral("localview/@AutoPlayMedia"))
            QCOMPARE(object[QStringLiteral("category")].toString(), QStringLiteral("page"));
        if(key == QStringLiteral("localview/@MediaVolume") ||
           key == QStringLiteral("localview/@AutoPlayMedia"))
            QCOMPARE(object[QStringLiteral("group")].toString(),
                     QCoreApplication::translate("SettingsSchema", "Local view"));
        if(key == QStringLiteral("gadgets/@Style")){
            const QJsonArray choices = object[QStringLiteral("choices")].toArray();
            QCOMPARE(choices.count(), 3);
            QCOMPARE(choices[0].toString(), QStringLiteral("PlainStyle"));
            QCOMPARE(choices[1].toString(), QStringLiteral("FlatStyle"));
            QCOMPARE(choices[2].toString(), QStringLiteral("GlassStyle"));
            QVERIFY(object[QStringLiteral("hint")].toString().isEmpty());
        }
        if(key == QStringLiteral("gadgets/thumblist/@EnableBackdropBlur")){
            const QJsonArray clauses = object[QStringLiteral("appliesWhen")].toArray();
            QCOMPARE(clauses.count(), 3);
            QCOMPARE(clauses[0].toObject()[QStringLiteral("value")].toString(), QStringLiteral("FlatStyle"));
            QCOMPARE(clauses[1].toObject()[QStringLiteral("value")].toString(), QStringLiteral("GlassStyle"));
            QCOMPARE(clauses[2].toObject()[QStringLiteral("key")].toString(),
                     QStringLiteral("application/@EnableTransparentWindow"));
            QCOMPARE(clauses[2].toObject()[QStringLiteral("value")].toBool(), true);
        }
    }
}

void tst_settingsschema::chromeExtensionsOfferADirectoryPicker(){
    const SettingsSchema::Item *item =
        SettingsSchema::Find(QStringLiteral("network/@Extensions"));
    QVERIFY(item);
    QCOMPARE(item->type, SettingsSchema::TextList);
    QCOMPARE(QString::fromLatin1(item->label), QStringLiteral("Chrome extensions"));
    QVERIFY(item->picker);
    QCOMPARE(QString::fromLatin1(item->picker), QStringLiteral("chrome-extension"));

    const QJsonArray described =
        SettingsSchema::Describe()[QStringLiteral("items")].toArray();
    int found = 0;
    for(const QJsonValue &value : described){
        const QJsonObject object = value.toObject();
        if(object[QStringLiteral("key")].toString() !=
           QStringLiteral("network/@Extensions")) continue;
        found++;
        QCOMPARE(object[QStringLiteral("picker")].toString(),
                 QStringLiteral("chrome-extension"));
    }
    QCOMPARE(found, 1);
}

void tst_settingsschema::theDownloadFoldersOfferAPickerAndWhatEmptyComesTo(){
    QStringList folders;
    for(const SettingsSchema::Item &item : SettingsSchema::Items())
        if(item.type == SettingsSchema::Directory) folders << QString::fromLatin1(item.key);
    QCOMPARE(folders, QStringList()
             << QStringLiteral("application/@FileSaveDirectory")
             << QStringLiteral("application/@FileOpenDirectory"));

    const QJsonArray described =
        SettingsSchema::Describe()[QStringLiteral("items")].toArray();
    int found = 0;
    for(const QJsonValue &value : described){
        const QJsonObject object = value.toObject();
        if(!folders.contains(object[QStringLiteral("key")].toString())) continue;
        found++;
        QCOMPARE(object[QStringLiteral("picker")].toString(), QStringLiteral("folder"));
        QVERIFY(!object[QStringLiteral("placeholder")].toString().isEmpty());
    }
    QCOMPARE(found, 2);

    const SettingsSchema::Item *save =
        SettingsSchema::Find(QStringLiteral("application/@FileSaveDirectory"));
    QVERIFY(save);
    QCOMPARE(SettingsSchema::EmptyDirectory(*save),
             QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));
    const SettingsSchema::Item *open =
        SettingsSchema::Find(QStringLiteral("application/@FileOpenDirectory"));
    QVERIFY(open);
    QCOMPARE(SettingsSchema::EmptyDirectory(*open),
             QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)));

    const QString kept = QStringLiteral("C:/Users/someone/Downloads/");
    const QJsonValue shown = SettingsSchema::ToJson(*save, kept);
    QCOMPARE(shown.toString(), QDir::toNativeSeparators(kept));
    QCOMPARE(SettingsSchema::FromJson(*save, shown).toString(), kept);
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
            QStringLiteral("value\\s*\\(QStringLiteral\\(\"%1\"\\)\\s*,\\s*"
                           "(QStringLiteral\\(\"[^\"]*\"\\)"
                           "|QString\\(\\)|QStringList\\(\\)"
                           "|[^),]+)\\)")
            .arg(QRegularExpression::escape(key)));

        bool compared = false;
        QRegularExpressionMatchIterator matches = pattern.globalMatch(all);
        while(matches.hasNext()){
            const QString expression = matches.next().captured(1).trimmed();

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
            compared = true;
        }
        if(compared) checked++;
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
        if(UserAgent::Find(key.mid(UserAgent::SettingsKey(QString()).length())) &&
           key.startsWith(UserAgent::SettingsKey(QString())))
            continue;

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

void tst_settingsschema::theUserAgentRowsAreTheTemplates(){
    const QString prefix = UserAgent::SettingsKey(QString());
    QStringList rows;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        const QString key = QString::fromLatin1(item.key);
        if(!key.startsWith(prefix)) continue;
        rows << key;
        QCOMPARE(QString::fromLatin1(item.category), QStringLiteral("hidden"));
        QCOMPARE(item.type, SettingsSchema::Text);
        QVERIFY2(!item.fallback[0], item.key);
    }
    QStringList templates;
    foreach(UserAgent::Entry entry, UserAgent::Entries())
        templates << UserAgent::SettingsKey(QString::fromLatin1(entry.name));
    QCOMPARE(rows, templates);
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

void tst_settingsschema::everyPreferenceRowIsAppliedByTheView(){
    QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
               QStringLiteral("/view/view.cpp"));
    QVERIFY2(file.open(QIODevice::ReadOnly),
             "'view.cpp' was not read; check VANILLA_SOURCE_DIR");

    QSet<QString> applied;
    const QRegularExpression setter
        (QStringLiteral("^\\s*gwes->setAttribute\\(QWebEngineSettings::(\\w+),"));
    for(const QString &line : QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))){
        const QRegularExpressionMatch match = setter.match(line);
        if(match.hasMatch()) applied << match.captured(1);
    }
    QVERIFY2(applied.size() > 20,
             qPrintable(QStringLiteral("only %1 attributes are applied; the pattern no longer matches")
                        .arg(applied.size())));

    int rows = 0;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        const QString key = QString::fromLatin1(item.key);
        if(!key.startsWith(QStringLiteral("webview/preferences/"))) continue;
        rows++;
        const QString attribute = key.section(QLatin1Char('/'), -1);
        QVERIFY2(applied.contains(attribute),
                 qPrintable(QStringLiteral("%1 is offered, but 'View::LoadSettings' never applies it")
                            .arg(key)));
    }
    QVERIFY2(rows > 20, qPrintable(QStringLiteral("only %1 preference rows were checked").arg(rows)));
}

void tst_settingsschema::everyAttributeTheViewAppliesReachesThePage(){
    QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
               QStringLiteral("/view/view.cpp"));
    QVERIFY2(file.open(QIODevice::ReadOnly),
             "'view.cpp' was not read; check VANILLA_SOURCE_DIR");
    const QString source = QString::fromUtf8(file.readAll());

    const int from = source.indexOf(QStringLiteral("void View::ApplySpecificSettings("));
    QVERIFY2(from != -1, "'View::ApplySpecificSettings' was not found");
    const int to = source.indexOf(QStringLiteral("\nvoid View::"), from + 1);
    const QString copy = source.mid(from, to == -1 ? -1 : to - from);

    QSet<QString> applied, copied;
    const QRegularExpression setter
        (QStringLiteral("^\\s*gwes->setAttribute\\(QWebEngineSettings::(\\w+),"));
    const QRegularExpression copier
        (QStringLiteral("^\\s*s->(?:setAttribute|resetAttribute)\\(QWebEngineSettings::(\\w+)[,)]"));
    for(const QString &line : source.split(QLatin1Char('\n'))){
        const QRegularExpressionMatch match = setter.match(line);
        if(match.hasMatch()) applied << match.captured(1);
    }
    for(const QString &line : copy.split(QLatin1Char('\n'))){
        const QRegularExpressionMatch match = copier.match(line);
        if(match.hasMatch()) copied << match.captured(1);
    }
    QVERIFY2(applied.size() > 20 && copied.size() > 20,
             qPrintable(QStringLiteral("%1 applied, %2 copied; the patterns no longer match")
                        .arg(applied.size()).arg(copied.size())));
    foreach(const QString &attribute, applied){
        QVERIFY2(copied.contains(attribute),
                 qPrintable(QStringLiteral("%1 is set on the default profile, but "
                                           "'View::ApplySpecificSettings' never copies it to a page")
                            .arg(attribute)));
    }
}

void tst_settingsschema::aValueTheSettingDoesNotTakeIsDroppedOrPutRight(){
    QMap<QString, QVariant> map;
    map.insert(QStringLiteral("gadgets/@Style"), QStringLiteral("Flat"));
    map.insert(QStringLiteral("application/@ColorScheme"), QStringLiteral("dark"));
    map.insert(QStringLiteral("application/@Viewport"), QStringLiteral("GLWidget"));
    map.insert(QStringLiteral("webview/@GestureMode"), 8);
    map.insert(QStringLiteral("application/@EnableAutoSave"), QStringLiteral("yes"));
    map.insert(QStringLiteral("application/@EnableAutoLoad"), QStringLiteral("False"));
    map.insert(QStringLiteral("application/@EnableTranslucentPanels"), true);
    map.insert(QStringLiteral("application/@AutoSaveInterval"), QStringLiteral("often"));
    map.insert(QStringLiteral("application/@AutoLoadInterval"), QStringLiteral("2000"));
    map.insert(QStringLiteral("application/@MaxViewCount"), 20);
    map.insert(QStringLiteral("application/@WindowEdgeColor"), QStringLiteral("black"));
    map.insert(QStringLiteral("webview/bookmarklet/x"), QStringLiteral("anything"));
    map.insert(QStringLiteral("application/@AcceptLanguage"), QStringLiteral("anything"));

    QStringList dropped = SettingsSchema::DropInvalidValues(map);
    dropped.sort();
    QCOMPARE(dropped, QStringList()
             << QStringLiteral("application/@AutoSaveInterval")
             << QStringLiteral("application/@EnableAutoSave")
             << QStringLiteral("application/@WindowEdgeColor")
             << QStringLiteral("gadgets/@Style"));
    QCOMPARE(map.value(QStringLiteral("application/@ColorScheme")).toString(), QStringLiteral("Dark"));
    QCOMPARE(map.value(QStringLiteral("application/@Viewport")).toString(), QStringLiteral("OpenGLWidget"));
    QCOMPARE(map.value(QStringLiteral("webview/@GestureMode")), QVariant(8));
    QCOMPARE(map.value(QStringLiteral("application/@EnableAutoLoad")).toString(), QStringLiteral("False"));
    QCOMPARE(map.value(QStringLiteral("application/@AutoLoadInterval")).toString(), QStringLiteral("2000"));
    QCOMPARE(map.size(), 9);

    QMap<QString, QVariant> kept;
    kept.insert(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewViewNodeNewWindow"));
    kept.insert(QStringLiteral("gadgets/accesskey/@AccessKeyAction"), QStringLiteral("OpenOnRootBackground"));
    QVERIFY(SettingsSchema::DropInvalidValues(kept).isEmpty());
    QCOMPARE(kept.size(), 2);
}

void tst_settingsschema::fromTextTakesOnlyWhatTheSettingCanBe(){
    using SettingsSchema::FromText;
    QCOMPARE(FromText(QStringLiteral("gadgets/@Style"), QStringLiteral("FlatStyle")),
             QVariant(QStringLiteral("FlatStyle")));
    QCOMPARE(FromText(QStringLiteral("gadgets/@Style"), QStringLiteral("flatstyle")),
             QVariant(QStringLiteral("FlatStyle")));
    QVERIFY(!FromText(QStringLiteral("gadgets/@Style"), QStringLiteral("Flat")).isValid());
    QCOMPARE(FromText(QStringLiteral("application/@GraphicsApi"), QStringLiteral("d3d11")),
             QVariant(QStringLiteral("Direct3D11")));
    QCOMPARE(FromText(QStringLiteral("application/@EnableAutoSave"), QStringLiteral("False")), QVariant(false));
    QVERIFY(!FromText(QStringLiteral("application/@EnableAutoSave"), QStringLiteral("no")).isValid());
    QCOMPARE(FromText(QStringLiteral("application/@AutoSaveInterval"), QStringLiteral("60000")), QVariant(60000));
    QVERIFY(!FromText(QStringLiteral("application/@AutoSaveInterval"), QStringLiteral("1m")).isValid());
    QCOMPARE(FromText(QStringLiteral("application/@WindowEdgeColor"), QStringLiteral("#AABBCC")),
             QVariant(QStringLiteral("#aabbcc")));
    QVERIFY(!FromText(QStringLiteral("application/@WindowEdgeColor"), QStringLiteral("red")).isValid());
    QVERIFY(!FromText(QStringLiteral("network/@SpellCheckLanguages"), QStringLiteral("en")).isValid());
    QCOMPARE(FromText(QStringLiteral("webview/bookmarklet/x"), QStringLiteral("a b")),
             QVariant(QStringLiteral("a b")));
    QVERIFY(!FromText(QStringLiteral("nothing/@At"), QStringLiteral("1")).isValid());

    const SettingsSchema::Item *style = SettingsSchema::Find(QStringLiteral("gadgets/@Style"));
    QVERIFY(style);
    QCOMPARE(SettingsSchema::Values(*style).length(), 3);
    QCOMPARE(SettingsSchema::Values(*style).at(1).first, QStringLiteral("FlatStyle"));
}

void tst_settingsschema::unknownKeysAreDroppedAndKnownOnesKept(){
    const QStringList alive = QStringList()
        << QStringLiteral("application/@EnableTransparentWindow")
        << QStringLiteral("application/UserAgent_Chrome")
        << QStringLiteral("webview/preferences/HyperlinkAuditingEnabled")
        << QStringLiteral("webview/@EnableScrollGesture")
        << QStringLiteral("webview/@SmartScreen")
        << QStringLiteral("webview/menu/LinkMenu")
        << QStringLiteral("receiver/keymap/Ctrl+Tab")
        << QStringLiteral("gadgets/thumblist/keymap/,")
        << QStringLiteral("webview/rightgesture/D,L")
        << QStringLiteral("webview/searchengine/brave")
        << QStringLiteral("webview/bookmarklet/x")
        << QStringLiteral("mainwindow/geometry4293");
    const QStringList dead = QStringList()
        << QStringLiteral("application/BrowserPath_Chrome")
        << QStringLiteral("application/@EnableTransparentBar")
        << QStringLiteral("application/@EnableFramelessWindow")
        << QStringLiteral("mainwindow/framelesstoolbar1")
        << QStringLiteral("application/UserAgent_IE")
        << QStringLiteral("webview/@EnableDragHack")
        << QStringLiteral("webview/preferences/XSSAuditingEnabled")
        << QStringLiteral("webview/searchengine")
        << QStringLiteral("webview/keymapx/A")
        << QStringLiteral("mainwindow/geometry")
        << QStringLiteral("mainwindow/geometry1x")
        << QStringLiteral("mainwindow/nothing1")
        << QStringLiteral("nothing/@At");

    QMap<QString, QVariant> map;
    for(const QString &key : alive + dead) map.insert(key, true);
    QStringList dropped = SettingsSchema::DropUnknownKeys(map);

    QStringList expected = alive;
    expected.sort();
    QCOMPARE(QStringList(map.keys()), expected);
    expected = dead;
    expected.sort();
    dropped.sort();
    QCOMPARE(dropped, expected);
}

void tst_settingsschema::everyKeyTheSourcesSpellIsKnown(){
    const QString root = QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR));
    QDirIterator it(root, QStringList() << QStringLiteral("*.cpp") << QStringLiteral("*.hpp")
                    << QStringLiteral("*.mm") << QStringLiteral("*.qml") << QStringLiteral("*.js"),
                    QDir::Files, QDirIterator::Subdirectories);

    const QRegularExpression literal(QStringLiteral(
        "\"((?:application|webview|gadgets|mainwindow|network|treebar|toolbar|localview|receiver)"
        "/[^\"\\s\\\\]*)\""));
    const QStringList mime = QStringList()
        << QStringLiteral("application/javascript") << QStringLiteral("application/json")
        << QStringLiteral("application/mxf") << QStringLiteral("application/ogg")
        << QStringLiteral("application/vnd.ms-asf") << QStringLiteral("application/vnd.rn-realmedia")
        << QStringLiteral("application/x-executable") << QStringLiteral("application/x-matroska")
        << QStringLiteral("application/x-ms-dos-executable") << QStringLiteral("application/x-msdownload")
        << QStringLiteral("application/x-msi") << QStringLiteral("application/x-ogg")
        << QStringLiteral("application/x-php") << QStringLiteral("application/x-ruby")
        << QStringLiteral("application/x-sharedlib") << QStringLiteral("application/x-shellscript")
        << QStringLiteral("application/signed-exchange;v=b3;q=0.7");

    QStringList beginnings = SettingsSchema::UnlistedPrefixes();
    for(const SettingsSchema::Item &item : SettingsSchema::Items()){
        const QString key = QString::fromLatin1(item.key);
        const int cut = key.indexOf(QLatin1Char('_'));
        if(cut > 0) beginnings << key.left(cut + 1);
    }
    const auto accepted = [&](const QString &key){
        if(SettingsSchema::IsKnownKey(key)) return true;
        if(key.contains(QLatin1Char('%')))
            return SettingsSchema::IsKnownKey(key.section(QLatin1Char('%'), 0, 0) + QLatin1Char('1'));
        for(const QString &beginning : beginnings)
            if(beginning == key || beginning == key + QLatin1Char('/')) return true;
        return false;
    };

    QVERIFY(!accepted(QStringLiteral("mainwindow/geometry")));
    QVERIFY(accepted(QStringLiteral("mainwindow/geometry%1")));
    QVERIFY(!accepted(QStringLiteral("mainwindow/nothing%1")));
    QVERIFY(!accepted(QStringLiteral("application/foo")));
    QVERIFY(!accepted(QStringLiteral("webview/menu")));
    QVERIFY(accepted(QStringLiteral("application/keymap")));
    QVERIFY(accepted(QStringLiteral("webview/searchengine")));
    QVERIFY(accepted(QStringLiteral("application/UserAgent_")));
    QVERIFY(!mime.contains(QStringLiteral("application/keymap")));

    int files = 0, checked = 0;
    while(it.hasNext()){
        const QString path = it.next();
        if(path.contains(QStringLiteral("/settingsschema."))) continue;
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        files++;
        for(const QString &line : QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))){
            const QString trimmed = line.trimmed();
            if(trimmed.startsWith(QStringLiteral("//")) || trimmed.startsWith(QLatin1Char('*')) ||
               trimmed.startsWith(QStringLiteral("/*")))
                continue;
            QRegularExpressionMatchIterator found = literal.globalMatch(line);
            while(found.hasNext()){
                const QString key = found.next().captured(1);
                if(mime.contains(key)) continue;
                checked++;
                QVERIFY2(accepted(key),
                         qPrintable(QStringLiteral("%1 spells '%2', which 'IsKnownKey' does not know: "
                                                   "the next launch would drop it")
                                    .arg(path, key)));
            }
        }
    }
    QVERIFY2(files > 100, qPrintable(QStringLiteral("%1 sources read; check VANILLA_SOURCE_DIR").arg(files)));
    QVERIFY2(checked > 150, qPrintable(QStringLiteral("only %1 keys checked; the pattern no longer matches")
                                       .arg(checked)));
}

void tst_settingsschema::aTabListItemMadeWithTheWindowAsksForARestart(){
    QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
               QStringLiteral("/gadgets/graphicstableview.cpp"));
    QVERIFY2(file.open(QIODevice::ReadOnly),
             "'graphicstableview.cpp' was not read; check VANILLA_SOURCE_DIR");

    const QRegularExpression made
        (QStringLiteral("^\\s*m_(Enable\\w+)\\s*\\?\\s*new\\s"));
    int rows = 0;
    for(const QString &line : QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))){
        const QRegularExpressionMatch match = made.match(line);
        if(!match.hasMatch()) continue;
        const QString key = QStringLiteral("gadgets/thumblist/@") + match.captured(1);
        const SettingsSchema::Item *item = SettingsSchema::Find(key);
        QVERIFY2(item, qPrintable(key + QStringLiteral(" is not in the table")));
        QVERIFY2(item->needsRestart,
                 qPrintable(key + QStringLiteral(" is made with the window but asks for no restart")));
        rows++;
    }
    QVERIFY2(rows >= 5, qPrintable(QStringLiteral("only %1 items were found; the pattern no longer matches")
                                   .arg(rows)));
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

    const SettingsSchema::Item *colour =
        SettingsSchema::Find(QStringLiteral("application/@WindowEdgeColor"));
    QVERIFY(colour);
    QCOMPARE(colour->type, SettingsSchema::Color);
    for(const QString &bad : { QStringLiteral("black"), QStringLiteral("#000"),
                               QStringLiteral("#ff000000"), QStringLiteral("000000"),
                               QStringLiteral("#gg0000") })
        QVERIFY2(!SettingsSchema::FromJson(*colour, QJsonValue(bad)).isValid(), qPrintable(bad));
    QVERIFY(!SettingsSchema::FromJson(*colour, QJsonValue(0)).isValid());
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

void tst_settingsschema::theGestureChoiceIsTheNumberTheViewReads(){
    const SettingsSchema::Item *item = SettingsSchema::Find(QStringLiteral("webview/@GestureMode"));
    QVERIFY(item);
    QCOMPARE(QString::fromLatin1(item->choices), QStringLiteral("4|8"));
    for(int count : { 4, 8 }){
        const QString text = QString::number(count);
        QCOMPARE(SettingsSchema::ToJson(*item, QVariant(count)).toString(), text);
        const QVariant written = SettingsSchema::FromJson(*item, QJsonValue(text));
        QVERIFY(written.isValid());
        QCOMPARE(written.value<int>(), count);
    }
    QVERIFY(!SettingsSchema::FromJson(*item, QJsonValue(QStringLiteral("6"))).isValid());
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

    const SettingsSchema::Item *colour =
        SettingsSchema::Find(QStringLiteral("application/@WindowEdgeColor"));
    QCOMPARE(SettingsSchema::FromJson(*colour, QJsonValue(QStringLiteral("#3A5F8c"))).toString(),
             QStringLiteral("#3a5f8c"));
    const QVariant system = SettingsSchema::FromJson(*colour, QJsonValue(QString()));
    QVERIFY(system.isValid());
    QCOMPARE(system.toString(), QString());
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

    const int from = source.indexOf(QStringLiteral("void NetworkAccessManager::SetupProfile(QWebEngineProfile *profile){"));
    QVERIFY2(from != -1, "'SetupProfile' was not found");
    const int to = source.indexOf(QStringLiteral("\n}\n"), from);
    QVERIFY(to != -1);
    const QString widget = source.mid(from, to - from);
    const QString quick = source.left(from) + source.mid(to);
    const QStringList setters = QStringList()
        << QStringLiteral("setPersistentCookiesPolicy")
        << QStringLiteral("setUrlRequestInterceptor")
        << QStringLiteral("setSpellCheckLanguages")
        << QStringLiteral("setSpellCheckEnabled")
        << QStringLiteral("setHttpCacheType")
        << QStringLiteral("setHttpCacheMaximumSize")
        << QStringLiteral("setPushServiceEnabled")
        << QStringLiteral("setHttpAcceptLanguage");

    for(const QString &setter : setters){
        QVERIFY2(widget.contains(QStringLiteral("profile->") + setter),
                 qPrintable(QStringLiteral("the widget profile is no longer told '%1'")
                            .arg(setter)));
        QVERIFY2(quick.contains(QStringLiteral("profile->") + setter),
                 qPrintable(QStringLiteral("the quick profile is no longer told '%1'")
                            .arg(setter)));
    }

    QVERIFY2(widget.contains(QStringLiteral("NetworkController::AdoptPermissions(profile)")),
             "the widget profile is no longer given the permissions table");

    const int at = source.indexOf
        (QStringLiteral("NetworkController::QuickPrivateProfile"));
    QVERIFY2(at > 0, "'QuickPrivateProfile' was not found");
    const int end = source.indexOf(QStringLiteral("\n}\n"), at);
    QVERIFY2(end > at, "the end of 'QuickPrivateProfile' was not found");
    const QString privateBody = source.mid(at, end - at);

    const QStringList helpers = QStringList()
        << QStringLiteral(" AdoptPermissions(profile)")
        << QStringLiteral("ApplyQuickBlockRules(profile)")
        << QStringLiteral("ApplyQuickCommonSettings(profile)");
    for(const QString &helper : helpers){
        QCOMPARE(source.count(helper), 3);
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
    for(const QString &setter : everyProfile){
        QVERIFY2(commonBody.contains(setter),
                 qPrintable(QStringLiteral("'%1' is no longer in the helper both "
                                           "quick profiles call").arg(setter)));
    }

    const QStringList diskOnly = QStringList()
        << QStringLiteral("setPersistentCookiesPolicy")
        << QStringLiteral("setHttpCacheType")
        << QStringLiteral("setHttpCacheMaximumSize");
    for(const QString &setter : diskOnly){
        QVERIFY2(!privateBody.contains(setter),
                 qPrintable(QStringLiteral("the private quick profile is told '%1', "
                                           "which is about disk").arg(setter)));
    }
}

void tst_settingsschema::chromiumSwitchesKeepQuotedSpaces(){
    QStringList ignored;
    QCOMPARE(Application::ChromiumSwitches(QStringLiteral("  --a   --b=c\t--d  "), &ignored),
             QStringList() << QStringLiteral("--a") << QStringLiteral("--b=c") << QStringLiteral("--d"));
    QVERIFY(ignored.isEmpty());

    const QStringList rules = QStringList() << QStringLiteral("--host-resolver-rules=MAP a b");
    QCOMPARE(Application::ChromiumSwitches(QStringLiteral("--host-resolver-rules=\"MAP a b\"")), rules);
    QCOMPARE(Application::ChromiumSwitches(QStringLiteral("\"--host-resolver-rules=MAP a b\"")), rules);
    QCOMPARE(Application::ChromiumSwitches(QStringLiteral("--host-resolver-rules=MAP\" a \"b")), rules);
    QCOMPARE(Application::ChromiumSwitches(QStringLiteral("--x --host-resolver-rules=\"MAP a b")),
             QStringList() << QStringLiteral("--x") << rules);

    ignored.clear();
    QCOMPARE(Application::ChromiumSwitches(QStringLiteral("url --a -b \"c d\" \"\""), &ignored),
             QStringList() << QStringLiteral("--a"));
    QCOMPARE(ignored, QStringList() << QStringLiteral("url") << QStringLiteral("-b")
                                    << QStringLiteral("c d"));
    ignored.clear();
    QCOMPARE(Application::ChromiumSwitches(QStringLiteral("\"\" --a"), &ignored),
             QStringList() << QStringLiteral("--a"));
    QVERIFY(ignored.isEmpty());
    QVERIFY(Application::ChromiumSwitches(QString()).isEmpty());

    const SettingsSchema::Item *item = SettingsSchema::Find(QStringLiteral("application/@ChromiumFlags"));
    QVERIFY(item);
    QCOMPARE(item->type, SettingsSchema::TextList);
    ignored.clear();
    QCOMPARE(Application::ChromiumSwitchesIn(QStringList()
                 << QStringLiteral("--a --b") << QStringLiteral("--c=\"d e") << QStringLiteral("f --g"), &ignored),
             QStringList() << QStringLiteral("--a") << QStringLiteral("--b")
                           << QStringLiteral("--c=d e") << QStringLiteral("--g"));
    QCOMPARE(ignored, QStringList() << QStringLiteral("f"));
    const QVariant older(QStringLiteral("--a --b=\"c d\""));
    QCOMPARE(Application::ChromiumSwitchesIn(older.value<QStringList>()),
             QStringList() << QStringLiteral("--a") << QStringLiteral("--b=c d"));
    QCOMPARE(SettingsSchema::ToJson(*item, older).toArray().size(), 1);
    QCOMPARE(SettingsSchema::ToJson(*item, older).toArray().at(0).toString(), older.toString());

    const QStringList switches = QStringList()
        << QStringLiteral("--a") << QStringLiteral("--host-resolver-rules=MAP a b")
        << QStringLiteral("--dir=C:\\my dir\\");
    QCOMPARE(Application::JoinChromiumSwitches(switches, false),
             QStringLiteral("--a \"--host-resolver-rules=MAP a b\" \"--dir=C:\\my dir\\\""));
    QCOMPARE(Application::ChromiumSwitches(Application::JoinChromiumSwitches(switches, false)), switches);
    QCOMPARE(Application::JoinChromiumSwitches(switches, true),
             QStringLiteral("--a \"--host-resolver-rules=MAP a b\" \"--dir=C:\\my dir\\\\\""));
    QCOMPARE(Application::JoinChromiumSwitches(QStringList() << QStringLiteral("--dir=C:\\x\\"), true),
             QStringLiteral("--dir=C:\\x\\"));
    QCOMPARE(Application::JoinChromiumSwitches(QStringList() << QStringLiteral("--a=\"b c"), true),
             QStringLiteral("\"--a=b c\""));
    const QStringList wide = QStringList() << QStringLiteral("--a=b") + QChar(0x3000) + QStringLiteral("c");
    QCOMPARE(Application::ChromiumSwitches(Application::JoinChromiumSwitches(wide, false)), wide);
}

void tst_settingsschema::theWidgetsCompositeOnTheGraphicsApiOrNotAtAll(){
    QCOMPARE(Application::WidgetsRhiBackendFor(QSGRendererInterface::OpenGL),     QByteArrayLiteral("opengl"));
    QCOMPARE(Application::WidgetsRhiBackendFor(QSGRendererInterface::Direct3D11), QByteArrayLiteral("d3d11"));
    QCOMPARE(Application::WidgetsRhiBackendFor(QSGRendererInterface::Direct3D12), QByteArrayLiteral("d3d12"));
    QCOMPARE(Application::WidgetsRhiBackendFor(QSGRendererInterface::Vulkan),     QByteArrayLiteral("vulkan"));
    QVERIFY(Application::WidgetsRhiBackendFor(QSGRendererInterface::Software).isEmpty());
    QVERIFY(Application::WidgetsRhiBackendFor(QSGRendererInterface::Unknown).isEmpty());
    QCOMPARE(Application::WidgetsRhiBackendFor(QSGRendererInterface::Metal),      QByteArrayLiteral("metal"));
}

void tst_settingsschema::theMainWindowIsAnRhiWindowOnlyWhenAskedFor(){
    const QByteArray rhi = qgetenv("QT_WIDGETS_RHI");
    const QByteArray backend = qgetenv("QT_WIDGETS_RHI_BACKEND");
    const bool hadRhi = qEnvironmentVariableIsSet("QT_WIDGETS_RHI");
    const bool hadBackend = qEnvironmentVariableIsSet("QT_WIDGETS_RHI_BACKEND");
    qunsetenv("QT_WIDGETS_RHI");
    qunsetenv("QT_WIDGETS_RHI_BACKEND");

    Application::ApplyWidgetsRhi(false, QSGRendererInterface::Direct3D11, false);
    QVERIFY(!qEnvironmentVariableIsSet("QT_WIDGETS_RHI"));
    QVERIFY(!qEnvironmentVariableIsSet("QT_WIDGETS_RHI_BACKEND"));
    Application::ApplyWidgetsRhi(false, QSGRendererInterface::Unknown, false);
    QVERIFY(!qEnvironmentVariableIsSet("QT_WIDGETS_RHI"));

    Application::ApplyWidgetsRhi(true, QSGRendererInterface::Software, false);
    QVERIFY(!qEnvironmentVariableIsSet("QT_WIDGETS_RHI"));

    Application::ApplyWidgetsRhi(true, QSGRendererInterface::Direct3D11, false);
    QCOMPARE(qgetenv("QT_WIDGETS_RHI"), QByteArrayLiteral("1"));
    QCOMPARE(qgetenv("QT_WIDGETS_RHI_BACKEND"), QByteArrayLiteral("d3d11"));

    qunsetenv("QT_WIDGETS_RHI");
    qunsetenv("QT_WIDGETS_RHI_BACKEND");
    if(hadRhi) qputenv("QT_WIDGETS_RHI", rhi);
    if(hadBackend) qputenv("QT_WIDGETS_RHI_BACKEND", backend);

    const SettingsSchema::Item *item = SettingsSchema::Find(QStringLiteral("application/@EnableMainWindowRhi"));
    QVERIFY(item);
    QCOMPARE(item->type, SettingsSchema::Bool);
    QCOMPARE(QString::fromLatin1(item->fallback), QStringLiteral("false"));
    QVERIFY(item->needsRestart);
}

void tst_settingsschema::everyGraphicsApiChoiceMeansSomethingToQt(){
    const SettingsSchema::Item *item = SettingsSchema::Find(QStringLiteral("application/@GraphicsApi"));
    QVERIFY(item);
    QCOMPARE(item->type, SettingsSchema::Choice);
#if defined(Q_OS_WIN)
    QCOMPARE(QString::fromLatin1(item->fallback), QStringLiteral("OpenGL"));
#else
    QCOMPARE(QString::fromLatin1(item->fallback), QStringLiteral("Auto"));
#endif

    const QStringList choices = QString::fromLatin1(item->choices).split(QLatin1Char('|'));
    for(const QString &choice : choices){
        const QSGRendererInterface::GraphicsApi api = Application::GraphicsApiFor(choice);
        if(choice == QStringLiteral("Auto")){
            QCOMPARE(api, QSGRendererInterface::Unknown);
        } else {
            QVERIFY2(api != QSGRendererInterface::Unknown,
                     qPrintable(QStringLiteral("'%1' is offered but means nothing to Qt").arg(choice)));
        }
    }
    QCOMPARE(Application::GraphicsApiFor(QStringLiteral("Software")), QSGRendererInterface::Software);
    QCOMPARE(Application::GraphicsApiFor(QStringLiteral("OpenGL")), QSGRendererInterface::OpenGL);
    QCOMPARE(Application::GraphicsApiFor(QStringLiteral("Direct3D11")), QSGRendererInterface::Direct3D11);
    QCOMPARE(Application::GraphicsApiFor(QStringLiteral("Direct3D12")), QSGRendererInterface::Direct3D12);
    QCOMPARE(Application::GraphicsApiFor(QStringLiteral("Vulkan")), QSGRendererInterface::Vulkan);
    QCOMPARE(Application::GraphicsApiFor(QStringLiteral("Metal")), QSGRendererInterface::Metal);

    const struct { const char *was; const char *now; QSGRendererInterface::GraphicsApi api; } older[] = {
        { "auto",     "Auto",       QSGRendererInterface::Unknown },
        { "software", "Software",   QSGRendererInterface::Software },
        { "opengl",   "OpenGL",     QSGRendererInterface::OpenGL },
        { "d3d11",    "Direct3D11", QSGRendererInterface::Direct3D11 },
        { "d3d12",    "Direct3D12", QSGRendererInterface::Direct3D12 },
        { "vulkan",   "Vulkan",     QSGRendererInterface::Vulkan },
        { "metal",    "Metal",      QSGRendererInterface::Metal },
    };
    for(const auto &word : older){
        QCOMPARE(Application::GraphicsApiFor(QLatin1String(word.was)), word.api);
        QCOMPARE(SettingsSchema::ToJson(*item, QVariant(QString::fromLatin1(word.was))).toString(),
                 QString::fromLatin1(word.now));
        QVERIFY2(choices.contains(QString::fromLatin1(word.now)), word.now);
    }

    QCOMPARE(Application::GraphicsApiFor(QStringLiteral("null")), QSGRendererInterface::Unknown);
    QCOMPARE(Application::GraphicsApiFor(QString()), QSGRendererInterface::Unknown);

    QVERIFY(Application::GraphicsApiRunsHere(QSGRendererInterface::Software));
    QVERIFY(Application::GraphicsApiRunsHere(QSGRendererInterface::OpenGL));
    QVERIFY(!Application::GraphicsApiRunsHere(QSGRendererInterface::Unknown));
    QVERIFY(!Application::GraphicsApiRunsHere(QSGRendererInterface::Null));
#if defined(Q_OS_WIN)
    QVERIFY(Application::GraphicsApiRunsHere(QSGRendererInterface::Direct3D11));
    QVERIFY(Application::GraphicsApiRunsHere(QSGRendererInterface::Direct3D12));
    QVERIFY(Application::GraphicsApiRunsHere(QSGRendererInterface::Vulkan));
    QVERIFY(!Application::GraphicsApiRunsHere(QSGRendererInterface::Metal));
#elif defined(Q_OS_MACOS)
    QVERIFY(!Application::GraphicsApiRunsHere(QSGRendererInterface::Direct3D11));
    QVERIFY(Application::GraphicsApiRunsHere(QSGRendererInterface::Metal));
    QVERIFY(!Application::GraphicsApiRunsHere(QSGRendererInterface::Vulkan));
#else
    QVERIFY(!Application::GraphicsApiRunsHere(QSGRendererInterface::Direct3D11));
    QVERIFY(!Application::GraphicsApiRunsHere(QSGRendererInterface::Metal));
    QVERIFY(Application::GraphicsApiRunsHere(QSGRendererInterface::Vulkan));
#endif
}

QTEST_MAIN(tst_settingsschema)
#include "tst_settingsschema.moc"
