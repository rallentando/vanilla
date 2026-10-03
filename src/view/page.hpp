#ifndef PAGE_HPP
#define PAGE_HPP

#include "switch.hpp"
#include "const.hpp"

#include <QObject>

#include <functional>

#include "actionmapper.hpp"
#include "callback.hpp"
#include "webelement.hpp"

#undef LoadImage

class QUrl;
class QString;
class QNetworkRequest;
class QMenu;
class QAction;

class View;
class ViewNode;
class TreeBank;
class MainWindow;
class NetworkAccessManager;

typedef QStringList Bookmarklet;

typedef QStringList SearchEngine;

class Page : public QObject {
    Q_OBJECT
    INSTALL_ACTION_MAP(PAGE, CustomAction)

public:
    Page(QObject *parent = 0, NetworkAccessManager *nam = 0);
    ~Page();

    void SetView(View *view){ m_View = view;}
    View *GetView(){ return m_View;}

    NetworkAccessManager *GetNetworkAccessManager();

    enum OpenCommandOperation {
        InNewViewNode,
        InNewDirectory,
        OnRoot,

        InNewViewNodeBackground,
        InNewDirectoryBackground,
        OnRootBackground,

        InNewViewNodeNewWindow,
        InNewDirectoryNewWindow,
        OnRootNewWindow,
    };

    enum FindElementsOption {
        ForAccessKey,
        HaveSource,
        HaveReference,
        RelIsNext,
        RelIsPrev,
    };

    enum MediaType {
        MediaTypeNone,
        MediaTypeImage,
        MediaTypePlayable,
    };

    static QUrl        CreateQueryUrl(QString, QString key = QString());
    static QUrl        CreateQueryUrl(const SearchEngine &engine, QString query);
    static QUrl        UpDirectoryUrl(QUrl);
    static QUrl        StringToUrl   (QString str,  QUrl baseUrl = QUrl());
    static QList<QUrl> ExtractUrlsFromText(QString text, QUrl baseUrl = QUrl());
    static QList<QUrl> ExtractUrlsFromHtml(QString html, QUrl baseUrl, FindElementsOption option);
    static QList<QUrl> DirtyStringToUrls(QString str);
    static QList<QUrl> MimeDataToUrls(const QMimeData *mime, QObject *source);

    static QString OptionToSelector(FindElementsOption option){
        QString selector;
        switch(option){
        case ForAccessKey:  selector = FOR_ACCESSKEY_CSS_SELECTOR;  break;
        case HaveSource:    selector = HAVE_SOURCE_CSS_SELECTOR;    break;
        case HaveReference: selector = HAVE_REFERENCE_CSS_SELECTOR; break;
        case RelIsNext:     selector = REL_IS_NEXT_CSS_SELECTOR;    break;
        case RelIsPrev:     selector = REL_IS_PREV_CSS_SELECTOR;    break;
        }
        return selector;
    }

    static void SetOpenCommandOperation(OpenCommandOperation operation){
        m_OpenCommandOperation = operation;
    }
    static OpenCommandOperation GetOpenOparation(){
        return m_OpenCommandOperation;
    }

    static void RegisterBookmarklet(QString, Bookmarklet);
    static void RemoveBookmarklet(QString);
    static void ClearBookmarklet();
    static QMap<QString, Bookmarklet> GetBookmarkletMap();
    static Bookmarklet GetBookmarklet(QString);

    static void RegisterDefaultSearchEngines();
    static void RegisterSearchEngine(QString, SearchEngine);
    static void RemoveSearchEngine(QString);
    static void ClearSearchEngine();
    static QMap<QString, SearchEngine> GetSearchEngineMap();
    static SearchEngine GetSearchEngine(QString);
    static SearchEngine PrimarySearchEngine();

    static bool ShiftMod();
    static bool CtrlMod();
    static bool Activate();

public slots:
    void Download(const QNetworkRequest &req,
                  const QString &file = QString());
    void Download(const QUrl &target,
                  const QUrl &referer,
                  const QString &file = QString());
    void Download(const QString &url,
                  const QString &file = QString());

    void SetSource(const QUrl&);
    void SetSource(const QByteArray&);
    void SetSource(const QString&);

    View *OpenInNew(QUrl url){ return (this->*m_OpenInNewMethod0)(url);}
    View *OpenInNew(QList<QUrl> urls){ return (this->*m_OpenInNewMethod1)(urls);}
    View *OpenInNew(QString query){ return (this->*m_OpenInNewMethod2)(query);}
    View *OpenInNew(QString key, QString query){ return (this->*m_OpenInNewMethod3)(key, query);}

    void UpKey();
    void DownKey();
    void RightKey();
    void LeftKey();
    void HomeKey();
    void EndKey();
    void PageUpKey();
    void PageDownKey();

