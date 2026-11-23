#include "switch.hpp"
#include "const.hpp"

#include "view.hpp"

QString View::GetBaseUrlJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    var baseUrl = \"\";\n"
        "    var baseDocument = \"index.html\";\n"
        "    var base = document.getElementsByTagName(\"base\");\n"
        "    if(base.length > 0 && base[0].href){\n"
        "        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
        "    } else {\n"
        "        baseUrl = location.href;\n"
        "    }\n"
        "    return baseUrl;\n"
        "})();");
}

QString View::GetCurrentBaseUrlJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    var doc = document;\n"
        "    for(var i = 0; i < frames.length; i++){\n"
        "        try{\n"
        "            if(frames[i].document.hasFocus()){\n"
        "                doc = frames[i].document;\n"
        "            }\n"
        "        }\n"
        "        catch(e){}\n"
        "    }\n"
        "    var baseUrl = \"\";\n"
        "    var baseDocument = \"index.html\";\n"
        "    var base = doc.getElementsByTagName(\"base\");\n"
        "    if(base.length > 0 && base[0].href){\n"
        "        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
        "    } else {\n"
        "        baseUrl = doc.location.href;\n"
        "    }\n"
        "    return baseUrl;\n"
        "})();");
}

QString View::UpKeyEventJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    elem.scrollTop -= 40;\n"
        "    body.scrollTop -= 40;\n"
        "})();");
}

QString View::DownKeyEventJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    elem.scrollTop += 40;\n"
        "    body.scrollTop += 40;\n"
        "})();");
}

QString View::RightKeyEventJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    elem.scrollLeft += 40;\n"
        "    body.scrollLeft += 40;\n"
        "})();");
}

QString View::LeftKeyEventJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    elem.scrollLeft -= 40;\n"
        "    body.scrollLeft -= 40;\n"
        "})();");
}

QString View::PageDownKeyEventJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    elem.scrollTop += elem.clientHeight * 0.9;\n"
        "    body.scrollTop += body.clientHeight * 0.9;\n"
        "})();");
}

QString View::PageUpKeyEventJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    elem.scrollTop -= elem.clientHeight * 0.9;\n"
        "    body.scrollTop -= body.clientHeight * 0.9;\n"
        "})();");
}

QString View::HomeKeyEventJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    elem.scrollTop = 0;\n"
        "    body.scrollTop = 0;\n"
        "})();");
}

QString View::EndKeyEventJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    var vmax = elem.scrollHeight - elem.clientHeight;\n"
        "    if(vmax <= 0) \n"
        "        vmax = body.scrollHeight - body.clientHeight;\n"
        "    elem.scrollTop = vmax;\n"
        "    body.scrollTop = vmax;\n"
        "})();");
}

QString View::ElementAtPointJsCode(){
    return QStringLiteral(
        "    var elem = document.elementFromPoint(x, y);\n"
        "    while(elem && (elem.tagName == \"FRAME\" || elem.tagName == \"IFRAME\")){\n"
        "        try{\n"
        "            var frameDocument = elem.contentDocument;\n"
        "            var rect = elem.getBoundingClientRect();\n"
        "            x -= rect.left + (elem.clientLeft || 0);\n"
        "            y -= rect.top + (elem.clientTop || 0);\n"
        "            elem = frameDocument.elementFromPoint(x, y);\n"
        "        }\n"
        "        catch(e){ break;}\n"
        "    }\n");
}

QString View::ElementPathJsCode(){
    return QStringLiteral(
        "var elementPath = function(elem){\n"
        "    var xpath = \"\";\n"
        "    var iter = elem;\n"
        "    var rootOf = function(n){ while(n && n.parentNode) n = n.parentNode; return n; };\n"
        "    var inShadow = function(n){ var r = rootOf(n); return !!(r && r.nodeType == 11 && r.host); };\n"
        "    var shadow = inShadow(iter);\n"
        "    while(iter && iter.nodeType == 1){\n"
        "        var str = iter.tagName;\n"
        "        var siblings = iter.parentNode.childNodes;\n"
        "        var synonym = [];\n"
        "        for(var j = 0; j < siblings.length; j++){\n"
        "            if(siblings[j].nodeType == 1 &&\n"
        "               (shadow ? siblings[j].tagName.toLowerCase() == iter.tagName.toLowerCase()\n"
        "                       : siblings[j].tagName == iter.tagName)){\n"
        "                synonym.push(siblings[j]);\n"
        "            }\n"
        "        }\n"
        "        if(synonym.length > 1 && synonym.indexOf(iter) != -1){\n"
        "            str += \"[\" + (synonym.indexOf(iter) + 1) + \"]\";\n"
        "        }\n"
        "        if(xpath && !xpath.startsWith(\",\") && !xpath.startsWith(\"|\")){\n"
        "            xpath = str + \"/\" + xpath;\n"
        "        } else {\n"
        "            xpath = str + xpath;\n"
        "        }\n"
        "        iter = iter.parentNode;\n"
        "        if(iter && iter.nodeType == 9 && iter !== document){\n"
        "            xpath = \",//\" + xpath.toLowerCase();\n"
        "            iter = iter.defaultView.frameElement;\n"
        "            shadow = inShadow(iter);\n"
        "        } else if(iter && iter.nodeType == 11 && iter.host){\n"
        "            xpath = \"|\" + xpath.toLowerCase();\n"
        "            iter = iter.host;\n"
        "            shadow = inShadow(iter);\n"
        "        }\n"
        "    }\n"
        "    return \"//\" + xpath.toLowerCase();\n"
        "};\n");
}

QString View::ResolveElementPathJsCode(){
    return QStringLiteral(
        "var resolveElementPath = function(path){\n"
        "    var elem = null;\n"
        "    var doc = document;\n"
        "    var segments = path.match(/[,|]?[^,|]+/g) || [];\n"
        "    for(var i = 0; i < segments.length; i++){\n"
        "        var seg = segments[i];\n"
        "        if(seg.charAt(0) == \"|\"){\n"
        "            var node = elem ? elem.shadowRoot : null;\n"
        "            var steps = seg.substring(1).split(\"/\");\n"
        "            for(var k = 0; node && k < steps.length; k++){\n"
        "                var m = /^([^\\[]+)(?:\\[(\\d+)\\])?$/.exec(steps[k]);\n"
        "                var nth = m && m[2] ? parseInt(m[2], 10) : 1;\n"
        "                var kids = node.childNodes;\n"
        "                var next = null;\n"
        "                for(var c = 0; m && c < kids.length; c++){\n"
        "                    if(kids[c].nodeType == 1 &&\n"
        "                       kids[c].tagName.toLowerCase() == m[1] &&\n"
        "                       --nth == 0){\n"
        "                        next = kids[c];\n"
        "                        break;\n"
        "                    }\n"
        "                }\n"
        "                node = next;\n"
        "            }\n"
        "            elem = node;\n"
        "        } else {\n"
        "            if(seg.charAt(0) == \",\"){\n"
        "                var inner = null;\n"
        "                try{ inner = elem ? elem.contentDocument : null; }\n"
        "                catch(e){ inner = null; }\n"
        "                if(!inner){ elem = null; break; }\n"
        "                doc = inner;\n"
        "                seg = seg.substring(1);\n"
        "            }\n"
        "            elem = doc.evaluate(seg, doc, null, 7, null).snapshotItem(0);\n"
        "        }\n"
        "    }\n"
        "    return { elem: elem, doc: doc };\n"
        "};\n");
}

