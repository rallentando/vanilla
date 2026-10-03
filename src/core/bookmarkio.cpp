#include "switch.hpp"
#include "const.hpp"

#include "bookmarkio.hpp"

#include "lightnode.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QDateTime>
#include <QTimeZone>
#include <QUrl>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QDomDocument>
#include <QDomElement>

#include <climits>

namespace BookmarkIO {

namespace {

ViewNode *Append(ViewNode *parent){
    return parent->MakeChild(INT_MAX);
}

QByteArray ReadWholeFile(const QString &path, bool *ok){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)){
        if(ok) *ok = false;
        return QByteArray();
    }
    QByteArray data = file.readAll();
    file.close();
    if(ok) *ok = true;
    return data;
}

bool WriteWholeFile(const QString &path, const QByteArray &data){
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)) return false;
    bool written = file.write(data) == data.length();
    file.close();
    return written;
}

bool ParseXml(const QByteArray &xml, QDomDocument &doc){
    return !!doc.setContent(xml);
}

bool ParseJson(const QByteArray &json, QJsonObject &obj){
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if(error.error != QJsonParseError::NoError || !doc.isObject()) return false;
    obj = doc.object();
    return true;
}

QDateTime DateOrNow(const QString &str){
    return str.isEmpty() ? QDateTime::currentDateTime() : NodeDateTimeFromString(str);
}

QDateTime DateOrNow(const QString &str, Qt::DateFormat format){
    return str.isEmpty() ? QDateTime::currentDateTime() : QDateTime::fromString(str, format);
}

QDateTime WithOffset(QDateTime date, int utcOffset){
    date.setTimeZone(QTimeZone::fromSecondsAheadOfUtc(utcOffset));
    return date;
}

QString HtmlEscape(QString str){
    str.replace(QLatin1Char('&'),  QStringLiteral("&amp;"));
    str.replace(QLatin1Char('<'),  QStringLiteral("&lt;"));
    str.replace(QLatin1Char('>'),  QStringLiteral("&gt;"));
    str.replace(QLatin1Char('"'),  QStringLiteral("&quot;"));
    return str;
}

QString HtmlUnescape(QString str){
    static const QRegularExpression decimal(QStringLiteral("&#([0-9]{1,7});"));
    static const QRegularExpression hex(QStringLiteral("&#[xX]([0-9a-fA-F]{1,6});"));

    for(const QRegularExpression *rx : {&decimal, &hex}){
        const int base = rx == &decimal ? 10 : 16;
        QString out;
        int pos = 0;
        QRegularExpressionMatch m;
        while((m = rx->match(str, pos)).hasMatch()){
            bool ok = false;
            const char32_t code = m.captured(1).toUInt(&ok, base);
            out += QStringView(str).mid(pos, m.capturedStart() - pos);
            if(ok && code && code <= 0x10FFFF) out += QString::fromUcs4(&code, 1);
            else                               out += m.captured(0);
            pos = m.capturedEnd();
        }
        str = out + QStringView(str).mid(pos).toString();
    }

    str.replace(QStringLiteral("&lt;"),   QStringLiteral("<"));
    str.replace(QStringLiteral("&gt;"),   QStringLiteral(">"));
    str.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    str.replace(QStringLiteral("&apos;"), QStringLiteral("'"));
    str.replace(QStringLiteral("&#39;"),  QStringLiteral("'"));
    str.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    str.replace(QStringLiteral("&amp;"),  QStringLiteral("&"));
    return str;
}

bool LooksLikeNetscapeHtml(const QString &html){
    return html.contains(QStringLiteral("<DL"), Qt::CaseInsensitive)
        || html.contains(QStringLiteral("NETSCAPE-Bookmark"), Qt::CaseInsensitive);
}

}

QString IeFavoritesDirectory(){
#if defined(Q_OS_WIN)
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
        QStringLiteral("/Favorites");
#else
    return QString();
#endif
}

QString FirefoxProfileDirectory(){
#if defined(Q_OS_WIN)
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
        QStringLiteral("/AppData/Roaming/Mozilla/Firefox/Profiles");
#elif defined(Q_OS_MAC)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QStringLiteral("/Firefox/Profiles");
#else
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
        QStringLiteral("/.mozilla/Firefox/Profiles");
#endif
}

