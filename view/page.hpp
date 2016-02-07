#ifndef PAGE_HPP
#define PAGE_HPP

#include "switch.hpp"
#include "const.hpp"

#include <QObject>

#include <functional>

#include "callback.hpp"
#include "application.hpp"

class QUrl;
class QString;
class QStringList;
class QNetworkRequest;
class QMenu;

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
    void UpDirectory();
    void Close();
    void Restore();
    void Recreate();
    void NextView();
    void PrevView();
    void BuryView();
    void DigView();
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

    // cannot use 'LoadImage' because of WINAPI...
    void LoadImage_();
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
    void ButtonCleared();
    void RenderFinished();

public:
    enum CustomAction {
        We_NoAction,

        // key events.
        Ke_Up,
        Ke_Down,
        Ke_Right,
        Ke_Left,
        Ke_Home,
        Ke_End,
        Ke_PageUp,
        Ke_PageDown,

        // application events.
        We_Import,
        We_Export,
        We_AboutVanilla,
        We_AboutQt,
        We_Quit,

        // window events.
        We_ToggleNotifier,
        We_ToggleReceiver,
        We_ToggleMenuBar,
        We_ToggleTreeBar,
        We_ToggleToolBar,
        We_ToggleFullScreen,
        We_ToggleMaximized,
        We_ToggleMinimized,
        We_ToggleShaded,
        We_ShadeWindow,
        We_UnshadeWindow,
        We_NewWindow,
        We_CloseWindow,
        We_SwitchWindow,
        We_NextWindow,
        We_PrevWindow,

        // treebank events.
        We_Back,
        We_Forward,
        We_UpDirectory,
        We_Close,
        We_Restore,
        We_Recreate,
        We_NextView,
        We_PrevView,
        We_BuryView,
        We_DigView,
        We_NewViewNode,
        We_NewHistNode,
        We_CloneViewNode,
        We_CloneHistNode,
        We_DisplayViewTree,
        We_DisplayHistTree,
        We_DisplayTrashTree,
        We_DisplayAccessKey,
        We_OpenTextSeeker,
        We_OpenQueryEditor,
        We_OpenUrlEditor,
        We_OpenCommand,
        We_ReleaseHiddenView,
        We_Load,

        // web events.
        We_Copy,
        We_Cut,
        We_Paste,
        We_Undo,
        We_Redo,
        We_SelectAll,
        We_Unselect,
        We_Reload,
        We_ReloadAndBypassCache,
        We_Stop,
        We_StopAndUnselect,

        We_Print,
        We_Save,
        We_ZoomIn,
        We_ZoomOut,
        We_ViewSource,
        We_ApplySource,

        We_OpenBookmarklet,
        We_SearchWith,
        We_AddSearchEngine,
        We_AddBookmarklet,
        We_InspectElement,

        We_CopyUrl,
        We_CopyTitle,
        We_CopyPageAsLink,
        We_CopySelectedHtml,
        We_OpenWithIE,
        We_OpenWithEdge,
        We_OpenWithFF,
        We_OpenWithOpera,
        We_OpenWithOPR,
        We_OpenWithSafari,
        We_OpenWithChrome,
        We_OpenWithSleipnir,
        We_OpenWithVivaldi,
        We_OpenWithCustom,

        // element events.
        We_ClickElement,
        We_FocusElement,
        We_HoverElement,

        We_LoadLink,
        We_OpenLink,
        We_DownloadLink,
        We_CopyLinkUrl,
        We_CopyLinkHtml,
        We_OpenLinkWithIE,
        We_OpenLinkWithEdge,
        We_OpenLinkWithFF,
        We_OpenLinkWithOpera,
        We_OpenLinkWithOPR,
        We_OpenLinkWithSafari,
        We_OpenLinkWithChrome,
        We_OpenLinkWithSleipnir,
        We_OpenLinkWithVivaldi,
        We_OpenLinkWithCustom,

        We_LoadImage,
        We_OpenImage,
        We_DownloadImage,
        We_CopyImage,
        We_CopyImageUrl,
        We_CopyImageHtml,
        We_OpenImageWithIE,
        We_OpenImageWithEdge,
        We_OpenImageWithFF,
        We_OpenImageWithOpera,
        We_OpenImageWithOPR,
        We_OpenImageWithSafari,
        We_OpenImageWithChrome,
        We_OpenImageWithSleipnir,
        We_OpenImageWithVivaldi,
        We_OpenImageWithCustom,

        // link opner(follow modifier).
        We_OpenInNewViewNode,
        We_OpenInNewHistNode,
        We_OpenInNewDirectory,
        We_OpenOnRoot,
        // link opner(foreground).
        We_OpenInNewViewNodeForeground,
        We_OpenInNewHistNodeForeground,
        We_OpenInNewDirectoryForeground,
        We_OpenOnRootForeground,
        // link opner(background).
        We_OpenInNewViewNodeBackground,
        We_OpenInNewHistNodeBackground,
        We_OpenInNewDirectoryBackground,
        We_OpenOnRootBackground,
        // link opner(same window).
        We_OpenInNewViewNodeThisWindow,
        We_OpenInNewHistNodeThisWindow,
        We_OpenInNewDirectoryThisWindow,
        We_OpenOnRootThisWindow,
        // link opner(new window).
        We_OpenInNewViewNodeNewWindow,
        We_OpenInNewHistNodeNewWindow,
        We_OpenInNewDirectoryNewWindow,
        We_OpenOnRootNewWindow,

        // image opner(follow modifier).
        We_OpenImageInNewViewNode,
        We_OpenImageInNewHistNode,
        We_OpenImageInNewDirectory,
        We_OpenImageOnRoot,
        // image opner(foreground).
        We_OpenImageInNewViewNodeForeground,
        We_OpenImageInNewHistNodeForeground,
        We_OpenImageInNewDirectoryForeground,
        We_OpenImageOnRootForeground,
        // image opner(background).
        We_OpenImageInNewViewNodeBackground,
        We_OpenImageInNewHistNodeBackground,
        We_OpenImageInNewDirectoryBackground,
        We_OpenImageOnRootBackground,
        // image opner(same window).
        We_OpenImageInNewViewNodeThisWindow,
        We_OpenImageInNewHistNodeThisWindow,
        We_OpenImageInNewDirectoryThisWindow,
        We_OpenImageOnRootThisWindow,
        // image opner(new window).
        We_OpenImageInNewViewNodeNewWindow,
        We_OpenImageInNewHistNodeNewWindow,
        We_OpenImageInNewDirectoryNewWindow,
        We_OpenImageOnRootNewWindow,

        // auto focus (solid link) opner.
        We_OpenAllUrl,
        We_OpenAllImage,
        We_OpenTextAsUrl,
        We_SaveAllUrl,
        We_SaveAllImage,
        We_SaveTextAsUrl,
    };

    static inline CustomAction StringToAction(QString str){

        if(str == QStringLiteral("NoAction"))                          return We_NoAction;

        // key events.
        if(str == QStringLiteral("Up"))                                return Ke_Up;
        if(str == QStringLiteral("Down"))                              return Ke_Down;
        if(str == QStringLiteral("Right"))                             return Ke_Right;
        if(str == QStringLiteral("Left"))                              return Ke_Left;
        if(str == QStringLiteral("Home"))                              return Ke_Home;
        if(str == QStringLiteral("End"))                               return Ke_End;
        if(str == QStringLiteral("PageUp"))                            return Ke_PageUp;
        if(str == QStringLiteral("PageDown"))                          return Ke_PageDown;

        // application events.
        if(str == QStringLiteral("Import"))                            return We_Import;
        if(str == QStringLiteral("Export"))                            return We_Export;
        if(str == QStringLiteral("AboutVanilla"))                      return We_AboutVanilla;
        if(str == QStringLiteral("AboutQt"))                           return We_AboutQt;
        if(str == QStringLiteral("Quit"))                              return We_Quit;

        // window events.
        if(str == QStringLiteral("ToggleNotifier"))                    return We_ToggleNotifier;
        if(str == QStringLiteral("ToggleReceiver"))                    return We_ToggleReceiver;
        if(str == QStringLiteral("ToggleMenuBar"))                     return We_ToggleMenuBar;
        if(str == QStringLiteral("ToggleTreeBar"))                     return We_ToggleTreeBar;
        if(str == QStringLiteral("ToggleToolBar"))                     return We_ToggleToolBar;
        if(str == QStringLiteral("ToggleFullScreen"))                  return We_ToggleFullScreen;
        if(str == QStringLiteral("ToggleMaximized"))                   return We_ToggleMaximized;
        if(str == QStringLiteral("ToggleMinimized"))                   return We_ToggleMinimized;
        if(str == QStringLiteral("ToggleShaded"))                      return We_ToggleShaded;
        if(str == QStringLiteral("ShadeWindow"))                       return We_ShadeWindow;
        if(str == QStringLiteral("UnshadeWindow"))                     return We_UnshadeWindow;
        if(str == QStringLiteral("NewWindow"))                         return We_NewWindow;
        if(str == QStringLiteral("CloseWindow"))                       return We_CloseWindow;
        if(str == QStringLiteral("SwitchWindow"))                      return We_SwitchWindow;
        if(str == QStringLiteral("NextWindow"))                        return We_NextWindow;
        if(str == QStringLiteral("PrevWindow"))                        return We_PrevWindow;

        // treebank events.
        if(str == QStringLiteral("Back"))                              return We_Back;
        if(str == QStringLiteral("Forward"))                           return We_Forward;
        if(str == QStringLiteral("UpDirectory"))                       return We_UpDirectory;
        if(str == QStringLiteral("Close"))                             return We_Close;
        if(str == QStringLiteral("Restore"))                           return We_Restore;
        if(str == QStringLiteral("Recreate"))                          return We_Recreate;
        if(str == QStringLiteral("NextView"))                          return We_NextView;
        if(str == QStringLiteral("PrevView"))                          return We_PrevView;
        if(str == QStringLiteral("BuryView"))                          return We_BuryView;
        if(str == QStringLiteral("DigView"))                           return We_DigView;
        if(str == QStringLiteral("NewViewNode"))                       return We_NewViewNode;
        if(str == QStringLiteral("NewHistNode"))                       return We_NewHistNode;
        if(str == QStringLiteral("CloneViewNode"))                     return We_CloneViewNode;
        if(str == QStringLiteral("CloneHistNode"))                     return We_CloneHistNode;
        if(str == QStringLiteral("DisplayAccessKey"))                  return We_DisplayAccessKey;
        if(str == QStringLiteral("DisplayViewTree"))                   return We_DisplayViewTree;
        if(str == QStringLiteral("DisplayHistTree"))                   return We_DisplayHistTree;
        if(str == QStringLiteral("DisplayTrashTree"))                  return We_DisplayTrashTree;
        if(str == QStringLiteral("OpenTextSeeker"))                    return We_OpenTextSeeker;
        if(str == QStringLiteral("OpenQueryEditor"))                   return We_OpenQueryEditor;
        if(str == QStringLiteral("OpenUrlEditor"))                     return We_OpenUrlEditor;
        if(str == QStringLiteral("OpenCommand"))                       return We_OpenCommand;
        if(str == QStringLiteral("ReleaseHiddenView"))                 return We_ReleaseHiddenView;
        if(str == QStringLiteral("Load"))                              return We_Load;

        // web events.
        if(str == QStringLiteral("Copy"))                              return We_Copy;
        if(str == QStringLiteral("Cut"))                               return We_Cut;
        if(str == QStringLiteral("Paste"))                             return We_Paste;
        if(str == QStringLiteral("Undo"))                              return We_Undo;
        if(str == QStringLiteral("Redo"))                              return We_Redo;
        if(str == QStringLiteral("SelectAll"))                         return We_SelectAll;
        if(str == QStringLiteral("Unselect"))                          return We_Unselect;
        if(str == QStringLiteral("Reload"))                            return We_Reload;
        if(str == QStringLiteral("ReloadAndBypassCache"))              return We_ReloadAndBypassCache;
        if(str == QStringLiteral("Stop"))                              return We_Stop;
        if(str == QStringLiteral("StopAndUnselect"))                   return We_StopAndUnselect;

        if(str == QStringLiteral("Print"))                             return We_Print;
        if(str == QStringLiteral("Save"))                              return We_Save;
        if(str == QStringLiteral("ZoomIn"))                            return We_ZoomIn;
        if(str == QStringLiteral("ZoomOut"))                           return We_ZoomOut;
        if(str == QStringLiteral("ViewSource"))                        return We_ViewSource;
        if(str == QStringLiteral("ApplySource"))                       return We_ApplySource;

        if(str == QStringLiteral("OpenBookmarklet"))                   return We_OpenBookmarklet;
        if(str == QStringLiteral("SearchWith"))                        return We_SearchWith;
        if(str == QStringLiteral("AddSearchEngine"))                   return We_AddSearchEngine;
        if(str == QStringLiteral("AddBookmarklet"))                    return We_AddBookmarklet;
        if(str == QStringLiteral("InspectElement"))                    return We_InspectElement;

        if(str == QStringLiteral("CopyUrl"))                           return We_CopyUrl;
        if(str == QStringLiteral("CopyTitle"))                         return We_CopyTitle;
        if(str == QStringLiteral("CopyPageAsLink"))                    return We_CopyPageAsLink;
        if(str == QStringLiteral("CopySelectedHtml"))                  return We_CopySelectedHtml;
        if(str == QStringLiteral("OpenWithIE"))                        return We_OpenWithIE;
        if(str == QStringLiteral("OpenWithEdge"))                      return We_OpenWithEdge;
        if(str == QStringLiteral("OpenWithFF"))                        return We_OpenWithFF;
        if(str == QStringLiteral("OpenWithOpera"))                     return We_OpenWithOpera;
        if(str == QStringLiteral("OpenWithOPR"))                       return We_OpenWithOPR;
        if(str == QStringLiteral("OpenWithSafari"))                    return We_OpenWithSafari;
        if(str == QStringLiteral("OpenWithChrome"))                    return We_OpenWithChrome;
        if(str == QStringLiteral("OpenWithSleipnir"))                  return We_OpenWithSleipnir;
        if(str == QStringLiteral("OpenWithVivaldi"))                   return We_OpenWithVivaldi;
        if(str == QStringLiteral("OpenWithCustom"))                    return We_OpenWithCustom;

        // element events.
        if(str == QStringLiteral("ClickElement"))                      return We_ClickElement;
        if(str == QStringLiteral("FocusElement"))                      return We_FocusElement;
        if(str == QStringLiteral("HoverElement"))                      return We_HoverElement;

        if(str == QStringLiteral("LoadLink"))                          return We_LoadLink;
        if(str == QStringLiteral("OpenLink"))                          return We_OpenLink;
        if(str == QStringLiteral("DownloadLink"))                      return We_DownloadLink;
        if(str == QStringLiteral("CopyLinkUrl"))                       return We_CopyLinkUrl;
        if(str == QStringLiteral("CopyLinkHtml"))                      return We_CopyLinkHtml;
        if(str == QStringLiteral("OpenLinkWithIE"))                    return We_OpenLinkWithIE;
        if(str == QStringLiteral("OpenLinkWithEdge"))                  return We_OpenLinkWithEdge;
        if(str == QStringLiteral("OpenLinkWithFF"))                    return We_OpenLinkWithFF;
        if(str == QStringLiteral("OpenLinkWithOpera"))                 return We_OpenLinkWithOpera;
        if(str == QStringLiteral("OpenLinkWithOPR"))                   return We_OpenLinkWithOPR;
        if(str == QStringLiteral("OpenLinkWithSafari"))                return We_OpenLinkWithSafari;
        if(str == QStringLiteral("OpenLinkWithChrome"))                return We_OpenLinkWithChrome;
        if(str == QStringLiteral("OpenLinkWithSleipnir"))              return We_OpenLinkWithSleipnir;
        if(str == QStringLiteral("OpenLinkWithVivaldi"))               return We_OpenLinkWithVivaldi;
        if(str == QStringLiteral("OpenLinkWithCustom"))                return We_OpenLinkWithCustom;

        if(str == QStringLiteral("LoadImage"))                         return We_LoadImage;
        if(str == QStringLiteral("OpenImage"))                         return We_OpenImage;
        if(str == QStringLiteral("DownloadImage"))                     return We_DownloadImage;
        if(str == QStringLiteral("CopyImage"))                         return We_CopyImage;
        if(str == QStringLiteral("CopyImageUrl"))                      return We_CopyImageUrl;
        if(str == QStringLiteral("CopyImageHtml"))                     return We_CopyImageHtml;
        if(str == QStringLiteral("OpenImageWithIE"))                   return We_OpenImageWithIE;
        if(str == QStringLiteral("OpenImageWithEdge"))                 return We_OpenImageWithEdge;
        if(str == QStringLiteral("OpenImageWithFF"))                   return We_OpenImageWithFF;
        if(str == QStringLiteral("OpenImageWithOpera"))                return We_OpenImageWithOpera;
        if(str == QStringLiteral("OpenImageWithOPR"))                  return We_OpenImageWithOPR;
        if(str == QStringLiteral("OpenImageWithSafari"))               return We_OpenImageWithSafari;
        if(str == QStringLiteral("OpenImageWithChrome"))               return We_OpenImageWithChrome;
        if(str == QStringLiteral("OpenImageWithSleipnir"))             return We_OpenImageWithSleipnir;
        if(str == QStringLiteral("OpenImageWithVivaldi"))              return We_OpenImageWithVivaldi;
        if(str == QStringLiteral("OpenImageWithCustom"))               return We_OpenImageWithCustom;

        // link opner(follow modifier).
        if(str == QStringLiteral("OpenInNewViewNode"))                 return We_OpenInNewViewNode;
        if(str == QStringLiteral("OpenInNewHistNode"))                 return We_OpenInNewHistNode;
        if(str == QStringLiteral("OpenInNewDirectory"))                return We_OpenInNewDirectory;
        if(str == QStringLiteral("OpenOnRoot"))                        return We_OpenOnRoot;
        // link opner(foreground).
        if(str == QStringLiteral("OpenInNewViewNodeForeground"))       return We_OpenInNewViewNodeForeground;
        if(str == QStringLiteral("OpenInNewHistNodeForeground"))       return We_OpenInNewHistNodeForeground;
        if(str == QStringLiteral("OpenInNewDirectoryForeground"))      return We_OpenInNewDirectoryForeground;
        if(str == QStringLiteral("OpenOnRootForeground"))              return We_OpenOnRootForeground;
        // link opner(background).
        if(str == QStringLiteral("OpenInNewViewNodeBackground"))       return We_OpenInNewViewNodeBackground;
        if(str == QStringLiteral("OpenInNewHistNodeBackground"))       return We_OpenInNewHistNodeBackground;
        if(str == QStringLiteral("OpenInNewDirectoryBackground"))      return We_OpenInNewDirectoryBackground;
        if(str == QStringLiteral("OpenOnRootBackground"))              return We_OpenOnRootBackground;
        // link opner(same window).
        if(str == QStringLiteral("OpenInNewViewNodeThisWindow"))       return We_OpenInNewViewNodeThisWindow;
        if(str == QStringLiteral("OpenInNewHistNodeThisWindow"))       return We_OpenInNewHistNodeThisWindow;
        if(str == QStringLiteral("OpenInNewDirectoryThisWindow"))      return We_OpenInNewDirectoryThisWindow;
        if(str == QStringLiteral("OpenOnRootThisWindow"))              return We_OpenOnRootThisWindow;
        // link opner(new window).
        if(str == QStringLiteral("OpenInNewViewNodeNewWindow"))        return We_OpenInNewViewNodeNewWindow;
        if(str == QStringLiteral("OpenInNewHistNodeNewWindow"))        return We_OpenInNewHistNodeNewWindow;
        if(str == QStringLiteral("OpenInNewDirectoryNewWindow"))       return We_OpenInNewDirectoryNewWindow;
        if(str == QStringLiteral("OpenOnRootNewWindow"))               return We_OpenOnRootNewWindow;

        // image opner(follow modifier).
        if(str == QStringLiteral("OpenImageInNewViewNode"))            return We_OpenImageInNewViewNode;
        if(str == QStringLiteral("OpenImageInNewHistNode"))            return We_OpenImageInNewHistNode;
        if(str == QStringLiteral("OpenImageInNewDirectory"))           return We_OpenImageInNewDirectory;
        if(str == QStringLiteral("OpenImageOnRoot"))                   return We_OpenImageOnRoot;
        // image opner(foreground).
        if(str == QStringLiteral("OpenImageInNewViewNodeForeground"))  return We_OpenImageInNewViewNodeForeground;
        if(str == QStringLiteral("OpenImageInNewHistNodeForeground"))  return We_OpenImageInNewHistNodeForeground;
        if(str == QStringLiteral("OpenImageInNewDirectoryForeground")) return We_OpenImageInNewDirectoryForeground;
        if(str == QStringLiteral("OpenImageOnRootForeground"))         return We_OpenImageOnRootForeground;
        // image opner(background).
        if(str == QStringLiteral("OpenImageInNewViewNodeBackground"))  return We_OpenImageInNewViewNodeBackground;
        if(str == QStringLiteral("OpenImageInNewHistNodeBackground"))  return We_OpenImageInNewHistNodeBackground;
        if(str == QStringLiteral("OpenImageInNewDirectoryBackground")) return We_OpenImageInNewDirectoryBackground;
        if(str == QStringLiteral("OpenImageOnRootBackground"))         return We_OpenImageOnRootBackground;
        // image opner(same window).
        if(str == QStringLiteral("OpenImageInNewViewNodeForeground"))  return We_OpenImageInNewViewNodeForeground;
        if(str == QStringLiteral("OpenImageInNewHistNodeForeground"))  return We_OpenImageInNewHistNodeForeground;
        if(str == QStringLiteral("OpenImageInNewDirectoryForeground")) return We_OpenImageInNewDirectoryForeground;
        if(str == QStringLiteral("OpenImageOnRootForeground"))         return We_OpenImageOnRootForeground;
        // image opner(new window).
        if(str == QStringLiteral("OpenImageInNewViewNodeNewWindow"))   return We_OpenImageInNewViewNodeNewWindow;
        if(str == QStringLiteral("OpenImageInNewHistNodeNewWindow"))   return We_OpenImageInNewHistNodeNewWindow;
        if(str == QStringLiteral("OpenImageInNewDirectoryNewWindow"))  return We_OpenImageInNewDirectoryNewWindow;
        if(str == QStringLiteral("OpenImageOnRootNewWindow"))          return We_OpenImageOnRootNewWindow;

        // auto focs (solid link) opner.
        if(str == QStringLiteral("OpenAllUrl"))                        return We_OpenAllUrl;
        if(str == QStringLiteral("OpenAllImage"))                      return We_OpenAllImage;
        if(str == QStringLiteral("OpenTextAsUrl"))                     return We_OpenTextAsUrl;
        if(str == QStringLiteral("SaveAllUrl"))                        return We_SaveAllUrl;
        if(str == QStringLiteral("SaveAllImage"))                      return We_SaveAllImage;
        if(str == QStringLiteral("SaveTextAsUrl"))                     return We_SaveTextAsUrl;
                                                                       return We_NoAction;
    }

    static inline QString ActionToString(CustomAction action){

        if(action == We_NoAction)                          return QStringLiteral("NoAction");

        // key events.
        if(action == Ke_Up)                                return QStringLiteral("Up");
        if(action == Ke_Down)                              return QStringLiteral("Down");
        if(action == Ke_Right)                             return QStringLiteral("Right");
        if(action == Ke_Left)                              return QStringLiteral("Left");
        if(action == Ke_Home)                              return QStringLiteral("Home");
        if(action == Ke_End)                               return QStringLiteral("End");
        if(action == Ke_PageUp)                            return QStringLiteral("PageUp");
        if(action == Ke_PageDown)                          return QStringLiteral("PageDown");

        // application events.
        if(action == We_Import)                            return QStringLiteral("Import");
        if(action == We_Export)                            return QStringLiteral("Export");
        if(action == We_AboutVanilla)                      return QStringLiteral("AboutVanilla");
        if(action == We_AboutQt)                           return QStringLiteral("AboutQt");
        if(action == We_Quit)                              return QStringLiteral("Quit");

        // window events.
        if(action == We_ToggleNotifier)                    return QStringLiteral("ToggleNotifier");
        if(action == We_ToggleReceiver)                    return QStringLiteral("ToggleReceiver");
        if(action == We_ToggleMenuBar)                     return QStringLiteral("ToggleMenuBar");
        if(action == We_ToggleTreeBar)                     return QStringLiteral("ToggleTreeBar");
        if(action == We_ToggleToolBar)                     return QStringLiteral("ToggleToolBar");
        if(action == We_ToggleFullScreen)                  return QStringLiteral("ToggleFullScreen");
        if(action == We_ToggleMaximized)                   return QStringLiteral("ToggleMaximized");
        if(action == We_ToggleMinimized)                   return QStringLiteral("ToggleMinimized");
        if(action == We_ToggleShaded)                      return QStringLiteral("ToggleShaded");
        if(action == We_ShadeWindow)                       return QStringLiteral("ShadeWindow");
        if(action == We_UnshadeWindow)                     return QStringLiteral("UnshadeWindow");
        if(action == We_NewWindow)                         return QStringLiteral("NewWindow");
        if(action == We_CloseWindow)                       return QStringLiteral("CloseWindow");
        if(action == We_SwitchWindow)                      return QStringLiteral("SwitchWindow");
        if(action == We_NextWindow)                        return QStringLiteral("NextWindow");
        if(action == We_PrevWindow)                        return QStringLiteral("PrevWindow");

        // treebank events.
        if(action == We_Back)                              return QStringLiteral("Back");
        if(action == We_Forward)                           return QStringLiteral("Forward");
        if(action == We_UpDirectory)                       return QStringLiteral("UpDirectory");
        if(action == We_Close)                             return QStringLiteral("Close");
        if(action == We_Restore)                           return QStringLiteral("Restore");
        if(action == We_Recreate)                          return QStringLiteral("Recreate");
        if(action == We_NextView)                          return QStringLiteral("NextView");
        if(action == We_PrevView)                          return QStringLiteral("PrevView");
        if(action == We_BuryView)                          return QStringLiteral("BuryView");
        if(action == We_DigView)                           return QStringLiteral("DigView");
        if(action == We_NewViewNode)                       return QStringLiteral("NewViewNode");
        if(action == We_NewHistNode)                       return QStringLiteral("NewHistNode");
        if(action == We_CloneViewNode)                     return QStringLiteral("CloneViewNode");
        if(action == We_CloneHistNode)                     return QStringLiteral("CloneHistNode");
        if(action == We_DisplayAccessKey)                  return QStringLiteral("DisplayAccessKey");
        if(action == We_DisplayViewTree)                   return QStringLiteral("DisplayViewTree");
        if(action == We_DisplayHistTree)                   return QStringLiteral("DisplayHistTree");
        if(action == We_DisplayTrashTree)                  return QStringLiteral("DisplayTrashTree");
        if(action == We_OpenTextSeeker)                    return QStringLiteral("OpenTextSeeker");
        if(action == We_OpenQueryEditor)                   return QStringLiteral("OpenQueryEditor");
        if(action == We_OpenUrlEditor)                     return QStringLiteral("OpenUrlEditor");
        if(action == We_OpenCommand)                       return QStringLiteral("OpenCommand");
        if(action == We_ReleaseHiddenView)                 return QStringLiteral("ReleaseHiddenView");
        if(action == We_Load)                              return QStringLiteral("Load");

        // web events.
        if(action == We_Copy)                              return QStringLiteral("Copy");
        if(action == We_Cut)                               return QStringLiteral("Cut");
        if(action == We_Paste)                             return QStringLiteral("Paste");
        if(action == We_Undo)                              return QStringLiteral("Undo");
        if(action == We_Redo)                              return QStringLiteral("Redo");
        if(action == We_SelectAll)                         return QStringLiteral("SelectAll");
        if(action == We_Unselect)                          return QStringLiteral("Unselect");
        if(action == We_Reload)                            return QStringLiteral("Reload");
        if(action == We_ReloadAndBypassCache)              return QStringLiteral("ReloadAndBypassCache");
        if(action == We_Stop)                              return QStringLiteral("Stop");
        if(action == We_StopAndUnselect)                   return QStringLiteral("StopAndUnselect");

        if(action == We_Print)                             return QStringLiteral("Print");
        if(action == We_Save)                              return QStringLiteral("Save");
        if(action == We_ZoomIn)                            return QStringLiteral("ZoomIn");
        if(action == We_ZoomOut)                           return QStringLiteral("ZoomOut");
        if(action == We_ViewSource)                        return QStringLiteral("ViewSource");
        if(action == We_ApplySource)                       return QStringLiteral("ApplySource");

        if(action == We_OpenBookmarklet)                   return QStringLiteral("OpenBookmarklet");
        if(action == We_SearchWith)                        return QStringLiteral("SearchWith");
        if(action == We_AddSearchEngine)                   return QStringLiteral("AddSearchEngine");
        if(action == We_AddBookmarklet)                    return QStringLiteral("AddBookmarklet");
        if(action == We_InspectElement)                    return QStringLiteral("InspectElement");

        if(action == We_CopyUrl)                           return QStringLiteral("CopyUrl");
        if(action == We_CopyTitle)                         return QStringLiteral("CopyTitle");
        if(action == We_CopyPageAsLink)                    return QStringLiteral("CopyPageAsLink");
        if(action == We_CopySelectedHtml)                  return QStringLiteral("CopySelectedHtml");
        if(action == We_OpenWithIE)                        return QStringLiteral("OpenWithIE");
        if(action == We_OpenWithEdge)                      return QStringLiteral("OpenWithEdge");
        if(action == We_OpenWithFF)                        return QStringLiteral("OpenWithFF");
        if(action == We_OpenWithOpera)                     return QStringLiteral("OpenWithOpera");
        if(action == We_OpenWithOPR)                       return QStringLiteral("OpenWithOPR");
        if(action == We_OpenWithSafari)                    return QStringLiteral("OpenWithSafari");
        if(action == We_OpenWithChrome)                    return QStringLiteral("OpenWithChrome");
        if(action == We_OpenWithSleipnir)                  return QStringLiteral("OpenWithSleipnir");
        if(action == We_OpenWithVivaldi)                   return QStringLiteral("OpenWithVivaldi");
        if(action == We_OpenWithCustom)                    return QStringLiteral("OpenWithCustom");

        // element events.
        if(action == We_ClickElement)                      return QStringLiteral("ClickElement");
        if(action == We_FocusElement)                      return QStringLiteral("FocusElement");
        if(action == We_HoverElement)                      return QStringLiteral("HoverElement");

        if(action == We_LoadLink)                          return QStringLiteral("LoadLink");
        if(action == We_OpenLink)                          return QStringLiteral("OpenLink");
        if(action == We_DownloadLink)                      return QStringLiteral("DownloadLink");
        if(action == We_CopyLinkUrl)                       return QStringLiteral("CopyLinkUrl");
        if(action == We_CopyLinkHtml)                      return QStringLiteral("CopyLinkHtml");
        if(action == We_OpenLinkWithIE)                    return QStringLiteral("OpenLinkWithIE");
        if(action == We_OpenLinkWithEdge)                  return QStringLiteral("OpenLinkWithEdge");
        if(action == We_OpenLinkWithFF)                    return QStringLiteral("OpenLinkWithFF");
        if(action == We_OpenLinkWithOpera)                 return QStringLiteral("OpenLinkWithOpera");
        if(action == We_OpenLinkWithOPR)                   return QStringLiteral("OpenLinkWithOPR");
        if(action == We_OpenLinkWithSafari)                return QStringLiteral("OpenLinkWithSafari");
        if(action == We_OpenLinkWithChrome)                return QStringLiteral("OpenLinkWithChrome");
        if(action == We_OpenLinkWithSleipnir)              return QStringLiteral("OpenLinkWithSleipnir");
        if(action == We_OpenLinkWithVivaldi)               return QStringLiteral("OpenLinkWithVivaldi");
        if(action == We_OpenLinkWithCustom)                return QStringLiteral("OpenLinkWithCustom");

        if(action == We_LoadImage)                         return QStringLiteral("LoadImage");
        if(action == We_OpenImage)                         return QStringLiteral("OpenImage");
        if(action == We_DownloadImage)                     return QStringLiteral("DownloadImage");
        if(action == We_CopyImage)                         return QStringLiteral("CopyImage");
        if(action == We_CopyImageUrl)                      return QStringLiteral("CopyImageUrl");
        if(action == We_CopyImageHtml)                     return QStringLiteral("CopyImageHtml");
        if(action == We_OpenImageWithIE)                   return QStringLiteral("OpenImageWithIE");
        if(action == We_OpenImageWithEdge)                 return QStringLiteral("OpenImageWithEdge");
        if(action == We_OpenImageWithFF)                   return QStringLiteral("OpenImageWithFF");
        if(action == We_OpenImageWithOpera)                return QStringLiteral("OpenImageWithOpera");
        if(action == We_OpenImageWithOPR)                  return QStringLiteral("OpenImageWithOPR");
        if(action == We_OpenImageWithSafari)               return QStringLiteral("OpenImageWithSafari");
        if(action == We_OpenImageWithChrome)               return QStringLiteral("OpenImageWithChrome");
        if(action == We_OpenImageWithSleipnir)             return QStringLiteral("OpenImageWithSleipnir");
        if(action == We_OpenImageWithVivaldi)              return QStringLiteral("OpenImageWithVivaldi");
        if(action == We_OpenImageWithCustom)               return QStringLiteral("OpenImageWithCustom");

        // link opner(follow modifier).
        if(action == We_OpenInNewViewNode)                 return QStringLiteral("OpenInNewViewNode");
        if(action == We_OpenInNewHistNode)                 return QStringLiteral("OpenInNewHistNode");
        if(action == We_OpenInNewDirectory)                return QStringLiteral("OpenInNewDirectory");
        if(action == We_OpenOnRoot)                        return QStringLiteral("OpenOnRoot");
        // link opner(foreground).
        if(action == We_OpenInNewViewNodeForeground)       return QStringLiteral("OpenInNewViewNodeForeground");
        if(action == We_OpenInNewHistNodeForeground)       return QStringLiteral("OpenInNewHistNodeForeground");
        if(action == We_OpenInNewDirectoryForeground)      return QStringLiteral("OpenInNewDirectoryForeground");
        if(action == We_OpenOnRootForeground)              return QStringLiteral("OpenOnRootForeground");
        // link opner(background).
        if(action == We_OpenInNewViewNodeBackground)       return QStringLiteral("OpenInNewViewNodeBackground");
        if(action == We_OpenInNewHistNodeBackground)       return QStringLiteral("OpenInNewHistNodeBackground");
        if(action == We_OpenInNewDirectoryBackground)      return QStringLiteral("OpenInNewDirectoryBackground");
        if(action == We_OpenOnRootBackground)              return QStringLiteral("OpenOnRootBackground");
        // link opner(same window).
        if(action == We_OpenInNewViewNodeThisWindow)       return QStringLiteral("OpenInNewViewNodeThisWindow");
        if(action == We_OpenInNewHistNodeThisWindow)       return QStringLiteral("OpenInNewHistNodeThisWindow");
        if(action == We_OpenInNewDirectoryThisWindow)      return QStringLiteral("OpenInNewDirectoryThisWindow");
        if(action == We_OpenOnRootThisWindow)              return QStringLiteral("OpenOnRootThisWindow");
        // link opner(newwindow).
        if(action == We_OpenInNewViewNodeNewWindow)        return QStringLiteral("OpenInNewViewNodeNewWindow");
        if(action == We_OpenInNewHistNodeNewWindow)        return QStringLiteral("OpenInNewHistNodeNewWindow");
        if(action == We_OpenInNewDirectoryNewWindow)       return QStringLiteral("OpenInNewDirectoryNewWindow");
        if(action == We_OpenOnRootNewWindow)               return QStringLiteral("OpenOnRootNewWindow");

        // image opner(follow modifier).
        if(action == We_OpenImageInNewViewNode)            return QStringLiteral("OpenImageInNewViewNode");
        if(action == We_OpenImageInNewHistNode)            return QStringLiteral("OpenImageInNewHistNode");
        if(action == We_OpenImageInNewDirectory)           return QStringLiteral("OpenImageInNewDirectory");
        if(action == We_OpenImageOnRoot)                   return QStringLiteral("OpenImageOnRoot");
        // image opner(foreground).
        if(action == We_OpenImageInNewViewNodeForeground)  return QStringLiteral("OpenImageInNewViewNodeForeground");
        if(action == We_OpenImageInNewHistNodeForeground)  return QStringLiteral("OpenImageInNewHistNodeForeground");
        if(action == We_OpenImageInNewDirectoryForeground) return QStringLiteral("OpenImageInNewDirectoryForeground");
        if(action == We_OpenImageOnRootForeground)         return QStringLiteral("OpenImageOnRootForeground");
        // image opner(background).
        if(action == We_OpenImageInNewViewNodeBackground)  return QStringLiteral("OpenImageInNewViewNodeBackground");
        if(action == We_OpenImageInNewHistNodeBackground)  return QStringLiteral("OpenImageInNewHistNodeBackground");
        if(action == We_OpenImageInNewDirectoryBackground) return QStringLiteral("OpenImageInNewDirectoryBackground");
        if(action == We_OpenImageOnRootBackground)         return QStringLiteral("OpenImageOnRootBackground");
        // image opner(same window).
        if(action == We_OpenImageInNewViewNodeThisWindow)  return QStringLiteral("OpenImageInNewViewNodeThisWindow");
        if(action == We_OpenImageInNewHistNodeThisWindow)  return QStringLiteral("OpenImageInNewHistNodeThisWindow");
        if(action == We_OpenImageInNewDirectoryThisWindow) return QStringLiteral("OpenImageInNewDirectoryThisWindow");
        if(action == We_OpenImageOnRootThisWindow)         return QStringLiteral("OpenImageOnRootThisWindow");
        // image opner(new window).
        if(action == We_OpenImageInNewViewNodeNewWindow)   return QStringLiteral("OpenImageInNewViewNodeNewWindow");
        if(action == We_OpenImageInNewHistNodeNewWindow)   return QStringLiteral("OpenImageInNewHistNodeNewWindow");
        if(action == We_OpenImageInNewDirectoryNewWindow)  return QStringLiteral("OpenImageInNewDirectoryNewWindow");
        if(action == We_OpenImageOnRootNewWindow)          return QStringLiteral("OpenImageOnRootNewWindow");

        // auto focs (solid link) opner.
        if(action == We_OpenAllUrl)                        return QStringLiteral("OpenAllUrl");
        if(action == We_OpenAllImage)                      return QStringLiteral("OpenAllImage");
        if(action == We_OpenTextAsUrl)                     return QStringLiteral("OpenTextAsUrl");
        if(action == We_SaveAllUrl)                        return QStringLiteral("SaveAllUrl");
        if(action == We_SaveAllImage)                      return QStringLiteral("SaveAllImage");
        if(action == We_SaveTextAsUrl)                     return QStringLiteral("SaveTextAsUrl");
                                                           return QStringLiteral("NoAction");
    }

    static inline bool IsValidAction(QString str){
        return str == ActionToString(StringToAction(str));
    }

    static inline bool IsValidAction(CustomAction action){
        return action == StringToAction(ActionToString(action));
    }

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
#endif
