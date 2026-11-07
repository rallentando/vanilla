#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QElapsedTimer>
#include <QPainter>
#include <QImage>
#include <QVariantAnimation>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

#include "application.hpp"
#include "lightnode.hpp"
#include "graphicstableview.hpp"
#include "gadgets.hpp"
#include "localview.hpp"
#include "treebank.hpp"
#include <QMenu>
#include "gadgetsstyle.hpp"
#include "thumbnail.hpp"
#include "nodetitle.hpp"

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
        tab->SetImage(Snapshot(i), false);
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
    void aPasteTakesTheSelectionOnlyWhenTheSettingSaysSo();
    void aPasteOnAnEmptyPlaceGoesWhereThePlaceIs();
    void theTableDrawsOnceWhenToldAndNotAgainUntilTold();
    void aClockStartedTwiceCountsEachFrameOnce();
    void applyCollectionSetting();
    void collectionSettingRebuildsVisibleNodes();
    void savedCollectionType_data();
    void savedCollectionType();
    void collectionMenuAndCycle();
    void styleSwitchRestoresOpacity();
    void foldAnimation_data();
    void foldAnimation();
    void unfoldingALargeDirectoryAnimatesOnlyTheScreen_data();
    void unfoldingALargeDirectoryAnimatesOnlyTheScreen();
    void foldedDirectoryGlyph_data();
    void foldedDirectoryGlyph();
    void cardsAnswerTheMouseOnlyWhereTheyAreDrawn_data();
    void cardsAnswerTheMouseOnlyWhereTheyAreDrawn();
    void closeButtonKeepsTheCardItHangsOn_data();
    void closeButtonKeepsTheCardItHangsOn();
    void recursivePrimaryTracksTheDisplayedPage_data();
    void recursivePrimaryTracksTheDisplayedPage();
    void upADirectoryFromWhereTheChosenTabsAre();
    void aLocalFolderGoesUpAsFarAsTheTop();
    void aLocalViewOfItsOwnStaysOnDeactivate();
    void theBarClosesTheTabListButNotALocalFolder();
    void intoADirectoryFromWhereTheChosenTabsAre();
    void initTestCase();
    void theTableLetsGoOfEverythingAboutALeavingNode();
    void anInactiveTableLetsGoAsWell();
    void theTrashDeletesForGoodOnlyWhenTheUserSaysSo();
    void thePageIsDrawnBehindNodesButNotBehindAccessKeys();
    void theBackdropBlurSoftensAnEdgeAndNothingElse();
    void plainStyleIsTheDefault();
    void plainStyleShowsWhatIsBehindAThroughATranslucentWindow();
    void plainStyleHoldsAsManyCardsAsFlatStyle();
    void plainStyleCoversThePageOnlyBehindNodes();
    void plainStyleFillsTheBoxAndRoundsItsCorners();
    void plainStyleCutsALongTitleOnceWithAnEllipsis();
    void plainStyleKeepsTheRingOnTheSmallestCard();
    void plainStyleCutsTheTitleOnceThroughAFold();
    void plainStyleCornersAreAtTheDevicesPixels();

    void zoomingOutDoesNotDivideByZero_data();
    void zoomingOutDoesNotDivideByZero();
    void aScreenfulOfNodesHoldsTwentyFramesASecond_data();
    void aScreenfulOfNodesHoldsTwentyFramesASecond();
    void scrollingAScreenful_data();
    void scrollingAScreenful();
    void aChordOnACardIsTheMapsAndNothingElse();

private:
    static QString KeptAbout(GraphicsTableView *table, const QSet<Node*> &going);

public:
    static bool SwapTranslucent(bool on){
        const bool before = Application::m_TransparentWindowEffective;
        Application::m_TransparentWindowEffective = on;
        return before;
    }
};

class TranslucentWindow {
public:
    explicit TranslucentWindow(bool on = true)
        : m_Before(tst_gadgetsframe::SwapTranslucent(on)) {}
    ~TranslucentWindow(){ tst_gadgetsframe::SwapTranslucent(m_Before);}
private:
    bool m_Before;
};

static const char TRANSLUCENT_ROW[] = "PlainStyle through a translucent window";

static void FrameStyleData(){
    QTest::addColumn<QString>("style");
    QTest::newRow("GlassStyle") << QStringLiteral("GlassStyle");
    QTest::newRow("FlatStyle")  << QStringLiteral("FlatStyle");
    QTest::newRow("PlainStyle") << QStringLiteral("PlainStyle");
    QTest::newRow(TRANSLUCENT_ROW) << QStringLiteral("PlainStyle");
}

static void StyleData(){
    QTest::addColumn<QString>("style");
    QTest::newRow("GlassStyle") << QStringLiteral("GlassStyle");
    QTest::newRow("FlatStyle")  << QStringLiteral("FlatStyle");
    QTest::newRow("PlainStyle") << QStringLiteral("PlainStyle");
}

void tst_gadgetsframe::plainStyleIsTheDefault(){
    auto &settings = Application::GlobalSettings();
    const QString key = QStringLiteral("gadgets/@Style");
    const QVariant before = settings.value(key);
    const auto restore = qScopeGuard([&]{
        if(before.isValid()) settings.setValue(key, before);
        else settings.remove(key);
        GraphicsTableView::LoadSettings();
    });

    settings.remove(key);
    GraphicsTableView::LoadSettings();
    QCOMPARE(GraphicsTableView::GetStyle()->StyleName(), QStringLiteral("PlainStyle"));

    UseStyle(QStringLiteral("FlatStyle"));
    QCOMPARE(GraphicsTableView::GetStyle()->StyleName(), QStringLiteral("FlatStyle"));
}

void tst_gadgetsframe::plainStyleHoldsAsManyCardsAsFlatStyle(){
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new GraphicsTableView(nullptr);
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));
    const auto layout = [&](const QString &style, float zoom){
        UseStyle(style);
        table->m_CurrentThumbnailZoomFactor = zoom;
        int col = 0, line = 0, width = 0, height = 0;
        GraphicsTableView::GetStyle()->ComputeContentsLayout(table, col, line, width, height);
        return QList<int>{col, line, width, height};
    };
    for(const float zoom : {0.25f, 0.5f, 0.8f, 1.0f, 1.5f, 2.0f}){
        const QList<int> flat = layout(QStringLiteral("FlatStyle"), zoom);
        QVERIFY(flat[0] > 0 && flat[1] > 0);
        QCOMPARE(layout(QStringLiteral("PlainStyle"), zoom), flat);
    }
    table->m_CurrentThumbnailZoomFactor = 1.0f;
    UseStyle(QStringLiteral("FlatStyle"));
}

void tst_gadgetsframe::plainStyleCoversThePageOnlyBehindNodes(){
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new GraphicsTableView(nullptr);
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));
    auto *root = MakeDirectory(1);
    UseStyle(QStringLiteral("PlainStyle"));
    GadgetsStyle *style = GraphicsTableView::GetStyle();
    QVERIFY(!table->IsDisplayingNode());
    QVERIFY(!style->CoversViewBehind(table));
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(root->GetChildAt(0)->ToViewNode()->GetChildAt(0));
    QVERIFY(table->IsDisplayingNode());
    QVERIFY(style->CoversViewBehind(table));

    QImage frame(QSize(100, 100), QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::red);
    {
        QPainter painter(&frame);
        table->RenderBackground(&painter);
    }
    QCOMPARE(frame.pixelColor(50, 50), Theme::Color(Theme::PlainSurface));

    for(const QString &other : {QStringLiteral("FlatStyle"), QStringLiteral("GlassStyle")}){
        UseStyle(other);
        QVERIFY(!GraphicsTableView::GetStyle()->CoversViewBehind(table));
    }
    table->Deactivate();
    delete table;
    delete root;
}

void tst_gadgetsframe::plainStyleFillsTheBoxAndRoundsItsCorners(){
    UseStyle(QStringLiteral("PlainStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    class CardTable : public GraphicsTableView {
    public:
        Thumbnail *thumb(Node *node) {
            for(auto *item : childItems())
                if(auto *thumb = dynamic_cast<Thumbnail*>(item))
                    if(thumb->GetNode() == node) return thumb;
            return nullptr;
        }
    };
    auto *table = new CardTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(3);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    QImage page(800, 300, QImage::Format_ARGB32_Premultiplied);
    page.fill(Qt::blue);
    {
        QPainter painter(&page);
        painter.fillRect(QRect(0, 0, 800, 150), Qt::red);
    }
    Node *node = folder->GetChildAt(1);
    node->ToViewNode()->SetImage(page, false);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder->GetChildAt(0));
    Thumbnail *thumb = table->thumb(node);
    QVERIFY(thumb);

    const QRectF cell = thumb->boundingRect();
    const int margin = table->ScaleByDevice(PlainStyle::m_CardMargin);
    const int padding = table->ScaleByDevice(PlainStyle::m_ImagePadding);
    const int title = table->ScaleByDevice(PlainStyle::m_TitleHeight);
    const int inset = margin + padding;
    const QRect box(qRound(cell.left()) + inset, qRound(cell.top()) + inset,
                    qRound(cell.width()) - inset * 2,
                    qRound(cell.height()) - inset - margin - title);
    QVERIFY(box.height() > 20 && box.width() > 20);

    const QColor surface = Theme::Color(Theme::PlainSurface);
    QImage frame(SCREEN, QImage::Format_ARGB32_Premultiplied);
    frame.fill(surface);
    {
        QPainter painter(&frame);
        painter.translate(thumb->pos());
        thumb->paint(&painter, nullptr, nullptr);
    }
    const QPoint offset = thumb->pos().toPoint();
    const auto at = [&](int x, int y){ return frame.pixelColor(QPoint(x, y) + offset); };

    const int edge = table->ScaleByDevice(PlainStyle::m_EdgeWidth);
    QVERIFY(edge > 0);
    const QColor hairline = Theme::Color(Theme::PlainDivider);
    QCOMPARE(at(box.left(), box.center().y()), hairline);
    QCOMPARE(at(box.center().x(), box.top()), hairline);
    QCOMPARE(at(box.left() + edge,  box.top() + box.height() / 4), QColor(Qt::red));
    QCOMPARE(at(box.right() - edge, box.top() + box.height() / 4), QColor(Qt::red));
    QCOMPARE(at(box.center().x(), box.top() + edge), QColor(Qt::red));
    QCOMPARE(at(box.center().x(), box.bottom() - edge), QColor(Qt::blue));
    QCOMPARE(at(box.left(),  box.top()),    surface);
    QCOMPARE(at(box.right(), box.top()),    surface);
    QCOMPARE(at(box.left(),  box.bottom()), surface);
    QCOMPARE(at(box.right(), box.bottom()), surface);
    QCOMPARE(at(box.left() - 1, box.center().y()), surface);

    QImage tall(200, 800, QImage::Format_ARGB32_Premultiplied);
    tall.fill(Qt::blue);
    {
        QPainter painter(&tall);
        painter.fillRect(QRect(0, 0, 200, 400), Qt::red);
    }
    node->ToViewNode()->SetImage(tall, false);
    frame.fill(surface);
    {
        QPainter painter(&frame);
        painter.translate(thumb->pos());
        thumb->paint(&painter, nullptr, nullptr);
    }
    QCOMPARE(at(box.center().x(), box.top() + edge), QColor(Qt::red));
    QCOMPARE(at(box.center().x(), box.bottom() - edge), QColor(Qt::red));
    node->ToViewNode()->SetImage(page, false);

    thumb->setSelected(true);
    frame.fill(surface);
    {
        QPainter painter(&frame);
        painter.translate(thumb->pos());
        thumb->paint(&painter, nullptr, nullptr);
    }
    const QColor plate = Theme::Color(Theme::PlainSelected);
    const QColor accent = Theme::Color(Theme::PlainAccent);
    QCOMPARE(at(box.left() - 1, box.center().y()), plate);
    QCOMPARE(at(box.left(), box.center().y()), accent);
    QCOMPARE(at(box.right(), box.center().y()), accent);
    QCOMPARE(at(box.left(), box.top()), plate);
    QCOMPARE(at(qRound(cell.left()) + margin, qRound(cell.top()) + margin), surface);
    thumb->setSelected(false);

    table->Deactivate();
    delete table;
    delete root;
    UseStyle(QStringLiteral("FlatStyle"));
}

