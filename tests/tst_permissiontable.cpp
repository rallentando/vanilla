#include "switch.hpp"

#include <QtTest>

#include "permissiontable.hpp"

using State = PermissionTable::State;
using Row = PermissionTable::Row;

namespace {
    QString Describe(const QList<Row> &rows){
        QStringList lines;
        foreach(const Row &row, rows)
            lines << row.origin + QStringLiteral(" ") + row.kind + QStringLiteral(" ") +
                     PermissionTable::StateName(row.state);
        lines.sort();
        return lines.join(QStringLiteral("\n"));
    }
}

class tst_permissiontable : public QObject {
    Q_OBJECT

private slots:

    void theOriginIsMadeFromWhatTheUserTyped_data(){
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("origin");
        QTest::newRow("a bare host is https") << QStringLiteral("Example.com") << QStringLiteral("https://example.com");
        QTest::newRow("spaces around") << QStringLiteral("  example.com  ") << QStringLiteral("https://example.com");
        QTest::newRow("a host and a port") << QStringLiteral("localhost:8080") << QStringLiteral("https://localhost:8080");
        QTest::newRow("http as written") << QStringLiteral("http://example.com:80/a") << QStringLiteral("http://example.com");
        QTest::newRow("a path dropped") << QStringLiteral("example.com/a?b") << QStringLiteral("https://example.com");
        QTest::newRow("the default port dropped") << QStringLiteral("example.com:443") << QStringLiteral("https://example.com");
        QTest::newRow("an extension as written")
            << QStringLiteral("chrome-extension://ddkjiahejlhfcafbddmgiahcphecmpfh/")
            << QStringLiteral("chrome-extension://ddkjiahejlhfcafbddmgiahcphecmpfh");
        QTest::newRow("another scheme") << QStringLiteral("ftp://example.com") << QString();
        QTest::newRow("file") << QStringLiteral("file:///c:/a") << QString();
        QTest::newRow("empty") << QString() << QString();
        QTest::newRow("words") << QStringLiteral("not a site") << QString();
        QTest::newRow("a scheme and nothing") << QStringLiteral("https://") << QString();
    }
    void theOriginIsMadeFromWhatTheUserTyped(){
        QFETCH(QString, text);
        QFETCH(QString, origin);
        QCOMPARE(PermissionTable::OriginFromUserText(text), origin);
    }

    void theOriginIsMadeFromTheAddress_data(){
        QTest::addColumn<QString>("url");
        QTest::addColumn<QString>("origin");

        QTest::newRow("https, the path dropped")
            << QStringLiteral("https://Example.COM/a?b#c") << QStringLiteral("https://example.com");
        QTest::newRow("the default port written out")
            << QStringLiteral("https://example.com:443/") << QStringLiteral("https://example.com");
        QTest::newRow("another port is kept")
            << QStringLiteral("https://example.com:8443") << QStringLiteral("https://example.com:8443");
        QTest::newRow("http, its own default port")
            << QStringLiteral("http://localhost:80/x") << QStringLiteral("http://localhost");
        QTest::newRow("http on another port")
            << QStringLiteral("http://127.0.0.1:8080") << QStringLiteral("http://127.0.0.1:8080");
        QTest::newRow("an IDN as its punycode")
            << QStringLiteral("https://bücher.example/") << QStringLiteral("https://xn--bcher-kva.example");
        QTest::newRow("IPv6 in brackets")
            << QStringLiteral("https://[::1]:8443/") << QStringLiteral("https://[::1]:8443");
        QTest::newRow("an extension's page, by its id")
            << QStringLiteral("chrome-extension://DDKJ/dashboard.html?a#b") << QStringLiteral("chrome-extension://ddkj");
        QTest::newRow("an extension has no port")
            << QStringLiteral("chrome-extension://ddkj:8080/") << QStringLiteral("chrome-extension://ddkj");
        QTest::newRow("an extension with no id has none") << QStringLiteral("chrome-extension:///a") << QString();
        QTest::newRow("a file has none") << QStringLiteral("file:///C:/a.html") << QString();
        QTest::newRow("data has none") << QStringLiteral("data:text/html,x") << QString();
        QTest::newRow("nothing has none") << QString() << QString();
    }
    void theOriginIsMadeFromTheAddress(){
        QFETCH(QString, url);
        QFETCH(QString, origin);
        QCOMPARE(PermissionTable::Origin(QUrl(url)), origin);
    }

    void twoSpellingsOfOneOriginAreOneRow(){
        PermissionTable table;
        QVERIFY(table.Set(QStringLiteral("https://example.com:443/page"),
                          QStringLiteral("camera"), PermissionTable::Allow));
        QCOMPARE(table.Lookup(QStringLiteral("https://EXAMPLE.com"), QStringLiteral("camera")),
                 PermissionTable::Allow);
        QVERIFY(!table.Set(QStringLiteral("https://example.com"),
                           QStringLiteral("camera"), PermissionTable::Allow));
        QCOMPARE(table.Rows().length(), 1);
        QVERIFY(table.Set(QStringLiteral("https://example.com"),
                          QStringLiteral("camera"), PermissionTable::Block));
        QCOMPARE(table.Rows().length(), 1);
        QVERIFY(table.Remove(QStringLiteral("https://example.com/"), QStringLiteral("camera")));
        QVERIFY(table.IsEmpty());
        QVERIFY(!table.Remove(QStringLiteral("https://example.com/"), QStringLiteral("camera")));
    }

