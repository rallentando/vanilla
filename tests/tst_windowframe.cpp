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
#include "extensionbar.hpp"
#include <QCompleter>
#include <QAbstractItemView>
#include "receiver.hpp"
#include "siteinfo.hpp"
#include "settingspage.hpp"
#include "dialog.hpp"
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
#include <QProxyStyle>
#include <QStyleFactory>
#include <memory>
#include <functional>
#include <QMenu>

class SlidingStyle : public QProxyStyle {
public:
    SlidingStyle() : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion"))) {}
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *ret = nullptr) const override {
        if(hint == QStyle::SH_Widget_Animation_Duration) return 200;
        return QProxyStyle::styleHint(hint, option, widget, ret);
    }
};

class IgnorePlatformActivation : public QObject {
    bool eventFilter(QObject *, QEvent *event) override {
        return event->type() == QEvent::ActivationChange;
    }
};

class KeyboardHoldingView : public QObject, public View {
    Q_OBJECT

public:
    KeyboardHoldingView() : QObject(nullptr), View(nullptr), m_TakenBack(0) {}

    QObject *base() override { return this;}
    QObject *page() override { return this;}

    QSize size() override { return QSize();}
    void resize(QSize) override {}
    void show() override {}
    void hide() override {}
    void raise() override {}
    void lower() override {}
    void repaint() override {}
    bool visible() override { return false;}
    void setFocus(Qt::FocusReason = Qt::OtherFocusReason) override { if(m_OnFocus) m_OnFocus();}
    void TakeKeyboardBack() override { m_TakenBack++;}
    QUrl url() override { return m_Url;}
    using View::Load;
    void Load(const QString &text) override {
        m_Loaded << text;
        if(m_OnLoad) m_OnLoad();
    }

    int m_TakenBack;
    QUrl m_Url;
    QStringList m_Loaded;
    std::function<void()> m_OnFocus;
    std::function<void()> m_OnLoad;

public slots:
    void DownloadSuggest(const QUrl&){}

signals:
    void urlChanged(const QUrl&);
    void loadStarted();
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
        Application::GlobalSettings().setValue("application/@EnableCustomTitleBar", true);
        Application::LoadGlobalSettings();
        Application::GlobalSettings().remove(QStringLiteral("mainwindow/titlebartoolbar901"));
        m_Custom = std::make_unique<MainWindow>(901, QPoint(100, 100));
        MainWindow &window = *m_Custom;
        QVERIFY(window.GetToolBar()->isHidden());
        QVERIFY(Application::GlobalSettings().value(QStringLiteral("mainwindow/titlebartoolbar901")).toBool());
        window.GetToolBar()->show();
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
#if defined(Q_OS_WIN)
        TitleBar *inner = window.findChild<TitleBar*>();
        QVERIFY(inner);
        QCOMPARE(inner->GetMode(), TitleBar::Inner);
        QCOMPARE(title->GetMode(), TitleBar::Strip);
        QVERIFY(!inner->internalWinId());
        QVERIFY(window.EdgeWidgets().isEmpty());
        QCOMPARE(window.contentsMargins().top(), height);
        QCOMPARE(inner->geometry(), QRect(0, 0, window.width(), height));
        QVERIFY(!inner->isHidden());
#else
        QCOMPARE(window.contentsMargins().top(), 0);
#endif
        window.showMaximized();
        QCoreApplication::processEvents();
        QCOMPARE(window.contentsMargins().top(), 0);
#if defined(Q_OS_WIN)
        QVERIFY(inner->isHidden());
#endif
        QCOMPARE(title->width(), title->CompactWidth());
        QVERIFY(poll->isActive());
        QTRY_COMPARE(title->geometry().right(), window.geometry().right());
        QCOMPARE(title->geometry().top(), window.screen()->geometry().top());
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
        QVERIFY(title->ButtonRect(TitleBar::ExpandButton).left() == 0);
        QVERIFY(title->ButtonRect(TitleBar::MenuButton).isEmpty());
        {
            const QSize was = title->size();
            title->resize(title->CompactWidth() - 1, title->height());
            const QRect lead = title->ButtonRect(TitleBar::ExpandButton);
            QVERIFY(!lead.isEmpty());
            QVERIFY(title->ButtonRect(TitleBar::ViewTreeButton).isEmpty());
            for(TitleBar::Button each : {TitleBar::ShadeButton, TitleBar::MinimizeButton,
                                         TitleBar::MaximizeButton, TitleBar::CloseButton}){
                const QRect r = title->ButtonRect(each);
                QVERIFY(!r.isEmpty());
                QVERIFY(r.left() > lead.right());
            }
            title->resize(was);
        }
        const QPointF first(title->ButtonRect(TitleBar::ViewTreeButton).center());
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
#if defined(Q_OS_WIN)
        const QMargins normalMargins(0, height, 0, 0);
        QCOMPARE(window.contentsMargins(), normalMargins);
        QVERIFY(!inner->isHidden());
        QVERIFY(!title->isVisible());
        QCOMPARE(title->geometry().top(), window.geometry().top());
#else
        const QMargins normalMargins;
        QCOMPARE(window.contentsMargins().top(), 0);
        QCOMPARE(title->geometry().bottom()+1, window.geometry().top());
#endif
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
                QCOMPARE(title->width(), title->CompactWidth());
                QTRY_COMPARE(title->geometry().right(), window.geometry().right());
                QVERIFY(title->geometry().left() > window.geometry().center().x());
                window.showNormal();
                QCoreApplication::processEvents();
                QCOMPARE(window.contentsMargins(), normalMargins);
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
#if defined(Q_OS_WIN)
        QVERIFY(!inner->isHidden());
        QCOMPARE(window.contentsMargins(), normalMargins);
        QVERIFY(!title->isVisible());