void tst_gadgetsframe::plainStyleCutsALongTitleOnceWithAnEllipsis(){
    ViewNode *root = MakeDirectory(1);
    GraphicsTableView table(nullptr);
    auto *item = new Thumbnail(root->GetChildAt(0), 0, &table);
    Thumbnail &thumb = *item;
    const QFont font = ThumbnailTitleFont();
    const QString title = QString(200, QLatin1Char('x'));
    const int width = 100;

    QCOMPARE(thumb.TitleText(title, font).text(), title);

    const QString cut = thumb.TitleText(title, font, width).text();
    QVERIFY(cut.endsWith(QChar(0x2026)));
    QVERIFY(QFontMetrics(font).horizontalAdvance(cut) <= width);

    QCOMPARE(thumb.TitleText(QStringLiteral("ab"), font, width).text(), QStringLiteral("ab"));
    const QString longer = thumb.TitleText(title, font, width * 2).text();
    QVERIFY(longer.length() > cut.length());
    QCOMPARE(thumb.TitleText(title, font, width).text(), cut);

    delete item;
    delete root;
}

namespace {
    class PlainCardTable : public GraphicsTableView {
    public:
        Thumbnail *thumb(Node *node) {
            for(auto *item : childItems())
                if(auto *thumb = dynamic_cast<Thumbnail*>(item))
                    if(thumb->GetNode() == node) return thumb;
            return nullptr;
        }
    };

    QImage PaintCard(Thumbnail *thumb, const QColor &under, qreal ratio = 1.0){
        QImage frame(SCREEN * ratio, QImage::Format_ARGB32_Premultiplied);
        frame.setDevicePixelRatio(ratio);
        frame.fill(under);
        QPainter painter(&frame);
        painter.translate(thumb->pos());
        thumb->paint(&painter, nullptr, nullptr);
        return frame;
    }
}

void tst_gadgetsframe::plainStyleShowsWhatIsBehindAThroughATranslucentWindow(){
    UseStyle(QStringLiteral("PlainStyle"));
    TranslucentWindow window;
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new PlainCardTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(3);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    Node *node = folder->GetChildAt(1);
    QImage solid(200, 150, QImage::Format_ARGB32_Premultiplied);
    solid.fill(Qt::green);
    node->ToViewNode()->SetImage(solid, false);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder->GetChildAt(0));
    QVERIFY(table->IsDisplayingNode());
    QVERIFY(!GraphicsTableView::GetStyle()->CoversViewBehind(table));

    QImage behind(QSize(100, 100), QImage::Format_ARGB32_Premultiplied);
    behind.fill(Qt::red);
    {
        QPainter painter(&behind);
        table->RenderBackground(&painter);
    }
    const QColor mixed = behind.pixelColor(50, 50);
    QVERIFY(mixed != Theme::Color(Theme::PlainSurface));
    QVERIFY(mixed != QColor(Qt::red));

    Thumbnail *thumb = table->thumb(node);
    QVERIFY(thumb);
    const QRectF cell = thumb->boundingRect();
    const int margin = table->ScaleByDevice(PlainStyle::m_CardMargin);
    const int inset = margin + table->ScaleByDevice(PlainStyle::m_ImagePadding);
    const int title = table->ScaleByDevice(PlainStyle::m_TitleHeight);
    const int edge = table->ScaleByDevice(PlainStyle::m_EdgeWidth);
    const QRect box(qRound(cell.left()) + inset, qRound(cell.top()) + inset,
                    qRound(cell.width()) - inset * 2,
                    qRound(cell.height()) - inset - margin - title);
    const QPoint offset = thumb->pos().toPoint();

    QImage frame = PaintCard(thumb, Qt::red);
    const auto at = [&](int x, int y){ return frame.pixelColor(QPoint(x, y) + offset); };
    QCOMPARE(at(box.left(), box.top()), QColor(Qt::red));
    QCOMPARE(at(box.right(), box.bottom()), QColor(Qt::red));
    QCOMPARE(at(box.center().x(), box.center().y()), QColor(Qt::green));
    QCOMPARE(at(box.left() + edge, box.center().y()), QColor(Qt::green));
    QCOMPARE(at(box.left(), box.center().y()), Theme::Color(Theme::PlainDivider));

    thumb->setSelected(true);
    frame = PaintCard(thumb, Qt::red);
    const QColor plate = at(box.left() - 1, box.center().y());
    QVERIFY(plate != Theme::Color(Theme::PlainSelected));
    QVERIFY(plate != QColor(Qt::red));
    QCOMPARE(at(box.left(), box.center().y()), Theme::Color(Theme::PlainAccent));
    thumb->setSelected(false);

    thumb->SetTransitionRect(cell);
    GraphicsTableView *base = table;
    const int width = base->m_CurrentThumbnailWidth;
    QVERIFY(width >= table->ScaleByDevice(PlainStyle::m_RoundedCellMinWidth));
    base->m_CurrentThumbnailWidth = table->ScaleByDevice(PlainStyle::m_RoundedCellMinWidth) - 1;
    frame = PaintCard(thumb, Qt::red);
    QCOMPARE(at(box.left(), box.top()), Theme::Color(Theme::PlainDivider));
    QCOMPARE(at(box.right(), box.bottom()), Theme::Color(Theme::PlainDivider));
    QCOMPARE(at(box.left(), box.center().y()), Theme::Color(Theme::PlainDivider));
    QCOMPARE(at(box.center().x(), box.center().y()), QColor(Qt::green));
    thumb->setSelected(true);
    frame = PaintCard(thumb, Qt::red);
    const QColor squarePlate = frame.pixelColor(offset + QPoint(qRound(cell.left()) + margin,
                                                                qRound(cell.top()) + margin));
    QVERIFY(squarePlate != QColor(Qt::red));
    QVERIFY(squarePlate != Theme::Color(Theme::PlainSelected));
    thumb->setSelected(false);
    base->m_CurrentThumbnailWidth = width;
    thumb->SetTransitionRect(std::nullopt);

    table->Deactivate();
    delete table;
    delete root;
    UseStyle(QStringLiteral("FlatStyle"));
}

