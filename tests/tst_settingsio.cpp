#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QVariant>
#include <QUrl>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QColor>
#include <QIcon>
#include <QPixmap>

#include "settingsio.hpp"
#include "fileexchange.hpp"

#include "testsupport.hpp"

namespace {

SettingsIO::Map MapOf(const QString &key, const QVariant &value){
    SettingsIO::Map map;
    map.insert(key, value);
    return map;
}

QVariant RoundTrip(const QVariant &value){
    SettingsIO::Map read;
    if(!SettingsIO::ReadJson(SettingsIO::WriteJson(MapOf(QStringLiteral("g/k"), value)), read))
        return QVariant();
    return read.value(QStringLiteral("g/k"));
}

QString Written(const QVariant &value){
    return QString::fromUtf8(SettingsIO::WriteJson(MapOf(QStringLiteral("g/k"), value)));
}

QString ReadAll(const QString &path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return QString();
    QString text = QString::fromUtf8(file.readAll());
    file.close();
    return text;
}

void WriteAll(const QString &path, const QString &text){
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(text.toUtf8());
    file.close();
}

}

class tst_settingsio : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_Dir;

    QString Dir() const { return m_Dir.path() + QStringLiteral("/");}
    QString Path(const QString &name) const { return Dir() + name;}

    static SettingsIO::Hooks Hooks(QStringList *restored = nullptr){
        SettingsIO::Hooks hooks;
        hooks.BackUpFiltersOf = [](){
            return QStringList() << QStringLiteral("*config.json") << QStringLiteral("*config.xml");
        };
        hooks.BackUpPrepositionOf = [](){ return QStringLiteral("~");};
        if(restored)
            hooks.RestoredFromBackUp = [restored](QString backup){ *restored << backup;};
        return hooks;
    }

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        QVERIFY(m_Dir.isValid());
    }

    void cleanup(){
        QDir dir(m_Dir.path());
        foreach(QString name, dir.entryList(QDir::Files)) dir.remove(name);
    }

    void thetypesJsonCanExpressAreWrittenNatively(){
        QCOMPARE(RoundTrip(QVariant(QStringLiteral("text"))).toString(), QStringLiteral("text"));
        QCOMPARE(RoundTrip(QVariant(true)).toBool(), true);
        QCOMPARE(RoundTrip(QVariant(false)).toBool(), false);
        QCOMPARE(RoundTrip(QVariant(42)).toInt(), 42);
        QCOMPARE(RoundTrip(QVariant(-7)).toInt(), -7);
        QCOMPARE(RoundTrip(QVariant(3.25)).toDouble(), 3.25);
        QCOMPARE(RoundTrip(QVariant(QStringList() << QStringLiteral("a") << QStringLiteral("b"))).toStringList(),
                 QStringList() << QStringLiteral("a") << QStringLiteral("b"));

        QVERIFY(Written(QVariant(QStringLiteral("text"))).contains(QStringLiteral("\"g/k\": \"text\"")));
        QVERIFY(Written(QVariant(true)).contains(QStringLiteral("\"g/k\": true")));
        QVERIFY(Written(QVariant(42)).contains(QStringLiteral("\"g/k\": 42")));
    }

    void everyOtherTypeIsWrittenWithItsTag(){
        QVERIFY(Written(QVariant(QUrl(QStringLiteral("https://example.com/")))).contains(QStringLiteral("\"@url\"")));
        QVERIFY(Written(QVariant(QSize(3, 4))).contains(QStringLiteral("\"@size\"")));
        QVERIFY(Written(QVariant(QSizeF(3.5, 4.5))).contains(QStringLiteral("\"@sizef\"")));
        QVERIFY(Written(QVariant(QPoint(3, 4))).contains(QStringLiteral("\"@point\"")));
        QVERIFY(Written(QVariant(QPointF(3.5, 4.5))).contains(QStringLiteral("\"@pointf\"")));
        QVERIFY(Written(QVariant(QRect(1, 2, 3, 4))).contains(QStringLiteral("\"@rect\"")));
        QVERIFY(Written(QVariant(QRectF(1.5, 2.5, 3.5, 4.5))).contains(QStringLiteral("\"@rectf\"")));
        QVERIFY(Written(QVariant(QColor(1, 2, 3, 4))).contains(QStringLiteral("\"@color\"")));
        QVERIFY(Written(QVariant(QByteArray("bytes"))).contains(QStringLiteral("\"@variant\"")));
    }

    void thetaggedTypesSurviveTheRoundTrip(){
        QCOMPARE(RoundTrip(QVariant(QUrl(QStringLiteral("https://example.com/?q=%E3%81%82")))).toUrl(),
                 QUrl(QStringLiteral("https://example.com/?q=%E3%81%82")));
        QCOMPARE(RoundTrip(QVariant(QSize(3, 4))).value<QSize>(), QSize(3, 4));
        QCOMPARE(RoundTrip(QVariant(QSizeF(3.5, 4.5))).value<QSizeF>(), QSizeF(3.5, 4.5));
        QCOMPARE(RoundTrip(QVariant(QPoint(-3, 4))).value<QPoint>(), QPoint(-3, 4));
        QCOMPARE(RoundTrip(QVariant(QPointF(-3.5, 4.5))).value<QPointF>(), QPointF(-3.5, 4.5));
        QCOMPARE(RoundTrip(QVariant(QRect(1, 2, 3, 4))).value<QRect>(), QRect(1, 2, 3, 4));
        QCOMPARE(RoundTrip(QVariant(QRectF(1.5, 2.5, 3.5, 4.5))).value<QRectF>(), QRectF(1.5, 2.5, 3.5, 4.5));
        QCOMPARE(RoundTrip(QVariant(QRect(-1920, -100, 800, 600))).value<QRect>(), QRect(-1920, -100, 800, 600));
        QCOMPARE(RoundTrip(QVariant(QColor(1, 2, 3, 4))).value<QColor>(), QColor(1, 2, 3, 4));
    }

    void abyteArrayOfEncryptedBytesSurvivesTheRoundTrip(){
        QByteArray secret;
        for(int i = 0; i < 256; i++) secret.append(static_cast<char>(i));

        QVariant out = RoundTrip(QVariant(secret));
        QCOMPARE(out.typeId(), static_cast<int>(QMetaType::QByteArray));
        QCOMPARE(out.toByteArray(), secret);
    }

    void aniconSurvivesTheRoundTrip(){
        QPixmap pixmap(16, 16);
        pixmap.fill(QColor(10, 20, 30));
        QIcon icon(pixmap);

        QVariant out = RoundTrip(QVariant::fromValue(icon));
        QCOMPARE(out.typeId(), static_cast<int>(QMetaType::QIcon));
        QIcon back = out.value<QIcon>();
        QVERIFY(!back.isNull());
        QCOMPARE(back.availableSizes().first(), QSize(16, 16));
        QCOMPARE(back.pixmap(QSize(16, 16)).toImage().pixelColor(8, 8), QColor(10, 20, 30));
    }

    void anunknownTagReadsAsAnEmptyValue(){
        SettingsIO::Map map;
        QVERIFY(SettingsIO::ReadJson("{\"a\": {\"@nosuchtag\": [1,2]}, \"b\": {}, \"c\": 5}", map));
        QVERIFY(!map.value(QStringLiteral("a")).isValid());
        QVERIFY(!map.value(QStringLiteral("b")).isValid());
        QCOMPARE(map.value(QStringLiteral("c")).toInt(), 5);
    }

    void writesAflatObjectWithTheKeysSorted(){
        SettingsIO::Map map;
        map.insert(QStringLiteral("zzz/@Last"), QVariant(1));
        map.insert(QStringLiteral("aaa/@First"), QVariant(QStringLiteral("x")));
        map.insert(QStringLiteral("mmm/@Middle"), QVariant(true));

        QCOMPARE(QString::fromUtf8(SettingsIO::WriteJson(map)),
                 QStringLiteral("{\n"
                                "    \"aaa/@First\": \"x\",\n"
                                "    \"mmm/@Middle\": true,\n"
                                "    \"zzz/@Last\": 1\n"
                                "}\n"));
    }

    void akeyIsNeverSplitOnItsSeparators(){
        const QString awkward = QStringLiteral("keymap/TreeBank/Ctrl+Shift+/");
        SettingsIO::Map read;
        QVERIFY(SettingsIO::ReadJson(SettingsIO::WriteJson(MapOf(awkward, QVariant(QStringLiteral("Close")))), read));
        QCOMPARE(read.keys(), QStringList() << awkward);
        QCOMPARE(read.value(awkward).toString(), QStringLiteral("Close"));
    }

    void thewholeMapSurvivesAwriteAndAread(){
        SettingsIO::Map map;
        map.insert(QStringLiteral("application/@ColorScheme"), QVariant(QStringLiteral("dark")));
        map.insert(QStringLiteral("application/@AutoSaveInterval"), QVariant(300000));
        map.insert(QStringLiteral("application/@ExternalCommands"),
                   QVariant(QStringList() << QStringLiteral("editor = notepad %u")));
        map.insert(QStringLiteral("mainwindow/geometry0"), QVariant(QRect(10, 20, 800, 600)));
        map.insert(QStringLiteral("searchengine/日本語"), QVariant(QUrl(QStringLiteral("https://example.jp/?q=%s"))));
        map.insert(QStringLiteral("icon/example.com"), QVariant(QByteArray("\x00\x01\xff", 3)));

        const QByteArray first = SettingsIO::WriteJson(map);
        SettingsIO::Map read;
        QVERIFY(SettingsIO::ReadJson(first, read));
        QCOMPARE(read.keys(), map.keys());
        QCOMPARE(SettingsIO::WriteJson(read), first);
    }

    void abrokenFileIsRefusedWithoutTouchingTheMap(){
        SettingsIO::Map map = MapOf(QStringLiteral("already/here"), QVariant(1));

        QVERIFY(!SettingsIO::ReadJson("{\"unterminated\": ", map));
        QVERIFY(!SettingsIO::ReadJson("[1,2,3]", map));
        QCOMPARE(map.keys(), QStringList() << QStringLiteral("already/here"));
    }

    void saveThenLoadIsTheSameMap(){
        SettingsIO::Map map;
        map.insert(QStringLiteral("application/@ColorScheme"), QVariant(QStringLiteral("dark")));
        map.insert(QStringLiteral("mainwindow/geometry0"), QVariant(QRect(1, 2, 3, 4)));

        QVERIFY(SettingsIO::Save(Dir(), QStringLiteral("config.json"), map, Hooks()));

        SettingsIO::Map read;
        SettingsIO::Load(Dir(), QStringLiteral("config.json"), read, Hooks());
        QCOMPARE(read.value(QStringLiteral("application/@ColorScheme")).toString(), QStringLiteral("dark"));
        QCOMPARE(read.value(QStringLiteral("mainwindow/geometry0")).value<QRect>(), QRect(1, 2, 3, 4));
    }

    void saveLeavesNoTemporaryBehind(){
        QVERIFY(SettingsIO::Save(Dir(), QStringLiteral("config.json"), MapOf(QStringLiteral("a"), QVariant(1)), Hooks()));
        QVERIFY(QFile::exists(Path(QStringLiteral("config.json"))));
        QVERIFY(!QFile::exists(Path(QStringLiteral("~config.json"))));

        WriteAll(Path(QStringLiteral("~config.json")), QStringLiteral("half written"));
        QVERIFY(SettingsIO::Save(Dir(), QStringLiteral("config.json"), MapOf(QStringLiteral("b"), QVariant(2)), Hooks()));
        QVERIFY(!QFile::exists(Path(QStringLiteral("~config.json"))));
        QVERIFY(ReadAll(Path(QStringLiteral("config.json"))).contains(QStringLiteral("\"b\"")));
        QVERIFY(!ReadAll(Path(QStringLiteral("config.json"))).contains(QStringLiteral("\"a\"")));
        QVERIFY(!QFile::exists(Path(QStringLiteral("config.json.prev"))));
    }

    void afailedSwapRestoresTheExistingFile(){
        const QString target = Path(QStringLiteral("session.json"));
        const QString missing = Path(QStringLiteral("missing.tmp"));
        WriteAll(target, QStringLiteral("kept"));

        QVERIFY(!FileExchange::Replace(missing, target));
        QCOMPARE(ReadAll(target), QStringLiteral("kept"));
        QVERIFY(!QFile::exists(target + QStringLiteral(".prev")));
    }

    void loadRecoversTheGenerationLeftAsideByAnInterruptedSwap(){
        WriteAll(Path(QStringLiteral("config.json.prev")), QStringLiteral("{\"a/k\": \"kept\"}"));

        QStringList restored;
        SettingsIO::Map map;
        SettingsIO::Load(Dir(), QStringLiteral("config.json"), map, Hooks(&restored));
        QCOMPARE(map.value(QStringLiteral("a/k")).toString(), QStringLiteral("kept"));
        QCOMPARE(restored, QStringList() << QStringLiteral("config.json.prev"));
    }

    void thefileItselfWinsOverAleftoverPrev(){
        WriteAll(Path(QStringLiteral("config.json")),      QStringLiteral("{\"a/k\": \"current\"}"));
        WriteAll(Path(QStringLiteral("config.json.prev")), QStringLiteral("{\"a/k\": \"stale\"}"));

        SettingsIO::Map map;
        SettingsIO::Load(Dir(), QStringLiteral("config.json"), map, Hooks());
        QCOMPARE(map.value(QStringLiteral("a/k")).toString(), QStringLiteral("current"));
    }

    void loadIgnoresTheLegacyXml(){
        WriteAll(Path(QStringLiteral("config.xml")),
                 QStringLiteral("<body><application><setting id=\"@ColorScheme\">light</setting></application></body>"));

        QStringList restored;
        SettingsIO::Map map;
        SettingsIO::Load(Dir(), QStringLiteral("config.json"), map, Hooks(&restored));
        QVERIFY(map.isEmpty());
        QVERIFY(restored.isEmpty());
    }

    void loadFallsBackToTheNewestBackupAndSaysSo(){
        WriteAll(Path(QStringLiteral("config.json")), QStringLiteral("{ truncated"));
        WriteAll(Path(QStringLiteral("20260101000000config.json")), QStringLiteral("{\"a/k\": \"old\"}"));
        WriteAll(Path(QStringLiteral("20260201000000config.json")), QStringLiteral("{\"a/k\": \"new\"}"));

        QStringList restored;
        SettingsIO::Map map;
        SettingsIO::Load(Dir(), QStringLiteral("config.json"), map, Hooks(&restored));

        QCOMPARE(map.value(QStringLiteral("a/k")).toString(), QStringLiteral("new"));
        QCOMPARE(restored, QStringList() << QStringLiteral("20260201000000config.json"));
    }

    void loadSkipsLegacyAndBrokenBackupsBeforeTheNextJson(){
        WriteAll(Path(QStringLiteral("config.json")), QStringLiteral("{ truncated"));
        WriteAll(Path(QStringLiteral("20260401000000config.xml")),
                 QStringLiteral("{\"a/k\": \"wrong extension\"}"));
        WriteAll(Path(QStringLiteral("20260301000000config.json")), QStringLiteral("{ broken"));
        WriteAll(Path(QStringLiteral("20260201000000config.xml")),
                 QStringLiteral("{\"a/k\": \"also wrong\"}"));
        WriteAll(Path(QStringLiteral("20260101000000config.json")), QStringLiteral("{\"a/k\": \"json\"}"));

        QStringList restored;
        SettingsIO::Map map;
        SettingsIO::Load(Dir(), QStringLiteral("config.json"), map, Hooks(&restored));
        QCOMPARE(map.value(QStringLiteral("a/k")).toString(), QStringLiteral("json"));
        QCOMPARE(restored, QStringList() << QStringLiteral("20260101000000config.json"));
    }

    void loadOnAnEmptyDirectoryIsSilent(){
        QStringList restored;
        SettingsIO::Map map;
        SettingsIO::Load(Dir(), QStringLiteral("config.json"), map, Hooks(&restored));
        QVERIFY(map.isEmpty());
        QVERIFY(restored.isEmpty());
    }

    void thehooksAreOptional(){
        QVERIFY(SettingsIO::Save(Dir(), QStringLiteral("config.json"), MapOf(QStringLiteral("a"), QVariant(1))));
        QVERIFY(QFile::exists(Path(QStringLiteral("config.json"))));

        SettingsIO::Map map;
        SettingsIO::Load(Dir(), QStringLiteral("config.json"), map);
        QCOMPARE(map.value(QStringLiteral("a")).toInt(), 1);

        QFile::remove(Path(QStringLiteral("config.json")));
        WriteAll(Path(QStringLiteral("config.xml")), QStringLiteral("<body><a><setting id=\"k\">x</setting></a></body>"));
        SettingsIO::Map none;
        SettingsIO::Load(Dir(), QStringLiteral("config.json"), none);
        QVERIFY(none.isEmpty());
    }
};

QTEST_MAIN(tst_settingsio)
#include "tst_settingsio.moc"
