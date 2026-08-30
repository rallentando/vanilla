#ifndef COMMANDMAP_HPP
#define COMMANDMAP_HPP

#include "switch.hpp"

#include <QString>
#include <QVector>

#define COMMANDMAP_FOR_EACH_SIGNAL_ACTION(F) \
    F(Up) F(Down) F(Right) F(Left) \
    F(Home) F(End) F(PageUp) F(PageDown) \
    F(Import) F(Export) F(AboutVanilla) F(AboutQt) \
    F(OpenDirectorySettings) F(OpenSettings) F(Quit) F(ToggleNotifier) \
    F(ClearCookies) F(ClearHttpCache) F(ClearVisitedLinks) \
    F(PasteAndMatchStyle) F(ToggleBold) F(ToggleItalic) F(ToggleUnderline) \
    F(ToggleStrikethrough) F(AlignLeft) F(AlignCenter) F(AlignRight) \
    F(AlignJustified) F(Indent) F(Outdent) F(InsertOrderedList) \
    F(InsertUnorderedList) F(ChangeTextDirectionLTR) F(ChangeTextDirectionRTL) \
    F(ToggleReceiver) F(ToggleMenuBar) F(ToggleTreeBar) F(ToggleToolBar) \
    F(ToggleFullScreen) F(ToggleMaximized) F(ToggleMinimized) F(ToggleShaded) \
    F(ShadeWindow) F(UnshadeWindow) F(NewWindow) F(CloseWindow) \
    F(SwitchWindow) F(NextWindow) F(PrevWindow) F(Back) \
    F(Forward) F(Rewind) F(FastForward) F(UpDirectory) \
    F(Close) F(Restore) F(Recreate) F(NextView) \
    F(PrevView) F(BuryView) F(DigView) F(FirstView) \
    F(SecondView) F(ThirdView) F(FourthView) F(FifthView) \
    F(SixthView) F(SeventhView) F(EighthView) F(NinthView) \
    F(TenthView) F(LastView) F(NewViewNode) F(CloneViewNode) \
    F(MakeLocalNode) F(DisplayAccessKey) F(DisplayViewTree) F(DisplayTrashTree) \
    F(ReleaseHiddenView) F(OpenCommand) F(Copy) F(Cut) F(Paste) \
    F(Undo) F(Redo) F(SelectAll) F(Unselect) \
    F(Reload) F(ReloadAndBypassCache) F(Stop) F(StopAndUnselect) \
    F(Print) F(Save) F(ZoomIn) F(ZoomOut) \
    F(ViewSource) F(ApplySource) F(InspectElement) F(CopyUrl) \
    F(CopyTitle) F(CopyPageAsLink) F(CopySelectedHtml) F(OpenWithDefault) \
    F(Deactivate) F(Refresh) F(RefreshNoScroll) F(OpenNode) \
    F(OpenNodeOnNewWindow) F(DeleteNode) F(DeleteRightNode) F(DeleteLeftNode) \
    F(DeleteOtherNode) F(PasteNode) F(RestoreNode) F(NewNode) \
    F(CloneNode) F(DownDirectory) F(MakeDirectory) F(MakeDirectoryWithSelectedNode) \
    F(MakeDirectoryWithSameDomainNode) F(RenameNode) F(CopyNodeUrl) F(CopyNodeTitle) \
    F(CopyNodeAsLink) F(OpenNodeWithDefault) F(ToggleTrash) F(ScrollUp) \
    F(ScrollDown) F(NextPage) F(PrevPage) F(MoveToUpperItem) \
    F(MoveToLowerItem) F(MoveToRightItem) F(MoveToLeftItem) F(MoveToPrevPage) \
    F(MoveToNextPage) F(MoveToFirstItem) F(MoveToLastItem) F(SelectToUpperItem) \
    F(SelectToLowerItem) F(SelectToRightItem) F(SelectToLeftItem) F(SelectToPrevPage) \
    F(SelectToNextPage) F(SelectToFirstItem) F(SelectToLastItem) F(SelectItem) \
    F(SelectRange) F(ClearSelection) F(TransferToUpper) F(TransferToLower) \
    F(TransferToRight) F(TransferToLeft) F(TransferToPrevPage) F(TransferToNextPage) \
    F(TransferToFirst) F(TransferToLast) F(TransferToUpDirectory) F(TransferToDownDirectory) \
    F(SwitchNodeCollectionType) F(SwitchNodeCollectionTypeReverse) F(Reconfigure)

namespace CommandMap {

enum Kind {
    NotACommand,

    Signal,
    Element,
    OpenWith,
    OpenNodeWith,
    Blank,

    Open,
    Load,
    Query,
    Download,
    Seek,
    Set,
    Unset,
    Key,
};

inline bool BeforeBookmarklet(Kind kind){ return kind >= Signal && kind <= Blank;}

struct Entry {
    const char *spelling;
    const char *action;
    Kind kind;
};

const QVector<Entry> &Entries();

Kind Resolve(const QString &word, QString *action = nullptr);

bool IsSignalAction(const QString &action);

}

#endif