void tst_gadgetsframe::plainStyleKeepsTheRingOnTheSmallestCard(){
    UseStyle(QStringLiteral("PlainStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new PlainCardTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(3);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    Node *node = folder->GetChildAt(1);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(node);
    Thumbnail *thumb = table->thumb(node);
    QVERIFY(thumb);
    QVERIFY(thumb->IsPrimary());
    static_cast<GraphicsTableView*>(table)->m_HoveredItemIndex = -1;
    QVERIFY(!thumb->IsHovered());

    const int inset = table->ScaleByDevice(PlainStyle::m_CardMargin)
                    + table->ScaleByDevice(PlainStyle::m_ImagePadding);
    const int title = table->ScaleByDevice(PlainStyle::m_TitleHeight);
    const int margin = table->ScaleByDevice(PlainStyle::m_CardMargin);
    const int boxHeight = table->ScaleByDevice(PlainStyle::m_CornerRadius) * 2 - 1;
    const QRectF cell(0, 0, 60, inset + boxHeight + title + margin);
    thumb->SetTransitionRect(cell);
    const QImage frame = PaintCard(thumb, Theme::Color(Theme::PlainSurface));
    thumb->SetTransitionRect(std::nullopt);

    const QPoint offset = thumb->pos().toPoint();
    const QColor accent = Theme::Color(Theme::PlainAccent);
    QCOMPARE(frame.pixelColor(offset + QPoint(30, inset)), accent);
    QCOMPARE(frame.pixelColor(offset + QPoint(30, inset + boxHeight - 1)), accent);
    QCOMPARE(frame.pixelColor(offset + QPoint(inset, inset + boxHeight / 2)), accent);
    QCOMPARE(frame.pixelColor(offset + QPoint(59 - inset, inset + boxHeight / 2)), accent);

    table->Deactivate();
    delete table;
    delete root;
    UseStyle(QStringLiteral("FlatStyle"));
}

void tst_gadgetsframe::plainStyleCutsTheTitleOnceThroughAFold(){
    UseStyle(QStringLiteral("PlainStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new PlainCardTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(3);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    Node *node = folder->GetChildAt(1);
    node->SetTitle(QString(300, QLatin1Char('x')));
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder->GetChildAt(0));
    Thumbnail *thumb = table->thumb(node);
    QVERIFY(thumb);

    PaintCard(thumb, Qt::black);
    const int settled = thumb->m_TitleTextElideWidth;
    QVERIFY(settled > 0);
    const QString cut = thumb->m_TitleText.text();
    QVERIFY(cut.endsWith(QChar(0x2026)));

    const QRectF cell = thumb->boundingRect();
    for(const qreal part : {0.9, 0.7, 0.5}){
        thumb->SetTransitionRect(QRectF(cell.topLeft(), cell.size() * part));
        PaintCard(thumb, Qt::black);
        QCOMPARE(thumb->m_TitleTextElideWidth, settled);
        QCOMPARE(thumb->m_TitleText.text(), cut);
    }
    thumb->SetTransitionRect(std::nullopt);

    table->Deactivate();
    delete table;
    delete root;
    UseStyle(QStringLiteral("FlatStyle"));
}

void tst_gadgetsframe::plainStyleCornersAreAtTheDevicesPixels(){
    UseStyle(QStringLiteral("PlainStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new PlainCardTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(3);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    Node *node = folder->GetChildAt(1);
    QImage solid(200, 150, QImage::Format_ARGB32_Premultiplied);
    solid.fill(Qt::red);
    node->ToViewNode()->SetImage(solid, false);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder->GetChildAt(0));
    Thumbnail *thumb = table->thumb(node);
    QVERIFY(thumb);

    const QImage frame = PaintCard(thumb, Theme::Color(Theme::PlainSurface), 2.0);
    const int inset = table->ScaleByDevice(PlainStyle::m_CardMargin)
                    + table->ScaleByDevice(PlainStyle::m_ImagePadding);
    const int radius = table->ScaleByDevice(PlainStyle::m_CornerRadius);
    const QRectF cell = thumb->boundingRect();
    const QPoint corner = (thumb->pos().toPoint()
                           + QPoint(qRound(cell.left()) + inset, qRound(cell.top()) + inset)) * 2;
    bool finer = false;
    for(int y = 0; y < radius * 2; y += 2){
        for(int x = 0; x < radius * 2; x += 2){
            const QRgb first = frame.pixel(corner + QPoint(x, y));
            if(frame.pixel(corner + QPoint(x + 1, y)) != first
               || frame.pixel(corner + QPoint(x, y + 1)) != first
               || frame.pixel(corner + QPoint(x + 1, y + 1)) != first)
                finer = true;
        }
    }
    QVERIFY(finer);

    table->Deactivate();
    delete table;
    delete root;
    UseStyle(QStringLiteral("FlatStyle"));
}

void tst_gadgetsframe::zoomingOutDoesNotDivideByZero_data(){ StyleData();}
void tst_gadgetsframe::aScreenfulOfNodesHoldsTwentyFramesASecond_data(){ FrameStyleData();}
void tst_gadgetsframe::scrollingAScreenful_data(){ FrameStyleData();}
void tst_gadgetsframe::foldAnimation_data(){ StyleData();}
void tst_gadgetsframe::unfoldingALargeDirectoryAnimatesOnlyTheScreen_data(){ StyleData();}
void tst_gadgetsframe::foldedDirectoryGlyph_data(){ StyleData();}
void tst_gadgetsframe::cardsAnswerTheMouseOnlyWhereTheyAreDrawn_data(){ StyleData();}
void tst_gadgetsframe::closeButtonKeepsTheCardItHangsOn_data(){ StyleData();}

void tst_gadgetsframe::unfoldingALargeDirectoryAnimatesOnlyTheScreen(){
    QFETCH(QString, style);
    UseStyle(style);
    class FoldTable : public GraphicsTableView {
    public:
        using GraphicsTableView::OnFoldedChanged;
        Node *displayed = nullptr;
        Node *DisplayedViewNode() const override { return displayed; }
        NodeCollectionType GetNodeCollectionType() const override { return Foldable; }
    };
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new FoldTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(300);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    table->displayed = folder->GetChildAt(0);
    folder->SetFolded(true);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder);
    auto *animation = table->findChild<QVariantAnimation*>(QStringLiteral("foldAnimation"));
    QVERIFY(animation);

    folder->SetFolded(false);
    table->OnFoldedChanged(NodeList() << folder);
    QCOMPARE(animation->state(), QAbstractAnimation::Running);
    animation->setCurrentTime(0);
    QList<AbstractNodeItem*> appearing;
    for(auto *item : table->childItems())
        if(auto *node = dynamic_cast<AbstractNodeItem*>(item))
            if(node->isVisible() && node->opacity() == 0) appearing << node;
    QVERIFY(!appearing.isEmpty());

    animation->setCurrentTime(animation->duration());
    const QRectF screen = QRectF(QPointF(), QSizeF(SCREEN));
    int offscreen = 0;
    for(auto *item : table->childItems())
        if(auto *node = dynamic_cast<AbstractNodeItem*>(item))
            if(node->isVisible() && !node->boundingRect().intersects(screen)) ++offscreen;
    QVERIFY(offscreen > 0);
    for(auto *node : appearing)
        QVERIFY2(node->boundingRect().intersects(screen),
                 qPrintable(node->GetNode()->GetTitle()));

    table->Deactivate();
    delete root;
}

void tst_gadgetsframe::foldAnimation(){
    QFETCH(QString, style);
    UseStyle(style);
    class FoldTable : public GraphicsTableView {
    public:
        Node *displayed = nullptr;
        NodeCollectionType type = Foldable;
        Node *DisplayedViewNode() const override { return displayed; }
        NodeCollectionType GetNodeCollectionType() const override { return type; }
        Thumbnail *thumb(Node *node) {
            for(auto *item : childItems())
                if(auto *thumb = dynamic_cast<Thumbnail*>(item))
                    if(thumb->GetNode() == node) return thumb;
            return nullptr;
        }
        NodeTitle *title(Node *node) {
            for(auto *item : childItems())
                if(auto *title = dynamic_cast<NodeTitle*>(item))
                    if(title->GetNode() == node) return title;
            return nullptr;
        }
    };
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new FoldTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(12);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    ViewNode *tail;
    ViewNode *deepPage;
    {
        Booting booting;
        tail = root->MakeChild();
        tail->SetHoldView(true);
        tail->SetTitle(QStringLiteral("tail"));
        auto *inner = folder->MakeChild();
        inner->SetFolded(false);
        inner->SetTitle(QStringLiteral("inner"));
        deepPage = inner->MakeChild();
        deepPage->SetHoldView(true);
        deepPage->SetTitle(QStringLiteral("deep page"));
    }
    auto *page = folder->GetChildAt(0);
    table->displayed = page;
    folder->SetFolded(false);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder);
    table->SetCurrent(folder);
    const qreal titleOpacity = table->title(page)->opacity();
    const qreal thumbOpacity = table->thumb(page)->opacity();
    auto item = [&](Node *node) -> AbstractNodeItem* {
        return style != QStringLiteral("GlassStyle") ?
            static_cast<AbstractNodeItem*>(table->thumb(node)) : table->title(node);
    };
    folder->SetFolded(true);
    table->SetCurrent(folder);
    const QRectF foldedTail = item(tail)->boundingRect();
    auto *animation = table->findChild<QVariantAnimation*>(QStringLiteral("foldAnimation"));
    QVERIFY(animation);
    auto ghosts = [&]{
        int count = 0;
        for(auto *item : table->childItems())
            if(dynamic_cast<QGraphicsPixmapItem*>(item)) ++count;
        return count;
    };
    auto toggle = [&](bool folded){
        folder->SetFolded(folded);
        table->OnFoldedChanged(NodeList() << folder);
        if(animation->state() == QAbstractAnimation::Running){
            QCOMPARE(animation->currentTime(), 16);
            animation->setCurrentTime(0);
        }
    };
    auto finish = [&]{ animation->setCurrentTime(animation->duration()); };
    auto clean = [&]{
        return ghosts() == 0 && animation->state() == QAbstractAnimation::Stopped &&
               table->title(page)->opacity() == titleOpacity && table->thumb(page)->opacity() == thumbOpacity;
    };
    auto render = [&](const QString &name){
        QImage image(SCREEN, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            scene.render(&painter);
        }
        const QString directory = qEnvironmentVariable("VANILLA_TEST_ARTIFACTS");
        if(!directory.isEmpty())
            image.save(QDir(directory).filePath(style + QLatin1Char('-') + name + QStringLiteral(".png")));
        return image;
    };
    const QImage foldedFrame = render(QStringLiteral("folded"));

    toggle(false);
    QCOMPARE(animation->state(), QAbstractAnimation::Running);
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), page);
    QCOMPARE(item(page)->opacity(), 0.0);
    QCOMPARE(table->thumb(page)->boundingRect().center(), table->thumb(folder)->boundingRect().center());
    QCOMPARE(table->thumb(deepPage)->boundingRect().center(), table->thumb(folder)->boundingRect().center());
    const bool titles = table->NodeTitleAreaRect().isValid();
    const QRectF startTitle = table->title(page)->boundingRect();
    const QRectF startDeep = table->title(deepPage)->boundingRect();
    if(titles){
        QCOMPARE(startTitle.center().y(), table->title(folder)->boundingRect().center().y());
        QCOMPARE(startDeep.center().y(), table->title(folder)->boundingRect().center().y());
        QCOMPARE(startTitle.height(), 1.0);
    }
    QCOMPARE(item(tail)->boundingRect(), foldedTail);
    animation->setCurrentTime(60);
    const QRectF middleTail = item(tail)->boundingRect();
    const QRectF middlePage = item(page)->boundingRect();
    const QRectF middleTitle = table->title(page)->boundingRect();
    const QImage unfoldingFrame = render(QStringLiteral("unfolding"));
    QVERIFY(middleTail.top() > foldedTail.top());
    QVERIFY(item(page)->opacity() > 0 && item(page)->opacity() < 1);
    QVERIFY(table->thumb(page)->opacity() > 0 && table->thumb(page)->opacity() < 1);
    finish();
    QVERIFY(clean());
    const QRectF unfoldedTail = item(tail)->boundingRect();
    const QRectF unfoldedPage = item(page)->boundingRect();
    const QImage unfoldedFrame = render(QStringLiteral("unfolded"));
    QVERIFY(unfoldingFrame != foldedFrame);
    QVERIFY(unfoldingFrame != unfoldedFrame);
    QVERIFY(middleTail.top() < unfoldedTail.top());
    QVERIFY(middlePage.height() < unfoldedPage.height());
    const QRectF unfoldedTitle = table->title(page)->boundingRect();
    QCOMPARE(startTitle.left(), unfoldedTitle.left());
    QCOMPARE(startTitle.width(), unfoldedTitle.width());
    QCOMPARE(startDeep.left(), table->title(deepPage)->boundingRect().left());
    QCOMPARE(startDeep.width(), table->title(deepPage)->boundingRect().width());
    QCOMPARE(middleTitle.left(), unfoldedTitle.left());
    QCOMPARE(middleTitle.width(), unfoldedTitle.width());
    if(titles) QVERIFY(middleTitle.height() < unfoldedTitle.height());

    toggle(true);
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), folder);
    QVERIFY(!item(page)->isVisible());
    QVERIFY(ghosts() > 0);
    animation->setCurrentTime(60);
    const QImage foldingFrame = render(QStringLiteral("folding"));
    QVERIFY(foldingFrame != foldedFrame);
    QVERIFY(foldingFrame != unfoldedFrame);
    QVERIFY(item(tail)->boundingRect().top() > foldedTail.top());
    QVERIFY(item(tail)->boundingRect().top() < unfoldedTail.top());
    for(auto *item : table->childItems()){
        if(auto *ghost = dynamic_cast<QGraphicsPixmapItem*>(item)){
            QVERIFY(ghost->opacity() > 0 && ghost->opacity() < 1);
            QVERIFY(ghost->transform().m22() > 0 && ghost->transform().m22() < 1);
            const bool title = ghost->boundingRect().left() >= table->NodeTitleAreaRect().left();
            if(title) QVERIFY(qAbs(ghost->transform().m11() - 1) < 0.01);
            else QVERIFY(ghost->transform().m11() > 0 && ghost->transform().m11() < 1);
            QCOMPARE(ghost->acceptedMouseButtons(), Qt::NoButton);
        }
    }
    animation->setCurrentTime(animation->duration() - 1);
    for(auto *child : table->childItems()){
        if(auto *ghost = dynamic_cast<QGraphicsPixmapItem*>(child)){
            const bool title = ghost->boundingRect().left() >= table->NodeTitleAreaRect().left();
            const QPointF target = title ? table->title(folder)->boundingRect().center() :
                                           table->thumb(folder)->boundingRect().center();
            const QRectF shown = ghost->mapRectToParent(ghost->boundingRect());
            if(title){
                QVERIFY(qAbs(shown.center().y() - target.y()) < 0.01);
                QVERIFY(qAbs(shown.left() - ghost->offset().x()) < 0.01);
                QVERIFY(qAbs(shown.width() - ghost->boundingRect().width()) < 0.01);
            } else {
                QVERIFY(QLineF(shown.center(), target).length() < 0.01);
            }
        }
    }
    finish();
    QVERIFY(clean());
    QCOMPARE(item(tail)->boundingRect(), foldedTail);

    toggle(false);
    animation->setCurrentTime(45);
    toggle(true);
    animation->setCurrentTime(45);
    toggle(false);
    QCOMPARE(ghosts(), 0);
    finish();
    QVERIFY(clean());
    QCOMPARE(item(tail)->boundingRect(), unfoldedTail);
    QCOMPARE(item(page)->boundingRect(), unfoldedPage);

    toggle(true);
    table->ThumbList_RefreshNoScroll();
    QVERIFY(clean());
    QCOMPARE(item(tail)->boundingRect(), foldedTail);
    toggle(false);
    table->SetScroll(table->GetScroll());
    QVERIFY(clean());
    QCOMPARE(item(page)->boundingRect(), unfoldedPage);
    toggle(true);
    table->Resize(SCREEN);
    QVERIFY(clean());
    toggle(false);
    table->Deactivate();
    QVERIFY(clean());
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder);
    table->type = GraphicsTableView::Recursive;
    toggle(true);
    QVERIFY(clean());
    QVERIFY(item(page)->isVisible());
    table->type = GraphicsTableView::Foldable;
    table->ThumbList_RefreshNoScroll();
    toggle(false);
    QCOMPARE(animation->state(), QAbstractAnimation::Running);
    finish();
    toggle(true);
    QVERIFY(ghosts() > 0);
    folder->RemoveChild(page);
    page->Delete();
    table->displayed = nullptr;
    animation->setCurrentTime(60);
    QImage frame(SCREEN, QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::transparent);
    {
        QPainter painter(&frame);
        scene.render(&painter);
    }
    delete table;
    delete root;
}