QString View::CollectElementsJsCode(){
    return QStringLiteral(
        "var collectElements = function(root, selector){\n"
        "    var found = [];\n"
        "    var walk = function(r){\n"
        "        var hit = r.querySelectorAll(selector);\n"
        "        for(var i = 0; i < hit.length; i++) found.push(hit[i]);\n"
        "        var all = r.querySelectorAll(\"*\");\n"
        "        for(var k = 0; k < all.length; k++){\n"
        "            if(all[k].shadowRoot) walk(all[k].shadowRoot);\n"
        "        }\n"
        "    };\n"
        "    walk(root);\n"
        "    return found;\n"
        "};\n");
}

QString View::SetFocusToElementJsCode(const QString &xpath){
    QString quoted = EscapeJsStringLiteral(xpath);
    return (QStringLiteral(
        "(function(){\n")
        + ResolveElementPathJsCode() +
        QStringLiteral(
        "    var elem = resolveElementPath(\"%1\").elem;\n"
        "    elem.focus();\n"
        "})();")).arg(quoted);
}

QString View::FireClickEventJsCode(const QString &xpath, const QPoint &pos){
    QString quoted = EscapeJsStringLiteral(xpath);
    return (QStringLiteral(
        "(function(){\n")
        + ResolveElementPathJsCode() +
        QStringLiteral(
        "    var found = resolveElementPath(\"%1\");\n"
        "    var event = new MouseEvent(\"click\", {\n"
        "        bubbles: true,\n"
        "        cancelable: true,\n"
        "        composed: true,\n"
        "        view: found.doc.defaultView,\n"
        "        clientX: %2,\n"
        "        clientY: %3,\n"
        "    });\n"
        "    found.elem.dispatchEvent(event);\n"
        "})();")).arg(quoted, QString::number(pos.x()), QString::number(pos.y()));
}

QString View::GetScrollValuePointJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    var hval = elem.scrollLeft || body.scrollLeft;\n"
        "    var vval = elem.scrollTop || body.scrollTop;\n"
        "    return [hval, vval];\n"
        "})();");
}

QString View::SetScrollValuePointJsCode(const QPoint &pos){
    return QStringLiteral("scrollTo(%1, %2);").arg(pos.x()).arg(pos.y());
}

QString View::GetMediaTimeJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    var vids = document.querySelectorAll(\"video\");\n"
        "    var time = 0;\n"
        "    for(var i = 0; i < vids.length; i++){\n"
        "        var v = vids[i];\n"
        "        if(!isFinite(v.duration) || v.duration <= 0) continue;\n"
        "        if(v.currentTime > time) time = v.currentTime;\n"
        "    }\n"
        "    return time;\n"
        "})();");
}

QString View::SetMediaTimeJsCode(float time){
    return QStringLiteral(
        "(function(){\n"
        "    var target = %1;\n"
        "    var tries = 40;\n"
        "    function apply(){\n"
        "        var vids = document.querySelectorAll(\"video\");\n"
        "        for(var i = 0; i < vids.length; i++){\n"
        "            var v = vids[i];\n"
        "            if(!isFinite(v.duration) || v.duration < target) continue;\n"
        "            v.currentTime = target;\n"
        "            return true;\n"
        "        }\n"
        "        return false;\n"
        "    }\n"
        "    function tick(){\n"
        "        if(apply() || --tries <= 0) return;\n"
        "        setTimeout(tick, 500);\n"
        "    }\n"
        "    tick();\n"
        "})();").arg(static_cast<double>(time));
}

QString View::GetScrollBarStateJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    var hmax = elem.scrollWidth - elem.clientWidth;\n"
        "    if(hmax <= 0) \n"
        "        hmax = body.scrollWidth - body.clientWidth;\n"
        "    var vmax = elem.scrollHeight - elem.clientHeight;\n"
        "    if(vmax <= 0) \n"
        "        vmax = body.scrollHeight - body.clientHeight;\n"
        "    return [hmax, vmax];\n"
        "})();");
}

QString View::GetScrollRatioPointJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    var hval = elem.scrollLeft || body.scrollLeft;\n"
        "    var vval = elem.scrollTop || body.scrollTop;\n"
        "    var hmax = elem.scrollWidth - elem.clientWidth;\n"
        "    if(hmax <= 0) \n"
        "        hmax = body.scrollWidth - body.clientWidth;\n"
        "    var vmax = elem.scrollHeight - elem.clientHeight;\n"
        "    if(vmax <= 0) \n"
        "        vmax = body.scrollHeight - body.clientHeight;\n"
        "    return [hmax <= 0 ? 0.5 : hval / hmax,\n"
        "            vmax <= 0 ? 0.5 : vval / vmax];\n"
        "})();");
}

QString View::SetScrollRatioPointJsCode(const QPointF &pos){
    return QStringLiteral(
        "(function(){\n"
        "    if(!document.body || !document.documentElement) return;\n"
        "    var elem = document.documentElement;\n"
        "    var body = document.body;\n"
        "    var hmax = elem.scrollWidth - elem.clientWidth;\n"
        "    if(hmax <= 0) \n"
        "        hmax = body.scrollWidth - body.clientWidth;\n"
        "    var vmax = elem.scrollHeight - elem.clientHeight;\n"
        "    if(vmax <= 0) \n"
        "        vmax = body.scrollHeight - body.clientHeight;\n"
        "    var hval = hmax * %1;\n"
        "    var vval = vmax * %2;\n"
        "    scrollTo(hval, vval);\n"
        "})();").arg(pos.x()).arg(pos.y());
}