    void Import();
    void Export();
    void AboutVanilla();
    void AboutQt();
    void OpenSettings();
    void OpenDirectorySettings();
    void Quit();

    void ClearCookies();
    void ClearHttpCache();
    void ClearVisitedLinks();

    void ToggleNotifier();
    void ToggleReceiver();
    void ToggleMenuBar();
    void ToggleTreeBar();
    void ToggleToolBar();
    void ToggleFullScreen();
    void ToggleMaximized();
    void ToggleMinimized();
    void ToggleShaded();
    MainWindow *ShadeWindow(MainWindow *win = 0);
    MainWindow *UnshadeWindow(MainWindow *win = 0);
    MainWindow *NewWindow(int id = 0);
    MainWindow *CloseWindow(MainWindow *win = 0);
    MainWindow *SwitchWindow(bool next = true);
    MainWindow *NextWindow();
    MainWindow *PrevWindow();

    void Back();
    void Forward();
    void Rewind();
    void FastForward();
    void UpDirectory();
    void Close();
    void Restore();
    void Recreate();
    void NextView();
    void PrevView();
    void BuryView();
    void DigView();
    void FirstView();
    void SecondView();
    void ThirdView();
    void FourthView();
    void FifthView();
    void SixthView();
    void SeventhView();
    void EighthView();
    void NinthView();
    void TenthView();
    void LastView();
    void NewViewNode();
    void CloneViewNode();
    void DisplayAccessKey();
    void DisplayViewTree();
    void DisplayTrashTree();
    void OpenTextSeeker();
    void OpenQueryEditor();
    void OpenUrlEditor();
    void OpenCommand();
    void ReleaseHiddenView();
    void Load();

