#include <QtTest>

#include <QMetaEnum>
#include <QSet>

#include <type_traits>

#include "dialog.hpp"

class tst_dialogbuttons : public QObject {
    Q_OBJECT

private slots:
    void everyButtonInTheEnumerationHasAWordOnIt();
    void noTwoButtonsShareABitOrAWord();
    void theTableSaysEachButtonOnceAndSaysWhatAClickMeans();
    void returnAnswersOnlyWhereAnAnswerIsSafeToGiveWithAKey();
    void escapeWalksAwayRatherThanAnswering();
    void quittingRefusesEveryRunningDialogAndWaitsForAllOfThem();
    void aDialogStopsCountingOnlyWhenItIsDeleted();
    void aDialogThatCameAndWentLeavesAMarkBehindIt();
    void nothingIsNotAButton();

private:
    static QMetaEnum Enumeration(){
        return QMetaEnum::fromType<Dialog::Button>();
    }

    static QList<Dialog::Button> Buttons(){
        const QMetaEnum meta = Enumeration();
        QList<Dialog::Button> buttons;
        for(int i = 0; i < meta.keyCount(); i++){
            const Dialog::Button button = static_cast<Dialog::Button>(meta.value(i));
            if(button != Dialog::NoButton) buttons << button;
        }
        return buttons;
    }

    static QString Name(Dialog::Button button){
        return QStringLiteral("Dialog::") +
            QString::fromLatin1(Enumeration().valueToKey(button));
    }
};

void tst_dialogbuttons::everyButtonInTheEnumerationHasAWordOnIt(){
    QVERIFY2(Enumeration().isValid(),
             "'Dialog::Button' is not registered; is its 'Q_ENUM' still there?");

    QVERIFY2(Enumeration().keyCount() >= 9,
             qPrintable(QStringLiteral("only %1 enumerators were found")
                        .arg(Enumeration().keyCount())));

    foreach(const Dialog::Button button, Buttons()){
        QVERIFY2(!Dialog::Text(button).isEmpty(),
                 qPrintable(QStringLiteral("'%1' has no row in 'ButtonSpecs', so no "
                                           "dialog builds it: a caller asking for it "
                                           "gets a dialog without that button and "
                                           "never hears the answer")
                            .arg(Name(button))));
    }
}

void tst_dialogbuttons::noTwoButtonsShareABitOrAWord(){
    int seen = 0;
    QSet<QString> words;

    foreach(const Dialog::Button button, Buttons()){
        const int value = static_cast<int>(button);

        QVERIFY2(value != 0 && (value & (value - 1)) == 0,
                 qPrintable(QStringLiteral("'%1' is not a single bit")
                            .arg(Name(button))));
        QVERIFY2((seen & value) == 0,
                 qPrintable(QStringLiteral("'%1' shares a bit with a button before it")
                            .arg(Name(button))));
        seen |= value;

        const QString word = Dialog::Text(button);
        QVERIFY2(!words.contains(word),
                 qPrintable(QStringLiteral("'%1' is written '%2', which another "
                                           "button is written too")
                            .arg(Name(button), word)));
        words << word;
    }
}

void tst_dialogbuttons::theTableSaysEachButtonOnceAndSaysWhatAClickMeans(){
    struct Expected { Dialog::Button button; bool returns; };
    const QList<Expected> expected = {
        { Dialog::Ok,     true  },
        { Dialog::Allow,  true  },
        { Dialog::Block,  true  },
        { Dialog::Yes,    true  },
        { Dialog::No,     false },
        { Dialog::Cancel, false },
        { Dialog::Open,   true  },
        { Dialog::Close,  false }
    };

    const QList<Dialog::Row> rows = Dialog::Table();

    QCOMPARE(rows.size(), Buttons().size());
    QCOMPARE(rows.size(), expected.size());

    QSet<int> seen;
    for(int i = 0; i < rows.size(); i++){
        const Dialog::Row &row = rows.at(i);
        const QString name = Name(row.button);

        QVERIFY2(!seen.contains(static_cast<int>(row.button)),
                 qPrintable(QStringLiteral("'%1' has more than one row in "
                                           "'ButtonSpecs'").arg(name)));
        seen << static_cast<int>(row.button);

        QVERIFY2(row.button == expected.at(i).button,
                 qPrintable(QStringLiteral("row %1 is '%2', and it is meant to "
                                           "be '%3'")
                            .arg(QString::number(i), name,
                                 Name(expected.at(i).button))));
        QVERIFY2(Dialog::Returns(row.button) == row.returns,
                 qPrintable(QStringLiteral("'Dialog::Returns' and the table "
                                           "disagree about '%1'").arg(name)));
        QVERIFY2(row.returns == expected.at(i).returns,
                 qPrintable(QStringLiteral("clicking '%1' is meant to %2 the "
                                           "dialog")
                            .arg(name, expected.at(i).returns
                                 ? QStringLiteral("answer ('Returned')")
                                 : QStringLiteral("refuse ('Aborted')"))));
        QVERIFY2(!row.text.isEmpty(),
                 qPrintable(QStringLiteral("'%1' has no word on it").arg(name)));
    }
}

