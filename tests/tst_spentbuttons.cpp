#include <QtTest>
#include <QVBoxLayout>
#include <memory>

#include "spentbuttons.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "settingspage.hpp"
#include "directorypage.hpp"
#include "treebank.hpp"
#include "lightnode.hpp"

#ifdef WEBENGINEVIEW
#  include <QtWebEngineQuick>
#endif

class tst_spentbuttons : public QObject {
    Q_OBJECT

private slots:
    void aSpentPressSpendsItsRelease();
    void anUnspentPressLeavesItsReleaseAlone();
    void aReleaseIsSpentOnlyOnce();
    void eachButtonIsKeptOnItsOwn();
    void aNewUnspentPressForgetsAnOldSpentOne();
    void aQuickPairSpendsBothReleases();
#ifdef WEBENGINEVIEW
    void eventEaterSpendsTheMappedPressAndRelease();
    void eventEaterSpendsBothQuickPairs();
    void quickViewSpendsBothQuickPairs();
    void viewsShareSpentButtonsInBothDirections();
    void aDirectoryOverridesMouseGestures();
    void eventEaterLetsFocusReachTheEngineChild();
    void quickViewHandsItsFocusToTheEngine();
    void quickViewHidesThePageWithTheWidget();
    void quickViewFreezesOnceHidden();
#endif
};

#ifdef WEBENGINEVIEW
class WidgetsViewHarness : public WebEngineView {
public:
    explicit WidgetsViewHarness(QStringList set = QStringList() << QStringLiteral("Private"))
        : WebEngineView(nullptr, QStringLiteral("spentbuttons-widgets"),
                        set) {}

    static void ResetInput(){
        m_MouseMap.clear();
        m_MouseMap.insert(QStringLiteral("ExtraButton1"),
                          QStringLiteral("NoAction"));
        m_SpentButtons.Settle(Qt::ExtraButton1);
        m_SpentButtons.Settle(Qt::ExtraButton2);
    }

    static bool IsSpent(Qt::MouseButton button){
        return m_SpentButtons.Spent().testFlag(button);
    }

    static void SetRightGestureDefault(bool enabled){
        m_EnableMouseGesture = enabled;
    }

    bool RightGestureEnabled() const { return m_EnableRightGestureLocal; }
};

class QuickViewHarness : public QuickWebEngineView {
public:
    QuickViewHarness()
        : QuickWebEngineView(nullptr, QStringLiteral("spentbuttons-quick"),
                             QStringList() << QStringLiteral("Private")) {}

    static void SetSuspendHiddenViews(const QString &word){
        m_SuspendHiddenViews = word;
    }

    void Press(Qt::MouseButton button){
        QMouseEvent press(QEvent::MouseButtonPress,
                          QPointF(1, 1), QPointF(1, 1),
                          button, button, Qt::NoModifier);
        press.setAccepted(false);
        mousePressEvent(&press);
    }

    void DoubleClick(Qt::MouseButton button){
        QMouseEvent press(QEvent::MouseButtonDblClick,
                          QPointF(1, 1), QPointF(1, 1),
                          button, button, Qt::NoModifier);
        press.setAccepted(false);
        mouseDoubleClickEvent(&press);
    }

    void Release(Qt::MouseButton button){
        QMouseEvent release(QEvent::MouseButtonRelease,
                            QPointF(1, 1), QPointF(1, 1),
                            button, Qt::NoButton, Qt::NoModifier);
        release.setAccepted(false);
        mouseReleaseEvent(&release);
    }

    static bool IsSpent(Qt::MouseButton button){
        return m_SpentButtons.Spent().testFlag(button);
    }

    int EngineReleaseCount() const { return m_EngineReleaseCount; }

private:
    void ForwardMouseReleaseToEngine(QMouseEvent *) override {
        ++m_EngineReleaseCount;
    }

    int m_EngineReleaseCount = 0;
};

class EventEaterHarness : public EventEater {
public:
    EventEaterHarness(WebEngineView *view, QObject *parent)
        : EventEater(view, parent) {}

    bool Filter(QObject *target, QEvent *event){
        return eventFilter(target, event);
    }
};

static bool SendMouse(EventEaterHarness *filter, QWidget *target, QEvent::Type type,
                      Qt::MouseButton button, Qt::MouseButtons buttons){
    QMouseEvent event(type, QPointF(1, 1), QPointF(1, 1),
                      button, buttons, Qt::NoModifier);
    event.setAccepted(false);
    return filter->Filter(target, &event);
}

class FocusCountingChild : public QWidget {
public:
    using QWidget::QWidget;
    int m_FocusIns = 0;
    int m_FocusOuts = 0;
protected:
    void focusInEvent(QFocusEvent *) override { ++m_FocusIns; }
    void focusOutEvent(QFocusEvent *) override { ++m_FocusOuts; }
};
#endif

