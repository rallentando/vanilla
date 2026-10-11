#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QObject>
#include <QJSEngine>

#include <algorithm>

#include "minimap.hpp"

#include "testsupport.hpp"

class tst_minimap : public QObject {
    Q_OBJECT

private slots:

    void slideStaysAtZeroWhileTheDrawingFits(){
        QCOMPARE(MiniMap::SlideOffset(500.0, 800.0, 0.0), 0.0);
        QCOMPARE(MiniMap::SlideOffset(500.0, 800.0, 0.5), 0.0);
        QCOMPARE(MiniMap::SlideOffset(500.0, 800.0, 1.0), 0.0);
        QCOMPARE(MiniMap::SlideOffset(800.0, 800.0, 1.0), 0.0);
    }

    void slideSpendsTheSpareHeightLinearly(){
        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0, 0.0),   0.0);
        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0, 0.5), 250.0);
        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0, 1.0), 500.0);

        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0, -1.0),  0.0);
        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0,  2.0), 500.0);
    }

    void theWindowMovesLikeAScrollBarThumb(){
        struct Regime { qreal contentsHeight, viewportHeight, scale, strip; };
        const Regime regimes[] = {
            { 10000.0, 500.0, 0.08, 400.0 },
            {  4000.0, 500.0, 0.08, 400.0 },
        };
        for(const Regime &regime : regimes){
            const qreal drawing = regime.contentsHeight * regime.scale;
            const qreal indicator = regime.viewportHeight * regime.scale;
            const qreal room = regime.contentsHeight - regime.viewportHeight;
            const qreal travel =
                MiniMap::IndicatorTravel(drawing, regime.strip, indicator);
            QVERIFY(travel > 0);
            for(qreal r = 0.0; r <= 1.0; r += 0.25){
                const qreal top =
                    r * room * regime.scale
                    - MiniMap::SlideOffset(drawing, regime.strip, r);
                QVERIFY2(qAbs(top - r * travel) < 1e-9,
                         qPrintable(QStringLiteral("r=%1: top %2, thumb %3")
                                    .arg(r).arg(top).arg(r * travel)));
            }
        }
    }

    void aClickAimsTheMiddleOfTheViewport(){
        QCOMPARE(MiniMap::JumpRatio(50.0, 0.1, 0.0, 100.0, 1000.0), 0.5);

        const qreal slid = MiniMap::JumpRatio(300.0, 0.1, 200.0, 500.0, 10000.0);
        QVERIFY(qAbs(slid - 0.5) < 1e-9);

        QCOMPARE(MiniMap::JumpRatio(   0.0, 0.1, 0.0, 100.0, 1000.0), 0.0);
        QCOMPARE(MiniMap::JumpRatio(1000.0, 0.1, 0.0, 100.0, 1000.0), 1.0);

        QCOMPARE(MiniMap::JumpRatio(50.0, 0.1, 0.0, 1000.0, 1000.0), 0.5);
        QCOMPARE(MiniMap::JumpRatio(50.0, 0.1, 0.0, 2000.0, 1000.0), 0.5);
    }

    void aDragMapsTheThumbLinearly(){
        QCOMPARE(MiniMap::DragRatio(  0.0, 0.0, 300.0), 0.0);
        QCOMPARE(MiniMap::DragRatio(150.0, 0.0, 300.0), 0.5);
        QCOMPARE(MiniMap::DragRatio(300.0, 0.0, 300.0), 1.0);
        QCOMPARE(MiniMap::DragRatio(310.0, 0.0, 300.0), 1.0);
        QCOMPARE(MiniMap::DragRatio( -10.0, 0.0, 300.0), 0.0);

        QCOMPARE(MiniMap::DragRatio(200.0, 50.0, 300.0), 0.5);

        QCOMPARE(MiniMap::DragRatio(100.0, 0.0, 0.0), 0.5);
        QCOMPARE(MiniMap::DragRatio(100.0, 0.0, -5.0), 0.5);
    }

    void parseSiftsTheAnswerOfThePage(){
        QVariantMap answer;
        answer[QStringLiteral("w")] = 1000;
        answer[QStringLiteral("h")] = 5000;
        answer[QStringLiteral("over")] = true;
        QVariantList values;
        values << 0 << false << 10.0 << 20.0 << 100.0 << 10.0
               << 1 << false << 0.0 << 100.0 << 50.0 << 50.0
               << 4 << true << 4.0 << 4.0 << 40.0 << 20.0
               << 2 << true << 5.0 << 5.0 << 30.0 << 10.0
               << 3 << false << 2.0 << 3.0 << 40.0 << 20.0
               << 7 << false << 0.0 << 0.0 << 10.0 << 10.0
               << 0 << false << QStringLiteral("x")
               << 0.0 << 10.0 << 10.0
               << 0 << false << 0.0 << 0.0 << 0.0 << 10.0
               << 0 << false << 1.0;
        answer[QStringLiteral("blocks")] = values;

        QSizeF pageSize;
        bool overflowed = false;
        const QList<MiniMap::Block> blocks =
            MiniMap::ParseBlocks(answer, &pageSize, &overflowed);

        QCOMPARE(pageSize, QSizeF(1000, 5000));
        QVERIFY(overflowed);
        QCOMPARE(blocks.size(), 5);
        QCOMPARE(blocks.at(0).kind, static_cast<int>(MiniMap::TextBlock));
        QCOMPARE(blocks.at(0).rect, QRectF(10, 20, 100, 10));
        QCOMPARE(blocks.at(1).kind, static_cast<int>(MiniMap::MediaBlock));
        QCOMPARE(blocks.at(2).kind,
                 static_cast<int>(MiniMap::PositionedBackgroundBlock));
        QVERIFY(blocks.at(2).followsViewport);
        QCOMPARE(blocks.at(3).kind, static_cast<int>(MiniMap::ControlBlock));
        QVERIFY(blocks.at(3).followsViewport);
        QCOMPARE(blocks.at(4).kind, static_cast<int>(MiniMap::FrameBlock));
    }

    void positionedBlocksFollowTheViewportIndicator(){
        const MiniMap::Block ordinary = {
            QRectF(10, 300, 40, 20), MiniMap::TextBlock, false
        };
        const MiniMap::Block positioned = {
            QRectF(10, 30, 40, 20),
            MiniMap::PositionedBackgroundBlock, true
        };

        const QRectF ordinaryAtTop =
            MiniMap::PaintedBlockRect(ordinary, 1.0, 0.5, 20.0, 40.0);
        const QRectF ordinaryAtBottom =
            MiniMap::PaintedBlockRect(ordinary, 1.0, 0.5, 120.0, 190.0);
        const QRectF fixedAtTop =
            MiniMap::PaintedBlockRect(positioned, 1.0, 0.5, 20.0, 40.0);
        const QRectF fixedAtBottom =
            MiniMap::PaintedBlockRect(positioned, 1.0, 0.5, 120.0, 190.0);

        QCOMPARE(ordinaryAtBottom.top() - ordinaryAtTop.top(), -100.0);
        QCOMPARE(fixedAtBottom.top() - fixedAtTop.top(), 150.0);
        QCOMPARE(fixedAtTop.top() - 40.0, fixedAtBottom.top() - 190.0);
    }

    void collectionWalksFrameDocumentsInTopPageCoordinates(){
        const QString fixture = QStringLiteral(R"JS(
(function(){
    function rectangle(left, top, width, height){
        return { left: left, top: top, width: width, height: height };
    }
    function documentOf(width, height){
        var d = {};
        d.documentElement = { scrollWidth: width, scrollHeight: height };
        d.defaultView = {
            getComputedStyle: function(el){ return el.style; },
            pageXOffset: 0,
            pageYOffset: 0
        };
        d.rangeCreations = 0;
        d.createRange = function(){
            ++d.rangeCreations;
            var selected = null;
            return {
                selectNodeContents: function(node){ selected = node; },
                getClientRects: function(){ return selected.rects; }
            };
        };
        return d;
    }
    function element(doc, tag, bounds){
        return {
            nodeType: 1,
            ownerDocument: doc,
            tagName: tag,
            namespaceURI: '',
            style: { display: 'block', visibility: 'visible', position: 'static' },
            shadowRoot: null,
            firstChild: null,
            nextSibling: null,
            getBoundingClientRect: function(){ return bounds; }
        };
    }
    function text(doc, value, rects){
        return { nodeType: 3, ownerDocument: doc, data: value,
                 rects: rects, nextSibling: null };
    }
    function children(parent, list){
        parent.firstChild = list.length ? list[0] : null;
        for(var i = 0; i < list.length; ++i)
            list[i].nextSibling = i + 1 < list.length ? list[i + 1] : null;
    }

    var nested = documentOf(100, 60);
    var nestedBody = element(nested, 'BODY', rectangle(0, 0, 100, 60));
    nested.body = nestedBody;
    children(nestedBody, [element(nested, 'INPUT', rectangle(3, 4, 20, 8))]);

    var inner = documentOf(500, 400);
    var innerBody = element(inner, 'BODY', rectangle(0, 0, 500, 400));
    inner.body = innerBody;
    var words = text(inner, 'inside', [rectangle(5, 6, 50, 10)]);
    var clippedImage = element(inner, 'IMG', rectangle(280, 140, 50, 30));
    var innerFixed = element(inner, 'BUTTON', rectangle(7, 8, 30, 10));
    innerFixed.style.position = 'fixed';
    var nestedFrame = element(inner, 'IFRAME', rectangle(70, 80, 100, 60));
    nestedFrame.contentDocument = nested;
    children(innerBody, [words, clippedImage, innerFixed, nestedFrame]);

    var legacy = documentOf(50, 40);
    var legacyBody = element(legacy, 'BODY', rectangle(0, 0, 50, 40));
    legacy.body = legacyBody;
    children(legacyBody, [element(legacy, 'BUTTON', rectangle(2, 3, 20, 8))]);

    var document = documentOf(1000, 5000);
    var body = element(document, 'BODY', rectangle(0, 0, 1000, 5000));
    body.scrollWidth = 1000;
    body.scrollHeight = 5000;
    document.body = body;
    var openFrame = element(document, 'IFRAME', rectangle(100, 200, 300, 150));
    openFrame.contentDocument = inner;
    var foreignFrame = element(document, 'IFRAME', rectangle(450, 100, 80, 60));
    foreignFrame.contentDocument = null;
    var legacyFrame = element(document, 'FRAME', rectangle(5, 10, 50, 40));
    legacyFrame.contentDocument = legacy;
    var fixedImage = element(document, 'IMG', rectangle(20, 30, 40, 50));
    fixedImage.style.position = 'fixed';
    var sticky = element(document, 'DIV', rectangle(20, 80, 100, 30));
    sticky.style.position = 'sticky';
    children(sticky, [text(document, 'sticky', [rectangle(25, 85, 60, 10)]),
                     text(document, 'next', [rectangle(25, 100, 60, 10)])]);
    var ordinaryImage = element(document, 'IMG', rectangle(600, 300, 40, 20));
    var ordinaryFigure = element(document, 'FIGURE', rectangle(650, 350, 60, 30));
    children(body, [openFrame, foreignFrame, legacyFrame,
                    fixedImage, sticky, ordinaryImage, ordinaryFigure]);

    var window = document.defaultView;
    window.pageXOffset = 10;
    window.pageYOffset = 20;
    var result = %1;
    result.rangeCreations = [document.rangeCreations, inner.rangeCreations,
                             nested.rangeCreations, legacy.rangeCreations];
    return result;
})()
)JS").arg(MiniMap::CollectBlocksJsCode());

        QJSEngine engine;
        const QJSValue result = engine.evaluate(fixture);
        QVERIFY2(!result.isError(), qPrintable(result.toString()));

        QSizeF pageSize;
        bool overflowed = true;
        const QList<MiniMap::Block> blocks =
            MiniMap::ParseBlocks(result.toVariant(), &pageSize, &overflowed);
        QCOMPARE(pageSize, QSizeF(1000, 5000));
        QVERIFY(!overflowed);
        QCOMPARE(result.property(QStringLiteral("rangeCreations")).toVariant().toList(),
                 (QVariantList{1, 1, 0, 0}));

        auto indexOf = [&blocks](MiniMap::BlockKind kind, const QRectF &rect,
                                 bool followsViewport = false){
            int index = 0;
            foreach(const MiniMap::Block &block, blocks){
                if(block.kind == kind && block.rect == rect &&
                   block.followsViewport == followsViewport) return index;
                ++index;
            }
            return -1;
        };
        auto has = [&indexOf](MiniMap::BlockKind kind, const QRectF &rect,
                              bool followsViewport = false){
            return indexOf(kind, rect, followsViewport) >= 0;
        };
        int frames = 0;
        foreach(const MiniMap::Block &block, blocks)
            if(block.kind == MiniMap::FrameBlock) ++frames;

        QCOMPARE(frames, 4);
        QVERIFY(has(MiniMap::FrameBlock, QRectF(110, 220, 300, 150)));
        QVERIFY(has(MiniMap::FrameBlock, QRectF(460, 120, 80, 60)));
        QVERIFY(has(MiniMap::FrameBlock, QRectF(15, 30, 50, 40)));
        QVERIFY(has(MiniMap::ControlBlock, QRectF(17, 33, 20, 8)));
        QVERIFY(has(MiniMap::TextBlock, QRectF(115, 226, 50, 10)));
        QVERIFY(has(MiniMap::FrameBlock, QRectF(180, 300, 100, 60)));
        QVERIFY(has(MiniMap::ControlBlock, QRectF(183, 304, 20, 8)));
        QVERIFY(has(MiniMap::PositionedBackgroundBlock,
                    QRectF(117, 228, 30, 10)));
        QVERIFY(!has(MiniMap::ControlBlock, QRectF(117, 228, 30, 10)));
        const QRectF fixedRect(20, 30, 40, 50);
        const QRectF stickyRect(20, 80, 100, 30);
        const QRectF stickyTextRect(25, 85, 60, 10);
        const QRectF nextStickyTextRect(25, 100, 60, 10);
        QVERIFY(has(MiniMap::PositionedBackgroundBlock, fixedRect, true));
        QVERIFY(!has(MiniMap::MediaBlock, fixedRect, true));
        QVERIFY(has(MiniMap::PositionedBackgroundBlock, stickyRect, true));
        QVERIFY(has(MiniMap::TextBlock, stickyTextRect, true));
        QVERIFY(has(MiniMap::TextBlock, nextStickyTextRect, true));
        QVERIFY(indexOf(MiniMap::PositionedBackgroundBlock,
                        stickyRect, true) <
                indexOf(MiniMap::TextBlock, stickyTextRect, true));
        QVERIFY(has(MiniMap::MediaBlock, QRectF(610, 320, 40, 20)));
        QVERIFY(has(MiniMap::MediaBlock, QRectF(660, 370, 60, 30)));
        QVERIFY(has(MiniMap::MediaBlock, QRectF(390, 360, 20, 10)));
    }

    void collectionKeepsShallowContentBeforeLongSubtrees(){
        const QString fixture = QStringLiteral(R"JS(
(function(){
    var document = {};
    document.documentElement = { scrollWidth: 1000, scrollHeight: 100000 };
    var window = document.defaultView = {
        pageXOffset: 0, pageYOffset: 100,
        getComputedStyle: function(el){ return el.style; }
    };
    document.createRange = function(){
        var selected;
        return {
            selectNodeContents: function(n){ selected = n; },
            getClientRects: function(){ return selected.rects; }
        };
    };
    function element(tag, position, top){
        return {
            nodeType: 1, ownerDocument: document, tagName: tag,
            style: { display: 'block', visibility: 'visible', position: position },
            getBoundingClientRect: function(){
                return { left: 0, top: top, width: 100, height: 20 };
            }
        };
    }
    function text(rects){
        return { nodeType: 3, ownerDocument: document, data: 'text', rects: rects };
    }
    var body = document.body = element('BODY', 'static', 0);
    var heading = element('H1', 'static', 0);
    heading.firstChild = text([{ left: 0, top: 0, width: 100, height: 20 }]);
    var sticky = element('DIV', 'sticky', 30);
    sticky.shadowRoot = { firstChild: text([{ left: 0, top: 35, width: 50, height: 10 }]) };
    var shadowChild = element('SPAN', 'static', 45);
    shadowChild.firstChild = text([{ left: 0, top: 45, width: 50, height: 10 }]);
    sticky.shadowRoot.firstChild.nextSibling = shadowChild;
    var list = element('DIV', 'static', 100);
    var paragraph = element('P', 'static', 100);
    list.firstChild = paragraph;
    var rects = [];
    for(var i = 0; i < 100001; ++i)
        rects.push({ left: 0, top: 100 + i * 3, width: 100, height: 3 });
    paragraph.firstChild = text(rects);
    var fixed = element('DIV', 'fixed', 60);
    fixed.firstChild = text([{ left: 0, top: 65, width: 50, height: 10 }]);
    body.firstChild = heading;
    heading.nextSibling = sticky;
    sticky.nextSibling = list;
    list.nextSibling = fixed;
    return %1;
})()
)JS").arg(MiniMap::CollectBlocksJsCode());
        QJSEngine engine;
        const QJSValue result = engine.evaluate(fixture);
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        QSizeF size;
        bool over = false;
        const auto blocks = MiniMap::ParseBlocks(result.toVariant(), &size, &over);
        QCOMPARE(blocks.size(), 100000);
        QVERIFY(over);
        auto indexOf = [&blocks](MiniMap::BlockKind kind, qreal top, bool viewport){
            for(int i = 0; i < blocks.size(); ++i)
                if(blocks[i].kind == kind && blocks[i].rect.top() == top &&
                   blocks[i].followsViewport == viewport) return i;
            return -1;
        };
        QVERIFY(indexOf(MiniMap::TextBlock, 100, false) >= 0);
        const int sticky = indexOf(MiniMap::PositionedBackgroundBlock, 30, true);
        const int fixed = indexOf(MiniMap::PositionedBackgroundBlock, 60, true);
        QVERIFY(sticky >= 0);
        QVERIFY(fixed >= 0);
        QVERIFY(indexOf(MiniMap::TextBlock, 35, true) > sticky);
        QVERIFY(indexOf(MiniMap::TextBlock, 45, true) > sticky);
        QVERIFY(indexOf(MiniMap::TextBlock, 65, true) > fixed);
    }

    void collectionSkipsContentsTheEngineHides(){
        const QString fixture = QStringLiteral(R"JS(
(function(){
    var document = {};
    document.documentElement = { scrollWidth: 1000, scrollHeight: 1000 };
    var window = document.defaultView = {
        pageXOffset: 0, pageYOffset: 0,
        getComputedStyle: function(el){ return el.style; }
    };
    document.createRange = function(){
        var selected;
        return {
            selectNodeContents: function(n){ selected = n; },
            getClientRects: function(){ return selected.rects; }
        };
    };
    function element(tag, top, shown){
        var el = {
            nodeType: 1, ownerDocument: document, tagName: tag,
            style: { display: 'block', visibility: 'visible', position: 'static',
                     contentVisibility: 'visible' },
            getBoundingClientRect: function(){
                return { left: 0, top: top, width: 100, height: 20 };
            }
        };
        if(shown !== undefined) el.checkVisibility = function(){ return shown; };
        return el;
    }
    function text(top){
        return { nodeType: 3, ownerDocument: document, data: 'text',
                 rects: [{ left: 0, top: top, width: 50, height: 10 }] };
    }
    var body = document.body = element('BODY', 0, true);
    var details = element('DETAILS', 10, true);
    var summary = element('SUMMARY', 10, true);
    summary.firstChild = text(10);
    var folded = element('DIV', 50, false);
    folded.firstChild = text(50);
    folded.firstChild.nextSibling = element('IMG', 60, false);
    summary.nextSibling = folded;
    details.firstChild = summary;
    var locked = element('DIV', 100, true);
    locked.style.position = 'sticky';
    locked.style.contentVisibility = 'hidden';
    locked.firstChild = text(105);
    var plain = element('P', 200);
    plain.firstChild = text(200);
    // display:contents has no box, so checkVisibility() says false for it;
    // what it wraps is shown all the same (D-483 追記 1).
    var wrapper = element('DIV', 0, false);
    wrapper.style.display = 'contents';
    var wrapped = element('P', 300, true);
    wrapped.firstChild = text(300);
    wrapper.firstChild = wrapped;
    body.firstChild = details;
    details.nextSibling = locked;
    locked.nextSibling = plain;
    plain.nextSibling = wrapper;
    return %1;
})()
)JS").arg(MiniMap::CollectBlocksJsCode());
        QJSEngine engine;
        const QJSValue result = engine.evaluate(fixture);
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        QSizeF size;
        bool over = true;
        const auto blocks = MiniMap::ParseBlocks(result.toVariant(), &size, &over);
        QVERIFY(!over);
        QList<QPair<int, qreal>> found;
        foreach(const MiniMap::Block &block, blocks)
            found << qMakePair(int(block.kind), block.rect.top());
        QCOMPARE(found, (QList<QPair<int, qreal>>{
                     {MiniMap::PositionedBackgroundBlock, 100},
                     {MiniMap::TextBlock, 200},
                     {MiniMap::TextBlock, 10},
                     {MiniMap::TextBlock, 300}}));
    }

    void collectionBoundsDiscoveryWithoutCollectingRectangles_data(){
        QTest::addColumn<int>("nodeType");
        QTest::addColumn<bool>("shadow");
        QTest::addColumn<bool>("deep");
        for(int kind : {1, 3, 8}){
            QTest::newRow(qPrintable(QStringLiteral("light-%1").arg(kind))) << kind << false << false;
            QTest::newRow(qPrintable(QStringLiteral("shadow-%1").arg(kind))) << kind << true << false;
        }
        QTest::newRow("deep-elements") << 1 << false << true;
    }

    void collectionBoundsDiscoveryWithoutCollectingRectangles(){
        QFETCH(int, nodeType);
        QFETCH(bool, shadow);
        QFETCH(bool, deep);
        const QString fixture = QStringLiteral(R"JS(
(function(){
    var examined = 0, advanced = 0;
    var document = { documentElement: { scrollWidth: 100, scrollHeight: 100 } };
    var window = document.defaultView = {
        pageXOffset: 0, pageYOffset: 0,
        getComputedStyle: function(el){ return el.style; }
    };
    var node = {
        data: ' ', tagName: 'DIV', ownerDocument: document,
        style: { display: 'block', visibility: 'visible', position: 'static' }
    };
    Object.defineProperty(node, 'nodeType', { get: function(){
        if(++examined > 200000) throw new Error('unbounded node inspection');
        return %1;
    }});
    Object.defineProperty(node, %3 ? 'firstChild' : 'nextSibling', { get: function(){
        if(++advanced > 100000) throw new Error('unbounded sibling enumeration');
        return node;
    }});
    var body = document.body = {
        nodeType: 1, tagName: 'BODY', ownerDocument: document,
        style: { display: 'block', visibility: 'visible', position: 'static' }
    };
    if(%2) body.shadowRoot = { firstChild: node };
    else body.firstChild = node;
    var result = %4;
    return { result: result, advanced: advanced };
})()
)JS").arg(nodeType).arg(shadow ? QStringLiteral("true") : QStringLiteral("false"))
      .arg(deep ? QStringLiteral("true") : QStringLiteral("false"))
      .arg(MiniMap::CollectBlocksJsCode());
        QJSEngine engine;
        const QJSValue result = engine.evaluate(fixture);
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        QCOMPARE(result.property(QStringLiteral("advanced")).toInt(), 99999);
        const QJSValue answer = result.property(QStringLiteral("result"));
        QVERIFY(answer.property(QStringLiteral("over")).toBool());
        QCOMPARE(answer.property(QStringLiteral("blocks")).property(QStringLiteral("length")).toInt(), 0);
    }

    void collectionKeepsHundredThousandBlocksAndVisits(){
        const QString code = MiniMap::CollectBlocksJsCode();
        const QString fixture = QStringLiteral(R"JS(
(function(){
    var rects = [];
    for(var i = 0; i < 100001; ++i)
        rects.push({ left: 0, top: i * 3, width: 10, height: 3 });

    var document = {};
    document.documentElement = { scrollWidth: 100, scrollHeight: 60003 };
    document.defaultView = {
        getComputedStyle: function(el){ return el.style; },
        pageXOffset: 0,
        pageYOffset: 0
    };
    document.createRange = function(){
        return {
            selectNodeContents: function(node){},
            getClientRects: function(){ return rects; }
        };
    };
    var body = {
        nodeType: 1,
        ownerDocument: document,
        tagName: 'BODY',
        namespaceURI: '',
        style: { display: 'block', visibility: 'visible', position: 'static' },
        shadowRoot: null,
        firstChild: null,
        nextSibling: null
    };
    body.scrollWidth = 100;
    body.scrollHeight = 60003;
    document.body = body;
    body.firstChild = {
        nodeType: 3,
        ownerDocument: document,
        data: 'many lines',
        nextSibling: null
    };
    var window = document.defaultView;
    return %1;
})()
)JS").arg(code);

        QJSEngine engine;
        const QJSValue result = engine.evaluate(fixture);
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        const QVariantMap answer = result.toVariant().toMap();
        QCOMPARE(answer.value(QStringLiteral("blocks")).toList().size(), 600000);
        QVERIFY(answer.value(QStringLiteral("over")).toBool());
        QSizeF pageSize;
        bool overflowed = false;
        QCOMPARE(MiniMap::ParseBlocks(answer, &pageSize, &overflowed).size(), 100000);
        QVERIFY(overflowed);
    }

    void theViewportWindowIsNeverThinnerThanItIsDrawn(){
        const qreal scale = 0.02;
        const qreal minimum = 3.0;
        QCOMPARE(MiniMap::IndicatorHeight(800.0, scale, minimum), 16.0);
        QCOMPARE(MiniMap::IndicatorHeight(100.0, scale, minimum), 3.0);
        QCOMPARE(MiniMap::IndicatorHeight(0.0, scale, minimum), 3.0);

        const qreal drawing = 40000.0 * scale;
        const qreal strip = 600.0;
        const qreal height = MiniMap::IndicatorHeight(100.0, scale, minimum);
        QCOMPARE(MiniMap::IndicatorTravel(drawing, strip, height),
                 strip - height);

        QVERIFY(MiniMap::IndicatorTravel(drawing, strip, 100.0 * scale) >
                MiniMap::IndicatorTravel(drawing, strip, height));
    }

    void aStripWithNoViewIsNeverActive(){
        MiniMap map(nullptr);
        QVERIFY(!map.IsActive());

        map.SetView(nullptr);
        QVERIFY(!map.IsActive());
        QVERIFY(map.isHidden());

        map.SetShelved(true);
        QVERIFY(!map.IsActive());
        QVERIFY(map.isHidden());

        map.SetShelved(false);
        QVERIFY(!map.IsActive());
        QVERIFY(map.isHidden());
    }

    void aBrokenEnvelopeLeavesTheStripBlank(){
        QSizeF pageSize(1, 1);
        bool overflowed = true;

        QVERIFY(MiniMap::ParseBlocks(QVariant(), &pageSize, &overflowed)
                .isEmpty());
        QVERIFY(pageSize.isEmpty());
        QVERIFY(!overflowed);

        QVariantMap noSize;
        noSize[QStringLiteral("blocks")] = QVariantList();
        QVERIFY(MiniMap::ParseBlocks(noSize, &pageSize, &overflowed)
                .isEmpty());
        QVERIFY(pageSize.isEmpty());
    }

    static QString InnerScrollerFixture(int docHeight, int innerHeight,
                                        const QString &tail,
                                        const QString &setup = QString()){
        return QStringLiteral(R"JS(
(function(){
    var document = {};
    document.documentElement = { scrollWidth: 1000, scrollHeight: %1 };
    var window = document.defaultView = {
        pageXOffset: 0, pageYOffset: 0, innerWidth: 1000, innerHeight: 800,
        getComputedStyle: function(el){ return el.style; }
    };
    document.createRange = function(){
        var selected;
        return {
            selectNodeContents: function(n){ selected = n; },
            getClientRects: function(){ return selected.rects; }
        };
    };
    function element(tag, left, top, width, height){
        return {
            nodeType: 1, ownerDocument: document, tagName: tag,
            isConnected: true, parentElement: null,
            clientLeft: 0, clientTop: 0,
            style: { display: 'block', visibility: 'visible',
                     position: 'static', overflowY: 'visible',
                     contentVisibility: 'visible' },
            getBoundingClientRect: function(){
                return { left: left, top: top, width: width, height: height };
            }
        };
    }
    function text(left, top){
        return { nodeType: 3, ownerDocument: document, data: 'text',
                 rects: [{ left: left, top: top, width: 50, height: 10 }] };
    }
    var body = document.body = element('BODY', 0, 0, 1000, 800);
    body.scrollWidth = 1000; body.scrollHeight = 800;
    var header = element('HEADER', 0, 0, 1000, 40);
    header.style.position = 'fixed';
    header.firstChild = text(0, 10);
    var scroller = element('DIV', 100, 50, 600, %2);
    scroller.parentElement = body;
    scroller.style.overflowY = 'auto';
    scroller.clientWidth = 600; scroller.clientHeight = %2;
    scroller.scrollWidth = 600; scroller.scrollHeight = 3000;
    scroller.scrollLeft = 0; scroller.scrollTop = 300;
    // wider than the element: cut to its visible box.
    var sticky = element('DIV', 100, 50, 800, 20);
    sticky.parentElement = scroller;
    sticky.style.position = 'sticky';
    var line = element('P', 100, 60, 600, 10);
    line.parentElement = scroller;
    line.firstChild = text(110, 60);
    scroller.firstChild = sticky;
    sticky.nextSibling = line;
    body.firstChild = header;
    header.nextSibling = scroller;
    %5
    var middle = line;
    document.elementFromPoint = function(){ return middle; };
    var collected = %3;
    var then = %4;
    return { collected: collected, then: then };
})()
)JS").arg(docHeight).arg(innerHeight)
     .arg(MiniMap::CollectBlocksJsCode(), tail, setup);
    }

    void aStillDocumentIsDrawnFromItsInnerScroller(){
        QJSEngine engine;
        const QJSValue result = engine.evaluate(
            InnerScrollerFixture(800, 700, MiniMap::PollScrollerJsCode()));
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        const QVariantMap map = result.toVariant().toMap();
        const QVariant collected = map.value(QStringLiteral("collected"));

        QSizeF size;
        bool over = true;
        const auto blocks = MiniMap::ParseBlocks(collected, &size, &over);
        QCOMPARE(size, QSizeF(600, 3000));
        const MiniMap::Scroller scroller = MiniMap::ParseScroller(collected);
        QVERIFY(scroller.fixed);
        QCOMPARE(scroller.id, qint64(1));
        QCOMPARE(scroller.viewport, QRectF(0, 300, 600, 700));

        QList<QPair<int, QRectF>> found;
        QList<bool> follows;
        foreach(const MiniMap::Block &block, blocks){
            found << qMakePair(int(block.kind), block.rect);
            follows << block.followsViewport;
        }
        QCOMPARE(found, (QList<QPair<int, QRectF>>{
                     {MiniMap::PositionedBackgroundBlock, QRectF(0, 0, 600, 20)},
                     {MiniMap::TextBlock, QRectF(10, 310, 50, 10)}}));
        QCOMPARE(follows, (QList<bool>{true, false}));

        const MiniMap::Poll poll =
            MiniMap::ParsePoll(map.value(QStringLiteral("then")));
        QVERIFY(poll.valid);
        QVERIFY(poll.fixed);
        QCOMPARE(poll.pick, qint64(1));
        QCOMPARE(poll.id, qint64(1));
        QCOMPARE(poll.viewport, QRectF(0, 300, 600, 700));
        QCOMPARE(poll.contents, QSizeF(600, 3000));
    }

    void anElementScrolledFromItsBottomIsMeasuredFromItsTop(){
        const QString setup = QStringLiteral(
            "scroller.style.display = 'flex';\n"
            "    scroller.style.flexDirection = 'column-reverse';\n"
            "    scroller.scrollTop = -300;");
        const QString tail = QStringLiteral(
            "(function(){ var polled = %1;\n"
            "  %2;\n"
            "  var top = scroller.scrollTop;\n"
            "  scroller.scrollTop = 0;\n"
            "  return { polled: polled, top: top, bottom: %1 }; })()")
            .arg(MiniMap::PollScrollerJsCode(),
                 MiniMap::SetScrollerJsCode(1, 0.5));
        QJSEngine engine;
        const QJSValue result =
            engine.evaluate(InnerScrollerFixture(800, 700, tail, setup));
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        const QVariantMap map = result.toVariant().toMap();
        const QVariant collected = map.value(QStringLiteral("collected"));

        const MiniMap::Scroller scroller = MiniMap::ParseScroller(collected);
        QCOMPARE(scroller.viewport, QRectF(0, 2000, 600, 700));
        QSizeF size;
        bool over = true;
        const auto blocks = MiniMap::ParseBlocks(collected, &size, &over);
        QVERIFY(std::any_of(blocks.cbegin(), blocks.cend(),
                            [](const MiniMap::Block &block){
                                return block.kind == MiniMap::TextBlock &&
                                    block.rect.top() == 2010;}));

        const QVariantMap then = map.value(QStringLiteral("then")).toMap();
        const MiniMap::Poll poll = MiniMap::ParsePoll(then.value(QStringLiteral("polled")));
        QVERIFY(poll.valid);
        QCOMPARE(poll.viewport.top(), 2000.0);
        QCOMPARE(then.value(QStringLiteral("top")).toDouble(), -1150.0);
        const MiniMap::Poll bottom =
            MiniMap::ParsePoll(then.value(QStringLiteral("bottom")));
        QVERIFY(bottom.valid);
        QCOMPARE(bottom.viewport.top(), 2300.0);
    }

    void aScrollingDocumentKeepsBeingDrawnAsItself(){
        QJSEngine engine;
        const QJSValue result = engine.evaluate(
            InnerScrollerFixture(2000, 700, QStringLiteral("null")));
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        const QVariant collected =
            result.toVariant().toMap().value(QStringLiteral("collected"));
        const MiniMap::Scroller scroller = MiniMap::ParseScroller(collected);
        QVERIFY(!scroller.fixed);
        QCOMPARE(scroller.id, qint64(0));
        QSizeF size;
        bool over = true;
        const auto blocks = MiniMap::ParseBlocks(collected, &size, &over);
        QCOMPARE(size, QSizeF(1000, 2000));
        QVERIFY(std::any_of(blocks.cbegin(), blocks.cend(),
                            [](const MiniMap::Block &block){
                                return block.kind ==
                                    MiniMap::PositionedBackgroundBlock &&
                                    block.rect.width() == 1000;}));
    }

    void aShortInnerElementIsNotChosen(){
        QJSEngine engine;
        const QJSValue result = engine.evaluate(
            InnerScrollerFixture(800, 300, MiniMap::PollScrollerJsCode()));
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        const QVariantMap map = result.toVariant().toMap();
        const MiniMap::Scroller scroller =
            MiniMap::ParseScroller(map.value(QStringLiteral("collected")));
        QVERIFY(scroller.fixed);
        QCOMPARE(scroller.id, qint64(0));
        const MiniMap::Poll poll =
            MiniMap::ParsePoll(map.value(QStringLiteral("then")));
        QVERIFY(poll.valid);
        QCOMPARE(poll.pick, qint64(0));
        QCOMPARE(poll.id, qint64(0));
    }

    void anotherElementInTheMiddleIsSeenByThePoll(){
        QJSEngine engine;
        const QJSValue result = engine.evaluate(InnerScrollerFixture(
            800, 700, QStringLiteral(
                "(function(){ var other = element('DIV', 0, 0, 1000, 800);\n"
                "  other.style.overflowY = 'scroll';\n"
                "  other.clientHeight = 800; other.parentElement = body;\n"
                "  middle = other; return %1; })()")
                .arg(MiniMap::PollScrollerJsCode())));
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        const MiniMap::Poll poll = MiniMap::ParsePoll(
            result.toVariant().toMap().value(QStringLiteral("then")));
        QVERIFY(poll.valid);
        QCOMPARE(poll.pick, qint64(-1));
        QCOMPARE(poll.id, qint64(1));
    }

    void thePollFollowsTheDrawnElement(){
        MiniMap map(nullptr);
        map.m_PageSize = QSizeF(600, 3000);
        map.m_Scroller.fixed = true;
        map.m_Scroller.id = 4;
        map.m_Scroller.viewport = QRectF(0, 300, 600, 700);

        MiniMap::Poll poll;
        poll.valid = true;
        poll.fixed = true;
        poll.pick = 4;
        poll.id = 4;
        poll.viewport = QRectF(0, 900, 600, 700);
        poll.contents = QSizeF(600, 3000);
        map.TakePoll(poll);
        QCOMPARE(map.m_Scroller.viewport, poll.viewport);
        QVERIFY(map.m_RecollectTimer.isActive());

        map.m_RecollectTimer.stop();
        poll.contents = QSizeF(600, 3500);
        map.TakePoll(poll);
        QCOMPARE(map.m_PageSize, QSizeF(600, 3500));
        QVERIFY(map.m_RecollectTimer.isActive());
        map.m_RecollectTimer.stop();
        map.TakePoll(poll);
        QVERIFY(!map.m_RecollectTimer.isActive());

        map.m_PollTimer.start();
        poll.pick = -1;
        map.TakePoll(poll);
        QVERIFY(!map.m_PollTimer.isActive());
        QCOMPARE(map.m_Scroller.id, qint64(4));
    }

    void aDocumentWhichScrollsAgainStopsThePoll(){
        MiniMap map(nullptr);
        map.m_Scroller.fixed = true;
        map.m_PollTimer.start();
        MiniMap::Poll poll;
        poll.valid = true;
        poll.fixed = false;
        map.TakePoll(poll);
        QVERIFY(!map.m_PollTimer.isActive());
    }

    void clearingTheStripForgetsTheInnerElementAndItsPolls(){
        MiniMap map(nullptr);
        map.m_Scroller.fixed = true;
        map.m_Scroller.id = 2;
        map.m_PollTimer.start();
        map.m_PollIssued = 7;
        map.m_PollApplied = 3;
        map.ClearStrip();
        QVERIFY(!map.m_PollTimer.isActive());
        QCOMPARE(map.m_Scroller.id, qint64(0));
        QVERIFY(!map.m_Scroller.fixed);
        QCOMPARE(map.m_PollApplied, map.m_PollIssued);
    }

    void aStripWithoutAViewDoesNotPoll(){
        MiniMap map(nullptr);
        MiniMap::Scroller scroller;
        scroller.fixed = true;
        map.TakeScroller(scroller);
        QVERIFY(!map.m_PollTimer.isActive());
    }

    void scrolledBooksOneSettledCollection(){
        MiniMap map(nullptr);
        QVERIFY(!map.m_RecollectTimer.isActive());
        map.Scrolled();
        QVERIFY(map.m_RecollectTimer.isActive());
        QVERIFY(map.m_RecollectTimer.isSingleShot());
        map.Scrolled();
        QVERIFY(map.m_RecollectTimer.isActive());
        QTest::qWait(map.m_RecollectTimer.interval() + 100);
        QVERIFY(!map.m_RecollectTimer.isActive());
    }

    void clearingTheStripCancelsTheBooking(){
        MiniMap map(nullptr);
        map.Scrolled();
        QVERIFY(map.m_RecollectTimer.isActive());
        map.ClearStrip();
        QVERIFY(!map.m_RecollectTimer.isActive());
    }
};

QTEST_MAIN(tst_minimap)
#include "tst_minimap.moc"
