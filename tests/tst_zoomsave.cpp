#include <QtTest>

#include "webengineview.hpp"
#include "settingspage.hpp"
#include "lightnode.hpp"

#ifdef WEBENGINEVIEW
#  include <QtWebEngineQuick>
#  include <QWebEnginePage>
#endif

class tst_zoomsave : public QObject {
    Q_OBJECT

private slots:
#ifdef WEBENGINEVIEW
    void aSaveBeforeThePageHasReportedLeavesTheNodeAlone();
    void aSaveAfterThePageHasReportedWritesWhatItSaid();
#else
    void nothingToTest(){ QSKIP("no WebEngine in this build"); }
#endif
};

#ifdef WEBENGINEVIEW
namespace {
    class Fixture {
    public:
        Fixture()
            : view(nullptr, QStringLiteral("zoom-save"), QStringList())
            , node(new ViewNode()) {
            view.SetViewNode(node);
        }
        ~Fixture(){
            view.SetViewNode(nullptr);
            delete node;
        }
        WebEngineView view;
        ViewNode *node;
    };
}

void tst_zoomsave::aSaveBeforeThePageHasReportedLeavesTheNodeAlone(){
    Fixture f;
    f.node->SetZoom(0.5f);
    f.view.page()->runJavaScript(QStringLiteral("1"));
    QVERIFY(!f.view.SaveZoom());
    QCOMPARE(f.node->GetZoom(), 0.5f);
}

void tst_zoomsave::aSaveAfterThePageHasReportedWritesWhatItSaid(){
    Fixture f;
    f.node->SetZoom(0.5f);
    f.view.page()->runJavaScript(QStringLiteral("1"));
    QSignalSpy reported(f.view.page(), &QWebEnginePage::zoomFactorChanged);
    QVERIFY(f.view.RestoreZoom());
    QCOMPARE(reported.count(), 1);
    f.node->SetZoom(0.7f);
    QVERIFY(f.view.SaveZoom());
    QVERIFY(qAbs(f.node->GetZoom() - 0.5f) < 0.001f);
}

int main(int argc, char **argv){
    SettingsSchemeHandler::RegisterScheme();
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    tst_zoomsave test;
    return QTest::qExec(&test, argc, argv);
}
#else
QTEST_APPLESS_MAIN(tst_zoomsave)
#endif

#include "tst_zoomsave.moc"