QString FirefoxBookmarkBackup(){
    const QString profile = FirefoxProfileDirectory();
    QDir profiledir = profile;
    QStringList list =
        profiledir.entryList(QStringList() << QStringLiteral("*.default"),
                             QDir::NoFilter, QDir::Name);
    if(list.isEmpty()) return QString();

    foreach(QString defaultdir, list){
        QString bookmarklist =
            profile + QStringLiteral("/") + defaultdir + QStringLiteral("/bookmarkbackups");
        QDir bookmarksdir = bookmarklist;
        QStringList jsonlist =
            bookmarksdir.entryList(QStringList() <<
                                   QStringLiteral("bookmarks-[0-9][0-9][0-9][0-9]-"
                                                  "[0-9][0-9]-[0-9][0-9]_[0-9]*.json"),
                                   QDir::NoFilter, QDir::Name);
        if(!jsonlist.isEmpty())
            return bookmarklist + QStringLiteral("/") + jsonlist.takeLast();
    }
    return QString();
}

QString ChromeBookmarkFile(){
#if defined(Q_OS_WIN)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QStringLiteral("/Google/Chrome/User Data/Default/Bookmarks");
#elif defined(Q_OS_MAC)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QStringLiteral("/Google/Chrome/Default/Bookmarks");
#else
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
        QStringLiteral("/google-chrome/Default/Bookmarks");
#endif
}

QString OperaBookmarkFile(){
#if defined(Q_OS_WIN)
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
        QStringLiteral("/AppData/Roaming/Opera Software/Opera Stable/Bookmarks");
#elif defined(Q_OS_MAC)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QStringLiteral("/com.operasoftware.Opera/Bookmarks");
#else
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
        QStringLiteral("/opera-software/Default/Bookmarks");
#endif
}

QString VivaldiBookmarkFile(){
#if defined(Q_OS_WIN)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QStringLiteral("/Vivaldi/User Data/Default/Bookmarks");
#elif defined(Q_OS_MAC)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QStringLiteral("/Vivaldi/Default/Bookmarks");
#else
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
        QStringLiteral("/vivaldi/Default/Bookmarks");
#endif
}

QString EdgeBookmarkFile(){
#if defined(Q_OS_WIN)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QStringLiteral("/Microsoft/Edge/User Data/Default/Bookmarks");
#elif defined(Q_OS_MAC)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QStringLiteral("/Microsoft Edge/Default/Bookmarks");
#else
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
        QStringLiteral("/microsoft-edge/Default/Bookmarks");
#endif
}

int LocalUtcOffset(){
    return QDateTime::currentDateTime().offsetFromUtc();
}

bool ReadInternalXml(const QByteArray &xml, ViewNode *root){
    QDomDocument doc;
    if(!ParseXml(xml, doc)) return false;

    std::function<void(QDomElement elem, ViewNode *parent)> collect;

    collect = [&](QDomElement elem, ViewNode *parent){
        ViewNode *vn = Append(parent);
        if(elem.attribute(QStringLiteral("primary"), QStringLiteral("false")) == QStringLiteral("true"))
            parent->SetPrimary(vn);
        if(elem.attribute(QStringLiteral("title"), QString()) != QString())
            vn->SetTitle(elem.attribute(QStringLiteral("title")));

        QString create = elem.attribute(QStringLiteral("create"), QString());
        QString lastAccess = elem.attribute(QStringLiteral("lastaccess"), QString());
        QString lastUpdate = elem.attribute(QStringLiteral("lastupdate"), QString());
        vn->SetCreateDate    (DateOrNow(create));
        vn->SetLastAccessDate(DateOrNow(lastAccess));
        vn->SetLastUpdateDate(DateOrNow(lastUpdate));

        if(elem.attribute(QStringLiteral("holdview"), QStringLiteral("false")) == QStringLiteral("true")){
            vn->SetHoldView(true);
            vn->SetUrl(QUrl::fromEncoded(elem.attribute(QStringLiteral("url")).toUtf8()));

            vn->SetScrollX(elem.attribute(QStringLiteral("scrollx"), QStringLiteral("0")).toInt());
            vn->SetScrollY(elem.attribute(QStringLiteral("scrolly"), QStringLiteral("0")).toInt());
            vn->SetZoom(elem.attribute(QStringLiteral("zoom"), QStringLiteral("1.0")).toFloat());
        } else {
            QDomNodeList children = elem.childNodes();

            for(int i = 0; i < children.length(); i++){
                collect(children.item(i).toElement(), vn);
            }
        }
    };

    QDomNodeList children = doc.documentElement().childNodes();
    for(int i = 0; i < children.length(); i++){
        collect(children.item(i).toElement(), root);
    }
    return true;
}

