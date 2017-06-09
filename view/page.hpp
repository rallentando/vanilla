#ifndef PAGE_HPP
#define PAGE_HPP

#include "switch.hpp"
#include "const.hpp"

#include <QObject>

#include <functional>

#include "actionmapper.hpp"
#include "callback.hpp"

class QUrl;
class QString;
class QStringList;
class QNetworkRequest;
class QMenu;
class QAction;

class View;
class ViewNode;
class HistNode;
class TreeBank;
class MainWindow;
class NetworkAccessManager;

//struct Bookmarklet{
//    QString name;
//    QString body;
//    QString replacement;
//    QString enctype;
//    QString encode;
//    //None,Apple Roman,Big5,Big5-HKSCS,CP949,EUC-JP,EUC-KR,GB18030-0,IBM 850,IBM 866,IBM 874,ISO 2022-JP,ISO 8859-1 to 10,ISO 8859-13 to 16,Iscii-Bng,Iscii-Dev,Iscii-Gjr,Iscii-Knd,Iscii-Mlm,Iscii-Ori,Iscii-Pnj,Iscii-Tlg,Iscii-and,Iscii-Tml,JIS X 0201,JIS X 0208,KOI8-R,KOI8-U,MuleLao-1,ROMAN8,Shift-JIS,TIS-620,TSCII,UTF-8,UTF-16,UTF-16BE,UTF-16LE,UTF-32,UTF-32BE,UTF-32LE,Windows-1250 to 1258,WINSAMI2
//};

typedef QStringList Bookmarklet;
// [url or js]

typedef QStringList SearchEngine;
// [url, encode, primary_mark]

class Page : public QObject{
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
        InNewHistNode,
        InNewDirectory,
        OnRoot,

        InNewViewNodeBackground,
        InNewHistNodeBackground,
        InNewDirectoryBackground,
        OnRootBackground,

        InNewViewNodeNewWindow,
        InNewHistNodeNewWindow,
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

    static QUrl        CreateQueryUrl(QString, QString key = QString());
    static QUrl        UpDirectoryUrl(QUrl);
    static QUrl        StringToUrl   (QString str,  QUrl baseUrl = QUrl());
    static QList<QUrl> ExtractUrlFromText(QString text, QUrl baseUrl = QUrl());
    static QList<QUrl> ExtractUrlFromHtml(QString html, QUrl baseUrl, FindElementsOption option);
    static QList<QUrl> DirtyStringToUrlList(QString str);

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
    void Quit();

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
    void NewHistNode();
    void CloneViewNode();
    void CloneHistNode();
    void DisplayAccessKey();
    void DisplayViewTree();
    void DisplayHistTree();
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
    void OpenWithIE();
    void OpenWithEdge();
    void OpenWithFF();
    void OpenWithOpera();
    void OpenWithOPR();
    void OpenWithSafari();
    void OpenWithChrome();
    void OpenWithSleipnir();
    void OpenWithVivaldi();
    void OpenWithCustom();

    void ClickElement();
    void FocusElement();
    void HoverElement();

    void LoadLink();
    void OpenLink();
    void DownloadLink();
    void CopyLinkUrl();
    void CopyLinkHtml();
    void OpenLinkWithIE();
    void OpenLinkWithEdge();
    void OpenLinkWithFF();
    void OpenLinkWithOpera();
    void OpenLinkWithOPR();
    void OpenLinkWithSafari();
    void OpenLinkWithChrome();
    void OpenLinkWithSleipnir();
    void OpenLinkWithVivaldi();
    void OpenLinkWithCustom();

    void LoadImage();
    void OpenImage();
    void DownloadImage();
    void CopyImage();
    void CopyImageUrl();
    void CopyImageHtml();
    void OpenImageWithIE();
    void OpenImageWithEdge();
    void OpenImageWithFF();
    void OpenImageWithOpera();
    void OpenImageWithOPR();
    void OpenImageWithSafari();
    void OpenImageWithChrome();
    void OpenImageWithSleipnir();
    void OpenImageWithVivaldi();
    void OpenImageWithCustom();

    void LoadMedia();
    void OpenMedia();
    void DownloadMedia();
    void ToggleMediaControls();
    void ToggleMediaLoop();
    void ToggleMediaPlayPause();
    void ToggleMediaMute();
    void CopyMediaUrl();
    void CopyMediaHtml();
    void OpenMediaWithIE();
    void OpenMediaWithEdge();
    void OpenMediaWithFF();
    void OpenMediaWithOpera();
    void OpenMediaWithOPR();
    void OpenMediaWithSafari();
    void OpenMediaWithChrome();
    void OpenMediaWithSleipnir();
    void OpenMediaWithVivaldi();
    void OpenMediaWithCustom();

    void OpenInNewViewNode();
    void OpenInNewHistNode();
    void OpenInNewDirectory();
    void OpenOnRoot();

    void OpenInNewViewNodeForeground();
    void OpenInNewHistNodeForeground();
    void OpenInNewDirectoryForeground();
    void OpenOnRootForeground();

    void OpenInNewViewNodeBackground();
    void OpenInNewHistNodeBackground();
    void OpenInNewDirectoryBackground();
    void OpenOnRootBackground();

