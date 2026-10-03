#include "switch.hpp"
#include "const.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "gadgets.hpp"
#include "testsupport.hpp"
#include "windowframereveal.hpp"
#include "settingspage.hpp"
#include <QJsonDocument>
#include <QJsonObject>

#include <QtTest>
#include <QPropertyAnimation>
#include <QStyle>
#include <QMenuBar>
#include <QMenu>
#include <QToolButton>
#include <QLineEdit>
#include <QTimer>
#include <QScreen>
#include <memory>

class IgnorePlatformActivation : public QObject {
    bool eventFilter(QObject *, QEvent *event) override {
        return event->type() == QEvent::ActivationChange;
    }
};

class KeyboardHoldingView : public QObject, public View {
    Q_OBJECT

public:
    KeyboardHoldingView() : QObject(nullptr), View(nullptr), m_TakenBack(0) {}

    QObject *base() Q_DECL_OVERRIDE { return this;}
    QObject *page() Q_DECL_OVERRIDE { return this;}

    QSize size() Q_DECL_OVERRIDE { return QSize();}
    void resize(QSize) Q_DECL_OVERRIDE {}
    void show() Q_DECL_OVERRIDE {}
    void hide() Q_DECL_OVERRIDE {}
    void raise() Q_DECL_OVERRIDE {}
    void lower() Q_DECL_OVERRIDE {}
    void repaint() Q_DECL_OVERRIDE {}
    bool visible() Q_DECL_OVERRIDE { return false;}
    void setFocus(Qt::FocusReason = Qt::OtherFocusReason) Q_DECL_OVERRIDE {}
    void TakeKeyboardBack() Q_DECL_OVERRIDE { m_TakenBack++;}

    int m_TakenBack;

public slots:
    void DownloadSuggest(const QUrl&){}

signals:
    void urlChanged(const QUrl&);
    void loadFinished(bool);
    void SuggestResult(const QByteArray&);
};

