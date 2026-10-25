#include "switch.hpp"

#include "settingsschema.hpp"

#include <QCoreApplication>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>

#include "application.hpp"

namespace {

    const SettingsSchema::Item ITEMS[] = {

        { "application/@ColorScheme", SettingsSchema::Choice, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Colour scheme"),
          QT_TRANSLATE_NOOP("SettingsSchema", "'Auto' follows the desktop."),
          "Auto|Light|Dark", "Auto", false },

        { "gadgets/@Style", SettingsSchema::Choice, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tab list style"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Flat' is a grid of cards, 'Glass' a translucent overlay."),
          "FlatStyle|GlassStyle", "FlatStyle", false },

        { "application/@EnableMiniMap", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show the minimap"),
          QT_TRANSLATE_NOOP("SettingsSchema",
                            "A strip beside the page with a reduced model of "
                            "the whole page. A scroll on it acts at the "
                            "middle of the page."),
          "", "false", true },

        { "application/@EnableTranslucentPanels", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Translucent menus, panels and dialogs"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Blurs what is behind them where the system can (Windows 10 1803 and later)."),
          "", "false", false },

        { "application/@EnableTransparentWindow", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Translucent window"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Shows what is behind the window, blurred, through it; web pages stay opaque (Windows)."),
          "", "false", true,
          "application/@EnableMainWindowRhi=false|application/@GraphicsApi=OpenGL" },

        { "application/@EnableFramelessWindow", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Draw the window frame in the application"),
          "", "", "false", true },

        { "application/@PurgeView", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Give the page its own window"),
          "", "", "false", true },

        { "application/@PurgeNotifier", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Give the status area its own window"),
          "", "", "false", true },

        { "application/@PurgeReceiver", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Give the command line its own window"),
          "", "", "false", true },

        { "treebar/@EnableCloseButton", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Close button on a tab"), "", "", "true", false },

        { "treebar/@EnableCloneButton", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Duplicate button on a tab"), "", "", "true", false },

        { "treebar/@EnableAnimation", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Animate the tab bar"), "", "", "true", false },

        { "treebar/@ScrollToSwitchNode", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Wheel over the bar switches tab"),
          "", "", "false", false },

        { "treebar/@WheelClickToClose", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Middle click closes a tab"), "", "", "true", false },

        { "treebar/@EnableFrameRate", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show the frame rate on the bar"),
          "", "", "false", false },

        { "application/@AddChildViewNodePosition", SettingsSchema::Choice, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where a tab is inserted in a directory"), "",
          "RightEnd|LeftEnd|RightOfPrimary|LeftOfPrimary|"
          "TailOfRightUnreadsOfPrimary|HeadOfLeftUnreadsOfPrimary",
          "RightEnd", false },

        { "application/@AddSiblingViewNodePosition", SettingsSchema::Choice, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where a sibling tab is inserted"), "",
          "RightEnd|LeftEnd|RightOfPrimary|LeftOfPrimary|"
          "TailOfRightUnreadsOfPrimary|HeadOfLeftUnreadsOfPrimary",
          "RightOfPrimary", false },

        { "application/@TraverseAllView", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Next and previous walk the whole tree"),
          "", "", "false", false },

        { "application/@MaxViewCount", SettingsSchema::Int, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Pages kept loaded at once"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Tabs beyond this keep their place in the tree but drop the page. "
              "0 means no limit."),
          "", "10", false },

        { "webview/@SuspendHiddenViews", SettingsSchema::Choice, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Hidden tabs"),
#ifdef EDGEWEBVIEW
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Freeze' and 'Discard' do nothing while a tab is loading, playing "
              "sound, or sharing its process.\n"
              "An Edge view treats 'Discard' as 'Freeze'."),
#else
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Freeze' and 'Discard' do nothing while a tab is loading, playing "
              "sound, or sharing its process."),
#endif
          "Active|Frozen|Discarded", "Active", false },

        { "application/@EnableAutoLoad", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load tabs ahead in the background"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Only tabs whose directory setting 'Auto load' is on."),
          "", "true", false, "webview/@SuspendHiddenViews=Active" },

        { "application/@AutoLoadInterval", SettingsSchema::Int, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load one tab every (ms)"), "", "", "1000", false,
          "application/@EnableAutoLoad=true" },

        { "application/@MaxTrashEntryCount", SettingsSchema::Int, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Entries kept in the wastebasket"),
          "", "", "500", false },

        { "gadgets/thumblist/@NodeCollectionType", SettingsSchema::Choice, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Display type"),
          "", "Flat|Recursive|Foldable", "Flat", false },

        { "gadgets/thumblist/@EnableCloseButton", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Close button on a thumbnail"),
          "", "", "true", true },

        { "gadgets/thumblist/@EnableCloneButton", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Duplicate button on a thumbnail"),
          "", "", "true", true },

        { "gadgets/thumblist/@EnableAnimation", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Animate the tab list"), "", "", "true", false },

        { "gadgets/thumblist/@EnableBackdropBlur", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Blur the page behind the tab list"),
          "", "", "true", false },

        { "gadgets/thumblist/@EnableInPlaceNotifier", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show a panel of dates and URLs"),
          "", "", "true", true, "gadgets/@Style=GlassStyle" },

        { "gadgets/thumblist/@EnableFrameRate", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show the frame rate on the tab list"),
          "", "", "false", false },

        { "gadgets/thumblist/@EnableHoveredSpotLight", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Join a hovered thumbnail to its title"),
          "", "", "true", true, "gadgets/@Style=GlassStyle" },

        { "gadgets/thumblist/@EnablePrimarySpotLight", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Join the current thumbnail to its title"),
          "", "", "false", true, "gadgets/@Style=GlassStyle" },

        { "gadgets/thumblist/@EnableLoadedSpotLight", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Join every loaded thumbnail to its title"),
          "", "", "false", false, "gadgets/@Style=GlassStyle" },

        { "gadgets/thumblist/@RightClickToRenameNode", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Right click renames"),
          "", "", "false", false, "gadgets/@Style=GlassStyle" },

        { "gadgets/thumblist/@ScrollToChangeDirectory", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Wheel changes directory"),
          "", "", "false", false, "gadgets/@Style=GlassStyle" },

        { "webview/@OpenCommandOperation", SettingsSchema::Choice, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where a link opens"), "",
          "InNewViewNode|InNewDirectory|OnRoot|InNewViewNodeBackground|"
          "InNewDirectoryBackground|OnRootBackground",
          "InNewViewNode", false },

        { "webview/@EnableDestinationInferrer", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Back and forward guess the page when there is no history"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "From a 'previous' or 'next' link in the page, or a number in "
              "the address. On, the rewind and fast forward buttons are hidden."),
          "", "false", false },

        { "webview/preferences/BackForwardCacheEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Keep the previous page in memory"),
          "", "", "false", false },

        { "webview/@ActivateNewViewDefault", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Switch to a newly opened tab"),
          "", "", "true", false },

        { "webview/preferences/NavigateOnDropEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Dropping a link on a page opens it there"),
          "", "", "true", false },

        { "webview/@DragToStartDownload", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Dragging a link out downloads it"),
          "", "", "false", false },

        { "webview/preferences/SpatialNavigationEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Arrow keys move between links"),
          "", "", "false", false },

        { "webview/preferences/LinksIncludedInFocusChain", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tab key stops on links"), "", "", "true", false },

        { "webview/preferences/FocusOnNavigationEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Focus the page when it navigates"),
          "", "", "false", false },

        { "network/@SpellCheckLanguages", SettingsSchema::TextList, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Spell checking dictionaries"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One name per line, as the '.bdic' file next to the executable "
              "is called without its extension ('en-US'). Empty turns spell "
              "checking off."),
          "", "", true },

        { "webview/preferences/ShowScrollBars", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show scroll bars"), "", "", "true", false },

        { "webview/preferences/ScrollAnimatorEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Smooth scrolling"), "", "", "true", false },

        { "webview/preferences/ForceDarkMode", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Darken pages which have no dark theme"),
          "", "", "false", false },

        { "webview/preferences/FullScreenSupportEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Allow a page to go full screen"),
          "", "", "true", false },

        { "webview/preferences/PlaybackRequiresUserGesture", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Media needs a click before it plays"),
          "", "", "true", false },

        { "webview/preferences/AutoLoadImages", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load images"), "", "", "true", false },

        { "webview/preferences/AutoLoadIconsForPage", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load favicons"), "", "", "true", false },

        { "webview/preferences/TouchIconsEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load touch icons"), "", "", "false", false },

        { "webview/preferences/PdfViewerEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show PDFs in the page"), "", "", "true", false },

        { "webview/@InspectorInMainWindow", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Open the inspector beside the page"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Takes effect the next time the inspector is opened."),
          "", "true", false },

        { "webview/font/StandardFont", SettingsSchema::Text, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Standard font"), "", "", "", false },

        { "webview/font/FixedFont", SettingsSchema::Text, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Fixed width font"), "", "", "", false },

        { "webview/font/DefaultFontSize", SettingsSchema::Int, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Default font size"), "", "", "16", false },

        { "webview/font/DefaultFixedFontSize", SettingsSchema::Int, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Default fixed width font size"),
          "", "", "13", false },

        { "webview/font/MinimumFontSize", SettingsSchema::Int, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Minimum font size"), "", "", "0", false },

        { "webview/preferences/JavascriptEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "JavaScript"), "", "", "true", false },

        { "webview/preferences/LocalStorageEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Local storage"), "", "", "true", false },

        { "webview/preferences/PluginsEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Plugins"), "", "", "true", false },

        { "webview/preferences/WebGLEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "WebGL"), "", "", "true", false },

        { "webview/preferences/ReadingFromCanvasEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Let a page read back its own canvas"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Reading a canvas back is one of the ways a site fingerprints "
              "the machine. Off, pages which use a canvas do not work."),
          "", "true", false },

        { "webview/preferences/ScreenCaptureEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Allow a page to capture the screen"),
          "", "", "false", false },

        { "webview/preferences/TouchEventsApiEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tell pages this machine has a touch screen"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Until this is changed, the engine answers based on whether a "
              "touch screen was found."),
          "", "false", false },

        { "webview/preferences/TrimAccessibilityIdentifiers", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Leave element IDs out of the accessibility tree"),
          "", "", "false", false },

        { "webview/@SavePageFormat", SettingsSchema::Choice, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "How a page is saved"),
          "", "MimeHtmlSaveFormat|CompleteHtmlSaveFormat|SingleHtmlSaveFormat",
          "MimeHtmlSaveFormat", false },

        { "webview/preferences/PrintElementBackgrounds", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Print backgrounds"), "", "", "true", false },

        { "webview/preferences/PrintHeaderAndFooter", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Print the page title and address in the margin"),
          "", "", "false", false },

        { "webview/preferences/PreferCSSMarginsForPrinting", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Let the page choose the printed margins"),
          "", "", "false", false },

        { "gadgets/accesskey/@EnableMultiStroke", SettingsSchema::Bool, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "Allow multi-stroke labels"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Every link gets a label of its own, typed one key at a time, "
              "instead of blocks of one key each."),
          "", "true", false },

        { "gadgets/accesskey/@AccessKeyMode", SettingsSchema::Choice, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "Which keys label the links"), "",
          "BothHands|LeftHand|RightHand|Custom", "BothHands", false },

        { "gadgets/accesskey/@AccessKeyCustomSequence", SettingsSchema::Text, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "Custom key sequence"),
          "", "", "", false },

        { "gadgets/accesskey/@AccessKeyAction", SettingsSchema::Choice, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "What choosing a link does"), "",
          "OpenMenu|ClickElement|FocusElement|HoverElement|OpenInNewViewNode|"
          "OpenInNewDirectory|OpenOnRoot|OpenInNewViewNodeBackground|"
          "OpenInNewDirectoryBackground",
          "OpenMenu", false },

        { "gadgets/accesskey/@AccessKeySelectBlockMethod", SettingsSchema::Choice, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "How a block of links is chosen"), "",
          "Number|ShiftedChar|CtrledChar|AltedChar|MetaChar", "Number", false,
          "gadgets/accesskey/@EnableMultiStroke=false" },

        { "gadgets/accesskey/@AccessKeySortOrientation", SettingsSchema::Choice, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "Order the links are labelled in"), "",
          "Vertical|Horizontal", "Vertical", false },

        { "webview/@EnableSingleKeyShortcut", SettingsSchema::Bool, "input",
          QT_TRANSLATE_NOOP("SettingsSchema", "Shortcuts on a single key"),
          "", "", "false", true },

        { "webview/@EnableMouseGesture", SettingsSchema::Bool, "input",
          QT_TRANSLATE_NOOP("SettingsSchema", "Mouse gestures"), "", "", "true", false },

        { "webview/@EnableDragGesture", SettingsSchema::Bool, "input",
          QT_TRANSLATE_NOOP("SettingsSchema", "Super drag"), "", "", "false", false },

        { "application/@DownloadPolicy", SettingsSchema::Choice, "downloads",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where downloads go"), "",
          "Undefined|FixedLocale|DownloadFolder|AskForEachDownload",
          "Undefined", false },

        { "application/@FileSaveDirectory", SettingsSchema::Directory, "downloads",
          QT_TRANSLATE_NOOP("SettingsSchema", "Download folder"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Replaced by the folder a file was last saved to."),
          "", "", false, "", "folder" },

        { "application/@FileOpenDirectory", SettingsSchema::Directory, "downloads",
          QT_TRANSLATE_NOOP("SettingsSchema", "Folder a file picker opens in"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Replaced by the folder a file was last chosen from."),
          "", "", false, "", "folder" },

        { "localview/@MediaVolume", SettingsSchema::Int, "files",
          QT_TRANSLATE_NOOP("SettingsSchema", "Volume"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "0 to 100."),
          "", "50", false },

        { "localview/@AutoPlayMedia", SettingsSchema::Bool, "files",
          QT_TRANSLATE_NOOP("SettingsSchema", "Start playing when a file is opened"),
          "", "", "true", false },

        { "application/@ExternalCommands", SettingsSchema::TextList, "commands",
          QT_TRANSLATE_NOOP("SettingsSchema", "Programs to open a page with"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Each line adds an item to the right-click menu 'Open with "
              "other browser', as 'name = command line'. The page's address "
              "goes where '%u' is, or at the end when there is none.\n"
              "For example: Firefox = \"C:\\Program Files\\Mozilla Firefox\\firefox.exe\" %u"),
          "", "", false },

        { "network/@Extensions", SettingsSchema::TextList, "extensions",
          QT_TRANSLATE_NOOP("SettingsSchema", "Chrome extensions"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One unpacked extension per line: the folder its 'manifest.json' "
              "sits in. A line starting with '#' is ignored. Manifest version 3 "
              "only.\n"
              "Not in private profiles. Reload open pages after a change."),
          "", "", false, "", "chrome-extension" },

        { "network/@ExtensionShims", SettingsSchema::Bool, "extensions",
          QT_TRANSLATE_NOOP("SettingsSchema", "Extension compatibility layer (WebEngineView / Edge)"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "WebEngine views leave out parts of the extension platform. On, "
              "each extension is loaded as a copy with a script added that fills "
              "them in; the folder you registered is not changed.\n"
              "An extension the layer cannot help is loaded as it is, and its row "
              "says why."),
          "", "true", true },

        { "network/@ExtensionTabScope", SettingsSchema::Choice, "extensions",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tabs shown to extensions"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Which tabs an extension's tab list, tab events and tab "
              "actions cover. Bookmarks and history still cover the whole "
              "tree.\n"
              "Content scripts still run on every page they match, so this "
              "does not hide pages from an extension."),
          "All|CurrentDirectory|Loaded", "All", false },

        { "network/@UnloadHangoutsExtension", SettingsSchema::Bool, "extensions",
          QT_TRANSLATE_NOOP("SettingsSchema", "Remove the built-in Google Hangouts extension"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "The engine ships an extension which only Google's own sites "
              "can use. Off, it is kept, which Google Meet may want for "
              "sharing a screen."),
          "", "true", true },

        { "application/@SaveSessionCookie", SettingsSchema::Bool, "sitedata",
          QT_TRANSLATE_NOOP("SettingsSchema", "Keep session cookies between launches"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Cookies meant to last only until the browser closes, such as a "
              "login, are kept too."),
          "", "false", true },

        { "network/@RememberPermissions", SettingsSchema::Bool, "sitedata",
          QT_TRANSLATE_NOOP("SettingsSchema", "Remember what a site was allowed"),
          "", "", "true", true },

#if defined(EDGEWEBVIEW) || defined(VANILLA_PASSWORDS)
        { "webview/@SavePasswords", SettingsSchema::Bool, "sitedata",
          QT_TRANSLATE_NOOP("SettingsSchema", "Offer to save passwords"),
#  if defined(EDGEWEBVIEW) && defined(VANILLA_PASSWORDS) && defined(VANILLA_QUICK_PASSWORDS)
          QT_TRANSLATE_NOOP("SettingsSchema",
              "An Edge view fills it in the next time. In a WebEngine view (Quick "
              "too), click the field and choose the account."),
#  elif defined(EDGEWEBVIEW) && defined(VANILLA_PASSWORDS)
          QT_TRANSLATE_NOOP("SettingsSchema",
              "An Edge view fills it in the next time. In a WebEngine view, click "
              "the field and choose the account.\n"
              "Quick views do not save passwords."),
#  elif defined(EDGEWEBVIEW)
          QT_TRANSLATE_NOOP("SettingsSchema",
              "An Edge view fills it in the next time. Other views do not save "
              "passwords."),
#  elif defined(VANILLA_QUICK_PASSWORDS)
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Click the field and choose the account to fill it in."),
#  else
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Click the field and choose the account to fill it in.\n"
              "Quick views do not save passwords."),
#  endif
          "", "true", false },
