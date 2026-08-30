#include "switch.hpp"
#include "const.hpp"

#include "treeserializer.hpp"

#include "lightnode.hpp"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QUrl>
#include <QDomDocument>
#include <QDomElement>

#include <charconv>
#include <climits>
#include <cmath>

namespace TreeSerializer {

static int m_ReadNodeCount = 0;
static int m_ReadViewCount = 0;

int ReadNodeCount(){ return m_ReadNodeCount;}
int ReadViewCount(){ return m_ReadViewCount;}

void ResetReadCounters(){
    m_ReadNodeCount = 0;
    m_ReadViewCount = 0;
}

static void ReadDates(ViewNode *vn, QString create, QString lastUpdate, QString lastAccess){
    vn->SetCreateDate    (create.isEmpty()     ? QDateTime::currentDateTime() : NodeDateTimeFromString(create));
    vn->SetLastUpdateDate(lastUpdate.isEmpty() ? QDateTime::currentDateTime() : NodeDateTimeFromString(lastUpdate));

    if(lastAccess.isEmpty())
        vn->SetLastAccessDate(QDateTime::currentDateTime());
    else if(create == lastAccess)
        vn->SetLastAccessDate(NodeDateTimeFromString(lastAccess).addSecs(1));
    else
        vn->SetLastAccessDate(NodeDateTimeFromString(lastAccess));
}

namespace {

struct Scalar {
    enum Kind { Null, False, True, Number, String } kind = Null;
    double number = 0.0;
    QString string;

    bool ToBool(bool def) const {
        return kind == True ? true : kind == False ? false : def;
    }
    QString ToString() const {
        return kind == String ? string : QString();
    }
    double ToDouble(double def) const {
        return kind == Number ? number : def;
    }
    int ToInt(int def) const {
        if(kind != Number) return def;
        if(number < INT_MIN || number > INT_MAX || std::trunc(number) != number) return def;
        return static_cast<int>(number);
    }
};

class JsonTreeReader {

public:
    JsonTreeReader(QIODevice *device, const Hooks &hooks)
        : m_Device(device), m_Hooks(hooks) {}

    bool ParseTree(ViewNode *root);

    QList<QPair<ViewNode*, int>> m_Restores;

private:
    static const int MAX_DEPTH = 512;

    static const int CHUNK = 64 * 1024;

    QIODevice *m_Device;
    const Hooks &m_Hooks;
    QByteArray m_Buffer;
    QByteArray m_Utf8;
    int m_Pos = 0;
    int m_Depth = 0;

    bool Refill(){
        m_Buffer = m_Device->read(CHUNK);
        m_Pos = 0;
        return !m_Buffer.isEmpty();
    }
    int Peek(){
        if(m_Pos >= m_Buffer.size() && !Refill()) return -1;
        return static_cast<uchar>(m_Buffer.at(m_Pos));
    }
    int Take(){
        int c = Peek();
        if(c >= 0) m_Pos++;
        return c;
    }
    void SkipSpace(){
        for(;;){
            const char *data = m_Buffer.constData();
            const int size = m_Buffer.size();
            while(m_Pos < size){
                const char c = data[m_Pos];
                if(c != ' ' && c != '\n' && c != '\r' && c != '\t') return;
                m_Pos++;
            }
            if(!Refill()) return;
        }
    }

