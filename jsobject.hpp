#ifndef JSOBJECT_HPP
#define JSOBJECT_HPP

#include "switch.hpp"

#include <QObject>
#include <QInputDialog>
#include <QTimer>

#include "treebank.hpp"
#include "view.hpp"
#include "dialog.hpp"

class _Vanilla : public QObject{
    Q_OBJECT

private:
    TreeBank *m_TreeBank;

public:
    _Vanilla(TreeBank *tbank) : QObject(0){
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

  //void import               (){ m_TreeBank->Import(); }
  //void export               (){ m_TreeBank->Export(); }
    void aboutVanilla         (){ m_TreeBank->AboutVanilla(); }
    void aboutQt              (){ m_TreeBank->AboutQt(); }
    void quit                 (){ QTimer::singleShot(0, m_TreeBank, SLOT(Quit()));}

    // window events?

    void back                 (){ m_TreeBank->Back(); }
    void forward              (){ m_TreeBank->Forward(); }
    void upDirectory          (){ m_TreeBank->UpDirectory(); }
    void close                (){ QTimer::singleShot(0, m_TreeBank, SLOT(Close()));}
    void restore              (){ m_TreeBank->Restore(); }
    void recreate             (){ QTimer::singleShot(0, m_TreeBank, SLOT(Recreate()));}
    void nextView             (){ m_TreeBank->NextView(); }
    void previousView         (){ m_TreeBank->PrevView(); }
    void buryView             (){ m_TreeBank->BuryView(); }
    void digView              (){ m_TreeBank->DigView(); }
    void displayHistTree      (){ m_TreeBank->DisplayHistTree(); }
    void displayViewtree      (){ m_TreeBank->DisplayViewTree(); }
    void displayAccessKey     (){ m_TreeBank->DisplayAccessKey(); }
    void openTextSeeker       (){ m_TreeBank->OpenTextSeeker(); }
    void openQueryEditor      (){ m_TreeBank->OpenQueryEditor(); }
    void openUrlEditor        (){ m_TreeBank->OpenUrlEditor(); }
    void openCommand          (){ m_TreeBank->OpenCommand(); }
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
};

class _View : public QObject{
    Q_OBJECT

private:
    View *m_View;

public:
    _View(View *view) : QObject(0){
        m_View = view;
    }

public slots:
    void up                    (){ m_View->TriggerAction(Page::Ke_Up); }
    void down                  (){ m_View->TriggerAction(Page::Ke_Down); }
    void right                 (){ m_View->TriggerAction(Page::Ke_Right); }
    void left                  (){ m_View->TriggerAction(Page::Ke_Left); }
    void home                  (){ m_View->TriggerAction(Page::Ke_Home); }
    void end                   (){ m_View->TriggerAction(Page::Ke_End); }
    void pageUp                (){ m_View->TriggerAction(Page::Ke_PageUp); }
    void pageDown              (){ m_View->TriggerAction(Page::Ke_PageDown); }

  //void import                (){ m_View->TriggerAction(Page::We_Import);}
  //void export                (){ m_View->TriggerAction(Page::We_Export);}
    void aboutVanilla          (){ m_View->TriggerAction(Page::We_AboutVanilla); }
    void aboutQt               (){ m_View->TriggerAction(Page::We_AboutQt); }
    void quit                  (){ QTimer::singleShot(0, m_View->GetTreeBank(), SLOT(Quit()));}

    void toggleNotifier        (){ m_View->TriggerAction(Page::We_ToggleNotifier); }
    void toggleReceiver        (){ m_View->TriggerAction(Page::We_ToggleReceiver); }
    void toggleMenuBar         (){ m_View->TriggerAction(Page::We_ToggleMenuBar); }
    void toggleTreeBar         (){ m_View->TriggerAction(Page::We_ToggleTreeBar); }
    void toggleToolBar         (){ m_View->TriggerAction(Page::We_ToggleToolBar); }
    void toggleFullScreen      (){ m_View->TriggerAction(Page::We_ToggleFullScreen); }
    void toggleMaximized       (){ m_View->TriggerAction(Page::We_ToggleMaximized); }
    void toggleMinimized       (){ m_View->TriggerAction(Page::We_ToggleMinimized); }
    void toggleShaded          (){ m_View->TriggerAction(Page::We_ToggleShaded); }
    void shadeWindow           (){ m_View->TriggerAction(Page::We_ShadeWindow); }
    void unshadeWindow         (){ m_View->TriggerAction(Page::We_UnshadeWindow);}
    void newWindow             (){ m_View->TriggerAction(Page::We_NewWindow); }
    void closeWindow           (){ m_View->TriggerAction(Page::We_CloseWindow); }
    void switchWindow          (){ m_View->TriggerAction(Page::We_SwitchWindow); }
    void nextWindow            (){ m_View->TriggerAction(Page::We_NextWindow); }
    void prevWindow            (){ m_View->TriggerAction(Page::We_PrevWindow); }

