#ifndef JSOBJECT_HPP
#define JSOBJECT_HPP

#include "switch.hpp"

#include <QObject>
#include <QInputDialog>
#include <QTimer>

#include "treebank.hpp"
#include "view.hpp"
#include "dialog.hpp"

class _Vanilla : public QObject {
    Q_OBJECT

private:
    TreeBank *m_TreeBank;

public:
    _Vanilla(TreeBank *tbank) : QObject(nullptr){
        m_TreeBank = tbank;
    }

public slots:
    int     getInt   (QString title, QString label, int    val = 0, int    min = INT_MIN, int    max = INT_MAX, int     step = 1){
        return ModalDialog::GetInt(title, label, val, min, max, step);
    }

    double  getDouble(QString title, QString label, double val = 0, double min = DBL_MIN, double max = DBL_MAX, int decimals = 1){
        return ModalDialog::GetDouble(title, label, val, min, max, decimals);
    }

    QString getItem  (QString title, QString label, QStringList items, bool editable = true){
        return ModalDialog::GetItem(title, label, items, editable);
    }

    QString getText  (QString title, QString label, QString text = QString()){
        return ModalDialog::GetText(title, label, text);
    }

    void repaint              (){ m_TreeBank->Repaint(); }
    void reconfigure          (){ m_TreeBank->Reconfigure(); }

    void up                   (){ m_TreeBank->Up(); }
    void down                 (){ m_TreeBank->Down(); }
    void right                (){ m_TreeBank->Right(); }
    void left                 (){ m_TreeBank->Left(); }
    void pageUp               (){ m_TreeBank->PageUp(); }
    void pageDown             (){ m_TreeBank->PageDown(); }
    void home                 (){ m_TreeBank->Home(); }
    void end                  (){ m_TreeBank->End(); }

    void aboutVanilla         (){ m_TreeBank->AboutVanilla(); }
    void aboutQt              (){ m_TreeBank->AboutQt(); }
    void quit                 (){ QTimer::singleShot(0, m_TreeBank, SLOT(Quit()));}

    void back                 (){ m_TreeBank->Back(); }
    void forward              (){ m_TreeBank->Forward(); }
    void rewind               (){ m_TreeBank->Rewind(); }
    void fastForward          (){ m_TreeBank->FastForward(); }
    void upDirectory          (){ m_TreeBank->UpDirectory(); }
    void close                (){ QTimer::singleShot(0, m_TreeBank, SLOT(Close())); }
    void restore              (){ m_TreeBank->Restore(); }
    void recreate             (){ QTimer::singleShot(0, m_TreeBank, SLOT(Recreate())); }
    void nextView             (){ m_TreeBank->NextView(); }
    void previousView         (){ m_TreeBank->PrevView(); }
    void buryView             (){ m_TreeBank->BuryView(); }
    void digView              (){ m_TreeBank->DigView(); }
    void firstView            (){ m_TreeBank->FirstView(); }
    void secondView           (){ m_TreeBank->SecondView(); }
    void thirdView            (){ m_TreeBank->ThirdView(); }
    void fourthView           (){ m_TreeBank->FourthView(); }
    void fifthView            (){ m_TreeBank->FifthView(); }
    void sixthView            (){ m_TreeBank->SixthView(); }
    void seventhView          (){ m_TreeBank->SeventhView(); }
    void eighthView           (){ m_TreeBank->EighthView(); }
    void ninthView            (){ m_TreeBank->NinthView(); }
    void tenthView            (){ m_TreeBank->TenthView(); }
    void lastView             (){ m_TreeBank->LastView(); }
    void displayViewtree      (){ m_TreeBank->DisplayViewTree(); }
    void displayAccessKey     (){ m_TreeBank->DisplayAccessKey(); }
    void openTextSeeker       (){ m_TreeBank->OpenTextSeeker(); }
    void openQueryEditor      (){ m_TreeBank->OpenQueryEditor(); }
    void openUrlEditor        (){ m_TreeBank->OpenUrlEditor(); }
    void openCommand          (){ m_TreeBank->OpenCommand(); }
    void releaseHiddenView    (){ m_TreeBank->ReleaseHiddenView(); }
    void load                 (){ m_TreeBank->Load(); }

