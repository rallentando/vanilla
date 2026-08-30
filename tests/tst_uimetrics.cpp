#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QFontDatabase>

#include "theme.hpp"

#include "testsupport.hpp"

namespace {

const QString SRC = QStringLiteral(VANILLA_SOURCE_DIR);
const QString RESOURCES = QStringLiteral(VANILLA_SOURCE_DIR "/../resources");

QStringList PaintingSources(){
    return QStringList()
        << SRC + QStringLiteral("/gadgets/gadgetsstyle.cpp")
        << SRC + QStringLiteral("/gadgets/graphicstableview.cpp")
        << SRC + QStringLiteral("/ui/dialog.cpp")
        << SRC + QStringLiteral("/ui/toolbar.cpp")
        << SRC + QStringLiteral("/ui/treebar.cpp")
        << SRC + QStringLiteral("/ui/notifier.cpp")
        << SRC + QStringLiteral("/app/receiver.cpp");
}

QStringList StyleSheetSources(){
    return QStringList()
        << SRC + QStringLiteral("/app/receiver.cpp")
        << SRC + QStringLiteral("/ui/dialog.hpp")
        << SRC + QStringLiteral("/ui/dialog.cpp")
        << SRC + QStringLiteral("/ui/mainwindow.cpp")
        << SRC + QStringLiteral("/ui/toolbar.cpp");
}

QStringList StyleSheets(){
    return QStringList()
        << RESOURCES + QStringLiteral("/settings/settings.css")
        << RESOURCES + QStringLiteral("/directory/directory.css");
}

QString Read(const QString &path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return QString();
    const QString text = QString::fromUtf8(file.readAll());
    file.close();
    return text;
}

QString Code(const QString &css){
    static const QRegularExpression block(QStringLiteral("/\\*.*?\\*/"),
                                          QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression line(QStringLiteral("//[^\n]*"));
    return QString(css).remove(block).remove(line);
}

}

class tst_uimetrics : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void theSourcesAreWhereTheyAreSaid();

    void noPaintingCodeRoundsByALiteral();
    void noPaintingCodeRoundsByALiteral_data();

    void noStyleSheetInTheSourcesHoldsALength();
    void noStyleSheetInTheSourcesHoldsALength_data();

    void noStyleSheetInTheSourcesNamesAColourOrAFace();
    void noStyleSheetInTheSourcesNamesAColourOrAFace_data();

    void neitherPageRoundsByALiteral();
    void neitherPageRoundsByALiteral_data();

    void neitherPageNamesAFont();
    void neitherPageNamesAFont_data();

    void theGeneratedLengthsComeFromTheTable();
    void theGeneratedFontIsTheDesktopsOwn();

    void theChromeAndThePagesShareTheSmallCorner();
};

void tst_uimetrics::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_uimetrics::theSourcesAreWhereTheyAreSaid(){
    foreach(const QString &path, PaintingSources() + StyleSheetSources() + StyleSheets()){
        QVERIFY2(!Read(path).isEmpty(), qPrintable(path));
    }
}

void tst_uimetrics::noPaintingCodeRoundsByALiteral_data(){
    QTest::addColumn<QString>("path");
    foreach(const QString &path, PaintingSources()){
        QTest::newRow(qPrintable(QFileInfo(path).fileName())) << path;
    }
}

void tst_uimetrics::noPaintingCodeRoundsByALiteral(){
    QFETCH(QString, path);

    static const QRegularExpression rounded(
        QStringLiteral("drawRoundedRect\\s*\\((?:[^;]*?),\\s*[0-9]"));

    const QRegularExpressionMatch match = rounded.match(Code(Read(path)));
    QVERIFY2(!match.hasMatch(),
             qPrintable(QStringLiteral("%1 rounds by a number of its own:\n  %2\n"
                                       "the radii are named in const.hpp.")
                        .arg(QFileInfo(path).fileName())
                        .arg(match.captured(0).simplified())));
}

void tst_uimetrics::noStyleSheetInTheSourcesHoldsALength_data(){
    noPaintingCodeRoundsByALiteral_data();
}

void tst_uimetrics::noStyleSheetInTheSourcesHoldsALength(){
    QFETCH(QString, path);

    static const QRegularExpression length(
        QStringLiteral("(?:border-radius|padding)\\s*:\\s*[0-9]"));

    const QRegularExpressionMatch match = length.match(Code(Read(path)));
    QVERIFY2(!match.hasMatch(),
             qPrintable(QStringLiteral("%1 writes a length into a style sheet:\n  %2")
                        .arg(QFileInfo(path).fileName())
                        .arg(match.captured(0).simplified())));
}

void tst_uimetrics::noStyleSheetInTheSourcesNamesAColourOrAFace_data(){
    QTest::addColumn<QString>("path");
    foreach(const QString &path, StyleSheetSources()){
        QTest::newRow(qPrintable(QFileInfo(path).fileName())) << path;
    }
}