    void back                  (){ m_View->TriggerAction(Page::We_Back); }
    void forward               (){ m_View->TriggerAction(Page::We_Forward); }
    void upDirectory           (){ m_View->TriggerAction(Page::We_UpDirectory); }
    void close                 (){ QTimer::singleShot(0, m_View->GetTreeBank(), SLOT(Close())); }
    void restore               (){ m_View->TriggerAction(Page::We_Restore); }
    void recreate              (){ QTimer::singleShot(0, m_View->GetTreeBank(), SLOT(Recreate())); }
    void prevView              (){ m_View->TriggerAction(Page::We_PrevView); }
    void nextView              (){ m_View->TriggerAction(Page::We_NextView); }
    void buryView              (){ m_View->TriggerAction(Page::We_BuryView); }
    void digView               (){ m_View->TriggerAction(Page::We_DigView); }
    void newViewNode           (){ m_View->TriggerAction(Page::We_NewViewNode); }
    void newHistNode           (){ m_View->TriggerAction(Page::We_NewHistNode); }
    void cloneViewNode         (){ m_View->TriggerAction(Page::We_CloneViewNode); }
    void cloneHistNode         (){ m_View->TriggerAction(Page::We_CloneHistNode); }
    void displayAccessKey      (){ m_View->TriggerAction(Page::We_DisplayAccessKey); }
    void displayViewTree       (){ m_View->TriggerAction(Page::We_DisplayViewTree); }
    void displayHistTree       (){ m_View->TriggerAction(Page::We_DisplayHistTree); }
    void displayTrashTree      (){ m_View->TriggerAction(Page::We_DisplayTrashTree); }
    void openTextSeeker        (){ m_View->TriggerAction(Page::We_OpenTextSeeker); }
    void openQueryEditor       (){ m_View->TriggerAction(Page::We_OpenQueryEditor); }
    void openUrlEditor         (){ m_View->TriggerAction(Page::We_OpenUrlEditor); }
    void openCommand           (){ m_View->TriggerAction(Page::We_OpenCommand); }
    void load                  (){ m_View->TriggerAction(Page::We_Load); }

    void copy                  (){ m_View->TriggerAction(Page::We_Copy); }
    void cut                   (){ m_View->TriggerAction(Page::We_Cut); }
    void paste                 (){ m_View->TriggerAction(Page::We_Paste); }
    void undo                  (){ m_View->TriggerAction(Page::We_Undo); }
    void redo                  (){ m_View->TriggerAction(Page::We_Redo); }
    void selectAll             (){ m_View->TriggerAction(Page::We_SelectAll); }
    void unselect              (){ m_View->TriggerAction(Page::We_Unselect); }
    void reload                (){ m_View->TriggerAction(Page::We_Reload); }
    void reloadAndBypassCache  (){ m_View->TriggerAction(Page::We_ReloadAndBypassCache); }
    void stop                  (){ m_View->TriggerAction(Page::We_Stop); }
    void stopAndUnselect       (){ m_View->TriggerAction(Page::We_StopAndUnselect); }

    void print                 (){ m_View->TriggerAction(Page::We_Print); }
    void save                  (){ m_View->TriggerAction(Page::We_Save); }
    void zoomIn                (){ m_View->TriggerAction(Page::We_ZoomIn); }
    void zoomOut               (){ m_View->TriggerAction(Page::We_ZoomOut); }
    void viewSource            (){ m_View->TriggerAction(Page::We_ViewSource); }
    void applySource           (){ m_View->TriggerAction(Page::We_ApplySource); }

    void openBookmarklet       (){ m_View->TriggerAction(Page::We_OpenBookmarklet); }
    void searchWith            (){ m_View->TriggerAction(Page::We_SearchWith); }
    void addSearchEngine       (){ m_View->TriggerAction(Page::We_AddSearchEngine); }
    void addBookmarklet        (){ m_View->TriggerAction(Page::We_AddBookmarklet); }
    void inspectElement        (){ m_View->TriggerAction(Page::We_InspectElement); }

    void copyUrl               (){ m_View->TriggerAction(Page::We_CopyUrl); }
    void copyTitle             (){ m_View->TriggerAction(Page::We_CopyTitle); }
    void copyPageAsLink        (){ m_View->TriggerAction(Page::We_CopyPageAsLink); }
    void copySelectedHtml      (){ m_View->TriggerAction(Page::We_CopySelectedHtml); }
    void openWithIE            (){ m_View->TriggerAction(Page::We_OpenWithIE); }
    void openWithFF            (){ m_View->TriggerAction(Page::We_OpenWithFF); }
    void openWithOpera         (){ m_View->TriggerAction(Page::We_OpenWithOpera); }
    void openWithOPR           (){ m_View->TriggerAction(Page::We_OpenWithOPR); }
    void openWithSafari        (){ m_View->TriggerAction(Page::We_OpenWithSafari); }
    void openWithChrome        (){ m_View->TriggerAction(Page::We_OpenWithChrome); }
    void openWithSleipnir      (){ m_View->TriggerAction(Page::We_OpenWithSleipnir); }
    void openWithVivaldi       (){ m_View->TriggerAction(Page::We_OpenWithVivaldi); }
    void openWithCustom        (){ m_View->TriggerAction(Page::We_OpenWithCustom); }

