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

#include "application.hpp"
#include "lightnode.hpp"
#include "graphicstableview.hpp"
#include "gadgets.hpp"
#include <QMenu>
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
        return style == QStringLiteral("FlatStyle") ?
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
    } else {
        QCOMPARE(hit, cell);
    }
    const QPointF centre = thumb->mapToScene(cell.center());
    const QPointF corner = thumb->mapToScene(cell.topLeft() + QPointF(1, 1));
    QCOMPARE(scene.itemAt(centre, QTransform()), static_cast<QGraphicsItem*>(thumb));
    QGraphicsItem *atCorner = scene.itemAt(corner, QTransform());
    if(style == QStringLiteral("FlatStyle")){
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
    if(style == QStringLiteral("FlatStyle")){
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
    for(const QString &style : {QStringLiteral("FlatStyle"), QStringLiteral("GlassStyle")}){
        UseStyle(style);
        table->ThumbList_RefreshNoScroll();
        for(auto *item : cached)
            QCOMPARE(item->opacity(), style == QStringLiteral("FlatStyle") ? 1.0 : 0.8);
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
    table->CollectNodes(folder, QStringLiteral("node 0"));
    QCOMPARE(table->GetPrimaryItemIndex(), -1);
    table->CollectNodes(folder, QStringLiteral("directory"));
    QVERIFY(table->GetPrimaryThumbnail());
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), folder);
    table->CollectNodes(folder);
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
    table->CollectNodes(folder, QStringLiteral("directory"));
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), inner);
    table->CollectNodes(folder);
    QCOMPARE(table->GetPrimaryThumbnail()->GetNode(), deepPage);
    table->displayed = nullptr;
    table->CollectNodes(folder);
    QCOMPARE(table->GetPrimaryItemIndex(), -1);
    table->Deactivate();
    delete table;
    delete root;
}

QTEST_MAIN(tst_gadgetsframe)
#include "tst_gadgetsframe.moc"