void tst_spentbuttons::aSpentPressSpendsItsRelease(){
    SpentButtons buttons;
    buttons.Press(Qt::ExtraButton1, true);
    QCOMPARE(buttons.Spent(), Qt::MouseButtons(Qt::ExtraButton1));
    QVERIFY(buttons.Settle(Qt::ExtraButton1));
    QCOMPARE(buttons.Spent(), Qt::MouseButtons(Qt::NoButton));
}

void tst_spentbuttons::anUnspentPressLeavesItsReleaseAlone(){
    SpentButtons buttons;
    buttons.Press(Qt::LeftButton, false);
    QVERIFY(!buttons.Settle(Qt::LeftButton));
    QVERIFY(!buttons.Settle(Qt::ExtraButton2));
}

void tst_spentbuttons::aReleaseIsSpentOnlyOnce(){
    SpentButtons buttons;
    buttons.Press(Qt::ExtraButton1, true);
    QVERIFY(buttons.Settle(Qt::ExtraButton1));
    QVERIFY(!buttons.Settle(Qt::ExtraButton1));
}

void tst_spentbuttons::eachButtonIsKeptOnItsOwn(){
    SpentButtons buttons;
    buttons.Press(Qt::ExtraButton1, true);
    buttons.Press(Qt::LeftButton, false);
    QVERIFY(!buttons.Settle(Qt::LeftButton));
    QVERIFY(buttons.Settle(Qt::ExtraButton1));
}

void tst_spentbuttons::aNewUnspentPressForgetsAnOldSpentOne(){
    SpentButtons buttons;
    buttons.Press(Qt::ExtraButton1, true);
    buttons.Press(Qt::ExtraButton1, false);
    QVERIFY(!buttons.Settle(Qt::ExtraButton1));
}

void tst_spentbuttons::aQuickPairSpendsBothReleases(){
    SpentButtons buttons;
    buttons.Press(Qt::ExtraButton1, true);
    QVERIFY(buttons.Settle(Qt::ExtraButton1));
    buttons.Press(Qt::ExtraButton1, true);
    QVERIFY(buttons.Settle(Qt::ExtraButton1));
    QCOMPARE(buttons.Spent(), Qt::MouseButtons(Qt::NoButton));
}

#ifdef WEBENGINEVIEW
void tst_spentbuttons::eventEaterSpendsTheMappedPressAndRelease(){
    WidgetsViewHarness::ResetInput();
    WidgetsViewHarness view;
    QWidget engineChild(&view);
    EventEaterHarness filter(&view, &engineChild);

    QVERIFY(SendMouse(&filter, &engineChild, QEvent::MouseButtonPress,
                      Qt::ExtraButton1, Qt::ExtraButton1));
    QVERIFY(WidgetsViewHarness::IsSpent(Qt::ExtraButton1));
    QVERIFY(SendMouse(&filter, &engineChild, QEvent::MouseButtonRelease,
                      Qt::ExtraButton1, Qt::NoButton));
    QVERIFY(!WidgetsViewHarness::IsSpent(Qt::ExtraButton1));
}

void tst_spentbuttons::eventEaterSpendsBothQuickPairs(){
    WidgetsViewHarness::ResetInput();
    WidgetsViewHarness view;
    QWidget engineChild(&view);
    EventEaterHarness filter(&view, &engineChild);

    QVERIFY(SendMouse(&filter, &engineChild, QEvent::MouseButtonPress,
                      Qt::ExtraButton1, Qt::ExtraButton1));
    QVERIFY(WidgetsViewHarness::IsSpent(Qt::ExtraButton1));
    QVERIFY(SendMouse(&filter, &engineChild, QEvent::MouseButtonRelease,
                      Qt::ExtraButton1, Qt::NoButton));
    QVERIFY(!WidgetsViewHarness::IsSpent(Qt::ExtraButton1));
    QVERIFY(SendMouse(&filter, &engineChild, QEvent::MouseButtonDblClick,
                      Qt::ExtraButton1, Qt::ExtraButton1));
    QVERIFY(WidgetsViewHarness::IsSpent(Qt::ExtraButton1));
    QVERIFY(SendMouse(&filter, &engineChild, QEvent::MouseButtonRelease,
                      Qt::ExtraButton1, Qt::NoButton));
    QVERIFY(!WidgetsViewHarness::IsSpent(Qt::ExtraButton1));
}

