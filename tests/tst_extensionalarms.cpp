#include "switch.hpp"

#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "extensionalarms.hpp"

using ExtensionAlarms::Book;

class tst_extensionalarms : public QObject {
    Q_OBJECT

    static QString Id(){ return QStringLiteral("idid"); }
    static bool Anyone(const QString &){ return true; }
    static QJsonObject Info(const char *key, double value){ return QJsonObject{ { QLatin1String(key), value } }; }
    static QStringList Names(const QJsonArray &alarms){
        QStringList out;
        for(const QJsonValue &one : alarms) out << one.toObject().value(QStringLiteral("name")).toString();
        return out;
    }
    static QString Made(Book *book, const QJsonArray &args, double now = 0){
        QString error;
        book->Create(Id(), args, now, &error);
        return error;
    }

private slots:
    void aCallIsReadAsChromeReadsIt();
    void aNameIsMadeAgainInItsPlace();
    void theBookHasItsLimit();
    void aSingleAlarmRingsOnceAndLeaves();
    void aPeriodMovesOnPastWhatItMissed();
    void aShortPeriodKeepsItsValueAndStepsBySeconds();
    void whatMayNotRingIsNotUsedUp();
    void whatWaitsGoesToWhoeverTakesItFirst();
    void whatWaitsGoesWhenItIsClearedOrMadeAgain();
    void everyAlarmWhichComesDueAtOnceWaits();
    void whatNobodyTakesGoesAfterAnHour();
    void theBookIsKeptAndReadBack();
    void whatCannotBeReadIsLeftOut();
    void theNextAlarmIsOfThoseWhichMayRing();
};

void tst_extensionalarms::aCallIsReadAsChromeReadsIt(){
    Book book;
    QCOMPARE(Made(&book, QJsonArray{ QStringLiteral("a"), QJsonObject{ { QStringLiteral("when"), 1.0 }, { QStringLiteral("delayInMinutes"), 1.0 } } }),
             QStringLiteral("Cannot set both when and delayInMinutes."));
    QCOMPARE(Made(&book, QJsonArray{ QStringLiteral("a"), QJsonObject() }),
             QStringLiteral("Must set at least one of when, delayInMinutes, or periodInMinutes."));
    QVERIFY(Made(&book, QJsonArray{ QStringLiteral("a"), QJsonObject{ { QStringLiteral("when"), QStringLiteral("soon") } } })
            .startsWith(QStringLiteral("Invalid value for argument 2.")));
    QVERIFY(Made(&book, QJsonArray{ 7, Info("when", 1) }).startsWith(QStringLiteral("Invalid value for argument 1.")));
    QVERIFY(Made(&book, QJsonArray{ QStringLiteral("a") }).startsWith(QStringLiteral("Invalid value for argument 2.")));
    QVERIFY(book.GetAll(Id()).isEmpty());

    QCOMPARE(Made(&book, QJsonArray{ Info("periodInMinutes", 2) }, 1000), QString());
    QCOMPARE(Made(&book, QJsonArray{ QStringLiteral("b"), Info("delayInMinutes", 1) }, 1000), QString());
    QCOMPARE(Made(&book, QJsonArray{ QStringLiteral("c"), Info("when", 5) }, 1000), QString());
    QCOMPARE(Names(book.GetAll(Id())), QStringList() << QString() << QStringLiteral("b") << QStringLiteral("c"));
    QString error;
    const QJsonObject unnamed = book.Get(Id(), QJsonArray(), &error).toObject();
    QCOMPARE(unnamed.value(QStringLiteral("scheduledTime")).toDouble(), 1000.0 + 2 * 60000);
    QCOMPARE(unnamed.value(QStringLiteral("periodInMinutes")).toDouble(), 2.0);
    const QJsonObject b = book.Get(Id(), QJsonArray{ QStringLiteral("b") }, &error).toObject();
    QCOMPARE(b.value(QStringLiteral("scheduledTime")).toDouble(), 1000.0 + 60000);
    QVERIFY(!b.contains(QStringLiteral("periodInMinutes")));
    QCOMPARE(book.Get(Id(), QJsonArray{ QStringLiteral("c") }, &error).toObject().value(QStringLiteral("scheduledTime")).toDouble(), 5.0);
    QVERIFY(book.Get(Id(), QJsonArray{ QStringLiteral("none") }, &error).isUndefined());
    QVERIFY(error.isEmpty());
    book.Get(Id(), QJsonArray{ 7 }, &error);
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(book.Clear(Id(), QJsonArray{ QStringLiteral("b") }, &error));
    QVERIFY(!book.Clear(Id(), QJsonArray{ QStringLiteral("b") }, &error));
    QVERIFY(book.Clear(Id(), QJsonArray(), &error));
    QVERIFY(book.ClearAll(Id()));
    QVERIFY(book.GetAll(Id()).isEmpty());
    QVERIFY(!book.HasAlarms(Id()));
}

