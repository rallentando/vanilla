#include <QtTest>

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

    bool RightGestureEnabled() const { return EnableRightGestureLocal(); }
};

class QuickViewHarness : public QuickWebEngineView {
public:
    QuickViewHarness()
        : QuickWebEngineView(nullptr, QStringLiteral("spentbuttons-quick"),
                             QStringList() << QStringLiteral("Private")) {}

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
    void ForwardMouseReleaseToEngine(QMouseEvent *) Q_DECL_OVERRIDE {
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
