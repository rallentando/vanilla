#include "switch.hpp"
#include "const.hpp"

#include "directorypage.hpp"

#include <QCheckBox>
#include <QCoreApplication>
#include <QGridLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QRegularExpression>
#include <QMap>
#include <QSet>
#include <QWidgetAction>

#include "application.hpp"
#include "lightnode.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "useragent.hpp"
#include "view.hpp"
#ifdef WEBENGINEVIEW
#  include <QWebEngineProfile>
#  include <QWebEngineSettings>
#endif

namespace {

    const DirectoryPage::Token TOKENS[] = {
        { "id",
          QT_TRANSLATE_NOOP("DirectoryPage", "Use folder name as profile ID"),
          QT_TRANSLATE_NOOP("DirectoryPage",
              "On, this directory's name is given a profile of its own: its "
              "cookies, its storage and its cache. Different names hold "
              "different data; the same name shares it. Renaming loses what "
              "the old name held."),
          "[iI][dD](?:entif(?:y|ier|ication))?", "ID", "", -1, false, true },
        { "private",
          QT_TRANSLATE_NOOP("DirectoryPage", "Private mode"),
          QT_TRANSLATE_NOOP("DirectoryPage",
              "On, nothing under this directory is kept on this machine: no "
              "cookies, no history, no cache, no session. What the sites "
              "themselves see is unchanged. The private directories of one "
              "profile share one private session, which lasts until the "
              "application quits."),
          "(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)", "Private", "!Private", -1, false, false },
        { "autoload",
          QT_TRANSLATE_NOOP("DirectoryPage", "Auto load"),
          QT_TRANSLATE_NOOP("DirectoryPage",
              "On, the tabs around the one being looked at are loaded "
              "without waiting to be opened. It cannot be turned on unless "
              "\"Hidden tabs\" is \"Active\". Off, a tab under this "
              "directory is loaded only when it is opened. (Default: off)"),
          "[nN](?:o)?(?:[aA](?:uto)?)?[lL](?:oad)?", "!NoAutoLoad", "NoAutoLoad",
          0, true, false },
        { "rightgesture",
          QT_TRANSLATE_NOOP("DirectoryPage", "Mouse gestures"),
          QT_TRANSLATE_NOOP("DirectoryPage",
              "Dragging with the right mouse button runs the action assigned "
              "to the shape drawn."),
          "(?:[rR](?:ight)?[gG](?:esture)?|[mM](?:ouse)?[gG](?:esture)?)",
          "RightGesture", "!RightGesture", -1, false, false },
        { "draggesture",
          QT_TRANSLATE_NOOP("DirectoryPage", "Super drag"),
          QT_TRANSLATE_NOOP("DirectoryPage",
              "Dragging a link, an image or selected text inside a page is a "
              "gesture which opens it, instead of handing it to whatever the "
              "drop lands on."),
          "[dD](?:rag)?[gG](?:esture)?", "DragGesture", "!DragGesture",
          -1, false, false },
        { "image",
          QT_TRANSLATE_NOOP("DirectoryPage", "Show images"),
          QT_TRANSLATE_NOOP("DirectoryPage",
              "Off, pages under this directory are shown without their "
              "images. Pages which do not allow for that come out "
              "misshapen."),
          "[iI]mage", "Image", "!Image", -1, false, false },
        { "javascript",
          QT_TRANSLATE_NOOP("DirectoryPage", "Enable javascript"),
          QT_TRANSLATE_NOOP("DirectoryPage",
              "Off, scripts do not run under this directory. Pages which "
              "need one do not work properly."),
          "[jJ](?:ava)?[sS](?:cript)?", "Javascript", "!Javascript",
          -1, false, false },
        { "plugins",
          QT_TRANSLATE_NOOP("DirectoryPage", "Enable plugins"),
          QT_TRANSLATE_NOOP("DirectoryPage",
              "The engine's built-in plugins, the PDF viewer above all"),
          "[pP]lugins?", "Plugins", "!Plugins", -1, false, false },
    };