    void copy                 (){ m_TreeBank->Copy(); }
    void cut                  (){ m_TreeBank->Cut(); }
    void paste                (){ m_TreeBank->Paste(); }
    void undo                 (){ m_TreeBank->Undo(); }
    void redo                 (){ m_TreeBank->Redo(); }
    void selectAll            (){ m_TreeBank->SelectAll(); }
    void unselect             (){ m_TreeBank->Unselect();}
    void reload               (){ m_TreeBank->Reload(); }
    void reloadAndBypassCache (){ m_TreeBank->ReloadAndBypassCache();}
    void stop                 (){ m_TreeBank->Stop(); }
    void stopAndUnselect      (){ m_TreeBank->StopAndUnselect();}

    void print                (){ m_TreeBank->Print(); }
    void save                 (){ m_TreeBank->Save(); }

#define VANILLA_EDIT_ACTION(name, jsName) void jsName(){ m_TreeBank->name();}
    FOR_EACH_EDIT_EVENTS_WITH_JS_NAME(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION

    void clearCookies         (){ m_TreeBank->ClearCookies();}
    void clearHttpCache       (){ m_TreeBank->ClearHttpCache();}
    void clearVisitedLinks    (){ m_TreeBank->ClearVisitedLinks();}
};

class _View : public QObject {
    Q_OBJECT

private:
    View *m_View;

public:
    _View(View *view) : QObject(nullptr){
        m_View = view;
    }

public slots:
    void up                    (){ m_View->TriggerAction(Page::_Up); }
    void down                  (){ m_View->TriggerAction(Page::_Down); }
    void right                 (){ m_View->TriggerAction(Page::_Right); }
    void left                  (){ m_View->TriggerAction(Page::_Left); }
    void home                  (){ m_View->TriggerAction(Page::_Home); }
    void end                   (){ m_View->TriggerAction(Page::_End); }
    void pageUp                (){ m_View->TriggerAction(Page::_PageUp); }
    void pageDown              (){ m_View->TriggerAction(Page::_PageDown); }

    void aboutVanilla          (){ m_View->TriggerAction(Page::_AboutVanilla); }
    void aboutQt               (){ m_View->TriggerAction(Page::_AboutQt); }
    void quit                  (){ QTimer::singleShot(0, m_View->GetTreeBank(), SLOT(Quit()));}

    void clearCookies          (){ m_View->TriggerAction(Page::_ClearCookies); }
    void clearHttpCache        (){ m_View->TriggerAction(Page::_ClearHttpCache); }
    void clearVisitedLinks     (){ m_View->TriggerAction(Page::_ClearVisitedLinks); }

    void toggleNotifier        (){ m_View->TriggerAction(Page::_ToggleNotifier); }
    void toggleReceiver        (){ m_View->TriggerAction(Page::_ToggleReceiver); }
    void toggleMenuBar         (){ m_View->TriggerAction(Page::_ToggleMenuBar); }
    void toggleTreeBar         (){ m_View->TriggerAction(Page::_ToggleTreeBar); }
    void toggleToolBar         (){ m_View->TriggerAction(Page::_ToggleToolBar); }
    void toggleFullScreen      (){ m_View->TriggerAction(Page::_ToggleFullScreen); }
    void toggleMaximized       (){ m_View->TriggerAction(Page::_ToggleMaximized); }
    void toggleMinimized       (){ m_View->TriggerAction(Page::_ToggleMinimized); }
    void toggleShaded          (){ m_View->TriggerAction(Page::_ToggleShaded); }
    void shadeWindow           (){ m_View->TriggerAction(Page::_ShadeWindow); }
    void unshadeWindow         (){ m_View->TriggerAction(Page::_UnshadeWindow);}
    void newWindow             (){ m_View->TriggerAction(Page::_NewWindow); }
    void closeWindow           (){ m_View->TriggerAction(Page::_CloseWindow); }
    void switchWindow          (){ m_View->TriggerAction(Page::_SwitchWindow); }
    void nextWindow            (){ m_View->TriggerAction(Page::_NextWindow); }
    void prevWindow            (){ m_View->TriggerAction(Page::_PrevWindow); }