bool ReadXbel(const QByteArray &xml, ViewNode *root){
    QDomDocument doc;
    if(!ParseXml(xml, doc)) return false;

    std::function<void(QDomElement elem, ViewNode *parent)> collect;

    collect = [&](QDomElement elem, ViewNode *parent){
        if(elem.tagName() == QStringLiteral("folder")){
            ViewNode *vn = Append(parent);
            QString added = elem.attribute(QStringLiteral("added"), QString());

            vn->SetCreateDate    (DateOrNow(added, Qt::ISODate));
            vn->SetLastAccessDate(DateOrNow(added, Qt::ISODate));
            vn->SetLastUpdateDate(DateOrNow(added, Qt::ISODate));

            QDomNodeList children = elem.childNodes();
            for(int i = 0; i < children.length(); i++){
                collect(children.item(i).toElement(), vn);
            }
        } else if(elem.tagName() == QStringLiteral("bookmark")){
            ViewNode *vn = Append(parent);

            QString href = elem.attribute(QStringLiteral("href"), QString());
            QString added = elem.attribute(QStringLiteral("added"), QString());
            QString visited = elem.attribute(QStringLiteral("visited"), QString());
            QString modified = elem.attribute(QStringLiteral("modified"), QString());

            vn->SetHoldView(true);
            vn->SetUrl(QUrl::fromEncoded(href.toUtf8()));
            vn->SetCreateDate    (DateOrNow(added,    Qt::ISODate));
            vn->SetLastAccessDate(DateOrNow(visited,  Qt::ISODate));
            vn->SetLastUpdateDate(DateOrNow(modified, Qt::ISODate));

            QDomNodeList children = elem.childNodes();

            for(int i = 0; i < children.length(); i++){
                collect(children.item(i).toElement(), vn);
            }
        } else if(elem.tagName() == QStringLiteral("title")){
            parent->SetTitle(elem.text());
        } else {
        }
    };

    QDomNodeList children = doc.documentElement().childNodes();
    for(int i = 0; i < children.length(); i++){
        collect(children.item(i).toElement(), root);
    }
    return true;
}

bool ReadNetscapeHtml(const QString &html, ViewNode *root){
    if(!LooksLikeNetscapeHtml(html)) return false;

    ViewNode *vn = root;
    QRegularExpression tagrx(QStringLiteral("<(/?)([a-zA-Z0-9]+)([^<>]*)>([^<>]*)"));
    QRegularExpression attrrx(QStringLiteral(" ([a-zA-Z-_]+)=([^ <>]+)"));
    QRegularExpressionMatch tagmatch;
    QRegularExpressionMatch attrmatch;
    int pos = 0;
    while((tagmatch = tagrx.match(html, pos)).hasMatch() && vn){
        pos = tagmatch.capturedEnd();
        QStringList capture = tagmatch.capturedTexts();
        capture.takeFirst();

        QString close = capture.takeFirst();
        QString tagName = capture.takeFirst();

        if(tagName == QStringLiteral("DT")){
            if(close.isEmpty())
                vn = Append(vn);
        } else if(tagName == QStringLiteral("DL")){
            if(!close.isEmpty() && vn->GetParent())
                vn = vn->GetParent()->ToViewNode();
        } else if(tagName == QStringLiteral("H3") && close.isEmpty()){
            QString attrs = capture.takeFirst();
            int attrpos = 0;
            while((attrmatch = attrrx.match(attrs, attrpos)).hasMatch()){
                attrpos = attrmatch.capturedEnd();
                QStringList attrcapture = attrmatch.capturedTexts();
                attrcapture.takeFirst();
                QString name = attrcapture.takeFirst();
                QString attr = attrcapture.takeFirst();
                attr.replace(QStringLiteral("\""), QString());
                if(name == QStringLiteral("ADD_DATE")){
                    vn->SetCreateDate(QDateTime::fromSecsSinceEpoch(attr.toUInt()));
                } else if(name == QStringLiteral("LAST_VISIT")){
                    vn->SetLastAccessDate(QDateTime::fromSecsSinceEpoch(attr.toUInt()));
                } else if(name == QStringLiteral("LAST_MODIFIED")){
                    vn->SetLastUpdateDate(QDateTime::fromSecsSinceEpoch(attr.toUInt()));
                } else if(name == QStringLiteral("FOLDED")){
                    vn->SetFolded(attr == QStringLiteral("true") ? true : false);
                }
            }
            vn->SetTitle(HtmlUnescape(capture.takeFirst()));
        } else if(tagName == QStringLiteral("A") && close.isEmpty()){
            if(!vn->GetParent()) continue;

            vn->SetHoldView(true);
            QString attrs = capture.takeFirst();
            int attrpos = 0;
            while((attrmatch = attrrx.match(attrs, attrpos)).hasMatch()){
                attrpos = attrmatch.capturedEnd();
                QStringList attrcapture = attrmatch.capturedTexts();
                attrcapture.takeFirst();
                QString name = attrcapture.takeFirst();
                QString attr = attrcapture.takeFirst();
                attr.replace(QStringLiteral("\""), QString());
                if(name == QStringLiteral("HREF")){
                    vn->SetUrl(QUrl::fromEncoded(HtmlUnescape(attr).toUtf8()));
                } else if(name == QStringLiteral("ADD_DATE")){
                    vn->SetCreateDate(QDateTime::fromSecsSinceEpoch(attr.toUInt()));
                } else if(name == QStringLiteral("LAST_VISIT")){
                    vn->SetLastAccessDate(QDateTime::fromSecsSinceEpoch(attr.toUInt()));
                } else if(name == QStringLiteral("LAST_MODIFIED")){
                    vn->SetLastUpdateDate(QDateTime::fromSecsSinceEpoch(attr.toUInt()));
                }
            }
            vn->SetTitle(HtmlUnescape(capture.takeFirst()));
            Node *parent = vn->GetParent();
            vn = parent ? parent->ToViewNode() : nullptr;
        }
    }
    return true;
}