void tst_dialogbuttons::returnAnswersOnlyWhereAnAnswerIsSafeToGiveWithAKey(){
    const Dialog::Buttons okCancel     = Dialog::Ok | Dialog::Cancel;
    const Dialog::Buttons yesNo        = Dialog::Yes | Dialog::No;
    const Dialog::Buttons yesNoCancel  = Dialog::Yes | Dialog::No | Dialog::Cancel;
    const Dialog::Buttons allowBlock   = Dialog::Allow | Dialog::Block;
    const Dialog::Buttons allowBlockC  = allowBlock | Dialog::Cancel;
    const Dialog::Buttons openClose    = Dialog::Open | Dialog::Close;

    QCOMPARE(Dialog::Default(okCancel, Dialog::NoButton), Dialog::Ok);
    QCOMPARE(Dialog::Default(Dialog::Ok, Dialog::NoButton), Dialog::Ok);

    QCOMPARE(Dialog::Default(yesNoCancel, Dialog::NoButton), Dialog::NoButton);
    QCOMPARE(Dialog::Default(allowBlock, Dialog::NoButton), Dialog::NoButton);
    QCOMPARE(Dialog::Default(allowBlockC, Dialog::NoButton), Dialog::NoButton);
    QCOMPARE(Dialog::Default(openClose, Dialog::NoButton), Dialog::NoButton);

    QCOMPARE(Dialog::Default(yesNo, Dialog::Yes), Dialog::Yes);
    QCOMPARE(Dialog::Default(yesNoCancel, Dialog::Yes), Dialog::Yes);

    QCOMPARE(Dialog::Default(yesNo, Dialog::Allow), Dialog::NoButton);
    QCOMPARE(Dialog::Default(allowBlock, Dialog::Ok), Dialog::NoButton);
    QCOMPARE(Dialog::Default(Dialog::NoButton, Dialog::Ok), Dialog::NoButton);
}

void tst_dialogbuttons::escapeWalksAwayRatherThanAnswering(){
    QCOMPARE(Dialog::Refusal(Dialog::Yes | Dialog::No | Dialog::Cancel), Dialog::Cancel);
    QCOMPARE(Dialog::Refusal(Dialog::Allow | Dialog::Block | Dialog::Cancel), Dialog::Cancel);
    QCOMPARE(Dialog::Refusal(Dialog::Ok | Dialog::Cancel), Dialog::Cancel);
    QCOMPARE(Dialog::Refusal(Dialog::Open | Dialog::Close), Dialog::Close);
    QCOMPARE(Dialog::Refusal(Dialog::Yes | Dialog::No), Dialog::No);

    QCOMPARE(Dialog::Refusal(Dialog::Allow | Dialog::Block), Dialog::NoButton);
    QCOMPARE(Dialog::Refusal(Dialog::Ok), Dialog::NoButton);
    QCOMPARE(Dialog::Refusal(Dialog::NoButton), Dialog::NoButton);

    foreach(const Dialog::Button button, Buttons()){
        const Dialog::Button refusal = Dialog::Refusal(button);
        if(refusal == Dialog::NoButton) continue;
        QVERIFY2(!Dialog::Returns(refusal),
                 qPrintable(QStringLiteral("Escape on a dialog offering '%1' lands "
                                           "on '%2', which answers the dialog "
                                           "instead of refusing it")
                            .arg(Name(button), Name(refusal))));
    }
}

