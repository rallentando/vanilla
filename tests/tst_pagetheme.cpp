#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QMap>

#include "theme.hpp"

#include "testsupport.hpp"

namespace {

const QString SETTINGS_CSS =
    QStringLiteral(VANILLA_SOURCE_DIR "/../resources/settings/settings.css");
const QString DIRECTORY_CSS =
    QStringLiteral(VANILLA_SOURCE_DIR "/../resources/directory/directory.css");

QString Read(const QString &path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return QString();
    const QString text = QString::fromUtf8(file.readAll());
    file.close();
    return text;
}

QString Rules(const QString &css){
    static const QRegularExpression comment(QStringLiteral("/\\*.*?\\*/"),
                                            QRegularExpression::DotMatchesEverythingOption);
    return QString(css).remove(comment);
}

QSet<QString> Used(const QString &css){
    QSet<QString> names;
    static const QRegularExpression use(
        QStringLiteral("var\\(\\s*(--[a-z0-9-]+)\\s*([,)])"));
    QRegularExpressionMatchIterator it = use.globalMatch(css);
    while(it.hasNext()){
        QRegularExpressionMatch match = it.next();
        if(match.captured(2) == QStringLiteral(")")) names << match.captured(1);
    }
    return names;
}

QList<QPair<QString, QString>> Declared(const QString &css){
    QList<QPair<QString, QString>> declarations;
    static const QRegularExpression declaration(
        QStringLiteral("(--[a-z0-9-]+)\\s*:\\s*([^;]+);"));
    QRegularExpressionMatchIterator it = declaration.globalMatch(css);
    while(it.hasNext()){
        QRegularExpressionMatch match = it.next();
        declarations << qMakePair(match.captured(1), match.captured(2).trimmed());
    }
    return declarations;
}

QString Generated(){
    return QString::fromUtf8(Theme::PageVariables());
}

QString GeneratedColors(){
    return QString::fromUtf8(Theme::PageColorVariables());
}

}

class tst_pagetheme : public QObject {
    Q_OBJECT

private:
    static QStringList StyleSheets(){
        return QStringList() << SETTINGS_CSS << DIRECTORY_CSS;
    }

private slots:
    void initTestCase();

    void theStyleSheetsAreWhereTheyAreSaid();

    void neitherStyleSheetHoldsAColour();
    void neitherStyleSheetHoldsAColour_data();

    void neitherStyleSheetDecidesTheScheme();
    void neitherStyleSheetDecidesTheScheme_data();

    void everyVariableUsedIsGenerated();
    void everyVariableUsedIsGenerated_data();

    void everyVariableGeneratedIsUsed();

    void theGeneratedBlockCarriesBothSchemes();

    void theGeneratedValuesComeFromThePalette();
    void theGeneratedValuesComeFromThePalette_data();
};

void tst_pagetheme::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_pagetheme::theStyleSheetsAreWhereTheyAreSaid(){
    foreach(const QString &path, StyleSheets()){
        QVERIFY2(!Read(path).isEmpty(), qPrintable(path));
    }
}

void tst_pagetheme::neitherStyleSheetHoldsAColour_data(){
    QTest::addColumn<QString>("path");
    QTest::newRow("settings")  << SETTINGS_CSS;
    QTest::newRow("directory") << DIRECTORY_CSS;
}

void tst_pagetheme::neitherStyleSheetHoldsAColour(){
    QFETCH(QString, path);
    const QString css = Rules(Read(path));

    static const QRegularExpression hex(
        QStringLiteral("#(?:[0-9a-fA-F]{3,4}|[0-9a-fA-F]{6}|[0-9a-fA-F]{8})\\b"));
    static const QRegularExpression named(
        QStringLiteral("\\b(?:aqua|black|blue|fuchsia|gray|grey|green|lime|maroon"
                       "|navy|olive|orange|purple|red|silver|teal|white|yellow)\\b(?!-)"));
    static const QRegularExpression functional(
        QStringLiteral("\\b(?:rgba?|hsla?|hwb|lab|lch|oklab|oklch)\\s*\\("));

    for(const QRegularExpression *check : {&hex, &named, &functional}){
        QRegularExpressionMatch match = check->match(css);
        QVERIFY2(!match.hasMatch(),
                 qPrintable(QStringLiteral("%1 holds the colour \"%2\". "
                                           "the palette belongs in Theme; add a role "
                                           "and a row to 'PageVariables'.")
                            .arg(QFileInfo(path).fileName()).arg(match.captured(0))));
    }
}

void tst_pagetheme::neitherStyleSheetDecidesTheScheme_data(){
    neitherStyleSheetHoldsAColour_data();
}