    struct Family {
        const char *subject;
        const char *pattern;
    };
    const Family FAMILIES[] = {
        { "encoding",  "(?:[dD]efault)?(?:[tT]ext)?(?:[eE]ncod(?:e|ing)|[cC]odecs?) [^ ].*" },
        { "useragent", "[uU](?:ser)?[aA](?:gent)? [^ ]+" },
        { "proxy",     "[pP][rR][oO][xX][yY] [^ ].*" },
        { "ssl",       "[sS][sS][lL] [^ ]+" },
        { "viewkind",
          "(?:[gG](?:raphics)?|[qQ](?:uick)?)?[wW](?:eb)?(?:[eE](?:ngine)?|[kK](?:it)?)?(?:[vV](?:iew)?)?|"
          "(?:[qQ](?:uick)?)?[nN](?:ative)?[wW](?:ev)?(?:[vV](?:iew)?)?|"
          "[eE](?:dge)?(?:[vV](?:iew)?)?|"
          "[eE](?:dge)?[wW](?:eb)?(?:[vV](?:iew)?)?|"
          "[lL](?:ocal)?(?:[vV](?:iew)?)?|"
          "[tT](?:rident)?(?:[vV](?:iew)?)?" },
        { "histnode",  "[lL](?:oad)?[hH](?:ack)?|[hH](?:ist)?[nN](?:ode)?" },
        { "draggesture", "[dD](?:rag)?[hH](?:ack)?" },
        { "inspector",         "[iI]nspector" },
        { "dnsprefetch",       "[dD][nN][sS][pP]refetch" },
        { "frameflatten",      "[fF]rame[fF]latten" },
        { "spatialnavigation", "[sS]patial(?:[nN]avigation)?" },
        { "tiledbackingstore", "[tT]iled[bB]acking[sS]tore" },
        { "zoomtextonly",      "[zZ]oom[tT]ext[oO]nly" },
        { "caretbrowse",       "[cC]aret[bB]rowse" },
        { "scrollanimator",    "[sS]croll[aA]nimator" },
        { "webaudio",          "[wW]eb[aA]udio" },
        { "webgl",             "[wW]eb[gG][lL]" },
    };

    bool WordAnswered(const QString &subject){
        return subject == QStringLiteral("viewkind")  ||
               subject == QStringLiteral("encoding")  ||
               subject == QStringLiteral("useragent") ||
               subject == QStringLiteral("proxy")     ||
               subject == QStringLiteral("ssl");
    }

    bool SaysTheSame(const QString &a, const QString &b, const QString &subject){
        if(a.startsWith(QLatin1Char('!')) != b.startsWith(QLatin1Char('!')))
            return false;
        if(!WordAnswered(subject)) return true;
        return QString::compare(a, b, Qt::CaseInsensitive) == 0;
    }

    QMap<QString, QString> WordsInForce(const QStringList &tokens){
        QMap<QString, QString> words;
        foreach(const QString &token, tokens){
            const QString subject = DirectoryPage::TokenSubject(token);
            if(subject.isEmpty()) continue;
            if(!words.contains(subject) ||
               (token.startsWith(QLatin1Char('!')) &&
                !words[subject].startsWith(QLatin1Char('!'))))
                words[subject] = token;
        }
        return words;
    }

    int AsControl(int state, const DirectoryPage::Token &token){
        if(!token.inverted || state == -1) return state;
        return 1 - state;
    }

    QList<DirectoryPage::Token> BuildTokens(){
        QList<DirectoryPage::Token> list;
        const int count = sizeof(TOKENS) / sizeof(TOKENS[0]);
        list.reserve(count);
        for(int i = 0; i < count; i++) list << TOKENS[i];
        return list;
    }

    const DirectoryPage::Token &IdToken(){
        return DirectoryPage::Tokens().first();
    }

    bool SpellsId(const QString &word){
        return QRegularExpression(QStringLiteral("\\A(?:%1)\\Z")
                                  .arg(QLatin1String(IdToken().pattern)))
            .match(word).hasMatch();
    }

    QString IdOf(const Node *nd){
        return nd ? QString::number(nd->GetSerial(), 16) : QString();
    }

    ViewNode *FindBelow(Node *nd, const QString &id, bool directoriesOnly){
        foreach(Node *child, nd->GetChildren()){
            if(!child->ToViewNode()) continue;
            if((!directoriesOnly || child->IsDirectory()) && IdOf(child) == id)
                return child->ToViewNode();
            if(ViewNode *found = FindBelow(child, id, directoriesOnly))
                return found;
        }
        return nullptr;
    }

