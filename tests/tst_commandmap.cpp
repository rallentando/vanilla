#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QStringList>
#include <QSet>
#include <QMap>
#include <QMetaMethod>
#include <QPair>
#include <QDir>
#include <QFile>
#include <QRegularExpression>

#include "commandmap.hpp"
#include "actionmapper.hpp"
#include "jsobject.hpp"

#include "testsupport.hpp"

static QStringList Expand(const QString &spelling){
    QStringList out;
    out << QString();

    int i = 0;
    while(i < spelling.length()){
        QStringList tails;

        if(spelling[i] == QLatin1Char('[')){
            const int close = spelling.indexOf(QLatin1Char(']'), i);
            for(int j = i + 1; j < close; j++)
                tails << spelling.mid(j, 1);
            i = close + 1;

        } else if(spelling.mid(i, 3) == QStringLiteral("(?:")){
            int depth = 0, close = i;
            for(int j = i; j < spelling.length(); j++){
                if(spelling[j] == QLatin1Char('(')) depth++;
                if(spelling[j] == QLatin1Char(')')){
                    depth--;
                    if(!depth){ close = j; break;}
                }
            }
            QStringList alternatives;
            QString current;
            depth = 0;
            for(const QChar &c : spelling.mid(i + 3, close - i - 3)){
                if(c == QLatin1Char('(')) depth++;
                if(c == QLatin1Char(')')) depth--;
                if(c == QLatin1Char('|') && !depth){
                    alternatives << current;
                    current.clear();
                } else current += c;
            }
            alternatives << current;
            foreach(QString alternative, alternatives)
                tails << Expand(alternative);
            i = close + 1;

        } else {
            tails << spelling.mid(i, 1);
            i++;
        }

        if(i < spelling.length() && spelling[i] == QLatin1Char('?')){
            tails << QString();
            i++;
        }

        QStringList next;
        foreach(QString head, out)
            foreach(QString tail, tails)
                next << head + tail;
        out = next;
    }
    return out;
}

static QStringList Sorted(QStringList list){
    std::sort(list.begin(), list.end());
    return list;
}

static QStringList OwnSlotNames(const QMetaObject &metaObject){
    QSet<QString> names;
    for(int i = metaObject.methodOffset(); i < metaObject.methodCount(); i++){
        const QMetaMethod method = metaObject.method(i);
        if(method.methodType() == QMetaMethod::Slot)
            names.insert(QString::fromLatin1(method.name()));
    }
    return Sorted(names.values());
}

static QStringList DialogMethodContracts(){
    QStringList contracts;
    for(int i = _Vanilla::staticMetaObject.methodOffset();
        i < _Vanilla::staticMetaObject.methodCount(); i++){
        const QMetaMethod method = _Vanilla::staticMetaObject.method(i);
        const QByteArray name = method.name();
        if(name == "getInt" || name == "getDouble" ||
           name == "getItem" || name == "getText"){
            contracts << QString::fromLatin1(method.methodSignature()) +
                QStringLiteral("->") + QString::fromLatin1(method.typeName());
        }
    }
    return Sorted(contracts);
}

class ActionRecordingView : public QObject, public View {

public:
    ActionRecordingView() : QObject(nullptr), View(nullptr) {}

    QObject *base() Q_DECL_OVERRIDE { return this;}
    QSize size() Q_DECL_OVERRIDE { return QSize();}
    void resize(QSize) Q_DECL_OVERRIDE {}
    void show() Q_DECL_OVERRIDE {}
    void hide() Q_DECL_OVERRIDE {}
    void raise() Q_DECL_OVERRIDE {}
    void lower() Q_DECL_OVERRIDE {}
    void repaint() Q_DECL_OVERRIDE {}
    bool visible() Q_DECL_OVERRIDE { return false;}
    void setFocus(Qt::FocusReason = Qt::OtherFocusReason) Q_DECL_OVERRIDE {}

    void TriggerAction(Page::CustomAction action, QVariant) Q_DECL_OVERRIDE {
        called = action;
    }

    Page::CustomAction called = Page::_NoAction;
};

class tst_commandmap : public QObject {
    Q_OBJECT

private slots:

