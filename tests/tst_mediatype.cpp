#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QTemporaryDir>
#include <QImage>

#include "mediatype.hpp"

#include "testsupport.hpp"

class tst_mediatype : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void classifiesAMimeNameByItsType();
    void classifiesAMimeNameByItsType_data();

    void nothingClaimsAnEmptyName();

    void findsTheMimeTypeOfTheSuffixesTheOldListHad();
    void findsTheMimeTypeOfTheSuffixesTheOldListHad_data();

    void findsTheSuffixesTheOldListDidNotHave();
    void findsTheSuffixesTheOldListDidNotHave_data();

    void readsARealImage();
    void doesNotCallAnUndecodableImageAnImage();
    void doesNotCallAPlainFileMedia();
    void sniffsTheContentsRatherThanTheName();

    void tellsWhatItMakesOfASuffixTwoThingsClaim();

private:
    QTemporaryDir m_Dir;
    QString Write(const QString &name, const QByteArray &contents);
};

void tst_mediatype::initTestCase(){
    TestSupport::SilenceDebugOutput();
    QVERIFY(m_Dir.isValid());
}

QString tst_mediatype::Write(const QString &name, const QByteArray &contents){
    const QString path = m_Dir.filePath(name);
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)) return QString();
    file.write(contents);
    file.close();
    return path;
}

void tst_mediatype::classifiesAMimeNameByItsType_data(){
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("kind");

    QTest::newRow("png")       << "image/png"                  << int(MediaType::Image);
    QTest::newRow("webp")      << "image/webp"                 << int(MediaType::Image);
    QTest::newRow("svg")       << "image/svg+xml"              << int(MediaType::Image);
    QTest::newRow("mp3")       << "audio/mpeg"                 << int(MediaType::Audio);
    QTest::newRow("flac")      << "audio/flac"                 << int(MediaType::Audio);
    QTest::newRow("wma")       << "audio/x-ms-wma"             << int(MediaType::Audio);
    QTest::newRow("mp4")       << "video/mp4"                  << int(MediaType::Video);
    QTest::newRow("matroska")  << "video/x-matroska"           << int(MediaType::Video);
    QTest::newRow("webm")      << "video/webm"                 << int(MediaType::Video);

    QTest::newRow("ogg")       << "application/ogg"            << int(MediaType::Video);
    QTest::newRow("mxf")       << "application/mxf"            << int(MediaType::Video);

    QTest::newRow("text")      << "text/plain"                 << int(MediaType::NotMedia);
    QTest::newRow("html")      << "text/html"                  << int(MediaType::NotMedia);
    QTest::newRow("pdf")       << "application/pdf"            << int(MediaType::NotMedia);
    QTest::newRow("binary")    << "application/octet-stream"   << int(MediaType::NotMedia);
    QTest::newRow("zip")       << "application/zip"            << int(MediaType::NotMedia);
    QTest::newRow("imagelike") << "imaginary/thing"            << int(MediaType::NotMedia);
    QTest::newRow("videolike") << "x-video/thing"              << int(MediaType::NotMedia);
}

void tst_mediatype::classifiesAMimeNameByItsType(){
    QFETCH(QString, name);
    QFETCH(int, kind);
    QCOMPARE(int(MediaType::KindOfMimeName(name)), kind);
}

void tst_mediatype::nothingClaimsAnEmptyName(){
    QCOMPARE(int(MediaType::KindOfMimeName(QString())), int(MediaType::NotMedia));
    QCOMPARE(int(MediaType::KindOfMimeName(QStringLiteral(""))), int(MediaType::NotMedia));
}

void tst_mediatype::findsTheMimeTypeOfTheSuffixesTheOldListHad_data(){
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<int>("kind");

    QTest::newRow("mpeg") << ".mpeg" << int(MediaType::Video);
    QTest::newRow("mpg")  << ".mpg"  << int(MediaType::Video);
    QTest::newRow("avi")  << ".avi"  << int(MediaType::Video);
    QTest::newRow("wmv")  << ".wmv"  << int(MediaType::Video);
    QTest::newRow("flv")  << ".flv"  << int(MediaType::Video);
    QTest::newRow("asf")  << ".asf"  << int(MediaType::Video);
    QTest::newRow("mp4")  << ".mp4"  << int(MediaType::Video);

    QTest::newRow("mp3")  << ".mp3"  << int(MediaType::Audio);
    QTest::newRow("wav")  << ".wav"  << int(MediaType::Audio);
    QTest::newRow("wma")  << ".wma"  << int(MediaType::Audio);
}

