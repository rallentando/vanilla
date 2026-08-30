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
