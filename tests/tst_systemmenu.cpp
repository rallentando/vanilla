#include "switch.hpp"
#include "systemmenu.hpp"

#include <QtTest>
#include <QMenu>
#include <QAction>
#include <QKeySequence>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

class tst_systemmenu : public QObject {
    Q_OBJECT

private slots:
    void idsStayBelowTheSystemsWithTheLowBitsClear();
    void indexOfDropsTheBitsTheSystemUses();
    void indexOfRefusesWhatIsNotACommandOfOurs();
    void labelPutsTheKeyAfterATab();
    void entriesFollowTheMenuAndNumberTheCommandsInOrder();
    void hiddenActionsAreLeftOut();
#if defined(Q_OS_WIN)
    void installKeepsTheSystemsAndOthersItemsAndPutsOursInOnce();
#endif
};

void tst_systemmenu::idsStayBelowTheSystemsWithTheLowBitsClear(){
    QCOMPARE(SystemMenu::IdOf(0), SystemMenu::FirstId);
    QCOMPARE(SystemMenu::IdOf(1), SystemMenu::FirstId + SystemMenu::IdStep);
    QCOMPARE(SystemMenu::IdOf(-1), -1);
    int last = -1;
    for(int i = 0; SystemMenu::IdOf(i) >= 0; i++){
        const int id = SystemMenu::IdOf(i);
        QVERIFY2((id & 0xF) == 0, "the low four bits are the system's");
        QVERIFY2(id < 0xF000, "0xF000 and up are the system's commands");
        QVERIFY(SystemMenu::IsOurs(id));
        last = i;
    }
    QVERIFY(last > 1000);
    QCOMPARE(SystemMenu::IdOf(last + 1), -1);
    QVERIFY(SystemMenu::IsOurs(SystemMenu::MarkId));
    QVERIFY(!SystemMenu::IsOurs(0));
    QVERIFY(!SystemMenu::IsOurs(0xF060));
}

void tst_systemmenu::indexOfDropsTheBitsTheSystemUses(){
    QCOMPARE(SystemMenu::IndexOf(static_cast<quintptr>(SystemMenu::IdOf(5))), 5);
    QCOMPARE(SystemMenu::IndexOf(static_cast<quintptr>(SystemMenu::IdOf(5) | 0x7)), 5);
    QCOMPARE(SystemMenu::IndexOf(static_cast<quintptr>(SystemMenu::IdOf(0))), 0);
    int last = 0;
    while(SystemMenu::IdOf(last + 1) >= 0) last++;
    QCOMPARE(SystemMenu::IndexOf(static_cast<quintptr>(SystemMenu::IdOf(last) | 0xF)), last);
}

void tst_systemmenu::indexOfRefusesWhatIsNotACommandOfOurs(){
    QCOMPARE(SystemMenu::IndexOf(0), -1);
    QCOMPARE(SystemMenu::IndexOf(0xF060), -1);
    QCOMPARE(SystemMenu::IndexOf(0xF000), -1);
    QCOMPARE(SystemMenu::IndexOf(static_cast<quintptr>(SystemMenu::MarkId)), -1);
    QCOMPARE(SystemMenu::IndexOf(static_cast<quintptr>(SystemMenu::FirstId - SystemMenu::IdStep)), -1);
}

void tst_systemmenu::labelPutsTheKeyAfterATab(){
    QAction withKey(QStringLiteral("New &Tab"));
    withKey.setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
    QCOMPARE(SystemMenu::Label(&withKey),
             QStringLiteral("New &Tab\t") + QKeySequence(QStringLiteral("Ctrl+T")).toString(QKeySequence::NativeText));
    QAction withoutKey(QStringLiteral("Quit"));
    QCOMPARE(SystemMenu::Label(&withoutKey), QStringLiteral("Quit"));
}

