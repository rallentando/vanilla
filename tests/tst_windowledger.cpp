#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QList>

#include "windowledger.hpp"

#include "testsupport.hpp"

struct Window {
    Window(int id) : id(id), focused(0) {}
    int id;
    int focused;
};

typedef WindowLedger<Window> Ledger;

class tst_windowledger : public QObject {
    Q_OBJECT

private slots:

    void anemptyLedgerAnswersEverything(){
        Ledger ledger;
        QCOMPARE(ledger.Count(), 0);
        QVERIFY(ledger.IsEmpty());
        QVERIFY(!ledger.Current());
        QCOMPARE(ledger.CurrentId(), 0);
        QVERIFY(!ledger.At(1));
        QVERIFY(!ledger.IdOf(nullptr));
        QVERIFY(!ledger.Switch(true));
        QVERIFY(!ledger.Switch(false));
    }

    void insertAndLookUp(){
        Ledger ledger;
        Window a(7), b(3);

        ledger.Insert(7, &a);
        ledger.Insert(3, &b);

        QCOMPARE(ledger.Count(), 2);
        QCOMPARE(ledger.At(7), &a);
        QCOMPARE(ledger.At(3), &b);
        QCOMPARE(ledger.IdOf(&a), 7);
        QCOMPARE(ledger.IdOf(&b), 3);
        QCOMPARE(ledger.Ids(), QList<int>() << 3 << 7);

        QVERIFY(!ledger.Current());
    }

    void alookupNeverInsertsAWindowThatIsNotThere(){
        Ledger ledger;
        Window a(1);
        ledger.Insert(1, &a);

        QVERIFY(!ledger.At(999));
        QCOMPARE(ledger.Count(), 1);

        ledger.SetCurrent(999);
        QVERIFY(!ledger.Current());
        QCOMPARE(ledger.Count(), 1);

        ledger.Remove(999);
        QCOMPARE(ledger.Count(), 1);

        QCOMPARE(ledger.Ids(), QList<int>() << 1);
        QCOMPARE(ledger.At(1), &a);
    }

    void removingTheCurrentWindowMovesTheCursorToTheLowestIdLeft(){
        Ledger ledger;
        Window a(5), b(2), c(9);
        ledger.Insert(5, &a);
        ledger.Insert(2, &b);
        ledger.Insert(9, &c);
        ledger.SetCurrent(&a);

        ledger.Remove(&a);

        QCOMPARE(ledger.Count(), 2);
        QCOMPARE(ledger.Current(), &b);
        QCOMPARE(ledger.CurrentId(), 2);
        QVERIFY(!ledger.At(5));
    }

    void removingSomeOtherWindowLeavesTheCursorWhereItIs(){
        Ledger ledger;
        Window a(5), b(2);
        ledger.Insert(5, &a);
        ledger.Insert(2, &b);
        ledger.SetCurrent(&a);

        ledger.Remove(&b);

        QCOMPARE(ledger.Current(), &a);
        QCOMPARE(ledger.Count(), 1);
    }

    void removingTheLastWindowLeavesNoCurrent(){
        Ledger ledger;
        Window a(1);
        ledger.Insert(1, &a);
        ledger.SetCurrent(&a);

        ledger.Remove(&a);

        QVERIFY(ledger.IsEmpty());
        QVERIFY(!ledger.Current());
        QCOMPARE(ledger.CurrentId(), 0);
    }

    void awindowTheLedgerDoesNotHoldRemovesNothing(){
        Ledger ledger;
        Window a(1), stranger(1);
        ledger.Insert(1, &a);

        ledger.Remove(&stranger);

        QCOMPARE(ledger.Count(), 1);
        QCOMPARE(ledger.At(1), &a);
    }

