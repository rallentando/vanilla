#ifndef ACTIONMAPPER_HPP
#define ACTIONMAPPER_HPP

#define FOR_EACH_KEYBOARD_EVENTS(F) \
    F(Up) F(Down) F(Right) F(Left) F(Home) F(End) F(PageUp) F(PageDown)

#define FOR_EACH_APPLICATION_EVENTS(F)          \
    F(Import)                                   \
    F(Export)                                   \
    F(AboutVanilla)                             \
    F(AboutQt)                                  \
    F(OpenSettings)                             \
    F(OpenDirectorySettings)                    \
    F(Quit)                                     \
    F(ClearCookies)                             \
    F(ClearHttpCache)                           \
    F(ClearVisitedLinks)                        \
    F(ToggleNotifier)                           \
    F(ToggleReceiver)                           \
    F(ToggleMenuBar)                            \
    F(ToggleTreeBar)                            \
    F(ToggleToolBar)                            \
    F(ToggleFullScreen)                         \
    F(ToggleMaximized)                          \
    F(ToggleMinimized)                          \
    F(ToggleShaded)                             \
    F(ShadeWindow)                              \
    F(UnshadeWindow)                            \
    F(NewWindow)                                \
    F(CloseWindow)                              \
    F(SwitchWindow)                             \
    F(NextWindow)                               \
    F(PrevWindow)

#define FOR_EACH_NAVIGATION_EVENTS(F) \
    F(Back) F(Forward) F(Rewind) F(FastForward) F(UpDirectory) F(Load)

#define FOR_EACH_VIEW_EVENTS(F)                 \
    F(Close)                                    \
    F(Restore)                                  \
    F(Recreate)                                 \
    F(NextView)                                 \
    F(PrevView)                                 \
    F(BuryView)                                 \
    F(DigView)                                  \
    F(FirstView)                                \
    F(SecondView)                               \
    F(ThirdView)                                \
    F(FourthView)                               \
    F(FifthView)                                \
    F(SixthView)                                \
    F(SeventhView)                              \
    F(EighthView)                               \
    F(NinthView)                                \
    F(TenthView)                                \
    F(LastView)                                 \
    F(NewViewNode)                              \
    F(CloneViewNode)                            \
    F(DisplayAccessKey)                         \
    F(DisplayViewTree)                          \
    F(DisplayTrashTree)                         \
    F(OpenTextSeeker)                           \
    F(OpenQueryEditor)                          \
    F(OpenUrlEditor)                            \
    F(OpenCommand)                              \
    F(ReleaseHiddenView)

#define FOR_EACH_EDIT_EVENTS(F)                 \
    F(PasteAndMatchStyle)                       \
    F(ToggleBold)                               \
    F(ToggleItalic)                             \
    F(ToggleUnderline)                          \
    F(ToggleStrikethrough)                      \
    F(AlignLeft)                                \
    F(AlignCenter)                              \
    F(AlignRight)                               \
    F(AlignJustified)                           \
    F(Indent)                                   \
    F(Outdent)                                  \
    F(InsertOrderedList)                        \
    F(InsertUnorderedList)                      \
    F(ChangeTextDirectionLTR)                   \
    F(ChangeTextDirectionRTL)

#define FOR_EACH_EDIT_EVENTS_WITH_JS_NAME(F)                    \
    F(PasteAndMatchStyle,     pasteAndMatchStyle)               \
    F(ToggleBold,             toggleBold)                       \
    F(ToggleItalic,           toggleItalic)                     \
    F(ToggleUnderline,        toggleUnderline)                  \
    F(ToggleStrikethrough,    toggleStrikethrough)              \
    F(AlignLeft,              alignLeft)                        \
    F(AlignCenter,            alignCenter)                      \
    F(AlignRight,             alignRight)                       \
    F(AlignJustified,         alignJustified)                   \
    F(Indent,                 indent)                           \
    F(Outdent,                outdent)                          \
    F(InsertOrderedList,      insertOrderedList)                \
    F(InsertUnorderedList,    insertUnorderedList)              \
    F(ChangeTextDirectionLTR, changeTextDirectionLTR)           \
    F(ChangeTextDirectionRTL, changeTextDirectionRTL)