    void expandEnumeratesASpelling(){
        QCOMPARE(Sorted(Expand(QStringLiteral("[bB]ack(?:ward)?"))),
                 Sorted(QStringList()
                        << QStringLiteral("back") << QStringLiteral("backward")
                        << QStringLiteral("Back") << QStringLiteral("Backward")));

        QCOMPARE(Sorted(Expand(QStringLiteral("[pP](?:g|age)?[uU]p"))),
                 Sorted(QStringList()
                        << QStringLiteral("pgup") << QStringLiteral("pageup") << QStringLiteral("pup")
                        << QStringLiteral("pgUp") << QStringLiteral("pageUp") << QStringLiteral("pUp")
                        << QStringLiteral("Pgup") << QStringLiteral("Pageup") << QStringLiteral("Pup")
                        << QStringLiteral("PgUp") << QStringLiteral("PageUp") << QStringLiteral("PUp")));

        QCOMPARE(Sorted(Expand(QStringLiteral("[sS]ettings?"))),
                 Sorted(QStringList()
                        << QStringLiteral("settings") << QStringLiteral("setting")
                        << QStringLiteral("Settings") << QStringLiteral("Setting")));

        QCOMPARE(Expand(QStringLiteral("quit")), QStringList() << QStringLiteral("quit"));
    }

    void theRowsWhichCannotBeReached(){
        QString action;

        QCOMPARE(CommandMap::Resolve(QStringLiteral("newnode"), &action), CommandMap::Signal);
        QCOMPARE(action, QStringLiteral("NewViewNode"));

        QCOMPARE(CommandMap::Resolve(QStringLiteral("clonenode"), &action), CommandMap::Signal);
        QCOMPARE(action, QStringLiteral("CloneViewNode"));

        QCOMPARE(CommandMap::Resolve(QStringLiteral("settings"), &action), CommandMap::Signal);
        QCOMPARE(action, QStringLiteral("OpenSettings"));
        QCOMPARE(CommandMap::Resolve(QStringLiteral("setting"), &action), CommandMap::Signal);
        QCOMPARE(action, QStringLiteral("OpenSettings"));

        QCOMPARE(CommandMap::Resolve(QStringLiteral("set"), &action), CommandMap::Set);
    }

    void everySpellingReachesItsOwnAction(){
        QMap<QString, QString> shadowed;
        shadowed[QStringLiteral("NewNode")]   = QStringLiteral("NewViewNode");
        shadowed[QStringLiteral("CloneNode")] = QStringLiteral("CloneViewNode");
        shadowed[QStringLiteral("Set")]       = QStringLiteral("OpenSettings");

        int words = 0;
        foreach(CommandMap::Entry entry, CommandMap::Entries()){
            const QString expected = QString::fromLatin1(entry.action);

            foreach(QString word, Expand(QString::fromLatin1(entry.spelling))){
                QString action;
                const CommandMap::Kind kind = CommandMap::Resolve(word, &action);
                words++;

                if(action != expected && shadowed.contains(expected) &&
                   action == shadowed[expected]) continue;

                QVERIFY2(action == expected,
                         qPrintable(QStringLiteral("'%1' is '%2', not '%3'")
                                    .arg(word).arg(action).arg(expected)));
                QCOMPARE(kind, entry.kind);
            }
        }
        QVERIFY2(words > 6000, qPrintable(QString::number(words)));
    }

    void noSpellingIsWrittenTwice(){
        QSet<QString> seen;
        foreach(CommandMap::Entry entry, CommandMap::Entries()){
            const QString spelling = QString::fromLatin1(entry.spelling);
            QVERIFY2(!seen.contains(spelling), qPrintable(spelling));
            seen << spelling;
        }
    }

