#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QCursor>
#include <QThreadPool>

#include "lightnode.hpp"
#include "nodepreview.hpp"
#include "theme.hpp"
#include "application.hpp"

#include "testsupport.hpp"

namespace {

QImage MakeImage(const QSize &size){
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    for(int i = 0; i < size.width(); i += 7){
        painter.setPen(QColor(i % 256, (i * 3) % 256, (i * 7) % 256));
        painter.drawLine(i, 0, size.width() - i, size.height());
    }
    painter.end();
    return image;
}

QString ThumbnailPath(ViewNode *vn){
    return Application::ThumbnailDirectory() + vn->GetImageFileName();
}

}

class tst_thumbnail : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void setImageWritesTheFileStraightAway();
    void setImageKeepsOnlyTheSmallCopyInMemory();
    void theSavedFileKeepsTheCaptureSize();
    void largeImageComesFromTheFile();
    void largeImageFallsBackToTheResidentCopy();
    void setImageAgainOverwritesTheSameFile();
    void clearingTheImageRemovesTheFile();

    void aPrivateNodesPictureStaysInMemory();
    void aPrivateNodesHistoryIsNeverSaved();
    void anEmptyPictureOfAPrivateNodeKeepsItPrivate();
    void goingOffTheDiskTakesTheFilesAndKeepsWhatMemoryHolds();
    void aCopyOfAPrivateNodeHasNoFiles();
    void aCopyOfANodeJustBackFromPrivateWritesWhatMemoryHolds();
    void aNodeWaitsForItsPictureToBeReadBeforeItGoes();

    void aFolderBorrowsThePictureOfTheTabItWouldOpen();
    void nothingToBorrowLeavesTheFolderWithoutAPicture();

    void thePreviewGoesUnderTheTab();
    void thePreviewFlipsAboveTheTabWhenThereIsNoRoomBelow();
    void thePreviewStaysOnTheScreen();
    void thePreviewStaysOnTheScreen_data();

    void thePreviewOfAFolderWithNothingToBorrowSaysSo();
    void thePreviewKnowsWhichTabItIsWaitingFor();
    void thePreviewHeldStaysAwayUntilThePointerMoves();
    void thePreviewGoesWhenTheFocusWindowChanges();

private:
    ViewNode *m_Root;
};

void tst_thumbnail::initTestCase(){
    TestSupport::SilenceDebugOutput();
    QDir().mkpath(Application::ThumbnailDirectory());
    QDir().mkpath(Application::HistoryDirectory());
    m_Root = new ViewNode();
}

void tst_thumbnail::cleanup(){
    QDir dir(Application::ThumbnailDirectory());
    foreach(const QString &name, dir.entryList(QDir::Files))
        QFile::remove(Application::ThumbnailDirectory() + name);
    QDir history(Application::HistoryDirectory());
    foreach(const QString &name, history.entryList(QDir::Files))
        QFile::remove(Application::HistoryDirectory() + name);
}

namespace {
int FilesIn(const QString &directory){
    return QDir(directory).entryList(QDir::Files).length();
}
}

void tst_thumbnail::aPrivateNodesPictureStaysInMemory(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);
    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);
    QCOMPARE(FilesIn(Application::ThumbnailDirectory()), 1);

    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), true);
    QCOMPARE(FilesIn(Application::ThumbnailDirectory()), 0);
    QVERIFY(vn->GetImageFileName().isEmpty());
    QVERIFY(vn->IsOffDisk());
    QCOMPARE(vn->GetImage().size(), RESIDENT_THUMBNAIL_SIZE);
    QCOMPARE(vn->GetLargeImage().size(), RESIDENT_THUMBNAIL_SIZE);

    vn->SaveImageIfNeed();
    QCOMPARE(FilesIn(Application::ThumbnailDirectory()), 0);
    vn->Delete();
}

void tst_thumbnail::aPrivateNodesHistoryIsNeverSaved(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);
    vn->SetHistoryData(QByteArrayLiteral("history"), true);
    vn->SaveHistoryIfNeed();
    QCOMPARE(FilesIn(Application::HistoryDirectory()), 0);
    QVERIFY(vn->GetHistoryFileName().isEmpty());
    QCOMPARE(vn->GetHistoryData(), QByteArrayLiteral("history"));
    vn->Delete();
}

void tst_thumbnail::anEmptyPictureOfAPrivateNodeKeepsItPrivate(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);
    vn->SetHistoryData(QByteArrayLiteral("history"), true);
    vn->SetImage(QImage(), true);
    QVERIFY(vn->IsOffDisk());
    vn->SaveHistoryIfNeed();
    QCOMPARE(FilesIn(Application::HistoryDirectory()), 0);
    vn->Delete();
}