void tst_spentbuttons::quickViewSpendsBothQuickPairs(){
    WidgetsViewHarness::ResetInput();
    QuickViewHarness view;

    view.Press(Qt::ExtraButton1);
    QVERIFY(QuickViewHarness::IsSpent(Qt::ExtraButton1));
    view.Release(Qt::ExtraButton1);
    QVERIFY(!QuickViewHarness::IsSpent(Qt::ExtraButton1));
    QCOMPARE(view.EngineReleaseCount(), 0);

    view.DoubleClick(Qt::ExtraButton1);
    QVERIFY(QuickViewHarness::IsSpent(Qt::ExtraButton1));
    view.Release(Qt::ExtraButton1);
    QVERIFY(!QuickViewHarness::IsSpent(Qt::ExtraButton1));
    QCOMPARE(view.EngineReleaseCount(), 0);

    view.Release(Qt::ExtraButton2);
    QCOMPARE(view.EngineReleaseCount(), 1);
}

void tst_spentbuttons::viewsShareSpentButtonsInBothDirections(){
    WidgetsViewHarness::ResetInput();
    WidgetsViewHarness widgetsView;
    QWidget engineChild(&widgetsView);
    EventEaterHarness filter(&widgetsView, &engineChild);
    QuickViewHarness quickView;

    QVERIFY(SendMouse(&filter, &engineChild, QEvent::MouseButtonPress,
                      Qt::ExtraButton1, Qt::ExtraButton1));
    QVERIFY(QuickViewHarness::IsSpent(Qt::ExtraButton1));
    quickView.Release(Qt::ExtraButton1);
    QVERIFY(!QuickViewHarness::IsSpent(Qt::ExtraButton1));
    QCOMPARE(quickView.EngineReleaseCount(), 0);

    quickView.Press(Qt::ExtraButton1);
    QVERIFY(QuickViewHarness::IsSpent(Qt::ExtraButton1));
    QVERIFY(SendMouse(&filter, &engineChild, QEvent::MouseButtonRelease,
                      Qt::ExtraButton1, Qt::NoButton));
    QVERIFY(!QuickViewHarness::IsSpent(Qt::ExtraButton1));
}

void tst_spentbuttons::aDirectoryOverridesMouseGestures(){
    WidgetsViewHarness disabled(QStringList()
                                << QStringLiteral("Private")
                                << QStringLiteral("!RightGesture"));
    QVERIFY(!disabled.RightGestureEnabled());

    disabled.ApplySpecificSettings(QStringList()
                                   << QStringLiteral("Private")
                                   << QStringLiteral("RightGesture"));
    QVERIFY(disabled.RightGestureEnabled());

    disabled.ApplySpecificSettings(QStringList()
                                   << QStringLiteral("Private")
                                   << QStringLiteral("!MouseGesture"));
    QVERIFY(!disabled.RightGestureEnabled());

    WidgetsViewHarness::SetRightGestureDefault(false);
    disabled.ApplySpecificSettings(QStringList() << QStringLiteral("Private"));
    QVERIFY(!disabled.RightGestureEnabled());
    bool described = false;
    foreach(const QJsonValue &value,
            DirectoryPage::Describe()[QStringLiteral("switches")].toArray()){
        const QJsonObject object = value.toObject();
        if(object[QStringLiteral("key")].toString() == QStringLiteral("rightgesture")){
            described = true;
            QCOMPARE(object[QStringLiteral("absence")].toInt(), 0);
        }
    }
    QVERIFY(described);

    WidgetsViewHarness::SetRightGestureDefault(true);
    disabled.ApplySpecificSettings(QStringList() << QStringLiteral("Private"));
    QVERIFY(disabled.RightGestureEnabled());
    foreach(const QJsonValue &value,
            DirectoryPage::Describe()[QStringLiteral("switches")].toArray()){
        const QJsonObject object = value.toObject();
        if(object[QStringLiteral("key")].toString() == QStringLiteral("rightgesture"))
            QCOMPARE(object[QStringLiteral("absence")].toInt(), 1);
    }

    TreeBank::Initialize();
    ViewNode *outer = TreeBank::GetViewRoot()->MakeChild();
    outer->SetTitle(QStringLiteral("outer"));
    ViewNode *profile = outer->MakeChild();
    profile->SetTitle(QStringLiteral("profile;ID"));
    ViewNode *tab = profile->MakeChild();
    tab->SetHoldView(true);
    disabled.SetViewNode(tab);
    tab->SetView(&disabled);

    const QString after = QStringLiteral("outer;!RightGesture");
    outer->SetTitle(after);
    TreeBank::ReconfigureDirectory(outer, QStringLiteral("outer"), after);
    QVERIFY(!disabled.RightGestureEnabled());
}

void tst_spentbuttons::eventEaterLetsFocusReachTheEngineChild(){
    WidgetsViewHarness view;
    FocusCountingChild engineChild(&view);
    engineChild.installEventFilter(new EventEater(&view, &engineChild));

    QFocusEvent in(QEvent::FocusIn, Qt::OtherFocusReason);
    QApplication::sendEvent(&engineChild, &in);
    QCOMPARE(engineChild.m_FocusIns, 1);

    QFocusEvent out(QEvent::FocusOut, Qt::OtherFocusReason);
    QApplication::sendEvent(&engineChild, &out);
    QCOMPARE(engineChild.m_FocusOuts, 1);
}

