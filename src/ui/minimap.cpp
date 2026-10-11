#include "switch.hpp"
#include "const.hpp"

#include "minimap.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QtMath>

#include "treebank.hpp"
#include "view.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "edgewebview.hpp"
#include "theme.hpp"

QString MiniMap::ScrollerJsPrelude(){
    return QStringLiteral(
            "    var KEY = '__vanillaMiniMapScroller';\n"
            "    function docRoom(){\n"
            "        var de = document.documentElement, b = document.body;\n"
            "        if(!de) return 0;\n"
            "        var h = Math.max(de.scrollHeight, b ? b.scrollHeight : 0);\n"
            "        return h - window.innerHeight;\n"
            "    }\n"
            "    function pick(){\n"
            "        var de = document.documentElement;\n"
            "        if(!de || !(docRoom() <= 1)) return null;\n"
            "        var x = window.innerWidth / 2, y = window.innerHeight / 2;\n"
            "        var el = document.elementFromPoint(x, y);\n"
            "        while(el && el.shadowRoot){\n"
            "            var inner = el.shadowRoot.elementFromPoint(x, y);\n"
            "            if(!inner || inner === el) break;\n"
            "            el = inner;\n"
            "        }\n"
            "        while(el && el !== de && el !== document.body){\n"
            "            var oy = window.getComputedStyle(el).overflowY;\n"
            "            if((oy === 'auto' || oy === 'scroll' || oy === 'overlay') &&\n"
            "               el.clientHeight >= window.innerHeight / 2) return el;\n"
            "            el = el.parentElement ||\n"
            "                 (el.parentNode && el.parentNode.host) || null;\n"
            "        }\n"
            "        return null;\n"
            "    }\n"
            "    function held(){\n"
            "        var s = window[KEY];\n"
            "        return s && s.el && s.el.isConnected ? s : null;\n"
            "    }\n"
            "    function reversed(e){\n"
            "        var c = window.getComputedStyle(e);\n"
            "        return e.scrollTop < 0 ||\n"
            "            (c.flexDirection === 'column-reverse' &&\n"
            "             String(c.display).indexOf('flex') >= 0);\n"
            "    }\n"
            "    function topOf(e){\n"
            "        return reversed(e) ? e.scrollTop + (e.scrollHeight - e.clientHeight)\n"
            "                           : e.scrollTop;\n"
            "    }\n"
            "    function leftOf(e){\n"
            "        return e.scrollLeft < 0 ? e.scrollLeft + (e.scrollWidth - e.clientWidth)\n"
            "                                : e.scrollLeft;\n"
            "    }\n");
}