    void everyActionNameIsKnownToTheSideThatRunsIt(){
#define COLLECT_ACTION(ACTION) << QStringLiteral(#ACTION)
        const QStringList pageActions     = QStringList() PAGE_FOR_EACH_ACTION(COLLECT_ACTION);
        const QStringList gadgetsActions  = QStringList() GADGETS_FOR_EACH_ACTION(COLLECT_ACTION);
        const QStringList treebankActions = QStringList() TREEBANK_FOR_EACH_ACTION(COLLECT_ACTION);

        foreach(QString action, treebankActions)
            QVERIFY2(pageActions.contains(action), qPrintable(action));

        int signals_ = 0, elements = 0;
        foreach(CommandMap::Entry entry, CommandMap::Entries()){
            const QString action = QString::fromLatin1(entry.action);

            if(entry.kind == CommandMap::Signal){
                QVERIFY2(CommandMap::IsSignalAction(action), qPrintable(action));
                QVERIFY2(pageActions.contains(action) ||
                         gadgetsActions.contains(action) ||
                         action == QStringLiteral("Reconfigure"),
                         qPrintable(action));
                signals_++;
            } else if(entry.kind == CommandMap::Element){
                QVERIFY2(pageActions.contains(action), qPrintable(action));
                elements++;
            }
        }
        QCOMPARE(signals_,  168);
        QCOMPARE(elements,   71);
    }

    void everyActionIsAnsweredByTheTableBehindIt(){
        const QStringList pageActions    = QStringList() PAGE_FOR_EACH_ACTION(COLLECT_ACTION);
        const QStringList gadgetsActions = QStringList() GADGETS_FOR_EACH_ACTION(COLLECT_ACTION);
        const QStringList treebankActions = QStringList() TREEBANK_FOR_EACH_ACTION(COLLECT_ACTION);
#undef COLLECT_ACTION

        QList<QPair<QString, QStringList> > tables;
        tables << QPair<QString, QStringList>(QStringLiteral("view/page.cpp"),      pageActions)
               << QPair<QString, QStringList>(QStringLiteral("gadgets/gadgets.cpp"), gadgetsActions)
               << QPair<QString, QStringList>(QStringLiteral("ui/treebank.cpp"),    treebankActions)
               << QPair<QString, QStringList>(QStringLiteral("view/localview.cpp"), gadgetsActions);

        for(int i = 0; i < tables.length(); i++){
            QFile source(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR "/") + tables[i].first));
            QVERIFY2(source.open(QIODevice::ReadOnly), qPrintable(tables[i].first));
            const QString text = QString::fromUtf8(source.readAll());