    void clickElement          (){ m_View->TriggerAction(Page::We_ClickElement); }
    void focusElement          (){ m_View->TriggerAction(Page::We_FocusElement); }
    void hoverElement          (){ m_View->TriggerAction(Page::We_HoverElement); }

    void loadLink              (){ m_View->TriggerAction(Page::We_LoadLink); }
    void openLink              (){ m_View->TriggerAction(Page::We_OpenLink); }
    void downloadLink          (){ m_View->TriggerAction(Page::We_DownloadLink); }
    void copyLinkUrl           (){ m_View->TriggerAction(Page::We_CopyLinkUrl); }
    void copyLinkHtml          (){ m_View->TriggerAction(Page::We_CopyLinkHtml); }
    void openLinkWithIE        (){ m_View->TriggerAction(Page::We_OpenLinkWithIE); }
    void openLinkWithFF        (){ m_View->TriggerAction(Page::We_OpenLinkWithFF); }
    void openLinkWithOpera     (){ m_View->TriggerAction(Page::We_OpenLinkWithOpera); }
    void openLinkWithOPR       (){ m_View->TriggerAction(Page::We_OpenLinkWithOPR); }
    void openLinkWithSafari    (){ m_View->TriggerAction(Page::We_OpenLinkWithSafari); }
    void openLinkWithChrome    (){ m_View->TriggerAction(Page::We_OpenLinkWithChrome); }
    void openLinkWithSleipnir  (){ m_View->TriggerAction(Page::We_OpenLinkWithSleipnir); }
    void openLinkWithVivaldi   (){ m_View->TriggerAction(Page::We_OpenLinkWithVivaldi); }
    void openLinkWithCustom    (){ m_View->TriggerAction(Page::We_OpenLinkWithCustom); }

    void loadImage             (){ m_View->TriggerAction(Page::We_LoadImage);}
    void openImage             (){ m_View->TriggerAction(Page::We_OpenImage); }
    void downloadImage         (){ m_View->TriggerAction(Page::We_DownloadImage); }
    void copyImage             (){ m_View->TriggerAction(Page::We_CopyImage); }
    void copyImageUrl          (){ m_View->TriggerAction(Page::We_CopyImageUrl); }
    void copyImageHtml         (){ m_View->TriggerAction(Page::We_CopyImageHtml); }
    void openImageWithIE       (){ m_View->TriggerAction(Page::We_OpenImageWithIE); }
    void openImageWithFF       (){ m_View->TriggerAction(Page::We_OpenImageWithFF); }
    void openImageWithOpera    (){ m_View->TriggerAction(Page::We_OpenImageWithOpera); }
    void openImageWithOPR      (){ m_View->TriggerAction(Page::We_OpenImageWithOPR); }
    void openImageWithSafari   (){ m_View->TriggerAction(Page::We_OpenImageWithSafari); }
    void openImageWithChrome   (){ m_View->TriggerAction(Page::We_OpenImageWithChrome); }
    void openImageWithSleipnir (){ m_View->TriggerAction(Page::We_OpenImageWithSleipnir); }
    void openImageWithVivaldi  (){ m_View->TriggerAction(Page::We_OpenImageWithVivaldi); }
    void openImageWithCustom   (){ m_View->TriggerAction(Page::We_OpenImageWithCustom); }

    void openInNewViewNode                 (){ m_View->TriggerAction(Page::We_OpenInNewViewNode); }
    void openInNewHistNode                 (){ m_View->TriggerAction(Page::We_OpenInNewHistNode); }
    void openInNewDirectory                (){ m_View->TriggerAction(Page::We_OpenInNewDirectory); }
    void openOnRoot                        (){ m_View->TriggerAction(Page::We_OpenOnRoot); }

    void openInNewViewNodeForeground       (){ m_View->TriggerAction(Page::We_OpenInNewViewNodeForeground); }
    void openInNewHistNodeForeground       (){ m_View->TriggerAction(Page::We_OpenInNewHistNodeForeground); }
    void openInNewDirectoryForeground      (){ m_View->TriggerAction(Page::We_OpenInNewDirectoryForeground); }
    void openOnRootForeground              (){ m_View->TriggerAction(Page::We_OpenOnRootForeground); }