void tst_spentbuttons::quickViewHandsItsFocusToTheEngine(){
    QuickViewHarness view;
    QQuickItem *engine = view.rootObject();
    QVERIFY(engine);
    QQuickWidget *widget = &view;
    widget->resize(200, 200);
    widget->show();
    widget->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(widget));
    view.setFocus();
    QTRY_VERIFY(engine->hasActiveFocus());
}

void tst_spentbuttons::quickViewHidesThePageWithTheWidget(){
    QWidget top;
    QVBoxLayout layout(&top);
    auto *view = new QuickViewHarness;
    layout.addWidget(view);
    top.resize(200, 200);
    QQuickItem *engine = view->rootObject();
    QVERIFY(engine);

    const auto eval = [view](const QString &code){
        struct Answer { QVariant value; bool done = false; };
        const auto answer = std::make_shared<Answer>();
        view->CallWithEvaluatedJavaScriptResult(code, [answer](QVariant v){
            answer->value = v;
            answer->done = true;
        });
        if(!QTest::qWaitFor([answer]{ return answer->done; }, 5000)) return QVariant();
        return answer->value;
    };

    QVERIFY(!engine->isVisible());
    engine->setProperty("url", QUrl(QStringLiteral("about:blank")));
    QTRY_VERIFY_WITH_TIMEOUT(!engine->property("loading").toBool() &&
                             engine->property("url").toUrl() == QUrl(QStringLiteral("about:blank")), 10000);
    QCOMPARE(eval(QStringLiteral("document.visibilityState")).toString(), QStringLiteral("hidden"));
    eval(QStringLiteral("window.turns = 0;"
                        "document.addEventListener('visibilitychange', () => window.turns++); 0"));

    top.show();
    top.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&top));
    view->setFocus();
    QTRY_VERIFY(engine->isVisible());
    QTRY_COMPARE(eval(QStringLiteral("document.visibilityState")).toString(), QStringLiteral("visible"));
    QTRY_VERIFY(eval(QStringLiteral("document.hasFocus()")).toBool());

    view->hide();
    QVERIFY(!engine->isVisible());
    QTRY_COMPARE(eval(QStringLiteral("document.visibilityState")).toString(), QStringLiteral("hidden"));
    view->show();
    view->setFocus();
    QTRY_COMPARE(eval(QStringLiteral("document.visibilityState")).toString(), QStringLiteral("visible"));
    QTRY_VERIFY(engine->hasActiveFocus());
    QTRY_VERIFY(eval(QStringLiteral("document.hasFocus()")).toBool());

    top.hide();
    QVERIFY(!engine->isVisible());
    QTRY_COMPARE(eval(QStringLiteral("document.visibilityState")).toString(), QStringLiteral("hidden"));
    top.show();
    QTRY_COMPARE(eval(QStringLiteral("document.visibilityState")).toString(), QStringLiteral("visible"));

    QTRY_COMPARE(eval(QStringLiteral("window.turns")).toInt(), 5);
}

void tst_spentbuttons::quickViewFreezesOnceHidden(){
    const QString old = View::SuspendHiddenViews();
    const auto restore = qScopeGuard([&]{ QuickViewHarness::SetSuspendHiddenViews(old); });
    QuickViewHarness::SetSuspendHiddenViews(QStringLiteral("Frozen"));

    QuickViewHarness view;
    QQuickItem *engine = view.rootObject();
    QVERIFY(engine);
    QQuickWidget *widget = &view;
    widget->resize(200, 200);
    widget->show();
    engine->setProperty("url", QUrl(QStringLiteral("about:blank")));
    QTRY_VERIFY_WITH_TIMEOUT(!engine->property("loading").toBool() &&
                             engine->property("url").toUrl() == QUrl(QStringLiteral("about:blank")), 10000);
    QCOMPARE(engine->property("lifecycleState").toInt(), 0);

    widget->hide();
    view.Suspend();
    QTRY_COMPARE(engine->property("lifecycleState").toInt(), 1);

    widget->show();
    view.WakeUp();
    QTRY_COMPARE(engine->property("lifecycleState").toInt(), 0);
}
#endif

#ifdef WEBENGINEVIEW
int main(int argc, char **argv){
    SettingsSchemeHandler::RegisterScheme();
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    tst_spentbuttons test;
    return QTest::qExec(&test, argc, argv);
}
#else
QTEST_APPLESS_MAIN(tst_spentbuttons)
#endif
#include "tst_spentbuttons.moc"
