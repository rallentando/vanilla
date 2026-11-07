#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QObject>
#include <QFile>
#include <QPainter>
#include <QWidget>
#include <QMenu>
#include <QLabel>
#include <QWidgetAction>

#include "view.hpp"
#include "page.hpp"
#include "treebank.hpp"
#include "mainwindow.hpp"
#include "application.hpp"
#include <QScopeGuard>
#include <memory>
#include <QAction>
#include <QMouseEvent>

#include "testsupport.hpp"

class BareView : public QObject, public View {

public:
    BareView() : QObject(nullptr), View(nullptr) {}

    QObject *base() override { return this;}

    QSize size() override { return QSize();}
    void resize(QSize) override {}
    void show() override {}
    void hide() override {}
    void raise() override {}
    void lower() override {}
    void repaint() override {}
    bool visible() override { return false;}
    void setFocus(Qt::FocusReason = Qt::OtherFocusReason) override {}
};

class SolidWidget : public QWidget {
public:
    explicit SolidWidget(const QColor &color, QWidget *parent = nullptr)
        : QWidget(parent), m_Color(color) {}

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.fillRect(rect(), m_Color);
    }

private:
    QColor m_Color;
};

class tst_viewinterface : public QObject {
    Q_OBJECT

private slots:

    void everyQuestionIsAnsweredOnceAndEmpty(){
        BareView view;
        int called = 0;

        view.CallWithGotBaseUrl([&](const QUrl &url){ called++; QVERIFY(url.isEmpty());});
        QCOMPARE(called, 1);

        view.CallWithGotCurrentBaseUrl([&](const QUrl &url){ called++; QVERIFY(url.isEmpty());});
        QCOMPARE(called, 2);

        view.CallWithHitLinkUrl(QPoint(1, 1), [&](const QUrl &url){ called++; QVERIFY(url.isEmpty());});
        QCOMPARE(called, 3);

        view.CallWithHitImageUrl(QPoint(1, 1), [&](const QUrl &url){ called++; QVERIFY(url.isEmpty());});
        QCOMPARE(called, 4);

        view.CallWithSelectedText([&](const QString &text){ called++; QVERIFY(text.isEmpty());});
        QCOMPARE(called, 5);

        view.CallWithSelectedHtml([&](const QString &html){ called++; QVERIFY(html.isEmpty());});
        QCOMPARE(called, 6);

        view.CallWithWholeText([&](const QString &text){ called++; QVERIFY(text.isEmpty());});
        QCOMPARE(called, 7);

        view.CallWithWholeHtml([&](const QString &html){ called++; QVERIFY(html.isEmpty());});
        QCOMPARE(called, 8);

        view.CallWithSelectionRegion([&](const QRegion &region){ called++; QVERIFY(region.isEmpty());});
        QCOMPARE(called, 9);

        view.CallWithEvaluatedJavaScriptResult(QStringLiteral("1 + 1"),
                                               [&](const QVariant &v){ called++; QVERIFY(!v.isValid());});
        QCOMPARE(called, 10);

        view.CallWithFoundElements(Page::FindElementsOption(),
                                   [&](SharedWebElementList list){ called++; QVERIFY(list.isEmpty());});
        QCOMPARE(called, 11);

        view.CallWithHitElement(QPoint(1, 1),
                                [&](SharedWebElement elem){ called++; QVERIFY(!elem);});
        QCOMPARE(called, 12);
    }

    void thesynchronousOneDoesNotWaitForAnAnswerItAlreadyHas(){
        BareView view;

        QElapsedTimer timer;
        timer.start();
        const QString html = view.WholeHtml();

        QVERIFY(html.isEmpty());
        QVERIFY2(timer.elapsed() < 1000, qPrintable(QString::number(timer.elapsed())));
    }

    void aMenuCaptionIsDrawnFaint(){
        QMenu menu;
        QWidgetAction *action = View::MenuCaption(&menu, QStringLiteral("(name)"), Qt::AlignRight);
        QVERIFY(action);
        QLabel *label = action->defaultWidget()->findChild<QLabel*>();
        QVERIFY(label);
        QCOMPARE(label->text(), QStringLiteral("(name)"));
        QCOMPARE(label->alignment(), Qt::Alignment(Qt::AlignRight));
        QCOMPARE(label->foregroundRole(), QPalette::PlaceholderText);

        QFile file(QStringLiteral(VANILLA_SOURCE_DIR) + QStringLiteral("/view/view.cpp"));
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString source = QString::fromUtf8(file.readAll());
        QCOMPARE(source.count(QStringLiteral("new QLabel(")), 1);
        QCOMPARE(source.count(QStringLiteral("MenuCaption(menu, name,")), 3);
    }