QString MiniMap::CollectBlocksJsCode(){
    return QStringLiteral(
            "(function(){\n")
        + ScrollerJsPrelude()
        + QStringLiteral(
            "    var MAX = 100000, VISITS = 100000;\n"
            "    var doc = document, de = doc.documentElement, body = doc.body;\n"
            "    if(!de) return null;\n"
            "    var fixed = docRoom() <= 1, sc = pick(), scroller = null;\n"
            "    var w = de.scrollWidth, h = de.scrollHeight;\n"
            "    if(body){\n"
            "        w = Math.max(w, body.scrollWidth);\n"
            "        h = Math.max(h, body.scrollHeight);\n"
            "    }\n"
            "    var sx = window.pageXOffset, sy = window.pageYOffset;\n"
            "    var ox = 0, oy = 0, viewClip = null;\n"
            "    if(sc){\n"
            "        var prev = window[KEY];\n"
            "        var id = prev && prev.el === sc ? prev.id\n"
            "               : ((prev && prev.id) || 0) + 1;\n"
            "        Object.defineProperty(window, KEY, {\n"
            "            value: { el: sc, id: id }, configurable: true,\n"
            "            writable: true, enumerable: false });\n"
            "        var box = sc.getBoundingClientRect();\n"
            "        ox = box.left + sc.clientLeft; oy = box.top + sc.clientTop;\n"
            "        sx = leftOf(sc); sy = topOf(sc);\n"
            "        w = sc.scrollWidth; h = sc.scrollHeight;\n"
            "        viewClip = { left: 0, top: 0,\n"
            "                     right: sc.clientWidth, bottom: sc.clientHeight };\n"
            "        scroller = [id, sx, sy, sc.clientWidth, sc.clientHeight];\n"
            "    }\n"
            "    var blocks = [], over = false, visited = 0, kept = 0;\n")
        + QStringLiteral(
            "    function place(r, item){\n"
            "        var left = r.left + item.x, top = r.top + item.y;\n"
            "        var right = left + r.width, bottom = top + r.height;\n"
            "        if(item.clip){\n"
            "            left = Math.max(left, item.clip.left);\n"
            "            top = Math.max(top, item.clip.top);\n"
            "            right = Math.min(right, item.clip.right);\n"
            "            bottom = Math.min(bottom, item.clip.bottom);\n"
            "        }\n"
            "        if(item.viewport && viewClip){\n"
            "            left = Math.max(left, viewClip.left);\n"
            "            top = Math.max(top, viewClip.top);\n"
            "            right = Math.min(right, viewClip.right);\n"
            "            bottom = Math.min(bottom, viewClip.bottom);\n"
            "        }\n"
            "        if(right <= left || bottom <= top) return null;\n"
            "        return { left: left, top: top, right: right, bottom: bottom,\n"
            "                 width: right - left, height: bottom - top };\n"
            "    }\n"
            "    function push(kind, r, item){\n"
            "        var p = place(r, item);\n"
            "        if(!p || p.width < 2 || p.height < 2) return;\n"
            "        if(kept >= MAX){ over = true; return;}\n"
            "        var x = p.left, y = p.top;\n"
            "        if(!item.viewport){ x += sx; y += sy;}\n"
            "        blocks.push(kind, item.viewport,\n"
            "                    x, y, p.width, p.height);\n"
            "        ++kept;\n"
            "    }\n"
            "    function found(){\n"
            "        if(++visited <= VISITS) return true;\n"
            "        over = true;\n"
            "        return false;\n"
            "    }\n"
            "    var queue = [], head = 0, ranges = new Map();\n"
            "    function collect(root, x, y, clip, viewport){\n"
            "        if(!found()) return;\n"
            "        queue.push({ el: root, x: x, y: y, clip: clip,\n"
            "                     viewport: viewport });\n"
            "    }\n"
            "    function scan(parent, item){\n"
            "            for(var n = parent.firstChild; n && !over; n = n.nextSibling){\n"
            "                if(!found()) break;\n"
            "                if(n.nodeType === 3){\n"
            "                    if(!/\\S/.test(n.data)) continue;\n"
            "                    var range = ranges.get(n.ownerDocument);\n"
            "                    if(!range){\n"
            "                        range = n.ownerDocument.createRange();\n"
            "                        ranges.set(n.ownerDocument, range);\n"
            "                    }\n"
            "                    range.selectNodeContents(n);\n"
            "                    var rects = range.getClientRects();\n"
            "                    for(var i = 0; i < rects.length && !over; i++)\n"
            "                        push(0, rects[i], item);\n"
            "                } else if(n.nodeType === 1){\n"
            "                    queue.push({ el: n, x: item.x, y: item.y,\n"
            "                                 clip: item.clip,\n"
            "                                 viewport: item.viewport });\n"
            "                }\n"
            "            }\n"
            "    }\n"
            "    if(sc) collect(sc, -ox, -oy, null, false);\n"
            "    else if(body) collect(body, 0, 0, null, false);\n"
            "    while(head < queue.length && !over){\n"
            "            var item = queue[head], el = item.el;\n"
            "            queue[head++] = null;\n"
            "            var tag = el.tagName;\n"
            "            if(tag === 'SCRIPT' || tag === 'STYLE' ||\n"
            "               tag === 'NOSCRIPT' || tag === 'TEMPLATE') continue;\n"
            "            var ownWindow = el.ownerDocument.defaultView || window;\n"
            "            var cs = ownWindow.getComputedStyle(el);\n"
            "            if(cs.display !== 'contents' &&\n"
            "               el.checkVisibility && !el.checkVisibility()) continue;\n"
            "            if(cs.display === 'none' || cs.visibility === 'hidden') continue;\n"
            "            var isPositioned = el !== sc &&\n"
            "                (cs.position === 'fixed' || cs.position === 'sticky');\n"
            "            if(isPositioned){\n"
            "                if(ownWindow === window || item.viewport)\n"
            "                    item.viewport = true;\n"
            "                push(4, el.getBoundingClientRect(), item);\n"
            "            }\n"
            "            if(el.namespaceURI && el.namespaceURI.indexOf('/svg') >= 0){\n"
            "                if(!isPositioned)\n"
            "                    push(1, el.getBoundingClientRect(), item);\n"
            "                continue;\n"
            "            }\n"
            "            if(tag === 'FRAME' || tag === 'IFRAME'){\n"
            "                var frameRect = el.getBoundingClientRect();\n"
            "                var frameBox = place(frameRect, item);\n"
            "                if(!isPositioned) push(3, frameRect, item);\n"
            "                try{\n"
            "                    var frameDocument = el.contentDocument;\n"
            "                    var frameRoot = frameDocument &&\n"
            "                        (frameDocument.body || frameDocument.documentElement);\n"
            "                    if(frameRoot && frameBox)\n"
            "                        collect(frameRoot, item.x + frameRect.left,\n"
            "                                item.y + frameRect.top, frameBox,\n"
            "                                item.viewport);\n"
            "                } catch(e){}\n"
            "                continue;\n"
            "            }\n"
            "            if(tag === 'IMG' || tag === 'VIDEO' || tag === 'CANVAS' ||\n"
            "               tag === 'EMBED' || tag === 'OBJECT' ||\n"
            "               tag === 'PICTURE' || tag === 'FIGURE'){\n"
            "                if(!isPositioned)\n"
            "                    push(1, el.getBoundingClientRect(), item);\n"
            "                continue;\n"
            "            }\n"
            "            if(tag === 'INPUT' || tag === 'TEXTAREA' ||\n"
            "               tag === 'SELECT' || tag === 'BUTTON'){\n"
            "                if(!isPositioned)\n"
            "                    push(2, el.getBoundingClientRect(), item);\n"
            "                continue;\n"
            "            }\n"
            "            if(cs.contentVisibility === 'hidden') continue;\n"
            "            if(el.shadowRoot) scan(el.shadowRoot, item);\n"
            "            scan(el, item);\n"
            "    }\n"
            "    return { w: w, h: h, over: over, blocks: blocks,\n"
            "             fixed: fixed, s: scroller };\n"
            "})()");
}

