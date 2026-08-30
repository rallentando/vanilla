#ifndef FILEOPERATION_HPP
#define FILEOPERATION_HPP

#include <QString>
#include <QStringList>

namespace FileOperation {

struct Reach {
    int files = 0;
    int directories = 0;
};

Reach ReachOf(const QStringList &paths);

bool Remove(const QString &path, QStringList *failed = nullptr);

bool Copy(const QString &source, const QString &destination,
          QStringList *failed = nullptr);

QString UniqueName(const QString &dir, const QString &name);

}

#endif