    void aViewIsNamedAfterTheObjectItReallyIs(){
        BareView view;
        QCOMPARE(view.GetViewTypeName(),
                 QString::fromLatin1(view.base()->metaObject()->className()));
        QVERIFY(!view.GetViewTypeName().isEmpty());
    }

    void loadingStateIsNotSomethingAnImplementationDecides(){
        BareView view;
        QCOMPARE(view.LoadProgress(), 0);
        QCOMPARE(view.IsLoading(), false);
    }

    void anEngineNotchIsFiveThirdsOfTheMouseNotch(){
        const QPoint notch(0, 120);
        QCOMPARE(View::EngineWheelAngle(notch), QPoint(0, 200));
        QCOMPARE(View::EngineWheelAngle(QPoint(0, -120)), QPoint(0, -200));
        QCOMPARE(View::EngineWheelAngle(QPoint(120, 0)), QPoint(200, 0));
        QCOMPARE(View::EngineWheelAngle(QPoint(-120, 120)), QPoint(-200, 200));
        QCOMPARE(View::EngineWheelAngle(QPoint(0, 30)), QPoint(0, 50));
        QCOMPARE(notch, QPoint(0, 120));
    }

    void aZoomStepStaysWithinWhatTheEngineCanShow(){
        const QList<float> &levels = View::GetZoomFactorLevels();
        const float min = View::ChromiumMinimumZoom / 1.25f;
        const float max = View::ChromiumMaximumZoom / 1.25f;
        const int one = levels.indexOf(1.0f);
        QVERIFY(one > 0);

        QCOMPARE(View::StepZoom(1.0f, true, min, max), levels.at(one + 1));
        QCOMPARE(View::StepZoom(1.0f, false, min, max), levels.at(one - 1));

        QCOMPARE(View::StepZoom(levels.at(levels.indexOf(5.0f) - 4), true, min, max), max);
        QCOMPARE(View::StepZoom(max, true, min, max), max);
        QCOMPARE(View::StepZoom(min, false, min, max), min);

        const float below = View::StepZoom(10.0f, false, min, max);
        QVERIFY(below < max);
        QVERIFY(below > max - 0.3f);

        BareView view;
        QCOMPARE(view.MinimumZoom(), levels.first());
        QCOMPARE(view.MaximumZoom(), levels.last());
        QCOMPARE(View::StepZoom(5.0f, true, view.MinimumZoom(), view.MaximumZoom()), 10.0f);
        QCOMPARE(view.FitZoom(20.0f), 10.0f);
    }

