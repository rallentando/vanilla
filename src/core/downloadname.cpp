#include "switch.hpp"

#include "downloadname.hpp"

#include <QByteArray>
#include <QChar>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStringConverter>
#include <QStringList>
#include <QStringView>

namespace {

const int MAX_NAME_LENGTH = 128;
const int MAX_NAME_BYTES = 200;

const int MAX_SUFFIX_LENGTH = 16;

QString Shorten(const QString &name){
    if(name.length() <= MAX_NAME_LENGTH && name.toUtf8().size() <= MAX_NAME_BYTES)
        return name;

    const int dot = name.lastIndexOf(QLatin1Char('.'));
    const QString tail =
        (dot > 0 && name.length() - dot <= MAX_SUFFIX_LENGTH) ? name.mid(dot) : QString();
    const int tailBytes = tail.toUtf8().size();

    QString stem = name.left(name.length() - tail.length());

    while(!stem.isEmpty() &&
          (stem.length() + tail.length() > MAX_NAME_LENGTH ||
           stem.toUtf8().size() + tailBytes > MAX_NAME_BYTES)){
        stem.chop(stem.at(stem.length() - 1).isLowSurrogate() && stem.length() > 1 ? 2 : 1);
    }

    while(!stem.isEmpty() &&
          (stem.endsWith(QLatin1Char('.')) || stem.endsWith(QLatin1Char(' '))))
        stem.chop(1);

    return stem + tail;
}

#ifdef Q_OS_WIN
bool IsDeviceName(const QString &name){
    const QString stem = name.section(QLatin1Char('.'), 0, 0).toUpper();

    if(stem == QStringLiteral("CON") || stem == QStringLiteral("PRN") ||
       stem == QStringLiteral("AUX") || stem == QStringLiteral("NUL"))
        return true;

    if(stem.length() == 4 &&
       (stem.startsWith(QStringLiteral("COM")) || stem.startsWith(QStringLiteral("LPT")))){
        const QChar last = stem.at(3);
        if((last >= QLatin1Char('1') && last <= QLatin1Char('9')) ||
           last == QChar(0x00b9) || last == QChar(0x00b2) || last == QChar(0x00b3))
            return true;
    }

    return false;
}
#endif

struct Parameter {
    QString name;
    QString value;
};

bool IsHexDigit(QChar c){
    return (c >= QLatin1Char('0') && c <= QLatin1Char('9')) ||
           (c >= QLatin1Char('a') && c <= QLatin1Char('f')) ||
           (c >= QLatin1Char('A') && c <= QLatin1Char('F'));
}

QList<Parameter> ParametersOf(const QString &value){
    QList<Parameter> out;

    int i = -1;

    while(i < value.length()){
        i++;

        int eq = i;
        while(eq < value.length() &&
              value.at(eq) != QLatin1Char('=') && value.at(eq) != QLatin1Char(';')) eq++;

        if(eq >= value.length() || value.at(eq) == QLatin1Char(';')){
            i = eq;
            continue;
        }

        Parameter p;
        p.name = value.mid(i, eq - i).trimmed().toLower();

        int j = eq + 1;
        while(j < value.length() && value.at(j) == QLatin1Char(' ')) j++;

        if(j < value.length() && value.at(j) == QLatin1Char('"')){
            j++;
            QString text;
            while(j < value.length() && value.at(j) != QLatin1Char('"')){
                if(value.at(j) == QLatin1Char('\\') && j + 1 < value.length()) j++;
                text += value.at(j++);
            }
            p.value = text;
            if(j < value.length()) j++;
            while(j < value.length() && value.at(j) != QLatin1Char(';')) j++;
        } else {
            int end = j;
            while(end < value.length() && value.at(end) != QLatin1Char(';')) end++;
            p.value = value.mid(j, end - j).trimmed();
            if(p.name != QStringLiteral("filename*") && p.value.length() > 1 &&
               p.value.startsWith(QLatin1Char('\'')) && p.value.endsWith(QLatin1Char('\'')))
                p.value = p.value.mid(1, p.value.length() - 2);
            j = end;
        }

        if(!p.name.isEmpty()) out.append(p);
        i = j;
    }

    return out;
}

QString FromExtendedValue(const QString &value){
    const int first = value.indexOf(QLatin1Char('\''));
    if(first == -1) return QString();
    const int second = value.indexOf(QLatin1Char('\''), first + 1);
    if(second == -1) return QString();

    const QString charset = value.left(first).trimmed().toLower();
    const QString encoded = value.mid(second + 1);
    if(encoded.isEmpty()) return QString();

    QByteArray bytes;
    bytes.reserve(encoded.length());

    for(int i = 0; i < encoded.length(); i++){
        const QChar c = encoded.at(i);

        if(c != QLatin1Char('%')){
            if(c.unicode() > 0x7f) return QString();
            bytes += static_cast<char>(c.unicode());
            continue;
        }

        if(i + 2 >= encoded.length()) return QString();
        if(!IsHexDigit(encoded.at(i + 1)) || !IsHexDigit(encoded.at(i + 2))) return QString();

        bytes += static_cast<char>(QStringView(encoded).mid(i + 1, 2).toUShort(nullptr, 16));
        i += 2;
    }

    if(charset == QStringLiteral("utf-8")){
        QStringDecoder utf8(QStringDecoder::Utf8);
        const QString text = utf8(bytes);
        if(utf8.hasError() || text.toUtf8() != bytes) return QString();
        return text;
    }

    if(charset == QStringLiteral("iso-8859-1")) return QString::fromLatin1(bytes);

    return QString();
}

}