            foreach(QString action, tables[i].second){
                if(action == QStringLiteral("NoAction")) continue;

                const bool answered =
                    text.contains(QRegularExpression
                                  (QStringLiteral("DEFINE_ACTION\\(\\s*%1\\s*,").arg(action))) ||
                    text.contains(QRegularExpression
                                  (QStringLiteral("case\\s+(?:\\w+::)?_%1\\s*:").arg(action)));

                QVERIFY2(answered,
                         qPrintable(QStringLiteral("%1 knows '%2' and does not answer it")
                                    .arg(tables[i].first).arg(action)));
            }
        }
    }

    void theListingFallbackNamesTheBaseVersionOfTheSameFunction(){
        QFile source(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR "/view/localview.cpp")));
        QVERIFY(source.open(QIODevice::ReadOnly));
        const QString text = QString::fromUtf8(source.readAll());

        QRegularExpression re(QStringLiteral(
            "bool LocalView::ThumbList_(\\w+)\\(\\)\\{\\s*"
            "return SelectMediaItem\\([^;]*"
            "\\[this\\]\\(\\)\\{ GraphicsTableView::ThumbList_(\\w+)\\(\\);\\}\\);"));

        int count = 0;
        QRegularExpressionMatchIterator it = re.globalMatch(text);
        while(it.hasNext()){
            QRegularExpressionMatch match = it.next();
            QCOMPARE(match.captured(2), match.captured(1));
            count++;
        }
        QCOMPARE(count, 10);
    }

    void theEditActionsAreSpelledTheSameOnBothSides(){
#define COLLECT_EDIT_ACTION(ACTION) << QStringLiteral(#ACTION)
        const QStringList actions =
            QStringList() FOR_EACH_EDIT_EVENTS(COLLECT_EDIT_ACTION);
#undef COLLECT_EDIT_ACTION

#define COLLECT_EDIT_PAIR(ACTION, JSNAME) \
        << QPair<QString, QString>(QStringLiteral(#ACTION), QStringLiteral(#JSNAME))
        const QList<QPair<QString, QString> > pairs =
            QList<QPair<QString, QString> >() FOR_EACH_EDIT_EVENTS_WITH_JS_NAME(COLLECT_EDIT_PAIR);
#undef COLLECT_EDIT_PAIR

        QCOMPARE(pairs.length(), actions.length());
        QCOMPARE(actions.length(), 15);

        QFile qml(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR "/view/webengine/quickwebengineview6.qml")));
        QVERIFY2(qml.open(QIODevice::ReadOnly), "check VANILLA_SOURCE_DIR");
        const QString source = QString::fromUtf8(qml.readAll());

        for(int i = 0; i < pairs.length(); i++){
            const QString action = pairs[i].first;
            const QString jsName = pairs[i].second;

            QCOMPARE(action, actions[i]);

            QString small = action;
            small[0] = small[0].toLower();
            QCOMPARE(jsName, small);

            QVERIFY2(source.contains(QStringLiteral("function %1(").arg(jsName)),
                     qPrintable(jsName));
        }
    }

    void theJavaScriptApiKeepsItsPublicNames(){
        const QStringList vanilla = QStringLiteral(
            "aboutQt aboutVanilla alignCenter alignJustified alignLeft alignRight back buryView "
            "changeTextDirectionLTR changeTextDirectionRTL clearCookies clearHttpCache "
            "clearVisitedLinks close copy cut digView displayAccessKey displayViewtree down "
            "eighthView end fastForward fifthView firstView forward fourthView getDouble getInt "
            "getItem getText home indent insertOrderedList insertUnorderedList lastView left load "
            "nextView ninthView openCommand openQueryEditor openTextSeeker openUrlEditor outdent "
            "pageDown pageUp paste pasteAndMatchStyle previousView print quit reconfigure recreate "
            "redo releaseHiddenView reload reloadAndBypassCache repaint restore rewind right save "
            "secondView selectAll seventhView sixthView stop stopAndUnselect tenthView thirdView "
            "toggleBold toggleItalic toggleStrikethrough toggleUnderline undo unselect up upDirectory"
        ).split(QLatin1Char(' '));

        const QStringList view = QStringLiteral(
            "aboutQt aboutVanilla addBookmarklet addSearchEngine alignCenter alignJustified alignLeft "
            "alignRight applySource back buryView changeTextDirectionLTR changeTextDirectionRTL "
            "clearCookies clearHttpCache clearVisitedLinks clickElement cloneViewNode close "
            "closeWindow copy copyImage copyImageHtml copyImageUrl copyLinkHtml copyLinkUrl "
            "copyMediaHtml copyMediaUrl copyPageAsLink copySelectedHtml copyTitle copyUrl cut digView "
            "displayAccessKey displayTrashTree displayViewTree down downloadImage downloadLink "
            "downloadMedia eighthView end fastForward fifthView firstView focusElement forward "
            "fourthView home hoverElement indent insertOrderedList insertUnorderedList inspectElement "
            "lastView left load loadImage loadLink loadMedia newViewNode newWindow nextView nextWindow "
            "ninthView openAllImage openAllUrl openBookmarklet openCommand openImage "
            "openImageInNewDirectory openImageInNewDirectoryBackground openImageInNewDirectoryForeground "
            "openImageInNewDirectoryNewWindow openImageInNewDirectoryThisWindow openImageInNewViewNode "
            "openImageInNewViewNodeBackground openImageInNewViewNodeForeground "
            "openImageInNewViewNodeNewWindow openImageInNewViewNodeThisWindow openImageOnRoot "
            "openImageOnRootBackground openImageOnRootForeground openImageOnRootNewWindow "
            "openImageOnRootThisWindow openImageWithDefault openInNewDirectory "
            "openInNewDirectoryBackground openInNewDirectoryForeground openInNewDirectoryNewWindow "
            "openInNewDirectoryThisWindow openInNewViewNode openInNewViewNodeBackground "
            "openInNewViewNodeForeground openInNewViewNodeNewWindow openInNewViewNodeThisWindow "
            "openLink openLinkWithDefault openMedia openMediaInNewDirectory "
            "openMediaInNewDirectoryBackground openMediaInNewDirectoryForeground "
            "openMediaInNewDirectoryNewWindow openMediaInNewDirectoryThisWindow openMediaInNewViewNode "
            "openMediaInNewViewNodeBackground openMediaInNewViewNodeForeground "
            "openMediaInNewViewNodeNewWindow openMediaInNewViewNodeThisWindow openMediaOnRoot "
            "openMediaOnRootBackground openMediaOnRootForeground openMediaOnRootNewWindow "
            "openMediaOnRootThisWindow openMediaWithDefault openOnRoot openOnRootBackground "
            "openOnRootForeground openOnRootNewWindow openOnRootThisWindow openQueryEditor "
            "openTextAsUrl openTextSeeker openUrlEditor openWithDefault outdent pageDown pageUp paste "
            "pasteAndMatchStyle prevView prevWindow print quit recreate redo releaseHiddenView reload "
            "reloadAndBypassCache restore rewind right save saveAllImage saveAllUrl saveTextAsUrl "
            "searchWith secondView selectAll seventhView shadeWindow sixthView stop stopAndUnselect "
            "switchWindow tenthView thirdView toggleBold toggleFullScreen toggleItalic toggleMaximized "
            "toggleMediaControls toggleMediaLoop toggleMediaMute toggleMediaPlayPause toggleMenuBar "
            "toggleMinimized toggleNotifier toggleReceiver toggleShaded toggleStrikethrough "
            "toggleToolBar toggleTreeBar toggleUnderline undo unselect unshadeWindow up upDirectory "
            "viewSource zoomIn zoomOut"
        ).split(QLatin1Char(' '));

        QCOMPARE(OwnSlotNames(_Vanilla::staticMetaObject), vanilla);
        QCOMPARE(OwnSlotNames(_View::staticMetaObject), view);

        const QStringList dialogContracts = Sorted(QStringLiteral(
            "getInt(QString,QString,int,int,int,int)->int "
            "getInt(QString,QString,int,int,int)->int "
            "getInt(QString,QString,int,int)->int "
            "getInt(QString,QString,int)->int "
            "getInt(QString,QString)->int "
            "getDouble(QString,QString,double,double,double,int)->double "
            "getDouble(QString,QString,double,double,double)->double "
            "getDouble(QString,QString,double,double)->double "
            "getDouble(QString,QString,double)->double "
            "getDouble(QString,QString)->double "
            "getItem(QString,QString,QStringList,bool)->QString "
            "getItem(QString,QString,QStringList)->QString "
            "getText(QString,QString,QString)->QString "
            "getText(QString,QString)->QString"
        ).split(QLatin1Char(' ')));
        QCOMPARE(DialogMethodContracts(), dialogContracts);
    }

    void everyViewJavaScriptMethodDispatchesItsPairedAction(){
        ActionRecordingView view;
        _View *api = view.GetJsObject();

#define CHECK_JS_ACTION(action, jsName)                              \
        {                                                           \
            QString expectedName = QStringLiteral(#action);         \
            expectedName[0] = expectedName[0].toLower();            \
            QCOMPARE(QStringLiteral(#jsName), expectedName);        \
        }                                                           \
        view.called = Page::_NoAction;                              \
        QVERIFY(QMetaObject::invokeMethod(api, #jsName,              \
                                          Qt::DirectConnection));   \
        QCOMPARE(view.called, Page::_##action);
        FOR_EACH_VIEW_JS_ACTION(CHECK_JS_ACTION)
#undef CHECK_JS_ACTION
    }

    void theVanillaJavaScriptPairsKeepTheirLegacySpellings(){
#define COLLECT_VANILLA_PAIR(method, jsName) \
        << QStringLiteral(#method ":" #jsName)
        const QStringList pairs = QStringList()
            FOR_EACH_VANILLA_JS_METHOD(COLLECT_VANILLA_PAIR);
#undef COLLECT_VANILLA_PAIR

        const QStringList expected = QStringLiteral(
            "Repaint:repaint Reconfigure:reconfigure Up:up Down:down Right:right Left:left "
            "PageUp:pageUp PageDown:pageDown Home:home End:end AboutVanilla:aboutVanilla "
            "AboutQt:aboutQt Back:back Forward:forward Rewind:rewind FastForward:fastForward "
            "UpDirectory:upDirectory Restore:restore NextView:nextView PrevView:previousView "
            "BuryView:buryView DigView:digView FirstView:firstView SecondView:secondView "
            "ThirdView:thirdView FourthView:fourthView FifthView:fifthView SixthView:sixthView "
            "SeventhView:seventhView EighthView:eighthView NinthView:ninthView TenthView:tenthView "
            "LastView:lastView DisplayViewTree:displayViewtree DisplayAccessKey:displayAccessKey "
            "OpenTextSeeker:openTextSeeker OpenQueryEditor:openQueryEditor OpenUrlEditor:openUrlEditor "
            "OpenCommand:openCommand ReleaseHiddenView:releaseHiddenView Load:load Copy:copy Cut:cut "
            "Paste:paste Undo:undo Redo:redo SelectAll:selectAll Unselect:unselect Reload:reload "
            "ReloadAndBypassCache:reloadAndBypassCache Stop:stop StopAndUnselect:stopAndUnselect "
            "Print:print Save:save ClearCookies:clearCookies ClearHttpCache:clearHttpCache "
            "ClearVisitedLinks:clearVisitedLinks"
        ).split(QLatin1Char(' '));
        QCOMPARE(pairs, expected);
    }

    void thecommandsWhichTakeArguments(){
        QCOMPARE(CommandMap::Resolve(QStringLiteral("openwith")), CommandMap::OpenWith);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("openon")),   CommandMap::OpenWith);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("opennodewith")), CommandMap::OpenNodeWith);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("blank")),    CommandMap::Blank);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("open")),     CommandMap::Open);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("load")),     CommandMap::Load);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("query")),    CommandMap::Query);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("download")), CommandMap::Download);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("search")),   CommandMap::Seek);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("seek")),     CommandMap::Seek);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("unset")),    CommandMap::Unset);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("key")),      CommandMap::Key);

        QVERIFY( CommandMap::BeforeBookmarklet(CommandMap::Blank));
        QVERIFY( CommandMap::BeforeBookmarklet(CommandMap::OpenWith));
        QVERIFY(!CommandMap::BeforeBookmarklet(CommandMap::Open));
        QVERIFY(!CommandMap::BeforeBookmarklet(CommandMap::NotACommand));
    }

    void aspellingIsMatchedWhole(){
        QCOMPARE(CommandMap::Resolve(QStringLiteral("searchengine")), CommandMap::NotACommand);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("kayakseek")),    CommandMap::NotACommand);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("search")),       CommandMap::Seek);
        QCOMPARE(CommandMap::Resolve(QStringLiteral("seek")),         CommandMap::Seek);
    }

    void prevpageIsSpelledLikeNextpage(){
        QString action;
        QCOMPARE(CommandMap::Resolve(QStringLiteral("prevpage"), &action), CommandMap::Signal);
        QCOMPARE(action, QStringLiteral("PrevPage"));
        QCOMPARE(CommandMap::Resolve(QStringLiteral("previouspage"), &action), CommandMap::Signal);
        QCOMPARE(action, QStringLiteral("PrevPage"));
        QCOMPARE(CommandMap::Resolve(QStringLiteral("nextpage"), &action), CommandMap::Signal);
        QCOMPARE(action, QStringLiteral("NextPage"));

        QCOMPARE(CommandMap::Resolve(QStringLiteral("prev"), &action), CommandMap::Signal);
        QCOMPARE(action, QStringLiteral("PrevView"));
    }

    void awordWhichIsNotACommand(){
        QString action = QStringLiteral("something");
        QCOMPARE(CommandMap::Resolve(QStringLiteral("example.com"), &action), CommandMap::NotACommand);
        QVERIFY(action.isEmpty());
        QCOMPARE(CommandMap::Resolve(QString()), CommandMap::NotACommand);
    }
};

QTEST_MAIN(tst_commandmap)
#include "tst_commandmap.moc"