    void openInNewViewNodeBackground       (){ m_View->TriggerAction(Page::We_OpenInNewViewNodeBackground); }
    void openInNewHistNodeBackground       (){ m_View->TriggerAction(Page::We_OpenInNewHistNodeBackground); }
    void openInNewDirectoryBackground      (){ m_View->TriggerAction(Page::We_OpenInNewDirectoryBackground); }
    void openOnRootBackground              (){ m_View->TriggerAction(Page::We_OpenOnRootBackground); }

    void openInNewViewNodeThisWindow       (){ m_View->TriggerAction(Page::We_OpenInNewViewNodeThisWindow); }
    void openInNewHistNodeThisWindow       (){ m_View->TriggerAction(Page::We_OpenInNewHistNodeThisWindow); }
    void openInNewDirectoryThisWindow      (){ m_View->TriggerAction(Page::We_OpenInNewDirectoryThisWindow); }
    void openOnRootThisWindow              (){ m_View->TriggerAction(Page::We_OpenOnRootThisWindow); }

    void openInNewViewNodeNewWindow        (){ m_View->TriggerAction(Page::We_OpenInNewViewNodeNewWindow); }
    void openInNewHistNodeNewWindow        (){ m_View->TriggerAction(Page::We_OpenInNewHistNodeNewWindow); }
    void openInNewDirectoryNewWindow       (){ m_View->TriggerAction(Page::We_OpenInNewDirectoryNewWindow); }
    void openOnRootNewWindow               (){ m_View->TriggerAction(Page::We_OpenOnRootNewWindow); }

    void openImageInNewViewNode            (){ m_View->TriggerAction(Page::We_OpenImageInNewViewNode); }
    void openImageInNewHistNode            (){ m_View->TriggerAction(Page::We_OpenImageInNewHistNode); }
    void openImageInNewDirectory           (){ m_View->TriggerAction(Page::We_OpenImageInNewDirectory); }
    void openImageOnRoot                   (){ m_View->TriggerAction(Page::We_OpenImageOnRoot); }

    void openImageInNewViewNodeForeground  (){ m_View->TriggerAction(Page::We_OpenImageInNewViewNodeForeground); }
    void openImageInNewHistNodeForeground  (){ m_View->TriggerAction(Page::We_OpenImageInNewHistNodeForeground); }
    void openImageInNewDirectoryForeground (){ m_View->TriggerAction(Page::We_OpenImageInNewDirectoryForeground); }
    void openImageOnRootForeground         (){ m_View->TriggerAction(Page::We_OpenImageOnRootForeground); }

    void openImageInNewViewNodeBackground  (){ m_View->TriggerAction(Page::We_OpenImageInNewViewNodeBackground); }
    void openImageInNewHistNodeBackground  (){ m_View->TriggerAction(Page::We_OpenImageInNewHistNodeBackground); }
    void openImageInNewDirectoryBackground (){ m_View->TriggerAction(Page::We_OpenImageInNewDirectoryBackground); }
    void openImageOnRootBackground         (){ m_View->TriggerAction(Page::We_OpenImageOnRootBackground); }

    void openImageInNewViewNodeThisWindow  (){ m_View->TriggerAction(Page::We_OpenImageInNewViewNodeThisWindow); }
    void openImageInNewHistNodeThisWindow  (){ m_View->TriggerAction(Page::We_OpenImageInNewHistNodeThisWindow); }
    void openImageInNewDirectoryThisWindow (){ m_View->TriggerAction(Page::We_OpenImageInNewDirectoryThisWindow); }
    void openImageOnRootThisWindow         (){ m_View->TriggerAction(Page::We_OpenImageOnRootThisWindow); }

    void openImageInNewViewNodeNewWindow   (){ m_View->TriggerAction(Page::We_OpenImageInNewViewNodeNewWindow); }
    void openImageInNewHistNodeNewWindow   (){ m_View->TriggerAction(Page::We_OpenImageInNewHistNodeNewWindow); }
    void openImageInNewDirectoryNewWindow  (){ m_View->TriggerAction(Page::We_OpenImageInNewDirectoryNewWindow); }
    void openImageOnRootNewWindow          (){ m_View->TriggerAction(Page::We_OpenImageOnRootNewWindow); }

    void openAllUrl    (){ m_View->TriggerAction(Page::We_OpenAllUrl); }
    void openAllImage  (){ m_View->TriggerAction(Page::We_OpenAllImage); }
    void openTextAsUrl (){ m_View->TriggerAction(Page::We_OpenTextAsUrl); }
    void saveAllUrl    (){ m_View->TriggerAction(Page::We_SaveAllUrl); }
    void saveAllImage  (){ m_View->TriggerAction(Page::We_SaveAllImage); }
    void saveTextAsUrl (){ m_View->TriggerAction(Page::We_SaveTextAsUrl); }
};

#endif