QString View::FindElementsJsCode(Page::FindElementsOption option){
    QString quoted = EscapeJsStringLiteral(Page::OptionToSelector(option));
    QString ignoreOutOfView = option == Page::ForAccessKey ? QStringLiteral("true") : QStringLiteral("false");
#if defined(Q_OS_WIN)
    QString fix = QStringLiteral("devicePixelRatio");
#elif defined(Q_OS_MAC)
    QString fix = QStringLiteral("(document.body.clientWidth / innerWidth)");
#else
    QString fix = QStringLiteral("1");
#endif

    return (QStringLiteral(
        "(function(){\n")
        + CollectElementsJsCode() + ElementPathJsCode() +
        QStringLiteral(
        "    var scrollX = document.documentElement.scrollLeft || document.body.scrollLeft;\n"
        "    var scrollY = document.documentElement.scrollTop || document.body.scrollTop;\n"
        "    var baseUrl = \"\";\n"
        "    var baseDocument = \"index.html\";\n"
        "    var base = document.getElementsByTagName(\"base\");\n"
        "    if(base.length > 0 && base[0].href){\n"
        "        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
        "    } else {\n"
        "        baseUrl = \n"
        "            location.protocol + \"//\" + location.hostname + \n"
        "            (location.port && \":\" + location.port) + \"/\";\n"
        "    }\n"
        "    var elems = collectElements(document, \"%1\");\n"
        "    var map = {};\n"
        "    for(var i = 0; i < elems.length; i++){\n"
        "        var data = {};\n"
        "        data.tagName = elems[i].tagName;\n"
        "        data.innerText = elems[i].innerText;\n"
        "        data.linkUrl = elems[i].href;\n"
        "        data.linkHtml = elems[i].outerHTML;\n"
        "        data.imageUrl = elems[i].currentSrc || elems[i].src;\n"
        "        data.imageHtml = elems[i].innerHTML;\n"
        "        data.baseUrl = baseUrl;\n"
        "        var offsetX = 0;\n"
        "        var offsetY = 0;\n"
        "        var win = elems[i].ownerDocument.defaultView;\n"
        "        while(win && top !== win){\n"
        "            var rect = win.frameElement.getBoundingClientRect();\n"
        "            offsetX += rect.left;\n"
        "            offsetY += rect.top;\n"
        "            win = win.parent;\n"
        "        }\n"
        "        var rect = elems[i].getBoundingClientRect();\n"
        "        data.x = (rect.left + offsetX) * %3;\n"
        "        data.y = (rect.top  + offsetY) * %3;\n"
        "        data.width  = rect.width  * %3;\n"
        "        data.height = rect.height * %3;\n"
        "        data.region = {};\n"
        "        if(data.width && data.height){\n"
        "            var rects = elems[i].getClientRects();\n"
        "            for(var j = 0; j < rects.length; j++){\n"
        "                data.region[j] = {};\n"
        "                data.region[j].x = (rects[j].left + offsetX) * %3;\n"
        "                data.region[j].y = (rects[j].top  + offsetY) * %3;\n"
        "                data.region[j].width  = rects[j].width  * %3;\n"
        "                data.region[j].height = rects[j].height * %3;\n"
        "            }\n"
        "            var w = elems[i].ownerDocument.defaultView;\n"
        "            var r1 = { left: 0, top: 0, right: w.innerWidth, bottom: w.innerHeight};\n"
        "            var r2 = elems[i].getBoundingClientRect();\n"
        "            if(%2 &&\n"
        "               (Math.max(r1.left, r2.left) >= Math.min(r1.right,  r2.right) ||\n"
        "                Math.max(r1.top,  r2.top)  >= Math.min(r1.bottom, r2.bottom))){\n"
        "                continue;\n"
        "            }\n"
        "        }\n"
        "        data.isTextInput = \n"
        "            (elems[i].tagName == \"TEXTAREA\" ||\n"
        "             (elems[i].tagName == \"INPUT\" &&\n"
        "              elems[i].type &&\n"
        "              (elems[i].type.toLowerCase() == \"text\" ||\n"
        "               elems[i].type.toLowerCase() == \"search\" ||\n"
        "               elems[i].type.toLowerCase() == \"password\"))) ? true : false;\n"
        "        data.isQueryInput =\n"
        "            (elems[i].tagName == \"INPUT\" &&\n"
        "             elems[i].type &&\n"
        "             (elems[i].type.toLowerCase() == \"text\" ||\n"
        "              elems[i].type.toLowerCase() == \"search\")) ? true : false;\n"
        "        data.isEditable = \n"
        "            (elems[i].isContentEditable ||\n"
        "             data.isTextInput ||\n"
        "             data.isQueryInput) ? true : false;\n"
        "        data.isFrame = \n"
        "            (elems[i].tagName == \"FRAME\" ||\n"
        "             elems[i].tagName == \"IFRAME\") ? true : false;\n"
        "        data.isLooped = elems[i].loop;\n"
        "        data.isPaused = elems[i].paused;\n"
        "        data.isMuted = elems[i].muted;\n"
        "        data.action = \n"
        "            (elems[i].href &&\n"
        "             elems[i].href.lastIndexOf &&\n"
        "             (elems[i].href.lastIndexOf(\"http:\", 0) === 0 ||\n"
        "              elems[i].href.lastIndexOf(\"https:\", 0) === 0)) ? \"None\" :\n"
        "            (elems[i].isContentEditable ||\n"
        "             elems[i].tagName == \"TEXTAREA\" ||\n"
        "             elems[i].tagName == \"OBJECT\"   ||\n"
        "             elems[i].tagName == \"EMBED\"    ||\n"
        "             elems[i].tagName == \"FRAME\"    ||\n"
        "             elems[i].tagName == \"IFRAME\"   ||\n"
        "             (elems[i].tagName == \"INPUT\" &&\n"
        "              elems[i].type &&\n"
        "              (elems[i].type.toLowerCase() == \"text\" ||\n"
        "               elems[i].type.toLowerCase() == \"search\" ||\n"
        "               elems[i].type.toLowerCase() == \"password\"))) ? \"Focus\" :\n"
        "            (elems[i].onclick ||\n"
        "             (elems[i].href &&\n"
        "              elems[i].href.lastIndexOf &&\n"
        "              elems[i].href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
        "             elems[i].tagName == \"BUTTON\" ||\n"
        "             elems[i].tagName == \"SELECT\" ||\n"
        "             elems[i].tagName == \"LABEL\"  ||\n"
        "             (elems[i].getAttribute &&\n"
        "              elems[i].getAttribute(\"role\") &&\n"
        "              (elems[i].getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
        "               elems[i].getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
        "               elems[i].getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
        "               elems[i].getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
        "               elems[i].getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
        "               elems[i].getAttribute(\"role\").toLowerCase() == \"tab\")) ||\n"
        "             (elems[i].tagName == \"INPUT\" &&\n"
        "              elems[i].type &&\n"
        "              (elems[i].type.toLowerCase() == \"checkbox\" ||\n"
        "               elems[i].type.toLowerCase() == \"radio\" ||\n"
        "               elems[i].type.toLowerCase() == \"file\" ||\n"
        "               elems[i].type.toLowerCase() == \"submit\" ||\n"
        "               elems[i].type.toLowerCase() == \"reset\" ||\n"
        "               elems[i].type.toLowerCase() == \"button\"))) ? \"Click\" :\n"
        "            (elems[i].onmouseover) ? \"Hover\" :\n"
        "            \"None\";\n"
        "        if(elems[i].tagName == \"FRAME\" || elems[i].tagName == \"IFRAME\"){\n"
        "            try{\n"
        "                var frameDocument = elems[i].contentDocument;\n"
        "                elems = elems.concat(collectElements(frameDocument, \"%1\"));\n"
        "            }\n"
        "            catch(e){}\n"
        "        }\n"
        "        data.xPath = elementPath(elems[i]);\n"
        "        map[i] = data;\n"
        "    }\n"
        "    return map;\n"
        "})();")).arg(quoted, ignoreOutOfView, fix);
}