    ViewNode *Find(const QString &id, bool directoriesOnly){
        if(id.isEmpty()) return nullptr;
        foreach(ViewNode *root, QList<ViewNode*>()
                    << TreeBank::GetViewRoot() << TreeBank::GetTrashRoot()){
            if(!root) continue;
            if(IdOf(root) == id) return root;
            if(ViewNode *found = FindBelow(root, id, directoriesOnly))
                return found;
        }
        return nullptr;
    }

    QString ProfileOf(ViewNode *vn){
        forever{
            if(!vn) return QString();
            const QString title = vn->GetTitle();
            if(vn == TreeBank::GetViewRoot() || vn == TreeBank::GetTrashRoot() ||
               (vn->IsDirectory() && DirectoryPage::SaysId(title))){

                return DirectoryPage::TitleName(title);
            }
            Node *parent = vn->GetParent();
            if(!parent) return DirectoryPage::TitleName(title);
            vn = parent->ToViewNode();
        }
    }

    QList<ViewNode*> ChainOf(ViewNode *subject){
        QList<ViewNode*> chain;
        for(Node *nd = subject; nd; nd = nd->GetParent()){
            if(nd->ToViewNode() && nd->IsDirectory())
                chain.prepend(nd->ToViewNode());
        }
        return chain;
    }

    QStringList SettingsAbove(ViewNode *vn){
        QList<ViewNode*> chain = ChainOf(vn);
        if(!chain.isEmpty()) chain.removeLast();
        QStringList titles;
        for(int i = chain.length() - 1; i >= 0; i--) titles << chain[i]->GetTitle();
        return DirectoryPage::InheritTokens(titles);
    }

    int InheritedState(ViewNode *vn, const DirectoryPage::Token &token){
        if(token.ownOnly) return -1;
        return AsControl(DirectoryPage::StateIn(SettingsAbove(vn),
                                                QLatin1String(token.pattern)),
                         token);
    }

    QJsonObject InheritedStates(ViewNode *vn){
        QJsonObject states;
        foreach(const DirectoryPage::Token &token, DirectoryPage::Tokens())
            states[QLatin1String(token.key)] = InheritedState(vn, token);
        return states;
    }

    QJsonObject DescribeEntry(ViewNode *vn){
        QJsonObject object;
        object[QStringLiteral("id")]      = IdOf(vn);
        object[QStringLiteral("name")]    = DirectoryPage::TitleName(vn->GetTitle());
        object[QStringLiteral("tokens")]  =
            QJsonArray::fromStringList(DirectoryPage::TitleTokens(vn->GetTitle()));
        object[QStringLiteral("profile")] = ProfileOf(vn);
        object[QStringLiteral("inherited")] = InheritedStates(vn);
        if(vn == TreeBank::GetTrashRoot()) object[QStringLiteral("root")] = true;
        return object;
    }

    QByteArray Compact(const QJsonObject &object){
        return QJsonDocument(object).toJson(QJsonDocument::Compact);
    }

    int LetDescendantsFollow(ViewNode *vn, const QStringList &changed){
        if(changed.isEmpty()) return 0;
        int followed = 0;
        foreach(Node *child, vn->GetChildren()){
            ViewNode *below = child->ToViewNode();
            if(!below) continue;
            if(below->IsDirectory()){
                const QString before = below->GetTitle();
                const QString after = DirectoryPage::TitleFollowing(before, changed);
                if(before != after){
                    below->SetTitle(after);
                    TreeBank::ReconfigureDirectory(below, before, after);
                    followed++;
                }
            }
            followed += LetDescendantsFollow(below, changed);
        }
        return followed;
    }

    QByteArray Error(const QString &message){
        QJsonObject object;
        object[QStringLiteral("error")] = message;
        return Compact(object);
    }

