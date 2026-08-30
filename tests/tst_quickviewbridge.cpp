#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QObject>
#include <QDir>
#include <QFile>
#include <QLibraryInfo>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace {

    QString WithoutComments(const QString &source){
        QString out;
        out.reserve(source.length());
        enum { Code, Line, Block, Text } state = Code;
        QChar quote;
        for(int i = 0; i < source.length(); i++){
            const QChar c = source[i];
            const QChar n = i + 1 < source.length() ? source[i+1] : QChar();
            switch(state){
            case Code:
                if(c == QLatin1Char('/') && n == QLatin1Char('/')){ state = Line; i++; }
                else if(c == QLatin1Char('/') && n == QLatin1Char('*')){ state = Block; i++; }
                else {
                    if(c == QLatin1Char('"') || c == QLatin1Char('\'')){ state = Text; quote = c; }
                    out += c;
                }
                break;
            case Line:
                if(c == QLatin1Char('\n')){ state = Code; out += c; }
                break;
            case Block:
                if(c == QLatin1Char('*') && n == QLatin1Char('/')){ state = Code; i++; }
                break;
            case Text:
                if(c == quote) state = Code;
                out += c;
                break;
            }
        }
        return out;
    }

    QMap<QString, QString> FunctionsOf(const QString &code){
        QMap<QString, QString> found;
        QRegularExpression head(QStringLiteral("\\bfunction\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\("));
        QRegularExpressionMatchIterator it = head.globalMatch(code);
        while(it.hasNext()){
            const QRegularExpressionMatch m = it.next();
            const int open = code.indexOf(QLatin1Char('{'), m.capturedEnd());
            if(open == -1) continue;

            int depth = 0, end = -1;
            QChar quote;
            for(int i = open; i < code.length(); i++){
                const QChar c = code[i];
                if(!quote.isNull()){ if(c == quote) quote = QChar(); continue; }
                if(c == QLatin1Char('"') || c == QLatin1Char('\'')){ quote = c; continue; }
                if(c == QLatin1Char('{')) depth++;
                if(c == QLatin1Char('}') && --depth == 0){ end = i; break; }
            }
            if(end == -1) continue;
            found.insert(m.captured(1), code.mid(open + 1, end - open - 1));
        }
        return found;
    }

    QString Read(const QString &path){
        QFile file(QDir::cleanPath(path));
        if(!file.open(QIODevice::ReadOnly)) return QString();
        return QString::fromUtf8(file.readAll());
    }

    QString EngineTypes(){
        return Read(QLibraryInfo::path(QLibraryInfo::QmlImportsPath) +
                    QStringLiteral("/QtWebEngine/plugins.qmltypes"));
    }
}

class tst_quickviewbridge : public QObject {
    Q_OBJECT

    QString m_Qml;
    QString m_Cpp;

private slots:

    void initTestCase(){
        m_Qml = Read(QStringLiteral(VANILLA_SOURCE_DIR "/view/webengine/quickwebengineview6.qml"));
        QVERIFY2(!m_Qml.isEmpty(), "check VANILLA_SOURCE_DIR");
        m_Cpp = Read(QStringLiteral(VANILLA_SOURCE_DIR "/view/webengine/quickwebengineview.cpp"))
              + Read(QStringLiteral(VANILLA_SOURCE_DIR "/view/webengine/quickwebengineview.hpp"));
        QVERIFY(!m_Cpp.isEmpty());
    }

    void noFunctionOfTheViewHasAnEmptyBody(){
        const QMap<QString, QString> functions = FunctionsOf(WithoutComments(m_Qml));
        QVERIFY2(functions.size() > 30, "the functions of the view moved");

        foreach(const QString &name, functions.keys())
            QVERIFY2(!functions[name].trimmed().isEmpty(),
                     qPrintable(QStringLiteral(
                         "'%1' is an empty QML function: it can be called and does nothing")
                         .arg(name)));
    }

    void everyQmlNameTheCppInvokesExists(){
        QSet<QString> asked;
        QRegularExpression call(QStringLiteral(
            "invokeMethod\\s*\\(\\s*(?:[A-Za-z_]\\w*\\s*(?:->|\\.))?m_QmlWebEngineView"
            "\\s*,\\s*\"([A-Za-z_][A-Za-z0-9_]*)\""));
        QRegularExpressionMatchIterator it = call.globalMatch(m_Cpp);
        while(it.hasNext()) asked.insert(it.next().captured(1));
        QVERIFY2(asked.size() > 20, "the calls into the view moved");

        const QMap<QString, QString> functions = FunctionsOf(WithoutComments(m_Qml));
        const QString types = EngineTypes();

        foreach(const QString &name, asked){
            if(functions.contains(name)) continue;
            if(types.isEmpty())
                QSKIP("no QtWebEngine plugins.qmltypes to check the engine's own names against");
            QVERIFY2(types.contains(QStringLiteral("\"%1\"").arg(name)),
                     qPrintable(QStringLiteral(
                         "nothing answers to '%1': neither the '.qml' nor the engine")
                         .arg(name)));
        }
    }

