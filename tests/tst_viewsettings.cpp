#include "switch.hpp"

#include <QtTest>

#include "application.hpp"
#include "view.hpp"
#ifdef WEBENGINEVIEW
#  include <QWebEngineProfile>
#  include <QWebEngineSettings>
#  include <QtWebEngineQuick/qtwebenginequickglobal.h>
#endif

class tst_viewsettings : public QObject {
    Q_OBJECT

private slots:
#ifdef WEBENGINEVIEW
    void aRemovedAttributeKeyGivesTheEngineDefaultBack();
    void aRemovedFontKeyGivesTheEngineDefaultBack();
    void savingWritesNoEngineKey();
#endif
};

#ifdef WEBENGINEVIEW
namespace {
    QWebEngineSettings *Globals(){
        return QWebEngineProfile::defaultProfile()->settings();
    }

    void RemoveGroup(const QString &group){
        Settings &s = Application::GlobalSettings();
        foreach(const QString &key, s.allKeys(group)) s.remove(key);
    }
}

void tst_viewsettings::aRemovedAttributeKeyGivesTheEngineDefaultBack(){
    const QList<QWebEngineSettings::WebAttribute> attributes = {
        QWebEngineSettings::AutoLoadImages,
        QWebEngineSettings::SpatialNavigationEnabled,
    };
    const QStringList keys = {
        QStringLiteral("webview/preferences/AutoLoadImages"),
        QStringLiteral("webview/preferences/SpatialNavigationEnabled"),
    };
    Settings &s = Application::GlobalSettings();
    for(int i = 0; i < attributes.size(); i++){
        Globals()->resetAttribute(attributes[i]);
        const bool engine = Globals()->testAttribute(attributes[i]);

        s.setValue(keys[i], !engine);
        View::LoadSettings();
        QCOMPARE(Globals()->testAttribute(attributes[i]), !engine);

        s.remove(keys[i]);
        View::LoadSettings();
        QVERIFY2(Globals()->testAttribute(attributes[i]) == engine,
                 qPrintable(keys[i] + QStringLiteral(" kept the value of the removed key")));
    }
}

void tst_viewsettings::aRemovedFontKeyGivesTheEngineDefaultBack(){
    Settings &s = Application::GlobalSettings();

    Globals()->resetFontFamily(QWebEngineSettings::StandardFont);
    Globals()->resetFontSize(QWebEngineSettings::DefaultFontSize);
    const QString family = Globals()->fontFamily(QWebEngineSettings::StandardFont);
    const int size = Globals()->fontSize(QWebEngineSettings::DefaultFontSize);
    const QString other = family == QStringLiteral("Courier New")
        ? QStringLiteral("Arial") : QStringLiteral("Courier New");

    s.setValue(QStringLiteral("webview/font/StandardFont"), other);
    s.setValue(QStringLiteral("webview/font/DefaultFontSize"), size + 7);
    View::LoadSettings();
    QCOMPARE(Globals()->fontFamily(QWebEngineSettings::StandardFont), other);
    QCOMPARE(Globals()->fontSize(QWebEngineSettings::DefaultFontSize), size + 7);

    s.remove(QStringLiteral("webview/font/StandardFont"));
    s.remove(QStringLiteral("webview/font/DefaultFontSize"));
    View::LoadSettings();
    QCOMPARE(Globals()->fontFamily(QWebEngineSettings::StandardFont), family);
    QCOMPARE(Globals()->fontSize(QWebEngineSettings::DefaultFontSize), size);

    s.setValue(QStringLiteral("webview/font/StandardFont"), other);
    View::LoadSettings();
    s.setValue(QStringLiteral("webview/font/StandardFont"), QString());
    View::LoadSettings();
    QCOMPARE(Globals()->fontFamily(QWebEngineSettings::StandardFont), family);
    s.remove(QStringLiteral("webview/font/StandardFont"));
}

void tst_viewsettings::savingWritesNoEngineKey(){
    RemoveGroup(QStringLiteral("webview/preferences/"));
    RemoveGroup(QStringLiteral("webview/font/"));
    View::LoadSettings();
    View::SaveSettings();

    Settings &s = Application::GlobalSettings();
    QVERIFY2(s.allKeys(QStringLiteral("webview/preferences/")).isEmpty(),
             qPrintable(s.allKeys(QStringLiteral("webview/preferences/")).join(QStringLiteral(", "))));
    QVERIFY2(s.allKeys(QStringLiteral("webview/font/")).isEmpty(),
             qPrintable(s.allKeys(QStringLiteral("webview/font/")).join(QStringLiteral(", "))));
    QVERIFY(s.contains(QStringLiteral("webview/@GestureMode")));
}
#endif

#ifdef WEBENGINEVIEW
int main(int argc, char **argv){
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    tst_viewsettings test;
    return QTest::qExec(&test, argc, argv);
}
#else
QTEST_MAIN(tst_viewsettings)
#endif
#include "tst_viewsettings.moc"
