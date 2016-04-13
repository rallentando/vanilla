#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include "switch.hpp"

#include <QWidget>
#include <QLineEdit>

#include "view.hpp"

class QHideEvent;
class QShowEvent;
class QKeyEvent;
class QKeySequence;
class QFocusEvent;
class QPaintEvent;
class QTimerEvent;
class QLocalServer;

class TreeBank;
class View;

class LineEdit : public QLineEdit{
    Q_OBJECT

public:
    LineEdit(QWidget *parent = 0);
    ~LineEdit();

protected:
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void inputMethodEvent(QInputMethodEvent *ev) DECL_OVERRIDE;

signals:
    void Returned();
    void Aborted();
    void FocusIn();
    void FocusOut();
    void SelectNextSuggest();
    void SelectPrevSuggest();
};

class Receiver : public QWidget{
    Q_OBJECT

public:
    Receiver(TreeBank *parent = 0, bool purge = false);
    ~Receiver();
    bool IsPurged() const;
    void Purge();
    void Join();
    void ResizeNotify(QSize size);
    void RepaintIfNeed(const QRect &rect);
    void OpenTextSeeker(View* = 0);
    void OpenQueryEditor(View* = 0);
    void OpenUrlEditor(View* = 0);
    void OpenCommand(View* = 0);

public slots:
    void OnReturned();
    void OnAborted();
    void ForeignCommandReceived();
    void SetString(QString str);
    void SetSuggest(QStringList list);
    void SuitableAction();
    void EditingFinished();
    void ReceiveCommand(QString cmd);

    void SelectNextSuggest();
    void SelectPrevSuggest();

private:
    enum Mode {
        Command,
        Query,
        UrlEdit,
        Search,
    } m_Mode;

    void InitializeDisplay(Mode mode);
    QString WaitForStringInput();

    TreeBank *m_TreeBank;

    static QLocalServer *m_LocalServer;
    LineEdit *m_LineEdit;
    QString m_LineString;
    QStringList m_SuggestStrings;
    int m_CurrentSuggestIndex;

protected:
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE;
    void timerEvent(QTimerEvent *ev) DECL_OVERRIDE;
    void paintEvent(QPaintEvent *ev) DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;

signals:
    void FocusIn();
    void FocusOut();

    // key action;
    void Up();
    void Down();
    void Right();
    void Left();
    void Home();
    void End();
    void PageUp();
    void PageDown();

    // application action;
    void Import();
    void Export();
    void AboutVanilla();
    void AboutQt();
    void Quit();

    // window action;
    void ToggleNotifier();
    void ToggleReceiver();
    void ToggleMenuBar();
    void ToggleTreeBar();
    void ToggleToolBar();
    void ToggleFullScreen();
    void ToggleMaximized();
    void ToggleMinimized();
    void ToggleShaded();
    void ShadeWindow();
    void UnshadeWindow();
    void NewWindow();
    void CloseWindow();
    void SwitchWindow();
    void NextWindow();
    void PrevWindow();

    // treebank action;
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
    void NewViewNode();
    void NewHistNode();
    void CloneViewNode();
    void CloneHistNode();
    void MakeLocalNode();
    void DisplayAccessKey();
    void DisplayViewTree();
    void DisplayHistTree();
    void DisplayTrashTree();
    void ReleaseHiddenView();

    // web action;
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

    // element action;
    void TriggerElementAction(Page::CustomAction);

    // gadgets action;
    void Deactivate();
    void Refresh();
    void RefreshNoScroll();
    void OpenNode();
    void OpenNodeOnNewWindow();
    void DeleteNode();
    void DeleteRightNode();
    void DeleteLeftNode();
    void DeleteOtherNode();
    void PasteNode();
    void RestoreNode();
    void NewNode();
    void CloneNode();
  //void UpDirectory();
    void DownDirectory();
  //void MakeLocalNode();
    void MakeDirectory();
    void MakeDirectoryWithSelectedNode();
    void MakeDirectoryWithSameDomainNode();
    void RenameNode();
    void CopyNodeUrl();
    void CopyNodeTitle();
    void CopyNodeAsLink();
    void OpenNodeWithIE();
    void OpenNodeWithEdge();
    void OpenNodeWithFF();
    void OpenNodeWithOpera();
    void OpenNodeWithOPR();
    void OpenNodeWithSafari();
    void OpenNodeWithChrome();
    void OpenNodeWithSleipnir();
    void OpenNodeWithVivaldi();
    void OpenNodeWithCustom();
    void ToggleTrash();
    void ScrollUp();
    void ScrollDown();
    void NextPage();
    void PrevPage();
  //void ZoomIn();
  //void ZoomOut();
    void MoveToUpperItem();
    void MoveToLowerItem();
    void MoveToRightItem();
    void MoveToLeftItem();
    void MoveToPrevPage();
    void MoveToNextPage();
    void MoveToFirstItem();
    void MoveToLastItem();
    void SelectToUpperItem();
    void SelectToLowerItem();
    void SelectToRightItem();
    void SelectToLeftItem();
    void SelectToPrevPage();
    void SelectToNextPage();
    void SelectToFirstItem();
    void SelectToLastItem();
    void SelectItem();
    void SelectRange();
  //void SelectAll();
    void ClearSelection();
    void TransferToUpper();
    void TransferToLower();
    void TransferToRight();
    void TransferToLeft();
    void TransferToPrevPage();
    void TransferToNextPage();
    void TransferToFirst();
    void TransferToLast();
    void TransferToUpDirectory();
    void TransferToDownDirectory();
    void SwitchNodeCollectionType();
    void SwitchNodeCollectionTypeReverse();

    void Reconfigure();

    void OpenUrl(QUrl);
    void OpenUrl(QList<QUrl>);
    void OpenQueryUrl(QString);
    void OpenBookmarklet(const QString&);
    void SearchWith(QString, QString);

    void Download(QString, QString);
    void SeekText(const QString&, View::FindFlags);
    void KeyEvent(QString);

public slots:
    void DisplaySuggest(const QByteArray&);
signals:
    void SuggestRequest(const QUrl&);
};
#endif //ifndef RECEIVER_HPP
