#include "switch.hpp"

#include "extensionui.hpp"

#include "extensionhostwire.hpp"
#include "extensionmainscripts.hpp"

#include <QDateTime>
#include <QJsonValue>
#include <QtCore/qnamespace.h>
#include <QtMath>
#include <QRegularExpression>

namespace ExtensionUi {

namespace {

    QJsonObject Ok(const QJsonValue &value = QJsonValue(QJsonValue::Undefined)){
        QJsonObject reply;
        reply[QStringLiteral("ok")] = true;
        if(!value.isUndefined()) reply[QStringLiteral("value")] = value;
        return reply;
    }
    QJsonObject Fail(const QString &error){
        QJsonObject reply;
        reply[QStringLiteral("ok")] = false;
        reply[QStringLiteral("error")] = error;
        return reply;
    }

    const QStringList &ContextNames(){
        static const QStringList names = QStringList()
            << QStringLiteral("all") << QStringLiteral("page") << QStringLiteral("frame") << QStringLiteral("selection")
            << QStringLiteral("link") << QStringLiteral("editable") << QStringLiteral("image") << QStringLiteral("video")
            << QStringLiteral("audio") << QStringLiteral("launcher") << QStringLiteral("browser_action")
            << QStringLiteral("page_action") << QStringLiteral("action");
        return names;
    }
    const QStringList &TypeNames(){
        static const QStringList names = QStringList()
            << QStringLiteral("normal") << QStringLiteral("checkbox") << QStringLiteral("radio") << QStringLiteral("separator");
        return names;
    }

    bool Strings(const QJsonValue &value, QStringList *out){
        if(!value.isArray()) return false;
        out->clear();
        foreach(const QJsonValue &one, value.toArray()){
            if(!one.isString()) return false;
            out->append(one.toString());
        }
        return true;
    }

    bool ReadablePattern(const QString &pattern){
        bool ok = false;
        ExtensionHostWire::Matches(pattern, QUrl(QStringLiteral("https://a.example/")), &ok);
        return ok;
    }

