#ifndef DIRECTORYPAGE_HPP
#define DIRECTORYPAGE_HPP

#include "switch.hpp"

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QUrl>

#include <functional>

class ViewNode;
class QMenu;
class QWidget;

namespace DirectoryPage {

struct Token {
    const char *key;
    const char *label;
    const char *hint;
    const char *pattern;
    const char *on;
    const char *off;
    int absence;
    bool inverted;
    bool ownOnly;
};

const QList<Token> &Tokens();

QString TokenSubject(const QString &token);

QStringList InheritTokens(const QStringList &titlesNearestFirst);

int StateIn(const QStringList &set, const QString &pattern);

QStringList ChangedWords(const QString &before, const QString &after);

QString TitleFollowing(const QString &title, const QStringList &changed);

QString     TitleName(const QString &title);
QStringList TitleTokens(const QString &title);
QString     ComposeTitle(const QString &name, const QStringList &tokens);

int     TokenState(const QString &title, const Token &token);
QString WithTokenState(const QString &title, const Token &token, int state);

bool TokenBlocked(const Token &token);

bool IsValidTitle(const QString &title);

QUrl PageUrl();
bool IsPageUrl(const QUrl &url);

QJsonObject Describe();

QByteArray HandleSet(const QJsonObject &request);

QMenu *CreateSettingsMenu(ViewNode *vn, std::function<void()> openPage,
                          QWidget *parent);

}

#endif