    void back                  (){ m_View->TriggerAction(Page::_Back); }
    void forward               (){ m_View->TriggerAction(Page::_Forward); }
    void rewind                (){ m_View->TriggerAction(Page::_Rewind); }
    void fastForward           (){ m_View->TriggerAction(Page::_FastForward); }
    void upDirectory           (){ m_View->TriggerAction(Page::_UpDirectory); }
    void close                 (){ QTimer::singleShot(0, m_View->GetTreeBank(), SLOT(Close())); }
    void restore               (){ m_View->TriggerAction(Page::_Restore); }
    void recreate              (){ QTimer::singleShot(0, m_View->GetTreeBank(), SLOT(Recreate())); }
    void prevView              (){ m_View->TriggerAction(Page::_PrevView); }
    void nextView              (){ m_View->TriggerAction(Page::_NextView); }
    void buryView              (){ m_View->TriggerAction(Page::_BuryView); }
    void digView               (){ m_View->TriggerAction(Page::_DigView); }
    void firstView             (){ m_View->TriggerAction(Page::_FirstView); }
    void secondView            (){ m_View->TriggerAction(Page::_SecondView); }
    void thirdView             (){ m_View->TriggerAction(Page::_ThirdView); }
    void fourthView            (){ m_View->TriggerAction(Page::_FourthView); }
    void fifthView             (){ m_View->TriggerAction(Page::_FifthView); }
    void sixthView             (){ m_View->TriggerAction(Page::_SixthView); }
    void seventhView           (){ m_View->TriggerAction(Page::_SeventhView); }
    void eighthView            (){ m_View->TriggerAction(Page::_EighthView); }
    void ninthView             (){ m_View->TriggerAction(Page::_NinthView); }
    void tenthView             (){ m_View->TriggerAction(Page::_TenthView); }
    void lastView              (){ m_View->TriggerAction(Page::_LastView); }
    void newViewNode           (){ m_View->TriggerAction(Page::_NewViewNode); }
    void cloneViewNode         (){ m_View->TriggerAction(Page::_CloneViewNode); }
    void displayAccessKey      (){ m_View->TriggerAction(Page::_DisplayAccessKey); }
    void displayViewTree       (){ m_View->TriggerAction(Page::_DisplayViewTree); }
    void displayTrashTree      (){ m_View->TriggerAction(Page::_DisplayTrashTree); }
    void openTextSeeker        (){ m_View->TriggerAction(Page::_OpenTextSeeker); }
    void openQueryEditor       (){ m_View->TriggerAction(Page::_OpenQueryEditor); }
    void openUrlEditor         (){ m_View->TriggerAction(Page::_OpenUrlEditor); }
    void openCommand           (){ m_View->TriggerAction(Page::_OpenCommand); }
    void releaseHiddenView     (){ m_View->TriggerAction(Page::_ReleaseHiddenView); }
    void load                  (){ m_View->TriggerAction(Page::_Load); }