#endif

        { "network/@EnablePushService", SettingsSchema::Bool, "sitedata",
          QT_TRANSLATE_NOOP("SettingsSchema", "Let a site send push messages"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "A site allowed to send notifications can then reach this "
              "machine while its page is not open."),
          "", "false", true },

        { "network/@HttpCacheType", SettingsSchema::Choice, "sitedata",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where the page cache is kept"),
          QT_TRANSLATE_NOOP("SettingsSchema", "A private tab always uses memory."),
          "DiskHttpCache|MemoryHttpCache|NoCache", "DiskHttpCache", true },

        { "network/@HttpCacheMaximumSize", SettingsSchema::Int, "sitedata",
          QT_TRANSLATE_NOOP("SettingsSchema", "Cache size limit (MB)"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Zero lets the engine decide."),
          "", "0", true },

        { "network/@BlockedUrlPatterns", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Requests never made"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One wildcard per line, matched against the whole address "
              "('*doubleclick.net*'). A line starting with '#' is ignored. "
              "A page itself is never blocked, only what it loads."),
          "", "", false },

        { "application/@AcceptLanguage", SettingsSchema::Text, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Accept-Language"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Empty means the languages of this machine followed by English, "
              "as 'ja-JP,ja;q=0.9,en-US;q=0.8,en;q=0.7'."),
          "", "", false },

        { "network/@SendDoNotTrack", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Ask not to be tracked"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Adds the 'DNT' and 'Sec-GPC' headers."),
          "", "false", false },

