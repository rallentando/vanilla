#include <QtTest>
#include <QWindow>
#include <QWidget>
#include <QLineEdit>
#include <QGuiApplication>

#include "switch.hpp"
#include "edgehostwindow.hpp"

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

class tst_edgehostwindow : public QObject {
    Q_OBJECT

private slots:
    void aPlainWindowTakesTheFocusWhenAskedToActivate();
    void theHostDoesNotTakeTheFocusWhenAskedToActivate();
    void theHostAnswersMouseActivateWithActivate();
    void theContainerOfTheHostCanHoldQtsFocus();
    void aPressGivesQtsFocusToTheWidgetAskedFor();
    void theContainersFocusDoesNotAskTheHostToActivate();
    void comingBackToTheWindowSaysSoOnce();
    void aWidgetTakingTheFocusOnActivationIsNotOverridden();
    void comingBackFromTheBackendsWindowSaysSoWithoutAFocusIn();
};

void tst_edgehostwindow::aPlainWindowTakesTheFocusWhenAskedToActivate(){
    QWindow other;
    other.show();
    other.requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &other);

    QWindow plain;
    plain.show();
    other.requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &other);
    plain.requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &plain);
}

void tst_edgehostwindow::theHostDoesNotTakeTheFocusWhenAskedToActivate(){
    QWindow other;
    other.show();
    other.requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &other);

    EdgeHostWindow host;
    QVERIFY(host.flags() & Qt::WindowDoesNotAcceptFocus);
    host.show();
    other.requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &other);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("WindowDoesNotAcceptFocus")));
    host.requestActivate();
    QTest::qWait(100);
    QCOMPARE(QGuiApplication::focusWindow(), &other);
}

void tst_edgehostwindow::theHostAnswersMouseActivateWithActivate(){
#ifdef Q_OS_WIN
    qintptr result = 0;
    QVERIFY(EdgeHostWindow::AnswerMouseActivate(WM_MOUSEACTIVATE, &result));
    QCOMPARE(result, qintptr(MA_ACTIVATE));

    result = 42;
    QVERIFY(!EdgeHostWindow::AnswerMouseActivate(WM_LBUTTONDOWN, &result));
    QCOMPARE(result, qintptr(42));
    QVERIFY(!EdgeHostWindow::AnswerMouseActivate(WM_MOUSEACTIVATE, nullptr));
#else
    QSKIP("WM_MOUSEACTIVATE is a Windows message");
#endif
}

void tst_edgehostwindow::theContainerOfTheHostCanHoldQtsFocus(){
    QWidget top;
    EdgeHostWindow *host = new EdgeHostWindow();
    QWidget *container = QWidget::createWindowContainer(host, &top);
    container->setFocusPolicy(Qt::StrongFocus);
    container->resize(100, 100);
    top.show();
    QVERIFY(QTest::qWaitForWindowActive(&top));

    container->setFocus();
    QTRY_VERIFY(container->hasFocus());
    QCOMPARE(QApplication::focusWidget(), container);
}

void tst_edgehostwindow::aPressGivesQtsFocusToTheWidgetAskedFor(){
    QWidget top;
    QLineEdit *edit = new QLineEdit(&top);
    QWidget *view = new QWidget(&top);
    EdgeHostWindow *host = new EdgeHostWindow();
    QWidget *container = QWidget::createWindowContainer(host, view);
    container->setFocusPolicy(Qt::StrongFocus);
    container->resize(100, 100);
    view->setFocusProxy(container);
    view->move(0, 40);
    top.show();
    QVERIFY(QTest::qWaitForWindowActive(&top));

    edit->setFocus();
    QTRY_VERIFY(edit->hasFocus());

    QVERIFY(!host->TakeQtFocus());
    QVERIFY(edit->hasFocus());
    QVERIFY(!view->hasFocus());

    host->GiveQtFocusOnPress(container);
    QVERIFY(host->TakeQtFocus());
    QTRY_VERIFY(view->hasFocus());
    QVERIFY(!edit->hasFocus());
    QCOMPARE(QApplication::focusWidget(), container);

    QVERIFY(!host->TakeQtFocus());
    QVERIFY(view->hasFocus());
}