void tst_gadgetsframe::aChordOnACardIsTheMapsAndNothingElse(){
    class ChordTable : public GraphicsTableView {
    public:
        int taken = 0;
        int menus = 0;
        NodeCollectionType GetNodeCollectionType() const override { return Foldable; }
        QMenu *CreateNodeMenu() override { ++menus; return nullptr; }
        bool TakeMappedPress(QGraphicsSceneMouseEvent *ev) override {
            ++taken;
            for(Qt::MouseButton button : { Qt::LeftButton, Qt::RightButton })
                if((ev->buttons() | ev->button()) & button) m_TakenButtons.Press(button, true);
            ev->setAccepted(true);
            return true;
        }
        Thumbnail *thumb(Node *node) {
            for(auto *item : childItems())
                if(auto *thumb = dynamic_cast<Thumbnail*>(item))
                    if(thumb->GetNode() == node) return thumb;
            return nullptr;
        }
    };
    UseStyle(QStringLiteral("FlatStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new ChordTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(3);
    auto *page = root->GetChildAt(0)->ToViewNode();
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(page);
    Thumbnail *thumb = table->thumb(page);
    QVERIFY(thumb);
    const QPointF at = thumb->sceneBoundingRect().center();
    const auto send = [&](QEvent::Type type, Qt::MouseButton button, Qt::MouseButtons held){
        QGraphicsSceneMouseEvent ev(type);
        ev.setScenePos(at);
        ev.setPos(table->mapFromScene(at));
        ev.setScreenPos(at.toPoint());
        ev.setButton(button);
        ev.setButtons(held);
        ev.setAccepted(false);
        QCoreApplication::sendEvent(&scene, &ev);
    };

    send(QEvent::GraphicsSceneMousePress, Qt::RightButton, Qt::RightButton);
    QVERIFY(scene.mouseGrabberItem() == thumb);
    QCOMPARE(table->taken, 0);
    send(QEvent::GraphicsSceneMousePress, Qt::LeftButton, Qt::RightButton | Qt::LeftButton);
    QCOMPARE(table->taken, 1);
    send(QEvent::GraphicsSceneMouseRelease, Qt::LeftButton, Qt::RightButton);
    send(QEvent::GraphicsSceneMouseRelease, Qt::RightButton, Qt::NoButton);
    QCOMPARE(table->menus, 0);

    send(QEvent::GraphicsSceneMousePress, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::GraphicsSceneMousePress, Qt::RightButton, Qt::LeftButton | Qt::RightButton);
    QCOMPARE(table->taken, 2);
    send(QEvent::GraphicsSceneMouseRelease, Qt::RightButton, Qt::LeftButton);
    QCOMPARE(table->menus, 0);
    send(QEvent::GraphicsSceneMouseRelease, Qt::LeftButton, Qt::NoButton);

    send(QEvent::GraphicsSceneMousePress, Qt::RightButton, Qt::RightButton);
    QCOMPARE(table->taken, 2);
    send(QEvent::GraphicsSceneMouseRelease, Qt::RightButton, Qt::NoButton);
    QCOMPARE(table->menus, 1);
    delete table;
    delete root;
}

void tst_gadgetsframe::foldedDirectoryGlyph(){
    QFETCH(QString, style);
    UseStyle(style);
    class GlyphTable : public GraphicsTableView {
    public:
        NodeCollectionType type = Foldable;
        NodeCollectionType GetNodeCollectionType() const override { return type; }
        Thumbnail *thumb(Node *node) {
            for(auto *item : childItems())
                if(auto *thumb = dynamic_cast<Thumbnail*>(item))
                    if(thumb->GetNode() == node) return thumb;
            return nullptr;
        }
    };
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new GlyphTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(3);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    table->Activate(GraphicsTableView::ViewTree);
    auto card = [&]{
        table->SetCurrent(folder);
        Thumbnail *thumb = table->thumb(folder);
        if(!thumb) return QImage();
        const QRect rect = thumb->sceneBoundingRect().toAlignedRect();
        QImage image(rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        scene.render(&painter, QRectF(QPointF(), rect.size()), rect);
        return image;
    };
    folder->SetFolded(true);
    const QImage folded = card();
    folder->SetFolded(false);
    const QImage unfolded = card();
    table->type = GraphicsTableView::Recursive;
    const QImage plain = card();
    QVERIFY(!folded.isNull());
    QCOMPARE(unfolded.size(), folded.size());
    QCOMPARE(plain.size(), folded.size());
    QVERIFY(folded != unfolded);
    QVERIFY(folded != plain);
    QVERIFY(unfolded != plain);
    delete table;
    delete root;
}

void tst_gadgetsframe::cardsAnswerTheMouseOnlyWhereTheyAreDrawn(){
    QFETCH(QString, style);
    UseStyle(style);
    class HitTable : public GraphicsTableView {
    public:
        Thumbnail *thumb(Node *node) {
            for(auto *item : childItems())
                if(auto *thumb = dynamic_cast<Thumbnail*>(item))
                    if(thumb->GetNode() == node) return thumb;
            return nullptr;
        }
    };
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new HitTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(3);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder->GetChildAt(1));
    Thumbnail *thumb = table->thumb(folder->GetChildAt(1));
    QVERIFY(thumb);
    const QRectF cell = thumb->boundingRect();
    const QRectF hit = thumb->shape().boundingRect();
    QVERIFY(cell.contains(hit));
    if(style == QStringLiteral("FlatStyle")){
        const qreal dx = table->ScaleByDevice(FlatStyle::m_ThumbnailPaddingX);
        const qreal dy = table->ScaleByDevice(FlatStyle::m_ThumbnailPaddingY);
        QVERIFY(dx > 0 && dy > 0);
        QCOMPARE(hit, cell.adjusted(dx, dy, -dx, -dy));
    } else if(style == QStringLiteral("PlainStyle")){
        const qreal margin = table->ScaleByDevice(PlainStyle::m_CardMargin);
        QVERIFY(margin > 0);
        QCOMPARE(hit, cell.adjusted(margin, margin, -margin, -margin));
    } else {
        QCOMPARE(hit, cell);
    }
    const QPointF centre = thumb->mapToScene(cell.center());
    const QPointF corner = thumb->mapToScene(cell.topLeft() + QPointF(1, 1));
    QCOMPARE(scene.itemAt(centre, QTransform()), static_cast<QGraphicsItem*>(thumb));
    QGraphicsItem *atCorner = scene.itemAt(corner, QTransform());
    if(style != QStringLiteral("GlassStyle")){
        QCOMPARE(atCorner, static_cast<QGraphicsItem*>(table));
    } else {
        QCOMPARE(atCorner, static_cast<QGraphicsItem*>(thumb));
    }
    delete table;
    delete root;
}

void tst_gadgetsframe::closeButtonKeepsTheCardItHangsOn(){
    QFETCH(QString, style);
    UseStyle(style);
    QVERIFY(GraphicsTableView::EnableCloseButton());
    class HoverTable : public GraphicsTableView {
    public:
        Thumbnail *thumb(Node *node) {
            for(auto *item : childItems())
                if(auto *thumb = dynamic_cast<Thumbnail*>(item))
                    if(thumb->GetNode() == node) return thumb;
            return nullptr;
        }
        CloseButton *closeButton() {
            for(auto *item : childItems())
                if(auto *button = dynamic_cast<CloseButton*>(item)) return button;
            return nullptr;
        }
        Node *hoveredNode() const { return GetHoveredNode(); }
    };
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new HoverTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(3);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    Node *node = folder->GetChildAt(1);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(node);
    Thumbnail *thumb = table->thumb(node);
    QVERIFY(thumb);
    CloseButton *button = table->closeButton();
    QVERIFY(button);
    auto hover = [&](const QPointF &scenePos){
        QGraphicsSceneHoverEvent ev(QEvent::GraphicsSceneHoverMove);
        ev.setScenePos(scenePos);
        QApplication::sendEvent(&scene, &ev);
    };
    const QRectF cell = thumb->boundingRect();
    hover(thumb->mapToScene(cell.center()));
    QCOMPARE(table->hoveredNode(), node);
    QVERIFY(button->isVisible());
    const QRectF buttonRect = button->boundingRect();
    QVERIFY(cell.contains(buttonRect));
    hover(QPointF(buttonRect.center().x(), cell.top() + 1));
    if(style != QStringLiteral("GlassStyle")){
        QCOMPARE(table->hoveredNode(), static_cast<Node*>(nullptr));
    } else {
        QCOMPARE(table->hoveredNode(), node);
    }
    hover(buttonRect.center());
    QCOMPARE(button->GetState(), GraphicsButton::Hovered);
    QCOMPARE(table->hoveredNode(), node);
    delete table;
    delete root;
}

void tst_gadgetsframe::styleSwitchRestoresOpacity(){
    class FoldTable : public GraphicsTableView {
    public:
        NodeCollectionType GetNodeCollectionType() const override { return Foldable; }
    };
    UseStyle(QStringLiteral("GlassStyle"));
    QGraphicsScene scene;
    auto *table = new FoldTable;
    scene.addItem(table);
    table->Resize(SCREEN);
    auto *root = MakeDirectory(2);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    folder->SetFolded(false);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder);
    table->ThumbList_RefreshNoScroll();
    QList<AbstractNodeItem*> cached;
    for(auto *child : table->childItems()){
        if(auto *item = dynamic_cast<AbstractNodeItem*>(child))
            if(item->GetNode() == folder->GetChildAt(0)) cached.append(item);
    }
    QCOMPARE(cached.size(), 2);
    for(auto *item : cached) QCOMPARE(item->opacity(), 0.8);
    for(const QString &style : {QStringLiteral("FlatStyle"), QStringLiteral("PlainStyle"), QStringLiteral("GlassStyle")}){
        UseStyle(style);
        table->ThumbList_RefreshNoScroll();
        for(auto *item : cached)
            QCOMPARE(item->opacity(), style != QStringLiteral("GlassStyle") ? 1.0 : 0.8);
    }
    folder->SetFolded(true);
    table->OnFoldedChanged(NodeList() << folder);
    folder->SetFolded(false);
    table->OnFoldedChanged(NodeList() << folder);
    auto *animation = table->findChild<QVariantAnimation*>(QStringLiteral("foldAnimation"));
    QVERIFY(animation);
    animation->setCurrentTime(60);
    for(auto *item : cached) QVERIFY(item->opacity() < 0.8);
    UseStyle(QStringLiteral("FlatStyle"));
    table->ThumbList_RefreshNoScroll();
    QCOMPARE(animation->state(), QAbstractAnimation::Stopped);
    for(auto *item : cached) QCOMPARE(item->opacity(), 1.0);
    QTest::qWait(200);
    for(auto *item : cached) QCOMPARE(item->opacity(), 1.0);
    delete table;
    delete root;
}

void tst_gadgetsframe::initTestCase(){
    TestSupport::SilenceDebugOutput();
    GraphicsTableView::LoadSettings();
    TreeBank::Initialize();
}

void tst_gadgetsframe::applyCollectionSetting(){
    class Table : public Gadgets {
    public:
        int refreshes = 0;
        bool ThumbList_Refresh() override { ++refreshes; return true; }
    };
    const QString key = QStringLiteral("gadgets/thumblist/@NodeCollectionType");
    auto &settings = Application::GlobalSettings();
    const QVariant before = settings.value(key);
    const auto restore = qScopeGuard([&]{
        if(before.isValid()) settings.setValue(key, before);
        else settings.remove(key);
        GraphicsTableView::LoadSettings();
    });
    for(auto mode : {GraphicsTableView::ViewTree, GraphicsTableView::TrashTree,
                     GraphicsTableView::AccessKey}){
        for(bool visible : {false, true}){
            Table table;
            table.m_DisplayType = mode;
            table.setVisible(visible);
            table.SetStat({QStringLiteral("2"), QStringLiteral("0.75")});
            for(auto type : {GraphicsTableView::Foldable, GraphicsTableView::Flat,
                             GraphicsTableView::Recursive}){
                settings.setValue(key, GraphicsTableView::NodeCollectionTypeName(type));
                GraphicsTableView::LoadSettings();
                table.refreshes = 0;
                table.ApplyNodeCollectionTypeSetting();
                QCOMPARE(table.GetStat().first().toInt(), int(type));
                QCOMPARE(table.GetZoomFactor(), 0.75f);
                QCOMPARE(table.refreshes, visible && mode != GraphicsTableView::AccessKey ? 1 : 0);
                QCOMPARE(table.GetNodeCollectionType(), !visible || mode == GraphicsTableView::AccessKey
                         ? GraphicsTableView::Flat : type);
                Gadgets restored;
                restored.SetStat(table.GetStat());
                QCOMPARE(restored.GetStat(), table.GetStat());
            }
        }
    }
}

void tst_gadgetsframe::collectionSettingRebuildsVisibleNodes(){
    class Table : public Gadgets {
    public:
        int count() const { return m_DisplayThumbnails.size(); }
    };
    const QString key = QStringLiteral("gadgets/thumblist/@NodeCollectionType");
    auto &settings = Application::GlobalSettings();
    const QVariant before = settings.value(key);
    const auto restore = qScopeGuard([&]{
        if(before.isValid()) settings.setValue(key, before);
        else settings.remove(key);
        GraphicsTableView::LoadSettings();
    });
    auto *root = MakeDirectory(2);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    folder->SetFolded(false);
    {
        Table table;
        table.Resize(SCREEN);
        table.m_DisplayType = GraphicsTableView::ViewTree;
        table.show();
        table.SetStat({"0", "1"});
        table.SetCurrent(folder);
        QCOMPARE(table.count(), 1);
        settings.setValue(key, "Recursive");
        GraphicsTableView::LoadSettings();
        table.ApplyNodeCollectionTypeSetting();
        QCOMPARE(table.count(), 3);
        folder->SetFolded(true);
        settings.setValue(key, "Foldable");
        GraphicsTableView::LoadSettings();
        table.ApplyNodeCollectionTypeSetting();
        QCOMPARE(table.count(), 1);
    }
    delete root;
}

void tst_gadgetsframe::savedCollectionType_data(){
    QTest::addColumn<QStringList>("saved");
    QTest::addColumn<int>("expected");
    for(const QString &value : { QStringLiteral("0"), QStringLiteral("2"),
                                QStringLiteral("3"), QStringLiteral("1"),
                                QStringLiteral("-1"), QStringLiteral("99"),
                                QStringLiteral("invalid") }){
        const int expected = value == QStringLiteral("2") ? 2
                           : value == QStringLiteral("3") ? 3 : 0;
        QTest::newRow(qPrintable(value + QStringLiteral("-two-fields")))
            << (QStringList() << value << QStringLiteral("0.75")) << expected;
        QTest::newRow(qPrintable(value + QStringLiteral("-legacy-three-fields")))
            << (QStringList() << value << QStringLiteral("99") << QStringLiteral("0.75")) << expected;
    }
}

void tst_gadgetsframe::savedCollectionType(){
    QFETCH(QStringList, saved);
    QFETCH(int, expected);
    QCOMPARE(static_cast<int>(GraphicsTableView::Flat), 0);
    QCOMPARE(static_cast<int>(GraphicsTableView::Recursive), 2);
    QCOMPARE(static_cast<int>(GraphicsTableView::Foldable), 3);
    Gadgets table(nullptr);
    table.SetStat(saved);
    QCOMPARE(table.GetStat().first().toInt(), expected);
    QCOMPARE(table.GetZoomFactor(), 0.75f);
    const QStringList normalized = table.GetStat();
    table.SetStat(QStringList());
    QCOMPARE(table.GetStat(), normalized);
    table.SetStat(normalized);
    QCOMPARE(table.GetStat(), normalized);
}

void tst_gadgetsframe::collectionMenuAndCycle(){
    class Table : public GraphicsTableView {
    public:
        Table(){ m_DisplayType = ViewTree; }
        NodeCollectionType type = Flat;
        int refreshes = 0;
        NodeCollectionType GetNodeCollectionType() const override { return type; }
        void SetNodeCollectionType(NodeCollectionType value) override { type = value; }
        bool ThumbList_Refresh() override { ++refreshes; return true; }
        using GraphicsTableView::CreateNodeCollectionTypeMenu;
        using GraphicsTableView::ThumbList_SwitchNodeCollectionType;
        using GraphicsTableView::ThumbList_SwitchNodeCollectionTypeReverse;
    } table;
    for(auto expected : { GraphicsTableView::Recursive, GraphicsTableView::Foldable,
                          GraphicsTableView::Flat }){
        QVERIFY(table.ThumbList_SwitchNodeCollectionType());
        QCOMPARE(table.type, expected);
    }
    for(auto expected : { GraphicsTableView::Foldable, GraphicsTableView::Recursive,
                          GraphicsTableView::Flat }){
        QVERIFY(table.ThumbList_SwitchNodeCollectionTypeReverse());
        QCOMPARE(table.type, expected);
    }
    QCOMPARE(table.refreshes, 6);
    QMenu parent;
    QMenu *menu = table.CreateNodeCollectionTypeMenu(&parent);
    QCOMPARE(menu->actions().size(), 3);
    const QList<int> values = { 0, 2, 3 };
    for(int i = 0; i < values.size(); ++i){
        QAction *action = menu->actions().at(i);
        QCOMPARE(action->data().toInt(), values.at(i));
        QCOMPARE(action->isChecked(), i == 0);
    }
    for(QAction *action : menu->actions()){
        action->trigger();
        QCOMPARE(static_cast<int>(table.type), action->data().toInt());
    }
    QCOMPARE(table.refreshes, 9);
}

namespace {
    void TakeOutOfTree(Node *nd){
        nd->GetParent()->RemoveChild(nd);
        nd->SetParent(nullptr);
    }
}

QString tst_gadgetsframe::KeptAbout(GraphicsTableView *table, const QSet<Node*> &going){
        QStringList found;
        for(Node *nd : table->m_ThumbnailCache.keys()) if(going.contains(nd)) found << QStringLiteral("thumbnail cache");
        for(Node *nd : table->m_NodeTitleCache.keys()) if(going.contains(nd)) found << QStringLiteral("title cache");
        for(Thumbnail *t : table->m_DisplayThumbnails) if(going.contains(t->GetNode())) found << QStringLiteral("display");
        for(NodeTitle *t : table->m_DisplayNodeTitles) if(going.contains(t->GetNode())) found << QStringLiteral("display titles");
        for(Node *nd : GraphicsTableView::m_NodesRegister) if(going.contains(nd)) found << QStringLiteral("register");
        if(going.contains(table->m_CurrentNode)) found << QStringLiteral("current");
        if(going.contains(table->m_DummyViewNode->GetParent())) found << QStringLiteral("empty place");
        if(going.contains(table->m_CloseButton->GetNode())) found << QStringLiteral("close button");
        if(going.contains(table->m_CloneButton->GetNode())) found << QStringLiteral("clone button");
        if(going.contains(table->m_SoundButton->GetNode())) found << QStringLiteral("sound button");
        if(going.contains(table->m_InPlaceNotifier->GetNode())) found << QStringLiteral("notifier");
        return found.join(QStringLiteral(", "));
}

void tst_gadgetsframe::theTableLetsGoOfEverythingAboutALeavingNode(){
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(0, 0), QSizeF(SCREEN)));
    GraphicsTableView *table = new GraphicsTableView(nullptr);
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = MakeDirectory(6);
    Node *folder = root->GetChildAt(0);
    Node *current = folder->GetChildAt(2);
    Node *hovered = folder->GetChildAt(3);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(current);

    Thumbnail *thumb = table->m_ThumbnailCache.value(hovered);
    QVERIFY(thumb);
    table->m_CloseButton->SetItem(thumb);
    table->m_CloneButton->SetItem(thumb);
    table->m_SoundButton->SetItem(thumb);
    table->m_InPlaceNotifier->SetNode(hovered);
    GraphicsTableView::m_NodesRegister << current << hovered;

    const QSet<Node*> going = QSet<Node*>() << current << hovered;
    TakeOutOfTree(current);
    TakeOutOfTree(hovered);
    const int stays = folder->ChildrenLength();

    table->LetGoOf(going, folder);
    QCOMPARE(KeptAbout(table, going), QString());
    QVERIFY(table->m_CurrentNode == table->m_DummyViewNode);
    QVERIFY(table->m_DummyViewNode->GetParent() == folder);
    QCOMPARE(table->m_DisplayThumbnails.length(), stays);

    delete current;
    delete hovered;
    QImage image(SCREEN, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();

    table->Deactivate();
    GraphicsTableView::m_NodesRegister.clear();
    delete root;
}