    QJsonObject PageStrings(){
        QJsonObject strings;
        strings[QStringLiteral("title")] =
            QCoreApplication::translate("DirectoryPage", "Directory settings");
        strings[QStringLiteral("inheritance")] =
            QCoreApplication::translate("DirectoryPage",
                "Changing a directory's settings applies them recursively to "
                "everything under it.");
        strings[QStringLiteral("followed")] =
            QCoreApplication::translate("DirectoryPage",
                "Saved, and %1 directories below were changed to follow it");
        strings[QStringLiteral("ancestorsShow")] =
            QCoreApplication::translate("DirectoryPage",
                "Show directories above (%1)");
        strings[QStringLiteral("ancestorsHide")] =
            QCoreApplication::translate("DirectoryPage",
                "Hide directories above (%1)");
        strings[QStringLiteral("currentDirectory")] =
            QCoreApplication::translate("DirectoryPage", "This directory");
        strings[QStringLiteral("name")] =
            QCoreApplication::translate("DirectoryPage", "Name");
        strings[QStringLiteral("profile")] =
            QCoreApplication::translate("DirectoryPage", "Profile in force:");
        strings[QStringLiteral("stateDefault")] =
            QCoreApplication::translate("DirectoryPage", "Default (%1)");
        strings[QStringLiteral("stateOn")] =
            QCoreApplication::translate("DirectoryPage", "On");
        strings[QStringLiteral("stateOff")] =
            QCoreApplication::translate("DirectoryPage", "Off");
        strings[QStringLiteral("otherTokens")] =
            QCoreApplication::translate("DirectoryPage", "Other tokens");
        strings[QStringLiteral("otherTokensHint")] =
#ifdef EDGEWEBVIEW
            QCoreApplication::translate("DirectoryPage",
                "Separated by ';'. Everything without a control here:\n"
                "\n"
                "UserAgent <browser> -- claim to be another browser.\n"
                "Encoding <name> -- the fallback text encoding.\n"
                "Proxy <address> -- a proxy for this directory alone.\n"
                "Ssl <protocol> -- which SSL protocol to offer: TLSv1.2, Any or Secure.\n"
                "WebEngine / QuickWebEngine / NativeWeb / Edge / Local -- "
                "which kind of view a new node here opens with.\n"
                "\n"
                "A name can be turned off by writing it with a '!' in front.");
#else
            QCoreApplication::translate("DirectoryPage",
                "Separated by ';'. Everything without a control here:\n"
                "\n"
                "UserAgent <browser> -- claim to be another browser.\n"
                "Encoding <name> -- the fallback text encoding.\n"
                "Proxy <address> -- a proxy for this directory alone.\n"
                "Ssl <protocol> -- which SSL protocol to offer: TLSv1.2, Any or Secure.\n"
                "WebEngine / QuickWebEngine / NativeWeb / Local -- which kind "
                "of view a new node here opens with.\n"
                "\n"
                "A name can be turned off by writing it with a '!' in front.");
#endif
        strings[QStringLiteral("userAgentNames")] =
            QCoreApplication::translate("DirectoryPage",
                "Browsers to name, or a whole user agent written out with its "
                "spaces percent encoded:");
        strings[QStringLiteral("userAgentWarning")] =
            QCoreApplication::translate("DirectoryPage",
                "Claiming another browser can cost more than it buys. Google "
                "refuses to sign in to a browser which says it is Chrome "
                "without being one -- the headers give it away, and the "
                "refusal is worded as if the browser were unsafe -- while it "
                "lets this engine's own user agent through. Use it for the "
                "sites that ask for a name, not everywhere.");
        strings[QStringLiteral("rootLocked")] =
            QCoreApplication::translate("DirectoryPage",
                "The trash's title is fixed at every launch, so there is "
                "nothing to edit here.");
        strings[QStringLiteral("saved")] =
            QCoreApplication::translate("DirectoryPage", "Saved");
        strings[QStringLiteral("failed")] =
            QCoreApplication::translate("DirectoryPage", "Not saved");
        strings[QStringLiteral("noDirectories")] =
            QCoreApplication::translate("DirectoryPage",
                "There is no directory above this node.");
        strings[QStringLiteral("loadFailed")] =
            QCoreApplication::translate("DirectoryPage",
                "Could not load the directories: ");
        return strings;
    }