#ifdef EDGEWEBVIEW
        { "webview/@TrackingPrevention", SettingsSchema::Choice, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tracking prevention"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Edge views only. Strict may break parts of some sites."),
          "None|Basic|Balanced|Strict", "Balanced", false },

        { "webview/@SmartScreen", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Microsoft Defender SmartScreen"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Edge views only. Sites and downloads are checked against "
              "Microsoft's list of unsafe ones."),
          "", "true", false },
#endif

        { "application/@EnableGoogleSuggest", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Suggestions from Google while typing a URL"),
          "", "", "false", false },

        { "application/@SslErrorPolicy", SettingsSchema::Choice, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "On a certificate error"), "",
          "Undefined|BlockAccess|IgnoreSslErrors|AskForEachAccess|"
          "AskForEachHost|AskForEachCertificate",
          "Undefined", false },

        { "application/@AllowedHosts", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Hosts whose certificate errors are accepted"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One per line. Answering its question adds a line."),
          "", "", false, "application/@SslErrorPolicy=AskForEachHost" },

        { "application/@BlockedHosts", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Hosts whose certificate errors are refused"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One per line. Answering its question adds a line."),
          "", "", false, "application/@SslErrorPolicy=AskForEachHost" },

        { "application/@AllowedCertificates", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Certificates always allowed"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One host:SHA-256 identity per line. Remove one to ask again."),
          "", "", false, "application/@SslErrorPolicy=AskForEachCertificate" },

        { "application/@BlockedCertificates", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Certificates always blocked"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One host:SHA-256 identity per line. Remove one to ask again."),
          "", "", false, "application/@SslErrorPolicy=AskForEachCertificate" },

        { "network/@SecureDnsMode", SettingsSchema::Choice, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "DNS over HTTPS"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "The two secure modes use the servers below."),
          "SystemOnly|SecureWithFallback|SecureOnly", "SystemOnly", true },

        { "network/@SecureDnsServers", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "DNS over HTTPS servers"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One URI template per line, for example "
              "'https://dns.google/dns-query{?dns}'."),
          "", "", true,
          "network/@SecureDnsMode=SecureWithFallback|network/@SecureDnsMode=SecureOnly" },

        { "application/@EnableAutoSave", SettingsSchema::Bool, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Save while running"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "At the interval below, so that a crash loses less. Everything is "
              "saved on quitting either way."),
          "", "true", false },

        { "application/@AutoSaveInterval", SettingsSchema::Int, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Save every (ms)"), "", "", "300000", false,
          "application/@EnableAutoSave=true" },

        { "application/@MaxBackUpGenerationCount", SettingsSchema::Int, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Backups kept"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Made at each launch, next to the saved files in the data folder. "
              "Older copies than this many are deleted."),
          "", "5", false },

        { "application/@GraphicsApi", SettingsSchema::Choice, "engine",
          QT_TRANSLATE_NOOP("SettingsSchema", "Graphics API (WebEngineView)"),
#if defined(Q_OS_WIN)
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'OpenGL': keeps the translucent window after a WebEngineView tab "
              "opens.\n"
              "'Auto': Direct3D 11. Choose this if the screen stays blank or is "
              "slow with OpenGL.\n"
              "\n"
              "A choice this OS cannot run is treated as 'Auto'. The environment "
              "variable QSG_RHI_BACKEND wins when it is set."),