void tst_gadgetsframe::anInactiveTableLetsGoAsWell(){
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(0, 0), QSizeF(SCREEN)));
    GraphicsTableView *table = new GraphicsTableView(nullptr);
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = MakeDirectory(4);
    Node *folder = root->GetChildAt(0);
    Node *leaving = folder->GetChildAt(1);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder->GetChildAt(0));
    table->Deactivate();
    QVERIFY(table->m_ThumbnailCache.contains(leaving));

    const QSet<Node*> going = QSet<Node*>() << leaving;
    TakeOutOfTree(leaving);
    table->LetGoOf(going, folder);
    QCOMPARE(KeptAbout(table, going), QString());
    delete leaving;

    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder->GetChildAt(0));
    QCOMPARE(table->m_DisplayThumbnails.length(), folder->ChildrenLength());
    table->Deactivate();
    delete root;
}

void tst_gadgetsframe::theTrashDeletesForGoodOnlyWhenTheUserSaysSo(){
    if(!TreeBank::GetTrashRoot()) TreeBank::Initialize();
    ViewNode *trash = TreeBank::GetTrashRoot();
    QVERIFY(trash->HasNoChildren());
    auto tab = [](ViewNode *parent, const QString &name){
        ViewNode *vn = parent->MakeChild();
        vn->SetHoldView(true);
        vn->SetTitle(name);
        vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + name));
        return vn;
    };
    ViewNode *folder;
    ViewNode *inner;
    ViewNode *inner2;
    ViewNode *loose;
    {
        Booting booting;
        folder = trash->MakeChild();
        folder->SetTitle(QStringLiteral("folder"));
        inner = tab(folder, QStringLiteral("inner"));
        inner2 = tab(folder, QStringLiteral("inner2"));
        loose = tab(trash, QStringLiteral("loose"));
    }

    class RecursiveTable : public GraphicsTableView {
    public:
        NodeCollectionType GetNodeCollectionType() const override { return Recursive;}
    };
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(0, 0), QSizeF(SCREEN)));
    GraphicsTableView *table = new RecursiveTable;
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));
    table->Activate(GraphicsTableView::ViewTree);
    table->m_DisplayType = GraphicsTableView::TrashTree;
    table->SetCurrent(loose);
    QVERIFY(table->IsDisplayingTrashTree());
    QSet<Node*> shown;
    for(Thumbnail *thumb : table->m_DisplayThumbnails) shown << thumb->GetNode();
    QCOMPARE(shown, QSet<Node*>() << folder << inner << inner2 << loose);

    int askedAbout = -1;
    bool answer = false;
    TreeBank::SetDiscardConfirmer([&](int count){ askedAbout = count; return answer; });
    const auto unhook = qScopeGuard([]{ TreeBank::SetDiscardConfirmer(nullptr); });

    GraphicsTableView::m_NodesRegister << table->m_DummyViewNode;
    QVERIFY(!table->ThumbList_DeleteNode());
    QTest::qWait(50);
    QCOMPARE(askedAbout, -1);

    GraphicsTableView::m_NodesRegister = NodeList() << folder << inner << inner2 << loose << loose;
    QVERIFY(table->ThumbList_DeleteNode());
    QTRY_COMPARE_WITH_TIMEOUT(askedAbout, 3, 1000);
    QTest::qWait(50);
    QCOMPARE(trash->GetChildren(), NodeList() << folder << loose);
    QCOMPARE(table->m_DisplayThumbnails.length(), 4);
    QCOMPARE(GraphicsTableView::m_NodesRegister.length(), 4);

    askedAbout = -1;
    QVERIFY(table->DeleteNodes(NodeList() << folder << inner2 << loose));
    QTRY_COMPARE_WITH_TIMEOUT(askedAbout, 2, 1000);
    askedAbout = -1;
    QVERIFY(!table->DeleteNodes(NodeList() << folder));
    QTest::qWait(50);
    QCOMPARE(askedAbout, -1);

    answer = true;
    askedAbout = -1;
    GraphicsTableView::m_NodesRegister = NodeList() << folder;
    QVERIFY(table->ThumbList_DeleteOtherNode());
    GraphicsTableView::m_NodesRegister.clear();
    table->Deactivate();
    delete table;
    QTRY_COMPARE_WITH_TIMEOUT(trash->GetChildren(), NodeList() << folder, 1000);
    QCOMPARE(askedAbout, 3);
    QVERIFY(folder->HasNoChildren());

    bool settledWhileThere = true;
    bool settled = false;
    TreeBank::DiscardFromTrashAsked(QList<quint64>() << folder->GetSerial(), 1, QString(),
                                    [&](){ settled = true; settledWhileThere = trash->GetChildren().contains(folder); });
    QTRY_VERIFY_WITH_TIMEOUT(settled, 1000);
    QVERIFY(!settledWhileThere);
    QVERIFY(trash->HasNoChildren());
}