        const QRect normal = window.geometry();
        window.Shade();
        QCoreApplication::processEvents();
        QVERIFY(window.IsShaded());
        QVERIFY(title->isVisible());
        QCOMPARE(title->geometry(), QRect(normal.topLeft(), QSize(normal.width(), height)));
        QCOMPARE(window.geometry(), normal);
        QCOMPARE(window.contentsMargins(), normalMargins);
        {
            const QImage card = title->grab().toImage();
            QCOMPARE(card.pixelColor(0, 0).alpha(), 0);
            QCOMPARE(card.pixelColor(card.width() - 1, card.height() - 1).alpha(), 0);
            QVERIFY(card.pixelColor(card.width() / 2, card.height() / 2).alpha() > 0);
        }
        window.Unshade();
        QCoreApplication::processEvents();
        QVERIFY(!title->isVisible());
        window.Shade();
        window.showMaximized();
        QCoreApplication::processEvents();
        QVERIFY(!window.IsShaded());
        window.showNormal();
        QCoreApplication::processEvents();
#else
        QVERIFY(title->isVisible());
#endif
        QCOMPARE(window.style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, &window), 0);
        QVERIFY(window.findChildren<QPropertyAnimation*>().isEmpty());
    }

    void theBarIsTheCaptionButItsButtons(){
        using namespace WindowFrameHit;
        Layout layout;
        layout.width = 1000;
        layout.border = 8;
        layout.bar = QRect(0, 0, 1000, 40);
        layout.maximize = QRect(885, 0, 57, 40);
        layout.buttons = {QRect(0, 0, 50, 40), QRect(942, 0, 58, 40)};
        QCOMPARE(At(layout, QPoint(500, 0)), Top);
        QCOMPARE(At(layout, QPoint(500, 7)), Top);
        QCOMPARE(At(layout, QPoint(15, 3)), TopLeft);
        QCOMPARE(At(layout, QPoint(16, 3)), Top);
        QCOMPARE(At(layout, QPoint(984, 3)), TopRight);
        QCOMPARE(At(layout, QPoint(983, 3)), Top);
        QCOMPARE(At(layout, QPoint(500, 8)), Caption);
        QCOMPARE(At(layout, QPoint(500, 39)), Caption);
        QCOMPARE(At(layout, QPoint(20, 20)), Client);
        QCOMPARE(At(layout, QPoint(960, 20)), Client);
        QCOMPARE(At(layout, QPoint(900, 20)), MaximizeButton);
        QCOMPARE(At(layout, QPoint(900, 3)), Top);
        QCOMPARE(At(layout, QPoint(500, 40)), Client);
        layout.resizable = false;
        QCOMPARE(At(layout, QPoint(500, 0)), Caption);
        QCOMPARE(At(layout, QPoint(900, 0)), MaximizeButton);
        QCOMPARE(At(layout, QPoint(2, 2)), Client);
        layout.bar = QRect();
        layout.maximize = QRect();
        QCOMPARE(At(layout, QPoint(500, 20)), Client);
        layout.bar = QRect(0, 0, 1000, 40);
        QCOMPARE(At(layout, QPoint(900, 20)), Caption);
        for(long button : {3L, 8L, 9L, 20L, 21L})
            QCOMPARE(SystemAnswer(button), 2L);
        for(long kept : {2L, 10L, 12L, 15L, 17L})
            QCOMPARE(SystemAnswer(kept), kept);
    }

    void aNarrowBarDropsButtonsFromTheLeftOfTheRow(){
        Application::GlobalSettings().setValue("application/@EnableCustomTitleBar", true);
        Application::LoadGlobalSettings();
        MainWindow &window = *m_Custom;
        window.showNormal();
        TitleBar bar(&window, TitleBar::Strip);
        const int side = bar.ScaleByDevice(40);
        const int caption = bar.ScaleByDevice(46);
        bar.resize(side + bar.ScaleByDevice(32) + caption*3 + side*2, bar.ScaleByDevice(32));
        for(TitleBar::Button each : {TitleBar::MenuButton, TitleBar::ViewTreeButton, TitleBar::ShadeButton,
                                     TitleBar::MinimizeButton, TitleBar::MaximizeButton, TitleBar::CloseButton})
            QVERIFY(!bar.ButtonRect(each).isEmpty());
        QCOMPARE(bar.ButtonRect(TitleBar::CloseButton).right(), bar.width()-1);
        QCOMPARE(bar.ButtonAt(bar.ButtonRect(TitleBar::MaximizeButton).center()), TitleBar::MaximizeButton);
        bar.resize(bar.width()-1, bar.height());
        QVERIFY(bar.ButtonRect(TitleBar::ViewTreeButton).isEmpty());
        QVERIFY(!bar.ButtonRect(TitleBar::ShadeButton).isEmpty());
        bar.resize(side + bar.ScaleByDevice(32) + caption, bar.height());
        QVERIFY(bar.ButtonRect(TitleBar::MaximizeButton).isEmpty());
        QVERIFY(!bar.ButtonRect(TitleBar::CloseButton).isEmpty());
        QVERIFY(!bar.ButtonRect(TitleBar::MenuButton).isEmpty());
        bar.resize(side, bar.height());
        QVERIFY(!bar.ButtonRect(TitleBar::CloseButton).isEmpty());
    }

    void theMaximizedStripWidensWithItsTitle(){
        MainWindow &window = *m_Custom;
        window.showMaximized();
        QCoreApplication::processEvents();
        TitleBar *strip = nullptr;
        for(QWidget *widget : QApplication::topLevelWidgets())
            if(auto *bar = qobject_cast<TitleBar*>(widget))
                if(bar->GetMode() == TitleBar::Strip) strip = bar;
        QVERIFY(strip);
        auto press = [&](QPoint at){
            QMouseEvent down(QEvent::MouseButtonPress, at, strip->mapToGlobal(at),
                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(strip, &down);
            QMouseEvent up(QEvent::MouseButtonRelease, at, strip->mapToGlobal(at),
                           Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(strip, &up);
        };
        const QRect expander = strip->ButtonRect(TitleBar::ExpandButton);
        QVERIFY(!expander.isEmpty());
        QVERIFY(!window.IsStripExpanded());
        QCOMPARE(strip->width(), qMin(window.width(), strip->CompactWidth()));

        IgnorePlatformActivation activation;
        window.installEventFilter(&activation);
        const QPoint away = window.geometry().center();

        press(expander.center());
        QVERIFY(window.IsStripExpanded());
        window.ApplyTitleBarVisibility(away, true, false);
        QCOMPARE(strip->width(), window.width());
        QVERIFY(strip->isVisible());
        QVERIFY(strip->windowHandle());
        QCOMPARE(strip->windowHandle()->transientParent(), window.windowHandle());
        QVERIFY(!strip->EditRect().isEmpty());
        QVERIFY(!strip->ButtonRect(TitleBar::MenuButton).isEmpty());
        QCOMPARE(strip->ButtonRect(TitleBar::MenuButton).left(), 0);
        QVERIFY(strip->ButtonRect(TitleBar::ExpandButton).left() > strip->TitleRect().right());
        QCOMPARE(strip->ButtonRect(TitleBar::ExpandButton).right() + 1,
                 strip->ButtonRect(TitleBar::ViewTreeButton).left());
        window.ApplyTitleBarVisibility(away, false, false);
        QVERIFY(!strip->isVisible());
        QVERIFY(window.IsStripExpanded());
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(strip->isVisible());

        press(strip->EditRect().center());
        QVERIFY(strip->IsEditing());
        QVERIFY(strip->GetAddressDisplay());
        Receiver *line = window.GetTreeBank()->GetReceiver();
        QVERIFY(line && line->IsAnchored());
        line->OnAborted();
        QVERIFY(!strip->IsEditing());
        QVERIFY(window.IsStripExpanded());
        for(int turn = 0; turn < 3; turn++) QCoreApplication::processEvents();
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(strip->isVisible());

        {
            const QRect before = window.geometry();
            const QPoint from = strip->EditRect().center();
            QMouseEvent down(QEvent::MouseButtonPress, from, strip->mapToGlobal(from),
                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(strip, &down);
            for(int step = 1; step <= 3; step++){
                const QPoint to = from + QPoint(QApplication::startDragDistance() * step + 3, 0);
                QMouseEvent move(QEvent::MouseMove, to, strip->mapToGlobal(to),
                                 Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                QCoreApplication::sendEvent(strip, &move);
            }
            const QPoint end = from + QPoint(QApplication::startDragDistance() * 3 + 3, 0);
            QMouseEvent up(QEvent::MouseButtonRelease, end, strip->mapToGlobal(end),
                           Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(strip, &up);
            QCOMPARE(window.geometry(), before);
            QVERIFY(!strip->IsEditing());
        }
        {
            const QPoint from = strip->EditRect().center();
            const QPoint to = from + QPoint(4, 0);
            QVERIFY(4 < QApplication::startDragDistance());
            QMouseEvent down(QEvent::MouseButtonPress, from, strip->mapToGlobal(from),
                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(strip, &down);
            QMouseEvent move(QEvent::MouseMove, to, strip->mapToGlobal(to),
                             Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(strip, &move);
            QMouseEvent up(QEvent::MouseButtonRelease, to, strip->mapToGlobal(to),
                           Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(strip, &up);
            QVERIFY(!strip->IsEditing());
        }

        press(strip->EditRect().center());
        QVERIFY(strip->IsEditing());
        press(strip->ButtonRect(TitleBar::ExpandButton).center());
        QVERIFY(!window.IsStripExpanded());
        QVERIFY(!strip->IsEditing());
        for(int turn = 0; turn < 3; turn++) QCoreApplication::processEvents();
        window.ApplyTitleBarVisibility(away, true, false);
        QCOMPARE(strip->width(), qMin(window.width(), strip->CompactWidth()));

        press(strip->ButtonRect(TitleBar::ExpandButton).center());
        QVERIFY(window.IsStripExpanded());
        QVERIFY(window.IsStripPinned());
        auto pressOn = [](QWidget *widget){
            const QPoint at(5, 5);
            QMouseEvent down(QEvent::MouseButtonPress, at, widget->mapToGlobal(at),
                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(widget, &down);
        };
        {
            QWidget popup(nullptr, Qt::Popup);
            pressOn(&popup);
            QVERIFY(window.IsStripPinned());
        }
        pressOn(&window);
        QVERIFY(!window.IsStripPinned());
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(!strip->isVisible());
        QVERIFY(window.IsStripExpanded());
        window.ApplyTitleBarVisibility(QPoint(strip->geometry().center().x(), window.screen()->geometry().top()), true, false);
        QVERIFY(strip->isVisible());
        QCOMPARE(strip->width(), window.width());
        press(strip->EditRect().center());
        QVERIFY(window.IsStripPinned());
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(strip->isVisible());
        strip->BeginEdit();
        QVERIFY(strip->IsEditing());
        QVERIFY(window.GetTreeBank()->GetReceiver()->IsAnchored());
        QVERIFY(strip->GetAddressDisplay()->isVisible());
        QVERIFY(window.IsStripPinned());
        pressOn(window.GetTreeBank()->GetReceiver());
        QVERIFY(window.IsStripPinned());

        window.Shade();
        QVERIFY(!strip->IsEditing());
        QVERIFY(!window.IsStripPinned());
        QVERIFY(window.IsStripExpanded());
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(strip->isVisible());
        QCOMPARE(strip->width(), window.width());
        QVERIFY(!strip->ButtonRect(TitleBar::ExpandButton).isEmpty());
        QVERIFY(!strip->IsEditable());
        window.Unshade();
        QVERIFY(window.IsStripExpanded());
        window.ApplyTitleBarVisibility(QPoint(strip->geometry().center().x(), window.screen()->geometry().top()), true, false);
        press(strip->ButtonRect(TitleBar::ExpandButton).center());
        QVERIFY(!window.IsStripExpanded());
        window.Shade();
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(strip->isVisible());
        QCOMPARE(strip->width(), qMin(window.width(), strip->CompactWidth()));
        window.Unshade();
        press(strip->ButtonRect(TitleBar::ExpandButton).center());
        QVERIFY(window.IsStripExpanded());
        window.showNormal();
        QCoreApplication::processEvents();
        QVERIFY(!window.IsStripExpanded());
        QVERIFY(strip->ButtonRect(TitleBar::ExpandButton).isEmpty());
        window.SetStripExpanded(true);
        QVERIFY(!window.IsStripExpanded());
        window.showMaximized();
        QCoreApplication::processEvents();
        QVERIFY(window.IsStripExpanded());
        QVERIFY(!window.IsStripPinned());
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(!strip->isVisible());
        window.ApplyTitleBarVisibility(QPoint(strip->geometry().center().x(), window.screen()->geometry().top()), true, false);
        press(strip->ButtonRect(TitleBar::ExpandButton).center());
        QVERIFY(!window.IsStripExpanded());
        window.showNormal();
        QCoreApplication::processEvents();
        window.showMaximized();
        QCoreApplication::processEvents();
        QVERIFY(!window.IsStripExpanded());
        window.showNormal();
        QCoreApplication::processEvents();
        window.removeEventFilter(&activation);
    }

    void theBarsRoomFollowsTheNativeState(){
        using WindowFrameMargin::Inner;
        using WindowFrameMargin::State;
        auto make = [](bool qtMax, bool full, bool known, bool zoomed, bool iconic, bool current){
            State s; s.qtMaximized = qtMax; s.fullScreen = full; s.nativeKnown = known;
            s.nativeZoomed = zoomed; s.nativeIconic = iconic; s.current = current; return s;
        };
        QVERIFY(Inner(make(true, false, true, false, false, false)));
        QVERIFY(!Inner(make(false, false, true, true, false, true)));
        QVERIFY(!Inner(make(true, false, true, false, true, false)));
        QVERIFY(Inner(make(false, false, true, false, true, true)));
        QVERIFY(!Inner(make(true, false, false, false, false, true)));
        QVERIFY(Inner(make(false, false, false, true, false, false)));
        QVERIFY(!Inner(make(false, true, true, false, false, true)));
        QVERIFY(!Inner(make(false, true, false, false, false, true)));
    }

    void theSystemsHoverOutlivesQtsLeave(){
        MainWindow &window = *m_Custom;
        window.showNormal();
        TitleBar bar(&window, TitleBar::Strip);
        bar.resize(bar.ScaleByDevice(600), bar.ScaleByDevice(32));
        const QRect maximize = bar.ButtonRect(TitleBar::MaximizeButton);
        QVERIFY(!maximize.isEmpty());
        const QPoint corner = maximize.topLeft() + QPoint(2, 2);
        const QColor plain = bar.grab().toImage().pixelColor(corner);
        bar.SetNativeHover(true);
        const QColor lit = bar.grab().toImage().pixelColor(corner);
        QVERIFY(lit != plain);
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(&bar, &leave);
        QCOMPARE(bar.grab().toImage().pixelColor(corner), lit);
        bar.SetNativeHover(false);
        QCOMPARE(bar.grab().toImage().pixelColor(corner), plain);
    }

    void aQuickSecondClickOnAButtonIsAClick(){
        MainWindow &window = *m_Custom;
        window.showNormal();
        QCoreApplication::processEvents();
        TitleBar *bar = window.findChild<TitleBar*>();
        QVERIFY(bar);
        QCOMPARE(bar->GetMode(), TitleBar::Inner);
        const QPoint at = bar->ButtonRect(TitleBar::ShadeButton).center();
        QVERIFY(bar->ButtonAt(at) == TitleBar::ShadeButton);
        auto send = [&](QEvent::Type type, Qt::MouseButtons buttons){
            QMouseEvent ev(type, at, bar->mapToGlobal(at), Qt::LeftButton, buttons, Qt::NoModifier);
            QCoreApplication::sendEvent(bar, &ev);
        };
        QVERIFY(!window.IsShaded());
        send(QEvent::MouseButtonPress, Qt::LeftButton);
        send(QEvent::MouseButtonRelease, Qt::NoButton);
        QVERIFY(window.IsShaded());
        send(QEvent::MouseButtonDblClick, Qt::LeftButton);
        send(QEvent::MouseButtonRelease, Qt::NoButton);
        QVERIFY(!window.IsShaded());
    }

#if defined(Q_OS_WIN)
    void theTitlesTextIsTheAddressField(){
        MainWindow &window = *m_Custom;
        window.showNormal();
        window.resize(qMax(window.width(), 1200), window.height());
        QCoreApplication::processEvents();
        TitleBar *resized = window.findChild<TitleBar*>();
        QVERIFY(resized);
        QTRY_VERIFY(resized->width() >= window.width() - resized->ScaleByDevice(64));
        TitleBar *inner = window.findChild<TitleBar*>();
        QVERIFY(inner);
        QCOMPARE(inner->GetMode(), TitleBar::Inner);
        window.SetWindowTitle(QStringLiteral("A page with a title"));
        const QRect area = inner->TitleRect();
        const QRect band = inner->EditRect();
        QVERIFY(!band.isEmpty());
        QCOMPARE(band.left(), area.left());
        QVERIFY(band.width() >= area.width() / 5);
        QVERIFY(band.width() <= area.width() * 9 / 10);
        {
            const QString title = window.windowTitle();
            window.SetWindowTitle(QString(400, QLatin1Char('W')));
            QCOMPARE(inner->EditRect().width(), area.width() * 9 / 10);
            window.SetWindowTitle(QStringLiteral("a"));
            QCOMPARE(inner->EditRect().width(), area.width() / 5);
            window.SetWindowTitle(title);
            QCOMPARE(inner->EditRect(), band);
        }
        const QRect text(QPoint(band.right() - inner->ScaleByDevice(20), band.top()), band.bottomRight());
        QVERIFY(!area.isEmpty());
        QVERIFY(area.contains(text));
        for(TitleBar::Button each : {TitleBar::MenuButton, TitleBar::ViewTreeButton, TitleBar::ShadeButton,
                                     TitleBar::MinimizeButton, TitleBar::MaximizeButton, TitleBar::CloseButton})
            QVERIFY(!area.intersects(inner->ButtonRect(each)));

        auto click = [&](QPoint at, QPoint releaseAt){
            QMouseEvent press(QEvent::MouseButtonPress, at, inner->mapToGlobal(at),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(inner, &press);
            if(releaseAt != at){
                QMouseEvent move(QEvent::MouseMove, releaseAt, inner->mapToGlobal(releaseAt),
                                 Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                QCoreApplication::sendEvent(inner, &move);
            }
            QMouseEvent release(QEvent::MouseButtonRelease, releaseAt, inner->mapToGlobal(releaseAt),
                                Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(inner, &release);
        };

        click(text.center(), text.center() + QPoint(QApplication::startDragDistance() + 2, 0));
        QVERIFY(!inner->IsEditing());

        click(text.center(), text.center());
        QVERIFY(inner->IsEditing());
        LineEdit *address = inner->GetAddressDisplay();
        QVERIFY(address);
        Receiver *line = window.GetTreeBank()->GetReceiver();
        QVERIFY(line && line->isVisible() && line->IsAnchored());
        QVERIFY(!address->isHidden());
        QVERIFY(area.contains(address->geometry()));
        QCOMPARE(inner->NavigationButtons().size(), 6);
        QToolButton *backButton = inner->NavigationButtons().first();
        QVERIFY(!backButton->isHidden());
        QVERIFY(area.contains(backButton->geometry()));
        QVERIFY(backButton->geometry().right() < address->geometry().left());
        QVERIFY(!inner->GetExtensionBar()->isHidden());
        QVERIFY(inner->GetExtensionBar()->geometry().left() > address->geometry().right());
        SiteButton *site = inner->GetSiteButton();
        QVERIFY(site && !site->isHidden());
        QVERIFY(site->geometry().left() > backButton->geometry().right());
        QVERIFY(site->geometry().right() < address->geometry().left());
        const int siteGap = address->geometry().left() - site->geometry().right() - 1;
        QCOMPARE(siteGap, 0);
        QCOMPARE(site->width(), inner->ScaleByDevice(TOOL_BAR_ICON_SIZE + SITE_BUTTON_SIDE));
        QCOMPARE(LineEdit::AddressRect(site, address),
                 QRect(QPoint(site->x(), address->y()), address->geometry().bottomRight()));
        QVERIFY(address->styleSheet().contains(QStringLiteral("QLineEdit:hover{ background: transparent;")));
        QVERIFY(inner->GetExtensionBar()->geometry().left() - address->geometry().right() >
                inner->ScaleByDevice(RECEIVER_PADDING));
        for(QToolButton *b : inner->NavigationButtons())
            QCOMPARE(!b->isHidden(), b->defaultAction()->isVisible());
        QAction *stop = inner->NavigationButtons().last()->defaultAction();
        const bool stopWas = stop->isVisible();
        stop->setVisible(!stopWas);
        QCOMPARE(!inner->NavigationButtons().last()->isHidden(), !stopWas);
        stop->setVisible(stopWas);
        {
            ExtensionBar *extensions = inner->GetExtensionBar();
            const int before = extensions->width();
            extensions->setMinimumWidth(before + inner->ScaleByDevice(40));
            QTRY_VERIFY(area.contains(extensions->geometry()) &&
                        extensions->width() >= before + inner->ScaleByDevice(40));
            QVERIFY(address->geometry().right() < extensions->geometry().left());
            extensions->setMinimumWidth(0);
            QTRY_COMPARE(extensions->width(), before);
        }
        const WindowFrameHit::Layout editing = window.FrameHitLayout();
        bool fieldIsClient = false;
        for(const QRect &r : editing.buttons) if(r.contains(area.center())) fieldIsClient = true;
        QVERIFY(fieldIsClient);

        line->OnAborted();
        QVERIFY(!inner->IsEditing());
        QVERIFY(address->isHidden());

        click(text.center(), text.center());
        QVERIFY(inner->IsEditing());
        line->SuitableAction();
        QVERIFY(!line->isVisible());
        QVERIFY(!inner->IsEditing());
        QVERIFY(address->isHidden());
        for(QToolButton *b : inner->NavigationButtons()) QVERIFY(b->isHidden());
        QVERIFY(inner->GetExtensionBar()->isHidden());

        click(text.center(), text.center());
        QVERIFY(inner->IsEditing());
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QCoreApplication::sendEvent(inner->GetExtensionBar(), &escape);
        QVERIFY(!inner->IsEditing());
        QVERIFY(!line->isVisible());

        {
            const QRect wide = window.geometry();
            window.resize(inner->ScaleByDevice(560), wide.height());
            QCoreApplication::processEvents();
            const QRect narrow = inner->TitleRect();
            QVERIFY(!narrow.isEmpty());
            click(narrow.center(), narrow.center());
            QVERIFY(inner->IsEditing());
            QVERIFY(address->width() >= qMin(narrow.width(), inner->ScaleByDevice(120)) - 1);
            QVERIFY(narrow.contains(address->geometry()));
            inner->EndEdit();
            window.setGeometry(wide);
            QCoreApplication::processEvents();
        }
        {
            const QRect wide = window.geometry();
            const int spacing = inner->ScaleByDevice(TOOL_BAR_ICON_SPACING);
            int buttons = 0;
            for(QToolButton *b : inner->NavigationButtons())
                if(b->defaultAction()->isVisible())
                    buttons += (buttons ? spacing : 0) + b->sizeHint().width();
            buttons += spacing + inner->ScaleByDevice(SITE_BUTTON_APART)
                + inner->ScaleByDevice(TOOL_BAR_ICON_SIZE + SITE_BUTTON_SIDE);
            const int clear = spacing * 2 + inner->ScaleByDevice(RECEIVER_PADDING);
            const int need = inner->ScaleByDevice(120) + buttons +
                             inner->GetExtensionBar()->sizeHint().width() + clear;
            const int chrome = window.width() - inner->TitleRect().width();
            window.resize(need + chrome, wide.height());
            QCoreApplication::processEvents();
            QCOMPARE(inner->TitleRect().width(), need);
            click(inner->EditRect().center(), inner->EditRect().center());
            QVERIFY(inner->IsEditing());
            QVERIFY(!inner->GetExtensionBar()->isHidden());
            for(QToolButton *b : inner->NavigationButtons())
                QCOMPARE(!b->isHidden(), b->defaultAction()->isVisible());
            inner->EndEdit();
            window.setGeometry(wide);
            QCoreApplication::processEvents();
        }

        click(text.center(), text.center());
        QVERIFY(inner->IsEditing());
        line->hide();
        QVERIFY(!inner->IsEditing());
        QVERIFY(address->isHidden());

        click(text.center(), text.center());
        QVERIFY(line->isVisible());
        inner->EndEdit();
        QVERIFY(!line->isVisible());
        QVERIFY(!line->IsAnchored());

        {
            click(text.center(), text.center());
            QVERIFY(line->isVisible());
            QToolButton *button = nullptr;
            for(QToolButton *b : inner->NavigationButtons())
                if(!b->isHidden() && !button) button = b;
            QVERIFY(button);
            const QPoint onButton = button->mapToGlobal(button->rect().center());
            QCursor::setPos(onButton);
            QTRY_COMPARE(QApplication::widgetAt(QCursor::pos()), static_cast<QWidget*>(button));
            line->DismissFocusLeft();
            QVERIFY(inner->IsEditing());
            QVERIFY(!line->isVisible());
            QVERIFY(!address->isHidden());
            QVERIFY(!button->isHidden());
            QTest::mouseClick(address, Qt::LeftButton);
            QVERIFY(line->isVisible() && line->IsAnchored());
            QCursor::setPos(onButton);
            window.activateWindow();
            QTRY_VERIFY(!line->isVisible());
            QTest::qWait(50);
            QVERIFY(inner->IsEditing());
            {
                QWidget off(&window);
                off.setFocusPolicy(Qt::NoFocus);
                off.setGeometry(0, window.height() / 2, 40, 40);
                off.show();
                QTest::mousePress(&off, Qt::LeftButton);
                QTRY_VERIFY(!inner->IsEditing());
                QTest::mouseRelease(&off, Qt::LeftButton);
            }
            click(text.center(), text.center());
            QVERIFY(line->isVisible());
            QCursor::setPos(onButton);
            window.activateWindow();
            QTRY_VERIFY(!line->isVisible());
            for(int turn = 0; turn < 3; turn++) QCoreApplication::processEvents();
            QVERIFY(inner->IsEditing());
            QKeyEvent pageEsc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            QCoreApplication::sendEvent(window.GetTreeBank(), &pageEsc);
            for(int turn = 0; turn < 3; turn++) QCoreApplication::processEvents();
            QVERIFY(!inner->IsEditing());
            {
                QWidget other;
                other.setGeometry(window.geometry().right() + 20, window.geometry().top(), 100, 100);
                other.show();
                click(text.center(), text.center());
                QVERIFY(line->isVisible());
                QCursor::setPos(onButton);
                other.activateWindow();
                QTRY_COMPARE(QApplication::activeWindow(), &other);
                line->DismissFocusLeft();
                QVERIFY(!inner->IsEditing());
            }
            click(text.center(), text.center());
            QCursor::setPos(window.GetTreeBank()->mapToGlobal(window.GetTreeBank()->rect().center()));
            line->DismissFocusLeft();
            QVERIFY(!inner->IsEditing());
        }

        {
            click(text.center(), text.center());
            QVERIFY(line->isVisible());
            inner->GetSiteButton()->click();
            QVERIFY(inner->IsEditing());
            QVERIFY(!line->isVisible());
            SiteInfoPanel *panel = inner->findChild<SiteInfoPanel*>();
            QVERIFY(panel && panel->isVisible());
            panel->close();
            QTRY_VERIFY(!inner->IsEditing());
            click(text.center(), text.center());
            inner->GetSiteButton()->click();
            QPointer<SiteInfoPanel> open = inner->findChild<SiteInfoPanel*>();
            QVERIFY(open);
            inner->EndEdit();
            QTRY_VERIFY(!open);
        }

        click(text.center(), text.center());
        QVERIFY(inner->IsEditing());
        window.CurrentViewChanged();
        QVERIFY(!inner->IsEditing());
        QVERIFY(address->isHidden());

        click(text.center(), text.center());
        QVERIFY(inner->IsEditing());
        window.showMaximized();
        QCoreApplication::processEvents();
        QVERIFY(!inner->IsEditing());
        window.showNormal();
        QCoreApplication::processEvents();

        const WindowFrameHit::Layout resting = window.FrameHitLayout();
        bool bandIsClient = false, outsideIsClient = false;
        const QPoint after(band.right() + 3, area.center().y());
        const QPoint gap(area.left() - 2, area.center().y());
        QVERIFY(area.contains(after));
        for(const QRect &r : resting.buttons){
            if(r.contains(QPoint(band.left() + 1, area.center().y())) &&
               r.contains(QPoint(band.right() - 1, area.center().y()))) bandIsClient = true;
            if(r.contains(after) || r.contains(gap)) outsideIsClient = true;
        }
        QVERIFY(bandIsClient);
        QVERIFY(!outsideIsClient);
        click(after, after);
        QVERIFY(!inner->IsEditing());

        QMouseEvent twice(QEvent::MouseButtonDblClick, text.center(), inner->mapToGlobal(text.center()),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(inner, &twice);
        QVERIFY(!window.isMaximized());
    }
#endif

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

    void theStripsChoiceIsKeptWithTheWindow(){
        Settings &s = Application::GlobalSettings();
        s.setValue(QStringLiteral("application/@EnableCustomTitleBar"), true);
        Application::LoadGlobalSettings();
        const QString key = QStringLiteral("mainwindow/stripexpanded904");
        s.setValue(key, true);
        {
            MainWindow window(904, QPoint(100, 100), false);
            window.showMaximized();
            QCoreApplication::processEvents();
            QVERIFY(window.IsStripExpanded());
            QVERIFY(!window.IsStripPinned());
            window.SetStripExpanded(false);
            window.SaveSettings();
            QCOMPARE(s.value(key), QVariant(false));
            window.SetStripExpanded(true);
            window.SaveSettings();
            QCOMPARE(s.value(key), QVariant(true));
            window.RemoveSettings();
            QVERIFY(!s.contains(key));
        }
        s.remove(QStringLiteral("mainwindow/titlebartoolbar904"));
    }

    void eachWindowKeepsItsOwnMiniMap(){
        Settings &s = Application::GlobalSettings();
        const QString key = QStringLiteral("mainwindow/minimap906");
        const QString otherKey = QStringLiteral("mainwindow/minimap907");
        s.setValue(key, false);
        s.setValue(otherKey, true);
        {
            MainWindow window(906, QPoint(100, 100), false);
            MainWindow other(907, QPoint(100, 100), false);
            TreeBank *tb = window.GetTreeBank();
            TreeBank *otherTb = other.GetTreeBank();
            QVERIFY(!tb->GetMiniMap());
            QVERIFY(otherTb->GetMiniMap());

            QAction *action = tb->Action(TreeBank::_ToggleMiniMap);
            QScopedPointer<QMenu> menu(window.createPopupMenu());
            QVERIFY(menu->actions().contains(action));
            QVERIFY(action->isCheckable());
            QVERIFY(!action->isChecked());

            action->trigger();
            QVERIFY(tb->GetMiniMap());
            QVERIFY(action->isChecked());
            QVERIFY(otherTb->GetMiniMap());
            otherTb->ToggleMiniMap();
            QVERIFY(!otherTb->GetMiniMap());
            QVERIFY(tb->GetMiniMap());

            window.SaveSettings();
            other.SaveSettings();
            QCOMPARE(s.value(key), QVariant(true));
            QCOMPARE(s.value(otherKey), QVariant(false));

            window.RemoveSettings();
            other.RemoveSettings();
            QVERIFY(!s.contains(key));
            QVERIFY(!s.contains(otherKey));
        }
    }

    void theMaximizedStripSlides(){
        Settings &s = Application::GlobalSettings();
        s.setValue(QStringLiteral("application/@EnableCustomTitleBar"), true);
        Application::LoadGlobalSettings();
        s.setValue(QStringLiteral("mainwindow/stripexpanded905"), false);
        QSet<QWidget*> before;
        for(QWidget *widget : QApplication::topLevelWidgets()) before.insert(widget);
        MainWindow window(905, QPoint(100, 100), false);
        TitleBar *strip = nullptr;
        for(QWidget *widget : QApplication::topLevelWidgets())
            if(auto *bar = qobject_cast<TitleBar*>(widget))
                if(!before.contains(bar) && bar->GetMode() != TitleBar::Inner) strip = bar;
        QVERIFY(strip);
        window.showMaximized();
        QCoreApplication::processEvents();
        QTimer *poll = window.findChild<QTimer*>(QStringLiteral("TitleBarRevealTimer"));
        QVERIFY(poll);
        poll->setInterval(24 * 60 * 60 * 1000);
        auto *sliding = new SlidingStyle;
        sliding->setParent(strip);
        strip->setStyle(sliding);
        IgnorePlatformActivation activation;
        window.installEventFilter(&activation);

        const int t = strip->ScaleByDevice(TITLE_BAR_HEIGHT);
        const QPoint edge(window.geometry().center().x(), window.screen()->geometry().top());
        const QPoint away = window.geometry().center();
        const int top = window.screen()->geometry().top();

        window.ApplyTitleBarVisibility(edge, true, false);
        QVERIFY(strip->isVisible());
        QVERIFY(strip->IsSliding());
        QVERIFY(strip->height() < t);
        QCOMPARE(strip->geometry().top(), top);
        window.ApplyTitleBarVisibility(QPoint(edge.x(), top + t - 1), true, false);
        QVERIFY(strip->isVisible());
        QVERIFY(strip->IsSliding());
        QVERIFY(strip->height() < t);
        QTRY_VERIFY(!strip->IsSliding());
        QVERIFY(strip->isVisible());
        QCOMPARE(strip->height(), t);
        QCOMPARE(strip->geometry().top(), top);

        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(strip->isVisible());
        QVERIFY(strip->IsSliding());
        QTRY_VERIFY(!strip->isVisible());

        window.ApplyTitleBarVisibility(edge, true, false);
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(strip->isVisible());
        QVERIFY(strip->IsSliding());
        window.ApplyTitleBarVisibility(edge, true, false);
        QTRY_VERIFY(!strip->IsSliding());
        QVERIFY(strip->isVisible());
        QCOMPARE(strip->height(), t);

        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(strip->IsSliding());
        const QPoint at = strip->ButtonRect(TitleBar::ExpandButton).center();
        QMouseEvent down(QEvent::MouseButtonPress, at, strip->mapToGlobal(at),
                         Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(strip, &down);
        QVERIFY(!strip->IsSliding());
        QVERIFY(strip->isVisible());
        QCOMPARE(strip->height(), t);
        const QPoint inside = strip->geometry().center();
        window.ApplyTitleBarVisibility(inside, true, true);
        QVERIFY(strip->isVisible());
        QVERIFY(!strip->IsSliding());
        const QPoint off(-100, -100);
        QMouseEvent up(QEvent::MouseButtonRelease, off, strip->mapToGlobal(off),
                       Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(strip, &up);
        QVERIFY(!window.IsStripExpanded());
        window.ApplyTitleBarVisibility(inside, true, false);
        QVERIFY(strip->isVisible());
        QVERIFY(!strip->IsSliding());

        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(strip->IsSliding());
        window.showNormal();
        QCoreApplication::processEvents();
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(!strip->IsSliding());
        QVERIFY(!strip->isVisible());

        window.showMaximized();
        QCoreApplication::processEvents();
        window.ApplyTitleBarVisibility(edge, true, false);
        QVERIFY(strip->IsSliding());
        window.Shade();
        window.ApplyTitleBarVisibility(edge, true, false);
        QVERIFY(window.IsShaded());
        QVERIFY(!strip->IsSliding());
        QVERIFY(strip->isVisible());
        QCOMPARE(strip->height(), t);
        window.Unshade();
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(!strip->IsSliding());
        QVERIFY(!strip->isVisible());

        window.SetStripExpanded(true);
        window.ApplyTitleBarVisibility(away, true, false);
        QTRY_VERIFY(!strip->IsSliding());
        QVERIFY(strip->isVisible());
        strip->BeginEdit();
        QVERIFY(strip->IsEditing());
        window.UnpinStrip();
        window.ApplyTitleBarVisibility(away, true, false);
        QVERIFY(!strip->IsEditing());
        QVERIFY(strip->IsSliding());
        QTRY_VERIFY(!strip->isVisible());

        window.removeEventFilter(&activation);
        strip->setStyle(nullptr);
        window.SetStripExpanded(false);
        window.RemoveSettings();
    }

    void nativeFrameDoesNotReserveSpace(){
        Application::GlobalSettings().setValue("application/@EnableCustomTitleBar", false);
        Application::LoadGlobalSettings();
        Application::GlobalSettings().setValue(QStringLiteral("mainwindow/titlebartoolbar902"), true);
        m_Native = std::make_unique<MainWindow>(902, QPoint(100, 100));
        QVERIFY(!Application::GlobalSettings().contains(QStringLiteral("mainwindow/titlebartoolbar902")));
        MainWindow &window = *m_Native;
        window.showMaximized();
        QCoreApplication::processEvents();
        QCOMPARE(window.contentsMargins(), QMargins());
    }
    void aWindowMadeHiddenWaitsForItsCallerToShowIt(){
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        std::unique_ptr<MainWindow> waiting(Application::NewWindow(905, QPoint(), false));
        std::unique_ptr<MainWindow> shown(Application::NewWindow(906));
        const auto unregister = qScopeGuard([&]{
            Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
            Application::RemoveWindow(905);
            Application::RemoveWindow(906);
        });
        QVERIFY(!waiting->isVisible());
        QVERIFY(waiting->internalWinId() != 0);
        QVERIFY(shown->isVisible());
        waiting->show();
        QVERIFY(waiting->isVisible());
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

    void theToolBarStopsAReloadOfTheSameAddress(){
        if(!m_Custom){
            m_Custom = std::make_unique<MainWindow>(901, QPoint(100, 100));
            m_Custom->setAnimated(false);
            m_Custom->resize(1200, 800);
        }
        MainWindow *window = m_Custom.get();
        window->show();
        ToolBar *toolBar = window->GetToolBar();
        TreeBank *bank = window->GetTreeBank();
        std::shared_ptr<KeyboardHoldingView> view = std::make_shared<KeyboardHoldingView>();
        toolBar->Connect(view);
        emit view->loadFinished(true);
        QVERIFY(bank->Action(TreeBank::_Reload)->isVisible());
        QVERIFY(!bank->Action(TreeBank::_Stop)->isVisible());
        emit view->loadStarted();
        QVERIFY(!bank->Action(TreeBank::_Reload)->isVisible());
        QVERIFY(bank->Action(TreeBank::_Stop)->isVisible());
        emit view->loadFinished(true);
        QVERIFY(bank->Action(TreeBank::_Reload)->isVisible());
        toolBar->Disconnect(view);
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

    void deletingTheGlobalContextMenuTakesItsSubMenus(){
        if(!m_Custom){
            m_Custom = std::make_unique<MainWindow>(901, QPoint(100, 100));
            m_Custom->setAnimated(false);
            m_Custom->resize(1200, 800);
        }
        TreeBank *treeBank = m_Custom->GetTreeBank();
        const int before = treeBank->findChildren<QMenu*>().size();
        for(int i = 0; i < 3; i++){
            QMenu *menu = treeBank->GlobalContextMenu();
            QVERIFY(treeBank->findChildren<QMenu*>().size() > before);
            delete menu;
        }
        QCOMPARE(treeBank->findChildren<QMenu*>().size(), before);
    }

    void theDialogFrameIsOwnedByItsWindow(){
        if(!m_Custom){
            m_Custom = std::make_unique<MainWindow>(901, QPoint(100, 100));
            m_Custom->setAnimated(false);
            m_Custom->resize(1200, 800);
        }
        MainWindow *window = m_Custom.get();
        window->show();
        QCoreApplication::processEvents();
        QWidget *frame = window->DialogFrame();
        QVERIFY(frame->windowHandle());
        QCOMPARE(frame->windowHandle()->transientParent(), window->windowHandle());
        QCOMPARE(frame->windowType(), Qt::Tool);
        QCOMPARE(frame->parent(), static_cast<QObject*>(nullptr));
    }

    void theMenuLeavesTheWindowItWentTo(){
        Application::GlobalSettings().setValue("application/@EnableCustomTitleBar", true);
        Application::LoadGlobalSettings();
        MainWindow &window = *m_Custom;
        window.showNormal();
        MainWindow other(907, QPoint(200, 200), false);
        other.show();
        MainWindow *before = Application::GetCurrentWindow();
        const auto restore = qScopeGuard([&]{
            Application::SetCurrentWindow(before);
            other.RemoveSettings();
        });
        TitleBar bar(&window, TitleBar::Strip);
        bar.resize(bar.ScaleByDevice(600), bar.ScaleByDevice(32));
        bar.show();
        QCoreApplication::processEvents();

        const auto choose = [&](std::function<void()> chosen){
            QTimer poll;
            poll.setInterval(10);
            QObject::connect(&poll, &QTimer::timeout, [&](){
                QWidget *popup = QApplication::activePopupWidget();
                if(!popup) return;
                poll.stop();
                chosen();
                popup->close();
            });
            poll.start();
            QTest::mouseClick(&bar, Qt::LeftButton, Qt::NoModifier,
                              bar.ButtonRect(TitleBar::MenuButton).center());
            QCoreApplication::processEvents();
        };

        Application::SetCurrentWindow(&window);
        window.activateWindow();
        QCoreApplication::processEvents();
        choose([&](){
            Application::SetCurrentWindow(&other);
            other.activateWindow();
        });
        QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&other));
        QCoreApplication::processEvents();
        QCOMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&other));

        Application::SetCurrentWindow(&window);
        choose([](){});
        QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&window));
    }

    void aDialogBelongsToTheCurrentWindowOnScreen(){
        MainWindow window(906, QPoint(100, 100), false);
        MainWindow *before = Application::GetCurrentWindow();
        Application::SetCurrentWindow(&window);
        QCOMPARE(MainWindow::DialogOwner(), static_cast<MainWindow*>(nullptr));
        window.show();
        QCoreApplication::processEvents();
        QCOMPARE(MainWindow::DialogOwner(), &window);
        window.showMinimized();
        QCoreApplication::processEvents();
        QCOMPARE(MainWindow::DialogOwner(), static_cast<MainWindow*>(nullptr));
        window.showNormal();
        QCoreApplication::processEvents();
        QCOMPARE(MainWindow::DialogOwner(), &window);
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        QCOMPARE(MainWindow::DialogOwner(), static_cast<MainWindow*>(nullptr));
        Application::SetCurrentWindow(before);
        window.RemoveSettings();
    }

    void anOwnerGoingFirstLeavesItsWindowsBehind(){
        QWidget owned(nullptr, Qt::FramelessWindowHint | Qt::Tool);
        QPointer<QWidget> frame;
        {
            MainWindow window(907, QPoint(100, 100), false);
            window.show();
            QCoreApplication::processEvents();
            MainWindow::SetOwner(&owned, &window);
            owned.show();
            QCoreApplication::processEvents();
            QCOMPARE(owned.windowHandle()->transientParent(), window.windowHandle());
            frame = window.DialogFrame();
            window.RemoveSettings();
        }
        QCoreApplication::processEvents();
        QVERIFY(owned.windowHandle());
        QCOMPARE(owned.windowHandle()->transientParent(), static_cast<QWindow*>(nullptr));
        MainWindow::SetOwner(&owned, nullptr);
        QCOMPARE(owned.windowHandle()->transientParent(), static_cast<QWindow*>(nullptr));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(frame.isNull());
    }
};

QTEST_MAIN(tst_windowframe)
#include "tst_windowframe.moc"