QString MiniMap::PollScrollerJsCode(){
    return QStringLiteral("(function(){\n")
        + ScrollerJsPrelude()
        + QStringLiteral(
            "    var s = held(), sc = pick();\n"
            "    var e = s && s.el;\n"
            "    return { fixed: docRoom() <= 1,\n"
            "             pick: sc ? (s && e === sc ? s.id : -1) : 0,\n"
            "             s: s ? [s.id, leftOf(e), topOf(e),\n"
            "                     e.clientWidth, e.clientHeight,\n"
            "                     e.scrollWidth, e.scrollHeight] : null };\n"
            "})()");
}

QString MiniMap::SetScrollerJsCode(qint64 id, qreal ratio){
    return QStringLiteral("(function(){\n")
        + ScrollerJsPrelude()
        + QStringLiteral(
            "    var s = held();\n"
            "    if(!s || s.id !== %1) return;\n"
            "    var room = s.el.scrollHeight - s.el.clientHeight;\n"
            "    s.el.scrollTop = %2 * room - (reversed(s.el) ? room : 0);\n"
            "})()").arg(id).arg(ratio, 0, 'g', 17);
}

MiniMap::MiniMap(TreeBank *parent)
    : QWidget(parent)
    , m_TreeBank(parent)
    , m_View(nullptr)
    , m_WiredBase(nullptr)
    , m_Generation(0)
    , m_Request(0)
    , m_Shelved(false)
    , m_Overflowed(false)
    , m_Dragging(false)
    , m_GrabOffset(0.0)
{
    m_RecollectTimer.setSingleShot(true);
    m_RecollectTimer.setInterval(300);
    connect(&m_RecollectTimer, &QTimer::timeout,
            this, [this](){ RequestBlocks();});
    m_PollIssued = 0;
    m_PollApplied = 0;
    m_PollTimer.setInterval(100);
    connect(&m_PollTimer, &QTimer::timeout,
            this, [this](){ RequestPoll();});
    hide();
}

MiniMap::~MiniMap(){
}

QObject *MiniMap::SupportedBase(View *view){
    QObject *base = view->base();
#ifdef WEBENGINEVIEW
    if(qobject_cast<WebEngineView*>(base)) return base;
    if(qobject_cast<QuickWebEngineView*>(base)) return base;
#endif
#ifdef EDGEWEBVIEW
    if(qobject_cast<EdgeWebView*>(base)) return base;
#endif
    Q_UNUSED(base)
    return nullptr;
}