void tst_gadgetsframe::thePageIsDrawnBehindNodesButNotBehindAccessKeys(){
    QVERIFY(!Gadgets::DrawsBackdrop(false, false, true));
    QVERIFY(!Gadgets::DrawsBackdrop(false, false, false));
    QVERIFY( Gadgets::DrawsBackdrop(true, true,  true));
    QVERIFY(!Gadgets::DrawsBackdrop(true, true,  false));
    QVERIFY( Gadgets::DrawsBackdrop(true, false, true));
    QVERIFY( Gadgets::DrawsBackdrop(true, false, false));
}

void tst_gadgetsframe::theBackdropBlurSoftensAnEdgeAndNothingElse(){
    const int radius = 24;
    const QSize size(400, 200);

    QImage edge(size, QImage::Format_ARGB32_Premultiplied);
    edge.setDevicePixelRatio(1.5);
    edge.fill(Qt::white);
    for(int y = 0; y < size.height(); y++)
        for(int x = 0; x < size.width() / 2; x++)
            edge.setPixel(x, y, qRgb(0, 0, 0));

    const QImage blurred = Gadgets::BlurBackdrop(edge, radius);
    QCOMPARE(blurred.size(), size);
    QCOMPARE(blurred.devicePixelRatio(), 1.5);

    const int y = size.height() / 2;
    const auto grey = [&](int x){ return qGray(blurred.pixel(x, y));};
    QVERIFY(grey(10) <= 2);
    QVERIFY(grey(size.width() - 10) >= 253);
    QVERIFY2(qAbs(grey(200) - 128) <= 24, qPrintable(QString::number(grey(200))));
    QVERIFY2(grey(200 - radius) > 4 && grey(200 - radius) < 80,
             qPrintable(QString::number(grey(200 - radius))));
    QVERIFY2(grey(200 + radius) > 175 && grey(200 + radius) < 251,
             qPrintable(QString::number(grey(200 + radius))));
    for(int x = 150; x < 250; x++)
        QVERIFY2(grey(x) <= grey(x + 1) + 1, qPrintable(QString::number(x)));

    QImage strip(size, QImage::Format_ARGB32_Premultiplied);
    strip.fill(Qt::transparent);
    for(int y2 = 0; y2 < size.height(); y2++)
        for(int x = 0; x < size.width() / 2; x++)
            strip.setPixel(x, y2, qRgb(200, 40, 40));
    const QImage soft = Gadgets::BlurBackdrop(strip, radius)
        .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    for(int x = 0; x < size.width(); x += 7){
        const QRgb p = soft.pixel(x, y);
        QVERIFY(qRed(p) <= qAlpha(p) && qGreen(p) <= qAlpha(p) && qBlue(p) <= qAlpha(p));
    }
    QCOMPARE(qAlpha(soft.pixel(size.width() - 10, y)), 0);

    QVERIFY(Gadgets::BlurBackdrop(QImage(), radius).isNull());
    QCOMPARE(Gadgets::BlurBackdrop(edge, 0), edge);
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
    TranslucentWindow window(QByteArray(QTest::currentDataTag()) == TRANSLUCENT_ROW);

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
    TranslucentWindow window(QByteArray(QTest::currentDataTag()) == TRANSLUCENT_ROW);

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

void tst_gadgetsframe::recursivePrimaryTracksTheDisplayedPage_data(){
    QTest::addColumn<int>("collection");
    QTest::newRow("recursive") << int(GraphicsTableView::Recursive);
    QTest::newRow("foldable") << int(GraphicsTableView::Foldable);
}

void tst_gadgetsframe::recursivePrimaryTracksTheDisplayedPage(){
    QFETCH(int, collection);
    class RecursiveTable : public GraphicsTableView {
    public:
        using GraphicsTableView::CollectNodes;
        Node *displayed = nullptr;
        Node *DisplayedViewNode() const override { return displayed; }
        NodeCollectionType type;
        NodeCollectionType GetNodeCollectionType() const override { return type; }
    };
    UseStyle(QStringLiteral("FlatStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new RecursiveTable;
    table->type = static_cast<GraphicsTableView::NodeCollectionType>(collection);
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));
    auto *root = MakeDirectory(2);
    auto *folder = root->GetChildAt(0)->ToViewNode();
    folder->SetFolded(false);
    auto *page = folder->GetChildAt(1)->ToViewNode();
    table->displayed = page;
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(folder);
    QVERIFY(table->GetPrimaryThumbnail());
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), page);
    table->SetFilter(QStringLiteral("node 0"));
    QCOMPARE(table->GetPrimaryItemIndex(), -1);
    table->SetFilter(QStringLiteral("directory"));
    QVERIFY(table->GetPrimaryThumbnail());
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), folder);
    table->SetFilter(QString());
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), page);
    if(collection == GraphicsTableView::Foldable){
        folder->SetFolded(true);
        table->CollectNodes(folder);
        QVERIFY(table->GetPrimaryThumbnail());
        QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), folder);
        folder->SetFolded(false);
        table->CollectNodes(folder);
        QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), page);
    }
    table->displayed = folder->GetChildAt(0);
    table->CollectNodes(folder);
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), folder->GetChildAt(0));
    ViewNode *inner;
    ViewNode *deepPage;
    {
        Booting booting;
        inner = folder->MakeChild();
        inner->SetTitle(QStringLiteral("inner directory"));
        inner->SetFolded(false);
        deepPage = inner->MakeChild();
        deepPage->SetHoldView(true);
        deepPage->SetTitle(QStringLiteral("deep page"));
    }
    table->displayed = deepPage;
    table->SetFilter(QStringLiteral("directory"));
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), inner);
    table->SetFilter(QString());
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), deepPage);
    table->displayed = nullptr;
    table->CollectNodes(folder);
    QCOMPARE(table->GetPrimaryItemIndex(), -1);
    table->Deactivate();
    delete table;
    delete root;
}