    void OpenInNewViewNodeThisWindow();
    void OpenInNewHistNodeThisWindow();
    void OpenInNewDirectoryThisWindow();
    void OpenOnRootThisWindow();

    void OpenInNewViewNodeNewWindow();
    void OpenInNewHistNodeNewWindow();
    void OpenInNewDirectoryNewWindow();
    void OpenOnRootNewWindow();

    void OpenImageInNewViewNode();
    void OpenImageInNewHistNode();
    void OpenImageInNewDirectory();
    void OpenImageOnRoot();

    void OpenImageInNewViewNodeForeground();
    void OpenImageInNewHistNodeForeground();
    void OpenImageInNewDirectoryForeground();
    void OpenImageOnRootForeground();

    void OpenImageInNewViewNodeBackground();
    void OpenImageInNewHistNodeBackground();
    void OpenImageInNewDirectoryBackground();
    void OpenImageOnRootBackground();

    void OpenImageInNewViewNodeThisWindow();
    void OpenImageInNewHistNodeThisWindow();
    void OpenImageInNewDirectoryThisWindow();
    void OpenImageOnRootThisWindow();

    void OpenImageInNewViewNodeNewWindow();
    void OpenImageInNewHistNodeNewWindow();
    void OpenImageInNewDirectoryNewWindow();
    void OpenImageOnRootNewWindow();

    void OpenMediaInNewViewNode();
    void OpenMediaInNewHistNode();
    void OpenMediaInNewDirectory();
    void OpenMediaOnRoot();

    void OpenMediaInNewViewNodeForeground();
    void OpenMediaInNewHistNodeForeground();
    void OpenMediaInNewDirectoryForeground();
    void OpenMediaOnRootForeground();

    void OpenMediaInNewViewNodeBackground();
    void OpenMediaInNewHistNodeBackground();
    void OpenMediaInNewDirectoryBackground();
    void OpenMediaOnRootBackground();

    void OpenMediaInNewViewNodeThisWindow();
    void OpenMediaInNewHistNodeThisWindow();
    void OpenMediaInNewDirectoryThisWindow();
    void OpenMediaOnRootThisWindow();

    void OpenMediaInNewViewNodeNewWindow();
    void OpenMediaInNewHistNodeNewWindow();
    void OpenMediaInNewDirectoryNewWindow();
    void OpenMediaOnRootNewWindow();

    View *OpenInNewViewNode(QUrl);
    View *OpenInNewHistNode(QUrl);
    View *OpenInNewDirectory(QUrl);
    View *OpenOnRoot(QUrl);

    View *OpenInNewViewNode(QList<QUrl>);
    View *OpenInNewHistNode(QList<QUrl>);
    View *OpenInNewDirectory(QList<QUrl>);
    View *OpenOnRoot(QList<QUrl>);

    View *OpenInNewViewNode(QString);
    View *OpenInNewHistNode(QString);
    View *OpenInNewDirectory(QString);
    View *OpenOnRoot(QString);

    View *OpenInNewViewNode(QString, QString);
    View *OpenInNewHistNode(QString, QString);
    View *OpenInNewDirectory(QString, QString);
    View *OpenOnRoot(QString, QString);

    View *OpenInNewViewNodeBackground(QUrl);
    View *OpenInNewHistNodeBackground(QUrl);
    View *OpenInNewDirectoryBackground(QUrl);
    View *OpenOnRootBackground(QUrl);

    View *OpenInNewViewNodeBackground(QList<QUrl>);
    View *OpenInNewHistNodeBackground(QList<QUrl>);
    View *OpenInNewDirectoryBackground(QList<QUrl>);
    View *OpenOnRootBackground(QList<QUrl>);

    View *OpenInNewViewNodeBackground(QString);
    View *OpenInNewHistNodeBackground(QString);
    View *OpenInNewDirectoryBackground(QString);
    View *OpenOnRootBackground(QString);

    View *OpenInNewViewNodeBackground(QString, QString);
    View *OpenInNewHistNodeBackground(QString, QString);
    View *OpenInNewDirectoryBackground(QString, QString);
    View *OpenOnRootBackground(QString, QString);

    View *OpenInNewViewNodeNewWindow(QUrl);
    View *OpenInNewHistNodeNewWindow(QUrl);
    View *OpenInNewDirectoryNewWindow(QUrl);
    View *OpenOnRootNewWindow(QUrl);

    View *OpenInNewViewNodeNewWindow(QList<QUrl>);
    View *OpenInNewHistNodeNewWindow(QList<QUrl>);
    View *OpenInNewDirectoryNewWindow(QList<QUrl>);
    View *OpenOnRootNewWindow(QList<QUrl>);

    View *OpenInNewViewNodeNewWindow(QString);
    View *OpenInNewHistNodeNewWindow(QString);
    View *OpenInNewDirectoryNewWindow(QString);
    View *OpenOnRootNewWindow(QString);

    View *OpenInNewViewNodeNewWindow(QString, QString);
    View *OpenInNewHistNodeNewWindow(QString, QString);
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
#endif //ifndef PAGE_HPP
