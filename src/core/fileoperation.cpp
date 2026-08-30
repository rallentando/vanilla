#include "fileoperation.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace FileOperation {

namespace {

bool IsRealDirectory(const QFileInfo &info){
    return info.isDir() && !info.isSymLink() && !info.isJunction();
}

QFileInfoList EntriesOf(const QString &path){
    return QDir(path).entryInfoList
        (QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
}

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
const Qt::CaseSensitivity PathCase = Qt::CaseInsensitive;
#else
const Qt::CaseSensitivity PathCase = Qt::CaseSensitive;
#endif

QString ResolvedPath(const QString &path){
    QFileInfo info(QFileInfo(path).absoluteFilePath());
    QString tail;

    for(int i = 0; i < 256; i++){
        const QString canonical = info.canonicalFilePath();
        if(!canonical.isEmpty())
            return tail.isEmpty()
                ? canonical
                : canonical + QLatin1Char('/') + tail;

        const QString parent = info.path();
        if(info.fileName().isEmpty() ||
           parent == info.filePath()) break;

        tail = tail.isEmpty()
            ? info.fileName()
            : info.fileName() + QLatin1Char('/') + tail;
        info = QFileInfo(parent);
    }
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool IsLink(const QFileInfo &info){
    return info.isSymLink() || info.isJunction();
}

bool IsDirectoryShapedLink(const QFileInfo &info){
#if defined(Q_OS_WIN)
    return info.isJunction() || (info.isSymbolicLink() && info.isDir());
#else
    Q_UNUSED(info)
    return false;
#endif
}

bool RemoveFile(const QString &path){
    if(QFile::remove(path)) return true;

    const QFile::Permissions permissions = QFile::permissions(path);
    if(permissions & QFile::WriteUser) return false;

    QFile::setPermissions(path, permissions | QFile::WriteUser);
    if(QFile::remove(path)) return true;

    QFile::setPermissions(path, permissions);
    return false;
}

bool IsAtOrUnder(const QString &path, const QString &ancestor){
    return path.compare(ancestor, PathCase) == 0 ||
           path.startsWith(ancestor + QLatin1Char('/'), PathCase);
}

void AddReach(const QFileInfo &info, Reach *reach){
    if(!IsRealDirectory(info)){
        reach->files++;
        return;
    }
    reach->directories++;
    foreach(const QFileInfo &entry, EntriesOf(info.absoluteFilePath()))
        AddReach(entry, reach);
}

QString NumberedName(const QString &name, int n){
    const QFileInfo info(name);
    const QString base = info.baseName();
    const QString suffix = info.completeSuffix();
    if(base.isEmpty() || suffix.isEmpty())
        return QStringLiteral("%1 (%2)").arg(name).arg(n);
    return QStringLiteral("%1 (%2).%3").arg(base).arg(n).arg(suffix);
}

}

Reach ReachOf(const QStringList &paths){
    Reach reach;
    foreach(const QString &path, paths){
        const QFileInfo info(path);
        if(!info.exists() && !info.isSymLink()) continue;
        AddReach(info, &reach);
    }
    return reach;
}

bool Remove(const QString &path, QStringList *failed){
    if(path.isEmpty()) return false;

    const QFileInfo info(path);
    if(!info.exists() && !info.isSymLink()){
        if(failed) *failed << path;
        return false;
    }

    if(!IsRealDirectory(info)){
        const bool ok = IsLink(info)
            ? (IsDirectoryShapedLink(info)
               ? (QDir().rmdir(path) || QFile::remove(path))
               : (QFile::remove(path) || QDir().rmdir(path)))
            : RemoveFile(path);

        if(!ok && failed) *failed << path;
        return ok;
    }

    bool childrenWent = true;
    foreach(const QFileInfo &entry, EntriesOf(path))
        childrenWent = Remove(entry.absoluteFilePath(), failed) && childrenWent;

    const bool ok = QDir().rmdir(path);
    if(!ok && childrenWent && failed) *failed << path;
    return ok && childrenWent;
}

bool Copy(const QString &source, const QString &destination, QStringList *failed){
    if(source.isEmpty() || destination.isEmpty()){
        if(failed && !source.isEmpty()) *failed << source;
        return false;
    }

    const QFileInfo from(source);
    if(!from.exists() && !from.isSymLink()){
        if(failed) *failed << source;
        return false;
    }

    if(IsAtOrUnder(ResolvedPath(destination), ResolvedPath(source))){
        if(failed) *failed << source;
        return false;
    }

    if(from.isSymbolicLink() || from.isJunction()){
        if(failed) *failed << source;
        return false;
    }

    if(!IsRealDirectory(from)){
        const bool ok = QFile::copy(source, destination);
        if(!ok && failed) *failed << source;
        return ok;
    }

    if(!QDir().mkpath(destination)){
        if(failed) *failed << source;
        return false;
    }

    bool ok = true;
    foreach(const QFileInfo &entry, EntriesOf(source)){
        ok = Copy(entry.absoluteFilePath(),
                  destination + QStringLiteral("/") + entry.fileName(),
                  failed) && ok;
    }
    return ok;
}

QString UniqueName(const QString &dir, const QString &name){
    if(name.isEmpty()) return QString();

    const QDir directory(dir);
    if(!directory.exists(name)) return name;

    for(int n = 2; n < 10000; n++){
        const QString candidate = NumberedName(name, n);
        if(!directory.exists(candidate)) return candidate;
    }
    return QString();
}

}