void MiniMap::SetView(View *view){
    QObject *base = view ? SupportedBase(view) : nullptr;
    if(!base) view = nullptr;

    if(base && m_ViewGuard == base){
        setVisible(!m_Shelved);
        return;
    }

    foreach(const QMetaObject::Connection &connection, m_Connections){
        disconnect(connection);
    }
    m_Connections.clear();
    EndDrag();
    m_View = view;
    m_ViewGuard = base;
    m_WiredBase = base;
    ClearStrip();

    if(!view){
        hide();
        return;
    }

    WatchCached(base);

    const auto it = m_Cache.constFind(base);
    if(it != m_Cache.constEnd() && it->url == view->url()){
        m_Blocks = it->blocks;
        m_PageSize = it->pageSize;
        m_Overflowed = it->overflowed;
        m_Scroller = it->scroller;
    }

    WireCurrentView();
    if(!m_Shelved) show();
    StartPoll();
    RequestBlocks();
}

void MiniMap::SetShelved(bool shelved){
    if(m_Shelved == shelved) return;
    m_Shelved = shelved;
    if(shelved){
        EndDrag();
        hide();
    }
    else if(m_ViewGuard) show();
}

void MiniMap::EndDrag(){
    m_Dragging = false;
    m_GrabOffset = 0.0;
}

void MiniMap::WireCurrentView(){
    QObject *base = m_ViewGuard;

    Q_UNUSED(base)
#ifdef WEBENGINEVIEW
    if(WebEngineView *w = qobject_cast<WebEngineView*>(base)){
        m_Connections
            << connect(w, &QWebEngineView::loadStarted,
                       this, [this](){ DropPage();})
            << connect(w, &QWebEngineView::urlChanged,
                       this, [this](const QUrl&){
                           DropPage(); m_RecollectTimer.start();})
            << connect(w, &QWebEngineView::loadFinished,
                       this, [this](bool ok){
                           if(ok){ DropPage(); RequestBlocks();}})
            << connect(w->page(), &QWebEnginePage::contentsSizeChanged,
                       this, [this](const QSizeF&){ Grown();})
            << connect(w->page(), &QWebEnginePage::scrollPositionChanged,
                       this, [this](const QPointF&){ Scrolled();});
        return;
    }
    if(QuickWebEngineView *q = qobject_cast<QuickWebEngineView*>(base)){
        m_Connections
            << connect(q, &QuickWebEngineView::loadStarted,
                       this, [this](){ DropPage();})
            << connect(q, &QuickWebEngineView::urlChanged,
                       this, [this](const QUrl&){
                           DropPage(); m_RecollectTimer.start();})
            << connect(q, &QuickWebEngineView::loadFinished,
                       this, [this](bool ok){
                           if(ok){ DropPage(); RequestBlocks();}})
            << connect(q, &QuickWebEngineView::contentsSizeChanged,
                       this, [this](const QSizeF&){ Grown();})
            << connect(q, &QuickWebEngineView::scrollPositionChanged,
                       this, [this](const QPointF&){ Scrolled();});
        return;
    }
#endif
#ifdef EDGEWEBVIEW
    if(EdgeWebView *e = qobject_cast<EdgeWebView*>(base)){
        m_Connections
            << connect(e, &EdgeWebView::loadStarted,
                       this, [this](){ DropPage();})
            << connect(e, &EdgeWebView::urlChanged,
                       this, [this](const QUrl&){
                           DropPage(); m_RecollectTimer.start();})
            << connect(e, &EdgeWebView::loadFinished,
                       this, [this](bool ok){
                           if(ok){ DropPage(); RequestBlocks();}})
            << connect(e, &EdgeWebView::PageGeometryChanged,
                       this, [this](){ GeometryReported();});
        return;
    }
#endif
}

bool MiniMap::IsActive() const {
    return m_ViewGuard && !m_Shelved;
}

int MiniMap::MapWidth() const {
    return ScaleByDevice(MINIMAP_WIDTH);
}

void MiniMap::ResizeNotify(QSize size){
    EndDrag();
    setGeometry(size.width() - MapWidth(), 0, MapWidth(), size.height());
    update();
}

void MiniMap::ClearStrip(){
    ++m_Generation;
    m_RecollectTimer.stop();
    StopPoll();
    m_Scroller = Scroller();
    m_Blocks.clear();
    m_PageSize = QSizeF();
    m_Overflowed = false;
    m_LastGeometryContents = QSizeF();
    update();
}

void MiniMap::DropPage(){
    if(QObject *base = m_ViewGuard) m_Cache.remove(base);
    ClearStrip();
}

void MiniMap::Grown(){
    if(m_PageSize.isEmpty()) RequestBlocks();
    else m_RecollectTimer.start();
    update();
}

void MiniMap::Scrolled(){
    m_RecollectTimer.start();
    update();
}

void MiniMap::GeometryReported(){
    qreal scale; QSizeF contents; QRectF viewport;
    if(Geometry(&scale, &contents, &viewport) &&
       contents != m_LastGeometryContents){
        m_LastGeometryContents = contents;
        Grown();
        return;
    }
    Scrolled();
}

