#include <QtTest>

#include "webengineview.hpp"
#include "settingspage.hpp"

#ifdef WEBENGINEVIEW
#  include <QtWebEngineQuick>
#  include <QQuickWidget>
#endif

class tst_drawingstack : public QObject {
    Q_OBJECT

private slots:
#ifdef WEBENGINEVIEW
    void theDrawingWidgetIsNotLeftStackedOnTop();
#else
    void noEngine(){ QSKIP("built without the engine view"); }
#endif
};

#ifdef WEBENGINEVIEW
void tst_drawingstack::theDrawingWidgetIsNotLeftStackedOnTop(){
    WebEngineView view(nullptr, QStringLiteral("drawing-stack"), QStringList());
    QQuickWidget *drawing = new QQuickWidget;
    drawing->setParent(&view);

    drawing->setAttribute(Qt::WA_AlwaysStackOnTop, true);
    QShowEvent show;
    QCoreApplication::sendEvent(drawing, &show);
    QVERIFY(!drawing->testAttribute(Qt::WA_AlwaysStackOnTop));

    drawing->setAttribute(Qt::WA_AlwaysStackOnTop, true);
    QResizeEvent resize(QSize(10, 10), QSize(1, 1));
    QCoreApplication::sendEvent(drawing, &resize);
    QVERIFY(!drawing->testAttribute(Qt::WA_AlwaysStackOnTop));

    drawing->setAttribute(Qt::WA_AlwaysStackOnTop, true);
    view.KeepDrawingWidgetClear(drawing);
    QVERIFY(!drawing->testAttribute(Qt::WA_AlwaysStackOnTop));

    QWidget *other = new QWidget(&view);
    other->setAttribute(Qt::WA_AlwaysStackOnTop, true);
    view.KeepDrawingWidgetClear(other);
    QVERIFY(other->testAttribute(Qt::WA_AlwaysStackOnTop));
}
#endif

#ifdef WEBENGINEVIEW
int main(int argc, char **argv){
    SettingsSchemeHandler::RegisterScheme();
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    tst_drawingstack test;
    return QTest::qExec(&test, argc, argv);
}
#else
QTEST_APPLESS_MAIN(tst_drawingstack)
#endif

#include "tst_drawingstack.moc"