    bool AnyMatches(const QStringList &patterns, const QUrl &url){
        foreach(const QString &pattern, patterns){
            bool ok = false;
            if(ExtensionHostWire::Matches(pattern, url, &ok) && ok) return true;
        }
        return false;
    }
}

const ActionValues *Action::OfTab(const TabNow &tab){
    if(tab.id <= 0 || !m_Tabs.contains(tab.id)) return nullptr;
    if(m_Tabs[tab.id].url != tab.url){
        m_Tabs.remove(tab.id);
        m_Order.removeAll(tab.id);
        return nullptr;
    }
    return &m_Tabs[tab.id].values;
}

ActionValues &Action::Layer(const TabNow &tab){
    if(tab.id <= 0) return m_All;
    if(m_Tabs.contains(tab.id)){
        TabLayer &layer = m_Tabs[tab.id];
        if(layer.url != tab.url){
            layer.url = tab.url;
            layer.values = ActionValues();
        }
        return layer.values;
    }
    while(m_Order.size() >= ACTION_TABS_KEPT){
        m_Tabs.remove(m_Order.takeFirst());
    }
    m_Order.append(tab.id);
    TabLayer &layer = m_Tabs[tab.id];
    layer.url = tab.url;
    return layer.values;
}

ActionShown Action::Shown(const TabNow &tab){
    ActionShown shown = ShownForAll();
    const ActionValues *of = OfTab(tab);
    if(!of) return shown;
    if(of->hasIcon){ shown.hasIcon = true; shown.icon = of->icon; }
    if(of->hasBadgeText) shown.badgeText = of->badgeText;
    if(of->hasBadgeColor) shown.badgeColor = of->badgeColor;
    if(of->hasTextColor) shown.textColor = of->textColor;
    if(of->hasTitle){ shown.hasTitle = true; shown.title = of->title; }
    if(of->hasEnabled) shown.enabled = of->enabled;
    return shown;
}

ActionShown Action::ShownForAll() const {
    ActionShown shown;
    if(m_All.hasIcon){ shown.hasIcon = true; shown.icon = m_All.icon; }
    if(m_All.hasBadgeText) shown.badgeText = m_All.badgeText;
    if(m_All.hasBadgeColor) shown.badgeColor = m_All.badgeColor;
    if(m_All.hasTextColor) shown.textColor = m_All.textColor;
    if(m_All.hasTitle){ shown.hasTitle = true; shown.title = m_All.title; }
    if(m_All.hasEnabled) shown.enabled = m_All.enabled;
    return shown;
}

void Action::SetIcon(const TabNow &tab, const Icon &icon){
    ActionValues &layer = Layer(tab);
    layer.hasIcon = true;
    layer.icon = icon;
}
void Action::SetBadgeText(const TabNow &tab, const QString &text){
    ActionValues &layer = Layer(tab);
    layer.hasBadgeText = true;
    layer.badgeText = text;
}
void Action::SetBadgeColor(const TabNow &tab, const QString &color){
    ActionValues &layer = Layer(tab);
    layer.hasBadgeColor = true;
    layer.badgeColor = color;
}
void Action::SetTextColor(const TabNow &tab, const QString &color){
    ActionValues &layer = Layer(tab);
    layer.hasTextColor = true;
    layer.textColor = color;
}
void Action::SetTitle(const TabNow &tab, const QString &title){
    ActionValues &layer = Layer(tab);
    layer.hasTitle = true;
    layer.title = title;
}
void Action::SetEnabled(const TabNow &tab, bool enabled){
    ActionValues &layer = Layer(tab);
    layer.hasEnabled = true;
    layer.enabled = enabled;
}
void Action::UnsetBadgeText(const TabNow &tab){
    ActionValues &layer = Layer(tab);
    layer.hasBadgeText = false;
    layer.badgeText.clear();
}
void Action::UnsetTitle(const TabNow &tab){
    ActionValues &layer = Layer(tab);
    layer.hasTitle = false;
    layer.title.clear();
}

QSize PopupSizeOf(const QString &measured, qreal ratio, qreal scale){
    static const QRegularExpression shape(QStringLiteral("\\A\"?([0-9]{1,5}),([0-9]{1,5})\"?\\z"));
    const QRegularExpressionMatch parts = shape.match(measured);
    if(!parts.hasMatch() || !(ratio > 0) || !(scale > 0)) return QSize();
    int width = parts.captured(1).toInt(), height = parts.captured(2).toInt();
    if(width < 1 || height < 1 || width > 4000 || height > 4000) return QSize();
    width = qMin(width, qCeil(800 * scale));
    height = qMin(height, qCeil(600 * scale));
    return QSize(qCeil(width / ratio), qCeil(height / ratio));
}

QString ColorOf(const QJsonValue &value){
    if(value.isString()){
        static const QRegularExpression hex(QStringLiteral("\\A#([0-9a-fA-F]{3}|[0-9a-fA-F]{6}|[0-9a-fA-F]{8})\\z"));
        const QRegularExpressionMatch m = hex.match(value.toString());
        if(!m.hasMatch()) return QString();
        QString digits = m.captured(1).toLower();
        if(digits.size() == 3){
            QString wide;
            foreach(const QChar c, digits){ wide += c; wide += c; }
            digits = wide;
        }
        if(digits.size() == 6) digits += QStringLiteral("ff");
        return QLatin1Char('#') + digits;
    }
    if(value.isArray()){
        const QJsonArray parts = value.toArray();
        if(parts.size() != 4) return QString();
        QString out = QStringLiteral("#");
        foreach(const QJsonValue &part, parts){
            if(!part.isDouble()) return QString();
            const double d = part.toDouble();
            if(d < 0 || d > 255 || d != static_cast<int>(d)) return QString();
            out += QStringLiteral("%1").arg(static_cast<int>(d), 2, 16, QLatin1Char('0'));
        }
        return out;
    }
    return QString();
}

namespace {
    QJsonArray ColorArray(const QString &folded){
        QJsonArray out;
        if(folded.size() != 9){
            for(int i = 0; i < 4; i++) out.append(0);
            return out;
        }
        for(int i = 0; i < 4; i++) out.append(folded.mid(1 + i * 2, 2).toInt(nullptr, 16));
        return out;
    }
}

QJsonObject GetOfAction(const QString &api, Action &action, const TabNow *tab, const QString &manifestTitle){
    const ActionShown shown = tab ? action.Shown(*tab) : action.ShownForAll();
    if(api == QStringLiteral("action.getBadgeText")) return Ok(shown.badgeText);
    if(api == QStringLiteral("action.getTitle")) return Ok(shown.hasTitle ? shown.title : manifestTitle);
    if(api == QStringLiteral("action.getBadgeBackgroundColor")) return Ok(ColorArray(shown.badgeColor));
    if(api == QStringLiteral("action.getBadgeTextColor")) return Ok(ColorArray(shown.textColor));
    if(api == QStringLiteral("action.isEnabled")) return Ok(shown.enabled);
    return Fail(QStringLiteral("chrome.%1 is not available in this browser").arg(api));
}

QString IconPathOf(const QJsonValue &path){
    if(path.isString()) return path.toString();
    if(!path.isObject()) return QString();
    int best = 0;
    QString chosen;
    const QJsonObject sizes = path.toObject();
    for(QJsonObject::const_iterator i = sizes.begin(); i != sizes.end(); ++i){
        bool ok = false;
        const int size = i.key().toInt(&ok);
        if(!ok || !i.value().isString()) continue;
        if(size > best && size <= ICON_SIZE_LIMIT){
            best = size;
            chosen = i.value().toString();
        }
    }
    return chosen;
}

QByteArray IconBytesOf(const QJsonValue &imageData){
    const QJsonValue png = imageData.toObject().value(QStringLiteral("png"));
    if(!png.isString()) return QByteArray();
    const QString text = png.toString();
    if(text.isEmpty() || text.size() > ICON_SENT_LIMIT) return QByteArray();
    const QByteArray::FromBase64Result decoded =
        QByteArray::fromBase64Encoding(text.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
    if(!decoded || !decoded.decoded.startsWith("\x89PNG\r\n\x1a\n")) return QByteArray();
    return decoded.decoded;
}

QString MenuIdOf(const QJsonValue &value){
    if(value.isString()) return value.toString();
    if(value.isDouble()){
        const double d = value.toDouble();
        if(d == static_cast<qint64>(d)) return QString::number(static_cast<qint64>(d));
    }
    return QString();
}

int Menus::IndexOf(const QString &id) const {
    for(int i = 0; i < m_Items.size(); i++)
        if(m_Items.at(i).id == id) return i;
    return -1;
}

namespace {
    QString ReadFields(const QJsonObject &properties, MenuItem *item, bool creating){
        if(properties.contains(QStringLiteral("onclick")))
            return QStringLiteral("Extensions using event pages or Service Workers cannot pass an onclick parameter to chrome.contextMenus.create. Instead, use the chrome.contextMenus.onClicked event.");
        if(properties.contains(QStringLiteral("type"))){
            const QString type = properties.value(QStringLiteral("type")).toString();
            if(!TypeNames().contains(type)) return QStringLiteral("Invalid value for argument 'type'.");
            item->type = type;
        }
        if(properties.contains(QStringLiteral("title"))){
            if(!properties.value(QStringLiteral("title")).isString()) return QStringLiteral("Invalid value for argument 'title'.");
            if(properties.value(QStringLiteral("title")).toString().size() > MENU_TITLE_LIMIT) return QStringLiteral("Invalid value for argument 'title': too long.");
            item->title = properties.value(QStringLiteral("title")).toString();
        }
        if(properties.contains(QStringLiteral("contexts"))){
            QStringList contexts;
            if(!Strings(properties.value(QStringLiteral("contexts")), &contexts) || contexts.isEmpty())
                return QStringLiteral("Invalid value for argument 'contexts'.");
            foreach(const QString &context, contexts)
                if(!ContextNames().contains(context)) return QStringLiteral("Invalid value for argument 'contexts'.");
            item->contexts = contexts;
        }
        if(properties.contains(QStringLiteral("checked"))){
            if(!properties.value(QStringLiteral("checked")).isBool()) return QStringLiteral("Invalid value for argument 'checked'.");
            item->checked = properties.value(QStringLiteral("checked")).toBool();
        }
        if(properties.contains(QStringLiteral("enabled"))){
            if(!properties.value(QStringLiteral("enabled")).isBool()) return QStringLiteral("Invalid value for argument 'enabled'.");
            item->enabled = properties.value(QStringLiteral("enabled")).toBool();
        }
        if(properties.contains(QStringLiteral("visible"))){
            if(!properties.value(QStringLiteral("visible")).isBool()) return QStringLiteral("Invalid value for argument 'visible'.");
            item->visible = properties.value(QStringLiteral("visible")).toBool();
        }
        foreach(const QString &name, QStringList() << QStringLiteral("documentUrlPatterns") << QStringLiteral("targetUrlPatterns")){
            if(!properties.contains(name)) continue;
            QStringList patterns;
            if(!Strings(properties.value(name), &patterns)) return QStringLiteral("Invalid value for argument '%1'.").arg(name);
            if(patterns.size() > MENU_PATTERNS_LIMIT) return QStringLiteral("Invalid value for argument '%1': too many patterns.").arg(name);
            foreach(const QString &pattern, patterns)
                if(pattern.size() > MENU_ID_LIMIT * 4 || !ReadablePattern(pattern)) return QStringLiteral("Invalid value for argument '%1': Invalid URL pattern '%2'.").arg(name, pattern.left(64));
            (name == QStringLiteral("documentUrlPatterns") ? item->documentUrlPatterns : item->targetUrlPatterns) = patterns;
        }
        if(item->type != QStringLiteral("separator") && item->title.isEmpty())
            return QStringLiteral("All menu items except for separators must have a title");
        if(item->type != QStringLiteral("checkbox") && item->type != QStringLiteral("radio")
           && properties.value(QStringLiteral("checked")).toBool())
            return QStringLiteral("Only items with type \"radio\" or \"checkbox\" can be checked");
        Q_UNUSED(creating)
        return QString();
    }
}

QJsonObject Menus::Create(const QJsonObject &properties){
    MenuItem item;
    item.id = MenuIdOf(properties.value(QStringLiteral("id")));
    if(item.id.isEmpty())
        return Fail(QStringLiteral("Extensions using event pages or Service Workers must pass an id parameter to chrome.contextMenus.create"));
    if(item.id.size() > MENU_ID_LIMIT)
        return Fail(QStringLiteral("Invalid value for argument 'id': too long."));
    if(IndexOf(item.id) >= 0)
        return Fail(QStringLiteral("Cannot create item with duplicate id %1").arg(item.id));
    if(m_Items.size() >= MENU_ITEMS_LIMIT)
        return Fail(QStringLiteral("An extension can create no more than %1 menu items").arg(MENU_ITEMS_LIMIT));
    item.contexts = QStringList() << QStringLiteral("page");
    const QString error = ReadFields(properties, &item, true);
    if(!error.isEmpty()) return Fail(error);
    if(properties.contains(QStringLiteral("parentId"))){
        item.parentId = MenuIdOf(properties.value(QStringLiteral("parentId")));
        if(item.parentId.isEmpty() || IndexOf(item.parentId) < 0)
            return Fail(QStringLiteral("Cannot find menu item with id %1").arg(item.parentId.isEmpty() ? QStringLiteral("undefined") : item.parentId));
        if(DepthOf(item.parentId) >= MENU_DEPTH_LIMIT)
            return Fail(QStringLiteral("Menu items cannot be nested more than %1 deep").arg(MENU_DEPTH_LIMIT));
    }
    m_Items.append(item);
    Sanitize(item.parentId);
    return Ok(item.id);
}

void Menus::Select(int at){
    QList<int> siblings;
    for(int i = 0; i < m_Items.size(); i++)
        if(m_Items.at(i).parentId == m_Items.at(at).parentId) siblings.append(i);
    const int mine = siblings.indexOf(at);
    for(int i = mine - 1; i >= 0 && m_Items.at(siblings.at(i)).type == QStringLiteral("radio"); i--)
        m_Items[siblings.at(i)].checked = false;
    for(int i = mine + 1; i < siblings.size() && m_Items.at(siblings.at(i)).type == QStringLiteral("radio"); i++)
        m_Items[siblings.at(i)].checked = false;
}

void Menus::Sanitize(const QString &parentId){
    QList<int> siblings;
    for(int i = 0; i < m_Items.size(); i++)
        if(m_Items.at(i).parentId == parentId) siblings.append(i);
    for(int i = 0; i < siblings.size(); ){
        if(m_Items.at(siblings.at(i)).type != QStringLiteral("radio")){ i++; continue; }
        int last = -1, end = i;
        for(; end < siblings.size() && m_Items.at(siblings.at(end)).type == QStringLiteral("radio"); end++){
            if(m_Items.at(siblings.at(end)).checked) last = end;
            m_Items[siblings.at(end)].checked = false;
        }
        m_Items[siblings.at(last >= 0 ? last : i)].checked = true;
        i = end;
    }
}

int Menus::DepthOf(const QString &id) const {
    int depth = 0;
    for(QString walk = id; !walk.isEmpty() && depth <= MENU_DEPTH_LIMIT; depth++){
        const int at = IndexOf(walk);
        walk = at < 0 ? QString() : m_Items.at(at).parentId;
    }
    return depth;
}

int Menus::HeightOf(const QString &id) const {
    int height = 0;
    foreach(const MenuItem &item, m_Items)
        if(item.parentId == id && !item.id.isEmpty() && item.id != id) height = qMax(height, 1 + HeightOf(item.id));
    return height;
}

QJsonObject Menus::Update(const QString &id, const QJsonObject &properties){
    const int at = IndexOf(id);
    if(at < 0) return Fail(QStringLiteral("Cannot find menu item with id %1").arg(id));
    MenuItem item = m_Items.at(at);
    const QString error = ReadFields(properties, &item, false);
    if(!error.isEmpty()) return Fail(error);
    const bool selecting = item.type == QStringLiteral("radio") && properties.contains(QStringLiteral("checked"))
        && properties.value(QStringLiteral("checked")).toBool();
    if(item.type == QStringLiteral("radio") && !selecting) item.checked = m_Items.at(at).checked;
    if(properties.contains(QStringLiteral("parentId"))){
        const QJsonValue parent = properties.value(QStringLiteral("parentId"));
        QString parentId = parent.isNull() ? QString() : MenuIdOf(parent);
        if(!parent.isNull() && (parentId.isEmpty() || IndexOf(parentId) < 0))
            return Fail(QStringLiteral("Cannot find menu item with id %1").arg(parentId.isEmpty() ? QStringLiteral("undefined") : parentId));
        for(QString walk = parentId; !walk.isEmpty(); ){
            if(walk == id) return Fail(QStringLiteral("Cannot set an item's parent to itself or one of its descendants"));
            const int up = IndexOf(walk);
            walk = up < 0 ? QString() : m_Items.at(up).parentId;
        }
        if(!parentId.isEmpty() && DepthOf(parentId) + 1 + HeightOf(id) > MENU_DEPTH_LIMIT)
            return Fail(QStringLiteral("Menu items cannot be nested more than %1 deep").arg(MENU_DEPTH_LIMIT));
        item.parentId = parentId;
    }
    const QString was = m_Items.at(at).parentId;
    m_Items[at] = item;
    int now = at;
    if(properties.contains(QStringLiteral("parentId"))){
        m_Items.move(at, m_Items.size() - 1);
        now = m_Items.size() - 1;
        Sanitize(was);
        Sanitize(item.parentId);
    }
    if(item.type == QStringLiteral("radio") && m_Items.at(now).checked) Select(now);
    return Ok(QJsonValue());
}

QJsonObject Menus::Remove(const QString &id){
    const int at = IndexOf(id);
    if(at < 0) return Fail(QStringLiteral("Cannot find menu item with id %1").arg(id));
    QSet<QString> gone;
    gone.insert(id);
    bool more = true;
    while(more){
        more = false;
        foreach(const MenuItem &item, m_Items){
            if(gone.contains(item.id) || item.parentId.isEmpty() || !gone.contains(item.parentId)) continue;
            gone.insert(item.id);
            more = true;
        }
    }
    const QString parentId = m_Items.at(at).parentId;
    for(int i = m_Items.size() - 1; i >= 0; i--)
        if(gone.contains(m_Items.at(i).id)) m_Items.removeAt(i);
    Sanitize(parentId);
    return Ok(QJsonValue());
}

QJsonObject Menus::RemoveAll(){
    m_Items.clear();
    return Ok(QJsonValue());
}

namespace {
    QList<QUrl> Targets(const MenuItem &item, const MenuContext &context){
        QList<QUrl> targets;
        const bool all = item.contexts.contains(QStringLiteral("all"));
        if((all || item.contexts.contains(QStringLiteral("link"))) && !context.linkUrl.isEmpty()) targets << context.linkUrl;
        if((all || item.contexts.contains(QStringLiteral("image")) || item.contexts.contains(QStringLiteral("video"))
            || item.contexts.contains(QStringLiteral("audio"))) && !context.srcUrl.isEmpty()) targets << context.srcUrl;
        return targets;
    }

    bool InContext(const MenuItem &item, const MenuContext &context){
        if(!item.visible) return false;
        bool named = false;
        foreach(const QString &one, item.contexts){
            if(one == QStringLiteral("all") || context.contexts.contains(one)){ named = true; break; }
        }
        if(!named) return false;
        if(context.contexts.contains(QStringLiteral("action"))) return true;
        if(!item.documentUrlPatterns.isEmpty() && !AnyMatches(item.documentUrlPatterns, context.pageUrl)) return false;
        if(!item.targetUrlPatterns.isEmpty()){
            bool any = false;
            foreach(const QUrl &target, Targets(item, context))
                if(AnyMatches(item.targetUrlPatterns, target)) any = true;
            if(!any) return false;
        }
        return true;
    }

    QList<MenuShown> Under(const QList<MenuItem> &items, const QString &parentId, const MenuContext &context, int depth){
        QList<MenuShown> shown;
        if(depth > 8) return shown;
        foreach(const MenuItem &item, items){
            if(item.parentId != parentId || !InContext(item, context)) continue;
            MenuShown one;
            one.item = item;
            one.title = QString(item.title).replace(QStringLiteral("%s"), context.selectionText);
            one.children = Under(items, item.id, context, depth + 1);
            shown.append(one);
        }
        return shown;
    }
}

QList<MenuShown> Menus::Shown(const MenuContext &context) const {
    return Under(m_Items, QString(), context, 0);
}

MenuContext Menus::ActionContext(const QUrl &pageUrl){
    MenuContext context;
    context.contexts.insert(QStringLiteral("action"));
    context.pageUrl = pageUrl;
    return context;
}

QList<MenuShown> Menus::ActionShown(const QUrl &pageUrl) const {
    QList<MenuShown> shown = Shown(ActionContext(pageUrl));
    int items = 0;
    for(int i = 0; i < shown.size(); i++){
        if(shown.at(i).item.type == QStringLiteral("separator")) continue;
        if(++items > 6){ shown = shown.mid(0, i); break; }
    }
    return shown;
}

Menus::Clicked Menus::Click(const QString &id){
    Clicked clicked;
    const int at = IndexOf(id);
    if(at < 0) return clicked;
    MenuItem &item = m_Items[at];
    clicked.found = true;
    clicked.parentId = item.parentId;
    if(item.type == QStringLiteral("checkbox")){
        clicked.checkable = true;
        clicked.wasChecked = item.checked;
        item.checked = !item.checked;
        clicked.checked = item.checked;
        clicked.changed = true;
    } else if(item.type == QStringLiteral("radio")){
        clicked.checkable = true;
        clicked.wasChecked = item.checked;
        clicked.checked = true;
        QList<int> siblings;
        for(int i = 0; i < m_Items.size(); i++)
            if(m_Items.at(i).parentId == item.parentId) siblings.append(i);
        const int mine = siblings.indexOf(at);
        int from = mine, to = mine;
        while(from > 0 && m_Items.at(siblings.at(from - 1)).type == QStringLiteral("radio")) from--;
        while(to + 1 < siblings.size() && m_Items.at(siblings.at(to + 1)).type == QStringLiteral("radio")) to++;
        for(int i = from; i <= to; i++){
            MenuItem &other = m_Items[siblings.at(i)];
            const bool want = other.id == id;
            if(other.checked != want){ other.checked = want; clicked.changed = true; }
        }
    }
    return clicked;
}

QJsonArray Menus::ToJson() const {
    QJsonArray out;
    foreach(const MenuItem &item, m_Items){
        QJsonObject one;
        one[QStringLiteral("id")] = item.id;
        if(!item.parentId.isEmpty()) one[QStringLiteral("parentId")] = item.parentId;
        one[QStringLiteral("type")] = item.type;
        one[QStringLiteral("title")] = item.title;
        one[QStringLiteral("contexts")] = QJsonArray::fromStringList(item.contexts);
        one[QStringLiteral("checked")] = item.checked;
        one[QStringLiteral("enabled")] = item.enabled;
        one[QStringLiteral("visible")] = item.visible;
        if(!item.documentUrlPatterns.isEmpty()) one[QStringLiteral("documentUrlPatterns")] = QJsonArray::fromStringList(item.documentUrlPatterns);
        if(!item.targetUrlPatterns.isEmpty()) one[QStringLiteral("targetUrlPatterns")] = QJsonArray::fromStringList(item.targetUrlPatterns);
        out.append(one);
    }
    return out;
}

Menus Menus::FromJson(const QJsonArray &json){
    Menus menus;
    QList<QPair<QJsonObject, bool>> waiting;
    foreach(const QJsonValue &value, json){
        if(!value.isObject()) continue;
        QJsonObject properties = value.toObject();
        const QString type = properties.value(QStringLiteral("type")).toString();
        bool hidden = false;
        if(type != QStringLiteral("checkbox") && type != QStringLiteral("radio")){
            hidden = properties.value(QStringLiteral("checked")).toBool();
            properties.remove(QStringLiteral("checked"));
        }
        waiting.append(qMakePair(properties, hidden));
    }
    bool made = true;
    while(made && !waiting.isEmpty()){
        made = false;
        for(int i = 0; i < waiting.size(); ){
            if(menus.Create(waiting.at(i).first).value(QStringLiteral("ok")).toBool()){
                if(waiting.at(i).second) menus.m_Items.last().checked = true;
                waiting.removeAt(i);
                made = true;
            }
            else i++;
        }
    }
    return menus;
}

qint64 TabIdOf(const QJsonValue &value, bool *ok){
    *ok = false;
    if(!value.isDouble()) return 0;
    const double d = value.toDouble();
    if(d < 1 || d > 9007199254740991.0 || d != static_cast<double>(static_cast<qint64>(d))) return 0;
    *ok = true;
    return static_cast<qint64>(d);
}

bool AcceptsDownloadUrl(const QString &text){
    if(text.isEmpty()) return false;
    const int colon = text.indexOf(QLatin1Char(':'));
    if(colon <= 0) return false;
    const QString scheme = text.left(colon).toLower();
    if(scheme == QStringLiteral("blob")){
        const QUrl inside(text.mid(colon + 1));
        static const QStringList origins = QStringList() << QStringLiteral("http") << QStringLiteral("https") << QStringLiteral("chrome-extension");
        return inside.isValid() && origins.contains(inside.scheme()) && !inside.host().isEmpty() && inside.path().size() > 1;
    }
    return scheme == QStringLiteral("data") && text.size() > colon + 1;
}

bool AcceptsDownloadFilename(const QString &name){
    if(name.isEmpty()) return true;
    static const QString forbidden = QStringLiteral(":*?\"<>|");
    static const QRegularExpression device(QStringLiteral("\\A(?:CON|PRN|AUX|NUL|CLOCK\\$|COM[1-9]|LPT[1-9])(?:\\..*)?\\z"),
                                           QRegularExpression::CaseInsensitiveOption);
    static const QStringList explorers = QStringList() << QStringLiteral("desktop.ini") << QStringLiteral("thumbs.db");
    static const QRegularExpression shell(QStringLiteral("\\.(?:lnk|local|\\{[0-9A-Fa-f-]+\\})\\z"), QRegularExpression::CaseInsensitiveOption);
    const QStringList parts = name.split(QRegularExpression(QStringLiteral("[/\\\\]")));
    foreach(const QString &part, parts){
        if(part.isEmpty() || part == QStringLiteral(".") || part == QStringLiteral("..")) return false;
        if(part.startsWith(QLatin1Char('.')) || part.endsWith(QLatin1Char(' ')) || part.endsWith(QLatin1Char('.'))) return false;
        if(device.match(part).hasMatch() || explorers.contains(part, Qt::CaseInsensitive) || shell.match(part).hasMatch()) return false;
        foreach(const QChar &c, part){
            const QChar::Category kind = c.category();
            if(kind == QChar::Other_Control || kind == QChar::Other_Format || forbidden.contains(c)) return false;
        }
    }
    return true;
}

QString PathOfEndedDownload(bool completed, const QString &path){
    return completed ? path : QString();
}

namespace {
    QString Stamp(){ return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }
    QJsonObject InterruptedDelta(qint64 id, const QString &previous, const QString &error){
        QJsonObject delta;
        delta[QStringLiteral("id")] = id;
        QJsonObject state;
        state[QStringLiteral("previous")] = previous;
        state[QStringLiteral("current")] = QStringLiteral("interrupted");
        delta[QStringLiteral("state")] = state;
        QJsonObject why;
        why[QStringLiteral("current")] = error;
        delta[QStringLiteral("error")] = why;
        return delta;
    }
}

void Downloads::Forget(qint64 now){
    Expired(now);
}

QList<Downloads::Gone> Downloads::Expired(qint64 now){
    QList<Gone> gone;
    for(int i = 0; i < m_Expected.size(); ){
        if(now - m_Expected.at(i).since <= DOWNLOADS_EXPECTED_MS){ i++; continue; }
        const Expectation one = m_Expected.takeAt(i);
        Gone g;
        g.extension = one.extension;
        g.delta = InterruptedDelta(one.id, QStringLiteral("in_progress"), QStringLiteral("FILE_FAILED"));
        gone.append(g);
    }
    return gone;
}

qint64 Downloads::Expect(const QString &extension, const QUrl &url, const QString &filename, qint64 now){
    Forget(now);
    int mine = 0;
    foreach(const Expectation &one, m_Expected) if(one.extension == extension) mine++;
    for(int i = 0; mine >= DOWNLOADS_EXPECTED_KEPT && i < m_Expected.size(); ){
        if(m_Expected.at(i).extension != extension){ i++; continue; }
        m_Expected.removeAt(i);
        mine--;
    }
    Expectation one;
    one.id = m_Next++;
    one.extension = extension;
    one.url = url;
    one.filename = filename;
    one.since = now;
    m_Expected.append(one);
    return one.id;
}

qint64 Downloads::Arrived(const QUrl &url, qint64 now, QString *extension){
    Forget(now);
    for(int i = 0; i < m_Expected.size(); i++){
        if(m_Expected.at(i).url != url) continue;
        const Expectation one = m_Expected.takeAt(i);
        int mine = 0;
        foreach(const DownloadRecord &record, m_Records) if(record.extension == one.extension) mine++;
        for(int j = 0; mine >= DOWNLOADS_KEPT && j < m_Records.size(); ){
            if(m_Records.at(j).extension != one.extension){ j++; continue; }
            m_Records.removeAt(j);
            mine--;
        }
        DownloadRecord record;
        record.id = one.id;
        record.extension = one.extension;
        record.url = one.url;
        record.startTime = Stamp();
        m_Records.append(record);
        if(extension) *extension = one.extension;
        return one.id;
    }
    return 0;
}

namespace {
    DownloadRecord *Find(QList<DownloadRecord> &records, qint64 id){
        for(int i = 0; i < records.size(); i++)
            if(records[i].id == id) return &records[i];
        return nullptr;
    }
}

const DownloadRecord *Downloads::RecordOf(qint64 id) const {
    foreach(const DownloadRecord &record, m_Records)
        if(record.id == id) return &record;
    return nullptr;
}

void Downloads::Describe(qint64 id, const QString &filename, const QString &mime, qint64 totalBytes, qint64 receivedBytes){
    DownloadRecord *record = Find(m_Records, id);
    if(!record) return;
    if(!filename.isEmpty()) record->filename = filename;
    if(!mime.isEmpty()) record->mime = mime;
    record->totalBytes = totalBytes;
    record->receivedBytes = receivedBytes;
}

QJsonObject Downloads::Ended(qint64 id, bool completed, const QString &error){
    DownloadRecord *record = Find(m_Records, id);
    if(!record || record->state != QStringLiteral("in_progress")) return QJsonObject();
    const QString previous = record->state;
    record->state = completed ? QStringLiteral("complete") : QStringLiteral("interrupted");
    record->error = completed ? QString() : error;
    record->endTime = Stamp();
    QJsonObject delta;
    delta[QStringLiteral("id")] = id;
    QJsonObject state;
    state[QStringLiteral("previous")] = previous;
    state[QStringLiteral("current")] = record->state;
    delta[QStringLiteral("state")] = state;
    if(record->paused){
        record->paused = false;
        QJsonObject unpaused;
        unpaused[QStringLiteral("previous")] = true;
        unpaused[QStringLiteral("current")] = false;
        delta[QStringLiteral("paused")] = unpaused;
        delta[QStringLiteral("canResume")] = unpaused;
    }
    if(!completed && !error.isEmpty()){
        QJsonObject why;
        why[QStringLiteral("current")] = error;
        delta[QStringLiteral("error")] = why;
    }
    return delta;
}

namespace {
    QJsonObject Item(const DownloadRecord &record){
        QJsonObject item;
        item[QStringLiteral("id")] = record.id;
        item[QStringLiteral("url")] = record.url.toString(QUrl::FullyEncoded);
        item[QStringLiteral("finalUrl")] = record.url.toString(QUrl::FullyEncoded);
        item[QStringLiteral("filename")] = record.filename;
        item[QStringLiteral("mime")] = record.mime;
        item[QStringLiteral("state")] = record.state;
        if(!record.error.isEmpty()) item[QStringLiteral("error")] = record.error;
        item[QStringLiteral("paused")] = record.paused;
        item[QStringLiteral("canResume")] = record.paused;
        item[QStringLiteral("exists")] = record.state == QStringLiteral("complete");
        item[QStringLiteral("incognito")] = false;
        item[QStringLiteral("danger")] = QStringLiteral("safe");
        item[QStringLiteral("totalBytes")] = record.totalBytes < 0 ? 0 : record.totalBytes;
        item[QStringLiteral("bytesReceived")] = record.receivedBytes < 0 ? 0 : record.receivedBytes;
        item[QStringLiteral("fileSize")] = record.totalBytes < 0 ? 0 : record.totalBytes;
        item[QStringLiteral("startTime")] = record.startTime;
        if(!record.endTime.isEmpty()) item[QStringLiteral("endTime")] = record.endTime;
        return item;
    }
}

QJsonObject Downloads::Created(qint64 id) const {
    const DownloadRecord *record = RecordOf(id);
    return record ? Item(*record) : QJsonObject();
}

QJsonArray Downloads::Search(const QString &extension, const QJsonObject &query) const {
    QJsonArray out;
    bool whole = false;
    const qint64 id = TabIdOf(query.value(QStringLiteral("id")), &whole);
    if(!whole) return out;
    const DownloadRecord *record = RecordOf(extension, id);
    if(record) out.append(Item(*record));
    return out;
}

const DownloadRecord *Downloads::RecordOf(const QString &extension, qint64 id) const {
    const DownloadRecord *record = RecordOf(id);
    return record && record->extension == extension ? record : nullptr;
}

namespace {
    template <class T>
    QJsonObject Change(const T &previous, const T &current){
        QJsonObject change;
        change[QStringLiteral("previous")] = previous;
        change[QStringLiteral("current")] = current;
        return change;
    }
}

QJsonObject Downloads::Named(qint64 id, const QString &filename){
    DownloadRecord *record = Find(m_Records, id);
    if(!record || filename.isEmpty() || !record->filename.isEmpty()) return QJsonObject();
    const QString previous = record->filename;
    record->filename = filename;
    QJsonObject delta;
    delta[QStringLiteral("id")] = id;
    delta[QStringLiteral("filename")] = Change(previous, filename);
    return delta;
}

QJsonObject Downloads::Paused(qint64 id, bool paused){
    DownloadRecord *record = Find(m_Records, id);
    if(!record || record->state != QStringLiteral("in_progress") || record->paused == paused) return QJsonObject();
    record->paused = paused;
    QJsonObject delta;
    delta[QStringLiteral("id")] = id;
    delta[QStringLiteral("paused")] = Change(!paused, paused);
    delta[QStringLiteral("canResume")] = Change(!paused, paused);
    return delta;
}

QList<qint64> Downloads::Erase(const QString &extension, const QJsonObject &query){
    QList<qint64> erased;
    bool whole = false;
    const qint64 id = TabIdOf(query.value(QStringLiteral("id")), &whole);
    if(!whole) return erased;
    for(int i = 0; i < m_Records.size(); i++){
        if(m_Records.at(i).id != id || m_Records.at(i).extension != extension) continue;
        m_Records.removeAt(i);
        erased << id;
        break;
    }
    return erased;
}

QString InterruptReasonOf(int reason){
    switch(reason){
    case 1: return QStringLiteral("FILE_FAILED");
    case 2: return QStringLiteral("FILE_ACCESS_DENIED");
    case 3: return QStringLiteral("FILE_NO_SPACE");
    case 5: return QStringLiteral("FILE_NAME_TOO_LONG");
    case 6: return QStringLiteral("FILE_TOO_LARGE");
    case 7: return QStringLiteral("FILE_VIRUS_INFECTED");
    case 10: return QStringLiteral("FILE_TRANSIENT_ERROR");
    case 11: return QStringLiteral("FILE_BLOCKED");
    case 12: return QStringLiteral("FILE_SECURITY_CHECK_FAILED");
    case 13: return QStringLiteral("FILE_TOO_SHORT");
    case 14: return QStringLiteral("FILE_HASH_MISMATCH");
    case 20: return QStringLiteral("NETWORK_FAILED");
    case 21: return QStringLiteral("NETWORK_TIMEOUT");
    case 22: return QStringLiteral("NETWORK_DISCONNECTED");
    case 23: return QStringLiteral("NETWORK_SERVER_DOWN");
    case 24: return QStringLiteral("NETWORK_INVALID_REQUEST");
    case 30: return QStringLiteral("SERVER_FAILED");
    case 33: return QStringLiteral("SERVER_BAD_CONTENT");
    case 34: return QStringLiteral("SERVER_UNAUTHORIZED");
    case 35: return QStringLiteral("SERVER_CERT_PROBLEM");
    case 36: return QStringLiteral("SERVER_FORBIDDEN");
    case 37: return QStringLiteral("SERVER_UNREACHABLE");
    case 15: return QStringLiteral("FILE_SAME_AS_SOURCE");
    case 31: return QStringLiteral("SERVER_NO_RANGE");
    case 38: return QStringLiteral("SERVER_CONTENT_LENGTH_MISMATCH");
    case 39: return QStringLiteral("SERVER_CROSS_ORIGIN_REDIRECT");
    case 40: return QStringLiteral("USER_CANCELED");
    case 41: return QStringLiteral("USER_SHUTDOWN");
    case 50: return QStringLiteral("CRASH");
    default: return QStringLiteral("FILE_FAILED");
    }
}

QString WhyNotDownloadAct(const QString &api, const DownloadRecord *record){
    if(api == QStringLiteral("downloads.cancel")) return QString();
    if(!record) return QStringLiteral("Invalid download id");
    const bool going = record->state == QStringLiteral("in_progress");
    if(api == QStringLiteral("downloads.pause")) return going ? QString() : QStringLiteral("Download must be in progress");
    if(api == QStringLiteral("downloads.resume")) return going && record->paused ? QString() : QStringLiteral("DownloadItem.canResume == false");
    if(api == QStringLiteral("downloads.show")) return record->filename.isEmpty() ? QStringLiteral("Invalid download id") : QString();
    return QStringLiteral("chrome.%1 is not available in this browser").arg(api);
}

QString WhyNotOpenPopup(bool hasPopup, Action *action, const TabNow *tab){
    const QString none = QStringLiteral("Extension does not have a popup on the active tab.");
    if(!hasPopup) return none;
    if(!tab) return QStringLiteral("Could not find an active browser window.");
    if(action && !action->Shown(*tab).enabled) return none;
    return QString();
}

QString MenusFileName(const QString &profileKey){
    return ExtensionMainScripts::ProfileFileName(QStringLiteral("extension-menus-"), profileKey);
}

QString RulesFileName(const QString &profileKey){
    return ExtensionMainScripts::ProfileFileName(QStringLiteral("extension-rules-"), profileKey);
}

QJsonObject ClickInfo(const MenuItem &item, const Menus::Clicked &clicked, const MenuContext &context){
    QJsonObject info;
    info[QStringLiteral("menuItemId")] = item.id;
    if(!clicked.parentId.isEmpty()) info[QStringLiteral("parentMenuItemId")] = clicked.parentId;
    foreach(const QString &media, QStringList() << QStringLiteral("image") << QStringLiteral("video") << QStringLiteral("audio")){
        if(context.contexts.contains(media)){
            info[QStringLiteral("mediaType")] = media;
            if(!context.srcUrl.isEmpty()) info[QStringLiteral("srcUrl")] = context.srcUrl.toString(QUrl::FullyEncoded);
            break;
        }
    }
    if(context.contexts.contains(QStringLiteral("link")) && !context.linkUrl.isEmpty())
        info[QStringLiteral("linkUrl")] = context.linkUrl.toString(QUrl::FullyEncoded);
    info[QStringLiteral("pageUrl")] = context.pageUrl.toString(QUrl::FullyEncoded);
    info[QStringLiteral("frameId")] = 0;
    info[QStringLiteral("editable")] = context.contexts.contains(QStringLiteral("editable"));
    if(!context.selectionText.isEmpty()) info[QStringLiteral("selectionText")] = context.selectionText;
    if(clicked.checkable){
        info[QStringLiteral("wasChecked")] = clicked.wasChecked;
        info[QStringLiteral("checked")] = clicked.checked;
    }
    return info;
}

QString InstalledFileName(const QString &profileKey){
    return ExtensionMainScripts::ProfileFileName(QStringLiteral("extension-installed-"), profileKey);
}

namespace {
    const QStringList &KeyNames(){
        static const QStringList names = QStringList()
            << QStringLiteral("Comma") << QStringLiteral("Period") << QStringLiteral("Home") << QStringLiteral("End")
            << QStringLiteral("PageUp") << QStringLiteral("PageDown") << QStringLiteral("Space") << QStringLiteral("Tab")
            << QStringLiteral("Insert") << QStringLiteral("Delete")
            << QStringLiteral("Up") << QStringLiteral("Down") << QStringLiteral("Left") << QStringLiteral("Right");
        return names;
    }
    bool IsMediaKey(const QString &token){
        return token == QStringLiteral("MediaNextTrack") || token == QStringLiteral("MediaPlayPause")
            || token == QStringLiteral("MediaPrevTrack") || token == QStringLiteral("MediaStop");
    }
    QString ParsedKey(const QString &text, const QString &platform){
        const QStringList tokens = text.split(QLatin1Char('+'));
        if(tokens.isEmpty() || tokens.size() > 3) return QString();
        bool ctrl = false, alt = false, shift = false;
        QString key;
        foreach(QString token, tokens){
            token = token.trimmed();
            if(token == QStringLiteral("Ctrl")) ctrl = true;
            else if(token == QStringLiteral("Command")){ if(platform != QStringLiteral("mac")) return QString(); ctrl = true; }
            else if(token == QStringLiteral("Alt")) alt = true;
            else if(token == QStringLiteral("Shift")) shift = true;
            else if(IsMediaKey(token)) return QString();
            else if((token.size() == 1 && ((token.at(0) >= QLatin1Char('A') && token.at(0) <= QLatin1Char('Z'))
                                           || (token.at(0) >= QLatin1Char('0') && token.at(0) <= QLatin1Char('9'))))
                    || KeyNames().contains(token)){
                if(!key.isEmpty()) return QString();
                key = token;
            } else return QString();
        }
        if(key.isEmpty() || (!ctrl && !alt) || (ctrl && alt)) return QString();
        QString out;
        if(ctrl) out += QStringLiteral("Ctrl+");
        if(alt) out += QStringLiteral("Alt+");
        if(shift) out += QStringLiteral("Shift+");
        return out + key;
    }
}

QList<Command> CommandsOf(const QJsonObject &commands, const QString &platform, bool hasAction){
    QList<Command> out;
    int keyed = 0;
    for(QJsonObject::const_iterator it = commands.constBegin(); it != commands.constEnd(); ++it){
        const QString name = it.key();
        if(name == QStringLiteral("_execute_browser_action") || name == QStringLiteral("_execute_page_action")) continue;
        if(name == QStringLiteral("_execute_action") && !hasAction) continue;
        const QJsonObject entry = it.value().toObject();
        Command command;
        command.name = name;
        command.description = entry.value(QStringLiteral("description")).toString();
        const QJsonValue suggested = entry.value(QStringLiteral("suggested_key"));
        QString text;
        if(suggested.isString()) text = suggested.toString();
        else if(suggested.isObject()){
            const QJsonObject keys = suggested.toObject();
            text = keys.contains(platform) ? keys.value(platform).toString() : keys.value(QStringLiteral("default")).toString();
        }
        command.key = text.isEmpty() ? QString() : ParsedKey(text, platform);
        if(!command.key.isEmpty() && ++keyed > 4) command.key.clear();
        out.append(command);
    }
    return out;
}

QString ChromeKeyOf(bool ctrl, bool alt, bool shift, int virtualKey, int qtKey){
    if(!ctrl && !alt) return QString();
    QString key;
    if(virtualKey){
        if(virtualKey >= 0x41 && virtualKey <= 0x5A) key = QChar(QLatin1Char('A' + (virtualKey - 0x41)));
        else if(virtualKey >= 0x30 && virtualKey <= 0x39) key = QChar(QLatin1Char('0' + (virtualKey - 0x30)));
        else switch(virtualKey){
            case 0xBC: key = QStringLiteral("Comma"); break;
            case 0xBE: key = QStringLiteral("Period"); break;
            case 0x09: key = QStringLiteral("Tab"); break;
            case 0x20: key = QStringLiteral("Space"); break;
            case 0x21: key = QStringLiteral("PageUp"); break;
            case 0x22: key = QStringLiteral("PageDown"); break;
            case 0x23: key = QStringLiteral("End"); break;
            case 0x24: key = QStringLiteral("Home"); break;
            case 0x25: key = QStringLiteral("Left"); break;
            case 0x26: key = QStringLiteral("Up"); break;
            case 0x27: key = QStringLiteral("Right"); break;
            case 0x28: key = QStringLiteral("Down"); break;
            case 0x2D: key = QStringLiteral("Insert"); break;
            case 0x2E: key = QStringLiteral("Delete"); break;
            default: break;
        }
    } else {
        if(qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) key = QChar(QLatin1Char('A' + (qtKey - Qt::Key_A)));
        else if(qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) key = QChar(QLatin1Char('0' + (qtKey - Qt::Key_0)));
        else switch(qtKey){
            case Qt::Key_Comma: key = QStringLiteral("Comma"); break;
            case Qt::Key_Period: key = QStringLiteral("Period"); break;
            case Qt::Key_Tab: case Qt::Key_Backtab: key = QStringLiteral("Tab"); break;
            case Qt::Key_Space: key = QStringLiteral("Space"); break;
            case Qt::Key_PageUp: key = QStringLiteral("PageUp"); break;
            case Qt::Key_PageDown: key = QStringLiteral("PageDown"); break;
            case Qt::Key_End: key = QStringLiteral("End"); break;
            case Qt::Key_Home: key = QStringLiteral("Home"); break;
            case Qt::Key_Left: key = QStringLiteral("Left"); break;
            case Qt::Key_Up: key = QStringLiteral("Up"); break;
            case Qt::Key_Right: key = QStringLiteral("Right"); break;
            case Qt::Key_Down: key = QStringLiteral("Down"); break;
            case Qt::Key_Insert: key = QStringLiteral("Insert"); break;
            case Qt::Key_Delete: key = QStringLiteral("Delete"); break;
            default: break;
        }
    }
    if(key.isEmpty()) return QString();
    QString out;
    if(ctrl) out += QStringLiteral("Ctrl+");
    if(alt) out += QStringLiteral("Alt+");
    if(shift) out += QStringLiteral("Shift+");
    return out + key;
}

QString QtKeyTextOf(const QString &chromeKey){
    QStringList parts = chromeKey.split(QLatin1Char('+'));
    if(parts.isEmpty()) return QString();
    QString &key = parts.last();
    if(key == QStringLiteral("Comma")) key = QStringLiteral(",");
    else if(key == QStringLiteral("Period")) key = QStringLiteral(".");
    else if(key == QStringLiteral("PageUp")) key = QStringLiteral("PgUp");
    else if(key == QStringLiteral("PageDown")) key = QStringLiteral("PgDown");
    else if(key == QStringLiteral("Insert")) key = QStringLiteral("Ins");
    else if(key == QStringLiteral("Delete")) key = QStringLiteral("Del");
    return parts.join(QLatin1Char('+'));
}

KeyRoute RouteKey(bool viewMapHit, bool appMapHit, bool commandHit){
    if(viewMapHit || appMapHit) return KeyRoute::Vanilla;
    return commandHit ? KeyRoute::Extension : KeyRoute::Pass;
}

QStringList Installed::Decide(const QList<QPair<QString, QString>> &registered, bool migrating){
    QStringList owed;
    for(const QPair<QString, QString> &one : registered){
        const QString &id = one.first, &version = one.second;
        if(id.isEmpty()) continue;
        auto found = m_Entries.find(id);
        if(found == m_Entries.end()){
            Entry entry;
            entry.version = version;
            if(migrating){
                entry.owed.reason = QStringLiteral("update");
                entry.owed.previousVersion = version;
            } else {
                entry.owed.reason = QStringLiteral("install");
            }
            m_Entries.insert(id, entry);
            owed.append(id);
            continue;
        }
        Entry &entry = found.value();
        if(entry.version == version) continue;
        if(entry.owed.reason.isEmpty()){
            entry.owed.reason = QStringLiteral("update");
            entry.owed.previousVersion = entry.version;
        }
        entry.version = version;
        owed.append(id);
    }
    return owed;
}

Installed::Owed Installed::Take(const QString &id){
    auto found = m_Entries.find(id);
    if(found == m_Entries.end()) return Owed();
    const Owed owed = found.value().owed;
    found.value().owed = Owed();
    return owed;
}

Installed::Owed Installed::OwedTo(const QString &id) const {
    return m_Entries.value(id).owed;
}

void Installed::Restore(const QString &id, const Owed &owed){
    auto found = m_Entries.find(id);
    if(found != m_Entries.end() && found.value().owed.reason.isEmpty()) found.value().owed = owed;
}

bool Installed::Forget(const QSet<QString> &registered){
    bool changed = false;
    for(auto i = m_Entries.begin(); i != m_Entries.end();){
        if(registered.contains(i.key())){ ++i; continue; }
        i = m_Entries.erase(i);
        changed = true;
    }
    return changed;
}

QJsonObject Installed::ToJson() const {
    QJsonObject all;
    for(auto i = m_Entries.constBegin(); i != m_Entries.constEnd(); ++i){
        QJsonObject one;
        one[QStringLiteral("version")] = i.value().version;
        if(!i.value().owed.reason.isEmpty()){
            one[QStringLiteral("owed")] = i.value().owed.reason;
            if(!i.value().owed.previousVersion.isEmpty())
                one[QStringLiteral("previousVersion")] = i.value().owed.previousVersion;
        }
        all[i.key()] = one;
    }
    return all;
}

Installed Installed::FromJson(const QJsonObject &json, bool *damaged){
    Installed installed;
    bool bad = false;
    for(auto i = json.constBegin(); i != json.constEnd(); ++i){
        const QJsonObject one = i.value().toObject();
        const QJsonValue version = one.value(QStringLiteral("version"));
        const QString owed = one.value(QStringLiteral("owed")).toString();
        if(i.key().isEmpty() || !i.value().isObject() || !version.isString() ||
           !(owed.isEmpty() || owed == QStringLiteral("install") || owed == QStringLiteral("update"))){
            bad = true;
            continue;
        }
        Entry entry;
        entry.version = version.toString();
        entry.owed.reason = owed;
        if(!owed.isEmpty()) entry.owed.previousVersion = one.value(QStringLiteral("previousVersion")).toString();
        installed.m_Entries.insert(i.key(), entry);
    }
    if(damaged) *damaged = bad;
    return installed;
}

namespace {
    QString SidePanelShape(const char *call, const char *arguments){
        return QStringLiteral("Error in invocation of sidePanel.%1(%2): No matching signature.")
            .arg(QLatin1String(call), QLatin1String(arguments));
    }
}

QJsonObject SidePanel::SetOptions(const QJsonArray &args, const Resolve &resolve){
    const QString shape = SidePanelShape("setOptions", "sidePanel.PanelOptions options, optional function callback");
    if(args.size() != 1 || !args.at(0).isObject()) return Fail(shape);
    const QJsonObject given = args.at(0).toObject();
    const QJsonValue tab = given.value(QStringLiteral("tabId")), path = given.value(QStringLiteral("path")),
                     enabled = given.value(QStringLiteral("enabled"));
    if((!tab.isUndefined() && !(tab.isDouble() && tab.toDouble() == qint64(tab.toDouble()) && tab.toDouble() >= 0))
       || (!path.isUndefined() && !path.isString()) || (!enabled.isUndefined() && !enabled.isBool()))
        return Fail(shape);
    if(path.isString() && (path.toString().isEmpty() || resolve(path.toString()).isEmpty()))
        return Fail(QStringLiteral("Invalid path."));
    if(tab.isUndefined()){
        if(path.isString()){ m_HasPath = true; m_Path = path.toString(); }
        if(enabled.isBool()) m_Enabled = enabled.toBool();
        return Ok();
    }
    const qint64 id = qint64(tab.toDouble());
    if(!m_Tabs.contains(id) && m_Tabs.size() >= TABS_KEPT) m_Tabs.erase(m_Tabs.begin());
    Options &options = m_Tabs[id];
    if(path.isString()){ options.hasPath = true; options.path = path.toString(); }
    if(enabled.isBool()){ options.hasEnabled = true; options.enabled = enabled.toBool(); }
    return Ok();
}

QJsonObject SidePanel::GetOptions(const QJsonArray &args, const QString &fallback) const {
    const QString shape = SidePanelShape("getOptions", "sidePanel.GetPanelOptions options, optional function callback");
    if(args.size() != 1 || !args.at(0).isObject()) return Fail(shape);
    const QJsonValue tab = args.at(0).toObject().value(QStringLiteral("tabId"));
    if(!tab.isUndefined() && !(tab.isDouble() && tab.toDouble() == qint64(tab.toDouble()) && tab.toDouble() >= 0))
        return Fail(shape);
    QJsonObject out;
    QString path = Path(fallback);
    bool enabled = m_Enabled;
    if(!tab.isUndefined()){
        const qint64 id = qint64(tab.toDouble());
        out[QStringLiteral("tabId")] = double(id);
        const Options options = m_Tabs.value(id);
        if(options.hasPath) path = options.path;
        if(options.hasEnabled) enabled = options.enabled;
    }
    out[QStringLiteral("enabled")] = enabled;
    if(!path.isEmpty()) out[QStringLiteral("path")] = path;
    return Ok(out);
}

QJsonObject SidePanel::SetBehavior(const QJsonArray &args){
    const QString shape = SidePanelShape("setPanelBehavior", "sidePanel.PanelBehavior behavior, optional function callback");
    if(args.size() != 1 || !args.at(0).isObject()) return Fail(shape);
    const QJsonValue on = args.at(0).toObject().value(QStringLiteral("openPanelOnActionClick"));
    if(!on.isUndefined() && !on.isBool()) return Fail(shape);
    if(on.isBool()) m_OnAction = on.toBool();
    return Ok();
}

QJsonObject SidePanel::GetBehavior() const {
    QJsonObject out;
    out[QStringLiteral("openPanelOnActionClick")] = m_OnAction;
    return Ok(out);
}

QString SidePanel::TabPath(qint64 tab) const {
    const Options options = m_Tabs.value(tab);
    return options.hasPath ? options.path : QString();
}

bool SidePanel::EnabledFor(qint64 tab) const {
    const Options options = m_Tabs.value(tab);
    return options.hasEnabled ? options.enabled : m_Enabled;
}

QUrl SidePanel::TabUrl(qint64 tab, const Resolve &resolve) const {
    const QString own = TabPath(tab);
    if(own.isEmpty() || !EnabledFor(tab)) return QUrl();
    return resolve(own);
}

QString SidePanel::Path(const QString &fallback) const {
    return m_HasPath ? m_Path : fallback;
}

QUrl SidePanel::Url(const QString &fallback, const Resolve &resolve) const {
    const QString path = Path(fallback);
    if(!m_Enabled || path.isEmpty()) return QUrl();
    return resolve(path);
}

namespace {
    const int NOTICE_TEXT_MOST = 1000;

    QString NoticeCut(const QString &text){
        return text.size() > NOTICE_TEXT_MOST ? text.left(NOTICE_TEXT_MOST) + QStringLiteral("...") : text;
    }
}

QJsonObject NoticeOf(const QJsonObject &given, const QJsonObject &previous, bool creating, QString *error){
    static const char *const STRINGS[] = { "type", "iconUrl", "title", "message", "contextMessage", "imageUrl" };
    for(const char *key : STRINGS){
        const QJsonValue value = given.value(QLatin1String(key));
        if(!value.isUndefined() && !value.isString()){
            *error = QStringLiteral("Invalid value for argument 2. Property '%1': Expected 'string'.").arg(QLatin1String(key));
            return QJsonObject();
        }
    }
    const QJsonValue type = given.value(QStringLiteral("type"));
    static const QStringList TYPES = { QStringLiteral("basic"), QStringLiteral("image"),
                                       QStringLiteral("list"), QStringLiteral("progress") };
    if(type.isString() && !TYPES.contains(type.toString())){
        *error = QStringLiteral("Invalid value for argument 2. Property 'type': Value must be one of basic, image, list, progress.");
        return QJsonObject();
    }
    if(creating){
        for(const char *key : { "type", "iconUrl", "title", "message" }){
            if(!given.value(QLatin1String(key)).isString()){
                *error = QStringLiteral("Some of the required properties are missing: type, iconUrl, title and message.");
                return QJsonObject();
            }
        }
    }
    QJsonObject merged = creating ? QJsonObject() : previous;
    for(QJsonObject::const_iterator i = given.constBegin(); i != given.constEnd(); ++i)
        merged.insert(i.key(), i.value());
    error->clear();
    return merged;
}

QString NoticeTitle(const QString &extensionName, const QJsonObject &options){
    const QString title = options.value(QStringLiteral("title")).toString();
    if(title.isEmpty()) return extensionName;
    return NoticeCut(QStringLiteral("%1: %2").arg(extensionName, title));
}

QString NoticeText(const QJsonObject &options){
    QString text = options.value(QStringLiteral("message")).toString();
    const QString context = options.value(QStringLiteral("contextMessage")).toString();
    if(!context.isEmpty()) text += (text.isEmpty() ? QString() : QStringLiteral("\n")) + context;
    return NoticeCut(text);
}

Notices::Added Notices::Add(const QString &extension, const QString &id, const QJsonObject &options, const QString &version){
    Added added;
    Own &own = m_Own[extension];
    own.version = version;
    for(int i = 0; i < own.entries.size(); i++){
        if(own.entries.at(i).id != id) continue;
        added.replaced = own.entries.at(i).serial;
        own.entries.removeAt(i);
        break;
    }
    while(own.entries.size() >= PER_EXTENSION){
        const Entry oldest = own.entries.takeFirst();
        Gone gone;
        gone.id = oldest.id;
        gone.serial = oldest.serial;
        added.pushedOut << gone;
    }
    Entry entry;
    entry.id = id;
    entry.serial = ++m_Serial;
    entry.options = options;
    own.entries << entry;
    added.serial = entry.serial;
    return added;
}

bool Notices::Take(const QString &extension, const QString &id, quint64 serial){
    QHash<QString, Own>::iterator own = m_Own.find(extension);
    if(own == m_Own.end()) return false;
    for(int i = 0; i < own->entries.size(); i++){
        const Entry &entry = own->entries.at(i);
        if(entry.id != id || entry.serial != serial) continue;
        own->entries.removeAt(i);
        if(own->entries.isEmpty()) m_Own.erase(own);
        return true;
    }
    return false;
}

quint64 Notices::SerialOf(const QString &extension, const QString &id) const {
    foreach(const Entry &entry, m_Own.value(extension).entries)
        if(entry.id == id) return entry.serial;
    return 0;
}

QJsonObject Notices::OptionsOf(const QString &extension, const QString &id) const {
    foreach(const Entry &entry, m_Own.value(extension).entries)
        if(entry.id == id) return entry.options;
    return QJsonObject();
}

QString Notices::VersionOf(const QString &extension) const {
    return m_Own.value(extension).version;
}

QStringList Notices::Ids(const QString &extension) const {
    QStringList ids;
    foreach(const Entry &entry, m_Own.value(extension).entries) ids << entry.id;
    return ids;
}

QStringList Notices::Extensions() const {
    return m_Own.keys();
}

QList<Notices::Gone> Notices::Drop(const QString &extension){
    QList<Gone> gone;
    foreach(const Entry &entry, m_Own.take(extension).entries){
        Gone one;
        one.id = entry.id;
        one.serial = entry.serial;
        gone << one;
    }
    return gone;
}

}
