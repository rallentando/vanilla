#ifndef EXTENSIONCOPY_HPP
#define EXTENSIONCOPY_HPP

#include "switch.hpp"

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace ExtensionCopy {

struct Rewritten {
    QString refusal;
    QJsonObject manifest;
    QString workerWrapper;
    QString workerWrapperText;
    QString workerShim;
    QString contentShim;
    QString carrier;
    QString pageShim;
    QString pageShimOpen;
    QString relayPage;
    QString relayScript;
    QStringList webAccessible;
    bool keyless = false;
    QStringList sandboxed;
};

QString ConnectingPolicy(const QString &policy);

Rewritten Rewrite(const QJsonObject &manifest, int stamp);

int PlaceOfPageShim(const QByteArray &html);
QByteArray PageShimLine(bool open = false);
bool Globbed(const QString &pattern, const QString &path);

bool CarriablePattern(const QString &pattern);
QString HostPatternOf(const QString &pattern);
bool Readable(const QStringList &patterns, const QString &relative);

QJsonObject Messages(const QString &source, const QJsonObject &manifest, const QString &locale);
QString UiLocale();
QByteArray MessagesLine(const QJsonObject &messages, const QString &locale);

int Stamp(const QString &source, const QByteArray &manifestBytes, const QByteArray &key = QByteArray(),
          const QString &locale = QString());

struct Made {
    QString path;
    bool keyless = false;
    bool withheld = false;
    QString note;
    QString detail;
};

Made Make(const QString &source, const QString &root, const QString &id, const QByteArray &key = QByteArray(),
          const QString &locale = QString());

QString SourceOf(const QString &path);

bool Stale(bool shimsOn, bool known, const QString &registered, const QString &current);

QString Pin(const QString &root, const QString &id, const QString &copy);
QString PinnedPath(const QString &root, const QString &id);

bool Held(bool shimsOn, const QString &registered, const QString &pinned, const QString &current);
bool Behind(const QString &registered, const QString &pinned, const QString &loaded, const QString &copy);
bool KeylessOfLoaded(bool was, const QString &loaded, const QString &copy, bool copyKeyless);

void ForgetForTesting();

}

namespace ExtensionMessages {
    using ExtensionCopy::Messages;
    using ExtensionCopy::UiLocale;
}

#endif