QString View::HitElementJsCode(QPoint pos){
#if defined(Q_OS_WIN)
    QString fix = QStringLiteral("devicePixelRatio");
#elif defined(Q_OS_MAC)
    QString fix = QStringLiteral("(document.body.clientWidth / innerWidth)");
#else
    QString fix = QStringLiteral("1");
#endif
    return (QStringLiteral(
        "(function(){\n")
        + ElementPathJsCode() +
        QStringLiteral(
        "    var scrollX = document.documentElement.scrollLeft || document.body.scrollLeft;\n"
        "    var scrollY = document.documentElement.scrollTop || document.body.scrollTop;\n"
        "    var baseUrl = \"\";\n"
        "    var baseDocument = \"index.html\";\n"
        "    var base = document.getElementsByTagName(\"base\");\n"
        "    if(base.length > 0 && base[0].href){\n"
        "        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
        "    } else {\n"
        "        baseUrl = \n"
        "            location.protocol + \"//\" + location.hostname + \n"
        "            (location.port && \":\" + location.port) + \"/\";\n"
        "    }\n"
        "    var x = %1;\n"
        "    var y = %2;\n"
        )
        + ElementAtPointJsCode() +
        QStringLiteral(
        "    if(!elem) return;\n"
        "    var data = {};\n"
        "    data.tagName = elem.tagName;\n"
        "    data.innerText = elem.innerText;\n"
        "    var link = elem;\n"
        "    while(link){\n"
        "        if(link.href){\n"
        "            data.linkUrl = link.href;\n"
        "            data.linkHtml = link.outerHTML;\n"
        "            break;\n"
        "        }\n"
        "        link = link.parentNode;\n"
        "    }\n"
        "    var image = elem;\n"
        "    while(image){\n"
        "        if(image.src){\n"
        "            data.imageUrl = image.src;\n"
        "            data.imageHtml = image.outerHTML;\n"
        "            data.isMedia = image.tagName == \"VIDEO\" || image.tagName == \"AUDIO\";\n"
        "            break;\n"
        "        }\n"
        "        image = image.parentNode;\n"
        "    }\n"
        "    if(!data.imageUrl){\n")
        + MediaAtJsFunction() +
        QStringLiteral(
        "        var player = mediaAt(elem.ownerDocument, x, y, true);\n"
        "        if(player){\n"
        "            data.imageUrl = player.currentSrc || player.src;\n"
        "            data.imageHtml = player.outerHTML;\n"
        "            data.isMedia = true;\n"
        "        }\n"
        "    }\n"
        "    data.baseUrl = baseUrl;\n"
        "    var offsetX = 0;\n"
        "    var offsetY = 0;\n"
        "    win = elem.ownerDocument.defaultView;\n"
        "    while(win && top !== win){\n"
        "        var rect = win.frameElement.getBoundingClientRect();\n"
        "        offsetX += rect.left + (win.frameElement.clientLeft || 0);\n"
        "        offsetY += rect.top + (win.frameElement.clientTop || 0);\n"
        "        win = win.parent;\n"
        "    }\n"
        "    var rect = elem.getBoundingClientRect();\n"
        "    data.x = (rect.left + offsetX) * %3;\n"
        "    data.y = (rect.top  + offsetY) * %3;\n"
        "    data.width = rect.width   * %3;\n"
        "    data.height = rect.height * %3;\n"
        "    data.region = {};\n"
        "    var rects = elem.getClientRects();\n"
        "    for(var i = 0; i < rects.length; i++){\n"
        "        data.region[i] = {};\n"
        "        data.region[i].x = (rects[i].left + offsetX) * %3;\n"
        "        data.region[i].y = (rects[i].top  + offsetY) * %3;\n"
        "        data.region[i].width  = rects[i].width  * %3;\n"
        "        data.region[i].height = rects[i].height * %3;\n"
        "    }\n"
        "    data.isTextInput = \n"
        "        (elem.tagName == \"TEXTAREA\" ||\n"
        "         (elem.tagName == \"INPUT\" &&\n"
        "          elem.type &&\n"
        "          (elem.type.toLowerCase() == \"text\" ||\n"
        "           elem.type.toLowerCase() == \"search\" ||\n"
        "           elem.type.toLowerCase() == \"password\"))) ? true : false;\n"
        "    data.isQueryInput = \n"
        "        (elem.tagName == \"INPUT\" &&\n"
        "         elem.type &&\n"
        "         (elem.type.toLowerCase() == \"text\" ||\n"
        "          elem.type.toLowerCase() == \"search\")) ? true : false;\n"
        "    data.isEditable = \n"
        "        (elem.isContentEditable ||\n"
        "         data.isTextInput ||\n"
        "         data.isQueryInput) ? true : false;\n"
        "    data.isFrame = \n"
        "        (elem.tagName == \"FRAME\" ||\n"
        "         elem.tagName == \"IFRAME\") ? true : false;\n"
        "    var deep = elem, dx = x, dy = y;\n"
        "    for(var n = 0; n < 8 && deep; n++){\n"
        "        var inner = null;\n"
        "        if(deep.shadowRoot && deep.shadowRoot.elementFromPoint){\n"
        "            inner = deep.shadowRoot.elementFromPoint(dx, dy);\n"
        "        } else if(deep.tagName == \"FRAME\" || deep.tagName == \"IFRAME\"){\n"
        "            try{\n"
        "                var fr = deep.getBoundingClientRect();\n"
        "                dx -= fr.left + (deep.clientLeft || 0);\n"
        "                dy -= fr.top + (deep.clientTop || 0);\n"
        "                inner = deep.contentDocument.elementFromPoint(dx, dy);\n"
        "            } catch(err){}\n"
        "        }\n"
        "        if(!inner || inner === deep) break;\n"
        "        deep = inner;\n"
        "    }\n"
        "    var active = elem.ownerDocument.activeElement;\n"
        "    for(var n = 0; n < 8 && active; n++){\n"
        "        var next = null;\n"
        "        if(active.shadowRoot && active.shadowRoot.activeElement){\n"
        "            next = active.shadowRoot.activeElement;\n"
        "        } else if(active.tagName == \"FRAME\" || active.tagName == \"IFRAME\"){\n"
        "            try{ next = active.contentDocument.activeElement;} catch(err){}\n"
        "        }\n"
        "        if(!next || next === active) break;\n"
        "        active = next;\n"
        "    }\n"
        "    data.isFocusedField = !!active &&\n"
        "        (active === deep || (active.isContentEditable && active.contains && active.contains(deep))) &&\n"
        "        (active.isContentEditable || active.tagName == \"TEXTAREA\" ||\n"
        "         (active.tagName == \"INPUT\" &&\n"
        "          !/^(button|checkbox|color|file|hidden|image|radio|range|reset|submit)$/i.test(active.type || \"\")));\n"
        "    data.isLooped = elem.loop;\n"
        "    data.isPaused = elem.paused;\n"
        "    data.isMuted = elem.muted;\n"
        "    data.action = \n"
        "        (elem.href &&\n"
        "         elem.href.lastIndexOf &&\n"
        "         (elem.href.lastIndexOf(\"http:\", 0) === 0 ||\n"
        "          elem.href.lastIndexOf(\"https:\", 0) === 0)) ? \"None\" :\n"
        "        (elem.isContentEditable ||\n"
        "         elem.tagName == \"TEXTAREA\" ||\n"
        "         elem.tagName == \"OBJECT\"   ||\n"
        "         elem.tagName == \"EMBED\"    ||\n"
        "         elem.tagName == \"FRAME\"    ||\n"
        "         elem.tagName == \"IFRAME\"   ||\n"
        "         (elem.tagName == \"INPUT\" &&\n"
        "          elem.type &&\n"
        "          (elem.type.toLowerCase() == \"text\" ||\n"
        "           elem.type.toLowerCase() == \"search\" ||\n"
        "           elem.type.toLowerCase() == \"password\"))) ? \"Focus\" :\n"
        "        (elem.onclick ||\n"
        "         (elem.href &&\n"
        "          elem.href.lastIndexOf &&\n"
        "          elem.href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
        "         elem.tagName == \"BUTTON\" ||\n"
        "         elem.tagName == \"SELECT\" ||\n"
        "         elem.tagName == \"LABEL\"  ||\n"
        "         (elem.getAttribute &&\n"
        "          elem.getAttribute(\"role\") &&\n"
        "          (elem.getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
        "           elem.getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
        "           elem.getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
        "           elem.getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
        "           elem.getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
        "           elem.getAttribute(\"role\").toLowerCase() == \"tab\")) ||\n"
        "         (elem.tagName == \"INPUT\" &&\n"
        "          elem.type &&\n"
        "          (elem.type.toLowerCase() == \"checkbox\" ||\n"
        "           elem.type.toLowerCase() == \"radio\" ||\n"
        "           elem.type.toLowerCase() == \"file\" ||\n"
        "           elem.type.toLowerCase() == \"submit\" ||\n"
        "           elem.type.toLowerCase() == \"reset\" ||\n"
        "           elem.type.toLowerCase() == \"button\"))) ? \"Click\" :\n"
        "         (elem.onmouseover) ? \"Hover\" :\n"
        "         \"None\";\n"
        "    data.xPath = elementPath(elem);\n"
        "    return data;\n"
        "})();")).arg(pos.x()).arg(pos.y()).arg(fix);
}