void tst_gadgetsframe::aLocalFolderGoesUpAsFarAsTheTop(){
    QCOMPARE(QUrl(QStringLiteral("file:///")).toLocalFile(), QStringLiteral("/"));
    QCOMPARE(LocalView::UpDirectoryPath(QStringLiteral("/")), QString());
#ifdef Q_OS_WIN
    QCOMPARE(LocalView::UpDirectoryPath(QStringLiteral("C:/Users/")), QStringLiteral("C:/"));
    QCOMPARE(LocalView::UpDirectoryPath(QStringLiteral("C:/")), QStringLiteral("/"));

    const QStringList folders = LocalView::PathFolders(QStringLiteral("C:/Users/"));
    QCOMPARE(folders, (QStringList{QStringLiteral("/"), QStringLiteral("C:"), QStringLiteral("Users")}));
    QCOMPARE(LocalView::PathOfFolder(folders, 0), QStringLiteral("/"));
    QCOMPARE(LocalView::PathOfFolder(folders, 1), QStringLiteral("C:/"));
    QCOMPARE(LocalView::PathOfFolder(folders, 2), QStringLiteral("C:/Users/"));
    QCOMPARE(LocalView::PathFolders(QStringLiteral("/")), QStringList{QStringLiteral("/")});
#else
    QCOMPARE(LocalView::UpDirectoryPath(QStringLiteral("/home/")), QStringLiteral("/"));
    const QStringList folders = LocalView::PathFolders(QStringLiteral("/home/"));
    QCOMPARE(LocalView::PathOfFolder(folders, 0), QStringLiteral("/"));
    QCOMPARE(LocalView::PathOfFolder(folders, 1), QStringLiteral("/home/"));
#endif
}

void tst_gadgetsframe::aLocalViewOfItsOwnStaysOnDeactivate(){
    QGraphicsScene scene;
    LocalView *view = new LocalView(nullptr, QString(), QStringList());
    scene.addItem(view->base());
    view->base()->show();
    view->Deactivate();
    QVERIFY(view->base()->isVisible());

    QGraphicsSceneMouseEvent ev(QEvent::GraphicsSceneMouseDoubleClick);
    ev.setButton(Qt::LeftButton);
    ev.setPos(QPointF(5, 5));
    scene.sendEvent(view->base(), &ev);
    QVERIFY(ev.isAccepted());
    QVERIFY(view->base()->isVisible());
    scene.removeItem(view->base());
    delete view;
}

void tst_gadgetsframe::theBarClosesTheTabListButNotALocalFolder(){
    UseStyle(QStringLiteral("FlatStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new GraphicsTableView;
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));
    auto *root = MakeDirectory(3);
    auto *folder = root->GetChildAt(0)->ToViewNode();

    for(auto type : {GraphicsTableView::ViewTree, GraphicsTableView::TrashTree}){
        table->Activate(type);
        table->SetCurrent(folder);
        GraphicsButton *close = table->m_CloseTableButton;
        QVERIFY(close->isVisible());
        QVERIFY(close->isEnabled());
        QVERIFY(!close->boundingRect().isEmpty());
        QVERIFY(table->HeaderFilterRect().right() < close->boundingRect().left());
        QVERIFY(close->boundingRect().right() > SCREEN.width() - table->ScaleByDevice(DISPLAY_PADDING_X));
        QCOMPARE(SCREEN.width() - close->boundingRect().right(),
                 close->boundingRect().left() - table->HeaderFilterRect().right());

        const QPointF at = close->boundingRect().center();
        QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
        press.setButton(Qt::LeftButton);
        press.setPos(at);
        press.setScenePos(at);
        scene.sendEvent(close, &press);
        QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
        release.setButton(Qt::LeftButton);
        release.setPos(at);
        release.setScenePos(at);
        scene.sendEvent(close, &release);
        QVERIFY(!table->isVisible());
    }

    table->Activate(GraphicsTableView::LocalFolderTree);
    table->SetCurrent(folder);
    QVERIFY(!table->m_CloseTableButton->isVisible());
    QVERIFY(table->HeaderCloseRect().isNull());
    QCOMPARE(table->HeaderFilterRect().right(),
             qreal(SCREEN.width() - table->ScaleByDevice(DISPLAY_PADDING_X)));

    table->Deactivate();
    delete table;
    delete root;
}

void tst_gadgetsframe::upADirectoryFromWhereTheChosenTabsAre(){
    class MoveTable : public GraphicsTableView {
    public:
        NodeCollectionType GetNodeCollectionType() const override { return Foldable; }
        void choose(Node *hovered, const NodeList &nodes){
            for(int i = 0; i < m_DisplayThumbnails.length(); i++)
                if(m_DisplayThumbnails[i]->GetNode() == hovered) SetHoveredItem(i);
            m_NodesRegister = nodes;
        }
        Node *current() const { return m_CurrentNode;}
        NodeList chosen() const { return m_NodesRegister;}
    };
    UseStyle(QStringLiteral("FlatStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new MoveTable;
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *level, *tab, *inner, *a, *b, *last;
    {
        Booting booting;
        auto page = [](ViewNode *parent, const char *title){
            ViewNode *vn = parent->MakeChild();
            vn->SetHoldView(true);
            vn->SetTitle(QString::fromLatin1(title));
            vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + QString::fromLatin1(title)));
            return vn;
        };
        level = root->MakeChild();
        level->SetTitle(QStringLiteral("level"));
        tab = page(level, "tab");
        inner = level->MakeChild();
        inner->SetTitle(QStringLiteral("inner"));
        inner->SetFolded(false);
        a = page(inner, "a");
        b = page(inner, "b");
        last = page(level, "last");
    }
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(tab);

    table->choose(a, NodeList() << a << b);
    QVERIFY(table->ThumbList_TransferToUpDirectory());
    QCOMPARE(a->GetParent(), static_cast<Node*>(level));
    QCOMPARE(b->GetParent(), static_cast<Node*>(level));
    QVERIFY(inner->HasNoChildren());
    QCOMPARE(table->current(), static_cast<Node*>(tab));
    auto chosen = [table](){
        const NodeList nodes = table->chosen();
        return QSet<Node*>(nodes.begin(), nodes.end());
    };
    QCOMPARE(chosen(), QSet<Node*>() << a << b);

    table->choose(last, NodeList() << last);
    QVERIFY(table->ThumbList_TransferToUpDirectory());
    QCOMPARE(last->GetParent(), static_cast<Node*>(root));
    QCOMPARE(table->current(), static_cast<Node*>(level));
    QCOMPARE(chosen(), QSet<Node*>() << last);

    table->Deactivate();
    delete table;
    for(ViewNode *vn : { last, level }){
        root->RemoveChild(vn);
        vn->Delete();
    }
}

void tst_gadgetsframe::intoADirectoryFromWhereTheChosenTabsAre(){
    class MoveTable : public GraphicsTableView {
    public:
        NodeCollectionType GetNodeCollectionType() const override { return Foldable; }
        void choose(Node *hovered, const NodeList &nodes){
            for(int i = 0; i < m_DisplayThumbnails.length(); i++)
                if(m_DisplayThumbnails[i]->GetNode() == hovered) SetHoveredItem(i);
            m_NodesRegister = nodes;
        }
        Node *current() const { return m_CurrentNode;}
        QSet<Node*> chosen() const { return QSet<Node*>(m_NodesRegister.begin(), m_NodesRegister.end());}
    };
    UseStyle(QStringLiteral("FlatStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new MoveTable;
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *level, *tab, *inner, *a, *b, *target, *kept;
    {
        Booting booting;
        auto page = [](ViewNode *parent, const char *title){
            ViewNode *vn = parent->MakeChild();
            vn->SetHoldView(true);
            vn->SetTitle(QString::fromLatin1(title));
            vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + QString::fromLatin1(title)));
            return vn;
        };
        level = root->MakeChild();
        level->SetTitle(QStringLiteral("level"));
        tab = page(level, "tab");
        inner = level->MakeChild();
        inner->SetTitle(QStringLiteral("inner"));
        inner->SetFolded(false);
        a = page(inner, "a");
        b = page(inner, "b");
        target = level->MakeChild();
        target->SetTitle(QStringLiteral("target"));
        target->SetFolded(true);
        kept = page(target, "kept");
    }
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(tab);

    table->choose(target, NodeList() << a << b << target);
    QVERIFY(table->ThumbList_TransferToDownDirectory());
    QCOMPARE(a->GetParent(), static_cast<Node*>(target));
    QCOMPARE(b->GetParent(), static_cast<Node*>(target));
    QCOMPARE(target->GetParent(), static_cast<Node*>(level));
    QCOMPARE(kept->GetParent(), static_cast<Node*>(target));
    QVERIFY(inner->HasNoChildren());
    QCOMPARE(table->current()->GetParent(), static_cast<Node*>(target));
    QCOMPARE(table->chosen(), QSet<Node*>() << a << b);

    table->Deactivate();
    delete table;
    root->RemoveChild(level);
    level->Delete();
}

