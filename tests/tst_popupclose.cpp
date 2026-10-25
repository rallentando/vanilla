#include "switch.hpp"
#include "application.hpp"

#include <QtTest>
#include <QMenu>
#include <QWidget>
#include <QPointer>

class tst_popupclose : public QObject {
    Q_OBJECT

    class Refusing : public QWidget {
    public:
        int asked = 0;
        Refusing() : QWidget(nullptr, Qt::Popup) {}
    protected:
        void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE { ++asked; ev->ignore(); }
    };

private slots:
    void nothingOpenIsNoop(){
        QVERIFY(!QApplication::activePopupWidget());
        Application::CloseAllPopups();
        QVERIFY(!QApplication::activePopupWidget());
    }

    void menuCloses(){
        QMenu menu;
        menu.addAction(QStringLiteral("one"));
        menu.popup(QPoint(10, 10));
        QTRY_COMPARE(QApplication::activePopupWidget(), &menu);
        Application::CloseAllPopups();
        QVERIFY(!QApplication::activePopupWidget());
        QVERIFY(!menu.isVisible());
    }

    void stackedPopupsAllClose(){
        QWidget panel(nullptr, Qt::Popup);
        panel.show();
        QTRY_COMPARE(QApplication::activePopupWidget(), &panel);
        QMenu menu;
        menu.addAction(QStringLiteral("one"));
        menu.popup(QPoint(10, 10));
        QTRY_COMPARE(QApplication::activePopupWidget(), &menu);
        Application::CloseAllPopups();
        QVERIFY(!QApplication::activePopupWidget());
        QVERIFY(!menu.isVisible());
        QVERIFY(!panel.isVisible());
    }

    void refusingPopupStopsTheLoop(){
        Refusing popup;
        popup.show();
        QTRY_COMPARE(QApplication::activePopupWidget(), &popup);
        Application::CloseAllPopups();
        QCOMPARE(popup.asked, 1);
        QCOMPARE(QApplication::activePopupWidget(), &popup);
        popup.hide();
        QVERIFY(!QApplication::activePopupWidget());
    }
};

QTEST_MAIN(tst_popupclose)
#include "tst_popupclose.moc"