void tst_thumbnail::goingOffTheDiskTakesTheFilesAndKeepsWhatMemoryHolds(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);
    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);
    vn->SetHistoryData(QByteArrayLiteral("history"), false);
    vn->SaveHistoryIfNeed();
    QCOMPARE(FilesIn(Application::ThumbnailDirectory()), 1);
    QCOMPARE(FilesIn(Application::HistoryDirectory()), 1);

    vn->SetOffDisk(true);
    QCOMPARE(FilesIn(Application::ThumbnailDirectory()), 0);
    QCOMPARE(FilesIn(Application::HistoryDirectory()), 0);
    QVERIFY(vn->GetImageFileName().isEmpty());
    QVERIFY(vn->GetHistoryFileName().isEmpty());
    QVERIFY(!vn->GetImage().isNull());
    QCOMPARE(vn->GetHistoryData(), QByteArrayLiteral("history"));

    vn->SetOffDisk(false);
    vn->SaveImageIfNeed();
    vn->SaveHistoryIfNeed();
    QCOMPARE(FilesIn(Application::ThumbnailDirectory()), 1);
    QCOMPARE(FilesIn(Application::HistoryDirectory()), 1);
    vn->Delete();
}

void tst_thumbnail::aCopyOfAPrivateNodeHasNoFiles(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);
    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), true);
    vn->SetHistoryData(QByteArrayLiteral("history"), true);
    ViewNode *copy = vn->Clone();
    QVERIFY(copy);
    QVERIFY(copy->IsOffDisk());
    QVERIFY(copy->GetImageFileName().isEmpty());
    QVERIFY(copy->GetHistoryFileName().isEmpty());
    QVERIFY(!copy->GetImage().isNull());
    copy->SaveImageIfNeed();
    copy->SaveHistoryIfNeed();
    QCOMPARE(FilesIn(Application::ThumbnailDirectory()), 0);
    QCOMPARE(FilesIn(Application::HistoryDirectory()), 0);
    copy->Delete();
    vn->Delete();
}

void tst_thumbnail::aCopyOfANodeJustBackFromPrivateWritesWhatMemoryHolds(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);
    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), true);
    vn->SetHistoryData(QByteArrayLiteral("history"), true);
    vn->SetOffDisk(false);
    ViewNode *copy = vn->Clone();
    QVERIFY(copy);
    QVERIFY(!copy->IsOffDisk());
    copy->SaveImageIfNeed();
    copy->SaveHistoryIfNeed();
    QCOMPARE(FilesIn(Application::ThumbnailDirectory()), 1);
    QCOMPARE(FilesIn(Application::HistoryDirectory()), 1);
    copy->Delete();
    vn->Delete();
}

void tst_thumbnail::aNodeWaitsForItsPictureToBeReadBeforeItGoes(){
    ViewNode *source = m_Root->MakeChild();
    source->SetHoldView(true);
    source->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);
    const QString name = source->GetImageFileName();

    {   ViewNode *vn = m_Root->MakeChild();
        vn->SetHoldView(true);
        vn->SetImageFileName(name);
        QVERIFY(vn->GetImage().isNull());
        QTRY_COMPARE(vn->GetImage().size(), RESIDENT_THUMBNAIL_SIZE);
        vn->Delete();
    }

    for(int i = 0; i < 20; i++){
        ViewNode *vn = m_Root->MakeChild();
        vn->SetHoldView(true);
        vn->SetImageFileName(name);
        QVERIFY(vn->GetImage().isNull());
        vn->Delete();
    }
    QThreadPool::globalInstance()->waitForDone();
    source->Delete();
}

void tst_thumbnail::setImageWritesTheFileStraightAway(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);

    QVERIFY(vn->GetImageFileName().isEmpty());

    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);

    QVERIFY(!vn->GetImageFileName().isEmpty());
    QVERIFY(QFile::exists(ThumbnailPath(vn)));

    vn->Delete();
}

void tst_thumbnail::setImageKeepsOnlyTheSmallCopyInMemory(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);

    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);

    QCOMPARE(vn->GetImage().size(), RESIDENT_THUMBNAIL_SIZE);
    QVERIFY(RESIDENT_THUMBNAIL_SIZE.width()  < SAVING_THUMBNAIL_SIZE.width());
    QVERIFY(RESIDENT_THUMBNAIL_SIZE.height() < SAVING_THUMBNAIL_SIZE.height());

    vn->Delete();
}

void tst_thumbnail::theSavedFileKeepsTheCaptureSize(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);

    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);

    QCOMPARE(QImage(ThumbnailPath(vn)).size(), SAVING_THUMBNAIL_SIZE);

    vn->Delete();
}

