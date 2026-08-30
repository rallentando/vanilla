#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QElapsedTimer>
#include <QPainter>
#include <QImage>

#include "application.hpp"
#include "lightnode.hpp"
#include "graphicstableview.hpp"
#include "gadgetsstyle.hpp"

#include "testsupport.hpp"

namespace {

class Booting {
public:
    Booting(){ Node::SetBooting(true);}
    ~Booting(){ Node::SetBooting(false);}
};

const int   NODES = 1000;
const QSize SCREEN = QSize(1920, 1080);

const int    FRAMES = 20;
const double FLOOR_FPS = 20.0;

const QDateTime &Created(){
    static const QDateTime date =
        QDateTime::fromString(QStringLiteral("20260101120000"), NODE_DATETIME_FORMAT);
    return date;
}

QImage Snapshot(int seed){
    QImage image(200, 150, QImage::Format_ARGB32_Premultiplied);
    for(int y = 0; y < image.height(); y++){
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for(int x = 0; x < image.width(); x++)
            line[x] = qRgb((x * 3 + seed) & 255, (y * 5 + seed) & 255, (x + y) & 255);
    }
    return image;
}

ViewNode *MakeDirectory(int count){
    ViewNode *root = new ViewNode();
    Booting booting;
    ViewNode *folder = root->MakeChild();
    folder->SetTitle(QStringLiteral("directory"));
    folder->SetCreateDate(Created());
    folder->SetLastUpdateDate(Created());
    folder->SetLastAccessDate(Created().addSecs(1));
    for(int i = 0; i < count; i++){
        ViewNode *tab = folder->MakeChild();
        tab->SetHoldView(true);
        tab->SetTitle(QStringLiteral("node %1").arg(i));
        tab->SetUrl(QUrl(QStringLiteral("https://example.com/page%1").arg(i)));
        tab->SetCreateDate(Created());
        tab->SetLastUpdateDate(Created());
        tab->SetLastAccessDate(Created().addSecs(1));
        tab->SetImage(Snapshot(i));
    }
    return root;
}

void UseStyle(const QString &name){
    Application::GlobalSettings()[QStringLiteral("gadgets/@Style")] = name;
    GraphicsTableView::LoadSettings();
    Q_ASSERT(GraphicsTableView::GetStyle()->StyleName() == name);
}

}

class tst_gadgetsframe : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void zoomingOutDoesNotDivideByZero_data();
    void zoomingOutDoesNotDivideByZero();
    void aScreenfulOfNodesHoldsTwentyFramesASecond_data();
    void aScreenfulOfNodesHoldsTwentyFramesASecond();
    void scrollingAScreenful_data();
    void scrollingAScreenful();
};

static void StyleData(){
    QTest::addColumn<QString>("style");
    QTest::newRow("GlassStyle") << QStringLiteral("GlassStyle");
    QTest::newRow("FlatStyle")  << QStringLiteral("FlatStyle");
}

void tst_gadgetsframe::zoomingOutDoesNotDivideByZero_data(){ StyleData();}
void tst_gadgetsframe::aScreenfulOfNodesHoldsTwentyFramesASecond_data(){ StyleData();}
void tst_gadgetsframe::scrollingAScreenful_data(){ StyleData();}

void tst_gadgetsframe::initTestCase(){
    TestSupport::SilenceDebugOutput();
    GraphicsTableView::LoadSettings();
}

void tst_gadgetsframe::zoomingOutDoesNotDivideByZero(){
    QFETCH(QString, style);
    UseStyle(style);

    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(0, 0), QSizeF(SCREEN)));

    GraphicsTableView *table = new GraphicsTableView(nullptr);
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = MakeDirectory(8);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(root->GetChildAt(0)->GetChildAt(0));

    QVERIFY2(table->ThumbList_ZoomOut(), "the overview would not zoom out at all.");
    QVERIFY2(table->GetZoomFactor() < 1.0f, "zooming out did not lower the factor.");

    QVERIFY(table->ThumbnailAreaRect().width() > 0);

    table->Deactivate();
    delete root;
}