#define FOR_EACH_VANILLA_JS_METHOD(F)               \
    F(Repaint, repaint)                              \
    F(Reconfigure, reconfigure)                      \
    F(Up, up) F(Down, down)                          \
    F(Right, right) F(Left, left)                    \
    F(PageUp, pageUp) F(PageDown, pageDown)          \
    F(Home, home) F(End, end)                        \
    F(AboutVanilla, aboutVanilla)                    \
    F(AboutQt, aboutQt)                              \
    F(Back, back) F(Forward, forward)                \
    F(Rewind, rewind) F(FastForward, fastForward)    \
    F(UpDirectory, upDirectory)                      \
    F(Restore, restore)                              \
    F(NextView, nextView)                            \
    F(PrevView, previousView)                        \
    F(BuryView, buryView) F(DigView, digView)        \
    F(FirstView, firstView)                          \
    F(SecondView, secondView)                        \
    F(ThirdView, thirdView)                          \
    F(FourthView, fourthView)                        \
    F(FifthView, fifthView)                          \
    F(SixthView, sixthView)                          \
    F(SeventhView, seventhView)                      \
    F(EighthView, eighthView)                        \
    F(NinthView, ninthView)                          \
    F(TenthView, tenthView)                          \
    F(LastView, lastView)                            \
    F(DisplayViewTree, displayViewtree)              \
    F(DisplayAccessKey, displayAccessKey)            \
    F(OpenTextSeeker, openTextSeeker)                \
    F(OpenQueryEditor, openQueryEditor)              \
    F(OpenUrlEditor, openUrlEditor)                  \
    F(OpenCommand, openCommand)                      \
    F(ReleaseHiddenView, releaseHiddenView)          \
    F(Load, load)                                    \
    F(Copy, copy) F(Cut, cut) F(Paste, paste)        \
    F(Undo, undo) F(Redo, redo)                      \
    F(SelectAll, selectAll) F(Unselect, unselect)    \
    F(Reload, reload)                                \
    F(ReloadAndBypassCache, reloadAndBypassCache)    \
    F(Stop, stop) F(StopAndUnselect, stopAndUnselect) \
    F(Print, print) F(Save, save)                    \
    F(ClearCookies, clearCookies)                    \
    F(ClearHttpCache, clearHttpCache)                \
    F(ClearVisitedLinks, clearVisitedLinks)