void tst_systemmenu::entriesFollowTheMenuAndNumberTheCommandsInOrder(){
    QMenu menu;
    QAction *a1 = menu.addAction(QStringLiteral("One"));
    a1->setCheckable(true);
    a1->setChecked(true);
    menu.addSeparator();
    QMenu *sub = menu.addMenu(QStringLiteral("Sub"));
    QAction *b1 = sub->addAction(QStringLiteral("Sub one"));
    QAction *b2 = sub->addAction(QStringLiteral("Sub two"));
    b2->setEnabled(false);
    QAction *a2 = menu.addAction(QStringLiteral("Two"));

    QList<QAction*> actions;
    const QList<SystemMenu::Entry> entries = SystemMenu::Entries(&menu, actions);

    QCOMPARE(entries.size(), 4);
    QCOMPARE(entries[0].kind, SystemMenu::Entry::Command);
    QCOMPARE(entries[0].label, QStringLiteral("One"));
    QVERIFY(entries[0].checked);
    QVERIFY(entries[0].enabled);
    QCOMPARE(entries[0].id, 0);
    QCOMPARE(entries[1].kind, SystemMenu::Entry::Separator);
    QCOMPARE(entries[2].kind, SystemMenu::Entry::Submenu);
    QCOMPARE(entries[2].label, QStringLiteral("Sub"));
    QCOMPARE(entries[2].children.size(), 2);
    QCOMPARE(entries[2].children[0].label, QStringLiteral("Sub one"));
    QCOMPARE(entries[2].children[0].id, 1);
    QCOMPARE(entries[2].children[1].label, QStringLiteral("Sub two"));
    QCOMPARE(entries[2].children[1].id, 2);
    QVERIFY(!entries[2].children[1].enabled);
    QCOMPARE(entries[3].kind, SystemMenu::Entry::Command);
    QCOMPARE(entries[3].id, 3);
    QVERIFY(!entries[3].checked);
    QCOMPARE(actions, (QList<QAction*>() << a1 << b1 << b2 << a2));
}

void tst_systemmenu::hiddenActionsAreLeftOut(){
    QMenu menu;
    QAction *shown = menu.addAction(QStringLiteral("Shown"));
    QAction *hidden = menu.addAction(QStringLiteral("Hidden"));
    hidden->setVisible(false);
    QMenu *sub = menu.addMenu(QStringLiteral("Sub"));
    sub->menuAction()->setVisible(false);
    sub->addAction(QStringLiteral("In the hidden sub"));
    QAction *after = menu.addAction(QStringLiteral("After"));

    QList<QAction*> actions;
    const QList<SystemMenu::Entry> entries = SystemMenu::Entries(&menu, actions);
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries[0].label, QStringLiteral("Shown"));
    QCOMPARE(entries[1].label, QStringLiteral("After"));
    QCOMPARE(entries[1].id, 1);
    QCOMPARE(actions, (QList<QAction*>() << shown << after));
}

#if defined(Q_OS_WIN)
namespace {
    QList<int> Ids(HMENU menu){
        QList<int> ids;
        for(int pos = 0; pos < GetMenuItemCount(menu); pos++){
            MENUITEMINFOW info;
            ZeroMemory(&info, sizeof(info));
            info.cbSize = sizeof(info);
            info.fMask = MIIM_ID;
            GetMenuItemInfoW(menu, static_cast<UINT>(pos), TRUE, &info);
            ids << static_cast<int>(info.wID);
        }
        return ids;
    }
}

void tst_systemmenu::installKeepsTheSystemsAndOthersItemsAndPutsOursInOnce(){
    const HWND hwnd = CreateWindowExW(0, L"STATIC", L"tst_systemmenu", WS_OVERLAPPEDWINDOW,
                                      0, 0, 100, 100, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    QVERIFY(hwnd);
    HMENU sys = GetSystemMenu(hwnd, FALSE);
    QVERIFY(sys);
    const QList<int> standard = Ids(sys);
    QVERIFY(standard.contains(SC_CLOSE));
    const int foreign = 0x8001;
    QVERIFY(!SystemMenu::IsOurs(foreign));
    QVERIFY(InsertMenuW(sys, SC_CLOSE, MF_BYCOMMAND | MF_STRING, foreign, L"foreign"));

    QMenu menu;
    menu.addAction(QStringLiteral("One"));
    menu.addSeparator();
    QMenu *sub = menu.addMenu(QStringLiteral("Sub"));
    sub->addAction(QStringLiteral("Sub one"));
    QList<QAction*> actions;
    const QList<SystemMenu::Entry> entries = SystemMenu::Entries(&menu, actions);

    SystemMenu::Install(reinterpret_cast<WId>(hwnd), entries);
    const QList<int> once = Ids(sys);
    QCOMPARE(once.size(), standard.size() + 1 + entries.size() + 1);
    QVERIFY(once.contains(foreign));
    for(int id : standard) QVERIFY(once.contains(id));
    QCOMPARE(once.count(SystemMenu::IdOf(0)), 1);
    QCOMPARE(once.count(SystemMenu::MarkId), 3);
    QCOMPARE(once.last(), SC_CLOSE);
    QCOMPARE(once[once.size() - 2], SystemMenu::MarkId);

    SystemMenu::Install(reinterpret_cast<WId>(hwnd), entries);
    QCOMPARE(Ids(sys), once);

    SystemMenu::Install(reinterpret_cast<WId>(hwnd), QList<SystemMenu::Entry>());
    QList<int> expected = standard;
    expected.insert(expected.indexOf(SC_CLOSE), foreign);
    QCOMPARE(Ids(sys), expected);
    DestroyWindow(hwnd);
}
#endif

QTEST_MAIN(tst_systemmenu)
#include "tst_systemmenu.moc"
