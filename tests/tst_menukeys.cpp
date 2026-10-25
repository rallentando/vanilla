#include "switch.hpp"

#include <QtTest>
#include <QAction>
#include <QKeySequence>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QWidget>

#include "menukeys.hpp"

class tst_menukeys : public QObject {
    Q_OBJECT

    static QAction *Named(QMenu *menu, const char *name){
        QAction *action = menu->addAction(QLatin1String(name));
        action->setObjectName(QLatin1String(name));
        return action;
    }

private slots:

    void eachNamedActionShowsItsKeyTheMenusUnderItToo(){
        QMenu menu;
        QAction *back  = Named(&menu, "Back");
        QAction *close = Named(&menu, "Close");
        menu.addSeparator();
        QAction *plain = menu.addAction(QStringLiteral("plain"));
        plain->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
        QMenu *sub = menu.addMenu(QStringLiteral("Sub"));
        QAction *quit = Named(sub, "Quit");

        close->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));

        QAction *prev = Named(&menu, "PrevView");
        QHash<QString, QKeySequence> keys;
        keys[QStringLiteral("Back")] = QKeySequence(Qt::ALT | Qt::Key_Left);
        keys[QStringLiteral("Quit")] = QKeySequence(Qt::CTRL | Qt::Key_Q);
        keys[QStringLiteral("PrevView")] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab);
        MenuKeys::Show(&menu, keys);
        QCOMPARE(prev->shortcut(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));

        QCOMPARE(back->shortcut(), QKeySequence(Qt::ALT | Qt::Key_Left));
        QCOMPARE(quit->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_Q));
        QVERIFY(close->shortcut().isEmpty());
        QCOMPARE(plain->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_P));

        foreach(QAction *action, QList<QAction*>() << back << close << quit){
            QCOMPARE(action->shortcutContext(), Qt::WidgetShortcut);
            QVERIFY(action->isShortcutVisibleInContextMenu());
        }
    }

    void aKeyShownIsNeverLive(){
        QWidget window;
        QLineEdit *edit = new QLineEdit(&window);
        window.show();
        if(!QTest::qWaitForWindowActive(&window)) QSKIP("no active window on this platform");
        edit->setFocus();

        QMenu *menu = new QMenu(&window);
        QAction *back = Named(menu, "Back");
        QSignalSpy triggered(back, &QAction::triggered);
        QHash<QString, QKeySequence> keys;
        keys[QStringLiteral("Back")] = QKeySequence(Qt::CTRL | Qt::Key_B);
        MenuKeys::Show(menu, keys);
        QCOMPARE(back->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_B));

        QTest::keyClick(edit, Qt::Key_B, Qt::ControlModifier);
        QTest::keyClick(&window, Qt::Key_B, Qt::ControlModifier);
        QCOMPARE(triggered.count(), 0);
    }

    void aKeyOfTheMenuBarsMenuIsNotLiveEither(){
        QMainWindow window;
        QMenu *menu = window.menuBar()->addMenu(QStringLiteral("Menu"));
        QAction *back = Named(menu, "Back");
        QSignalSpy triggered(back, &QAction::triggered);
        QHash<QString, QKeySequence> keys;
        keys[QStringLiteral("Back")] = QKeySequence(Qt::CTRL | Qt::Key_B);
        MenuKeys::Show(menu, keys);
        window.show();
        if(!QTest::qWaitForWindowActive(&window)) QSKIP("no active window on this platform");
        window.menuBar()->setFocus();
        QVERIFY(window.menuBar()->hasFocus());
        QTest::keyClick(window.menuBar(), Qt::Key_B, Qt::ControlModifier);
        QCOMPARE(triggered.count(), 0);
        MenuKeys::Show(menu, keys);
        QCOMPARE(back->findChildren<QObject*>(QStringLiteral("MenuKeys::Mute")).size(), 1);
    }

    void nothingToShowIsNoError(){
        MenuKeys::Show(nullptr, QHash<QString, QKeySequence>());
        QMenu menu;
        MenuKeys::Show(&menu, QHash<QString, QKeySequence>());
        QVERIFY(menu.actions().isEmpty());
    }
};

QTEST_MAIN(tst_menukeys)
#include "tst_menukeys.moc"