    int DefaultState(const DirectoryPage::Token &token){
        if(QLatin1String(token.key) == QLatin1String("rightgesture"))
            return View::EnableRightGesture() ? 1 : 0;
        const QLatin1String key(token.key);
        if(key == QLatin1String("draggesture"))
            return View::EnableDragGesture() ? 1 : 0;
        if(key == QLatin1String("id") || key == QLatin1String("private"))
            return 0;
#ifdef WEBENGINEVIEW
        const auto *settings = QWebEngineProfile::defaultProfile()->settings();
        if(key == QLatin1String("image"))
            return settings->testAttribute(QWebEngineSettings::AutoLoadImages) ? 1 : 0;
        if(key == QLatin1String("javascript"))
            return settings->testAttribute(QWebEngineSettings::JavascriptEnabled) ? 1 : 0;
        if(key == QLatin1String("plugins"))
            return settings->testAttribute(QWebEngineSettings::PluginsEnabled) ? 1 : 0;
#else
        const auto &settings = Application::GlobalSettings();
        if(key == QLatin1String("image"))
            return settings.value(QStringLiteral("webview/preferences/AutoLoadImages"), true).toBool() ? 1 : 0;
        if(key == QLatin1String("javascript"))
            return settings.value(QStringLiteral("webview/preferences/JavascriptEnabled"), true).toBool() ? 1 : 0;
        if(key == QLatin1String("plugins"))
            return settings.value(QStringLiteral("webview/preferences/PluginsEnabled"), true).toBool() ? 1 : 0;
#endif
        return token.absence;
    }
}