    void copy                  (){ m_View->TriggerAction(Page::_Copy); }
    void cut                   (){ m_View->TriggerAction(Page::_Cut); }
    void paste                 (){ m_View->TriggerAction(Page::_Paste); }
    void undo                  (){ m_View->TriggerAction(Page::_Undo); }
    void redo                  (){ m_View->TriggerAction(Page::_Redo); }
    void selectAll             (){ m_View->TriggerAction(Page::_SelectAll); }
    void unselect              (){ m_View->TriggerAction(Page::_Unselect); }
    void reload                (){ m_View->TriggerAction(Page::_Reload); }
    void reloadAndBypassCache  (){ m_View->TriggerAction(Page::_ReloadAndBypassCache); }
    void stop                  (){ m_View->TriggerAction(Page::_Stop); }
    void stopAndUnselect       (){ m_View->TriggerAction(Page::_StopAndUnselect); }

#define VANILLA_EDIT_ACTION(name, jsName)                       \
    void jsName(){ m_View->TriggerAction(Page::_##name); }
    FOR_EACH_EDIT_EVENTS_WITH_JS_NAME(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION

    void print                 (){ m_View->TriggerAction(Page::_Print); }
    void save                  (){ m_View->TriggerAction(Page::_Save); }
    void zoomIn                (){ m_View->TriggerAction(Page::_ZoomIn); }
    void zoomOut               (){ m_View->TriggerAction(Page::_ZoomOut); }
    void viewSource            (){ m_View->TriggerAction(Page::_ViewSource); }
    void applySource           (){ m_View->TriggerAction(Page::_ApplySource); }

    void openBookmarklet       (){ m_View->TriggerAction(Page::_OpenBookmarklet); }
    void searchWith            (){ m_View->TriggerAction(Page::_SearchWith); }
    void addSearchEngine       (){ m_View->TriggerAction(Page::_AddSearchEngine); }
    void addBookmarklet        (){ m_View->TriggerAction(Page::_AddBookmarklet); }
    void inspectElement        (){ m_View->TriggerAction(Page::_InspectElement); }

    void copyUrl               (){ m_View->TriggerAction(Page::_CopyUrl); }
    void copyTitle             (){ m_View->TriggerAction(Page::_CopyTitle); }
    void copyPageAsLink        (){ m_View->TriggerAction(Page::_CopyPageAsLink); }
    void copySelectedHtml      (){ m_View->TriggerAction(Page::_CopySelectedHtml); }
    void openWithDefault       (){ m_View->TriggerAction(Page::_OpenWithDefault); }

    void clickElement          (){ m_View->TriggerAction(Page::_ClickElement); }
    void focusElement          (){ m_View->TriggerAction(Page::_FocusElement); }
    void hoverElement          (){ m_View->TriggerAction(Page::_HoverElement); }

    void loadLink              (){ m_View->TriggerAction(Page::_LoadLink); }
    void openLink              (){ m_View->TriggerAction(Page::_OpenLink); }
    void downloadLink          (){ m_View->TriggerAction(Page::_DownloadLink); }
    void copyLinkUrl           (){ m_View->TriggerAction(Page::_CopyLinkUrl); }
    void copyLinkHtml          (){ m_View->TriggerAction(Page::_CopyLinkHtml); }
    void openLinkWithDefault   (){ m_View->TriggerAction(Page::_OpenLinkWithDefault); }

    void loadImage             (){ m_View->TriggerAction(Page::_LoadImage);}
    void openImage             (){ m_View->TriggerAction(Page::_OpenImage); }
    void downloadImage         (){ m_View->TriggerAction(Page::_DownloadImage); }
    void copyImage             (){ m_View->TriggerAction(Page::_CopyImage); }
    void copyImageUrl          (){ m_View->TriggerAction(Page::_CopyImageUrl); }
    void copyImageHtml         (){ m_View->TriggerAction(Page::_CopyImageHtml); }
    void openImageWithDefault  (){ m_View->TriggerAction(Page::_OpenImageWithDefault); }

    void loadMedia             (){ m_View->TriggerAction(Page::_LoadMedia);}
    void openMedia             (){ m_View->TriggerAction(Page::_OpenMedia); }
    void downloadMedia         (){ m_View->TriggerAction(Page::_DownloadMedia); }
    void toggleMediaControls   (){ m_View->TriggerAction(Page::_ToggleMediaControls); }
    void toggleMediaLoop       (){ m_View->TriggerAction(Page::_ToggleMediaLoop); }
    void toggleMediaPlayPause  (){ m_View->TriggerAction(Page::_ToggleMediaPlayPause); }
    void toggleMediaMute       (){ m_View->TriggerAction(Page::_ToggleMediaMute); }
    void copyMediaUrl          (){ m_View->TriggerAction(Page::_CopyMediaUrl); }
    void copyMediaHtml         (){ m_View->TriggerAction(Page::_CopyMediaHtml); }
    void openMediaWithDefault  (){ m_View->TriggerAction(Page::_OpenMediaWithDefault); }

    void openInNewViewNode                 (){ m_View->TriggerAction(Page::_OpenInNewViewNode); }
    void openInNewDirectory                (){ m_View->TriggerAction(Page::_OpenInNewDirectory); }
    void openOnRoot                        (){ m_View->TriggerAction(Page::_OpenOnRoot); }

    void openInNewViewNodeForeground       (){ m_View->TriggerAction(Page::_OpenInNewViewNodeForeground); }
    void openInNewDirectoryForeground      (){ m_View->TriggerAction(Page::_OpenInNewDirectoryForeground); }
    void openOnRootForeground              (){ m_View->TriggerAction(Page::_OpenOnRootForeground); }

    void openInNewViewNodeBackground       (){ m_View->TriggerAction(Page::_OpenInNewViewNodeBackground); }
    void openInNewDirectoryBackground      (){ m_View->TriggerAction(Page::_OpenInNewDirectoryBackground); }
    void openOnRootBackground              (){ m_View->TriggerAction(Page::_OpenOnRootBackground); }

    void openInNewViewNodeThisWindow       (){ m_View->TriggerAction(Page::_OpenInNewViewNodeThisWindow); }
    void openInNewDirectoryThisWindow      (){ m_View->TriggerAction(Page::_OpenInNewDirectoryThisWindow); }
    void openOnRootThisWindow              (){ m_View->TriggerAction(Page::_OpenOnRootThisWindow); }

    void openInNewViewNodeNewWindow        (){ m_View->TriggerAction(Page::_OpenInNewViewNodeNewWindow); }
    void openInNewDirectoryNewWindow       (){ m_View->TriggerAction(Page::_OpenInNewDirectoryNewWindow); }
    void openOnRootNewWindow               (){ m_View->TriggerAction(Page::_OpenOnRootNewWindow); }

    void openImageInNewViewNode            (){ m_View->TriggerAction(Page::_OpenImageInNewViewNode); }
    void openImageInNewDirectory           (){ m_View->TriggerAction(Page::_OpenImageInNewDirectory); }
    void openImageOnRoot                   (){ m_View->TriggerAction(Page::_OpenImageOnRoot); }

    void openImageInNewViewNodeForeground  (){ m_View->TriggerAction(Page::_OpenImageInNewViewNodeForeground); }
    void openImageInNewDirectoryForeground (){ m_View->TriggerAction(Page::_OpenImageInNewDirectoryForeground); }
    void openImageOnRootForeground         (){ m_View->TriggerAction(Page::_OpenImageOnRootForeground); }

    void openImageInNewViewNodeBackground  (){ m_View->TriggerAction(Page::_OpenImageInNewViewNodeBackground); }
    void openImageInNewDirectoryBackground (){ m_View->TriggerAction(Page::_OpenImageInNewDirectoryBackground); }
    void openImageOnRootBackground         (){ m_View->TriggerAction(Page::_OpenImageOnRootBackground); }

    void openImageInNewViewNodeThisWindow  (){ m_View->TriggerAction(Page::_OpenImageInNewViewNodeThisWindow); }
    void openImageInNewDirectoryThisWindow (){ m_View->TriggerAction(Page::_OpenImageInNewDirectoryThisWindow); }
    void openImageOnRootThisWindow         (){ m_View->TriggerAction(Page::_OpenImageOnRootThisWindow); }

    void openImageInNewViewNodeNewWindow   (){ m_View->TriggerAction(Page::_OpenImageInNewViewNodeNewWindow); }
    void openImageInNewDirectoryNewWindow  (){ m_View->TriggerAction(Page::_OpenImageInNewDirectoryNewWindow); }
    void openImageOnRootNewWindow          (){ m_View->TriggerAction(Page::_OpenImageOnRootNewWindow); }

    void openMediaInNewViewNode            (){ m_View->TriggerAction(Page::_OpenMediaInNewViewNode); }
    void openMediaInNewDirectory           (){ m_View->TriggerAction(Page::_OpenMediaInNewDirectory); }
    void openMediaOnRoot                   (){ m_View->TriggerAction(Page::_OpenMediaOnRoot); }

    void openMediaInNewViewNodeForeground  (){ m_View->TriggerAction(Page::_OpenMediaInNewViewNodeForeground); }
    void openMediaInNewDirectoryForeground (){ m_View->TriggerAction(Page::_OpenMediaInNewDirectoryForeground); }
    void openMediaOnRootForeground         (){ m_View->TriggerAction(Page::_OpenMediaOnRootForeground); }

    void openMediaInNewViewNodeBackground  (){ m_View->TriggerAction(Page::_OpenMediaInNewViewNodeBackground); }
    void openMediaInNewDirectoryBackground (){ m_View->TriggerAction(Page::_OpenMediaInNewDirectoryBackground); }
    void openMediaOnRootBackground         (){ m_View->TriggerAction(Page::_OpenMediaOnRootBackground); }

    void openMediaInNewViewNodeThisWindow  (){ m_View->TriggerAction(Page::_OpenMediaInNewViewNodeThisWindow); }
    void openMediaInNewDirectoryThisWindow (){ m_View->TriggerAction(Page::_OpenMediaInNewDirectoryThisWindow); }
    void openMediaOnRootThisWindow         (){ m_View->TriggerAction(Page::_OpenMediaOnRootThisWindow); }

    void openMediaInNewViewNodeNewWindow   (){ m_View->TriggerAction(Page::_OpenMediaInNewViewNodeNewWindow); }
    void openMediaInNewDirectoryNewWindow  (){ m_View->TriggerAction(Page::_OpenMediaInNewDirectoryNewWindow); }
    void openMediaOnRootNewWindow          (){ m_View->TriggerAction(Page::_OpenMediaOnRootNewWindow); }

    void openAllUrl    (){ m_View->TriggerAction(Page::_OpenAllUrl); }
    void openAllImage  (){ m_View->TriggerAction(Page::_OpenAllImage); }
    void openTextAsUrl (){ m_View->TriggerAction(Page::_OpenTextAsUrl); }
    void saveAllUrl    (){ m_View->TriggerAction(Page::_SaveAllUrl); }
    void saveAllImage  (){ m_View->TriggerAction(Page::_SaveAllImage); }
    void saveTextAsUrl (){ m_View->TriggerAction(Page::_SaveTextAsUrl); }
};

#endif
