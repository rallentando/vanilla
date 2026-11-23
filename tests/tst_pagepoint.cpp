#include <QtTest>
#include <memory>

#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "settingspage.hpp"
#include "page.hpp"
#include "webelement.hpp"
#ifdef WEBENGINEVIEW
#  include <QtWebEngineQuick>
#endif

class tst_pagepoint : public QObject {
    Q_OBJECT

private slots:
#ifdef WEBENGINEVIEW
    void theWidgetViewLooksForTheFormInThePagesPixels();
    void theQuickViewLooksForTheFormInThePagesPixels();
    void aBookmarkletIsLookedForWhereTheButtonWas();
#endif
};

#ifdef WEBENGINEVIEW
namespace {
    template <class Base>
    class Recording : public Base {
    public:
        Recording() : Base(nullptr, QStringLiteral("pagepoint"),
                           QStringList() << QStringLiteral("Private")) {}
        QStringList scripts;
        QList<QPoint> hits;
        void CallWithGotCurrentBaseUrl(UrlCallBack callBack) override {
            callBack(QUrl(QStringLiteral("http://example.com/")));
        }
        void CallWithEvaluatedJavaScriptResult(const QString &code, VariantCallBack) override {
            scripts << code;
        }
        void CallWithHitElement(const QPoint &pos, WebElementCallBack) override {
            hits << pos;
        }
    };
    typedef Recording<WebEngineView> WidgetsView;
    typedef Recording<QuickWebEngineView> QuickView;

    SharedWebElement ElementAt(View *view, const QPoint &pos){
        return std::make_shared<JsWebElement>(view, pos, QUrl(), QUrl(), true);
    }

    bool LooksAt(const QString &script, int x, int y){
        return script.contains(QStringLiteral("var x = %1;").arg(x)) &&
               script.contains(QStringLiteral("var y = %1;").arg(y));
    }
}

void tst_pagepoint::theWidgetViewLooksForTheFormInThePagesPixels(){
    WidgetsView view;
    view.setZoomFactor(2.0);
    QCOMPARE(view.zoomFactor(), 2.0);

    view.TriggerAction(Page::_AddSearchEngine, QVariant(QPoint(200, 100)));
    view.TriggerAction(Page::_AddSearchEngine, QVariant::fromValue(ElementAt(&view, QPoint(200, 100))));

    QCOMPARE(view.scripts.size(), 2);
    QVERIFY2(LooksAt(view.scripts.at(0), 100, 50), "the menu's point is not the page's");
    QVERIFY2(LooksAt(view.scripts.at(1), 100, 50), "the element's point is not the page's");
}

void tst_pagepoint::theQuickViewLooksForTheFormInThePagesPixels(){
    QuickView view;
    QVERIFY(view.rootObject());
    view.rootObject()->setProperty("zoomFactor", 2.0);
    QCOMPARE(view.rootObject()->property("zoomFactor").toReal(), 2.0);

    view.TriggerAction(Page::_AddSearchEngine, QVariant(QPoint(200, 100)));
    view.TriggerAction(Page::_AddSearchEngine, QVariant::fromValue(ElementAt(&view, QPoint(200, 100))));

    QCOMPARE(view.scripts.size(), 2);
    QVERIFY2(LooksAt(view.scripts.at(0), 100, 50), "the menu's point is not the page's");
    QVERIFY2(LooksAt(view.scripts.at(1), 200, 100), "the element's point was scaled again");
}

void tst_pagepoint::aBookmarkletIsLookedForWhereTheButtonWas(){
    WidgetsView view;
    view.setZoomFactor(2.0);

    view.TriggerAction(Page::_AddBookmarklet, QVariant(QPoint(200, 100)));
    view.TriggerAction(Page::_AddBookmarklet, QVariant::fromValue(ElementAt(&view, QPoint(200, 100))));

    QCOMPARE(view.hits, QList<QPoint>() << QPoint(200, 100) << QPoint(200, 100));
}
#endif

#ifdef WEBENGINEVIEW
int main(int argc, char **argv){
    SettingsSchemeHandler::RegisterScheme();
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    tst_pagepoint test;
    return QTest::qExec(&test, argc, argv);
}
#else
QTEST_APPLESS_MAIN(tst_pagepoint)
#endif
#include "tst_pagepoint.moc"