bool ReadChromeJson(const QByteArray &json, ViewNode *root){
    QJsonObject doc;
    if(!ParseJson(json, doc)) return false;

    QJsonObject roots = doc[QStringLiteral("roots")].toObject();

    std::function<void(ViewNode *parent, QJsonObject obj)> traverse;

    traverse = [&](ViewNode *parent, QJsonObject obj){
        ViewNode *vn = Append(parent);
        const QString name = QStringLiteral("name");
        const QString children = QStringLiteral("children");
        const QString url = QStringLiteral("url");
        vn->SetTitle(obj[name].toString());

        if(!obj.contains(children)){
            vn->SetHoldView(true);
            vn->SetUrl(QUrl(obj[url].toString()));
            return;
        }
        QJsonValue val = obj[children];
        if(!val.isArray()) return;
        foreach(QJsonValue child, val.toArray()){
            traverse(vn, child.toObject());
        }
    };

    ViewNode *rootnode = Append(root);
    rootnode->SetTitle(QStringLiteral("Favorites"));
    foreach(QString key, roots.keys()){
        if(!roots[key].isObject()) continue;
        traverse(rootnode, roots[key].toObject());
    }
    return true;
}

bool ReadFirefoxJson(const QByteArray &json, ViewNode *root){
    QJsonObject doc;
    if(!ParseJson(json, doc)) return false;

    std::function<void(ViewNode *parent, QJsonObject obj)> traverse;

    traverse = [&](ViewNode *parent, QJsonObject obj){
        ViewNode *vn = Append(parent);
        const QString title = QStringLiteral("title");
        const QString children = QStringLiteral("children");
        const QString uri = QStringLiteral("uri");
        vn->SetTitle(obj[title].toString());

        if(!obj.contains(children)){
            vn->SetHoldView(true);
            vn->SetUrl(QUrl(obj[uri].toString()));
            return;
        }
        QJsonValue val = obj[children];
        if(!val.isArray()) return;
        foreach(QJsonValue child, val.toArray()){
            traverse(vn, child.toObject());
        }
    };

    traverse(root, doc);
    return true;
}

bool ReadIeFavorites(const QString &directory, ViewNode *root){
    if(directory.isEmpty() || !QFileInfo(directory).isDir()) return false;

    std::function<void(ViewNode *parent, QString path)> traverse;

    traverse = [&](ViewNode *parent, QString path){
        ViewNode *vn = Append(parent);
        QString title = path.split(QStringLiteral("/")).last();
        if(title.endsWith(QStringLiteral(".url"))){
            title = title.left(title.length()-4);
        } else if(title.endsWith(QStringLiteral(".website"))){
            title = title.left(title.length()-8);
        }
        vn->SetTitle(title);

        if(QFileInfo(path).isDir()){
            QDir dir = path;
            QStringList files = dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
            foreach(QString file, files){
                traverse(vn, path + QStringLiteral("/") + file);
            }
        } else {
            QSettings info(path, QSettings::IniFormat);
            vn->SetHoldView(true);
            vn->SetUrl(info.value(QStringLiteral("InternetShortcut/URL")).toUrl());
        }
    };

    traverse(root, directory);
    return true;
}