    void theChromiumViewsHoldTheirZoomToChromiumsRange(){
        const QStringList files = {
            QStringLiteral("/view/webengine/webengineview.hpp"),
            QStringLiteral("/view/webengine/quickwebengineview.hpp"),
            QStringLiteral("/view/edge/edgewebview.hpp"),
        };
        for(const QString &name : files){
            QFile file(QStringLiteral(VANILLA_SOURCE_DIR) + name);
            QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(name));
            const QString source = QString::fromUtf8(file.readAll());
            QVERIFY2(source.contains(QStringLiteral("float MinimumZoom() const override { return ChromiumMinimumZoom")), qPrintable(name));
            QVERIFY2(source.contains(QStringLiteral("float MaximumZoom() const override { return ChromiumMaximumZoom")), qPrintable(name));
        }
    }

    void widgetRenderKeepsANonOriginClipAtItsWidgetCoordinates(){
        SolidWidget source(Qt::red);
        source.resize(24, 20);
        SolidWidget inset(Qt::green, &source);
        inset.setGeometry(12, 8, 6, 5);
        inset.show();

        QImage target(source.size(), QImage::Format_ARGB32);
        target.fill(Qt::transparent);
        QPainter painter(&target);
        const QRegion clip(inset.geometry());
        source.render(&painter, clip.boundingRect().topLeft(), clip);
        painter.end();

        QCOMPARE(target.pixelColor(13, 9), QColor(Qt::green));
        QCOMPARE(target.pixelColor(0, 0), QColor(Qt::transparent));
    }

    void widgetWebEnginePreservesClipCoordinatesAndSkipsAnEmptyClip(){
        QFile file(QStringLiteral(VANILLA_SOURCE_DIR "/view/webengine/webengineview.hpp"));
        QVERIFY2(file.open(QIODevice::ReadOnly),
                 "webengineview.hpp was not read; check VANILLA_SOURCE_DIR");
        const QString source = QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));
        const int begin = source.indexOf(QStringLiteral("void Render(QPainter *painter, const QRegion &clip)"));
        const int end = source.indexOf(QStringLiteral("QSize GetViewportSize()"), begin);
        QVERIFY(begin != -1);
        QVERIFY(end != -1);
        const QString body = source.mid(begin, end - begin);

        QVERIFY(body.contains(QStringLiteral("if(!clip.isEmpty())")));
        QVERIFY(body.contains(QStringLiteral("render(painter, clip.boundingRect().topLeft(), clip, QWidget::DrawChildren)")));
        QVERIFY(!body.contains(QStringLiteral("to investigate")));
    }

    void widgetWebEngineRendersWithoutItsOwnGround(){
        QFile file(QStringLiteral(VANILLA_SOURCE_DIR "/view/webengine/webengineview.hpp"));
        QVERIFY2(file.open(QIODevice::ReadOnly),
                 "webengineview.hpp was not read; check VANILLA_SOURCE_DIR");
        const QString source = QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));
        const int begin = source.indexOf(QStringLiteral("void Render(QPainter *painter) override"));
        const int end = source.indexOf(QStringLiteral("QSize GetViewportSize()"), begin);
        QVERIFY(begin != -1);
        QVERIFY(end != -1);
        const QString body = source.mid(begin, end - begin);

        QCOMPARE(body.count(QStringLiteral("image.fill(Qt::transparent);")), 2);
        QCOMPARE(body.count(QStringLiteral("render(&p, QPoint(), QRegion(), QWidget::DrawChildren);")), 2);
        QCOMPARE(body.count(QStringLiteral("render(&p);")), 0);
        QVERIFY(body.contains(QStringLiteral("render(painter, QPoint(), QRegion(), QWidget::DrawChildren);")));
    }

    void theLedgerRecoversUntilTheLimitThenGivesUpOnce(){
        RenderProcessLedger ledger;
        QCOMPARE(ledger.Deaths(), 0);
        QVERIFY(!ledger.GaveUp());

        for(int i = 1; i < RenderProcessLedger::Limit; i++){
            QVERIFY(ledger.Count() == RenderProcessLedger::Verdict::Recover);
            QCOMPARE(ledger.Deaths(), i);
            QVERIFY(!ledger.GaveUp());
        }

        QVERIFY(ledger.Count() == RenderProcessLedger::Verdict::GiveUp);
        QCOMPARE(ledger.Deaths(), RenderProcessLedger::Limit);
        QVERIFY(ledger.GaveUp());

        for(int i = 0; i < 5; i++)
            QVERIFY(ledger.Count() == RenderProcessLedger::Verdict::Silent);
    }

    void onlyALoadWhichFinishedClearsTheLedger(){
        BareView view;
        for(int i = 0; i < RenderProcessLedger::Limit; i++)
            view.RenderProcessDeaths().Count();
        QVERIFY(view.RenderProcessDeaths().GaveUp());

        view.OnLoadFinished(false);
        QVERIFY(view.RenderProcessDeaths().GaveUp());
        QCOMPARE(view.RenderProcessDeaths().Deaths(), RenderProcessLedger::Limit);

        view.OnLoadFinished(true);
        QVERIFY(!view.RenderProcessDeaths().GaveUp());
        QCOMPARE(view.RenderProcessDeaths().Deaths(), 0);
        QVERIFY(view.RenderProcessDeaths().Count() == RenderProcessLedger::Verdict::Recover);
    }

    void theGestureLineSaysWhatLettingGoWouldDo(){
        class LineView : public BareView {
        public:
            QAction back{QStringLiteral("Go back")};
            QAction *Action(Page::CustomAction a, QVariant = QVariant()) override {
                return a == Page::_Back ? &back : nullptr;
            }
            static QString shape(GestureVector v){ return GestureToString(Gesture() << v);}
            QString line(const Gesture &shape, const QMap<QString, QString> &map){
                m_Gesture = shape;
                const QString text = GestureStatusLine(map, QStringLiteral("nothing"));
                m_Gesture.clear();
                return text;
            }
        };
        LineView view;
        QMap<QString, QString> map;
        map[LineView::shape(View::Gv_Left)]  = QStringLiteral("Back");
        map[LineView::shape(View::Gv_Right)] = QStringLiteral("Forward");
        map[LineView::shape(View::Gv_Up)]    = QStringLiteral("notAnAction");
        auto expect = [](View::GestureVector v, const char *action){
            return LineView::shape(v) + QStringLiteral(" (") +
                QString::fromLatin1(action) + QStringLiteral(")");
        };
        QCOMPARE(view.line(View::Gesture() << View::Gv_Left, map),  expect(View::Gv_Left, "Go back"));
        QCOMPARE(view.line(View::Gesture() << View::Gv_Right, map), expect(View::Gv_Right, "Forward"));
        QCOMPARE(view.line(View::Gesture() << View::Gv_Up, map),    expect(View::Gv_Up, "notAnAction"));
        QCOMPARE(view.line(View::Gesture() << View::Gv_Down, map),  expect(View::Gv_Down, "nothing"));
    }

    void aMoveGoesWhereTheButtonsSay(){
        class Element : public WebElement {
        public:
            bool IsNull() const override { return false;}
            QRect Rectangle() const override { return QRect(10, 10, 50, 20);}
        };
        class MoveView : public BareView {
        public:
            QSize size() override { return QSize(400, 300);}
            int forwarded = 0, shown = 0, asked = 0;
            MoveHooks hooks(bool acceptWhileDragging){
                MoveHooks h;
                h.dragParent = this;
                h.forward = [this](QMouseEvent*){ ++forwarded;};
                h.networkAccessManager = [this]() -> NetworkAccessManager* { ++asked; return nullptr;};
                h.status = [this](){ ++shown;};
                h.acceptForwardedWhileDragging = acceptWhileDragging;
                return h;
            }
            void setBank(TreeBank *bank){ m_TreeBank = bank;}
            void setRightGesture(bool on){ m_EnableRightGestureLocal = on;}
            void state(bool dragging, const QPoint &started, SharedWebElement element){
                GestureAborted();
                m_BeforeGesturePos = started;
                m_DragStarted = dragging;
                m_GestureStartedPos = started;
                m_ClickedElement = element;
                m_HadSelection = false;
                m_Gesture.clear();
            }
            void counts(int f, int s, int a){ forwarded = f; shown = s; asked = a;}
            QPoint started() const { return m_GestureStartedPos;}
            bool drawn() const { return !m_Gesture.isEmpty();}
            bool move(const QPoint &pos, Qt::MouseButtons buttons, bool acceptWhileDragging = false){
                QMouseEvent ev(QEvent::MouseMove, QPointF(pos), QPointF(pos), Qt::NoButton, buttons, Qt::NoModifier);
                ev.setAccepted(true);
                MouseMoveWithGestures(&ev, hooks(acceptWhileDragging));
                return ev.isAccepted();
            }
        };
        MoveView view;
        const SharedWebElement element = std::make_shared<Element>();

        view.state(false, QPoint(), nullptr);
        QVERIFY(view.move(QPoint(5, 5), Qt::NoButton));
        QCOMPARE(view.forwarded, 0);

        TreeBank::Initialize();
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        std::unique_ptr<MainWindow> window(Application::NewWindow(951));
        auto closeWindow = qScopeGuard([&window, &view](){
            view.state(false, QPoint(), nullptr);
            view.setBank(nullptr);
            Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
            Application::RemoveWindow(951);
            TreeBank::Initialize();
            window.reset();
        });
        view.setBank(window->GetTreeBank());

        view.state(true, QPoint(), nullptr);
        QVERIFY(view.move(QPoint(5, 5), Qt::LeftButton, true));
        QVERIFY(!view.move(QPoint(5, 5), Qt::LeftButton, false));
        QCOMPARE(view.forwarded, 2);

        view.counts(0, 0, 0);
        view.setRightGesture(true);
        view.state(false, QPoint(20, 20), nullptr);
        for(int x = 35; x <= 170; x += 15)
            QVERIFY(!view.move(QPoint(x, 20), Qt::RightButton));
        QVERIFY(view.drawn());
        QCOMPARE(view.shown, 10);
        QCOMPARE(view.forwarded, 0);

        view.counts(0, 0, 0);
        view.state(false, QPoint(20, 20), element);
        QVERIFY(!view.move(QPoint(21, 20), Qt::LeftButton));
        QCOMPARE(view.forwarded, 1);
        QCOMPARE(view.asked, 0);

        view.counts(0, 0, 0);
        QVERIFY(!view.move(QPoint(40, 20), Qt::LeftButton));
        QCOMPARE(view.asked, 1);
        QCOMPARE(view.forwarded, 1);
        QVERIFY(view.started().isNull());

        view.counts(0, 0, 0);
        view.state(false, QPoint(), nullptr);
        QVERIFY(!view.move(QPoint(40, 20), Qt::NoButton));
        QCOMPARE(view.forwarded, 1);
        QCOMPARE(view.shown, 0);

        view.state(false, QPoint(), nullptr);
        view.setBank(nullptr);
    }

    void theButtonsAndTheWheelGoWhereTheMapSays_data(){
        QTest::addColumn<bool>("acceptForwarded");
        QTest::newRow("WebEngineView") << false;
        QTest::newRow("QuickWebEngineView") << true;
    }
    void theButtonsAndTheWheelGoWhereTheMapSays(){
        QFETCH(bool, acceptForwarded);
        class ButtonView : public BareView {
        public:
            QList<Page::CustomAction> triggered;
            int pressed = 0, released = 0, doubled = 0, wheeled = 0, cleared = 0;
            void TriggerAction(Page::CustomAction action, QVariant = QVariant()) override {
                triggered << action;
            }
            ButtonHooks buttons(bool acceptForwarded){
                ButtonHooks hooks;
                hooks.pressToEngine = [this](QMouseEvent*){ ++pressed;};
                hooks.releaseToEngine = [this](QMouseEvent*){ ++released;};
                hooks.doubleClickToEngine = [this](QMouseEvent*){ ++doubled;};
                hooks.acceptForwarded = acceptForwarded;
                hooks.clearStatus = [this](){ ++cleared;};
                return hooks;
            }
            WheelHooks wheel(){
                WheelHooks hooks;
                hooks.toEngine = [this](QWheelEvent *ev){ ++wheeled; ev->setAccepted(true);};
                hooks.actionPos = [](QWheelEvent *ev){ return ev->position();};
                return hooks;
            }
            void reset(){
                triggered.clear();
                pressed = released = doubled = wheeled = cleared = 0;
                GestureAborted();
            }
            void map(const QString &key, const QString &action){ m_MouseMap[key] = action;}
            void unmap(const QString &key){ m_MouseMap.remove(key);}
            void shape(View::GestureVector v){ m_Gesture << v;}
            bool drawn() const { return !m_Gesture.isEmpty();}
            bool started() const { return !m_GestureStartedPos.isNull();}
            void dragging(){ m_DragStarted = true;}
            bool stillDragging() const { return m_DragStarted;}
            void settle(){
                for(Qt::MouseButton button : { Qt::ExtraButton1, Qt::LeftButton, Qt::RightButton })
                    m_SpentButtons.Settle(button);
            }
        };
        auto key = [](Qt::MouseButton button){
            QString key;
            Application::AddMouseButtonToString(key, button);
            return key;
        };
        ButtonView view;
        const View::ButtonHooks hooks = view.buttons(acceptForwarded);
        auto press = [&](Qt::MouseButton button){
            QMouseEvent ev(QEvent::MouseButtonPress, QPointF(30, 30), QPointF(30, 30), button, button, Qt::NoModifier);
            view.MousePressWithMap(&ev, hooks);
            return ev.isAccepted();
        };
        auto twice = [&](Qt::MouseButton button){
            QMouseEvent ev(QEvent::MouseButtonDblClick, QPointF(30, 30), QPointF(30, 30), button, button, Qt::NoModifier);
            view.MouseDoubleClickWithMap(&ev, hooks);
            return ev.isAccepted();
        };
        auto release = [&](Qt::MouseButton button, Qt::MouseButtons held = Qt::NoButton){
            QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(30, 30), QPointF(30, 30), button, held, Qt::NoModifier);
            view.MouseReleaseWithGestures(&ev, hooks);
            return ev.isAccepted();
        };
        const QString back = key(Qt::ExtraButton1);
        QString up;
        Application::AddMouseButtonsToString(up, Qt::RightButton);
        Application::AddWheelDirectionToString(up, true);
        view.map(back, QStringLiteral("Back"));
        auto cleanup = qScopeGuard([&view, &back, &up](){
            view.unmap(back);
            view.unmap(up);
            view.settle();
            view.reset();
            View::TakeRightButtonConsumed();
        });

        view.reset();
        QVERIFY(press(Qt::ExtraButton1));
        QCOMPARE(view.triggered, QList<Page::CustomAction>() << Page::_Back);
        QCOMPARE(view.pressed, 0);
        QVERIFY(release(Qt::ExtraButton1));
        QCOMPARE(view.released, 0);
        QCOMPARE(view.cleared, 1);

        view.reset();
        view.map(back, QStringLiteral("noSuchAction"));
        QVERIFY(!press(Qt::ExtraButton1));
        QCOMPARE(view.pressed, 0);
        QCOMPARE(release(Qt::ExtraButton1), acceptForwarded);
        QCOMPARE(view.released, 1);
        view.map(back, QStringLiteral("Back"));

        view.reset();
        twice(Qt::ExtraButton1);
        QCOMPARE(view.triggered.size(), 1);
        QCOMPARE(view.doubled, 0);
        release(Qt::ExtraButton1);
        QCOMPARE(view.released, 0);
        QVERIFY(!twice(Qt::LeftButton));
        QCOMPARE(view.doubled, 1);

        view.reset();
        QCOMPARE(press(Qt::RightButton), acceptForwarded);
        QVERIFY(view.started());
        QCOMPARE(view.pressed, 1);

        QCOMPARE(release(Qt::RightButton), acceptForwarded);
        QCOMPARE(view.released, 1);

        view.reset();
        press(Qt::RightButton);
        view.shape(View::Gv_Left);
        QVERIFY(release(Qt::RightButton));
        QCOMPARE(view.released, 0);
        QVERIFY(View::TakeRightButtonConsumed());

        view.reset();
        press(Qt::LeftButton);
        view.shape(View::Gv_Up);
        release(Qt::LeftButton, Qt::RightButton);
        QVERIFY(view.drawn());
        QCOMPARE(view.released, 1);
        view.reset();
        press(Qt::LeftButton);
        release(Qt::LeftButton);
        QVERIFY(!view.started());
        QCOMPARE(view.released, 1);

        view.reset();
        view.dragging();
        QVERIFY(release(Qt::LeftButton));
        QVERIFY(!view.stillDragging());
        QCOMPARE(view.released, 0);

        const auto chord = [&](Qt::MouseButton held, Qt::MouseButton pressedNow){
            QMouseEvent ev(QEvent::MouseButtonPress, QPointF(30, 30), QPointF(30, 30),
                           pressedNow, held | pressedNow, Qt::NoModifier);
            view.MousePressWithMap(&ev, hooks);
            view.settle();
        };
        const auto chordKey = [](Qt::MouseButton held, Qt::MouseButton pressedNow){
            QString name;
            Application::AddMouseButtonsToString(name, held);
            Application::AddMouseButtonToString(name, pressedNow);
            return name;
        };
        const QString rightLeft = chordKey(Qt::RightButton, Qt::LeftButton);
        const QString leftRight = chordKey(Qt::LeftButton, Qt::RightButton);
        const QString leftBack = chordKey(Qt::LeftButton, Qt::ExtraButton1);
        auto chordsGone = qScopeGuard([&](){ view.unmap(rightLeft); view.unmap(leftRight); view.unmap(leftBack); });
        view.map(rightLeft, QStringLiteral("Back"));
        view.map(leftRight, QStringLiteral("Forward"));
        view.map(leftBack, QStringLiteral("Back"));
        View::TakeRightButtonConsumed();
        view.reset();
        chord(Qt::RightButton, Qt::LeftButton);
        QVERIFY(View::TakeRightButtonConsumed());
        view.reset();
        chord(Qt::LeftButton, Qt::RightButton);
        QVERIFY(View::TakeRightButtonConsumed());
        view.reset();
        chord(Qt::LeftButton, Qt::ExtraButton1);
        QVERIFY(!View::TakeRightButtonConsumed());
        view.map(rightLeft, QStringLiteral("NoSuchAction"));
        view.reset();
        chord(Qt::RightButton, Qt::LeftButton);
        QVERIFY(!View::TakeRightButtonConsumed());

        view.map(up, QStringLiteral("PrevView"));
        view.reset();
        press(Qt::RightButton);
        QWheelEvent turn(QPointF(30, 30), QPointF(30, 30), QPoint(), QPoint(0, 120),
                         Qt::RightButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        view.WheelWithMap(&turn, view.wheel());
        QCOMPARE(view.triggered, QList<Page::CustomAction>() << Page::_PrevView);
        QVERIFY(!view.started());
        QVERIFY(View::TakeRightButtonConsumed());
        QCOMPARE(view.wheeled, 0);
        QWheelEvent made(QPointF(30, 30), QPointF(30, 30), QPoint(), QPoint(0, 120),
                         Qt::RightButton, Qt::NoModifier, Qt::NoScrollPhase, false,
                         Qt::MouseEventSynthesizedBySystem);
        view.WheelWithMap(&made, view.wheel());
        QCOMPARE(view.wheeled, 1);
        QWheelEvent plain(QPointF(30, 30), QPointF(30, 30), QPoint(), QPoint(0, -120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        view.WheelWithMap(&plain, view.wheel());
        QCOMPARE(view.wheeled, 2);

    }

    void theElementsAnswerKeepsOrderAndDropsWhatIsOffScreen(){
        BareView view;
        auto element = [](int x, const QString &text){
            QVariantMap map;
            map[QStringLiteral("x")] = x;
            map[QStringLiteral("y")] = 10;
            map[QStringLiteral("width")] = 20;
            map[QStringLiteral("height")] = 20;
            map[QStringLiteral("innerText")] = text;
            return QVariant(map);
        };
        QVariantMap answer;
        answer[QStringLiteral("0")] = element(10, QStringLiteral("on"));
        answer[QStringLiteral("1")] = element(500, QStringLiteral("off"));
        const QRect viewport(0, 0, 100, 100);

        SharedWebElementList list = View::ElementsFrom(&view, answer, viewport);
        QCOMPARE(list.size(), 2);
        QCOMPARE(list[0]->InnerText(), QStringLiteral("on"));
        QCOMPARE(list[0]->Rectangle(), QRect(10, 10, 20, 20));
        QCOMPARE(list[1]->InnerText(), QStringLiteral("off"));
        QCOMPARE(list[1]->Rectangle(), QRect());

        auto shift = [](const QRect &rect){ return rect.translated(-450, 0);};
        list = View::ElementsFrom(&view, answer, viewport, shift);
        QCOMPARE(list[0]->Rectangle(), QRect());
        QCOMPARE(list[1]->Rectangle(), QRect(50, 10, 20, 20));

        QVERIFY(View::ElementsFrom(&view, QVariant(), viewport).isEmpty());
    }

    void theSelectionAnswerIsCutToTheViewport(){
        auto rect = [](int x, int y, int w, int h){
            QVariantMap map;
            map[QStringLiteral("x")] = x;
            map[QStringLiteral("y")] = y;
            map[QStringLiteral("width")] = w;
            map[QStringLiteral("height")] = h;
            return QVariant(map);
        };
        QVariantMap answer;
        answer[QStringLiteral("0")] = rect(10, 10, 20, 10);
        answer[QStringLiteral("1")] = rect(90, 50, 30, 10);
        const QRect viewport(0, 0, 100, 100);

        QCOMPARE(View::SelectionRegionFrom(answer, viewport),
                 QRegion(10, 10, 20, 10) | QRegion(90, 50, 10, 10));
        QCOMPARE(View::SelectionRegionFrom(answer, viewport,
                                           [](const QRect &r){ return r.translated(0, 80);}),
                 QRegion(10, 90, 20, 10));
        QCOMPARE(View::SelectionRegionFrom(QVariant(), viewport), QRegion());
        QCOMPARE(View::SelectionRegionFrom(QVariant(QStringLiteral("x")), viewport), QRegion());
    }
};

QTEST_MAIN(tst_viewinterface)
#include "tst_viewinterface.moc"