void MiniMap::WatchCached(QObject *base){
    if(!base || m_CacheWatch.contains(base)) return;

    QList<QMetaObject::Connection> watch;
    watch << connect(base, &QObject::destroyed,
                     this, &MiniMap::OnViewDestroyed, Qt::UniqueConnection);

#ifdef WEBENGINEVIEW
    if(WebEngineView *w = qobject_cast<WebEngineView*>(base)){
        watch << connect(w, &QWebEngineView::loadStarted,
                         this, [this, base](){ ForgetCached(base);})
              << connect(w, &QWebEngineView::urlChanged,
                         this, [this, base](const QUrl&){ ForgetCached(base);});
    } else if(QuickWebEngineView *q = qobject_cast<QuickWebEngineView*>(base)){
        watch << connect(q, &QuickWebEngineView::loadStarted,
                         this, [this, base](){ ForgetCached(base);})
              << connect(q, &QuickWebEngineView::urlChanged,
                         this, [this, base](const QUrl&){ ForgetCached(base);});
    }
#endif
#ifdef EDGEWEBVIEW
    if(EdgeWebView *e = qobject_cast<EdgeWebView*>(base)){
        watch << connect(e, &EdgeWebView::loadStarted,
                         this, [this, base](){ ForgetCached(base);})
              << connect(e, &EdgeWebView::urlChanged,
                         this, [this, base](const QUrl&){ ForgetCached(base);});
    }
#endif

    m_CacheWatch.insert(base, watch);
}

void MiniMap::ForgetCached(QObject *base){
    if(!base) return;
    m_Cache.remove(base);

    if(base == m_WiredBase) return;

    foreach(const QMetaObject::Connection &connection, m_CacheWatch.value(base)){
        disconnect(connection);
    }
    m_CacheWatch.remove(base);
}

void MiniMap::GoDormant(){
    EndDrag();

    foreach(const QMetaObject::Connection &connection, m_Connections){
        disconnect(connection);
    }
    m_Connections.clear();
    m_View = nullptr;
    m_ViewGuard = nullptr;
    m_WiredBase = nullptr;
    ClearStrip();
    hide();
}

void MiniMap::OnViewDestroyed(QObject *base){
    m_Cache.remove(base);
    m_CacheWatch.remove(base);

    if(base == m_WiredBase) GoDormant();
}

void MiniMap::RequestBlocks(){
    if(!m_ViewGuard || !m_View) return;
    const quint64 generation = m_Generation;
    const quint64 request = ++m_Request;
    QPointer<MiniMap> self = this;
    m_View->CallWithEvaluatedJavaScriptResult
        (CollectBlocksJsCode(), [self, generation, request](QVariant answer){
            if(!self || self->m_Generation != generation ||
               self->m_Request != request) return;
            QSizeF pageSize;
            bool overflowed = false;
            const QList<Block> blocks =
                ParseBlocks(answer, &pageSize, &overflowed);
            if(pageSize.isEmpty()) return;
            self->m_Blocks = blocks;
            self->m_PageSize = pageSize;
            self->m_Overflowed = overflowed;
            self->TakeScroller(ParseScroller(answer));
            if(QObject *base = self->m_ViewGuard){
                Snapshot snapshot;
                snapshot.blocks = blocks;
                snapshot.pageSize = pageSize;
                snapshot.overflowed = overflowed;
                snapshot.url = self->m_View->url();
                snapshot.scroller = self->m_Scroller;
                self->m_Cache.insert(base, snapshot);
                self->WatchCached(base);
            }
            self->update();
        });
}

QList<MiniMap::Block> MiniMap::ParseBlocks(const QVariant &answer,
                                           QSizeF *pageSize,
                                           bool *overflowed){
    QList<Block> blocks;
    *pageSize = QSizeF();
    *overflowed = false;
    if(!answer.isValid() || !answer.canConvert<QVariantMap>())
        return blocks;
    const QVariantMap map = answer.toMap();
    const qreal w = map.value(QStringLiteral("w")).toReal();
    const qreal h = map.value(QStringLiteral("h")).toReal();
    if(w <= 0 || h <= 0) return blocks;
    *pageSize = QSizeF(w, h);
    *overflowed = map.value(QStringLiteral("over")).toBool();
    const QVariantList list = map.value(QStringLiteral("blocks")).toList();
    blocks.reserve(list.size() / 6);
    for(int row = 0; row + 5 < list.size(); row += 6){
        bool ok = false;
        const int kind = list.at(row).toInt(&ok);
        if(!ok || kind < TextBlock || kind > PositionedBackgroundBlock)
            continue;
        if(!list.at(row + 1).canConvert<bool>()) continue;
        const bool followsViewport = list.at(row + 1).toBool();
        qreal number[4];
        for(int i = 0; i < 4; i++){
            number[i] = list.at(row + i + 2).toDouble(&ok);
            if(!ok) break;
        }
        if(!ok) continue;
        const QRectF rect(number[0], number[1], number[2], number[3]);
        if(rect.width() <= 0 || rect.height() <= 0) continue;
        blocks << Block{ rect, kind, followsViewport };
    }
    return blocks;
}