    void whatIsNotKeptIsNotWritten(){
        PermissionTable table;
        QVERIFY(!table.Set(QStringLiteral("file:///C:/a.html"),
                           QStringLiteral("camera"), PermissionTable::Allow));
        QVERIFY(!table.Set(QStringLiteral("https://example.com"),
                           QStringLiteral("no-such-kind"), PermissionTable::Allow));
        QVERIFY(table.IsEmpty());
    }

    void twoRowsAtOnce_data(){
        QTest::addColumn<int>("a");
        QTest::addColumn<int>("b");
        QTest::addColumn<int>("both");

        QTest::newRow("both allowed")     << int(PermissionTable::Allow) << int(PermissionTable::Allow) << int(PermissionTable::Allow);
        QTest::newRow("one allowed")      << int(PermissionTable::Allow) << int(PermissionTable::Ask)   << int(PermissionTable::Ask);
        QTest::newRow("one refused")      << int(PermissionTable::Allow) << int(PermissionTable::Block) << int(PermissionTable::Block);
        QTest::newRow("refused, unknown") << int(PermissionTable::Ask)   << int(PermissionTable::Block) << int(PermissionTable::Block);
        QTest::newRow("neither")          << int(PermissionTable::Ask)   << int(PermissionTable::Ask)   << int(PermissionTable::Ask);
    }
    void twoRowsAtOnce(){
        QFETCH(int, a);
        QFETCH(int, b);
        QFETCH(int, both);
        QCOMPARE(int(PermissionTable::Both(State(a), State(b))), both);
        QCOMPARE(int(PermissionTable::Both(State(b), State(a))), both);
    }

    void theFileComesBackAsItWentOut(){
        PermissionTable table;
        table.Set(QStringLiteral("https://b.example"), QStringLiteral("geolocation"), PermissionTable::Block);
        table.Set(QStringLiteral("https://a.example"), QStringLiteral("camera"), PermissionTable::Allow);
        table.Set(QStringLiteral("https://a.example"), QStringLiteral("notifications"), PermissionTable::Allow);

        bool broken = true;
        const PermissionTable back = PermissionTable::FromJson(table.ToJson(), &broken);
        QVERIFY(!broken);
        QCOMPARE(Describe(back.Rows()), Describe(table.Rows()));

        const QList<Row> rows = back.Rows();
        QCOMPARE(rows[0].kind, QStringLiteral("notifications"));
        QCOMPARE(rows[1].kind, QStringLiteral("camera"));
        QCOMPARE(rows[2].origin, QStringLiteral("https://b.example"));
    }

    void anEmptyFileIsAnEmptyTable(){
        bool broken = true;
        QVERIFY(PermissionTable::FromJson(QByteArray(), &broken).IsEmpty());
        QVERIFY(!broken);
    }

    void aFileWhichDoesNotReadSaysSo(){
        bool broken = false;
        QVERIFY(PermissionTable::FromJson(QByteArrayLiteral("{ not json"), &broken).IsEmpty());
        QVERIFY(broken);

        broken = false;
        const PermissionTable table = PermissionTable::FromJson(QByteArrayLiteral(
            "{\"permissions\":["
            "{\"origin\":\"https://a.example\",\"kind\":\"camera\",\"state\":\"allow\"},"
            "{\"origin\":\"https://a.example/path\",\"kind\":\"camera\",\"state\":\"allow\"},"
            "{\"origin\":\"file:///x\",\"kind\":\"camera\",\"state\":\"allow\"},"
            "{\"origin\":\"https://b.example\",\"kind\":\"camera\",\"state\":\"maybe\"}"
            "]}"), &broken);
        QVERIFY(broken);
        QCOMPARE(table.Rows().length(), 1);
        QCOMPARE(table.Lookup(QStringLiteral("https://a.example"), QStringLiteral("camera")),
                 PermissionTable::Allow);
    }

    void aFileOfAnotherShapeSaysSo_data(){
        QTest::addColumn<QByteArray>("json");
        QTest::newRow("no rows at all") << QByteArrayLiteral("{\"version\":1}");
        QTest::newRow("rows which are not a list") << QByteArrayLiteral("{\"version\":1,\"permissions\":{}}");
        QTest::newRow("a version this build does not know")
            << QByteArrayLiteral("{\"version\":2,\"permissions\":[]}");
        QTest::newRow("not an object") << QByteArrayLiteral("[]");
    }
    void aFileOfAnotherShapeSaysSo(){
        QFETCH(QByteArray, json);
        bool broken = false;
        QVERIFY(PermissionTable::FromJson(json, &broken).IsEmpty());
        QVERIFY(broken);
    }