void tst_mediatype::findsTheMimeTypeOfTheSuffixesTheOldListHad(){
    QFETCH(QString, suffix);
    QFETCH(int, kind);

    const QString name = QStringLiteral("sample") + suffix;
    QCOMPARE(int(MediaType::KindOfMimeName(MediaType::MimeNameOfPath(name))), kind);
}

void tst_mediatype::findsTheSuffixesTheOldListDidNotHave_data(){
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<int>("kind");

    QTest::newRow("webm") << ".webm" << int(MediaType::Video);
    QTest::newRow("mkv")  << ".mkv"  << int(MediaType::Video);
    QTest::newRow("mov")  << ".mov"  << int(MediaType::Video);
    QTest::newRow("m4v")  << ".m4v"  << int(MediaType::Video);
    QTest::newRow("3gp")  << ".3gp"  << int(MediaType::Video);

    QTest::newRow("m4a")  << ".m4a"  << int(MediaType::Audio);
    QTest::newRow("flac") << ".flac" << int(MediaType::Audio);
    QTest::newRow("opus") << ".opus" << int(MediaType::Audio);
    QTest::newRow("aac")  << ".aac"  << int(MediaType::Audio);
}

void tst_mediatype::findsTheSuffixesTheOldListDidNotHave(){
    QFETCH(QString, suffix);
    QFETCH(int, kind);

    const QString name = QStringLiteral("sample") + suffix;
    QCOMPARE(int(MediaType::KindOfMimeName(MediaType::MimeNameOfPath(name))), kind);
}

void tst_mediatype::readsARealImage(){
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QVERIFY(QImage(4, 4, QImage::Format_RGB32).save(&buffer, "PNG"));
    buffer.close();

    const QString path = Write(QStringLiteral("real.png"), buffer.data());
    QVERIFY(!path.isEmpty());

    QVERIFY(MediaType::IsDecodableImage(path));
    QCOMPARE(int(MediaType::KindOfPath(path)), int(MediaType::Image));
    QVERIFY(MediaType::IsMedia(path));
}

void tst_mediatype::doesNotCallAnUndecodableImageAnImage(){
    const QString path = Write(QStringLiteral("broken.png"), QByteArray("not a png at all"));
    QVERIFY(!path.isEmpty());

    QCOMPARE(int(MediaType::KindOfMimeName(MediaType::MimeNameOfPath(path))),
             int(MediaType::Image));
    QVERIFY(!MediaType::IsDecodableImage(path));
    QCOMPARE(int(MediaType::KindOfPath(path)), int(MediaType::NotMedia));
}

void tst_mediatype::doesNotCallAPlainFileMedia(){
    const QString path = Write(QStringLiteral("notes.txt"), QByteArray("hello"));
    QVERIFY(!path.isEmpty());

    QCOMPARE(int(MediaType::KindOfPath(path)), int(MediaType::NotMedia));
    QVERIFY(!MediaType::IsMedia(path));

    QCOMPARE(int(MediaType::KindOfPath(m_Dir.filePath(QStringLiteral("missing.txt")))),
             int(MediaType::NotMedia));
}

void tst_mediatype::sniffsTheContentsRatherThanTheName(){
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QVERIFY(QImage(4, 4, QImage::Format_RGB32).save(&buffer, "PNG"));
    buffer.close();

    const QString path = Write(QStringLiteral("disguised.dat"), buffer.data());
    QVERIFY(!path.isEmpty());

    QCOMPARE(int(MediaType::KindOfMimeName(MediaType::MimeNameOfPath(path))),
             int(MediaType::NotMedia));
    QCOMPARE(int(MediaType::KindOfPath(path)), int(MediaType::Image));
}

void tst_mediatype::tellsWhatItMakesOfASuffixTwoThingsClaim(){
    const QString name = MediaType::MimeNameOfPath(QStringLiteral("vanilla_ja.ts"));
    QVERIFY2(MediaType::KindOfMimeName(name) == MediaType::NotMedia,
             qPrintable(QStringLiteral("'.ts' now reads as %1").arg(name)));
}

QTEST_MAIN(tst_mediatype)
#include "tst_mediatype.moc"
