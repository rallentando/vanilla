#include "switch.hpp"

#include <QtTest>
#include <QClipboard>
#include <QLineEdit>
#include <QMenu>

#include "editmenu.hpp"
#include "receiver.hpp"

class tst_editmenu : public QObject {
    Q_OBJECT

    static QStringList Names(QMenu *menu){
        QStringList names;
        foreach(QAction *action, menu->actions())
            names << (action->isSeparator() ? QStringLiteral("Separator") : action->objectName());
        return names;
    }

    static QAction *Find(QMenu *menu, const char *name){
        foreach(QAction *action, menu->actions())
            if(action->objectName() == QLatin1String(name)) return action;
        return nullptr;
    }

private slots:

    void theNamesAreThePagesInThePagesOrderAndWhatIsNotAnEditIsLeftOut(){
        QLineEdit edit;
        std::unique_ptr<QMenu> menu(EditMenu::Build(&edit, QStringList()
            << QStringLiteral("Undo") << QStringLiteral("Redo") << QStringLiteral("Separator")
            << QStringLiteral("CopySelectedHtml") << QStringLiteral("SelectAll")
            << QStringLiteral("Paste") << QStringLiteral("Cut")));
        QCOMPARE(Names(menu.get()), QStringList()
                 << QStringLiteral("Undo") << QStringLiteral("Redo") << QStringLiteral("Separator")
                 << QStringLiteral("SelectAll") << QStringLiteral("Paste") << QStringLiteral("Cut"));
        QCOMPARE(Find(menu.get(), "SelectAll")->text(), QCoreApplication::translate("Page", "SelectAll"));
        std::unique_ptr<QMenu> leading(EditMenu::Build(&edit, QStringList()
            << QStringLiteral("Separator") << QStringLiteral("Copy")));
        QCOMPARE(Names(leading.get()), QStringList() << QStringLiteral("Copy"));
    }

    void copyJoinsTheMenuWhileSomethingIsSelected(){
        QLineEdit edit;
        edit.setText(QStringLiteral("abc"));
        const QStringList plain = EditMenu::NamesFor(&edit);
        QVERIFY(!plain.contains(QStringLiteral("Copy")));
        edit.selectAll();
        const QStringList selected = EditMenu::NamesFor(&edit);
        QCOMPARE(selected.mid(plain.size()), QStringList() << QStringLiteral("Separator") << QStringLiteral("Copy"));
    }

    void eachItemIsEnabledAsTheFieldStands(){
        QGuiApplication::clipboard()->clear();
        QLineEdit edit;
        const QStringList all = QStringList()
            << QStringLiteral("Undo") << QStringLiteral("Redo") << QStringLiteral("Cut")
            << QStringLiteral("Copy") << QStringLiteral("Paste") << QStringLiteral("Delete")
            << QStringLiteral("SelectAll");

        std::unique_ptr<QMenu> empty(EditMenu::Build(&edit, all));
        foreach(QAction *action, empty->actions()) QVERIFY2(!action->isEnabled(), qPrintable(action->objectName()));

        QTest::keyClicks(&edit, QStringLiteral("abc"));
        edit.selectAll();
        std::unique_ptr<QMenu> selected(EditMenu::Build(&edit, all));
        QVERIFY(Find(selected.get(), "Undo")->isEnabled());
        QVERIFY(!Find(selected.get(), "Redo")->isEnabled());
        QVERIFY(Find(selected.get(), "Cut")->isEnabled());
        QVERIFY(Find(selected.get(), "Copy")->isEnabled());
        QVERIFY(!Find(selected.get(), "Paste")->isEnabled());
        QVERIFY(Find(selected.get(), "Delete")->isEnabled());
        QVERIFY(Find(selected.get(), "SelectAll")->isEnabled());

        edit.setEchoMode(QLineEdit::Password);
        std::unique_ptr<QMenu> secret(EditMenu::Build(&edit, all));
        QVERIFY(!Find(secret.get(), "Cut")->isEnabled());
        QVERIFY(!Find(secret.get(), "Copy")->isEnabled());
        edit.setEchoMode(QLineEdit::Normal);

        QGuiApplication::clipboard()->setText(QStringLiteral("x"));
        std::unique_ptr<QMenu> pastable(EditMenu::Build(&edit, all));
        QVERIFY(Find(pastable.get(), "Paste")->isEnabled());
        edit.setReadOnly(true);
        std::unique_ptr<QMenu> readOnly(EditMenu::Build(&edit, all));
        QVERIFY(!Find(readOnly.get(), "Paste")->isEnabled());
        QVERIFY(!Find(readOnly.get(), "Cut")->isEnabled());
        QVERIFY(Find(readOnly.get(), "Copy")->isEnabled());
        QGuiApplication::clipboard()->clear();
    }

    void theKeysAreShownAndTheItemsActOnTheField(){
        QGuiApplication::clipboard()->clear();
        QLineEdit edit;
        QTest::keyClicks(&edit, QStringLiteral("abc"));
        edit.selectAll();
        const QStringList all = QStringList()
            << QStringLiteral("Undo") << QStringLiteral("Cut") << QStringLiteral("Copy")
            << QStringLiteral("Paste") << QStringLiteral("SelectAll");
        auto trigger = [&edit, &all](const char *name){
            std::unique_ptr<QMenu> menu(EditMenu::Build(&edit, all));
            QAction *action = Find(menu.get(), name);
            QVERIFY2(action && action->isEnabled(), name);
            action->trigger();
        };
        std::unique_ptr<QMenu> menu(EditMenu::Build(&edit, all));
        QCOMPARE(Find(menu.get(), "Copy")->shortcut(), QKeySequence(QKeySequence::Copy));
        QCOMPARE(Find(menu.get(), "SelectAll")->shortcut(), QKeySequence(QKeySequence::SelectAll));
        QCOMPARE(Find(menu.get(), "Copy")->shortcutContext(), Qt::WidgetShortcut);
        QVERIFY(Find(menu.get(), "Copy")->isShortcutVisibleInContextMenu());

        trigger("Copy");
        QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("abc"));
        trigger("Cut");
        QCOMPARE(edit.text(), QString());
        trigger("Paste");
        QCOMPARE(edit.text(), QStringLiteral("abc"));
        edit.deselect();
        trigger("SelectAll");
        QCOMPARE(edit.selectedText(), QStringLiteral("abc"));
        trigger("Undo");
        QCOMPARE(edit.text(), QString());
        QGuiApplication::clipboard()->clear();
    }

    void installingMakesTheFieldAskForItsOwnMenu(){
        QLineEdit edit;
        EditMenu::Install(&edit);
        QCOMPARE(edit.contextMenuPolicy(), Qt::CustomContextMenu);
    }

    void aRightPressOnTheAddressBarKeepsTheSelectionForTheMenu(){
        LineEdit edit;
        edit.setContextMenuPolicy(Qt::NoContextMenu);
        edit.setText(QStringLiteral("abc"));
        edit.selectAll();
        QTest::mousePress(&edit, Qt::RightButton);
        QTest::mouseRelease(&edit, Qt::RightButton);
        QCOMPARE(edit.selectedText(), QStringLiteral("abc"));
        QVERIFY(EditMenu::NamesFor(&edit).contains(QStringLiteral("Copy")));
        QTest::mousePress(&edit, Qt::LeftButton);
        QTest::mouseRelease(&edit, Qt::LeftButton);
        QVERIFY(!edit.hasSelectedText());
    }
};

QTEST_MAIN(tst_editmenu)
#include "tst_editmenu.moc"