MiniMap::Scroller MiniMap::ParseScroller(const QVariant &answer){
    Scroller scroller;
    if(!answer.isValid() || !answer.canConvert<QVariantMap>())
        return scroller;
    const QVariantMap map = answer.toMap();
    scroller.fixed = map.value(QStringLiteral("fixed")).toBool();
    const QVariantList s = map.value(QStringLiteral("s")).toList();
    if(!scroller.fixed || s.size() < 5) return scroller;
    bool ok = false;
    const qint64 id = s.at(0).toLongLong(&ok);
    if(!ok || id <= 0) return scroller;
    qreal number[4];
    for(int i = 0; i < 4; i++){
        number[i] = s.at(i + 1).toDouble(&ok);
        if(!ok || !qIsFinite(number[i])) return scroller;
    }
    if(number[2] <= 0 || number[3] <= 0) return scroller;
    scroller.id = id;
    scroller.viewport = QRectF(number[0], number[1], number[2], number[3]);
    return scroller;
}

MiniMap::Poll MiniMap::ParsePoll(const QVariant &answer){
    Poll poll;
    if(!answer.isValid() || !answer.canConvert<QVariantMap>()) return poll;
    const QVariantMap map = answer.toMap();
    bool ok = false;
    poll.pick = map.value(QStringLiteral("pick")).toLongLong(&ok);
    if(!ok) return poll;
    poll.fixed = map.value(QStringLiteral("fixed")).toBool();
    const QVariantList s = map.value(QStringLiteral("s")).toList();
    if(s.size() >= 7){
        const qint64 id = s.at(0).toLongLong(&ok);
        qreal number[6];
        for(int i = 0; ok && i < 6; i++){
            number[i] = s.at(i + 1).toDouble(&ok);
            if(ok && !qIsFinite(number[i])) ok = false;
        }
        if(ok && id > 0 && number[2] > 0 && number[3] > 0 &&
           number[4] > 0 && number[5] > 0){
            poll.id = id;
            poll.viewport = QRectF(number[0], number[1], number[2], number[3]);
            poll.contents = QSizeF(number[4], number[5]);
        }
    }
    poll.valid = true;
    return poll;
}

void MiniMap::TakeScroller(const Scroller &scroller){
    m_Scroller = scroller;
    m_PollApplied = m_PollIssued;
    if(m_Scroller.fixed) StartPoll();
    else StopPoll();
}

void MiniMap::StartPoll(){
    if(!m_Scroller.fixed || !m_ViewGuard || !m_View || !isVisible()) return;
    if(!m_PollTimer.isActive()) m_PollTimer.start();
}

void MiniMap::StopPoll(){
    m_PollTimer.stop();
    m_PollApplied = m_PollIssued;
}

void MiniMap::RequestPoll(){
    if(!m_ViewGuard || !m_View) return;
    const quint64 generation = m_Generation;
    const quint64 poll = ++m_PollIssued;
    QPointer<MiniMap> self = this;
    m_View->CallWithEvaluatedJavaScriptResult
        (PollScrollerJsCode(), [self, generation, poll](QVariant answer){
            if(!self || self->m_Generation != generation ||
               poll <= self->m_PollApplied) return;
            self->m_PollApplied = poll;
            const Poll parsed = ParsePoll(answer);
            if(parsed.valid) self->TakePoll(parsed);
        });
}

void MiniMap::TakePoll(const Poll &poll){
    const qint64 drawn = m_Scroller.id;
    if(!poll.fixed || poll.pick != drawn ||
       (drawn && poll.id != drawn)){
        StopPoll();
        m_RecollectTimer.stop();
        RequestBlocks();
        return;
    }
    if(!drawn) return;
    if(poll.contents != m_PageSize ||
       poll.viewport.size() != m_Scroller.viewport.size()){
        m_PageSize = poll.contents;
        m_Scroller.viewport = poll.viewport;
        Grown();
        return;
    }
    if(poll.viewport.topLeft() != m_Scroller.viewport.topLeft()){
        m_Scroller.viewport = poll.viewport;
        Scrolled();
    }
}