void tst_pagetheme::neitherStyleSheetDecidesTheScheme(){
    QFETCH(QString, path);
    const QString css = Rules(Read(path));

    QVERIFY2(!css.contains(QStringLiteral("prefers-color-scheme")),
             "the style sheet decides the scheme; Theme does that.");
    QVERIFY2(!css.contains(QStringLiteral("color-scheme")),
             "the style sheet sets 'color-scheme'; the generated block does that.");
}

void tst_pagetheme::everyVariableUsedIsGenerated_data(){
    neitherStyleSheetHoldsAColour_data();
}

void tst_pagetheme::everyVariableUsedIsGenerated(){
    QFETCH(QString, path);

    QSet<QString> generated;
    foreach(const auto &declaration, Declared(Generated())) generated << declaration.first;

    foreach(const QString &name, Used(Read(path))){
        QVERIFY2(generated.contains(name),
                 qPrintable(QStringLiteral("%1 uses \"%2\", which Theme does not "
                                           "generate. a variable with no value "
                                           "falls back to nothing at all.")
                            .arg(QFileInfo(path).fileName()).arg(name)));
    }
}

void tst_pagetheme::everyVariableGeneratedIsUsed(){
    QSet<QString> used;
    foreach(const QString &path, StyleSheets()) used += Used(Read(path));

    QSet<QString> seen;
    foreach(const auto &declaration, Declared(Generated())){
        if(seen.contains(declaration.first)) continue;
        seen << declaration.first;
        QVERIFY2(used.contains(declaration.first),
                 qPrintable(QStringLiteral("Theme generates \"%1\" and no page uses it. "
                                           "a role kept for nobody is the drift this "
                                           "was meant to stop.")
                            .arg(declaration.first)));
    }
}

void tst_pagetheme::theGeneratedBlockCarriesBothSchemes(){
    const QString css = GeneratedColors();

    qInfo().noquote() << QStringLiteral("\n") + css;

    QCOMPARE(css.count(QLatin1Char('{')), css.count(QLatin1Char('}')));

    QVERIFY2(css.contains(QStringLiteral("color-scheme: light dark")),
             "without 'color-scheme' the engine keeps its own controls light.");
    QVERIFY2(css.contains(QStringLiteral("@media (prefers-color-scheme: dark)")),
             "a page already open follows the scheme through this and nothing else.");

    const QList<QPair<QString, QString>> declarations = Declared(css);
    QVERIFY(!declarations.isEmpty());
    QCOMPARE(declarations.length() % 2, 0);

    const int half = declarations.length() / 2;
    for(int i = 0; i < half; i++){
        QCOMPARE(declarations[i].first, declarations[i + half].first);
    }

    static const QRegularExpression colour(
        QStringLiteral("\\A(?:#[0-9a-f]{6}|rgb\\([0-9 ]+/ [0-9.]+\\))\\z"));
    foreach(const auto &declaration, declarations){
        QVERIFY2(colour.match(declaration.second).hasMatch(),
                 qPrintable(QStringLiteral("\"%1\" came out as \"%2\".")
                            .arg(declaration.first).arg(declaration.second)));
    }
}

void tst_pagetheme::theGeneratedValuesComeFromThePalette_data(){
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("role");

#define ROW(name, role) \
    QTest::newRow(name) << QStringLiteral(name) << static_cast<int>(Theme::role)

    ROW("--bg",         VanillaPageBackground);
    ROW("--bg-sunken",  VanillaPageBackgroundSunken);
    ROW("--fg",         VanillaPageText);
    ROW("--fg-dim",     VanillaPageTextDim);
    ROW("--line",       VanillaPageBorder);
    ROW("--accent",     VanillaPageAccent);
    ROW("--accent-fg",  VanillaPageAccentText);
    ROW("--field-bg",   VanillaPageFieldBackground);
    ROW("--invalid",    VanillaPageInvalidBorder);
    ROW("--toast-bg",   VanillaPageToastBackground);
    ROW("--toast-fg",   VanillaPageToastText);

#undef ROW
}

void tst_pagetheme::theGeneratedValuesComeFromThePalette(){
    QFETCH(QString, name);
    QFETCH(int, role);

    const QList<QPair<QString, QString>> declarations = Declared(GeneratedColors());

    QStringList values;
    for(int i = 0; i < declarations.length(); i++){
        if(declarations[i].first == name) values << declarations[i].second;
    }
    QCOMPARE(values.length(), 2);

    QCOMPARE(QColor(values[0]),
             Theme::Color(Theme::Light, static_cast<Theme::Role>(role)));
    QCOMPARE(QColor(values[1]),
             Theme::Color(Theme::Dark,  static_cast<Theme::Role>(role)));
}

QTEST_MAIN(tst_pagetheme)
#include "tst_pagetheme.moc"