QString View::HitLinkUrlJsCode(QPoint pos){
    return (QStringLiteral(
        "(function(){\n"
        "    var x = %1;\n"
        "    var y = %2;\n")
        + ElementAtPointJsCode() +
        QStringLiteral(
        "    while(elem){\n"
        "        if(elem.href) return elem.href;\n"
        "        elem = elem.parentNode;\n"
        "    }\n"
        "    return \"\";\n"
        "})();")).arg(pos.x()).arg(pos.y());
}

QString View::HitImageUrlJsCode(QPoint pos){
    return (QStringLiteral(
        "(function(){\n"
        "    var x = %1;\n"
        "    var y = %2;\n")
        + ElementAtPointJsCode() +
        QStringLiteral(
        "    while(elem){\n"
        "        if(elem.src) return elem.src;\n"
        "        elem = elem.parentNode;\n"
        "    }\n"
        "    return \"\";\n"
        "})();")).arg(pos.x()).arg(pos.y());
}

QString View::SelectedTextJsCode(){
    return QStringLiteral("getSelection().toString();");
}

QString View::SelectedHtmlJsCode(){
    return QStringLiteral(
        "(function(){\n"
        "    var div = document.createElement(\"div\");\n"
        "    var selection = getSelection();\n"
        "    if(!selection.rangeCount) return \"\";\n"
        "    div.appendChild(selection.getRangeAt(0).cloneContents());\n"
        "    return div.innerHTML;\n"
        "})();");
}

QString View::WholeTextJsCode(){
    return QStringLiteral("document.documentElement.innerText;");
}

QString View::WholeHtmlJsCode(){
    return QStringLiteral("document.documentElement.outerHTML;");
}

QString View::SelectionRegionJsCode(){
#if defined(Q_OS_WIN)
    QString fix = QStringLiteral("devicePixelRatio");
#elif defined(Q_OS_MAC)
    QString fix = QStringLiteral("(document.body.clientWidth / innerWidth)");
#else
    QString fix = QStringLiteral("1");
#endif
    return QStringLiteral(
        "(function(){\n"
        "    var map = {};\n"
        "    var selection = getSelection();\n"
        "    if(!selection.rangeCount) return map;\n"
        "    var rects = selection.getRangeAt(0).getClientRects();\n"
        "    for(var i = 0; i < rects.length; i++){\n"
        "        map[i] = {};\n"
        "        map[i].x = rects[i].left * %1;\n"
        "        map[i].y = rects[i].top  * %1;\n"
        "        map[i].width  = rects[i].width  * %1;\n"
        "        map[i].height = rects[i].height * %1;\n"
        "    }\n"
        "    return map;\n"
        "})();").arg(fix);
}

QString View::SetTextValueJsCode(const QString &xpath, const QString &text){
    QString quotedXpath = EscapeJsStringLiteral(xpath);
    QString quotedText = EscapeJsStringLiteral(text);
    return (QStringLiteral(
        "(function(){\n")
        + ResolveElementPathJsCode() +
        QStringLiteral(
        "    var elem = resolveElementPath(\"%1\").elem;\n"
        "    elem.setAttribute(\"value\", \"%2\");\n"
        "    elem.focus();\n"
        "})();")).arg(quotedXpath, quotedText);
}

QString View::ExecCommandJsCode(const QString &command){
    return QStringLiteral("document.execCommand(\"%1\");").arg(command);
}

QString View::ChangeTextDirectionJsCode(const QString &direction){
    return QStringLiteral(
        "(function(){\n"
        "    var node = document.getSelection().anchorNode;\n"
        "    if(!node) return;\n"
        "    if(node.nodeType !== 1) node = node.parentElement;\n"
        "    while(node && !node.isContentEditable) node = node.parentElement;\n"
        "    if(!node) node = document.documentElement;\n"
        "    node.setAttribute(\"dir\", \"%1\");\n"
        "})();").arg(direction);
}

QString View::MediaAtPointJsCode(const QPoint &point){
    return MediaAtJsFunction() + QStringLiteral(
        "    var media = %1 >= 0 ? mediaAt(document, %1, %2, false) : null;\n").arg(point.x()).arg(point.y());
}

QString View::MediaAtJsFunction(){
    return QStringLiteral(
        "    function mediaAt(root, x, y, sourced, depth){\n"
        "        depth = depth || 0;\n"
        "        if(!root || !root.elementsFromPoint || depth > 4) return null;\n"
        "        var stack = root.elementsFromPoint(x, y);\n"
        "        for(var i = 0; i < stack.length; i++){\n"
        "            var e = stack[i], found = null;\n"
        "            if(e.tagName == \"VIDEO\" || e.tagName == \"AUDIO\"){\n"
        "                if(!sourced || e.currentSrc || e.src) return e;\n"
        "            } else if(e.shadowRoot && e.shadowRoot !== root){\n"
        "                found = mediaAt(e.shadowRoot, x, y, sourced, depth + 1);\n"
        "            } else if(e.tagName == \"IFRAME\" || e.tagName == \"FRAME\"){\n"
        "                try{\n"
        "                    var r = e.getBoundingClientRect();\n"
        "                    found = mediaAt(e.contentDocument, x - r.left - (e.clientLeft || 0),\n"
        "                                    y - r.top - (e.clientTop || 0), sourced, depth + 1);\n"
        "                } catch(err){}\n"
        "            }\n"
        "            if(found) return found;\n"
        "        }\n"
        "        return null;\n"
        "    }\n");
}

QString View::MediaElementJsCode(const QString &statement,
                                 const QPoint &point){
    return QStringLiteral(
        "(function(){\n")
        + MediaAtPointJsCode(point) +
        QStringLiteral(
        "    if(media){\n"
        "        %1\n"
        "        return;\n"
        "    }\n"
        "    var all = Array.prototype.slice.call(\n"
        "        document.querySelectorAll(\"video, audio\"));\n"
        "    if(!all.length) return;\n"
        "    function area(e){\n"
        "        var r = e.getBoundingClientRect();\n"
        "        return r.width * r.height;\n"
        "    }\n"
        "    var playing = all.filter(function(e){ return !e.paused;});\n"
        "    var from = playing.length ? playing : all;\n"
        "    media = from.reduce(function(a, b){\n"
        "        return area(b) > area(a) ? b : a;\n"
        "    });\n"
        "    if(!media) return;\n"
        "    %1\n"
        "})();").arg(statement);
}

QString View::ToggleMediaControlsJsCode(const QPoint &point){
    return MediaElementJsCode(QStringLiteral("media.controls = !media.controls;"), point);
}

QString View::ToggleMediaLoopJsCode(const QPoint &point){
    return MediaElementJsCode(QStringLiteral("media.loop = !media.loop;"), point);
}

QString View::ToggleMediaPlayPauseJsCode(const QPoint &point){
    return MediaElementJsCode
        (QStringLiteral("if(media.paused) media.play(); else media.pause();"), point);
}

QString View::ToggleMediaMuteAtJsCode(const QPoint &point){
    return QStringLiteral("(function(){\n")
        + MediaAtPointJsCode(point) +
        QStringLiteral(
        "    if(!media) return false;\n"
        "    media.muted = !media.muted;\n"
        "    return true;\n"
        "})();");
}