void tst_edgehostwindow::theContainersFocusDoesNotAskTheHostToActivate(){
    QWidget top;
    QLineEdit *edit = new QLineEdit(&top);
    EdgeHostWindow *host = new EdgeHostWindow();
    QWidget *container = QWidget::createWindowContainer(host, &top);
    container->setFocusPolicy(Qt::StrongFocus);
    container->resize(100, 100);
    container->move(0, 40);
    host->QuietContainerFocusIn(container);
    top.show();
    QVERIFY(QTest::qWaitForWindowActive(&top));

    QTest::failOnWarning(QRegularExpression(QStringLiteral("WindowDoesNotAcceptFocus")));
    container->setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(container->hasFocus());
    edit->setFocus();
    QTRY_VERIFY(edit->hasFocus());
    QVERIFY(!container->hasFocus());
    container->setFocus(Qt::TabFocusReason);
    QTRY_VERIFY(container->hasFocus());
    QCOMPARE(QApplication::focusWidget(), container);
    QTest::qWait(50);
    QVERIFY(QGuiApplication::focusWindow() != host);
}

void tst_edgehostwindow::comingBackToTheWindowSaysSoOnce(){
    QWidget top;
    QLineEdit *edit = new QLineEdit(&top);
    EdgeHostWindow *host = new EdgeHostWindow();
    QWidget *container = QWidget::createWindowContainer(host, &top);
    container->setFocusPolicy(Qt::StrongFocus);
    container->resize(100, 100);
    container->move(0, 40);
    host->QuietContainerFocusIn(container);
    top.show();
    QVERIFY(QTest::qWaitForWindowActive(&top));
    QTest::qWait(50);
    QSignalSpy spy(host, &EdgeHostWindow::ReactivatedWithFocus);

    edit->setFocus();
    QTRY_VERIFY(edit->hasFocus());
    container->setFocus(Qt::TabFocusReason);
    QTRY_VERIFY(container->hasFocus());
    QTest::qWait(50);
    QCOMPARE(spy.count(), 0);

    QWidget other;
    other.show();
    other.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&other));
    QVERIFY(!container->hasFocus());
    QCOMPARE(spy.count(), 0);

    top.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&top));
    QTRY_COMPARE(spy.count(), 1);
    QVERIFY(container->hasFocus());
    QTest::qWait(50);
    QCOMPARE(spy.count(), 1);
}

void tst_edgehostwindow::aWidgetTakingTheFocusOnActivationIsNotOverridden(){
    QWidget top;
    QLineEdit *edit = new QLineEdit(&top);
    EdgeHostWindow *host = new EdgeHostWindow();
    QWidget *container = QWidget::createWindowContainer(host, &top);
    container->setFocusPolicy(Qt::StrongFocus);
    container->resize(100, 100);
    container->move(0, 40);
    host->QuietContainerFocusIn(container);
    top.show();
    QVERIFY(QTest::qWaitForWindowActive(&top));
    container->setFocus();
    QTRY_VERIFY(container->hasFocus());

    QWidget other;
    other.show();
    other.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&other));

    struct TakeOnActivation : QObject {
        QWidget *taker;
        bool fired = false;
        bool eventFilter(QObject *, QEvent *ev) override {
            if(ev->type() == QEvent::FocusIn &&
               static_cast<QFocusEvent*>(ev)->reason() == Qt::ActiveWindowFocusReason){
                taker->setFocus();
                fired = true;
            }
            return false;
        }
    } take;
    take.taker = edit;
    container->installEventFilter(&take);

    QSignalSpy spy(host, &EdgeHostWindow::ReactivatedWithFocus);
    top.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&top));
    QTRY_VERIFY(edit->hasFocus());
    QVERIFY(take.fired);
    QTest::qWait(50);
    QCOMPARE(spy.count(), 0);
    container->removeEventFilter(&take);
}

void tst_edgehostwindow::comingBackFromTheBackendsWindowSaysSoWithoutAFocusIn(){
    QWidget top;
    EdgeHostWindow *host = new EdgeHostWindow();
    QWidget *container = QWidget::createWindowContainer(host, &top);
    container->setFocusPolicy(Qt::StrongFocus);
    container->resize(100, 100);
    host->QuietContainerFocusIn(container);
    top.show();
    QVERIFY(QTest::qWaitForWindowActive(&top));
    container->setFocus();
    QTRY_VERIFY(container->hasFocus());

    QWindow *backend = new QWindow(top.windowHandle());
    backend->resize(10, 10);
    backend->show();
    backend->requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), backend);
    QTest::qWait(50);

    int focusIns = 0;
    struct CountFocusIn : QObject {
        int *count;
        bool eventFilter(QObject *, QEvent *ev) override {
            if(ev->type() == QEvent::FocusIn) ++*count;
            return false;
        }
    } counter;
    counter.count = &focusIns;
    container->installEventFilter(&counter);

    QSignalSpy spy(host, &EdgeHostWindow::ReactivatedWithFocus);
    top.windowHandle()->requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), top.windowHandle());
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(focusIns, 0);
    QVERIFY(container->hasFocus());
    container->removeEventFilter(&counter);
}

QTEST_MAIN(tst_edgehostwindow)
#include "tst_edgehostwindow.moc"