    void everyCppNameTheQmlCallsBackExists(){
        QSet<QString> called;
        QRegularExpression call(QStringLiteral(
            "viewInterface\\.([A-Za-z_][A-Za-z0-9_]*)\\s*\\("));
        QRegularExpressionMatchIterator it = call.globalMatch(WithoutComments(m_Qml));
        while(it.hasNext()) called.insert(it.next().captured(1));
        QVERIFY2(called.size() > 30, "the calls back into the C++ moved");

        const QString header = Read(QStringLiteral(VANILLA_SOURCE_DIR "/view/webengine/quickwebengineview.hpp"));
        QVERIFY(!header.isEmpty());

        foreach(const QString &name, called)
            QVERIFY2(header.contains(QRegularExpression(
                         QStringLiteral("\\b%1\\s*\\(").arg(QRegularExpression::escape(name)))),
                     qPrintable(QStringLiteral(
                         "the '.qml' calls 'viewInterface.%1', which the view does not declare")
                         .arg(name)));
    }

    void theQmlSpellsTheEnginesLifecycleStatesTheWayTheEngineDoes(){
        QSet<QString> asked;
        QRegularExpression spelling(QStringLiteral("LifecycleState\\.([A-Za-z]\\w*)"));
        QRegularExpressionMatchIterator it = spelling.globalMatch(WithoutComments(m_Qml));
        while(it.hasNext()) asked.insert(it.next().captured(1));
        QCOMPARE(asked.size(), 3);

        const QString types = EngineTypes();
        if(types.isEmpty()) QSKIP("no QtWebEngine plugins.qmltypes to check the spelling against");

        const int begin = types.indexOf(QStringLiteral("name: \"LifecycleState\""));
        QVERIFY2(begin != -1, "the engine no longer describes a lifecycle state");
        int end = types.indexOf(QLatin1Char('}'), begin);
        const QString described = types.mid(begin, end - begin);

        foreach(const QString &name, asked)
            QVERIFY2(described.contains(QStringLiteral("\"%1\"").arg(name)),
                     qPrintable(QStringLiteral("the engine has no lifecycle state %1").arg(name)));
    }

    void theCppNamesNoEngineLifecycleValue(){
        QVERIFY2(!m_Cpp.contains(QStringLiteral("lifecycleState")),
                 "the C++ reaches for the engine's lifecycle property itself: "
                 "ask the '.qml' ('suspend' / 'wakeUp' / 'isDiscarded') instead");
    }

    void printingGoesThroughTheQml(){
        QVERIFY2(m_Cpp.contains(QStringLiteral("\"print_\"")),
                 "the C++ no longer asks the '.qml' to print");

        const QMap<QString, QString> functions = FunctionsOf(WithoutComments(m_Qml));
        QVERIFY2(functions.contains(QStringLiteral("print_")), "'print_' is gone");
        QVERIFY2(functions[QStringLiteral("print_")].contains(QStringLiteral("printToPdf")),
                 "'print_' no longer reaches the engine");
    }

    void theEngineStillOffersTheQmlNoFontSetting(){
        const QString types = EngineTypes();
        if(types.isEmpty()) QSKIP("no QtWebEngine plugins.qmltypes to read");

        const int begin = types.indexOf
            (QStringLiteral("\n        name: \"QQuickWebEngineSettings\""));
        QVERIFY2(begin != -1, "the settings type of the QML view moved");
        int end = types.indexOf(QStringLiteral("\n    Component {"), begin);
        if(end == -1) end = types.length();

        QVERIFY2(end - begin > 4000, "the settings type is not where it was read from");

        QVERIFY2(!types.mid(begin, end - begin).contains(QStringLiteral("font"), Qt::CaseInsensitive),
                 "the QML 'WebEngineSettings' now has a font setting: "
                 "give 'quickwebengineview6.qml' its 'setFontFamily' and 'setFontSize' back, "
                 "and the comparison table the row it lost");
    }
};

QTEST_MAIN(tst_quickviewbridge)
#include "tst_quickviewbridge.moc"