QString DownloadName::Sanitize(const QString &name){
    const int slash = qMax(name.lastIndexOf(QLatin1Char('/')),
                           name.lastIndexOf(QLatin1Char('\\')));
    const QString base = slash == -1 ? name : name.mid(slash + 1);

    QString out;
    out.reserve(base.length());

    for(const QChar c : base){
        if(c.unicode() < 0x20 || c.unicode() == 0x7f) continue;

        switch(c.unicode()){
        case '<': case '>': case ':': case '"':
        case '/': case '\\': case '|': case '?': case '*':
            out += QLatin1Char('_');
            break;
        default:
            out += c;
            break;
        }
    }

    out = Shorten(out.trimmed());

    while(!out.isEmpty() &&
          (out.endsWith(QLatin1Char('.')) || out.endsWith(QLatin1Char(' '))))
        out.chop(1);

#ifdef Q_OS_WIN
    if(!out.isEmpty() && IsDeviceName(out)) out = QStringLiteral("_") + out;
#endif

    return out;
}

QString DownloadName::FromUrl(const QUrl &url){
    const QString path = url.path(QUrl::FullyDecoded);
    return Sanitize(path.section(QLatin1Char('/'), -1));
}

QString DownloadName::WithSuffixForMimeType(const QString &name, const QString &mimeType){
    const QString type = mimeType.section(QLatin1Char(';'), 0, 0).trimmed();
    if(type.isEmpty()) return name;

    static QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForName(type);

    if(!mime.isValid() || mime.isDefault()) return name;

    const QStringList suffixes = mime.suffixes();
    if(suffixes.isEmpty()) return name;

    const QString suffix = QFileInfo(name).suffix();
    if(!suffix.isEmpty() && suffixes.contains(suffix, Qt::CaseInsensitive)) return name;

    const QString preferred = mime.preferredSuffix();
    if(preferred.isEmpty()) return name;

    return name + QLatin1Char('.') + preferred;
}

QString DownloadName::FromContentDisposition(const QByteArray &header){
    if(header.isEmpty()) return QString();

    const QString value = QString::fromUtf8(header);

    QString plain;
    QString field;

    for(const Parameter &p : ParametersOf(value)){
        if(p.name == QStringLiteral("filename*")){
            const QString name = FromExtendedValue(p.value);
            if(!name.isEmpty()) return name;

        } else if(p.name == QStringLiteral("filename")){
            if(plain.isEmpty()) plain = p.value;

        } else if(p.name == QStringLiteral("name")){
            if(field.isEmpty()) field = p.value;
        }
    }

    return plain.isEmpty() ? field : plain;
}

QString DownloadName::Suggest(const QString &suggested,
                              const QString &mimeType,
                              const QUrl &url){
    QString name = Sanitize(suggested);
    if(name.isEmpty()) name = FromUrl(url);
    if(name.isEmpty()) name = QStringLiteral("download");

    return Shorten(WithSuffixForMimeType(name, mimeType));
}

QString DownloadName::Unique(const QString &path, int limit){
    if(path.isEmpty() || !QFileInfo::exists(path)) return path;

    const int slash = qMax(path.lastIndexOf(QLatin1Char('/')),
                           path.lastIndexOf(QLatin1Char('\\')));
    const QString head = path.left(slash + 1);
    const QString name = path.mid(slash + 1);

    int dot = name.lastIndexOf(QLatin1Char('.'));
    if(dot <= 0) dot = -1;

    QString stem = dot == -1 ? name : name.left(dot);

    if(dot != -1 && stem.endsWith(QStringLiteral(".tar"), Qt::CaseInsensitive))
        stem.chop(4);

    const QString tail = name.mid(stem.length());

    for(int n = 1; n <= limit; n++){
        const QString candidate =
            head + stem + QStringLiteral("_") + QString::number(n) + tail;
        if(!QFileInfo::exists(candidate)) return candidate;
    }

    return QString();
}