void tst_uimetrics::noStyleSheetInTheSourcesNamesAColourOrAFace(){
    QFETCH(QString, path);
    const QString code = Code(Read(path));

    static const QRegularExpression colour(
        QStringLiteral("[{;\"][ \\t]*(?:background-color|background|color|border)[ \\t]*:[^;\"\\n]*"
                       "(?:#[0-9a-fA-F]{3}|rgba?[ \\t]*\\(|\\b(?:white|black|red|green|blue|gr[ae]y)\\b)"));

    QRegularExpressionMatch match = colour.match(code);
    QVERIFY2(!match.hasMatch(),
             qPrintable(QStringLiteral("%1 writes a colour into a style sheet:\n  %2\n"
                                       "the palette reaches one through "
                                       "'Theme::StyleSheetColor'.")
                        .arg(QFileInfo(path).fileName())
                        .arg(match.captured(0).simplified())));

    static const QRegularExpression face(
        QStringLiteral("[{;\"][ \\t]*font(?:-family|-size|-weight|-style)?[ \\t]*:"));

    match = face.match(code);
    QVERIFY2(!match.hasMatch(),
             qPrintable(QStringLiteral("%1 names a font in a style sheet:\n  %2\n"
                                       "the faces and the sizes are in const.hpp, "
                                       "and a widget takes them as a 'QFont'.")
                        .arg(QFileInfo(path).fileName())
                        .arg(match.captured(0).simplified())));
}

void tst_uimetrics::neitherPageRoundsByALiteral_data(){
    QTest::addColumn<QString>("path");
    QTest::newRow("settings.css")  << StyleSheets().at(0);
    QTest::newRow("directory.css") << StyleSheets().at(1);
}

void tst_uimetrics::neitherPageRoundsByALiteral(){
    QFETCH(QString, path);

    static const QRegularExpression radius(
        QStringLiteral("border-radius\\s*:\\s*[0-9]"));

    const QRegularExpressionMatch match = radius.match(Code(Read(path)));
    QVERIFY2(!match.hasMatch(),
             qPrintable(QStringLiteral("%1 rounds by a number of its own:\n  %2\n"
                                       "the pages are handed '--radius-small', "
                                       "'--radius' and '--radius-pill'.")
                        .arg(QFileInfo(path).fileName())
                        .arg(match.captured(0).simplified())));
}

void tst_uimetrics::neitherPageNamesAFont_data(){
    neitherPageRoundsByALiteral_data();
}

void tst_uimetrics::neitherPageNamesAFont(){
    QFETCH(QString, path);
    const QString css = Code(Read(path));

    static const QRegularExpression family(
        QStringLiteral("font-family\\s*:(?!\\s*var\\()"));
    QRegularExpressionMatch match = family.match(css);
    QVERIFY2(!match.hasMatch(),
             qPrintable(QStringLiteral("%1 names a font family of its own.")
                        .arg(QFileInfo(path).fileName())));

    static const QRegularExpression shorthand(
        QStringLiteral("[^-]font\\s*:[^;]*(?:\"|sans-serif|serif|monospace|system-ui)"));
    match = shorthand.match(css);
    QVERIFY2(!match.hasMatch(),
             qPrintable(QStringLiteral("%1 names a font in a shorthand:\n  %2")
                        .arg(QFileInfo(path).fileName())
                        .arg(match.captured(0).simplified())));
}

void tst_uimetrics::theGeneratedFontIsTheDesktopsOwn(){
    const QString css = QString::fromUtf8(Theme::PageFontVariables());

    QCOMPARE(css.count(QLatin1Char('{')), css.count(QLatin1Char('}')));
    QVERIFY(!css.contains(QStringLiteral("prefers-color-scheme")));

    const QString ui = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    QVERIFY2(!ui.isEmpty(), "the desktop names no general font.");
    QVERIFY2(css.contains(QStringLiteral("--font-family: \"%1\"").arg(ui)),
             qPrintable(css));
    QVERIFY2(css.contains(QStringLiteral("--font-mono: %1;").arg(MONOSPACE_FAMILIES)),
             qPrintable(css));
    QVERIFY2(!MONOSPACE_FAMILIES.contains(QStringLiteral("Courier")),
             "the code font is Courier New again.");

    QCOMPARE(DefaultFont().family(), ui);
    QCOMPARE(TreeBarTitleFont().family(), ui);
}

void tst_uimetrics::theGeneratedLengthsComeFromTheTable(){
    const QString css = QString::fromUtf8(Theme::PageMetricVariables());

    QCOMPARE(css.count(QLatin1Char('{')), css.count(QLatin1Char('}')));
    QVERIFY(!css.contains(QStringLiteral("prefers-color-scheme")));

    QVERIFY2(css.contains(QStringLiteral("--radius-small: %1px;")
                          .arg(CHIP_CORNER_RADIUS)), qPrintable(css));
    QVERIFY2(css.contains(QStringLiteral("--radius: %1px;")
                          .arg(PAGE_FIELD_CORNER_RADIUS)), qPrintable(css));
    QVERIFY2(css.contains(QStringLiteral("--radius-pill: %1px;")
                          .arg(PAGE_PILL_CORNER_RADIUS)), qPrintable(css));
}

void tst_uimetrics::theChromeAndThePagesShareTheSmallCorner(){
    QVERIFY2(QString::fromUtf8(Theme::PageMetricVariables())
             .contains(QStringLiteral("--radius-small: %1px;").arg(CHIP_CORNER_RADIUS)),
             "the pages' small corner is no longer the chrome's chip corner.");

    QVERIFY(MARKER_CORNER_RADIUS < CHIP_CORNER_RADIUS);
    QVERIFY(CHIP_CORNER_RADIUS   < FIELD_CORNER_RADIUS);
    QVERIFY(FIELD_CORNER_RADIUS  < PAGE_FIELD_CORNER_RADIUS);
}

QTEST_MAIN(tst_uimetrics)
#include "tst_uimetrics.moc"