bool ReadInternalXmlFile(const QString &path, ViewNode *root){
    bool ok = false;
    QByteArray data = ReadWholeFile(path, &ok);
    return ok && ReadInternalXml(data, root);
}

bool ReadXbelFile(const QString &path, ViewNode *root){
    bool ok = false;
    QByteArray data = ReadWholeFile(path, &ok);
    return ok && ReadXbel(data, root);
}

bool ReadNetscapeHtmlFile(const QString &path, ViewNode *root){
    bool ok = false;
    QByteArray data = ReadWholeFile(path, &ok);
    return ok && ReadNetscapeHtml(QString::fromUtf8(data), root);
}

bool ReadChromeJsonFile(const QString &path, ViewNode *root){
    bool ok = false;
    QByteArray data = ReadWholeFile(path, &ok);
    return ok && ReadChromeJson(data, root);
}

bool ReadFirefoxJsonFile(const QString &path, ViewNode *root){
    bool ok = false;
    QByteArray data = ReadWholeFile(path, &ok);
    return ok && ReadFirefoxJson(data, root);
}

QByteArray WriteInternalXml(ViewNode *root, const Hooks &hooks){
    QDomDocument doc;

    std::function<void(ViewNode *nd, QDomElement elem)> collect;

    collect = [&](ViewNode *nd, QDomElement elem){
        QDomElement child = doc.createElement(QStringLiteral("viewnode"));
        child.setAttribute(QStringLiteral("primary"), nd->IsPrimaryOfParent() ? QStringLiteral("true") : QStringLiteral("false"));
        child.setAttribute(QStringLiteral("holdview"), nd->HoldsView() ? QStringLiteral("true") : QStringLiteral("false"));
        child.setAttribute(QStringLiteral("title"), nd->GetTitle());

        elem.appendChild(child);
        if(nd->HoldsView()){
            child.setAttribute(QStringLiteral("index"), hooks.WindowIndex(nd));
            if(!nd->GetUrl().isEmpty()) child.setAttribute(QStringLiteral("url"), QString::fromUtf8(nd->GetUrl().toEncoded()));
            if(nd->GetScrollX()) child.setAttribute(QStringLiteral("scrollx"), QStringLiteral("%1").arg(nd->GetScrollX()));
            if(nd->GetScrollY()) child.setAttribute(QStringLiteral("scrolly"), QStringLiteral("%1").arg(nd->GetScrollY()));
            child.setAttribute(QStringLiteral("zoom"), QStringLiteral("%1").arg(static_cast<double>(nd->GetZoom())));
        } else {
            foreach(Node *childnode, nd->GetChildren()){
                collect(childnode->ToViewNode(), child);
            }
        }
    };

    doc.appendChild(doc.createProcessingInstruction(QStringLiteral("xml"),
                                                    QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));
    QDomElement elem = doc.createElement(QStringLiteral("viewnode"));
    elem.setAttribute(QStringLiteral("root"), QStringLiteral("true"));
    doc.appendChild(elem);
    foreach(Node *child, root->GetChildren()){
        collect(child->ToViewNode(), elem);
    }

    QString text;
    QTextStream out(&text);
    doc.save(out, 2);
    return text.toUtf8();
}