namespace DirectoryPage {

const QList<Token> &Tokens(){
    static const QList<Token> tokens = BuildTokens();
    return tokens;
}

QString TitleName(const QString &title){
    return title.split(QStringLiteral(";")).first();
}

QString TokenSubject(const QString &token){
    QString word = token;
    if(word.startsWith(QStringLiteral("!"))) word = word.mid(1);
    if(word.isEmpty()) return QString();

    foreach(const Token &t, Tokens()){
        if(QRegularExpression(QStringLiteral("\\A%1\\Z")
                              .arg(QLatin1String(t.pattern)))
           .match(word).hasMatch())
            return QLatin1String(t.key);
    }
    const int count = sizeof(FAMILIES) / sizeof(FAMILIES[0]);
    for(int i = 0; i < count; i++){
        if(QRegularExpression(QStringLiteral("\\A(?:%1)\\Z")
                              .arg(QLatin1String(FAMILIES[i].pattern)))
           .match(word).hasMatch())
            return QLatin1String(FAMILIES[i].subject);
    }

    return word.section(QStringLiteral(" "), 0, 0).toLower();
}

QStringList InheritTokens(const QStringList &titlesNearestFirst){
    QStringList kept;
    QSet<QString> settled;
    foreach(const QString &title, titlesNearestFirst){
        QSet<QString> here;
        foreach(const QString &token, TitleTokens(title)){
            const QString subject = TokenSubject(token);
            if(settled.contains(subject)) continue;
            kept << token;
            here << subject;
        }
        settled.unite(here);
    }
    return kept;
}

QStringList TitleTokens(const QString &title){
    QStringList tokens = title.split(QStringLiteral(";")).mid(1);
    tokens.removeAll(QString());
    return tokens;
}

QString ComposeTitle(const QString &name, const QStringList &tokens){
    if(tokens.isEmpty()) return name;
    return name + QStringLiteral(";") + tokens.join(QStringLiteral(";"));
}

int StateIn(const QStringList &set, const QString &pattern){
    const QRegularExpression on (QStringLiteral("\\A(?:%1)\\Z").arg(pattern));
    const QRegularExpression off(QStringLiteral("\\A!(?:%1)\\Z").arg(pattern));
    foreach(const QString &token, set) if(off.match(token).hasMatch()) return 0;
    foreach(const QString &token, set) if(on .match(token).hasMatch()) return 1;
    return -1;
}

bool SaysPrivate(const QStringList &set){
    return StateIn(set, QStringLiteral("(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)")) == 1;
}

QStringList ChangedWords(const QString &before, const QString &after){
    const QMap<QString, QString> was = WordsInForce(TitleTokens(before));
    const QMap<QString, QString> now = WordsInForce(TitleTokens(after));
    QStringList changed;
    for(QMap<QString, QString>::const_iterator it = now.constBegin();
        it != now.constEnd(); ++it){
        if(!was.contains(it.key()) ||
           !SaysTheSame(was[it.key()], it.value(), it.key()))
            changed << it.value();
    }
    return changed;
}

QString TitleFollowing(const QString &title, const QStringList &changed){
    const QMap<QString, QString> above = WordsInForce(changed);
    QStringList kept;
    foreach(const QString &token, TitleTokens(title)){
        const QString subject = TokenSubject(token);
        if(above.contains(subject) &&
           !SaysTheSame(token, above[subject], subject)) continue;
        kept << token;
    }
    return ComposeTitle(TitleName(title), kept);
}

int TokenState(const QString &title, const Token &token){
    return AsControl(StateIn(TitleTokens(title), QLatin1String(token.pattern)),
                     token);
}

bool TokenBlocked(const Token &token){
    return QLatin1String(token.key) == QLatin1String("autoload") &&
           !View::HiddenViewsStayActive();
}

QString WithTokenState(const QString &title, const Token &token, int state){
    const QRegularExpression any
        (QStringLiteral("\\A!?%1\\Z").arg(QLatin1String(token.pattern)));
    QStringList kept;
    foreach(const QString &t, TitleTokens(title))
        if(!any.match(t).hasMatch()) kept << t;
    if(state == 1)
        kept << QLatin1String(token.on);
    else if(state == 0 && token.off[0])
        kept << QLatin1String(token.off);
    return ComposeTitle(TitleName(title), kept);
}

bool SaysId(const QString &title){
    foreach(const QString &token, TitleTokens(title))
        if(SpellsId(token)) return true;
    return false;
}

bool NameSpellsId(const QString &name){
    return SpellsId(name);
}

bool IsValidTitle(const QString &title){
    if(title.contains(QRegularExpression(QStringLiteral("[<>\":\\?\\|\\*/\\\\]"))))
        return false;
    return !TitleName(title).isEmpty() || !SaysId(title);
}

QUrl PageUrl(){
    return QUrl(VANILLA_SCHEME + QStringLiteral("://directory/"));
}

bool IsPageUrl(const QUrl &url){
    return url.scheme() == VANILLA_SCHEME &&
           url.host()   == QStringLiteral("directory");
}

QJsonObject Describe(){
    QJsonArray switches;
    foreach(const Token &token, Tokens()){
        QJsonObject object;
        object[QStringLiteral("key")]     = QLatin1String(token.key);
        object[QStringLiteral("label")]   =
            QCoreApplication::translate("DirectoryPage", token.label);
        object[QStringLiteral("hint")]    =
            QCoreApplication::translate("DirectoryPage", token.hint);
        object[QStringLiteral("pattern")] = QLatin1String(token.pattern);
        object[QStringLiteral("on")]      = QLatin1String(token.on);
        if(token.off[0]) object[QStringLiteral("off")] = QLatin1String(token.off);
        object[QStringLiteral("absence")] = DefaultState(token);
        if(token.inverted) object[QStringLiteral("inverted")] = true;
        if(TokenBlocked(token)) object[QStringLiteral("blocked")] = true;
        switches.append(object);
    }

    QJsonArray agents;
    foreach(const UserAgent::Entry &entry, UserAgent::Entries())
        agents.append(QLatin1String(entry.name));

    QJsonObject root;
    root[QStringLiteral("switches")]  = switches;
    root[QStringLiteral("useragents")] = agents;
    root[QStringLiteral("strings")]   = PageStrings();

    MainWindow *win = Application::GetCurrentWindow();
    TreeBank *tb = win ? win->GetTreeBank() : nullptr;
    ViewNode *subject = tb ? tb->GetCurrentViewNode() : nullptr;
    if(!subject) subject = TreeBank::GetViewRoot();

    QJsonArray chain;
    foreach(ViewNode *vn, ChainOf(subject))
        chain.append(DescribeEntry(vn));
    root[QStringLiteral("chain")] = chain;
    return root;
}

QByteArray HandleSet(const QJsonObject &request){
    const QString id = request[QStringLiteral("node")].toString();
    ViewNode *vn = Find(id, true);
    if(!vn) return Error(QStringLiteral("unknown node: \"%1\"").arg(id));
    if(vn == TreeBank::GetTrashRoot())
        return Error(QStringLiteral("the trash is not editable"));

    if(!request[QStringLiteral("name")].isString() ||
       !request[QStringLiteral("tokens")].isArray())
        return Error(QStringLiteral("bad value"));

    const QString name = request[QStringLiteral("name")].toString().trimmed();
    if(name.contains(QLatin1Char(';'))) return Error(QStringLiteral("bad name"));

    QStringList tokens;
    foreach(const QJsonValue &value, request[QStringLiteral("tokens")].toArray()){
        if(!value.isString()) return Error(QStringLiteral("bad value"));
        const QString token = value.toString().trimmed();
        if(token.isEmpty()) continue;
        if(token.contains(QLatin1Char(';'))) return Error(QStringLiteral("bad token"));
        tokens << token;
    }

    const QString after = ComposeTitle(name, tokens);
    if(!IsValidTitle(after)) return Error(QStringLiteral("bad title"));
    if(name.isEmpty() && vn == TreeBank::GetViewRoot())
        return Error(QStringLiteral("bad title"));
    if(name.isEmpty() && NameSpellsId(TitleName(vn->GetTitle())))
        return Error(QStringLiteral("bad title"));

    const QString before = vn->GetTitle();
    int followed = 0;
    if(before != after){
        vn->SetTitle(after);
        TreeBank::ReconfigureDirectory(vn, before, after);
        followed = LetDescendantsFollow(vn, ChangedWords(before, after));
        TreeBank::EmitTreeStructureChanged();
    }
    QJsonObject reply;
    reply[QStringLiteral("ok")] = true;
    reply[QStringLiteral("followed")] = followed;
    return Compact(reply);
}

QMenu *CreateSettingsMenu(ViewNode *vn, std::function<void()> openPage,
                          QWidget *parent){
    QMenu *menu = new QMenu(QCoreApplication::translate("DirectoryPage",
                                                        "DirectorySettings"),
                            parent);

    QWidgetAction *boxes = new QWidgetAction(menu);
    QWidget *widget = new QWidget(menu);
    QGridLayout *layout = new QGridLayout();

    const QString id = IdOf(vn);

    const QList<Token> &tokens = Tokens();
    for(int i = 0; i < tokens.length(); i++){
        QCheckBox *check = new QCheckBox();
        const int own = TokenState(vn->GetTitle(), tokens[i]);
        const int inherited = InheritedState(vn, tokens[i]);
        const int state = own != -1 ? own
                        : inherited != -1 ? inherited
                        : (DefaultState(tokens[i]) == 1 ? 1 : 0);
        check->setChecked(state == 1);
        if(!tokens[i].off[0] && own == -1 && inherited == 1)
            check->setEnabled(false);
        if(TokenBlocked(tokens[i])) check->setEnabled(false);
        if(QLatin1String(tokens[i].key) == QLatin1String("id") &&
           own != 1 && TitleName(vn->GetTitle()).isEmpty())
            check->setEnabled(false);
        QLabel *label = new QLabel(QCoreApplication::translate
                                   ("DirectoryPage", tokens[i].label));
        const QString hint = QCoreApplication::translate
            ("DirectoryPage", tokens[i].hint);
        check->setToolTip(hint);
        label->setToolTip(hint);
        QObject::connect
            (check, &QCheckBox::checkStateChanged, check,
             [id, i](Qt::CheckState state){
                 ViewNode *nd = Find(id, true);
                 if(!nd || nd == TreeBank::GetTrashRoot()) return;
                 const QString before = nd->GetTitle();
                 const QString after =
                     WithTokenState(before, Tokens()[i],
                                    state == Qt::Checked ? 1 : 0);
                 if(before == after) return;
                 if(!IsValidTitle(after)) return;
                 nd->SetTitle(after);
                 TreeBank::ReconfigureDirectory(nd, before, after);
                 LetDescendantsFollow(nd, ChangedWords(before, after));
                 TreeBank::EmitTreeStructureChanged();
             });
        layout->addWidget(check, i, 0);
        layout->addWidget(label, i, 1);
    }

    widget->setLayout(layout);
    boxes->setDefaultWidget(widget);
    menu->addAction(boxes);

    if(openPage){
        menu->addSeparator();
        QAction *open = new QAction(menu);
        open->setText(QCoreApplication::translate("DirectoryPage",
                                                  "OpenDirectorySettingsPage"));
        QObject::connect(open, &QAction::triggered, open,
                         [openPage](){ openPage(); });
        menu->addAction(open);
    }

    return menu;
}

}