class tst_windowframe : public QObject {
    Q_OBJECT
    std::unique_ptr<MainWindow> m_Custom;
    std::unique_ptr<MainWindow> m_Native;
private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        TestSupport::DisableWidgetAnimation();
        TreeBank::Initialize();
    }

    void maximizedFrameIsCompactAndRestoresLayout(){
        Application::GlobalSettings().setValue("application/@EnableFramelessWindow", true);
        Application::LoadGlobalSettings();
        m_Custom = std::make_unique<MainWindow>(901, QPoint(100, 100));
        MainWindow &window = *m_Custom;
        window.setAnimated(false);
        window.resize(1200, 800);
        window.GetTreeBank()->show();
        TitleBar *title = nullptr;
        for(QWidget *widget : QApplication::topLevelWidgets())
            if(auto *bar = qobject_cast<TitleBar*>(widget)) title = bar;
        QVERIFY(title);
        QTimer *poll = window.findChild<QTimer*>(QStringLiteral("TitleBarRevealTimer"));
        QVERIFY(poll);
        QVERIFY(!poll->isActive());
        const int height = window.ScaleByDevice(32);
        QCOMPARE(title->height(), height);
        QCOMPARE(window.contentsMargins().top(), 0);
        window.showMaximized();
        QCoreApplication::processEvents();
        QCOMPARE(window.contentsMargins().top(), 0);
        QCOMPARE(title->width(), title->ScaleByDevice(32)+title->ScaleByDevice(28)*4+title->ScaleByDevice(6));
        QVERIFY(poll->isActive());
        QTRY_COMPARE(title->geometry().right(), window.geometry().right());
        QCOMPARE(title->geometry().top(), window.geometry().top());
        QVERIFY(!title->isVisible());

        IgnorePlatformActivation activation;
        window.installEventFilter(&activation);
        const QPoint edge(window.geometry().center().x(), window.screen()->geometry().top());
        window.ApplyTitleBarVisibility(edge + QPoint(0, 1), true, false);
        QVERIFY(!title->isVisible());
        window.ApplyTitleBarVisibility(edge, true, false);
        QVERIFY(title->isVisible());
        window.ApplyTitleBarVisibility(title->geometry().center(), true, false);
        QVERIFY(title->isVisible());
        window.ApplyTitleBarVisibility(title->geometry().bottomRight()+QPoint(0, 1), true, false);
        QVERIFY(!title->isVisible());
        window.ApplyTitleBarVisibility(edge, true, false);
        QVERIFY(title->isVisible());
        window.ApplyTitleBarVisibility(edge, false, false);
        QVERIFY(!title->isVisible());
        window.removeEventFilter(&activation);

        window.showMinimized();
        QCoreApplication::processEvents();
        QVERIFY(!poll->isActive());
        QVERIFY(!title->isVisible());
        window.showMaximized();
        QCoreApplication::processEvents();
        QCOMPARE(window.contentsMargins().top(), 0);
        QVERIFY(!title->isVisible());

        const QImage painted = title->grab().toImage();
        QVERIFY(painted.pixelColor(painted.width()/2, painted.height()/2).alpha() > 0);
        window.SetWindowTitle(QStringLiteral("This title must not appear in compact controls"));
        QCOMPARE(title->grab().toImage(), painted);

        bool popupOpened = false;
        QTimer popupCloser;
        connect(&popupCloser, &QTimer::timeout, &window, [&](){
            if(QWidget *popup = QApplication::activePopupWidget()){
                popupOpened = true;
                popup->close();
            }
        });
        popupCloser.start(0);
        const QPointF first(title->ScaleByDevice(14), title->height()/2);
        const QPointF firstGlobal = title->mapToGlobal(first.toPoint());
        QMouseEvent firstPress(QEvent::MouseButtonPress, first, firstGlobal,
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent firstRelease(QEvent::MouseButtonRelease, first, firstGlobal,
                                 Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(title, &firstPress);
        QCoreApplication::sendEvent(title, &firstRelease);
        popupCloser.stop();
        QVERIFY(!popupOpened);
        QVERIFY(window.GetTreeBank()->GetGadgets()->IsActive());
        window.GetTreeBank()->GetGadgets()->Deactivate();

        window.SetFullScreen(true);
        QCoreApplication::processEvents();
        QVERIFY(!poll->isActive());
        QCOMPARE(window.contentsMargins().top(), 0);
        QVERIFY(!title->isVisible());
        window.RaiseAllEdgeWidgets();
        QVERIFY(!title->isVisible());
        window.SetFullScreen(false);
        QCoreApplication::processEvents();
        QVERIFY(window.isMaximized());
        QCOMPARE(window.contentsMargins().top(), 0);
        QVERIFY(!title->isVisible());

        window.showNormal();
        QCoreApplication::processEvents();
        QCOMPARE(window.contentsMargins().top(), 0);
        QCOMPARE(title->geometry().bottom()+1, window.geometry().top());
        QVERIFY(!poll->isActive());

        for(bool menuVisible : {true, false}){
            window.SetMenuBar(menuVisible);
            for(Qt::ToolBarArea area : {Qt::TopToolBarArea, Qt::LeftToolBarArea}){
                window.addToolBar(area, window.GetTreeBar());
                QCoreApplication::processEvents();
                QTRY_VERIFY(window.GetTreeBank()->geometry().left() >= 0);
                const QRect barBefore = window.GetTreeBar()->geometry();
                const QRect pageBefore = window.GetTreeBank()->geometry();
                const QRect toolBefore = window.GetToolBar()->geometry();
                window.showMaximized();
                QCoreApplication::processEvents();
                QCOMPARE(window.contentsMargins(), QMargins());
                QCOMPARE(title->width(), title->ScaleByDevice(32)+title->ScaleByDevice(28)*4+title->ScaleByDevice(6));
                QTRY_COMPARE(title->geometry().right(), window.geometry().right());
                QVERIFY(title->geometry().left() > window.geometry().center().x());
                window.showNormal();
                QCoreApplication::processEvents();
                QCOMPARE(window.contentsMargins(), QMargins());
                QTRY_COMPARE(window.GetTreeBar()->geometry(), barBefore);
                QTRY_COMPARE(window.GetTreeBank()->geometry(), pageBefore);
                QTRY_COMPARE(window.GetToolBar()->geometry(), toolBefore);
                QCOMPARE(title->width(), window.width());
            }
        }

        const QPointF button(title->width()-title->ScaleByDevice(50), title->height()-2);
        const QPointF global = title->mapToGlobal(button.toPoint());
        QMouseEvent press(QEvent::MouseButtonPress, button, global,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease, button, global,
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(title, &press);
        QCoreApplication::sendEvent(title, &release);
        QVERIFY(window.isMaximized());
        window.showNormal();

        window.ToggleFullScreen();
        QCoreApplication::processEvents();
        QCOMPARE(window.contentsMargins().top(), 0);
        QVERIFY(!title->isVisible());
        window.SetFullScreen(true);
        window.SetFullScreen(false);
        QCoreApplication::processEvents();
        QVERIFY(window.isFullScreen());
        QVERIFY(!title->isVisible());
        window.ToggleFullScreen();
        QCoreApplication::processEvents();
        QVERIFY(title->isVisible());
        QCOMPARE(window.style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, &window), 0);
        QVERIFY(window.findChildren<QPropertyAnimation*>().isEmpty());
    }

    void onlyTheScreenEdgeRevealsMaximizedControls(){
        WindowFrameReveal state;
        const QRect strip(-1920, -200, 1920, 40);
        const QPoint edge(-1000, -200);
        const QPoint below(-1000, -199);
        const QPoint bottom(-20, -161);
        const QPoint outside(-20, -160);
        QVERIFY(!state.Update(false, true, strip, edge, false));
        QVERIFY(!state.Update(true, false, strip, edge, false));
        QVERIFY(!state.Update(true, true, strip, below, false));
        QVERIFY(!state.Update(true, true, strip, QPoint(0, -200), false));
        QVERIFY(state.Update(true, true, strip, edge, false));
        QVERIFY(state.Update(true, true, strip, bottom, false));
        QVERIFY(!state.Update(true, true, strip, outside, false));
        QVERIFY(!state.Update(true, true, strip, below, false));
        QVERIFY(state.Update(true, true, strip, edge, false));
        QVERIFY(state.Update(true, true, strip, outside, true));
        QVERIFY(!state.Update(true, true, strip, outside, false));
        QVERIFY(state.Update(true, true, strip, edge, false));
        QVERIFY(!state.Update(true, false, strip, edge, true));
        QVERIFY(!state.Update(true, true, strip, below, false));
        QVERIFY(state.Update(true, true, strip, edge, false));
        QVERIFY(!state.Update(false, true, strip, edge, true));
        QVERIFY(!state.Update(true, true, strip, below, false));
    }

    void nativeFrameDoesNotReserveSpace(){
        Application::GlobalSettings().setValue("application/@EnableFramelessWindow", false);
        Application::LoadGlobalSettings();
        m_Native = std::make_unique<MainWindow>(902, QPoint(100, 100));
        MainWindow &window = *m_Native;
        window.showMaximized();
        QCoreApplication::processEvents();
        QCOMPARE(window.contentsMargins(), QMargins());
    }
    void settingsCollectionReachesEveryWindow(){
        const QString key = QStringLiteral("gadgets/thumblist/@NodeCollectionType");
        Application::GlobalSettings().setValue(key, "Foldable");
        Application::GlobalSettings().remove("mainwindow/tableview903");
        Application::GlobalSettings().remove("mainwindow/tableview904");
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        GraphicsTableView::LoadSettings();
        std::unique_ptr<MainWindow> firstWindow(Application::NewWindow(903));
        std::unique_ptr<MainWindow> secondWindow(Application::NewWindow(904));
        const auto unregister = qScopeGuard([&]{
            Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
            Application::RemoveWindow(903);
            Application::RemoveWindow(904);
        });
        auto *first = firstWindow->GetTreeBank()->GetGadgets();
        auto *second = secondWindow->GetTreeBank()->GetGadgets();
        first->hide();
        second->hide();
        QCOMPARE(first->GetStat().first().toInt(), 3);
        QCOMPARE(second->GetStat().first().toInt(), 3);
        auto request = [&](const QString &endpoint, const QString &setting, const QJsonValue &value){
            return QJsonDocument::fromJson(VanillaPage::Answer(
                QUrl(QStringLiteral("vanilla://settings/api/") + endpoint), QByteArrayLiteral("POST"),
                QJsonDocument(QJsonObject{{"key", setting}, {"value", value}}).toJson(),
                QUrl(QStringLiteral("vanilla://settings/"))).m_Body).object();
        };
        auto expect = [&](int type){
            QCOMPARE(first->GetStat().first().toInt(), type);
            QCOMPARE(second->GetStat().first().toInt(), type);
            QCOMPARE(first->GetZoomFactor(), 0.75f);
            QCOMPARE(second->GetZoomFactor(), 1.25f);
        };
        first->SetStat({"0", "0.75"});
        second->SetStat({"2", "1.25"});
        for(const QString &type : {QStringLiteral("Foldable"), QStringLiteral("Recursive"),
                                  QStringLiteral("Flat")}){
            QVERIFY(!request("set", key, type).contains("error"));
            expect(int(GraphicsTableView::NodeCollectionTypeFromName(type)));
        }
        first->SetStat({"3", "0.75"});
        second->SetStat({"3", "1.25"});
        QVERIFY(!request("set", key, "Flat").contains("error"));
        expect(0);
        first->SetStat({"3", "0.75"});
        second->SetStat({"3", "1.25"});
        QVERIFY(!request("set", "gadgets/thumblist/@RightClickToRenameNode", true).contains("error"));
        expect(3);
        QVERIFY(request("set", key, "invalid").contains("error"));
        expect(3);
        QVERIFY(!request("reset", key, QJsonValue()).contains("error"));
        expect(0);
    }

    void theOverviewTakesTheKeyboardBackFromAToolbarButton(){
        if(!m_Custom){
            m_Custom = std::make_unique<MainWindow>(901, QPoint(100, 100));
            m_Custom->setAnimated(false);
            m_Custom->resize(1200, 800);
        }
        MainWindow *window = m_Custom.get();
        TreeBank *treeBank = window->GetTreeBank();
        QToolButton *button = new QToolButton(window);
        button->setObjectName(QStringLiteral("PinnedExtension_test"));
        button->show();
        window->show();
        window->activateWindow();
        QTRY_VERIFY(window->isActiveWindow());

        Gadgets *gadgets = treeBank->GetGadgets();
        gadgets->Activate(Gadgets::AccessKey);
        button->setFocus(Qt::ActiveWindowFocusReason);
        QVERIFY2(button->hasFocus(),
                 qPrintable(QStringLiteral("focus widget: %1").arg(
                     QApplication::focusWidget()
                     ? QString::fromLatin1(QApplication::focusWidget()->metaObject()->className())
                     : QStringLiteral("none"))));
        QVERIFY(!treeBank->GetView()->hasFocus());

        QTRY_VERIFY(treeBank->GetView()->hasFocus());
        QVERIFY(!button->hasFocus());
        QVERIFY(gadgets->hasFocus());
        gadgets->Deactivate();
        QCoreApplication::processEvents();

        QLineEdit *editor = new QLineEdit(window);
        editor->setObjectName(QStringLiteral("CommandLine_test"));
        editor->show();
        gadgets->Activate(Gadgets::AccessKey);
        editor->setFocus(Qt::OtherFocusReason);
        QVERIFY(editor->hasFocus());
        QTest::qWait(100);
        QVERIFY(editor->hasFocus());
        QVERIFY(!treeBank->GetView()->hasFocus());
        gadgets->Deactivate();
    }

    void addressBarPressAsksTheViewForTheKeyboard(){
        if(!m_Custom){
            m_Custom = std::make_unique<MainWindow>(901, QPoint(100, 100));
            m_Custom->setAnimated(false);
            m_Custom->resize(1200, 800);
        }
        MainWindow *window = m_Custom.get();
        window->show();
        ToolBar *toolBar = window->GetToolBar();
        QVERIFY(toolBar);
        LineEdit *field = toolBar->findChild<LineEdit*>();
        QVERIFY(field);

        std::shared_ptr<KeyboardHoldingView> view = std::make_shared<KeyboardHoldingView>();
        toolBar->Connect(view);
        QCOMPARE(view->m_TakenBack, 0);

        QTest::mousePress(field, Qt::LeftButton, Qt::NoModifier, field->rect().center());
        QCOMPARE(view->m_TakenBack, 1);
        QTest::mouseRelease(field, Qt::LeftButton, Qt::NoModifier, field->rect().center());
        QCOMPARE(view->m_TakenBack, 1);

        QTest::mousePress(toolBar, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
        QTest::mouseRelease(toolBar, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
        QCOMPARE(view->m_TakenBack, 1);

        toolBar->Disconnect(view);
        QTest::mousePress(field, Qt::LeftButton, Qt::NoModifier, field->rect().center());
        QTest::mouseRelease(field, Qt::LeftButton, Qt::NoModifier, field->rect().center());
        QCOMPARE(view->m_TakenBack, 1);
    }

    void activateIfNeededAsksTheWindowSystemAndNotQt(){
        if(!m_Custom){
            m_Custom = std::make_unique<MainWindow>(901, QPoint(100, 100));
            m_Custom->setAnimated(false);
            m_Custom->resize(1200, 800);
        }
        MainWindow *window = m_Custom.get();
        window->show();
        QWidget other;
        other.resize(200, 200);
        other.show();
        other.activateWindow();
        QTRY_VERIFY(other.isActiveWindow());
        QVERIFY(!window->isActiveWindow());

        MainWindow::ActivateWindowIfNeeded(window, window->winId());
        QCoreApplication::processEvents();
        QVERIFY(!window->isActiveWindow());
        QVERIFY(other.isActiveWindow());

        QWidget *child = new QWidget(window);
        child->show();
        MainWindow::ActivateWindowIfNeeded(child, window->winId());
        QCoreApplication::processEvents();
        QVERIFY(!window->isActiveWindow());
        QVERIFY(other.isActiveWindow());
        delete child;

        MainWindow::ActivateWindowIfNeeded(window, other.winId());
        QTRY_VERIFY(window->isActiveWindow());
    }
};

QTEST_MAIN(tst_windowframe)
#include "tst_windowframe.moc"