#define FOR_EACH_VIEW_JS_ACTION(F)                                  \
    F(Up, up) F(Down, down) F(Right, right) F(Left, left)            \
    F(Home, home) F(End, end) F(PageUp, pageUp) F(PageDown, pageDown) \
    F(AboutVanilla, aboutVanilla) F(AboutQt, aboutQt)                \
    F(ClearCookies, clearCookies) F(ClearHttpCache, clearHttpCache) \
    F(ClearVisitedLinks, clearVisitedLinks)                         \
    F(ToggleNotifier, toggleNotifier) F(ToggleReceiver, toggleReceiver) \
    F(ToggleMenuBar, toggleMenuBar) F(ToggleTreeBar, toggleTreeBar) \
    F(ToggleToolBar, toggleToolBar) F(ToggleFullScreen, toggleFullScreen) \
    F(ToggleMaximized, toggleMaximized) F(ToggleMinimized, toggleMinimized) \
    F(ToggleShaded, toggleShaded) F(ShadeWindow, shadeWindow)        \
    F(UnshadeWindow, unshadeWindow) F(NewWindow, newWindow)          \
    F(CloseWindow, closeWindow) F(SwitchWindow, switchWindow)        \
    F(NextWindow, nextWindow) F(PrevWindow, prevWindow)              \
    F(Back, back) F(Forward, forward) F(Rewind, rewind)              \
    F(FastForward, fastForward) F(UpDirectory, upDirectory)          \
    F(Restore, restore) F(PrevView, prevView) F(NextView, nextView)  \
    F(BuryView, buryView) F(DigView, digView)                        \
    F(FirstView, firstView) F(SecondView, secondView)                \
    F(ThirdView, thirdView) F(FourthView, fourthView)                \
    F(FifthView, fifthView) F(SixthView, sixthView)                  \
    F(SeventhView, seventhView) F(EighthView, eighthView)            \
    F(NinthView, ninthView) F(TenthView, tenthView) F(LastView, lastView) \
    F(NewViewNode, newViewNode) F(CloneViewNode, cloneViewNode)      \
    F(DisplayAccessKey, displayAccessKey) F(DisplayViewTree, displayViewTree) \
    F(DisplayTrashTree, displayTrashTree)                            \
    F(OpenTextSeeker, openTextSeeker) F(OpenQueryEditor, openQueryEditor) \
    F(OpenUrlEditor, openUrlEditor) F(OpenCommand, openCommand)      \
    F(ReleaseHiddenView, releaseHiddenView) F(Load, load)            \
    FOR_EACH_EDIT_EVENTS_WITH_JS_NAME(F)                             \
    F(Copy, copy) F(Cut, cut) F(Paste, paste)                        \
    F(Undo, undo) F(Redo, redo) F(SelectAll, selectAll)              \
    F(Unselect, unselect) F(Reload, reload)                          \
    F(ReloadAndBypassCache, reloadAndBypassCache)                    \
    F(Stop, stop) F(StopAndUnselect, stopAndUnselect)                \
    F(Print, print) F(Save, save) F(ZoomIn, zoomIn) F(ZoomOut, zoomOut) \
    F(ViewSource, viewSource) F(ApplySource, applySource)            \
    F(OpenBookmarklet, openBookmarklet) F(SearchWith, searchWith)    \
    F(AddSearchEngine, addSearchEngine) F(AddBookmarklet, addBookmarklet) \
    F(InspectElement, inspectElement)                                \
    F(CopyUrl, copyUrl) F(CopyTitle, copyTitle)                      \
    F(CopyPageAsLink, copyPageAsLink) F(CopySelectedHtml, copySelectedHtml) \
    F(OpenWithDefault, openWithDefault)                              \
    F(ClickElement, clickElement) F(FocusElement, focusElement)      \
    F(HoverElement, hoverElement)                                   \
    F(LoadLink, loadLink) F(OpenLink, openLink)                      \
    F(DownloadLink, downloadLink) F(CopyLinkUrl, copyLinkUrl)        \
    F(CopyLinkHtml, copyLinkHtml) F(OpenLinkWithDefault, openLinkWithDefault) \
    F(LoadImage, loadImage) F(OpenImage, openImage)                  \
    F(DownloadImage, downloadImage) F(CopyImage, copyImage)          \
    F(CopyImageUrl, copyImageUrl) F(CopyImageHtml, copyImageHtml)    \
    F(OpenImageWithDefault, openImageWithDefault)                    \
    F(LoadMedia, loadMedia) F(OpenMedia, openMedia)                  \
    F(DownloadMedia, downloadMedia)                                 \
    F(ToggleMediaControls, toggleMediaControls)                      \
    F(ToggleMediaLoop, toggleMediaLoop)                              \
    F(ToggleMediaPlayPause, toggleMediaPlayPause)                    \
    F(ToggleMediaMute, toggleMediaMute)                              \
    F(CopyMediaUrl, copyMediaUrl) F(CopyMediaHtml, copyMediaHtml)    \
    F(OpenMediaWithDefault, openMediaWithDefault)                    \
    F(OpenInNewViewNode, openInNewViewNode)                          \
    F(OpenInNewDirectory, openInNewDirectory) F(OpenOnRoot, openOnRoot) \
    F(OpenInNewViewNodeForeground, openInNewViewNodeForeground)      \
    F(OpenInNewDirectoryForeground, openInNewDirectoryForeground)    \
    F(OpenOnRootForeground, openOnRootForeground)                    \
    F(OpenInNewViewNodeBackground, openInNewViewNodeBackground)      \
    F(OpenInNewDirectoryBackground, openInNewDirectoryBackground)    \
    F(OpenOnRootBackground, openOnRootBackground)                    \
    F(OpenInNewViewNodeThisWindow, openInNewViewNodeThisWindow)      \
    F(OpenInNewDirectoryThisWindow, openInNewDirectoryThisWindow)    \
    F(OpenOnRootThisWindow, openOnRootThisWindow)                    \
    F(OpenInNewViewNodeNewWindow, openInNewViewNodeNewWindow)        \
    F(OpenInNewDirectoryNewWindow, openInNewDirectoryNewWindow)      \
    F(OpenOnRootNewWindow, openOnRootNewWindow)                      \
    F(OpenImageInNewViewNode, openImageInNewViewNode)                \
    F(OpenImageInNewDirectory, openImageInNewDirectory)              \
    F(OpenImageOnRoot, openImageOnRoot)                              \
    F(OpenImageInNewViewNodeForeground, openImageInNewViewNodeForeground) \
    F(OpenImageInNewDirectoryForeground, openImageInNewDirectoryForeground) \
    F(OpenImageOnRootForeground, openImageOnRootForeground)          \
    F(OpenImageInNewViewNodeBackground, openImageInNewViewNodeBackground) \
    F(OpenImageInNewDirectoryBackground, openImageInNewDirectoryBackground) \
    F(OpenImageOnRootBackground, openImageOnRootBackground)          \
    F(OpenImageInNewViewNodeThisWindow, openImageInNewViewNodeThisWindow) \
    F(OpenImageInNewDirectoryThisWindow, openImageInNewDirectoryThisWindow) \
    F(OpenImageOnRootThisWindow, openImageOnRootThisWindow)          \
    F(OpenImageInNewViewNodeNewWindow, openImageInNewViewNodeNewWindow) \
    F(OpenImageInNewDirectoryNewWindow, openImageInNewDirectoryNewWindow) \
    F(OpenImageOnRootNewWindow, openImageOnRootNewWindow)            \
    F(OpenMediaInNewViewNode, openMediaInNewViewNode)                \
    F(OpenMediaInNewDirectory, openMediaInNewDirectory)              \
    F(OpenMediaOnRoot, openMediaOnRoot)                              \
    F(OpenMediaInNewViewNodeForeground, openMediaInNewViewNodeForeground) \
    F(OpenMediaInNewDirectoryForeground, openMediaInNewDirectoryForeground) \
    F(OpenMediaOnRootForeground, openMediaOnRootForeground)          \
    F(OpenMediaInNewViewNodeBackground, openMediaInNewViewNodeBackground) \
    F(OpenMediaInNewDirectoryBackground, openMediaInNewDirectoryBackground) \
    F(OpenMediaOnRootBackground, openMediaOnRootBackground)          \
    F(OpenMediaInNewViewNodeThisWindow, openMediaInNewViewNodeThisWindow) \
    F(OpenMediaInNewDirectoryThisWindow, openMediaInNewDirectoryThisWindow) \
    F(OpenMediaOnRootThisWindow, openMediaOnRootThisWindow)          \
    F(OpenMediaInNewViewNodeNewWindow, openMediaInNewViewNodeNewWindow) \
    F(OpenMediaInNewDirectoryNewWindow, openMediaInNewDirectoryNewWindow) \
    F(OpenMediaOnRootNewWindow, openMediaOnRootNewWindow)            \
    F(OpenAllUrl, openAllUrl) F(OpenAllImage, openAllImage)          \
    F(OpenTextAsUrl, openTextAsUrl) F(SaveAllUrl, saveAllUrl)        \
    F(SaveAllImage, saveAllImage) F(SaveTextAsUrl, saveTextAsUrl)