QString View::JsKeyCodeSet(const QList<int> &codes){
    QStringList entries;
    foreach(int code, codes)
        entries << QStringLiteral("%1:1").arg(code);
    return QStringLiteral("{%1}").arg(entries.join(QStringLiteral(",")));
}

QString View::FocusedElementJsCode(){
    return QStringLiteral(
            "var focusedElement = function(e, d){\n"
            "    var el = (e && e.composedPath) ? e.composedPath()[0] : null;\n"
            "    if(!el || !el.tagName) el = d ? d.activeElement : null;\n"
            "    while(el && el.shadowRoot && el.shadowRoot.activeElement)\n"
            "        el = el.shadowRoot.activeElement;\n"
            "    return el;\n"
            "};\n");
}

QString View::EdgeInputBridgeJsCode(const QList<int> &plain,
                                    const QList<int> &shifted){
    const QString plainSet = JsKeyCodeSet(plain);
    const QString shiftedSet = JsKeyCodeSet(shifted);

    return (QStringLiteral(
            "(function(){\n"
            "    if(!window.chrome || !window.chrome.webview) return;\n"
            "    var wv = window.chrome.webview;\n"
            "    var d = document;\n"
            "    var later = window.setTimeout;\n"
            "    var tag = %1;\n"
            "    var apply = Reflect.apply;\n"
            "    var own = Object.getOwnPropertyDescriptor;\n"
            "    var post = wv.postMessage;\n"
            "    var slot = function(type, name, key){\n"
            "        var p = type ? type.prototype : undefined;\n"
            "        var dsc = p ? own(p, name) : undefined;\n"
            "        return dsc ? dsc[key || \"get\"] : undefined;\n"
            "    };\n"
            "    var K = window.KeyboardEvent, E = window.Event, El = window.Element;\n"
            "    var G = {\n"
            "        keyCode: slot(K, \"keyCode\"), shiftKey: slot(K, \"shiftKey\"),\n"
            "        ctrlKey: slot(K, \"ctrlKey\"), altKey: slot(K, \"altKey\"),\n"
            "        metaKey: slot(K, \"metaKey\"), repeat: slot(K, \"repeat\"),\n"
            "        isComposing: slot(K, \"isComposing\"),\n"
            "        defaultPrevented: slot(E, \"defaultPrevented\"),\n"
            "        composedPath: slot(E, \"composedPath\", \"value\"),\n"
            "        preventDefault: slot(E, \"preventDefault\", \"value\"),\n"
            "        tagName: slot(El, \"tagName\"), shadowRoot: slot(El, \"shadowRoot\"),\n"
            "        isContentEditable: slot(window.HTMLElement, \"isContentEditable\"),\n"
            "        type: slot(window.HTMLInputElement, \"type\"),\n"
            "        documentActive: slot(window.Document, \"activeElement\"),\n"
            "        rootActive: slot(window.ShadowRoot, \"activeElement\")\n"
            "    };\n"
            "    var read = function(o, name, get){\n"
            "        if(o === null || o === undefined) return undefined;\n"
            "        if(!get) return o[name];\n"
            "        try { return apply(get, o, []); } catch(x){ return undefined; }\n"
            "    };\n"
            "    var invoke = function(o, name, fn){\n"
            "        if(o === null || o === undefined) return undefined;\n"
            "        if(!fn) fn = o[name];\n"
            "        if(typeof fn !== \"function\") return undefined;\n"
            "        try { return apply(fn, o, []); } catch(x){ return undefined; }\n"
            "    };\n"
            "    var send = function(message){ apply(post, wv, [message]); };\n"
            "    var plainSet = Object.setPrototypeOf(%3, null);\n"
            "    var shiftedSet = Object.setPrototypeOf(%2, null);\n"
            "    var notText = Object.setPrototypeOf({button: 1, submit: 1, reset: 1, checkbox: 1,\n"
            "        radio: 1, image: 1, file: 1, color: 1, range: 1}, null);\n"
            "    var focusedElement = function(e){\n"
            "        var path = invoke(e, \"composedPath\", G.composedPath);\n"
            "        var el = path ? path[0] : null;\n"
            "        if(!el || !read(el, \"tagName\", G.tagName))\n"
            "            el = read(d, \"activeElement\", G.documentActive);\n"
            "        for(;;){\n"
            "            var root = read(el, \"shadowRoot\", G.shadowRoot);\n"
            "            var inner = read(root, \"activeElement\", G.rootActive);\n"
            "            if(!inner) return el;\n"
            "            el = inner;\n"
            "        }\n"
            "    };\n"
            "    if(window === window.top){\n"
            "    d.addEventListener(\"keydown\", function(e){\n"
            "        // a key the page made up is not a key (D-355). 'isTrusted' is the\n"
            "        // event's own and cannot be redefined.\n"
            "        if(!e.isTrusted) return;\n"
            "        var c = read(e, \"keyCode\", G.keyCode);\n"
            "        var shift = !!read(e, \"shiftKey\", G.shiftKey);\n"
            "        // the IME owns the keystroke while it is composing.\n"
            "        if(read(e, \"isComposing\", G.isComposing) || c == 229) return;\n"
            "        // a held key acts once.\n"
            "        if(read(e, \"repeat\", G.repeat)) return;\n"
            "        // these are the accelerator's. reporting them here is the double\n"
            "        // arrival this file exists to avoid.\n"
            "        if(read(e, \"ctrlKey\", G.ctrlKey) || read(e, \"altKey\", G.altKey) ||\n"
            "           read(e, \"metaKey\", G.metaKey)) return;\n"
            "        var wanted = shift ? shiftedSet : plainSet;\n"
            "        if(!wanted[c]) return;\n"
            "        if(c == 27){\n"
            "            later(function(){\n"
            "                if(read(e, \"defaultPrevented\", G.defaultPrevented)) return;\n"
            "                send({\n"
            "                    v: 1, tag: tag, kind: \"key\",\n"
            "                    code: 27, shift: false});\n"
            "            }, 0);\n"
            "            return;\n"
            "        }\n"
            "        // where the caret is decides what the key means.\n"
            "        var el = focusedElement(e);\n"
            "        var name = read(el, \"tagName\", G.tagName);\n"
            "        var typed = el && (read(el, \"isContentEditable\", G.isContentEditable) ||\n"
            "                  name == \"TEXTAREA\" ||\n"
            "                  name == \"SELECT\" ||\n"
            "                  name == \"FRAME\" ||\n"
            "                  name == \"IFRAME\" ||\n"
            "                  (name == \"INPUT\" &&\n"
            "                   !notText[read(el, \"type\", G.type) || \"\"]));\n"
            "        if(typed) return;\n"
            "        if(el && (name == \"BUTTON\" || name == \"INPUT\")){\n"
            "            later(function(){\n"
            "                if(read(e, \"defaultPrevented\", G.defaultPrevented)) return;\n"
            "                send({\n"
            "                    v: 1, tag: tag, kind: \"key\",\n"
            "                    code: c, shift: shift});\n"
            "            }, 0);\n"
            "            return;\n"
            "        }\n"
            "        send({\n"
            "            v: 1, tag: tag, kind: \"key\",\n"
            "            code: c, shift: shift});\n"
            "        invoke(e, \"preventDefault\", G.preventDefault);\n"
            "    }, false);\n"
            "    }\n"
            "    window.print = function(){\n"
            "        send({\n"
            "            v: 1, tag: tag, kind: \"print\"});\n"
            "    };\n"
            "})();\n"
        )).arg(EdgeBridgeTag()).arg(shiftedSet, plainSet);
}

