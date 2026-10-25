#include "switch.hpp"

#include <QtTest>
#include <QWindow>

#include "application.hpp"
#include "page.hpp"

class tst_keymodifiers : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void aKeysOwnModifiersAreNotAsked();
    void notifyTakesKeysAndClicksOnly();
    void anAnswerLaterIsAskedForAsItsKeyWas();

private:
    QWindow *m_Window = nullptr;
};

void tst_keymodifiers::init(){
    m_Window = new QWindow;
    m_Window->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_Window));
    QTest::keyPress(m_Window, Qt::Key_Control, Qt::ControlModifier);
    QTest::keyPress(m_Window, Qt::Key_Shift, Qt::ControlModifier | Qt::ShiftModifier);
    QVERIFY(Application::keyboardModifiers() & Qt::ControlModifier);
}

void tst_keymodifiers::cleanup(){
    QTest::keyRelease(m_Window, Qt::Key_Shift, Qt::ControlModifier);
    QTest::keyRelease(m_Window, Qt::Key_Control, Qt::NoModifier);
    QVERIFY(!Page::CtrlMod());
    delete m_Window;
    m_Window = nullptr;
}

void tst_keymodifiers::notifyTakesKeysAndClicksOnly(){
    QKeyEvent press(QEvent::KeyPress, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(Application::ByKeyFor(&press), std::optional<bool>(true));
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(Application::ByKeyFor(&release), std::optional<bool>());

    for(QEvent::Type type : { QEvent::MouseButtonPress, QEvent::MouseButtonRelease,
                              QEvent::MouseButtonDblClick }){
        QMouseEvent click(type, QPointF(1, 1), QPointF(1, 1),
                          Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
        QCOMPARE(Application::ByKeyFor(&click), std::optional<bool>(false));
    }
    QMouseEvent move(QEvent::MouseMove, QPointF(1, 1), QPointF(1, 1),
                     Qt::NoButton, Qt::NoButton, Qt::ControlModifier);
    QCOMPARE(Application::ByKeyFor(&move), std::optional<bool>());
}

void tst_keymodifiers::anAnswerLaterIsAskedForAsItsKeyWas(){
    bool ctrl = true, shift = true;
    std::function<void(QList<QNetworkRequest>)> later;
    {
        Application::InputScope key(true);
        later = Page::AsAskedBy([&](QList<QNetworkRequest>){
            ctrl = Page::CtrlMod();
            shift = Page::ShiftMod();
        });
    }
    later(QList<QNetworkRequest>());
    QVERIFY(!ctrl);
    QVERIFY(!shift);
    QVERIFY(!Application::ByKey());

    later = Page::AsAskedBy([&](QList<QNetworkRequest>){
        ctrl = Page::CtrlMod();
    });
    {
        Application::InputScope key(true);
        later(QList<QNetworkRequest>());
    }
    QVERIFY(ctrl);
}

void tst_keymodifiers::aKeysOwnModifiersAreNotAsked(){
    QVERIFY(Page::CtrlMod());
    QVERIFY(Page::ShiftMod());

    {
        Application::InputScope key(true);
        QVERIFY(Application::ByKey());
        QVERIFY(!Page::CtrlMod());
        QVERIFY(!Page::ShiftMod());
        {
            Application::InputScope click(false);
            QVERIFY(Page::CtrlMod());
            QVERIFY(Page::ShiftMod());
        }
        QVERIFY(!Page::CtrlMod());
    }
    QVERIFY(!Application::ByKey());
    QVERIFY(Page::CtrlMod());
}

QTEST_MAIN(tst_keymodifiers)
#include "tst_keymodifiers.moc"