QRectF MiniMap::PaintedBlockRect(const Block &block, qreal conversion,
                                 qreal scale, qreal offset,
                                 qreal indicatorTop){
    const qreal factor = conversion * scale;
    const qreal top = block.followsViewport
        ? indicatorTop + block.rect.y() * factor
        : block.rect.y() * factor - offset;
    return QRectF(block.rect.x() * factor, top,
                  block.rect.width() * factor,
                  qMax<qreal>(1.0, block.rect.height() * factor));
}

qreal MiniMap::SlideOffset(qreal drawingHeight, qreal stripHeight,
                           qreal scrollRatio){
    const qreal spare = drawingHeight - stripHeight;
    if(spare <= 0) return 0.0;
    return spare * qBound<qreal>(0.0, scrollRatio, 1.0);
}

qreal MiniMap::IndicatorTravel(qreal drawingHeight, qreal stripHeight,
                               qreal indicatorHeight){
    return qMin(drawingHeight, stripHeight) - indicatorHeight;
}

qreal MiniMap::JumpRatio(qreal y, qreal scale, qreal offset,
                         qreal viewportHeight, qreal contentsHeight){
    const qreal room = contentsHeight - viewportHeight;
    if(room <= 0 || scale <= 0) return 0.5;
    const qreal top = (y + offset) / scale - viewportHeight / 2;
    return qBound<qreal>(0.0, top / room, 1.0);
}

qreal MiniMap::DragRatio(qreal y, qreal grabOffset, qreal travel){
    if(travel <= 0) return 0.5;
    return qBound<qreal>(0.0, (y - grabOffset) / travel, 1.0);
}

qreal MiniMap::IndicatorHeight(qreal viewportHeight, qreal scale,
                               qreal minimum){
    return qMax<qreal>(minimum, viewportHeight * scale);
}

bool MiniMap::Geometry(qreal *scale, QSizeF *contents,
                       QRectF *viewport) const {
    if(!m_ViewGuard || !m_View) return false;
    if(width() <= 0 || height() <= 0) return false;

    if(m_Scroller.id && !m_PageSize.isEmpty()){
        *scale = width() / m_PageSize.width();
        *contents = m_PageSize;
        *viewport = m_Scroller.viewport;
        return true;
    }

    QSizeF whole; QRectF port;
    if(!m_View->PageGeometry(&whole, &port)) return false;
    if(whole.width() <= 0 || whole.height() <= 0) return false;

    *scale = width() / whole.width();
    *contents = whole;
    *viewport = port;
    return true;
}

void MiniMap::paintEvent(QPaintEvent *ev){
    Q_UNUSED(ev)
    QPainter painter(this);
    painter.fillRect(rect(), Theme::Brush(Theme::MiniMapBackground));

    qreal scale; QSizeF contents; QRectF viewport;
    if(!Geometry(&scale, &contents, &viewport)) return;

    const qreal room = contents.height() - viewport.height();
    const qreal ratio =
        room > 0 ? qBound<qreal>(0.0, viewport.y() / room, 1.0) : 0.0;
    const qreal drawingHeight = contents.height() * scale;
    const qreal offset = SlideOffset(drawingHeight, height(), ratio);
    const qreal indicatorHeight =
        IndicatorHeight(viewport.height(), scale, ScaleByDevice(3));
    const QRectF window(0, viewport.y() * scale - offset,
                        width(), indicatorHeight);

    if(!m_Blocks.isEmpty() && m_PageSize.width() > 0){
        const qreal k = contents.width() / m_PageSize.width();
        const QBrush text = Theme::Brush(Theme::MiniMapText);
        const QBrush control = Theme::Brush(Theme::MiniMapControl);
        const QBrush media = Theme::Brush(Theme::MiniMapMedia);
        const QBrush frame = Theme::Brush(Theme::MiniMapFrame);
        const QBrush positioned =
            Theme::Brush(Theme::MiniMapPositionedBackground);
        auto brushFor = [&](const Block &block){
            return block.kind == PositionedBackgroundBlock ? positioned :
                   block.kind == FrameBlock ? frame :
                   block.kind == MediaBlock ? media :
                   block.kind == ControlBlock ? control : text;
        };
        foreach(const Block &block, m_Blocks){
            if(block.followsViewport) continue;
            const QRectF r = PaintedBlockRect(block, k, scale, offset,
                                               window.top());
            if(r.bottom() < 0 || r.top() > height()) continue;
            painter.fillRect(r, brushFor(block));
        }

        painter.fillRect(window, Theme::Brush(Theme::MiniMapViewport));
        painter.save();
        painter.setClipRect(window);
        foreach(const Block &block, m_Blocks){
            if(!block.followsViewport) continue;
            const QRectF r = PaintedBlockRect(block, k, scale, offset,
                                               window.top());
            painter.fillRect(r, brushFor(block));
        }
        painter.restore();
    } else {
        painter.fillRect(window, Theme::Brush(Theme::MiniMapViewport));
    }

    painter.setPen(Theme::Pen(Theme::MiniMapViewportBorder));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(window.adjusted(0, 0, -1, -1));
}