void tst_extensionalarms::aNameIsMadeAgainInItsPlace(){
    Book book;
    Made(&book, QJsonArray{ QStringLiteral("a"), Info("when", 10) });
    Made(&book, QJsonArray{ QStringLiteral("b"), Info("when", 20) });
    Made(&book, QJsonArray{ QStringLiteral("a"), Info("when", 30) });
    QCOMPARE(Names(book.GetAll(Id())), QStringList() << QStringLiteral("b") << QStringLiteral("a"));
    QVERIFY(!book.Advance(15, Anyone));
    QVERIFY(book.Take(Id()).isEmpty());
}

void tst_extensionalarms::theBookHasItsLimit(){
    Book book;
    for(int i = 0; i < ExtensionAlarms::MOST; i++) QCOMPARE(Made(&book, QJsonArray{ QString::number(i), Info("when", 10) }), QString());
    QCOMPARE(Made(&book, QJsonArray{ QStringLiteral("one more"), Info("when", 10) }), QStringLiteral("An extension cannot have more than 500 active alarms."));
    QCOMPARE(Made(&book, QJsonArray{ QStringLiteral("0"), Info("when", 10) }), QStringLiteral("An extension cannot have more than 500 active alarms."));
}

void tst_extensionalarms::aSingleAlarmRingsOnceAndLeaves(){
    Book book;
    Made(&book, QJsonArray{ QStringLiteral("a"), Info("when", 100) });
    QVERIFY(!book.Advance(99, Anyone));
    QVERIFY(book.WithDue().isEmpty());
    QVERIFY(book.Advance(100, Anyone));
    QVERIFY(book.GetAll(Id()).isEmpty());
    QCOMPARE(book.WithDue(), QStringList() << Id());
    QVERIFY(book.HasAlarms(Id()));
    const QJsonArray due = book.Take(Id());
    QCOMPARE(Names(due), QStringList() << QStringLiteral("a"));
    QCOMPARE(due.at(0).toObject().value(QStringLiteral("scheduledTime")).toDouble(), 100.0);
    QVERIFY(!book.Advance(200, Anyone));
    QVERIFY(book.Take(Id()).isEmpty());
    QVERIFY(!book.HasAlarms(Id()));
}

void tst_extensionalarms::aPeriodMovesOnPastWhatItMissed(){
    Book book;
    Made(&book, QJsonArray{ QStringLiteral("a"), Info("periodInMinutes", 1) }, 0);
    QVERIFY(book.Advance(60000, Anyone));
    QJsonArray due = book.Take(Id());
    QCOMPARE(due.at(0).toObject().value(QStringLiteral("scheduledTime")).toDouble(), 60000.0);
    QCOMPARE(due.at(0).toObject().value(QStringLiteral("periodInMinutes")).toDouble(), 1.0);
    QString error;
    QCOMPARE(book.Get(Id(), QJsonArray{ QStringLiteral("a") }, &error).toObject().value(QStringLiteral("scheduledTime")).toDouble(), 120000.0);
    QVERIFY(book.Advance(305000, Anyone));
    QCOMPARE(book.Take(Id()).size(), 1);
    QCOMPARE(book.Get(Id(), QJsonArray{ QStringLiteral("a") }, &error).toObject().value(QStringLiteral("scheduledTime")).toDouble(), 360000.0);
}

void tst_extensionalarms::aShortPeriodKeepsItsValueAndStepsBySeconds(){
    Book book;
    Made(&book, QJsonArray{ QStringLiteral("a"), QJsonObject{ { QStringLiteral("when"), 100.0 }, { QStringLiteral("periodInMinutes"), 0.0 } } });
    QVERIFY(book.Advance(100, Anyone));
    QString error;
    const QJsonObject a = book.Get(Id(), QJsonArray{ QStringLiteral("a") }, &error).toObject();
    QCOMPARE(a.value(QStringLiteral("periodInMinutes")).toDouble(), 0.0);
    QCOMPARE(a.value(QStringLiteral("scheduledTime")).toDouble(), 1100.0);
}

void tst_extensionalarms::whatMayNotRingIsNotUsedUp(){
    Book book;
    Made(&book, QJsonArray{ QStringLiteral("a"), Info("when", 100) });
    QVERIFY(!book.Advance(500, [](const QString &){ return false; }));
    QCOMPARE(Names(book.GetAll(Id())), QStringList() << QStringLiteral("a"));
    QVERIFY(book.WithDue().isEmpty());
    QVERIFY(book.Advance(600, Anyone));
    QCOMPARE(Names(book.Take(Id())), QStringList() << QStringLiteral("a"));
}