void tst_thumbnail::largeImageComesFromTheFile(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);

    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);

    QCOMPARE(vn->GetLargeImage().size(), SAVING_THUMBNAIL_SIZE);
    QVERIFY(vn->GetLargeImage().size() != vn->GetImage().size());

    vn->Delete();
}

void tst_thumbnail::largeImageFallsBackToTheResidentCopy(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);

    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);
    QFile::remove(ThumbnailPath(vn));

    QCOMPARE(vn->GetLargeImage().size(), RESIDENT_THUMBNAIL_SIZE);

    vn->Delete();
}

void tst_thumbnail::setImageAgainOverwritesTheSameFile(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);

    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);
    const QString first = vn->GetImageFileName();

    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);

    QCOMPARE(vn->GetImageFileName(), first);
    QCOMPARE(QDir(Application::ThumbnailDirectory()).entryList(QDir::Files).length(), 1);

    vn->Delete();
}

void tst_thumbnail::clearingTheImageRemovesTheFile(){
    ViewNode *vn = m_Root->MakeChild();
    vn->SetHoldView(true);

    vn->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);
    const QString path = ThumbnailPath(vn);

    vn->SetImage(QImage(), false);

    QVERIFY(vn->GetImageFileName().isEmpty());
    QVERIFY(vn->GetImage().isNull());
    QVERIFY(!QFile::exists(path));

    vn->Delete();
}

void tst_thumbnail::aFolderBorrowsThePictureOfTheTabItWouldOpen(){
    ViewNode *folder = m_Root->MakeChild();
    ViewNode *first  = folder->MakeChild();
    ViewNode *second = folder->MakeChild();
    first->SetHoldView(true);
    second->SetHoldView(true);

    first->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);
    second->SetImage(MakeImage(SAVING_THUMBNAIL_SIZE), false);

    QVERIFY(folder->IsDirectory());
    QVERIFY(folder->GetImage().isNull());

    QCOMPARE(folder->ImageOwner(), static_cast<Node*>(first));

    second->ResetPrimaryPath();
    QCOMPARE(folder->ImageOwner(), static_cast<Node*>(second));

    QCOMPARE(folder->VisibleImage().size(), RESIDENT_THUMBNAIL_SIZE);
    QCOMPARE(folder->VisibleLargeImage().size(), SAVING_THUMBNAIL_SIZE);

    folder->Delete();
}

void tst_thumbnail::nothingToBorrowLeavesTheFolderWithoutAPicture(){
    ViewNode *folder = m_Root->MakeChild();
    ViewNode *child  = folder->MakeChild();
    child->SetHoldView(true);

    QVERIFY(!folder->ImageOwner());
    QVERIFY(folder->VisibleImage().isNull());
    QVERIFY(folder->VisibleLargeImage().isNull());

    folder->Delete();
}

void tst_thumbnail::thePreviewGoesUnderTheTab(){
    const QRect screen(0, 0, 1920, 1080);
    const QSize size(272, 227);
    const QRect tab(800, 30, 150, 28);

    const QRect placed = NodePreview::Place(tab, size, screen, 4);

    QCOMPARE(placed.size(), size);
    QCOMPARE(placed.y(), tab.y() + tab.height() + 4);
    QVERIFY(qAbs(placed.center().x() - tab.center().x()) <= 1);
}

void tst_thumbnail::thePreviewFlipsAboveTheTabWhenThereIsNoRoomBelow(){
    const QRect screen(0, 0, 1920, 1080);
    const QSize size(272, 227);
    const QRect tab(800, 1040, 150, 28);

    const QRect placed = NodePreview::Place(tab, size, screen, 4);

    QCOMPARE(placed.y(), tab.y() - 4 - size.height());
    QVERIFY(placed.y() >= screen.y());
}

void tst_thumbnail::thePreviewStaysOnTheScreen_data(){
    QTest::addColumn<QRect>("screen");
    QTest::addColumn<QRect>("tab");

    const QRect main(0, 0, 1920, 1080);

    QTest::newRow("left edge")   << main << QRect(0, 30, 150, 28);
    QTest::newRow("right edge")  << main << QRect(1900, 30, 150, 28);
    QTest::newRow("bottom edge") << main << QRect(800, 1070, 150, 28);
    QTest::newRow("top edge")    << main << QRect(800, 0, 150, 28);
    QTest::newRow("negative origin") << QRect(-1920, -200, 1920, 1080)
                                     << QRect(-1900, -170, 150, 28);
    QTest::newRow("tiny screen") << QRect(0, 0, 400, 200) << QRect(100, 90, 150, 28);
}