    void Copy();
    void Cut();
    void Paste();

#define VANILLA_EDIT_ACTION(name) void name();
    FOR_EACH_EDIT_EVENTS(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION

    void Undo();
    void Redo();
    void SelectAll();
    void Unselect();
    void Reload();
    void ReloadAndBypassCache();
    void Stop();
    void StopAndUnselect();

    void Print();
    void Save();
    void ZoomIn();
    void ZoomOut();
    void ViewSource();
    void ApplySource();

    void OpenBookmarklet();
    void SearchWith();
    void AddSearchEngine();
    void AddBookmarklet();
    void InspectElement();

    void CopyUrl();
    void CopyTitle();
    void CopyPageAsLink();
    void CopySelectedHtml();
    void OpenWithDefault();
    void OpenWithCommand();

    void ClickElement();
    void FocusElement();
    void HoverElement();

    void LoadLink();
    void OpenLink();
    void DownloadLink();
    void CopyLinkUrl();
    void CopyLinkHtml();
    void OpenLinkWithDefault();
    void OpenLinkWithCommand();

    void LoadImage();
    void OpenImage();
    void DownloadImage();
    void CopyImage();
    void CopyImageUrl();
    void CopyImageHtml();
    void OpenImageWithDefault();
    void OpenImageWithCommand();

    void LoadMedia();
    void OpenMedia();
    void DownloadMedia();
    void ToggleMediaControls();
    void ToggleMediaLoop();
    void ToggleMediaPlayPause();
    void ToggleMediaMute();
    void CopyMediaUrl();
    void CopyMediaHtml();
    void OpenMediaWithDefault();
    void OpenMediaWithCommand();

    void OpenInNewViewNode();
    void OpenInNewDirectory();
    void OpenOnRoot();

    void OpenInNewViewNodeForeground();
    void OpenInNewDirectoryForeground();
    void OpenOnRootForeground();

    void OpenInNewViewNodeBackground();
    void OpenInNewDirectoryBackground();
    void OpenOnRootBackground();

    void OpenInNewViewNodeThisWindow();
    void OpenInNewDirectoryThisWindow();
    void OpenOnRootThisWindow();

    void OpenInNewViewNodeNewWindow();
    void OpenInNewDirectoryNewWindow();
    void OpenOnRootNewWindow();

    void OpenImageInNewViewNode();
    void OpenImageInNewDirectory();
    void OpenImageOnRoot();

    void OpenImageInNewViewNodeForeground();
    void OpenImageInNewDirectoryForeground();
    void OpenImageOnRootForeground();

    void OpenImageInNewViewNodeBackground();
    void OpenImageInNewDirectoryBackground();
    void OpenImageOnRootBackground();

    void OpenImageInNewViewNodeThisWindow();
    void OpenImageInNewDirectoryThisWindow();
    void OpenImageOnRootThisWindow();

    void OpenImageInNewViewNodeNewWindow();
    void OpenImageInNewDirectoryNewWindow();
    void OpenImageOnRootNewWindow();

    void OpenMediaInNewViewNode();
    void OpenMediaInNewDirectory();
    void OpenMediaOnRoot();

    void OpenMediaInNewViewNodeForeground();
    void OpenMediaInNewDirectoryForeground();
    void OpenMediaOnRootForeground();

    void OpenMediaInNewViewNodeBackground();
    void OpenMediaInNewDirectoryBackground();
    void OpenMediaOnRootBackground();

    void OpenMediaInNewViewNodeThisWindow();
    void OpenMediaInNewDirectoryThisWindow();
    void OpenMediaOnRootThisWindow();

    void OpenMediaInNewViewNodeNewWindow();
    void OpenMediaInNewDirectoryNewWindow();
    void OpenMediaOnRootNewWindow();

    View *OpenInNewViewNode(QUrl);
    View *OpenInNewDirectory(QUrl);
    View *OpenOnRoot(QUrl);

    View *OpenInNewViewNode(QList<QUrl>);
    View *OpenInNewDirectory(QList<QUrl>);
    View *OpenOnRoot(QList<QUrl>);

    View *OpenInNewViewNode(QString);
    View *OpenInNewDirectory(QString);
    View *OpenOnRoot(QString);

    View *OpenInNewViewNode(QString, QString);
    View *OpenInNewDirectory(QString, QString);
    View *OpenOnRoot(QString, QString);

    View *OpenInNewViewNodeBackground(QUrl);
    View *OpenInNewDirectoryBackground(QUrl);
    View *OpenOnRootBackground(QUrl);

    View *OpenInNewViewNodeBackground(QList<QUrl>);
    View *OpenInNewDirectoryBackground(QList<QUrl>);
    View *OpenOnRootBackground(QList<QUrl>);

    View *OpenInNewViewNodeBackground(QString);
    View *OpenInNewDirectoryBackground(QString);
    View *OpenOnRootBackground(QString);

    View *OpenInNewViewNodeBackground(QString, QString);
    View *OpenInNewDirectoryBackground(QString, QString);
    View *OpenOnRootBackground(QString, QString);

    View *OpenInNewViewNodeNewWindow(QUrl);
    View *OpenInNewDirectoryNewWindow(QUrl);
    View *OpenOnRootNewWindow(QUrl);

    View *OpenInNewViewNodeNewWindow(QList<QUrl>);
    View *OpenInNewDirectoryNewWindow(QList<QUrl>);
    View *OpenOnRootNewWindow(QList<QUrl>);

    View *OpenInNewViewNodeNewWindow(QString);
    View *OpenInNewDirectoryNewWindow(QString);
    View *OpenOnRootNewWindow(QString);

    View *OpenInNewViewNodeNewWindow(QString, QString);
    View *OpenInNewDirectoryNewWindow(QString, QString);
    View *OpenOnRootNewWindow(QString, QString);

    void OpenAllUrl();
    void OpenAllImage();
    void OpenTextAsUrl();
    void SaveAllUrl();
    void SaveAllImage();
    void SaveTextAsUrl();

signals:
    void urlChanged(const QUrl&);
    void titleChanged(const QString&);
    void loadStarted();
    void loadProgress(int);
    void loadFinished(bool);
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void linkHovered(const QString&, const QString&, const QString&);

    void ViewChanged();
    void ScrollChanged(QPointF);

public:
    QAction *Action(CustomAction a, QVariant data = QVariant());
    void DisplayContextMenu(QWidget *parent, SharedWebElement elem, QPoint localPos, QPoint globalPos, MediaType type = MediaTypeNone,
                            const std::function<void(QMenu*)> &extra = std::function<void(QMenu*)>());

public slots:
    void DownloadSuggest(const QUrl&);
signals:
    void SuggestResult(const QByteArray&);

private:
    QMap<Page::CustomAction, QAction*> m_ActionTable;

    static QMap<QString, SearchEngine> m_SearchEngineMap;
    static QMap<QString, Bookmarklet> m_BookmarkletMap;
    static OpenCommandOperation m_OpenCommandOperation;

    View *m_View;
    NetworkAccessManager *m_NetworkAccessManager;

    View *(Page::*m_OpenInNewMethod0)(QUrl);
    View *(Page::*m_OpenInNewMethod1)(QList<QUrl>);
    View *(Page::*m_OpenInNewMethod2)(QString);
    View *(Page::*m_OpenInNewMethod3)(QString, QString);

    TreeBank *GetTB();
    TreeBank *MakeTB();
    TreeBank *SuitTB();
    void LinkReq(QAction*, std::function<void(QList<QNetworkRequest>)>);
    void ImageReq(QAction*, std::function<void(QList<QNetworkRequest>)>);

    void UrlCountCheck(int count, BoolCallBack callBack);
};
#endif