QByteArray WriteXbel(ViewNode *root, int utcOffset){
    QDomDocument doc(QStringLiteral("xbel"));

    std::function<void(ViewNode *nd, QDomElement elem)> collect;

    collect = [&](ViewNode *nd, QDomElement elem){
        QDomElement child;

        if(nd->HoldsView()){
            child = doc.createElement(QStringLiteral("bookmark"));
            child.setAttribute(QStringLiteral("href"), QString::fromUtf8(nd->GetUrl().toEncoded()));
            child.setAttribute(QStringLiteral("added"),    WithOffset(nd->GetCreateDate(),     utcOffset).toString(Qt::ISODate));
            child.setAttribute(QStringLiteral("visited"),  WithOffset(nd->GetLastAccessDate(), utcOffset).toString(Qt::ISODate));
            child.setAttribute(QStringLiteral("modified"), WithOffset(nd->GetLastUpdateDate(), utcOffset).toString(Qt::ISODate));
        } else {
            child = doc.createElement(QStringLiteral("folder"));
            child.setAttribute(QStringLiteral("folded"), nd->GetFolded() ? QStringLiteral("yes") : QStringLiteral("no"));
            child.setAttribute(QStringLiteral("added"), WithOffset(nd->GetCreateDate(), utcOffset).toString(Qt::ISODate));
        }

        QDomElement titleNode = doc.createElement(QStringLiteral("title"));
        QDomText title = doc.createTextNode(nd->GetTitle());
        titleNode.appendChild(title);
        child.appendChild(titleNode);

        elem.appendChild(child);

        if(!nd->HoldsView()){
            foreach(Node *childnode, nd->GetChildren()){
                collect(childnode->ToViewNode(), child);
            }
        }
    };

    doc.appendChild(doc.createProcessingInstruction(QStringLiteral("xml"),
                                                    QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));
    QDomElement elem = doc.createElement(QStringLiteral("xbel"));
    elem.setAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
    doc.appendChild(elem);
    foreach(Node *child, root->GetChildren()){
        collect(child->ToViewNode(), elem);
    }

    QString text;
    QTextStream out(&text);
    doc.save(out, 2);
    return text.toUtf8();
}

QString WriteNetscapeHtml(ViewNode *root, int utcOffset){
    QString text;
    QTextStream out(&text);

    std::function<void(ViewNode *vn, int nest)> collect;

    collect = [&](ViewNode *vn, int nest){
        out << QString(4*(nest+1), ' ');

        QString title = vn->GetTitle();
        if(title.isEmpty()) title = QStringLiteral("NoTitle");
        QDateTime added    = WithOffset(vn->GetCreateDate(),     utcOffset);
        QDateTime visited  = WithOffset(vn->GetLastAccessDate(), utcOffset);
        QDateTime modified = WithOffset(vn->GetLastUpdateDate(), utcOffset);

        if(vn->HoldsView()){
            out <<
                QStringLiteral("<DT><A HREF=\"%1\" ADD_DATE=\"%2\""
                               " LAST_VISIT=\"%3\" LAST_MODIFIED=\"%4\">"
                               "%5</A>\n").arg(HtmlEscape(QString::fromUtf8(vn->GetUrl().toEncoded())))
                                          .arg(added.toSecsSinceEpoch())
                                          .arg(visited.toSecsSinceEpoch())
                                          .arg(modified.toSecsSinceEpoch())
                                          .arg(HtmlEscape(title));
        } else {
            QString folded = vn->GetFolded() ? QStringLiteral("true") : QStringLiteral("false");
            out <<
                QStringLiteral("<DT><H3 ADD_DATE=\"%1\" LAST_VISIT=\"%2\""
                               " LAST_MODIFIED=\"%3\" FOLDED=\"%4\">"
                               "%5</H3>\n").arg(added.toSecsSinceEpoch())
                                           .arg(visited.toSecsSinceEpoch())
                                           .arg(modified.toSecsSinceEpoch())
                                           .arg(folded)
                                           .arg(HtmlEscape(title));
            out << QString(4*(nest+1), ' ') << QStringLiteral("<DL><p>\n");
            foreach(Node *child, vn->GetChildren()){
                collect(child->ToViewNode(), nest+1);
            }
            out << QString(4*(nest+1), ' ') << QStringLiteral("</DL><p>\n");
        }
    };

    out <<
        "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
        "<!-- This is an automatically generated file.\n"
        "     It will be read and overwritten.\n"
        "     DO NOT EDIT! -->\n"
        "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
        "<TITLE>Bookmarks</TITLE>\n"
        "<H1>Bookmarks</H1>\n\n";
    out << QStringLiteral("<DL><p>\n");
    foreach(Node *child, root->GetChildren()){
        collect(child->ToViewNode(), 0);
    }
    out << QStringLiteral("</DL><p>\n");

    return text;
}

bool WriteInternalXmlFile(const QString &path, ViewNode *root, const Hooks &hooks){
    return WriteWholeFile(path, WriteInternalXml(root, hooks));
}

bool WriteXbelFile(const QString &path, ViewNode *root, int utcOffset){
    return WriteWholeFile(path, WriteXbel(root, utcOffset));
}

bool WriteNetscapeHtmlFile(const QString &path, ViewNode *root, int utcOffset){
    return WriteWholeFile(path, WriteNetscapeHtml(root, utcOffset).toUtf8());
}

}