#define FOR_EACH_WEB_EVENTS1(F)                 \
    FOR_EACH_EDIT_EVENTS(F)                     \
    F(Copy)                                     \
    F(Cut)                                      \
    F(Paste)                                    \
    F(Undo)                                     \
    F(Redo)                                     \
    F(SelectAll)                                \
    F(Unselect)                                 \
    F(Reload)                                   \
    F(ReloadAndBypassCache)                     \
    F(Stop)                                     \
    F(StopAndUnselect)                          \
    F(Print)                                    \
    F(Save)                                     \
    F(ZoomIn)                                   \
    F(ZoomOut)                                  \
    F(ViewSource)                               \
    F(ApplySource)                              \
    F(InspectElement)                           \
    F(CopyUrl)                                  \
    F(CopyTitle)                                \
    F(CopyPageAsLink)                           \
    F(CopySelectedHtml)                         \
    F(OpenWithDefault)

#define FOR_EACH_WEB_EVENTS2(F)                 \
    F(OpenBookmarklet)                          \
    F(SearchWith)                               \
    F(AddSearchEngine)                          \
    F(AddBookmarklet)                           \
    F(ClickElement)                             \
    F(FocusElement)                             \
    F(HoverElement)                             \
    F(LoadLink)                                 \
    F(OpenLink)                                 \
    F(DownloadLink)                             \
    F(CopyLinkUrl)                              \
    F(CopyLinkHtml)                             \
    F(OpenLinkWithDefault)                      \
    F(LoadImage)                                \
    F(OpenImage)                                \
    F(DownloadImage)                            \
    F(CopyImage)                                \
    F(CopyImageUrl)                             \
    F(CopyImageHtml)                            \
    F(OpenImageWithDefault)                     \
    F(LoadMedia)                                \
    F(OpenMedia)                                \
    F(DownloadMedia)                            \
    F(ToggleMediaControls)                      \
    F(ToggleMediaLoop)                          \
    F(ToggleMediaPlayPause)                     \
    F(ToggleMediaMute)                          \
    F(CopyMediaUrl)                             \
    F(CopyMediaHtml)                            \
    F(OpenMediaWithDefault)                     \
    F(OpenInNewViewNode)                        \
    F(OpenInNewDirectory)                       \
    F(OpenOnRoot)                               \
    F(OpenInNewViewNodeForeground)              \
    F(OpenInNewDirectoryForeground)             \
    F(OpenOnRootForeground)                     \
    F(OpenInNewViewNodeBackground)              \
    F(OpenInNewDirectoryBackground)             \
    F(OpenOnRootBackground)                     \
    F(OpenInNewViewNodeThisWindow)              \
    F(OpenInNewDirectoryThisWindow)             \
    F(OpenOnRootThisWindow)                     \
    F(OpenInNewViewNodeNewWindow)               \
    F(OpenInNewDirectoryNewWindow)              \
    F(OpenOnRootNewWindow)                      \
    F(OpenImageInNewViewNode)                   \
    F(OpenImageInNewDirectory)                  \
    F(OpenImageOnRoot)                          \
    F(OpenImageInNewViewNodeForeground)         \
    F(OpenImageInNewDirectoryForeground)        \
    F(OpenImageOnRootForeground)                \
    F(OpenImageInNewViewNodeBackground)         \
    F(OpenImageInNewDirectoryBackground)        \
    F(OpenImageOnRootBackground)                \
    F(OpenImageInNewViewNodeThisWindow)         \
    F(OpenImageInNewDirectoryThisWindow)        \
    F(OpenImageOnRootThisWindow)                \
    F(OpenImageInNewViewNodeNewWindow)          \
    F(OpenImageInNewDirectoryNewWindow)         \
    F(OpenImageOnRootNewWindow)                 \
    F(OpenMediaInNewViewNode)                   \
    F(OpenMediaInNewDirectory)                  \
    F(OpenMediaOnRoot)                          \
    F(OpenMediaInNewViewNodeForeground)         \
    F(OpenMediaInNewDirectoryForeground)        \
    F(OpenMediaOnRootForeground)                \
    F(OpenMediaInNewViewNodeBackground)         \
    F(OpenMediaInNewDirectoryBackground)        \
    F(OpenMediaOnRootBackground)                \
    F(OpenMediaInNewViewNodeThisWindow)         \
    F(OpenMediaInNewDirectoryThisWindow)        \
    F(OpenMediaOnRootThisWindow)                \
    F(OpenMediaInNewViewNodeNewWindow)          \
    F(OpenMediaInNewDirectoryNewWindow)         \
    F(OpenMediaOnRootNewWindow)                 \
    F(OpenAllUrl)                               \
    F(OpenAllImage)                             \
    F(OpenTextAsUrl)                            \
    F(SaveAllUrl)                               \
    F(SaveAllImage)                             \
    F(SaveTextAsUrl)