    void anEmptyTableWrittenOutReadsBackWhole(){
        bool broken = true;
        QVERIFY(PermissionTable::FromJson(PermissionTable().ToJson(), &broken).IsEmpty());
        QVERIFY(!broken);
    }

    void aChangeIsHandedOnAsWhatChanged(){
        PermissionTable before;
        before.Set(QStringLiteral("https://same.example"), QStringLiteral("camera"), PermissionTable::Allow);
        before.Set(QStringLiteral("https://flip.example"), QStringLiteral("notifications"), PermissionTable::Allow);
        before.Set(QStringLiteral("https://gone.example"), QStringLiteral("geolocation"), PermissionTable::Block);

        PermissionTable after = before;
        after.Set(QStringLiteral("https://flip.example"), QStringLiteral("notifications"), PermissionTable::Block);
        after.Remove(QStringLiteral("https://gone.example"), QStringLiteral("geolocation"));
        after.Set(QStringLiteral("https://new.example"), QStringLiteral("notifications"), PermissionTable::Allow);

        QCOMPARE(Describe(after.Delta(before)), Describe({
            Row{ QStringLiteral("https://flip.example"), QStringLiteral("notifications"), PermissionTable::Block },
            Row{ QStringLiteral("https://gone.example"), QStringLiteral("geolocation"), PermissionTable::Ask },
            Row{ QStringLiteral("https://new.example"), QStringLiteral("notifications"), PermissionTable::Allow },
        }));
        QVERIFY(after.Delta(after).isEmpty());
    }

    void aKindThisBuildDoesNotNameIsKept(){
        bool broken = true;
        const PermissionTable table = PermissionTable::FromJson(QByteArrayLiteral(
            "{\"permissions\":[{\"origin\":\"https://a.example\",\"kind\":\"future\",\"state\":\"block\"}]}"),
            &broken);
        QVERIFY(!broken);
        QCOMPARE(PermissionTable::FromJson(table.ToJson(), nullptr).Rows().length(), 1);

        PermissionTable copy = table;
        QVERIFY(!copy.Set(QStringLiteral("https://a.example"), QStringLiteral("future"), PermissionTable::Allow));
        QVERIFY(copy.Remove(QStringLiteral("https://a.example"), QStringLiteral("future")));
        QVERIFY(copy.IsEmpty());
    }

    void theEngineIsBroughtToTheTable(){
        PermissionTable table;
        table.Set(QStringLiteral("https://keep.example"), QStringLiteral("notifications"), PermissionTable::Allow);
        table.Set(QStringLiteral("https://flip.example"), QStringLiteral("geolocation"), PermissionTable::Block);
        table.Set(QStringLiteral("https://new.example"), QStringLiteral("notifications"), PermissionTable::Block);
        table.Set(QStringLiteral("https://new.example"), QStringLiteral("camera"), PermissionTable::Allow);

        const QList<Row> held = {
            Row{ QStringLiteral("https://keep.example:443"), QStringLiteral("notifications"), PermissionTable::Allow },
            Row{ QStringLiteral("https://flip.example"), QStringLiteral("geolocation"), PermissionTable::Allow },
            Row{ QStringLiteral("https://old.example:443"), QStringLiteral("notifications"), PermissionTable::Allow },
            Row{ QStringLiteral("https://old.example"), QStringLiteral("edge:99"), PermissionTable::Block },
            Row{ QStringLiteral("https://new.example"), QStringLiteral("camera"), PermissionTable::Allow },
        };
        const QStringList storable = { QStringLiteral("notifications"), QStringLiteral("geolocation") };

        QCOMPARE(Describe(table.Changes(held, storable)), Describe({
            Row{ QStringLiteral("https://flip.example"), QStringLiteral("geolocation"), PermissionTable::Block },
            Row{ QStringLiteral("https://old.example:443"), QStringLiteral("notifications"), PermissionTable::Ask },
            Row{ QStringLiteral("https://old.example"), QStringLiteral("edge:99"), PermissionTable::Ask },
            Row{ QStringLiteral("https://new.example"), QStringLiteral("camera"), PermissionTable::Ask },
            Row{ QStringLiteral("https://new.example"), QStringLiteral("notifications"), PermissionTable::Block },
        }));
    }

    void anEmptyTableTakesEverythingBack(){
        const QList<Row> held = {
            Row{ QStringLiteral("https://a.example"), QStringLiteral("notifications"), PermissionTable::Allow },
            Row{ QStringLiteral("https://b.example"), QStringLiteral("geolocation"), PermissionTable::Block },
        };
        QCOMPARE(PermissionTable().Changes(held, PermissionTable::Kinds()).length(), 2);
        PermissionTable table;
        table.Set(QStringLiteral("https://a.example"), QStringLiteral("notifications"), PermissionTable::Allow);
        table.Set(QStringLiteral("https://b.example"), QStringLiteral("geolocation"), PermissionTable::Block);
        QVERIFY(table.Changes(held, PermissionTable::Kinds()).isEmpty());
    }
};

QTEST_GUILESS_MAIN(tst_permissiontable)
#include "tst_permissiontable.moc"