QString View::EdgeScrollReportJsCode(){
    return QStringLiteral(
            "(function(){\n"
            "    if(window !== window.top) return;\n"
            "    if(!window.chrome || !window.chrome.webview) return;\n"
            "    var wv = window.chrome.webview;\n"
            "    var last = \"\";\n"
            "    function report(){\n"
            "        var de = document.documentElement;\n"
            "        if(!de) return;\n"
            "        var b = document.body;\n"
            "        var m = { v: 1, tag: %1, kind: \"scroll\",\n"
            "                  x: Math.max(0, window.pageXOffset),\n"
            "                  y: Math.max(0, window.pageYOffset),\n"
            "                  w: Math.max(de.scrollWidth, b ? b.scrollWidth : 0),\n"
            "                  h: Math.max(de.scrollHeight, b ? b.scrollHeight : 0),\n"
            "                  vw: window.innerWidth, vh: window.innerHeight };\n"
            "        var key = location.href+\"|\"+m.x+\",\"+m.y+\",\"+m.w+\",\"+m.h+\",\"+m.vw+\",\"+m.vh;\n"
            "        if(key === last) return;\n"
            "        last = key;\n"
            "        wv.postMessage(m);\n"
            "    }\n"
            "    var pending = false;\n"
            "    function schedule(){\n"
            "        if(pending) return;\n"
            "        pending = true;\n"
            "        requestAnimationFrame(function(){ pending = false; report();});\n"
            "    }\n"
            "    window.addEventListener(\"scroll\", schedule, {passive: true, capture: true});\n"
            "    window.addEventListener(\"resize\", schedule);\n"
            "    window.addEventListener(\"load\", schedule);\n"
            "    document.addEventListener(\"DOMContentLoaded\", schedule);\n"
            "    setInterval(schedule, 500);\n"
            "})();\n"
        ).arg(EdgeBridgeTag());
}

QString View::EdgeHideWebViewJsCode(){
    return QStringLiteral(
            "(function(){\n"
            "    if(!window.chrome || !window.chrome.webview) return;\n"
            "    try { delete window.chrome.webview; } catch(e){}\n"
            "})();\n");
}

QString View::ExtensionPopupSizeJsCode(){
    return QStringLiteral(
            "(function(){\n"
            "    var root = document.documentElement;\n"
            "    if(!root || !document.body) return null;\n"
            "    var text = root.getAttribute(\"style\");\n"
            "    var bar = Math.max(0, window.innerWidth - root.clientWidth);\n"
            "    var width, height;\n"
            "    try {\n"
            "        root.style.setProperty(\"height\", \"auto\", \"important\");\n"
            "        root.style.setProperty(\"width\", \"min-content\", \"important\");\n"
            "        width = Math.min(Math.max(root.getBoundingClientRect().width, 25), 800);\n"
            "        root.style.setProperty(\"width\", Math.ceil(width) + \"px\", \"important\");\n"
            "        height = root.getBoundingClientRect().height;\n"
            "    } finally {\n"
            "        root.getAttribute(\"style\");\n"
            "        if(text !== null) root.setAttribute(\"style\", text); else root.removeAttribute(\"style\");\n"
            "    }\n"
            "    if(height > 600){ height = 600; width = Math.min(width + bar, 800); }\n"
            "    height = Math.max(height, 25);\n"
            "    var ratio = window.devicePixelRatio || 1;\n"
            "    return Math.ceil(width * ratio) + \",\" + Math.ceil(height * ratio);\n"
            "})()"
        );
}

QString View::EdgeExtensionTabQueryJsCode(){
    return QStringLiteral(
            "(function(){\n"
            "    if(window !== window.top) return;\n"
            "    var chrome = window.chrome;\n"
            "    if(!chrome || !chrome.tabs || typeof chrome.tabs.query !== \"function\" ||\n"
            "       !chrome.webview || typeof chrome.webview.postMessage !== \"function\") return;\n"
            "    var tabs = chrome.tabs;\n"
            "    var original = tabs.query;\n"
            "    var manifest = (chrome.runtime && typeof chrome.runtime.getManifest === \"function\")\n"
            "                   ? chrome.runtime.getManifest() : null;\n"
            "    var permissions = (manifest && Array.isArray(manifest.permissions)) ? manifest.permissions : [];\n"
            "    var hosts = (manifest && Array.isArray(manifest.host_permissions)) ? manifest.host_permissions : [];\n"
            "    var seesAll = permissions.indexOf(\"tabs\") >= 0;\n"
            "    if(!seesAll && !hosts.length) return;\n"
            "    var CURRENT = (chrome.windows && typeof chrome.windows.WINDOW_ID_CURRENT === \"number\")\n"
            "                  ? chrome.windows.WINDOW_ID_CURRENT : -2;\n"
            "    var mark = Math.random().toString(36).slice(2) + Date.now().toString(36);\n"
            "    var sequence = 0;\n"
            "    var pending = {};\n"
            "    function settle(seq, id){\n"
            "        var request = pending[seq];\n"
            "        if(!request) return;\n"
            "        delete pending[seq];\n"
            "        clearTimeout(request.timer);\n"
            "        request.take(id);\n"
            "    }\n"
            "    chrome.webview.addEventListener(\"message\", function(event){\n"
            "        var data = event.data;\n"
            "        if(!data || data.vanilla !== \"extension-tab\" || data.doc !== mark ||\n"
            "           typeof data.seq !== \"number\") return;\n"
            "        settle(data.seq, typeof data.id === \"number\" ? data.id : -1);\n"
            "    });\n"
            "    function ask(candidates, take){\n"
            "        var seq = ++sequence;\n"
            "        pending[seq] = { take: take, timer: setTimeout(function(){ settle(seq, -1);}, 2000) };\n"
            "        chrome.webview.postMessage({ vanilla: \"extension-tab\", doc: mark, seq: seq, tabs: candidates });\n"
            "    }\n"
            "    function wantsCurrent(info){\n"
            "        return !!info && typeof info === \"object\" &&\n"
            "               (info.active === true || info.currentWindow === true ||\n"
            "                info.lastFocusedWindow === true || info.windowId === CURRENT);\n"
            "    }\n"
            "    function rest(info, all){\n"
            "        var out = {};\n"
            "        Object.keys(info).forEach(function(key){\n"
            "            var value = info[key];\n"
            "            if(!all && (key === \"active\" || key === \"currentWindow\" || key === \"lastFocusedWindow\") && value === true) return;\n"
            "            if(!all && key === \"windowId\" && value === CURRENT) return;\n"
            "            out[key] = Array.isArray(value) ? value.slice() : value;\n"
            "        });\n"
            "        return out;\n"
            "    }\n"
            "    function resolve(info, done, fail){\n"
            "        var conditions = rest(info, false);\n"
            "        var asked = rest(info, true);\n"
            "        var settled = false;\n"
            "        function finish(take, value){\n"
            "            if(settled) return;\n"
            "            settled = true;\n"
            "            take(value);\n"
            "        }\n"
            "        function call(query, take){\n"
            "            try { original.call(tabs, query, take); }\n"
            "            catch(e){ if(settled) throw e; finish(fail, e); }\n"
            "        }\n"
            "        call({}, function(list){\n"
            "            var error = chrome.runtime.lastError;\n"
            "            if(error){ finish(fail, error); return; }\n"
            "            var candidates = [];\n"
            "            (Array.isArray(list) ? list : []).forEach(function(tab){\n"
            "                if(typeof tab.id !== \"number\" || typeof tab.url !== \"string\") return;\n"
            "                candidates.push({ id: tab.id, url: tab.url });\n"
            "            });\n"
            "            ask(candidates, function(id){\n"
            "                if(id < 0 && !seesAll){\n"
            "                    call(asked, function(list){\n"
            "                        var error = chrome.runtime.lastError;\n"
            "                        if(error){ finish(fail, error); return; }\n"
            "                        finish(done, Array.isArray(list) ? list : []);\n"
            "                    });\n"
            "                    return;\n"
            "                }\n"
            "                if(id < 0){ finish(done, []); return; }\n"
            "                call(conditions, function(filtered){\n"
            "                    var error = chrome.runtime.lastError;\n"
            "                    if(error){ finish(fail, error); return; }\n"
            "                    finish(done, (Array.isArray(filtered) ? filtered : []).filter(function(tab){ return tab.id === id;}));\n"
            "                });\n"
            "            });\n"
            "        });\n"
            "    }\n"
            "    tabs.query = function(info, callback){\n"
            "        if(!wantsCurrent(info)) return original.apply(tabs, arguments);\n"
            "        if(typeof callback === \"function\"){\n"
            "            resolve(info, callback, function(){ callback([]);});\n"
            "            return;\n"
            "        }\n"
            "        return new Promise(function(fulfil, reject){\n"
            "            resolve(info, fulfil, function(error){\n"
            "                reject(new Error(error && error.message ? error.message : String(error)));\n"
            "            });\n"
            "        });\n"
            "    };\n"
            "})();\n"
        );
}