    bool ParseNode(ViewNode *parent);
    bool ParseChildren(ViewNode *parent);
    bool ParseScalar(Scalar *out);
    bool ParseString(QString *out);
    bool ParseStringBytes(QByteArray *out);
    bool ParseEscape(QByteArray *out);
    bool EscapeChar(int e, QByteArray *out);
    int  ParseHex4();
    static void AppendCodePoint(QByteArray *out, uint cp);
    bool ParseNumber(double *out);
    bool ParseLiteral(const char *word);
    bool SkipValue();
};

bool JsonTreeReader::ParseTree(ViewNode *root){
    SkipSpace();
    if(Take() != '{') return false;
    SkipSpace();
    if(Peek() == '}'){
        Take();
    } else {
        QByteArray key;
        for(;;){
            SkipSpace();
            key.resize(0);
            if(!ParseStringBytes(&key)) return false;
            SkipSpace();
            if(Take() != ':') return false;
            SkipSpace();
            if(key == "children" && Peek() == '['){
                if(!ParseChildren(root)) return false;
            } else {
                if(!SkipValue()) return false;
            }
            SkipSpace();
            int c = Take();
            if(c == '}') break;
            if(c != ',') return false;
        }
    }
    SkipSpace();
    return Peek() < 0;
}

bool JsonTreeReader::ParseChildren(ViewNode *parent){
    if(++m_Depth > MAX_DEPTH) return false;
    if(Take() != '[') return false;
    SkipSpace();
    if(Peek() == ']'){
        Take();
    } else {
        for(;;){
            SkipSpace();
            if(!ParseNode(parent)) return false;
            SkipSpace();
            int c = Take();
            if(c == ']') break;
            if(c != ',') return false;
        }
    }
    --m_Depth;
    return true;
}

bool JsonTreeReader::ParseNode(ViewNode *parent){
    m_ReadNodeCount++;

    m_Hooks.Tock();

    ViewNode *vn = parent->MakeChild();

    Scalar primary, folded, title, create, lastUpdate, lastAccess,
           holdView, url, scrollX, scrollY, zoom, history, thumb, index;
#ifdef MEDIATIME
    Scalar mediaTime;
#endif

    const int restoreMark = m_Restores.length();

    if(Peek() == '{'){
        Take();
        SkipSpace();
        if(Peek() == '}'){
            Take();
        } else {
            QByteArray key;
            for(;;){
                SkipSpace();
                key.resize(0);
                if(!ParseStringBytes(&key)) return false;
                SkipSpace();
                if(Take() != ':') return false;
                SkipSpace();
                bool ok;
                if(key == "children" && Peek() == '['){
                    ok = ParseChildren(vn);
                } else if(key == "primary"){
                    ok = ParseScalar(&primary);
                } else if(key == "folded"){
                    ok = ParseScalar(&folded);
                } else if(key == "title"){
                    ok = ParseScalar(&title);
                } else if(key == "create"){
                    ok = ParseScalar(&create);
                } else if(key == "lastupdate"){
                    ok = ParseScalar(&lastUpdate);
                } else if(key == "lastaccess"){
                    ok = ParseScalar(&lastAccess);
                } else if(key == "holdview"){
                    ok = ParseScalar(&holdView);
                } else if(key == "url"){
                    ok = ParseScalar(&url);
                } else if(key == "scrollx"){
                    ok = ParseScalar(&scrollX);
                } else if(key == "scrolly"){
                    ok = ParseScalar(&scrollY);
                } else if(key == "zoom"){
                    ok = ParseScalar(&zoom);
#ifdef MEDIATIME
                } else if(key == "mediatime"){
                    ok = ParseScalar(&mediaTime);
#endif
                } else if(key == "history"){
                    ok = ParseScalar(&history);
                } else if(key == "thumb"){
                    ok = ParseScalar(&thumb);
                } else if(key == "index"){
                    ok = ParseScalar(&index);
                } else {
                    ok = SkipValue();
                }
                if(!ok) return false;
                SkipSpace();
                int c = Take();
                if(c == '}') break;
                if(c != ',') return false;
            }
        }
    } else {
        if(!SkipValue()) return false;
    }

    if(primary.ToBool(false))
        parent->SetPrimary(vn);
    vn->SetFolded(folded.ToBool(true));
    if(!title.ToString().isEmpty())
        vn->SetTitle(title.ToString());

    ReadDates(vn,
              create.ToString(),
              lastUpdate.ToString(),
              lastAccess.ToString());

    if(holdView.ToBool(false)){
        m_ReadViewCount++;

        if(vn->ChildrenLength()){
            while(m_Restores.length() > restoreMark) m_Restores.removeLast();
            vn->SetPrimary(nullptr);
            foreach(Node *child, vn->GetChildren()){
                delete child;
            }
            vn->ClearChildren();
        }

        vn->SetHoldView(true);
        vn->SetUrl(QUrl::fromEncoded(url.ToString().toLatin1()));

        vn->SetScrollX(scrollX.ToInt(0));
        vn->SetScrollY(scrollY.ToInt(0));
        float z = zoom.ToDouble(1.0);
        if(z > 10) vn->SetZoom(z/100);
        else       vn->SetZoom(z);

#ifdef MEDIATIME
        vn->SetMediaTime(static_cast<float>(mediaTime.ToDouble(0)));
#endif

        if(!history.ToString().isEmpty())
            vn->SetHistoryFileName(history.ToString());

        if(!thumb.ToString().isEmpty())
            vn->SetImageFileName(thumb.ToString());

        int id = index.ToInt(0);

        if(id) m_Restores << qMakePair(vn, id);
    }
    return true;
}

bool JsonTreeReader::ParseScalar(Scalar *out){
    switch(Peek()){
    case '"':
        out->kind = Scalar::String;
        return ParseString(&out->string);
    case 't':
        out->kind = Scalar::True;
        return ParseLiteral("true");
    case 'f':
        out->kind = Scalar::False;
        return ParseLiteral("false");
    case 'n':
        out->kind = Scalar::Null;
        return ParseLiteral("null");
    case '{': case '[':
        out->kind = Scalar::Null;
        return SkipValue();
    default:
        out->kind = Scalar::Number;
        return ParseNumber(&out->number);
    }
}

bool JsonTreeReader::ParseString(QString *out){
    if(!out) return ParseStringBytes(nullptr);
    m_Utf8.resize(0);
    if(!ParseStringBytes(&m_Utf8)) return false;
    *out = QString::fromUtf8(m_Utf8);
    return true;
}

bool JsonTreeReader::ParseStringBytes(QByteArray *out){
    if(Take() != '"') return false;
    for(;;){
        const char *data = m_Buffer.constData();
        const int size = m_Buffer.size();
        const int start = m_Pos;
        int i = m_Pos;
        while(i < size){
            const char c = data[i];
            if(c == '"' || c == '\\') break;
            i++;
        }
        if(out && i > start) out->append(data + start, i - start);
        m_Pos = i;

        int c = Take();
        if(c < 0) return false;
        if(c == '"') return true;
        if(c == '\\'){
            if(!ParseEscape(out)) return false;
        } else if(out){
            *out += static_cast<char>(c);
        }
    }
}

bool JsonTreeReader::ParseEscape(QByteArray *out){
    return EscapeChar(Take(), out);
}

bool JsonTreeReader::EscapeChar(int e, QByteArray *out){
    switch(e){
    case '"':  if(out) *out += '"';  return true;
    case '\\': if(out) *out += '\\'; return true;
    case '/':  if(out) *out += '/';  return true;
    case 'b':  if(out) *out += '\b'; return true;
    case 'f':  if(out) *out += '\f'; return true;
    case 'n':  if(out) *out += '\n'; return true;
    case 'r':  if(out) *out += '\r'; return true;
    case 't':  if(out) *out += '\t'; return true;
    case 'u':{
        int unit = ParseHex4();
        if(unit < 0) return false;
        if(unit >= 0xD800 && unit <= 0xDBFF){
            if(Peek() == '\\'){
                Take();
                int f = Take();
                if(f == 'u'){
                    int low = ParseHex4();
                    if(low < 0) return false;
                    if(low >= 0xDC00 && low <= 0xDFFF){
                        AppendCodePoint(out, 0x10000u
                                        + ((static_cast<uint>(unit) - 0xD800) << 10)
                                        + (static_cast<uint>(low) - 0xDC00));
                    } else {
                        AppendCodePoint(out, static_cast<uint>(unit));
                        AppendCodePoint(out, static_cast<uint>(low));
                    }
                    return true;
                }
                AppendCodePoint(out, static_cast<uint>(unit));
                return EscapeChar(f, out);
            }
        }
        AppendCodePoint(out, static_cast<uint>(unit));
        return true;
    }
    default: return false;
    }
}

int JsonTreeReader::ParseHex4(){
    int value = 0;
    for(int i = 0; i < 4; i++){
        int c = Take();
        if     (c >= '0' && c <= '9') value = value * 16 + (c - '0');
        else if(c >= 'a' && c <= 'f') value = value * 16 + (c - 'a' + 10);
        else if(c >= 'A' && c <= 'F') value = value * 16 + (c - 'A' + 10);
        else return -1;
    }
    return value;
}

void JsonTreeReader::AppendCodePoint(QByteArray *out, uint cp){
    if(!out) return;
    if(cp >= 0xD800 && cp <= 0xDFFF) cp = 0xFFFD;
    if(cp < 0x80){
        *out += static_cast<char>(cp);
    } else if(cp < 0x800){
        *out += static_cast<char>(0xC0 | (cp >> 6));
        *out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if(cp < 0x10000){
        *out += static_cast<char>(0xE0 | (cp >> 12));
        *out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        *out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        *out += static_cast<char>(0xF0 | (cp >> 18));
        *out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        *out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        *out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

bool JsonTreeReader::ParseNumber(double *out){
    char buf[48];
    int len = 0;
    const auto put = [&buf, &len](int c){
        if(len < int(sizeof(buf)) - 1) buf[len++] = static_cast<char>(c);
    };
    int c = Peek();
    if(c == '-'){
        Take();
        put('-');
        c = Peek();
    }
    if(c < '0' || c > '9') return false;
    while(c >= '0' && c <= '9'){
        Take();
        put(c);
        c = Peek();
    }
    if(c == '.'){
        Take();
        put('.');
        c = Peek();
        if(c < '0' || c > '9') return false;
        while(c >= '0' && c <= '9'){
            Take();
            put(c);
            c = Peek();
        }
    }
    if(c == 'e' || c == 'E'){
        Take();
        put('e');
        c = Peek();
        if(c == '+' || c == '-'){
            Take();
            put(c);
            c = Peek();
        }
        if(c < '0' || c > '9') return false;
        while(c >= '0' && c <= '9'){
            Take();
            put(c);
            c = Peek();
        }
    }
    const std::from_chars_result result = std::from_chars(buf, buf + len, *out);
    return result.ec == std::errc() && result.ptr == buf + len;
}

bool JsonTreeReader::ParseLiteral(const char *word){
    for(const char *p = word; *p; ++p){
        if(Take() != *p) return false;
    }
    return true;
}

bool JsonTreeReader::SkipValue(){
    SkipSpace();
    switch(Peek()){
    case '{':{
        if(++m_Depth > MAX_DEPTH) return false;
        Take();
        SkipSpace();
        if(Peek() == '}'){
            Take();
        } else {
            for(;;){
                SkipSpace();
                if(!ParseStringBytes(nullptr)) return false;
                SkipSpace();
                if(Take() != ':') return false;
                if(!SkipValue()) return false;
                SkipSpace();
                int c = Take();
                if(c == '}') break;
                if(c != ',') return false;
            }
        }
        --m_Depth;
        return true;
    }
    case '[':{
        if(++m_Depth > MAX_DEPTH) return false;
        Take();
        SkipSpace();
        if(Peek() == ']'){
            Take();
        } else {
            for(;;){
                if(!SkipValue()) return false;
                SkipSpace();
                int c = Take();
                if(c == ']') break;
                if(c != ',') return false;
            }
        }
        --m_Depth;
        return true;
    }
    case '"': return ParseStringBytes(nullptr);
    case 't': return ParseLiteral("true");
    case 'f': return ParseLiteral("false");
    case 'n': return ParseLiteral("null");
    default:{
        double d;
        return ParseNumber(&d);
    }
    }
}

}

bool ReadJsonFile(const QString &path, ViewNode *root, const Hooks &hooks){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return false;

    ViewNode *scratch = new ViewNode();
    JsonTreeReader reader(&file, hooks);
    bool ok = reader.ParseTree(scratch);
    file.close();
    if(!ok){
        delete scratch;
        return false;
    }

    foreach(Node *child, scratch->GetChildren()){
        child->SetParent(root);
        root->AppendChild(child);
    }
    if(scratch->GetPrimary()) root->SetPrimary(scratch->GetPrimary());
    scratch->ClearChildren();
    delete scratch;

    typedef QPair<ViewNode*, int> Restore;
    foreach(const Restore &restore, reader.m_Restores){
        hooks.Restore(restore.first, restore.second);
    }
    return true;
}

bool ReadLegacyXmlFile(const QString &path, ViewNode *root, const Hooks &hooks){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return false;
    QDomDocument doc;
    bool check = !!doc.setContent(&file);
    file.close();
    if(!check) return false;

    QDomNodeList children = doc.documentElement().childNodes();
    QDomElement de;
    for(int i = 0; i < children.length(); i++){
        de = children.item(i).toElement();
        ReadLegacyNode(de, root, hooks);
    }
    return true;
}

bool WriteJsonFile(const QString &path, ViewNode *root, const Hooks &hooks){
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)) return false;

    QTextStream out(&file);
    out << "{\n";
    QList<Node*> children = root->GetChildren();
    if(children.isEmpty()){
        out << "\"children\": []\n";
    } else {
        out << "\"children\": [{\n";
        for(int i = 0; i < children.length(); i++){
            WriteNode(children[i]->ToViewNode(), out, 1, hooks);
            out << (i == children.length() - 1 ? "}]\n" : "},{\n");
        }
    }
    out << "}\n";
    out.flush();
    const bool ok = file.flush() && file.error() == QFileDevice::NoError;
    file.close();
    if(!ok) file.remove();
    return ok;
}

void ReadLegacyNode(const QDomElement &elem, ViewNode *parent, const Hooks &hooks){
    m_ReadNodeCount++;

    hooks.Tock();

    ViewNode *vn = parent->MakeChild();
    if(elem.attribute(QStringLiteral("primary"), QStringLiteral("false")) == QStringLiteral("true"))
        parent->SetPrimary(vn);
    if(elem.attribute(QStringLiteral("folded"), QStringLiteral("true")) == QStringLiteral("true"))
        vn->SetFolded(true);
    else vn->SetFolded(false);
    if(elem.attribute(QStringLiteral("title"), QString()) != QString())
        vn->SetTitle(elem.attribute(QStringLiteral("title")));

    ReadDates(vn,
              elem.attribute(QStringLiteral("create"),     QString()),
              elem.attribute(QStringLiteral("lastupdate"), QString()),
              elem.attribute(QStringLiteral("lastaccess"), QString()));

    if(elem.attribute(QStringLiteral("holdview"), QStringLiteral("false")) == QStringLiteral("true")){
        m_ReadViewCount++;

        vn->SetHoldView(true);
        vn->SetUrl(QUrl::fromEncoded(elem.attribute(QStringLiteral("url")).toLatin1()));

        vn->SetScrollX(elem.attribute(QStringLiteral("scrollx"), QStringLiteral("0")).toInt());
        vn->SetScrollY(elem.attribute(QStringLiteral("scrolly"), QStringLiteral("0")).toInt());
        float zoom = elem.attribute(QStringLiteral("zoom"), QStringLiteral("1.0")).toFloat();
        if(zoom > 10) vn->SetZoom(zoom/100);
        else          vn->SetZoom(zoom);

        if(!elem.attribute(QStringLiteral("history")).isEmpty())
            vn->SetHistoryFileName(elem.attribute(QStringLiteral("history")));

        if(!elem.attribute(QStringLiteral("thumb")).isEmpty())
            vn->SetImageFileName(elem.attribute(QStringLiteral("thumb")));

        int id = elem.attribute(QStringLiteral("index"), QStringLiteral("0")).toInt();

        if(id) hooks.Restore(vn, id);

    } else {
        QDomNodeList children = elem.childNodes();
        QDomElement de;
        for(int i = 0; i < children.length(); i++){
            de = children.item(i).toElement();
            ReadLegacyNode(de, vn, hooks);
        }
    }
}

QString JsonEscape(QString str){
    str.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    str.replace(QLatin1Char('"'),  QStringLiteral("\\\""));
    str.replace(QLatin1Char('\b'), QStringLiteral("\\b"));
    str.replace(QLatin1Char('\f'), QStringLiteral("\\f"));
    str.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    str.replace(QLatin1Char('\r'), QStringLiteral("\\r"));
    str.replace(QLatin1Char('\t'), QStringLiteral("\\t"));
    for(int i = 0; i < str.length(); i++){
        if(str.at(i) >= QChar(0x20)) continue;
        QString escaped = QStringLiteral("\\u%1")
            .arg(static_cast<uint>(str.at(i).unicode()), 4, 16, QLatin1Char('0'));
        str.replace(i, 1, escaped);
        i += escaped.length() - 1;
    }
    return str;
}

void WriteNode(ViewNode *nd, QTextStream &out, int depth, const Hooks &hooks){
    QString pad = QString((depth - 1) * 2, QLatin1Char(' '));

    out << pad << "\"primary\": "       << (nd->IsPrimaryOfParent() ? "true" : "false")
        <<        ", \"holdview\": "    << (nd->HoldsView()         ? "true" : "false")
        <<        ", \"folded\": "      << (nd->GetFolded()         ? "true" : "false")
        <<        ", \"title\": \""     << JsonEscape(nd->GetTitle())                              << "\",\n"
        << pad << "\"create\": \""      << nd->GetCreateDate().toString(NODE_DATETIME_FORMAT)
        <<        "\", \"lastupdate\": \"" << nd->GetLastUpdateDate().toString(NODE_DATETIME_FORMAT)
        <<        "\", \"lastaccess\": \"" << nd->GetLastAccessDate().toString(NODE_DATETIME_FORMAT) << "\"";

    if(nd->HoldsView()){
        out << ",\n"
            << pad << "\"index\": "     << hooks.WindowIndex(nd)
            <<        ", \"url\": \""   << JsonEscape(QString::fromUtf8(nd->GetUrl().toEncoded()))
            <<        "\", \"scrollx\": " << nd->GetScrollX()
            <<        ", \"scrolly\": "   << nd->GetScrollY()
            <<        ", \"zoom\": "      << nd->GetZoom();
#ifdef MEDIATIME
        out << ", \"mediatime\": " << nd->GetMediaTime();
#endif

        if(hooks.KeepSideFiles(nd)){
            nd->SaveImageIfNeed();
            nd->SaveHistoryIfNeed();
            out << ",\n"
                << pad << "\"history\": \"" << JsonEscape(nd->GetHistoryFileName())
                <<        "\", \"thumb\": \"" << JsonEscape(nd->GetImageFileName()) << "\"";
        }
        out << "\n";
    } else {
        QList<Node*> children = nd->GetChildren();
        if(children.isEmpty()){
            out << ",\n" << pad << "\"children\": []\n";
        } else {
            out << ",\n" << pad << "\"children\": [{\n";
            QString cpad = QString(depth * 2, QLatin1Char(' '));
            for(int i = 0; i < children.length(); i++){
                WriteNode(children[i]->ToViewNode(), out, depth + 1, hooks);
                out << cpad << (i == children.length() - 1 ? "}]\n" : "},{\n");
            }
        }
    }
}

}