#define FOR_EACH_GADGETS_EVENTS(F)              \
    F(Deactivate)                               \
    F(Refresh)                                  \
    F(RefreshNoScroll)                          \
    F(OpenNode)                                 \
    F(OpenNodeOnNewWindow)                      \
    F(DeleteNode)                               \
    F(DeleteRightNode)                          \
    F(DeleteLeftNode)                           \
    F(DeleteOtherNode)                          \
    F(PasteNode)                                \
    F(RestoreNode)                              \
    F(NewNode)                                  \
    F(CloneNode)                                \
    F(UpDirectory)                              \
    F(DownDirectory)                            \
    F(MakeLocalNode)                            \
    F(MakeDirectory)                            \
    F(MakeDirectoryWithSelectedNode)            \
    F(MakeDirectoryWithSameDomainNode)          \
    F(RenameNode)                               \
    F(CopyNodeUrl)                              \
    F(CopyNodeTitle)                            \
    F(CopyNodeAsLink)                           \
    F(OpenNodeWithDefault)                      \
    F(ToggleTrash)                              \
    F(ScrollUp)                                 \
    F(ScrollDown)                               \
    F(NextPage)                                 \
    F(PrevPage)                                 \
    F(ZoomIn)                                   \
    F(ZoomOut)                                  \
    F(MoveToUpperItem)                          \
    F(MoveToLowerItem)                          \
    F(MoveToRightItem)                          \
    F(MoveToLeftItem)                           \
    F(MoveToPrevPage)                           \
    F(MoveToNextPage)                           \
    F(MoveToFirstItem)                          \
    F(MoveToLastItem)                           \
    F(SelectToUpperItem)                        \
    F(SelectToLowerItem)                        \
    F(SelectToRightItem)                        \
    F(SelectToLeftItem)                         \
    F(SelectToPrevPage)                         \
    F(SelectToNextPage)                         \
    F(SelectToFirstItem)                        \
    F(SelectToLastItem)                         \
    F(SelectItem)                               \
    F(SelectRange)                              \
    F(SelectAll)                                \
    F(ClearSelection)                           \
    F(TransferToUpper)                          \
    F(TransferToLower)                          \
    F(TransferToRight)                          \
    F(TransferToLeft)                           \
    F(TransferToPrevPage)                       \
    F(TransferToNextPage)                       \
    F(TransferToFirst)                          \
    F(TransferToLast)                           \
    F(TransferToUpDirectory)                    \
    F(TransferToDownDirectory)                  \
    F(SwitchNodeCollectionType)                 \
    F(SwitchNodeCollectionTypeReverse)