void MiniMap::ApplyRatio(qreal ratio, const QRectF &viewport,
                         const QSizeF &contents){
    if(!m_ViewGuard || !m_View) return;
    if(m_Scroller.id){
        m_View->CallWithEvaluatedJavaScriptResult
            (SetScrollerJsCode(m_Scroller.id, ratio), [](QVariant){});
        m_PollApplied = m_PollIssued;
        const qreal room = contents.height() - viewport.height();
        if(room > 0){
            m_Scroller.viewport.moveTop(room * qBound<qreal>(0.0, ratio, 1.0));
            Scrolled();
        }
        return;
    }
    const qreal maxX = qMax<qreal>(0.0, contents.width() - viewport.width());
    m_View->SetScroll(QPointF(
        maxX > 0 ? qBound<qreal>(0.0, viewport.x() / maxX, 1.0) : 0.5,
        ratio));
}

void MiniMap::mousePressEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){
        qreal scale; QSizeF contents; QRectF viewport;
        if(Geometry(&scale, &contents, &viewport)){
            const qreal room = contents.height() - viewport.height();
            const qreal ratio =
                room > 0 ? qBound<qreal>(0.0, viewport.y() / room, 1.0) : 0.0;
            const qreal drawingHeight = contents.height() * scale;
            const qreal offset = SlideOffset(drawingHeight, height(), ratio);
            const qreal indicatorHeight =
                IndicatorHeight(viewport.height(), scale, ScaleByDevice(3));
            const qreal top = viewport.y() * scale - offset;
            const qreal y = ev->pos().y();
            m_Dragging = true;
            if(y >= top && y < top + indicatorHeight){
                m_GrabOffset = y - top;
            } else {
                ApplyRatio(JumpRatio(y, scale, offset,
                                     viewport.height(), contents.height()),
                           viewport, contents);
                m_GrabOffset = indicatorHeight / 2;
            }
        }
        ev->setAccepted(true);
        return;
    }
    QWidget::mousePressEvent(ev);
}

void MiniMap::mouseMoveEvent(QMouseEvent *ev){

    if(m_Dragging && !(ev->buttons() & Qt::LeftButton)) EndDrag();

    if(m_Dragging){
        qreal scale; QSizeF contents; QRectF viewport;
        if(Geometry(&scale, &contents, &viewport)){
            const qreal drawingHeight = contents.height() * scale;
            const qreal travel =
                IndicatorTravel(drawingHeight, height(),
                                IndicatorHeight(viewport.height(), scale,
                                                ScaleByDevice(3)));
            ApplyRatio(DragRatio(ev->pos().y(), m_GrabOffset, travel),
                       viewport, contents);
        }
        ev->setAccepted(true);
        return;
    }
    QWidget::mouseMoveEvent(ev);
}

void MiniMap::mouseReleaseEvent(QMouseEvent *ev){
    EndDrag();
    QWidget::mouseReleaseEvent(ev);
}

void MiniMap::wheelEvent(QWheelEvent *ev){
    QWidget *widget = qobject_cast<QWidget*>(m_ViewGuard.data());
    if(widget && m_View){
        const QPointF pos(widget->width() / 2.0, widget->height() / 2.0);
        QWheelEvent turned(pos, widget->mapToGlobal(pos),
                           ev->pixelDelta(), ev->angleDelta(),
                           ev->buttons(), ev->modifiers(), ev->phase(),
                           ev->inverted(), ev->source(),
                           ev->pointingDevice());
        turned.setTimestamp(ev->timestamp());
        m_View->WheelEvent(&turned);
    }
    ev->setAccepted(true);
}

void MiniMap::hideEvent(QHideEvent *ev){
    EndDrag();
    StopPoll();
    QWidget::hideEvent(ev);
}

void MiniMap::showEvent(QShowEvent *ev){
    QWidget::showEvent(ev);
    StartPoll();
}