void tst_gadgetsframe::aScreenfulOfNodesHoldsTwentyFramesASecond(){
    QFETCH(QString, style);
    UseStyle(style);

    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(0, 0), QSizeF(SCREEN)));

    GraphicsTableView *table = new GraphicsTableView(nullptr);
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = MakeDirectory(NODES);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(root->GetChildAt(0)->GetChildAt(0));

    for(int i = 0; i < 40; i++){
        if(!table->ThumbList_ZoomOut()) break;
    }

    const QRectF area = table->ThumbnailAreaRect();
    int onScreen = 0;
    for(int i = 0; i < NODES; i++){
        if(area.intersects(table->ComputeRect(static_cast<const Thumbnail*>(nullptr), i)))
            onScreen++;
    }

    QImage frame(SCREEN, QImage::Format_ARGB32_Premultiplied);

    auto render = [&scene, &frame]() -> qint64 {
        QElapsedTimer timer;
        timer.start();
        QPainter painter(&frame);
        scene.render(&painter,
                     QRectF(QPointF(0, 0), QSizeF(SCREEN)),
                     QRectF(QPointF(0, 0), QSizeF(SCREEN)));
        return timer.nsecsElapsed();
    };

    const double first = double(render()) / 1000000.0;

    QList<qint64> times;
    for(int i = 0; i < FRAMES; i++) times << render();
    std::sort(times.begin(), times.end());

    const double median = double(times[times.length() / 2]) / 1000000.0;
    const double worst  = double(times.last()) / 1000000.0;

    qInfo().noquote()
        << QStringLiteral("\n[%10] %1 nodes in the directory, %2 of them on the screen "
                          "at zoom %3\nfirst frame %4 ms (%5 fps)\n"
                          "sustained: typical %6 ms (%7 fps), worst %8 ms (%9 fps)")
           .arg(NODES).arg(onScreen)
           .arg(double(table->GetZoomFactor()), 0, 'f', 2)
           .arg(first,  0, 'f', 1).arg(1000.0 / first,  0, 'f', 1)
           .arg(median, 0, 'f', 1).arg(1000.0 / median, 0, 'f', 1)
           .arg(worst,  0, 'f', 1).arg(1000.0 / worst,  0, 'f', 1)
           .arg(GraphicsTableView::GetStyle()->StyleName());

    QVERIFY2(onScreen >= NODES * 3 / 4,
             qPrintable(QStringLiteral("only %1 of %2 nodes are on the screen; "
                                       "the frame below is not a screenful.")
                        .arg(onScreen).arg(NODES)));

    QVERIFY2(1000.0 / median >= FLOOR_FPS,
             qPrintable(QStringLiteral("a typical frame of %1 nodes took %2 ms, "
                                       "which is %3 frames a second; %4 is the floor.")
                        .arg(onScreen).arg(median, 0, 'f', 1)
                        .arg(1000.0 / median, 0, 'f', 1)
                        .arg(FLOOR_FPS, 0, 'f', 1)));

    table->Deactivate();
    delete root;
}

void tst_gadgetsframe::scrollingAScreenful(){
    QFETCH(QString, style);
    UseStyle(style);

    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(0, 0), QSizeF(SCREEN)));

    QGraphicsView view(&scene);
    view.setCacheMode(QGraphicsView::CacheBackground);
    view.setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    view.setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing |
                              QGraphicsView::DontSavePainterState);
    view.setFrameShape(QFrame::NoFrame);
    view.resize(SCREEN);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    GraphicsTableView *table = new GraphicsTableView(nullptr);
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = MakeDirectory(NODES);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(root->GetChildAt(0)->GetChildAt(0));
    for(int i = 0; i < 40; i++){
        if(!table->ThumbList_ZoomOut()) break;
    }
    QCoreApplication::processEvents();

    QList<qint64> marks, frames;
    for(int i = 0; i < FRAMES; i++){
        QElapsedTimer timer;
        timer.start();
        table->SetScroll(qreal(i * 3));
        marks << timer.nsecsElapsed();
        timer.restart();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::UpdateRequest);
        QCoreApplication::processEvents();
        frames << timer.nsecsElapsed();
    }
    std::sort(marks.begin(), marks.end());
    std::sort(frames.begin(), frames.end());

    const double mark  = double(marks[marks.length() / 2])   / 1000000.0;
    const double frame = double(frames[frames.length() / 2]) / 1000000.0;

    qInfo().noquote()
        << QStringLiteral("\n[%1] scroll step: marking %2 ms + repaint %3 ms "
                          "= %4 ms (%5 fps)")
           .arg(GraphicsTableView::GetStyle()->StyleName())
           .arg(mark,  0, 'f', 1).arg(frame, 0, 'f', 1)
           .arg(mark + frame, 0, 'f', 1)
           .arg(1000.0 / (mark + frame), 0, 'f', 1);

    QVERIFY2(1000.0 / (mark + frame) >= FLOOR_FPS,
             qPrintable(QStringLiteral("one scroll step of %1 nodes took %2 ms "
                                       "(%3 ms of it before anything was drawn), "
                                       "which is %4 frames a second; %5 is the floor.")
                        .arg(NODES).arg(mark + frame, 0, 'f', 1)
                        .arg(mark, 0, 'f', 1)
                        .arg(1000.0 / (mark + frame), 0, 'f', 1)
                        .arg(FLOOR_FPS, 0, 'f', 1)));

    table->Deactivate();
    delete root;
}

QTEST_MAIN(tst_gadgetsframe)
#include "tst_gadgetsframe.moc"
