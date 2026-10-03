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

QTEST_MAIN(tst_edgehostwindow)
#include "tst_edgehostwindow.moc"
