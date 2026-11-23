#include "switch.hpp"

#include <QtTest>
#include <QAction>
#include <QKeySequence>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QWidget>
#include <QWidgetAction>
#include <QSignalSpy>

#include "menukeys.hpp"
#include "menuiconrow.hpp"
#include "menuschema.hpp"

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

    void aLineOfIconsIsChosenWithTheKeys(){
        QMenu menu;
        MenuIconRow *row = new MenuIconRow(&menu);
        QAction *back = new QAction(QStringLiteral("Back"), &menu);
        back->setObjectName(QStringLiteral("Back"));
        QAction *forward = new QAction(QStringLiteral("Forward"), &menu);
        QAction *reload = new QAction(QStringLiteral("Reload"), &menu);
        QAction *line = menu.addAction(QStringLiteral("Line"));
        row->AddCell(QStringLiteral("Back"), back, QStringLiteral("Back"));
        row->AddCell(QStringLiteral("Forward"), forward, QStringLiteral("Forward"), false);
        row->AddCell(QStringLiteral("Reload"), reload, QStringLiteral("Reload"));
        QWidgetAction *icons = row->Install();
        QCOMPARE(menu.actions().first(), icons);
        QCOMPARE(MenuIconRow::Of(&menu), row);
        QVERIFY(row->Holds(QStringLiteral("Forward")));
        QVERIFY(!row->Holds(QStringLiteral("Line")));

        QHash<QString, QKeySequence> keys;
        keys.insert(QStringLiteral("Back"), QKeySequence(Qt::ALT | Qt::Key_Left));
        MenuKeys::Show(&menu, keys);
        QVERIFY(!back->shortcut().isEmpty());

        QSignalSpy ranBack(back, &QAction::triggered);
        QSignalSpy ranForward(forward, &QAction::triggered);
        QSignalSpy ranReload(reload, &QAction::triggered);

        menu.popup(QPoint(0, 0));
        QVERIFY(QTest::qWaitForWindowExposed(&menu));
        QTest::keyClick(&menu, Qt::Key_Down);
        QCOMPARE(menu.activeAction(), icons);
        QCOMPARE(menu.focusWidget(), row);

        QTest::keyClick(row, Qt::Key_Down);
        QCOMPARE(menu.activeAction(), line);
        QTest::keyClick(&menu, Qt::Key_Up);
        QCOMPARE(menu.activeAction(), icons);
        QCOMPARE(menu.focusWidget(), row);

        QTest::keyClick(row, Qt::Key_Right);
        QTest::keyClick(row, Qt::Key_Return);
        QCOMPARE(ranReload.count(), 1);
        QCOMPARE(ranForward.count(), 0);
        QCOMPARE(ranBack.count(), 0);
        QVERIFY(!menu.isVisible());

        QMenu twice;
        MenuIconRow *again = new MenuIconRow(&twice);
        again->AddCell(QStringLiteral("NewNode"), back, QStringLiteral("New"));
        again->AddCell(QStringLiteral("NewNode"), forward, QStringLiteral("New"));
        QCOMPARE(again->CellCount(), 1);
    }

    void theKeysFollowThePointerIntoTheLineOfIcons(){
        QMenu menu;
        MenuIconRow *row = new MenuIconRow(&menu);
        QAction *back = new QAction(QStringLiteral("Back"), &menu);
        QAction *forward = new QAction(QStringLiteral("Forward"), &menu);
        QAction *reload = new QAction(QStringLiteral("Reload"), &menu);
        menu.addAction(QStringLiteral("Line"));
        row->AddCell(QStringLiteral("Back"), back, QStringLiteral("Back"));
        row->AddCell(QStringLiteral("Forward"), forward, QStringLiteral("Forward"), false);
        row->AddCell(QStringLiteral("Reload"), reload, QStringLiteral("Reload"));
        QWidgetAction *icons = row->Install();
        QSignalSpy ranBack(back, &QAction::triggered);
        QSignalSpy ranReload(reload, &QAction::triggered);

        menu.popup(QPoint(0, 0));
        QVERIFY(QTest::qWaitForWindowExposed(&menu));
        QVERIFY(menu.focusWidget() != row);
        const QPoint first(12, row->height() / 2);
        QTest::mouseMove(row, first);
        QTRY_COMPARE(menu.activeAction(), icons);
        QCOMPARE(menu.focusWidget(), row);

        QWidget *keys = menu.focusWidget();
        QTest::keyClick(keys, Qt::Key_Right);
        QTest::keyClick(keys, Qt::Key_Return);
        QCOMPARE(ranReload.count(), 1);
        QCOMPARE(ranBack.count(), 0);
        QVERIFY(!menu.isVisible());
    }

    void everyIconOfTheTablesIsDrawnTheSameSize(){
        QWidget ink;
        QSize size;
        for(const MenuSchema::Table &table : MenuSchema::Tables()){
            if(!table.icons) continue;
            QVERIFY(!MenuSchema::IsKnown(table, QStringLiteral("Separator")));
            QVERIFY(!MenuSchema::IsKnown(table, QStringLiteral("OpenCommand")));
            for(const QString &name : table.defaults.split(QLatin1Char(',')))
                QVERIFY2(table.vocabulary.contains(name), qPrintable(name));
            for(const QString &name : table.vocabulary){
                const QString icon = MenuIconRow::IconOf(name);
                QVERIFY2(!icon.isEmpty(), qPrintable(name));
                const QImage image = MenuIconRow::Icon(icon, &ink).pixmap(64).toImage();
                if(size.isEmpty()) size = image.size();
                QCOMPARE(image.size(), size);
                bool inked = false;
                for(int y = 0; y < image.height() && !inked; ++y)
                    for(int x = 0; x < image.width() && !inked; ++x)
                        inked = qAlpha(image.pixel(x, y)) > 0;
                QVERIFY2(inked, qPrintable(name));
            }
        }
        for(const char *icon : { "Stop", "Muted", "Trash", "Dots" })
            QVERIFY(!MenuIconRow::Icon(QString::fromLatin1(icon), &ink).pixmap(64).toImage().isNull());

        for(const char *lines : { "RegularMenu", "LinkMenu", "ImageMenu", "MediaMenu", "TextMenu",
                                  "SelectionMenu", "TreeBarNodeMenu", "TableNodeMenu" })
            QVERIFY2(MenuSchema::IconsOf(QString::fromLatin1(lines)), lines);
        QCOMPARE(MenuSchema::Default(QStringLiteral("LinkIcons")), QStringLiteral("OpenLink,DownloadLink"));
        QCOMPARE(MenuSchema::Default(QStringLiteral("ImageIcons")), QStringLiteral("OpenImage,DownloadImage"));
        QCOMPARE(MenuSchema::Default(QStringLiteral("MediaIcons")),
                 QStringLiteral("ToggleMediaPlayPause,ToggleMediaMute,ToggleMediaLoop"));
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
