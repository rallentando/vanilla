#ifndef BOOKMARKIO_HPP
#define BOOKMARKIO_HPP

#include "switch.hpp"

#include <QString>
#include <QByteArray>

#include <functional>

class ViewNode;

namespace BookmarkIO {

class Hooks {

public:
    std::function<int(ViewNode*)> WindowIndexOf;

    int WindowIndex(ViewNode *nd) const { return WindowIndexOf ? WindowIndexOf(nd) : 0;}
};

QString IeFavoritesDirectory();
QString FirefoxProfileDirectory();
QString FirefoxBookmarkBackup();
QString ChromeBookmarkFile();
QString OperaBookmarkFile();
QString VivaldiBookmarkFile();

int LocalUtcOffset();

bool ReadInternalXml(const QByteArray &xml, ViewNode *root);
bool ReadXbel(const QByteArray &xml, ViewNode *root);
bool ReadNetscapeHtml(const QString &html, ViewNode *root);
bool ReadChromeJson(const QByteArray &json, ViewNode *root);
bool ReadFirefoxJson(const QByteArray &json, ViewNode *root);
bool ReadIeFavorites(const QString &directory, ViewNode *root);

bool ReadInternalXmlFile(const QString &path, ViewNode *root);
bool ReadXbelFile(const QString &path, ViewNode *root);
bool ReadNetscapeHtmlFile(const QString &path, ViewNode *root);
bool ReadChromeJsonFile(const QString &path, ViewNode *root);
bool ReadFirefoxJsonFile(const QString &path, ViewNode *root);

QByteArray WriteInternalXml(ViewNode *root, const Hooks &hooks = Hooks());
QByteArray WriteXbel(ViewNode *root, int utcOffset);
QString    WriteNetscapeHtml(ViewNode *root, int utcOffset);

bool WriteInternalXmlFile(const QString &path, ViewNode *root, const Hooks &hooks = Hooks());
bool WriteXbelFile(const QString &path, ViewNode *root, int utcOffset);
bool WriteNetscapeHtmlFile(const QString &path, ViewNode *root, int utcOffset);

}

#endif