void tst_thumbnail::thePreviewStaysOnTheScreen(){
    QFETCH(QRect, screen);
    QFETCH(QRect, tab);

    const QSize size(272, 227);
    const QRect placed = NodePreview::Place(tab, size, screen, 4);

    QCOMPARE(placed.size(), size);
    QVERIFY2(placed.x() >= screen.x(),
             qPrintable(QStringLiteral("x %1 is left of the screen").arg(placed.x())));
    QVERIFY2(placed.y() >= screen.y(),
             qPrintable(QStringLiteral("y %1 is above the screen").arg(placed.y())));
    if(size.width() <= screen.width())
        QVERIFY(placed.x() + placed.width() <= screen.x() + screen.width());
    if(size.height() <= screen.height())
        QVERIFY(placed.y() + placed.height() <= screen.y() + screen.height());
}

void tst_thumbnail::thePreviewOfAFolderWithNothingToBorrowSaysSo(){
    Theme::ApplyScheme(QStringLiteral("light"));

    NodePreview *preview = NodePreview::Instance();
    const QRect tab(100, 30, 150, 28);

    auto shotOf = [&](bool isDirectory){
        preview->Request(QImage(), QStringLiteral("t"), tab, isDirectory, QPoint(120, 40));
        QImage image(preview->size(), QImage::Format_ARGB32);
        image.fill(Qt::magenta);
        preview->render(&image);
        preview->Dismiss();
        return image;
    };

    const QImage folder = shotOf(true);
    const QImage page = shotOf(false);

    auto has = [](const QImage &image, Theme::Role role){
        const QColor wanted = Theme::Color(role);
        for(int y = 0; y < image.height(); y++)
            for(int x = 0; x < image.width(); x++)
                if(image.pixelColor(x, y) == wanted) return true;
        return false;
    };

    QVERIFY(has(folder, Theme::PreviewPlaceholderDirectory));
    QVERIFY(has(page, Theme::PreviewPlaceholderPage));
    QVERIFY(folder != page);
}

void tst_thumbnail::thePreviewKnowsWhichTabItIsWaitingFor(){
    NodePreview *preview = NodePreview::Instance();
    const QRect tab(100, 30, 150, 28);

    preview->Dismiss();
    QVERIFY(!preview->IsAbout(QStringLiteral("t"), tab));

    preview->Request(QImage(), QStringLiteral("t"), tab, false, QPoint(120, 40));
    QVERIFY(preview->IsAbout(QStringLiteral("t"), tab));
    QVERIFY(!preview->IsAbout(QStringLiteral("u"), tab));
    QVERIFY(!preview->IsAbout(QStringLiteral("t"), tab.translated(150, 0)));

    preview->Dismiss();
    QVERIFY(!preview->IsAbout(QStringLiteral("t"), tab));
}

void tst_thumbnail::thePreviewHeldStaysAwayUntilThePointerMoves(){
    NodePreview *preview = NodePreview::Instance();
    const QRect tab(100, 30, 150, 28);
    const QPoint held(120, 40);

    preview->HoldUntilPointerMoves(held);
    preview->Request(QImage(), QStringLiteral("t"), tab, false, held);
    QVERIFY(!preview->IsAbout(QStringLiteral("t"), tab));

    preview->Request(QImage(), QStringLiteral("t"), tab, false, held + QPoint(1, 0));
    QVERIFY(preview->IsAbout(QStringLiteral("t"), tab));

    preview->Dismiss();
    preview->Request(QImage(), QStringLiteral("t"), tab, false, held);
    QVERIFY(preview->IsAbout(QStringLiteral("t"), tab));
    preview->Dismiss();
}

void tst_thumbnail::thePreviewGoesWhenTheFocusWindowChanges(){
    NodePreview *preview = NodePreview::Instance();
    const QRect tab(100, 30, 150, 28);
    const QPoint pointer(120, 40);

    preview->Request(QImage(), QStringLiteral("t"), tab, false, pointer);
    QVERIFY(preview->IsAbout(QStringLiteral("t"), tab));

    emit qGuiApp->focusWindowChanged(nullptr);
    QVERIFY(!preview->IsAbout(QStringLiteral("t"), tab));

    preview->Request(QImage(), QStringLiteral("t"), tab, false, QCursor::pos());
    QVERIFY(!preview->IsAbout(QStringLiteral("t"), tab));
    preview->Request(QImage(), QStringLiteral("t"), tab, false, QCursor::pos() + QPoint(1, 0));
    QVERIFY(preview->IsAbout(QStringLiteral("t"), tab));
    preview->Dismiss();
}

QTEST_MAIN(tst_thumbnail)
#include "tst_thumbnail.moc"