void tst_dialogbuttons::quittingRefusesEveryRunningDialogAndWaitsForAllOfThem(){
    QVERIFY2(!ModalDialog::AnyRunning(),
             "a dialog was left in the running list by an earlier test");

    ModalDialog outer;
    ModalDialog inner;
    int outerAborted = 0, innerAborted = 0;
    int outerReturned = 0, innerReturned = 0;
    QObject::connect(&outer, &ModalDialog::Aborted,  [&](){ outerAborted++;});
    QObject::connect(&inner, &ModalDialog::Aborted,  [&](){ innerAborted++;});
    QObject::connect(&outer, &ModalDialog::Returned, [&](){ outerReturned++;});
    QObject::connect(&inner, &ModalDialog::Returned, [&](){ innerReturned++;});

    ModalDialog::AbortAll();
    QCOMPARE(outerAborted, 0);
    QCOMPARE(innerAborted, 0);

    {
        ModalDialog::Running outerRunning(&outer);
        QVERIFY(ModalDialog::AnyRunning());
        {
            ModalDialog::Running innerRunning(&inner);
            QVERIFY(ModalDialog::AnyRunning());

            ModalDialog::AbortAll();
            QCOMPARE(outerAborted, 1);
            QCOMPARE(innerAborted, 1);

            QCOMPARE(outerReturned, 0);
            QCOMPARE(innerReturned, 0);

            QVERIFY(ModalDialog::AnyRunning());
        }
        QVERIFY(ModalDialog::AnyRunning());
    }

    QVERIFY(ModalDialog::AnyRunning());
}

void tst_dialogbuttons::aDialogStopsCountingOnlyWhenItIsDeleted(){
    static_assert(!std::is_copy_constructible<ModalDialog::Running>::value,
                  "ModalDialog::Running must not be copyable");
    static_assert(!std::is_copy_assignable<ModalDialog::Running>::value,
                  "ModalDialog::Running must not be copy assignable");
    static_assert(!std::is_move_constructible<ModalDialog::Running>::value,
                  "ModalDialog::Running must not be movable");

    QVERIFY(!ModalDialog::AnyRunning());
    {
        ModalDialog dialog;
        { ModalDialog::Running running(&dialog); }
        QVERIFY2(ModalDialog::AnyRunning(),
                 "the loop ended, but the dialog is still on the stack");
    }
    QVERIFY2(!ModalDialog::AnyRunning(),
             "a deleted dialog was left counting, and quitting would wait for ever");
}

void tst_dialogbuttons::aDialogThatCameAndWentLeavesAMarkBehindIt(){
    QVERIFY(!ModalDialog::AnyRunning());
    const quint64 before = ModalDialog::Generation();

    ModalDialog *dialog = new ModalDialog();
    QCOMPARE(ModalDialog::Generation(), before);

    quint64 mark = ModalDialog::Generation();
    {
        ModalDialog::Running running(dialog);
        QVERIFY2(ModalDialog::Generation() > mark, "starting to run left no mark");
        mark = ModalDialog::Generation();
    }
    QVERIFY2(ModalDialog::Generation() > mark, "the loop ending left no mark");

    mark = ModalDialog::Generation();
    delete dialog;
    QVERIFY2(ModalDialog::Generation() > mark, "being deleted left no mark");

    QVERIFY(!ModalDialog::AnyRunning());
    QVERIFY2(ModalDialog::Generation() > before,
             "a dialog came and went and left no mark, so a wait for quiet "
             "would have counted it as quiet");

    const quint64 settled = ModalDialog::Generation();
    QCOMPARE(ModalDialog::Generation(), settled);
}

void tst_dialogbuttons::nothingIsNotAButton(){
    QVERIFY(Dialog::Text(Dialog::NoButton).isEmpty());
}

QTEST_MAIN(tst_dialogbuttons)
#include "tst_dialogbuttons.moc"