    void switchWalksInIdOrderAndWrapsAtBothEnds(){
        Ledger ledger;
        Window a(2), b(5), c(9);
        ledger.Insert(5, &b);
        ledger.Insert(9, &c);
        ledger.Insert(2, &a);
        ledger.SetCurrent(&a);

        QCOMPARE(ledger.Switch(true),  &b);
        QCOMPARE(ledger.Switch(true),  &c);
        QCOMPARE(ledger.Switch(true),  &a);
        QCOMPARE(ledger.Switch(false), &c);
        QCOMPARE(ledger.Switch(false), &b);
        QCOMPARE(ledger.Switch(false), &a);
    }

    void switchWithoutACurrentWindowStartsAtTheFarEnd(){
        Ledger ledger;
        Window a(2), b(5);
        ledger.Insert(2, &a);
        ledger.Insert(5, &b);

        QCOMPARE(ledger.Switch(true), &a);

        Ledger other;
        other.Insert(2, &a);
        other.Insert(5, &b);
        QCOMPARE(other.Switch(false), &b);
    }

    void switchWithOneWindowStaysOnIt(){
        Ledger ledger;
        Window a(3);
        ledger.Insert(3, &a);
        ledger.SetCurrent(&a);

        QCOMPARE(ledger.Switch(true),  &a);
        QCOMPARE(ledger.Switch(false), &a);
    }

    void thefocusFollowsTheCursorWhenTheCursorMovesOnItsOwn(){
        Ledger ledger;
        ledger.Focus = [](Window *win){ win->focused++;};

        Window a(1), b(2);
        ledger.Insert(1, &a);
        ledger.Insert(2, &b);

        ledger.SetCurrent(&a);
        QCOMPARE(a.focused, 0);

        ledger.Switch(true);
        QCOMPARE(b.focused, 1);

        ledger.Remove(&b);
        QCOMPARE(a.focused, 1);
    }

    void thefocusIsLeftAloneWhileSomethingIsBeingDragged(){
        Ledger ledger;
        bool dragging = true;
        ledger.Focus  = [](Window *win){ win->focused++;};
        ledger.IsBusy = [&dragging](){ return dragging;};

        Window a(1), b(2);
        ledger.Insert(1, &a);
        ledger.Insert(2, &b);
        ledger.SetCurrent(&b);

        ledger.Remove(&b);
        QCOMPARE(ledger.Current(), &a);
        QCOMPARE(a.focused, 0);

        ledger.Switch(true);
        QCOMPARE(a.focused, 1);
    }

    void unusedIdSkipsTheOnesAlreadyTaken(){
        Ledger ledger;
        Window a(1), b(2);
        ledger.Insert(1, &a);
        ledger.Insert(2, &b);

        QList<int> offered = QList<int>() << 1 << 2 << 1 << 8;
        int n = 0;
        ledger.NewId = [&offered, &n](){ return offered.value(n++, 99);};

        QCOMPARE(ledger.UnusedId(), 8);
        QCOMPARE(n, 4);
        QCOMPARE(ledger.Count(), 2);
    }

    void abrokenIdSourceStillTerminates(){
        Ledger ledger;
        Window a(1);
        ledger.Insert(1, &a);
        ledger.NewId = [](){ return 1;};

        QCOMPARE(ledger.UnusedId(), 2);
    }

    void zeroIsNeverHandedOutAsAnId(){
        Ledger empty;
        empty.NewId = [](){ return 0;};
        QCOMPARE(empty.UnusedId(), 1);

        Ledger ledger;
        Window a(1);
        ledger.Insert(4, &a);
        QList<int> offered = QList<int>() << 0 << 0 << 6;
        int n = 0;
        ledger.NewId = [&offered, &n](){ return offered.value(n++, 99);};
        QCOMPARE(ledger.UnusedId(), 6);
    }

    void theledgerKeysOnTheIdItWasGivenNotTheOneTheWindowCarries(){
        Ledger ledger;
        Window a(1);

        ledger.Insert(42, &a);

        QCOMPARE(ledger.IdOf(&a), 42);
        QCOMPARE(ledger.At(42), &a);
        QVERIFY(!ledger.At(1));
    }
};

QTEST_MAIN(tst_windowledger)
#include "tst_windowledger.moc"