#elif defined(Q_OS_MACOS)
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Auto': Metal.\n"
              "'OpenGL': choose this if the screen flickers.\n"
              "\n"
              "A choice this OS cannot run is treated as 'Auto'. The "
              "environment variable QSG_RHI_BACKEND wins when it is set."),
#else
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Auto': OpenGL.\n"
              "\n"
              "A choice this OS cannot run is treated as 'Auto'. The "
              "environment variable QSG_RHI_BACKEND wins when it is set."),
#endif
          "Auto|Software|OpenGL|Direct3D11|Direct3D12|Vulkan|Metal",
#if defined(Q_OS_WIN)
          "OpenGL",
#else
          "Auto",
#endif
          true },

        { "application/@EnableMainWindowRhi", SettingsSchema::Bool, "engine",
          QT_TRANSLATE_NOOP("SettingsSchema", "Draw the main window with the graphics API too"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "For using Edge and WebEngineView tabs side by side: keeps the "
              "main window from flickering once the first time a "
              "WebEngineView tab is switched to.\n"
              "While on, tools which take GPU-drawn windows for games (such "
              "as the GeForce Experience overlay) may react to this "
              "application. Not needed when the graphics API is 'Software'."),
          "", "false", true },

        { "application/@Viewport", SettingsSchema::Choice, "engine",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tab list drawing surface"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "What the tab list and the access key labels are drawn on; the "
              "page is not. 'GLWidget' and 'OpenGLWidget' are the same.\n"
              "Change it only if they are drawn wrongly."),
          "Widget|GLWidget|OpenGLWidget", "Widget", true },

        { "application/@ChromiumFlags", SettingsSchema::TextList, "engine",
          QT_TRANSLATE_NOOP("SettingsSchema", "Chromium switches"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Passed to the rendering engine at startup. Separated by spaces "
              "or line breaks, each beginning with '--'.\n"
              "Put double quotes around a value which has spaces in it: "
              "--switch=\"a b\".\n"
              "A switch it does not accept can stop it from starting; to "
              "recover, edit 'data/config.json'."),
          "", "", true },
    };

    const SettingsSchema::Group GROUPS[] = {
        { "treebar/@EnableCloseButton",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tab bar") },
        { "application/@AddChildViewNodePosition",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tree") },
        { "application/@MaxViewCount",
          QT_TRANSLATE_NOOP("SettingsSchema", "Loading and limits") },

        { "webview/@OpenCommandOperation",
          QT_TRANSLATE_NOOP("SettingsSchema", "Links and new tabs") },
        { "webview/preferences/SpatialNavigationEnabled",
          QT_TRANSLATE_NOOP("SettingsSchema", "Keys and typing") },
        { "webview/preferences/ShowScrollBars",
          QT_TRANSLATE_NOOP("SettingsSchema", "Display") },
        { "webview/font/StandardFont",
          QT_TRANSLATE_NOOP("SettingsSchema", "Fonts") },
        { "webview/preferences/JavascriptEnabled",
          QT_TRANSLATE_NOOP("SettingsSchema", "What a page may do") },
        { "webview/@SavePageFormat",
          QT_TRANSLATE_NOOP("SettingsSchema", "Saving and printing") },

        { "webview/@EnableSingleKeyShortcut",
          QT_TRANSLATE_NOOP("SettingsSchema", "Gestures and single keys") },
        { "application/@DownloadPolicy",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where files go") },

        { "network/@BlockedUrlPatterns",
          QT_TRANSLATE_NOOP("SettingsSchema", "Requests") },
        { "application/@SslErrorPolicy",
          QT_TRANSLATE_NOOP("SettingsSchema", "Certificates") },
        { "network/@SecureDnsMode",
          QT_TRANSLATE_NOOP("SettingsSchema", "DNS") },
    };

    QList<SettingsSchema::Item> BuildItems(){
        QList<SettingsSchema::Item> list;
        const int count = sizeof(ITEMS) / sizeof(ITEMS[0]);
        list.reserve(count);
        for(int i = 0; i < count; i++) list << ITEMS[i];
        return list;
    }
}

namespace SettingsSchema {

const QList<Item> &Items(){
    static const QList<Item> items = BuildItems();
    return items;
}

const QList<Group> &Groups(){
    static const QList<Group> groups = [](){
        QList<Group> list;
        const int count = sizeof(GROUPS) / sizeof(GROUPS[0]);
        list.reserve(count);
        for(int i = 0; i < count; i++) list << GROUPS[i];
        return list;
    }();
    return groups;
}

const Item *Find(const QString &key){
    const QList<Item> &items = Items();
    for(int i = 0; i < items.length(); i++){
        if(QLatin1String(items[i].key) == key) return &items[i];
    }
    return nullptr;
}

QList<QPair<QString, QString> > Categories(){
    QList<QPair<QString, QString> > list;
    list << qMakePair(QStringLiteral("appearance"),
                      QCoreApplication::translate("SettingsSchema", "Appearance"));
    list << qMakePair(QStringLiteral("tabs"),
                      QCoreApplication::translate("SettingsSchema", "Tabs"));
    list << qMakePair(QStringLiteral("gadgets"),
                      QCoreApplication::translate("SettingsSchema", "Tab list"));
    list << qMakePair(QStringLiteral("page"),
                      QCoreApplication::translate("SettingsSchema", "Pages"));
#ifdef WEBENGINEVIEW
    list << qMakePair(QStringLiteral("directory"),
                      QCoreApplication::translate("SettingsSchema", "Directory"));
#endif
    list << qMakePair(QStringLiteral("accesskey"),
                      QCoreApplication::translate("SettingsSchema", "Access keys"));
    list << qMakePair(QStringLiteral("input"),
                      QCoreApplication::translate("SettingsSchema", "Keys and mouse"));
    list << qMakePair(QStringLiteral("downloads"),
                      QCoreApplication::translate("SettingsSchema", "Downloads"));
    list << qMakePair(QStringLiteral("files"),
                      QCoreApplication::translate("SettingsSchema", "File browser"));
    list << qMakePair(QStringLiteral("commands"),
                      QCoreApplication::translate("SettingsSchema", "Open in other programs"));
    list << qMakePair(QStringLiteral("extensions"),
                      QCoreApplication::translate("SettingsSchema", "Extensions"));
    list << qMakePair(QStringLiteral("sitedata"),
                      QCoreApplication::translate("SettingsSchema", "Site data"));
#if defined(EDGEWEBVIEW) || defined(VANILLA_PASSWORDS)
    list << qMakePair(QStringLiteral("passwords"),
                      QCoreApplication::translate("SettingsSchema", "Saved passwords"));
#endif
    list << qMakePair(QStringLiteral("permissions"),
                      QCoreApplication::translate("SettingsSchema", "Site permissions"));
    list << qMakePair(QStringLiteral("network"),
                      QCoreApplication::translate("SettingsSchema", "Network"));
    list << qMakePair(QStringLiteral("session"),
                      QCoreApplication::translate("SettingsSchema", "Saving and backups"));
    list << qMakePair(QStringLiteral("engine"),
                      QCoreApplication::translate("SettingsSchema", "Rendering and engine"));
    return list;
}

QStringList CategoriesBeginningARun(){
    return QStringList()
        << QStringLiteral("accesskey")
        << QStringLiteral("downloads")
        << QStringLiteral("extensions")
        << QStringLiteral("session");
}

const QList<ChoiceLabel> &ChoiceLabels(){
    static const QList<ChoiceLabel> table = {
        { "application/@ColorScheme", "Auto",  QT_TRANSLATE_NOOP("SettingsSchema", "Auto") },
        { "application/@ColorScheme", "Light", QT_TRANSLATE_NOOP("SettingsSchema", "Light") },
        { "application/@ColorScheme", "Dark",  QT_TRANSLATE_NOOP("SettingsSchema", "Dark") },

        { "network/@ExtensionTabScope", "All",              QT_TRANSLATE_NOOP("SettingsSchema", "All tabs") },
        { "network/@ExtensionTabScope", "CurrentDirectory", QT_TRANSLATE_NOOP("SettingsSchema", "Tabs in the current tab's directory") },
        { "network/@ExtensionTabScope", "Loaded",           QT_TRANSLATE_NOOP("SettingsSchema", "Loaded tabs") },

        { "gadgets/@Style", "FlatStyle",  QT_TRANSLATE_NOOP("SettingsSchema", "Flat") },
        { "gadgets/@Style", "GlassStyle", QT_TRANSLATE_NOOP("SettingsSchema", "Glass") },

        { "application/@Viewport", "Widget",       QT_TRANSLATE_NOOP("SettingsSchema", "Widget") },
        { "application/@Viewport", "GLWidget",     QT_TRANSLATE_NOOP("SettingsSchema", "GLWidget") },
        { "application/@Viewport", "OpenGLWidget", QT_TRANSLATE_NOOP("SettingsSchema", "OpenGLWidget") },

        { "application/@AddChildViewNodePosition", "RightEnd",
          QT_TRANSLATE_NOOP("SettingsSchema", "At the right end") },
        { "application/@AddChildViewNodePosition", "LeftEnd",
          QT_TRANSLATE_NOOP("SettingsSchema", "At the left end") },
        { "application/@AddChildViewNodePosition", "RightOfPrimary",
          QT_TRANSLATE_NOOP("SettingsSchema", "Right of the current tab") },
        { "application/@AddChildViewNodePosition", "LeftOfPrimary",
          QT_TRANSLATE_NOOP("SettingsSchema", "Left of the current tab") },
        { "application/@AddChildViewNodePosition", "TailOfRightUnreadsOfPrimary",
          QT_TRANSLATE_NOOP("SettingsSchema", "After the unread tabs to the right of the current one") },
        { "application/@AddChildViewNodePosition", "HeadOfLeftUnreadsOfPrimary",
          QT_TRANSLATE_NOOP("SettingsSchema", "Before the unread tabs to the left of the current one") },

        { "application/@AddSiblingViewNodePosition", "RightEnd",
          QT_TRANSLATE_NOOP("SettingsSchema", "At the right end") },
        { "application/@AddSiblingViewNodePosition", "LeftEnd",
          QT_TRANSLATE_NOOP("SettingsSchema", "At the left end") },
        { "application/@AddSiblingViewNodePosition", "RightOfPrimary",
          QT_TRANSLATE_NOOP("SettingsSchema", "Right of the current tab") },
        { "application/@AddSiblingViewNodePosition", "LeftOfPrimary",
          QT_TRANSLATE_NOOP("SettingsSchema", "Left of the current tab") },
        { "application/@AddSiblingViewNodePosition", "TailOfRightUnreadsOfPrimary",
          QT_TRANSLATE_NOOP("SettingsSchema", "After the unread tabs to the right of the current one") },
        { "application/@AddSiblingViewNodePosition", "HeadOfLeftUnreadsOfPrimary",
          QT_TRANSLATE_NOOP("SettingsSchema", "Before the unread tabs to the left of the current one") },

        { "webview/@SuspendHiddenViews", "Active",    QT_TRANSLATE_NOOP("SettingsSchema", "Keep running") },
        { "webview/@SuspendHiddenViews", "Frozen",    QT_TRANSLATE_NOOP("SettingsSchema", "Freeze") },
        { "webview/@SuspendHiddenViews", "Discarded", QT_TRANSLATE_NOOP("SettingsSchema", "Discard") },

        { "gadgets/thumblist/@NodeCollectionType", "Flat",
          QT_TRANSLATE_NOOP("SettingsSchema", "Normal") },
        { "gadgets/thumblist/@NodeCollectionType", "Recursive",
          QT_TRANSLATE_NOOP("SettingsSchema", "Recursive") },
        { "gadgets/thumblist/@NodeCollectionType", "Foldable",
          QT_TRANSLATE_NOOP("SettingsSchema", "Expandable") },

        { "webview/@OpenCommandOperation", "InNewViewNode",
          QT_TRANSLATE_NOOP("SettingsSchema", "In a new tab") },
        { "webview/@OpenCommandOperation", "InNewDirectory",
          QT_TRANSLATE_NOOP("SettingsSchema", "In a new directory") },
        { "webview/@OpenCommandOperation", "OnRoot",
          QT_TRANSLATE_NOOP("SettingsSchema", "At the top level") },
        { "webview/@OpenCommandOperation", "InNewViewNodeBackground",
          QT_TRANSLATE_NOOP("SettingsSchema", "In a new tab, in the background") },
        { "webview/@OpenCommandOperation", "InNewDirectoryBackground",
          QT_TRANSLATE_NOOP("SettingsSchema", "In a new directory, in the background") },
        { "webview/@OpenCommandOperation", "OnRootBackground",
          QT_TRANSLATE_NOOP("SettingsSchema", "At the top level, in the background") },

        { "webview/@SavePageFormat", "MimeHtmlSaveFormat",
          QT_TRANSLATE_NOOP("SettingsSchema", "One .mhtml file") },
        { "webview/@SavePageFormat", "CompleteHtmlSaveFormat",
          QT_TRANSLATE_NOOP("SettingsSchema", ".html and a folder of its files") },
        { "webview/@SavePageFormat", "SingleHtmlSaveFormat",
          QT_TRANSLATE_NOOP("SettingsSchema", ".html only") },

        { "gadgets/accesskey/@AccessKeyMode", "BothHands", QT_TRANSLATE_NOOP("SettingsSchema", "Both hands") },
        { "gadgets/accesskey/@AccessKeyMode", "LeftHand",  QT_TRANSLATE_NOOP("SettingsSchema", "Left hand") },
        { "gadgets/accesskey/@AccessKeyMode", "RightHand", QT_TRANSLATE_NOOP("SettingsSchema", "Right hand") },
        { "gadgets/accesskey/@AccessKeyMode", "Custom",    QT_TRANSLATE_NOOP("SettingsSchema", "Custom") },

        { "gadgets/accesskey/@AccessKeyAction", "OpenMenu",
          QT_TRANSLATE_NOOP("SettingsSchema", "Open a menu") },
        { "gadgets/accesskey/@AccessKeyAction", "ClickElement",
          QT_TRANSLATE_NOOP("SettingsSchema", "Click it") },
        { "gadgets/accesskey/@AccessKeyAction", "FocusElement",
          QT_TRANSLATE_NOOP("SettingsSchema", "Focus it") },
        { "gadgets/accesskey/@AccessKeyAction", "HoverElement",
          QT_TRANSLATE_NOOP("SettingsSchema", "Hover over it") },
        { "gadgets/accesskey/@AccessKeyAction", "OpenInNewViewNode",
          QT_TRANSLATE_NOOP("SettingsSchema", "Open in a new tab") },
        { "gadgets/accesskey/@AccessKeyAction", "OpenInNewDirectory",
          QT_TRANSLATE_NOOP("SettingsSchema", "Open in a new directory") },
        { "gadgets/accesskey/@AccessKeyAction", "OpenOnRoot",
          QT_TRANSLATE_NOOP("SettingsSchema", "Open at the top level") },
        { "gadgets/accesskey/@AccessKeyAction", "OpenInNewViewNodeBackground",
          QT_TRANSLATE_NOOP("SettingsSchema", "Open in a new tab, in the background") },
        { "gadgets/accesskey/@AccessKeyAction", "OpenInNewDirectoryBackground",
          QT_TRANSLATE_NOOP("SettingsSchema", "Open in a new directory, in the background") },

        { "gadgets/accesskey/@AccessKeySelectBlockMethod", "Number",
          QT_TRANSLATE_NOOP("SettingsSchema", "A number") },
        { "gadgets/accesskey/@AccessKeySelectBlockMethod", "ShiftedChar",
          QT_TRANSLATE_NOOP("SettingsSchema", "Shift and a letter") },
        { "gadgets/accesskey/@AccessKeySelectBlockMethod", "CtrledChar",
          QT_TRANSLATE_NOOP("SettingsSchema", "Ctrl and a letter") },
        { "gadgets/accesskey/@AccessKeySelectBlockMethod", "AltedChar",
          QT_TRANSLATE_NOOP("SettingsSchema", "Alt and a letter") },
        { "gadgets/accesskey/@AccessKeySelectBlockMethod", "MetaChar",
          QT_TRANSLATE_NOOP("SettingsSchema", "Meta and a letter") },

        { "gadgets/accesskey/@AccessKeySortOrientation", "Vertical",
          QT_TRANSLATE_NOOP("SettingsSchema", "Top to bottom") },
        { "gadgets/accesskey/@AccessKeySortOrientation", "Horizontal",
          QT_TRANSLATE_NOOP("SettingsSchema", "Left to right") },

        { "network/@HttpCacheType", "DiskHttpCache",   QT_TRANSLATE_NOOP("SettingsSchema", "On disk") },
        { "network/@HttpCacheType", "MemoryHttpCache", QT_TRANSLATE_NOOP("SettingsSchema", "In memory") },
        { "network/@HttpCacheType", "NoCache",         QT_TRANSLATE_NOOP("SettingsSchema", "No cache") },

        { "application/@DownloadPolicy", "Undefined",
          QT_TRANSLATE_NOOP("SettingsSchema", "Not decided yet (asks at the next download)") },
        { "application/@DownloadPolicy", "FixedLocale",
          QT_TRANSLATE_NOOP("SettingsSchema", "The folder below") },
        { "application/@DownloadPolicy", "DownloadFolder",
          QT_TRANSLATE_NOOP("SettingsSchema", "The system's Downloads folder") },
        { "application/@DownloadPolicy", "AskForEachDownload",
          QT_TRANSLATE_NOOP("SettingsSchema", "Ask for each download") },

        { "application/@SslErrorPolicy", "Undefined",
          QT_TRANSLATE_NOOP("SettingsSchema", "Not decided yet (asks at the next error)") },
        { "application/@SslErrorPolicy", "BlockAccess",
          QT_TRANSLATE_NOOP("SettingsSchema", "Block the page") },
        { "application/@SslErrorPolicy", "IgnoreSslErrors",
          QT_TRANSLATE_NOOP("SettingsSchema", "Ignore the error") },
        { "application/@SslErrorPolicy", "AskForEachAccess",
          QT_TRANSLATE_NOOP("SettingsSchema", "Ask every time") },
        { "application/@SslErrorPolicy", "AskForEachHost",
          QT_TRANSLATE_NOOP("SettingsSchema", "Ask once per host") },
        { "application/@SslErrorPolicy", "AskForEachCertificate",
          QT_TRANSLATE_NOOP("SettingsSchema", "Ask once per certificate") },

#ifdef EDGEWEBVIEW
        { "webview/@TrackingPrevention", "None",     QT_TRANSLATE_NOOP("SettingsSchema", "Off") },
        { "webview/@TrackingPrevention", "Basic",    QT_TRANSLATE_NOOP("SettingsSchema", "Basic") },
        { "webview/@TrackingPrevention", "Balanced", QT_TRANSLATE_NOOP("SettingsSchema", "Balanced") },
        { "webview/@TrackingPrevention", "Strict",   QT_TRANSLATE_NOOP("SettingsSchema", "Strict") },
#endif

        { "network/@SecureDnsMode", "SystemOnly",
          QT_TRANSLATE_NOOP("SettingsSchema", "The system's resolver") },
        { "network/@SecureDnsMode", "SecureWithFallback",
          QT_TRANSLATE_NOOP("SettingsSchema", "Secure, falling back to the system") },
        { "network/@SecureDnsMode", "SecureOnly",
          QT_TRANSLATE_NOOP("SettingsSchema", "Secure only") },

        { "application/@GraphicsApi", "Auto",       QT_TRANSLATE_NOOP("SettingsSchema", "Auto") },
        { "application/@GraphicsApi", "Software",   QT_TRANSLATE_NOOP("SettingsSchema", "Software") },
        { "application/@GraphicsApi", "OpenGL",     QT_TRANSLATE_NOOP("SettingsSchema", "OpenGL") },
        { "application/@GraphicsApi", "Direct3D11", QT_TRANSLATE_NOOP("SettingsSchema", "Direct3D 11") },
        { "application/@GraphicsApi", "Direct3D12", QT_TRANSLATE_NOOP("SettingsSchema", "Direct3D 12") },
        { "application/@GraphicsApi", "Vulkan",     QT_TRANSLATE_NOOP("SettingsSchema", "Vulkan") },
        { "application/@GraphicsApi", "Metal",      QT_TRANSLATE_NOOP("SettingsSchema", "Metal") },
    };
    return table;
}

namespace {

    const struct { const char *key, *was, *now; } RENAMED[] = {
        { "webview/@SuspendHiddenViews", "Off", "Active" },
        { "application/@GraphicsApi", "d3d11", "Direct3D11" },
        { "application/@GraphicsApi", "d3d12", "Direct3D12" },
    };

    QString Canonical(const Item &item, const QString &value){
        const QStringList choices =
            QString::fromLatin1(item.choices).split(QLatin1Char('|'));
        foreach(const QString &choice, choices){
            if(choice.compare(value, Qt::CaseInsensitive) == 0) return choice;
        }
        const int count = sizeof(RENAMED) / sizeof(RENAMED[0]);
        for(int i = 0; i < count; i++){
            if(QLatin1String(item.key) == QLatin1String(RENAMED[i].key) &&
               value.compare(QLatin1String(RENAMED[i].was),
                             Qt::CaseInsensitive) == 0)
                return QLatin1String(RENAMED[i].now);
        }
        return value;
    }
}

static QString LabelOfChoice(const Item &item, const QString &choice){
    foreach(const ChoiceLabel &line, ChoiceLabels()){
        if(QLatin1String(line.key) == QLatin1String(item.key) &&
           QLatin1String(line.choice) == choice)
            return QCoreApplication::translate("SettingsSchema", line.label);
    }
    return choice;
}

QList<QPair<QString, QString> > AppliesWhenClauses(const Item &item){
    QList<QPair<QString, QString> > clauses;
    if(!item.appliesWhen || !item.appliesWhen[0]) return clauses;
    foreach(const QString &clause,
            QString::fromLatin1(item.appliesWhen).split(QLatin1Char('|'))){
        const int equals = clause.indexOf(QLatin1Char('='));
        clauses << qMakePair(clause.left(equals),
                             equals < 0 ? QString() : clause.mid(equals + 1));
    }
    return clauses;
}

QVariant Fallback(const Item &item){
    const QString text = QString::fromLatin1(item.fallback);
    switch(item.type){
    case Bool:      return QVariant(text == QStringLiteral("true"));
    case Int:       return QVariant(text.toInt());
    case TextList:  return QVariant(text.isEmpty() ? QStringList() : QStringList(text));
    case Text:
    case Choice:
    case Directory: return QVariant(text);
    }
    return QVariant();
}

QVariant FromJson(const Item &item, const QJsonValue &value){
    switch(item.type){
    case Bool:
        if(!value.isBool()) return QVariant();
        return QVariant(value.toBool());
    case Int:
        if(!value.isDouble()) return QVariant();
        return QVariant(static_cast<int>(value.toDouble()));
    case Choice: {
        if(!value.isString()) return QVariant();
        const QString text = value.toString();
        if(!QString::fromLatin1(item.choices)
               .split(QLatin1Char('|')).contains(text)) return QVariant();
        return QVariant(text);
    }
    case Text:
        if(!value.isString()) return QVariant();
        return QVariant(value.toString());
    case Directory:
        if(!value.isString()) return QVariant();
        return QVariant(QDir::fromNativeSeparators(value.toString()));
    case TextList: {
        if(!value.isArray()) return QVariant();
        QStringList list;
        const QJsonArray array = value.toArray();
        for(int i = 0; i < array.size(); i++){
            if(!array[i].isString()) return QVariant();
            const QString line = array[i].toString().trimmed();
            if(!line.isEmpty()) list << line;
        }
        return QVariant(list);
    }
    }
    return QVariant();
}

QJsonValue ToJson(const Item &item, const QVariant &value){
    switch(item.type){
    case Bool:     return QJsonValue(value.value<bool>());
    case Int:      return QJsonValue(value.value<int>());
    case TextList: return QJsonValue(QJsonArray::fromStringList(value.value<QStringList>()));
    case Choice: {
        const QString canonical = Canonical(item, value.value<QString>());
        if(QLatin1String(item.key) == QLatin1String("gadgets/thumblist/@NodeCollectionType") &&
           !QString::fromLatin1(item.choices).split(QLatin1Char('|')).contains(canonical))
            return QJsonValue(QString::fromLatin1(item.fallback));
        return QJsonValue(canonical);
    }
    case Text:      return QJsonValue(value.value<QString>());
    case Directory: return QJsonValue(QDir::toNativeSeparators(value.value<QString>()));
    }
    return QJsonValue();
}

QString EmptyDirectory(const Item &item){
    const QLatin1String key(item.key);
    QStandardPaths::StandardLocation where;
    if(key == QLatin1String("application/@FileSaveDirectory"))
        where = QStandardPaths::DownloadLocation;
    else if(key == QLatin1String("application/@FileOpenDirectory"))
        where = QStandardPaths::DesktopLocation;
    else return QString();
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(where));
}

QJsonObject Describe(){
    Settings &s = Application::GlobalSettings();

    QJsonArray categories;
    const QList<QPair<QString, QString> > names = Categories();
    const QStringList runs = CategoriesBeginningARun();
    for(int i = 0; i < names.length(); i++){
        QJsonObject category;
        category[QStringLiteral("name")]  = names[i].first;
        category[QStringLiteral("label")] = names[i].second;
        if(runs.contains(names[i].first))
            category[QStringLiteral("divider")] = true;
        categories.append(category);
    }

    QJsonArray items;
    const QList<Item> &table = Items();
    const QList<Group> &headings = Groups();
    const char *group = nullptr;
    const char *category = nullptr;
    for(int i = 0; i < table.length(); i++){
        const Item &item = table[i];
        const QString key = QString::fromLatin1(item.key);

        if(!category || QLatin1String(category) != QLatin1String(item.category)){
            category = item.category;
            group = nullptr;
        }
        for(int j = 0; j < headings.length(); j++){
            if(QLatin1String(headings[j].key) == key){ group = headings[j].label; break; }
        }

        QJsonObject object;
        object[QStringLiteral("key")]      = key;
        object[QStringLiteral("category")] = QString::fromLatin1(item.category);
        object[QStringLiteral("label")]    =
            QCoreApplication::translate("SettingsSchema", item.label);
        if(group)
            object[QStringLiteral("group")] =
                QCoreApplication::translate("SettingsSchema", group);
        if(item.hint[0])
            object[QStringLiteral("hint")] =
                QCoreApplication::translate("SettingsSchema", item.hint);
        if(item.needsRestart)
            object[QStringLiteral("needsRestart")] = true;
        const QList<QPair<QString, QString> > clauses = AppliesWhenClauses(item);
        if(!clauses.isEmpty()){
            QJsonArray applies;
            for(const QPair<QString, QString> &clause : clauses){
                QJsonObject clauseObject;
                clauseObject[QStringLiteral("key")] = clause.first;
                const Item *other = Find(clause.first);
                if(other && other->type == Bool){
                    clauseObject[QStringLiteral("value")] = clause.second == QStringLiteral("true");
                } else {
                    clauseObject[QStringLiteral("value")] = clause.second;
                    clauseObject[QStringLiteral("label")] =
                        other ? LabelOfChoice(*other, clause.second) : clause.second;
                }
                applies.append(clauseObject);
            }
            object[QStringLiteral("appliesWhen")] = applies;
        }
        if(item.picker && item.picker[0])
            object[QStringLiteral("picker")] = QString::fromLatin1(item.picker);
        if(item.type == Directory){
            const QString empty = EmptyDirectory(item);
            if(!empty.isEmpty()) object[QStringLiteral("placeholder")] = empty;
        }

        switch(item.type){
        case Bool:      object[QStringLiteral("type")] = QStringLiteral("bool");      break;
        case Int:       object[QStringLiteral("type")] = QStringLiteral("int");       break;
        case Text:      object[QStringLiteral("type")] = QStringLiteral("text");      break;
        case Choice:    object[QStringLiteral("type")] = QStringLiteral("choice");    break;
        case Directory: object[QStringLiteral("type")] = QStringLiteral("directory"); break;
        case TextList:  object[QStringLiteral("type")] = QStringLiteral("textlist");  break;
        }

        if(item.type == Choice){
            const QStringList choices =
                QString::fromLatin1(item.choices).split(QLatin1Char('|'));
            object[QStringLiteral("choices")] = QJsonArray::fromStringList(choices);
            QJsonArray labels;
            foreach(const QString &choice, choices)
                labels.append(LabelOfChoice(item, choice));
            object[QStringLiteral("choiceLabels")] = labels;
        }

        const QVariant fallback = Fallback(item);
        object[QStringLiteral("fallback")] = ToJson(item, fallback);
        object[QStringLiteral("value")]    = ToJson(item, s.value(key, fallback));

        items.append(object);
    }

    QJsonObject root;
    root[QStringLiteral("categories")] = categories;
    root[QStringLiteral("items")]      = items;
    root[QStringLiteral("strings")]    = PageStrings();
    return root;
}

QJsonObject PageStrings(){
    QJsonObject strings;
    strings[QStringLiteral("title")] =
        QCoreApplication::translate("SettingsPage", "Settings");
    strings[QStringLiteral("search")] =
        QCoreApplication::translate("SettingsPage", "Search settings");
    strings[QStringLiteral("reset")] =
        QCoreApplication::translate("SettingsPage", "Reset");
    strings[QStringLiteral("resetTitle")] =
        QCoreApplication::translate("SettingsPage", "Back to the default");
    strings[QStringLiteral("browse")] =
        QCoreApplication::translate("SettingsPage", "Browse...");
    strings[QStringLiteral("saved")] =
        QCoreApplication::translate("SettingsPage", "Saved");
    strings[QStringLiteral("savedRestart")] =
        QCoreApplication::translate("SettingsPage", "Saved. Takes effect on the next launch.");
    strings[QStringLiteral("failed")] =
        QCoreApplication::translate("SettingsPage", "Not saved");
    strings[QStringLiteral("restart")] =
        QCoreApplication::translate("SettingsPage", "restart");
    strings[QStringLiteral("notUsedWhile")] =
        QCoreApplication::translate("SettingsPage", "Not used while %1.");
    strings[QStringLiteral("clauseOn")] =
        QCoreApplication::translate("SettingsPage", "'%1' is on");
    strings[QStringLiteral("clauseOff")] =
        QCoreApplication::translate("SettingsPage", "'%1' is off");
    strings[QStringLiteral("clauseNot")] =
        QCoreApplication::translate("SettingsPage", "'%1' is not '%2'");
    strings[QStringLiteral("clauseJoin")] =
        QCoreApplication::translate("SettingsPage", " and ");
    strings[QStringLiteral("noMatch")] =
        QCoreApplication::translate("SettingsPage", "Nothing matches.");
    strings[QStringLiteral("loadFailed")] =
        QCoreApplication::translate("SettingsPage", "Could not load the settings: ");
    strings[QStringLiteral("openPasswords")] =
        QCoreApplication::translate("SettingsPage", "See the saved passwords");
    strings[QStringLiteral("openPermissions")] =
        QCoreApplication::translate("SettingsPage", "See the site permissions");
    strings[QStringLiteral("autosave")] =
        QCoreApplication::translate("SettingsPage",
            "Changes are saved as they are made. Text is saved when its box is left.");
    return strings;
}

}