#define PAGE_FOR_EACH_ACTION(F)     \
    F(NoAction)                     \
    FOR_EACH_KEYBOARD_EVENTS(F)     \
    FOR_EACH_APPLICATION_EVENTS(F)  \
    FOR_EACH_NAVIGATION_EVENTS(F)   \
    FOR_EACH_VIEW_EVENTS(F)         \
    FOR_EACH_WEB_EVENTS1(F)         \
    FOR_EACH_WEB_EVENTS2(F)

#define TREEBANK_FOR_EACH_ACTION(F) \
    F(NoAction)                     \
    FOR_EACH_KEYBOARD_EVENTS(F)     \
    FOR_EACH_APPLICATION_EVENTS(F)  \
    FOR_EACH_NAVIGATION_EVENTS(F)   \
    FOR_EACH_VIEW_EVENTS(F)         \
    FOR_EACH_WEB_EVENTS1(F)

#define GADGETS_FOR_EACH_ACTION(F)  \
    F(NoAction)                     \
    FOR_EACH_KEYBOARD_EVENTS(F)     \
    FOR_EACH_APPLICATION_EVENTS(F)  \
    FOR_EACH_VIEW_EVENTS(F)         \
    FOR_EACH_GADGETS_EVENTS(F)

#define ENUMERATE_ACTION(ACTION) _##ACTION,

#define STRING_TO_ACTION(ACTION) \
    if(str == QStringLiteral(#ACTION)) return _##ACTION;

#define ACTION_TO_STRING(ACTION) \
    if(action == _##ACTION) return QStringLiteral(#ACTION);

#define INSTALL_ACTION_MAP(CLASS, ENUM)                             \
    public:                                                         \
    enum ENUM {                                                     \
        CLASS##_FOR_EACH_ACTION(ENUMERATE_ACTION)                   \
    };                                                              \
    static inline ENUM StringToAction(QString str){                 \
        CLASS##_FOR_EACH_ACTION(STRING_TO_ACTION)                   \
        return _NoAction;                                           \
    }                                                               \
    static inline QString ActionToString(ENUM action){              \
        CLASS##_FOR_EACH_ACTION(ACTION_TO_STRING)                   \
        return QStringLiteral("NoAction");                          \
    }                                                               \
    static inline bool IsValidAction(QString str){                  \
        return str == ActionToString(StringToAction(str));          \
    }                                                               \
    static inline bool IsValidAction(ENUM action){                  \
        return action == StringToAction(ActionToString(action));    \
    }

#endif