void tst_gadgetsframe::aPasteTakesTheSelectionOnlyWhenTheSettingSaysSo(){
    class ClipTable : public GraphicsTableView {
    public:
        void choose(Node *hovered, const NodeList &nodes){
            for(int i = 0; i < m_DisplayThumbnails.length(); i++)
                if(m_DisplayThumbnails[i]->GetNode() == hovered) SetHoveredItem(i);
            m_NodesRegister = nodes;
        }
        void setOperation(const QString &operation){ m_SelectionOperation = operation;}
        Node *current() const { return m_CurrentNode;}
        bool middle(){ return PasteNodeAt(QPointF(-1.0, -1.0));}
    };
    UseStyle(QStringLiteral("FlatStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new ClipTable;
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *from, *to, *a, *b, *kept;
    {
        Booting booting;
        auto page = [](ViewNode *parent, const char *title){
            ViewNode *vn = parent->MakeChild();
            vn->SetHoldView(true);
            vn->SetTitle(QString::fromLatin1(title));
            vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + QString::fromLatin1(title)));
            return vn;
        };
        from = root->MakeChild();
        from->SetTitle(QStringLiteral("from"));
        a = page(from, "a");
        b = page(from, "b");
        to = root->MakeChild();
        to->SetTitle(QStringLiteral("to"));
        kept = page(to, "kept");
    }
    TreeBank::SetClipboardNodes(NodeList(), false);
    table->setOperation(QStringLiteral("None"));
    table->Activate(GraphicsTableView::ViewTree);

    table->SetCurrent(a);
    QVERIFY(table->ThumbList_SelectAll());
    QVERIFY(table->ThumbList_UpDirectory());
    table->choose(to, NodeList() << a << b);
    QVERIFY(table->ThumbList_DownDirectory());
    QCOMPARE(table->current()->GetParent(), static_cast<Node*>(to));
    table->choose(kept, NodeList() << a << b);
    QVERIFY(!table->middle());
    QCOMPARE(to->ChildrenLength(), 1);
    QCOMPARE(a->GetParent(), static_cast<Node*>(from));

    QVERIFY(!table->ThumbList_PasteNode());
    table->setOperation(QStringLiteral("Copy"));
    QVERIFY(table->ThumbList_PasteNode());
    QCOMPARE(to->ChildrenLength(), 3);
    TreeBank::SetClipboardNodes(NodeList(), false);
    foreach(Node *nd, to->GetChildren()) if(nd != kept) nd->ToViewNode()->Delete();
    to->SetChildren(NodeList() << kept);
    table->SetCurrent(kept);
    table->choose(kept, NodeList() << a << b);

    table->setOperation(QStringLiteral("Copy"));
    QVERIFY(table->middle());
    QCOMPARE(to->ChildrenLength(), 3);
    QVERIFY(table->middle());
    QCOMPARE(to->ChildrenLength(), 5);
    QCOMPARE(a->GetParent(), static_cast<Node*>(from));

    table->SetCurrent(kept);
    table->choose(kept, NodeList() << kept);
    QVERIFY(table->ThumbList_CutNode());
    table->SetCurrent(a);
    table->choose(a, NodeList() << kept);
    QVERIFY(table->middle());
    QCOMPARE(kept->GetParent(), static_cast<Node*>(from));

    table->setOperation(QStringLiteral("Cut"));
    table->SetCurrent(to->GetFirstChild());
    table->choose(to->GetFirstChild(), NodeList() << a << b);
    QVERIFY(table->middle());
    QCOMPARE(a->GetParent(), static_cast<Node*>(to));
    QCOMPARE(b->GetParent(), static_cast<Node*>(to));
    QVERIFY(!table->middle());

    LocalNode file;
    table->SetCurrent(a);
    TreeBank::SetClipboardNodes(NodeList() << b, false);
    table->choose(a, NodeList() << &file);
    QVERIFY(!table->ThumbList_CutNode());
    QVERIFY(!table->ThumbList_CopyNode());
    QVERIFY(TreeBank::ClipboardHolds(NodeList() << b));
    table->setOperation(QStringLiteral("Cut"));
    const int before = a->GetParent()->ChildrenLength();
    QVERIFY(table->middle());
    QVERIFY(TreeBank::ClipboardHolds(NodeList() << b));
    QCOMPARE(a->GetParent()->ChildrenLength(), before + 1);
    table->choose(a, NodeList());

    table->setOperation(QStringLiteral("None"));
    TreeBank::SetClipboardNodes(NodeList(), false);
    table->Deactivate();
    delete table;
    root->RemoveChild(from);
    root->RemoveChild(to);
    from->Delete();
    to->Delete();
}

void tst_gadgetsframe::aPasteOnAnEmptyPlaceGoesWhereThePlaceIs(){
    class PlaceTable : public GraphicsTableView {
    public:
        QPointF centerOf(Node *nd) const {
            for(Thumbnail *thumb : m_DisplayThumbnails)
                if(thumb->GetNode() == nd) return thumb->boundingRect().center() + thumb->pos();
            return QPointF();
        }
        QPointF titleOf(Node *nd) const {
            for(NodeTitle *title : m_DisplayNodeTitles)
                if(title->GetNode() == nd) return title->boundingRect().center() + title->pos();
            return QPointF();
        }
        qreal halfWidth() const { return m_CurrentThumbnailWidth / 2.0;}
        qreal halfHeight() const { return m_CurrentThumbnailHeight / 2.0;}
        using GraphicsTableView::PasteNodeAt;
        void select(Node *nd, const QString &operation){
            m_NodesRegister = NodeList() << nd;
            m_SelectionOperation = operation;
        }
        void hover(Node *nd){
            for(int i = 0; i < m_DisplayThumbnails.length(); i++)
                if(m_DisplayThumbnails[i]->GetNode() == nd) SetHoveredItem(i);
        }
        void unhover(){ SetHoveredItem(-1);}
        void menuAt(const QPointF &pos){ m_MenuPos = pos;}
        void menuClosed(){ m_MenuPos.reset();}
        void clearSelection(const QString &operation){
            m_NodesRegister.clear();
            m_SelectionOperation = operation;
        }
        bool middleClick(const QPointF &pos){
            QGraphicsSceneMouseEvent ev(QEvent::GraphicsSceneMouseRelease);
            ev.setButton(Qt::MiddleButton);
            ev.setPos(pos);
            ev.setAccepted(false);
            mouseReleaseEvent(&ev);
            return ev.isAccepted();
        }
    };
    UseStyle(QStringLiteral("FlatStyle"));
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(), QSizeF(SCREEN)));
    auto *table = new PlaceTable;
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = TreeBank::GetViewRoot();
    ViewNode *dir, *a, *b, *c;
    auto page = [](ViewNode *parent, const char *title){
        ViewNode *vn = parent->MakeChild();
        vn->SetHoldView(true);
        vn->SetTitle(QString::fromLatin1(title));
        vn->SetUrl(QUrl(QStringLiteral("https://example.com/") + QString::fromLatin1(title)));
        return vn;
    };
    {
        Booting booting;
        dir = root->MakeChild();
        dir->SetTitle(QStringLiteral("dir"));
        a = page(dir, "a");
        b = page(dir, "b");
        c = page(dir, "c");
    }
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(a);

    auto titles = [dir](){
        QStringList list;
        foreach(Node *nd, dir->GetChildren()) list << nd->GetTitle();
        return list;
    };

    TreeBank::SetClipboardNodes(NodeList() << a, true);
    QVERIFY(table->middleClick(table->centerOf(c) - QPointF(table->halfWidth(), 0)));
    QCOMPARE(titles(), (QStringList{QStringLiteral("b"), QStringLiteral("a"), QStringLiteral("c")}));

    TreeBank::SetClipboardNodes(NodeList() << b, true);
    table->unhover();
    QVERIFY(table->PasteNodeAt(table->centerOf(c) + QPointF(table->halfWidth(), 0)));
    QCOMPARE(titles(), (QStringList{QStringLiteral("a"), QStringLiteral("c"), QStringLiteral("b")}));

    TreeBank::SetClipboardNodes(NodeList() << a, true);
    QVERIFY(table->PasteNodeAt(table->centerOf(a) + QPointF(0, table->halfHeight() * 3)));
    QCOMPARE(titles(), (QStringList{QStringLiteral("c"), QStringLiteral("b"), QStringLiteral("a")}));

    TreeBank::SetClipboardNodes(NodeList(), false);
    table->select(a, QStringLiteral("Cut"));
    QVERIFY(table->middleClick(table->centerOf(b) - QPointF(table->halfWidth(), 0)));
    QCOMPARE(titles(), (QStringList{QStringLiteral("c"), QStringLiteral("a"), QStringLiteral("b")}));
    table->clearSelection(QStringLiteral("None"));

    TreeBank::SetClipboardNodes(NodeList() << b, true);
    table->hover(c);
    table->menuAt(table->centerOf(a) + QPointF(table->halfWidth(), 0));
    QVERIFY(table->ThumbList_PasteNode());
    QCOMPARE(titles(), (QStringList{QStringLiteral("c"), QStringLiteral("b"), QStringLiteral("a")}));

    TreeBank::SetClipboardNodes(NodeList() << a, true);
    table->unhover();
    table->menuAt(table->centerOf(b) - QPointF(table->halfWidth(), 0));
    QVERIFY(table->ThumbList_PasteNode());
    QCOMPARE(titles(), (QStringList{QStringLiteral("c"), QStringLiteral("a"), QStringLiteral("b")}));
    table->menuClosed();

    TreeBank::SetClipboardNodes(NodeList() << c, true);
    table->unhover();
    QVERIFY(table->ThumbList_PasteNode());
    QCOMPARE(titles(), (QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}));

    table->Deactivate();
    delete table;

    UseStyle(QStringLiteral("GlassStyle"));
    table = new PlaceTable;
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(b);
    const QPointF aboveA = table->titleOf(a) - QPointF(0, 1);
    QVERIFY(table->NodeTitleAreaRect().contains(aboveA));
    TreeBank::SetClipboardNodes(NodeList() << b, true);
    table->unhover();
    QVERIFY(table->PasteNodeAt(aboveA));
    QCOMPARE(titles(), (QStringList{QStringLiteral("b"), QStringLiteral("a"), QStringLiteral("c")}));

    TreeBank::SetClipboardNodes(NodeList(), false);
    table->Deactivate();
    delete table;
    root->RemoveChild(dir);
    dir->Delete();
}

void tst_gadgetsframe::theTableDrawsOnceWhenToldAndNotAgainUntilTold(){
    QGraphicsScene scene;
    scene.setSceneRect(QRectF(QPointF(0, 0), QSizeF(SCREEN)));

    GraphicsTableView *table = new GraphicsTableView(nullptr);
    scene.addItem(table);
    table->Resize(QSizeF(SCREEN));

    ViewNode *root = MakeDirectory(8);
    table->Activate(GraphicsTableView::ViewTree);
    table->SetCurrent(root->GetChildAt(0)->GetChildAt(0));
    QVERIFY(table->isVisible());
    QVERIFY(!table->m_FrameRateTimerId);
    QTest::qWait(300);

    QSignalSpy drawn(&scene, &QGraphicsScene::changed);
    QTest::qWait(700);
    QCOMPARE(drawn.count(), 0);

    table->ScheduleRepaint();
    QTRY_VERIFY(drawn.count() > 0);
    QTest::qWait(200);
    const int once = drawn.count();
    for(int i = 0; i < 100; i++) table->ScheduleRepaint();
    QTRY_COMPARE(drawn.count(), once * 2);
    QTest::qWait(700);
    QCOMPARE(drawn.count(), once * 2);

    table->Deactivate();
    table->hide();
    table->ScheduleRepaint();
    QVERIFY(!table->m_RepaintTimer.isActive());

    delete root;
}

void tst_gadgetsframe::aClockStartedTwiceCountsEachFrameOnce(){
    QGraphicsScene scene;
    GraphicsTableView *table = new GraphicsTableView(nullptr);
    scene.addItem(table);

    table->StartFrameRateTimer();
    table->StartFrameRateTimer();
    table->StartAutoUpdateTimer();
    table->StartAutoUpdateTimer();
    table->m_Frame = 0;
    emit scene.changed(QList<QRectF>() << QRectF(0, 0, 10, 10));
    QCOMPARE(table->m_Frame, 1);

    QVERIFY(table->IsCountingFrames());
    QVERIFY(table->IsAutoUpdating());
    table->StopFrameRateTimer();
    table->StopAutoUpdateTimer();
    QVERIFY(!table->IsCountingFrames());
    QVERIFY(!table->IsAutoUpdating());
    emit scene.changed(QList<QRectF>() << QRectF(0, 0, 10, 10));
    QCOMPARE(table->m_Frame, 1);

    QGraphicsScene other;
    table->StartFrameRateTimer();
    table->StopFrameRateTimer();
    scene.removeItem(table);
    other.addItem(table);
    table->StartFrameRateTimer();
    table->m_Frame = 0;
    emit scene.changed(QList<QRectF>() << QRectF(0, 0, 10, 10));
    QCOMPARE(table->m_Frame, 0);
    emit other.changed(QList<QRectF>() << QRectF(0, 0, 10, 10));
    QCOMPARE(table->m_Frame, 1);
    table->StopFrameRateTimer();
}

QTEST_MAIN(tst_gadgetsframe)
#include "tst_gadgetsframe.moc"