void tst_extensionalarms::whatWaitsGoesToWhoeverTakesItFirst(){
    Book book;
    Made(&book, QJsonArray{ QStringLiteral("a"), Info("when", 100) });
    book.Advance(100, Anyone);
    QCOMPARE(book.Take(Id()).size(), 1);
    QCOMPARE(book.Take(Id()).size(), 0);
}

void tst_extensionalarms::whatWaitsGoesWhenItIsClearedOrMadeAgain(){
    QString error;
    Book cleared;
    Made(&cleared, QJsonArray{ QStringLiteral("a"), Info("when", 100) });
    cleared.Advance(100, Anyone);
    QVERIFY(!cleared.Clear(Id(), QJsonArray{ QStringLiteral("a") }, &error));
    QVERIFY(cleared.Take(Id()).isEmpty());

    Book madeAgain;
    Made(&madeAgain, QJsonArray{ QStringLiteral("a"), Info("when", 100) });
    madeAgain.Advance(100, Anyone);
    Made(&madeAgain, QJsonArray{ QStringLiteral("a"), Info("when", 900) });
    QVERIFY(madeAgain.Take(Id()).isEmpty());

    Book all;
    Made(&all, QJsonArray{ QStringLiteral("a"), Info("when", 100) });
    all.Advance(100, Anyone);
    all.ClearAll(Id());
    QVERIFY(all.Take(Id()).isEmpty());

    Book twice;
    Made(&twice, QJsonArray{ QStringLiteral("a"), Info("periodInMinutes", 1) }, 0);
    twice.Advance(60000, Anyone);
    twice.Advance(120000, Anyone);
    const QJsonArray due = twice.Take(Id());
    QCOMPARE(due.size(), 1);
    QCOMPARE(due.at(0).toObject().value(QStringLiteral("scheduledTime")).toDouble(), 120000.0);
}

void tst_extensionalarms::everyAlarmWhichComesDueAtOnceWaits(){
    Book book;
    for(int i = 0; i < ExtensionAlarms::MOST; i++) Made(&book, QJsonArray{ QString::number(i), Info("when", 10) });
    QVERIFY(book.Advance(10, Anyone));
    QCOMPARE(book.Take(Id()).size(), ExtensionAlarms::MOST);
}

void tst_extensionalarms::whatNobodyTakesGoesAfterAnHour(){
    Book book;
    Made(&book, QJsonArray{ QStringLiteral("a"), Info("when", 100) });
    book.Advance(100, Anyone);
    QVERIFY(!book.Advance(100 + ExtensionAlarms::DUE_KEPT_MS, Anyone));
    QCOMPARE(book.WithDue(), QStringList() << Id());
    QVERIFY(book.Advance(101 + ExtensionAlarms::DUE_KEPT_MS, Anyone));
    QVERIFY(book.WithDue().isEmpty());
}

void tst_extensionalarms::theBookIsKeptAndReadBack(){
    Book book;
    Made(&book, QJsonArray{ QStringLiteral("single"), Info("when", 10) });
    Made(&book, QJsonArray{ QStringLiteral("period"), Info("periodInMinutes", 1) }, 1000);
    book.Advance(10, Anyone);
    Made(&book, QJsonArray{ QStringLiteral("later"), Info("when", 5000) });
    Book read = Book::FromJson(book.ToJson());
    QCOMPARE(read.GetAll(Id()), book.GetAll(Id()));
    QCOMPARE(read.WithDue(), QStringList() << Id());
    QVERIFY(!read.Advance(10 + ExtensionAlarms::DUE_KEPT_MS, [](const QString &){ return false; }));
    QCOMPARE(Names(read.Take(Id())), QStringList() << QStringLiteral("single"));
    QCOMPARE(read.ToJson(), Book::FromJson(read.ToJson()).ToJson());
}

void tst_extensionalarms::whatCannotBeReadIsLeftOut(){
    QVERIFY(Book::FromJson("not json").WithDue().isEmpty());
    const Book book = Book::FromJson(R"({"idid":{"alarms":[{"name":7,"scheduledTime":1},{"name":"good","scheduledTime":2},{"name":"bad"}]},"other":3})");
    QCOMPARE(Names(book.GetAll(Id())), QStringList() << QStringLiteral("good"));
    QVERIFY(book.GetAll(QStringLiteral("other")).isEmpty());
}

void tst_extensionalarms::theNextAlarmIsOfThoseWhichMayRing(){
    Book book;
    QCOMPARE(book.NextAt(Anyone), -1.0);
    Made(&book, QJsonArray{ QStringLiteral("a"), Info("when", 500) });
    Made(&book, QJsonArray{ QStringLiteral("b"), Info("when", 100) });
    QCOMPARE(book.NextAt(Anyone), 100.0);
    QCOMPARE(book.NextAt([](const QString &){ return false; }), -1.0);
}

QTEST_MAIN(tst_extensionalarms)
#include "tst_extensionalarms.moc"