QString View::InstallEventFilterJsCode(const QList<QEvent::Type> &types){
    QString inner;
    if(types.contains(QEvent::KeyPress) || types.contains(QEvent::KeyRelease))
        inner += QStringLiteral("\n") + FocusedElementJsCode();
    if(types.contains(QEvent::KeyPress))
        inner += QStringLiteral(
            "\n"
            "doc.addEventListener(\"keydown\", function(e){\n"
            "    if(!e.isTrusted) return;\n"
            "    var prevent = false;\n"
            "    var elem = focusedElement(e, e.target.ownerDocument);\n"
            "    if(e.keyCode == 9 || e.keyCode == 13){\n"
            "        prevent = false;\n"
            "        console.info(\"keyPressEvent%1,\" + \n"
            "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
            "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
            "    } else if(!e.altKey && !e.ctrlKey && !e.metaKey &&\n"
            "       32 <= e.keyCode && e.keyCode <= 40){\n"
            "        prevent = false;\n"
            "        console.info(\"preventScrollRestoration%1\");\n"
            "    } else if(elem.isContentEditable ||\n"
            "              elem.tagName == \"BUTTON\" ||\n"
            "              elem.tagName == \"SELECT\" ||\n"
            "              elem.tagName == \"INPUT\"  ||\n"
            "              elem.tagName == \"TEXTAREA\" ||\n"
            "              elem.tagName == \"FRAME\" ||\n"
            "              elem.tagName == \"IFRAME\"){\n"
            "        if(e.ctrlKey || e.altKey || e.metaKey){\n"
            "            prevent = true;\n"
            "        }\n"
            "    } else {\n"
            "        prevent = true;\n"
            "    }\n"
            "    if(prevent){\n"
            "        console.info(\"keyPressEvent%1,\" + \n"
            "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
            "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
            "        e.preventDefault();\n"
            "    }\n"
            "}, false);\n");

    if(types.contains(QEvent::KeyRelease))
        inner += QStringLiteral(
            "\n"
            "doc.addEventListener(\"keyup\", function(e){\n"
            "    if(!e.isTrusted) return;\n"
            "    var prevent = false;\n"
            "    var elem = focusedElement(e, e.target.ownerDocument);\n"
            "    if(e.keyCode == 9 || e.keyCode == 13){\n"
            "        prevent = false;\n"
            "        console.info(\"keyReleaseEvent%1,\" + \n"
            "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
            "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
            "    } else if(!e.altKey && !e.ctrlKey && !e.metaKey &&\n"
            "       32 <= e.keyCode && e.keyCode <= 40){\n"
            "        prevent = false;\n"
            "        console.info(\"keyReleaseEvent%1,\" + \n"
            "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
            "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
            "    } else if(elem.isContentEditable ||\n"
            "              elem.tagName == \"BUTTON\" ||\n"
            "              elem.tagName == \"SELECT\" ||\n"
            "              elem.tagName == \"INPUT\"  ||\n"
            "              elem.tagName == \"TEXTAREA\" ||\n"
            "              elem.tagName == \"FRAME\" ||\n"
            "              elem.tagName == \"IFRAME\"){\n"
            "        if(e.ctrlKey || e.altKey || e.metaKey){\n"
            "            prevent = true;\n"
            "        }\n"
            "    } else {\n"
            "        prevent = true;\n"
            "    }\n"
            "    if(prevent){\n"
            "        console.info(\"keyReleaseEvent%1,\" + \n"
            "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
            "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
            "        e.preventDefault();\n"
            "    }\n"
            "}, false);\n");

    if(types.contains(QEvent::MouseMove))
        inner += QStringLiteral(
            "\n"
            "doc.addEventListener(\"mousemove\", function(e){\n"
            "    if(!e.isTrusted) return;\n"
            "    console.info(\"mouseMoveEvent%1,\" + \n"
            "                 e.button.toString() + \",\" + \n"
            "                 e.clientX.toString() + \",\" + e.clientY.toString() + \",\" + \n"
            "                 e.shiftKey.toString() + \",\" + e.ctrlKey.toString() + \",\" + \n"
            "                 e.altKey.toString() + \",\" + e.metaKey.toString());\n"
            "}, false);\n");

    if(types.contains(QEvent::MouseButtonPress))
        inner += QStringLiteral(
            "\n"
            "doc.addEventListener(\"mousedown\", function(e){\n"
            "    if(!e.isTrusted) return;\n"
            "    console.info(\"mousePressEvent%1,\" + \n"
            "                 e.button.toString() + \",\" + \n"
            "                 e.clientX.toString() + \",\" + e.clientY.toString() + \",\" + \n"
            "                 e.shiftKey.toString() + \",\" + e.ctrlKey.toString() + \",\" + \n"
            "                 e.altKey.toString() + \",\" + e.metaKey.toString());\n"
            "}, false);\n");

    if(types.contains(QEvent::MouseButtonRelease))
        inner += QStringLiteral(
            "\n"
            "doc.addEventListener(\"mouseup\", function(e){\n"
            "    if(!e.isTrusted) return;\n"
            "    console.info(\"mouseReleaseEvent%1,\" + \n"
            "                 e.button.toString() + \",\" + \n"
            "                 e.clientX.toString() + \",\" + e.clientY.toString() + \",\" + \n"
            "                 e.shiftKey.toString() + \",\" + e.ctrlKey.toString() + \",\" + \n"
            "                 e.altKey.toString() + \",\" + e.metaKey.toString());\n"
            "}, false);\n");

    if(types.contains(QEvent::Wheel))
        inner += QStringLiteral(
            "\n"
            "doc.addEventListener(\"mousewheel\", function(e){\n"
            "    if(!e.isTrusted) return;\n"
            "    console.info(\"wheelEvent%1,\" + e.wheelDelta.toString());\n"
            "}, false);\n");

    return QStringLiteral(
            "(function(){\n"
            "    var wins = [window];\n"
            "    wins = wins.concat(Array.from(frames));\n"
            "    for(var i = 0; i < wins.length; i++){\n"
            "        try{\n"
            "            var doc = wins[i].document;\n"
            "            %1\n"
            "        }\n"
            "        catch(e){}\n"
            "    }\n"
            "})();").arg(inner.arg(Application::EventToken()));
}
