#include "switch.hpp"

#include "settingsschema.hpp"

#include <QCoreApplication>
#include <QJsonArray>

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
              "'Flat' is a grid of cards, 'Glass' a translucent overlay. Either "
              "is drawn in either colour scheme."),
          "FlatStyle|GlassStyle", "FlatStyle", false },

        { "application/@EnableTransparentBar", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Translucent tab bar"), "", "", "false", true },

        { "application/@EnableFramelessWindow", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Draw the window frame in the application"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Replaces the desktop's title bar with the application's own."),
          "", "false", true },

        { "application/@PurgeView", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Give the page its own window"),
          "", "", "false", true },

        { "application/@PurgeNotifier", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Give the status area its own window"),
          "", "", "false", true },

        { "application/@PurgeReceiver", SettingsSchema::Bool, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Give the command line its own window"),
          "", "", "false", true },

        { "application/@Viewport", SettingsSchema::Choice, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tab list drawing surface"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "What the tab list and the access key labels are drawn on; the "
              "page is not. 'GLWidget' and 'OpenGLWidget' are the same. Change "
              "it only if they are drawn wrongly."),
          "Widget|GLWidget|OpenGLWidget", "Widget", true },

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

        { "application/@AddChildViewNodePosition", SettingsSchema::Choice, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where a child tab is inserted"), "",
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
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Off, they stay among the siblings of the current tab."),
          "", "false", false },

        { "application/@MaxViewCount", SettingsSchema::Int, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Pages kept loaded at once"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Tabs beyond this keep their place in the tree but drop the page. "
              "-1 for the default (10)."),
          "", "-1", false },

        { "webview/@SuspendHiddenViews", SettingsSchema::Choice, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Hidden tabs"),
#ifdef EDGEWEBVIEW
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Active': keeps running while hidden.\n"
              "'Frozen': stops timers and animations.\n"
              "'Discarded': throws the page away and frees its render "
              "process.\n"
              "\n"
              "'Frozen' and 'Discarded' do nothing while a tab is "
              "loading, playing sound, or sharing its process.\n"
              "An Edge view treats 'Discarded' as 'Frozen'."),
#else
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Active': keeps running while hidden.\n"
              "'Frozen': stops timers and animations.\n"
              "'Discarded': throws the page away and frees its render "
              "process.\n"
              "\n"
              "'Frozen' and 'Discarded' do nothing while a tab is "
              "loading, playing sound, or sharing its process."),
#endif
          "Active|Frozen|Discarded", "Active", false },

        { "application/@EnableAutoLoad", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load tabs ahead in the background"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Only tabs whose directory setting 'Auto load' is on, and only "
              "while 'Hidden tabs' above is 'Active'."),
          "", "true", false },

        { "application/@AutoLoadInterval", SettingsSchema::Int, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load one tab every (ms)"), "", "", "1000", false },

        { "application/@MaxTrashEntryCount", SettingsSchema::Int, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Entries kept in the wastebasket"),
          QT_TRANSLATE_NOOP("SettingsSchema", "-1 for the default (100)."),
          "", "-1", false },

        { "treebar/@EnableFrameRate", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show the frame rate on the bar"),
          "", "", "false", false },

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

        { "gadgets/thumblist/@EnableInPlaceNotifier", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show a panel of dates and url"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Glass style only."),
          "", "true", true },

        { "gadgets/thumblist/@EnableHoveredSpotLight", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Join a hovered thumbnail to its title"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Glass style only."),
          "", "true", true },

        { "gadgets/thumblist/@EnablePrimarySpotLight", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Join the current thumbnail to its title"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Glass style only."),
          "", "false", true },

        { "gadgets/thumblist/@EnableLoadedSpotLight", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Join every loaded thumbnail to its title"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Glass style only."),
          "", "false", false },

        { "gadgets/thumblist/@RightClickToRenameNode", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Right click renames"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Glass style only."),
          "", "false", false },

        { "gadgets/thumblist/@ScrollToChangeDirectory", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Wheel changes directory"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Glass style only."),
          "", "false", false },

        { "gadgets/thumblist/@EnableFrameRate", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show the frame rate on the tab list"),
          "", "", "false", false },

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

        { "webview/@ActivateNewViewDefault", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Switch to a newly opened tab"),
          "", "", "true", false },

        { "webview/preferences/NavigateOnDropEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Dropping a link on a page opens it there"),
          "", "", "true", false },

        { "webview/@DragToStartDownload", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Dragging a link out downloads it"),
          "", "", "false", false },

        { "webview/@EnableSingleKeyShortcut", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Shortcuts on a single key"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "A letter or symbol pressed without Ctrl or Alt runs its action. "
              "Off, the page gets the key."),
          "", "false", true },

        { "webview/@EnableMouseGesture", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Mouse gestures"), "", "", "true", false },

        { "webview/@EnableDragGesture", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Super drag"), "", "", "false", false },

        { "webview/preferences/SpatialNavigationEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Arrow keys move between links"),
          "", "", "false", false },

        { "webview/preferences/LinksIncludedInFocusChain", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tab key stops on links"), "", "", "true", false },

        { "webview/preferences/FocusOnNavigationEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Focus the page when it navigates"),
          "", "", "false", false },

        { "application/@EnableMiniMap", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show the minimap scroll bar"),
          QT_TRANSLATE_NOOP("SettingsSchema",
                            "A reduced model of the whole page. A scroll "
                            "on the minimap acts at the middle of the "
                            "page."),
          "", "false", true },

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

        { "webview/preferences/AutoLoadImages", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load images"), "", "", "true", false },

        { "webview/preferences/AutoLoadIconsForPage", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load favicons"), "", "", "true", false },

        { "webview/preferences/TouchIconsEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load touch icons"), "", "", "false", false },

        { "webview/preferences/PdfViewerEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show PDFs in the page"), "", "", "true", false },

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

        { "network/@SpellCheckLanguages", SettingsSchema::TextList, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Spell checking dictionaries"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One name per line, as the '.bdic' file next to the executable "
              "is called without its extension ('en-US'). Empty turns spell "
              "checking off."),
          "", "", true },

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

        { "webview/preferences/PlaybackRequiresUserGesture", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Media needs a click before it plays"),
          "", "", "true", false },

        { "webview/preferences/TouchEventsApiEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tell pages this machine has a touch screen"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Until this is changed, the engine answers by whether a touch "
              "screen was found."),
          "", "false", false },

        { "webview/preferences/TrimAccessibilityIdentifiers", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Leave element ids out of the accessibility tree"),
          "", "", "false", false },

        { "webview/@SavePageFormat", SettingsSchema::Choice, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "How a page is saved"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'MimeHtml' is one '.mhtml' with everything in it. "
              "'CompleteHtml' is an '.html' beside a folder of what it refers "
              "to. 'SingleHtml' is the markup alone."),
          "MimeHtmlSaveFormat|CompleteHtmlSaveFormat|SingleHtmlSaveFormat",
          "MimeHtmlSaveFormat", false },

        { "webview/preferences/PrintElementBackgrounds", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Print backgrounds"), "", "", "true", false },

        { "webview/preferences/PrintHeaderAndFooter", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Print the page title and address in the margin"),
          "", "", "false", false },

        { "webview/preferences/PreferCSSMarginsForPrinting", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Let the page choose the printed margins"),
          "", "", "false", false },

        { "webview/@InspectorInMainWindow", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Open the inspector beside the page"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "A pane of this window instead of a window of its own. Takes "
              "effect the next time the inspector is opened."),
          "", "true", false },

        { "webview/preferences/BackForwardCacheEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Keep the previous page in memory"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Going back then shows the page as it was, without loading it "
              "again."),
          "", "false", false },

        { "gadgets/accesskey/@EnableMultiStroke", SettingsSchema::Bool, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "Allow multi stroke labels"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Every link gets a label of its own, typed one key at a time, "
              "instead of blocks of one key each."),
          "", "true", false },

        { "gadgets/accesskey/@AccessKeyMode", SettingsSchema::Choice, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "Which keys label the links"), "",
          "BothHands|LeftHand|RightHand|Custom", "BothHands", false },

        { "gadgets/accesskey/@AccessKeyCustomSequence", SettingsSchema::Text, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "Custom key sequence"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Used when the mode above is 'Custom'."),
          "", "", false },

        { "gadgets/accesskey/@AccessKeyAction", SettingsSchema::Choice, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "What choosing a link does"), "",
          "OpenMenu|ClickElement|FocusElement|HoverElement|OpenInNewViewNode|"
          "OpenInNewDirectory|OpenOnRoot|OpenInNewViewNodeBackground|"
          "OpenInNewDirectoryBackground",
          "OpenMenu", false },

        { "gadgets/accesskey/@AccessKeySelectBlockMethod", SettingsSchema::Choice, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "How a block of links is chosen"), "",
          "Number|ShiftedChar|CtrledChar|AltedChar|MetaChar", "Number", false,
          "gadgets/accesskey/@EnableMultiStroke", false },

        { "gadgets/accesskey/@AccessKeySortOrientation", SettingsSchema::Choice, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "Order the links are labelled in"), "",
          "Vertical|Horizontal", "Vertical", false },

        { "localview/@MediaVolume", SettingsSchema::Int, "files",
          QT_TRANSLATE_NOOP("SettingsSchema", "Volume"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "0 to 100. The up and down arrows over a playing file change it."),
          "", "50", false },

        { "localview/@AutoPlayMedia", SettingsSchema::Bool, "files",
          QT_TRANSLATE_NOOP("SettingsSchema", "Start playing when a file is opened"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Off, the file opens paused and the space bar starts it."),
          "", "true", false },

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
              "Extensions work in the WebEngine and Edge views, except in "
              "private profiles.\n"
              "Use the address bar's Extensions button to enable, disable, pin "
              "or open one.\n"
              "Reload open pages after a change."),
          "", "", false, nullptr, false, "chrome-extension" },

        { "network/@ExtensionShims", SettingsSchema::Bool, "extensions",
          QT_TRANSLATE_NOOP("SettingsSchema", "Extension compatibility layer (WebEngineView / Edge)"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "WebEngine views leave out parts of the extension platform, so "
              "some extensions load and do nothing.\n"
              "On, Vanilla loads a copy of each extension with a script added "
              "that fills them in. The Edge view uses the same copy for its "
              "tabs.\n"
              "The folder you registered is not changed; the copy is kept in "
              "Vanilla's data directory.\n"
              "An extension the layer cannot help is loaded as it is, and its "
              "row says why.\n"
              "Some features, such as an extension's page shown inside a web "
              "page, need a WebEngine built with Vanilla's patches."),
          "", "true", true },

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
              "Cookies a site sets to last only until the browser closes, such "
              "as a login, are saved too and used again at the next launch."),
          "", "false", true },

        { "network/@RememberPermissions", SettingsSchema::Bool, "sitedata",
          QT_TRANSLATE_NOOP("SettingsSchema", "Remember what a site was allowed"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "An answer to a permission request is kept for that site instead "
              "of being asked for again at the next launch."),
          "", "true", true },

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

        { "application/@DownloadPolicy", SettingsSchema::Choice, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where downloads go"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Undefined' asks at the next download which of the other three "
              "to use. 'FixedLocale' and 'DownloadFolder' both save to the "
              "download folder below without asking; 'DownloadFolder' chosen "
              "at that question also sets the folder to the system's Downloads."),
          "Undefined|FixedLocale|DownloadFolder|AskForEachDownload",
          "Undefined", false },

        { "application/@FileSaveDirectory", SettingsSchema::Directory, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Download folder"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Replaced by the folder a file was last saved to. Empty means the "
              "system's Downloads."),
          "", "", false },

        { "application/@FileOpenDirectory", SettingsSchema::Directory, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Folder a file picker opens in"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Replaced by the folder a file was last chosen from. Empty means "
              "the desktop."),
          "", "", false },

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
              "Adds the 'DNT' and 'Sec-GPC' headers. Whether a site honours "
              "them is up to the site."),
          "", "false", false },

        { "application/@EnableGoogleSuggest", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Suggestions from Google while typing a url"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Sends what is typed in the address bar to Google."),
          "", "false", false },

        { "application/@SslErrorPolicy", SettingsSchema::Choice, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "On a certificate error"), "",
          "Undefined|BlockAccess|IgnoreSslErrors|AskForEachAccess|"
          "AskForEachHost|AskForEachCertificate",
          "Undefined", false },

        { "application/@AllowedHosts", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Hosts whose certificate errors are accepted"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Used only while 'On a certificate error' is 'AskForEachHost'. "
              "One per line. Answering its question adds a line."),
          "", "", false },

        { "application/@BlockedHosts", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Hosts whose certificate errors are refused"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Used only while 'On a certificate error' is 'AskForEachHost'. "
              "One per line. Answering its question adds a line."),
          "", "", false },

        { "application/@AllowedCertificates", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Certificates always allowed"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Used only while 'On a certificate error' is 'AskForEachCertificate'. "
              "One host:SHA-256 identity per line. Remove one to ask again."),
          "", "", false },

        { "application/@BlockedCertificates", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Certificates always blocked"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Used only while 'On a certificate error' is 'AskForEachCertificate'. "
              "One host:SHA-256 identity per line. Remove one to ask again."),
          "", "", false },

        { "network/@SecureDnsMode", SettingsSchema::Choice, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "DNS over HTTPS"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'SystemOnly' asks the machine's resolver. The other two use "
              "the servers below, falling back to the system or refusing to."),
          "SystemOnly|SecureWithFallback|SecureOnly", "SystemOnly", true },

        { "network/@SecureDnsServers", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "DNS over HTTPS servers"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One URI template per line, for example "
              "'https://dns.google/dns-query{?dns}'."),
          "", "", true },

        { "application/@EnableAutoSave", SettingsSchema::Bool, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Save while running"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Saves the tab tree, the settings, cookies and site icons at the "
              "interval below, so that a crash loses less. They are saved on "
              "quitting either way."),
          "", "true", false },

        { "application/@AutoSaveInterval", SettingsSchema::Int, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Save every (ms)"), "", "", "300000", false },

        { "application/@MaxBackUpGenerationCount", SettingsSchema::Int, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Backups kept"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "At each launch the saved files are copied with the date in "
              "their names, next to them in the data folder. Older copies "
              "than this many are deleted."),
          "", "5", false },

        { "application/@GraphicsApi", SettingsSchema::Choice, "engine",
          QT_TRANSLATE_NOOP("SettingsSchema", "Graphics API (WebEngineView)"),
#if defined(Q_OS_WIN)
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Auto': Direct3D 11.\n"
              "'OpenGL': choose this if the screen flickers.\n"
              "\n"
              "A choice this OS cannot run is treated as 'Auto'. The "
              "environment variable QSG_RHI_BACKEND wins when it is set."),
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
          "Auto|Software|OpenGL|Direct3D11|Direct3D12|Vulkan|Metal", "Auto", true },

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

        { "application/@ChromiumFlags", SettingsSchema::TextList, "engine",
          QT_TRANSLATE_NOOP("SettingsSchema", "Chromium switches"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Passed to the rendering engine at startup. Separated by spaces "
              "or line breaks, each beginning with '--'.\n"
              "Put double quotes around a value which has spaces in it: "
              "--switch=\"a b\".\n"
              "A switch it will not take can stop it starting, and the way "
              "back is to edit 'data/config.json'."),
          "", "", true },
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
    list << qMakePair(QStringLiteral("accesskey"),
                      QCoreApplication::translate("SettingsSchema", "Access keys"));
    list << qMakePair(QStringLiteral("input"),
                      QCoreApplication::translate("SettingsSchema", "Keys and mouse"));
    list << qMakePair(QStringLiteral("files"),
                      QCoreApplication::translate("SettingsSchema", "File browser"));
    list << qMakePair(QStringLiteral("commands"),
                      QCoreApplication::translate("SettingsSchema", "Open in other programs"));
    list << qMakePair(QStringLiteral("extensions"),
                      QCoreApplication::translate("SettingsSchema", "Extensions"));
    list << qMakePair(QStringLiteral("sitedata"),
                      QCoreApplication::translate("SettingsSchema", "Site data"));
    list << qMakePair(QStringLiteral("network"),
                      QCoreApplication::translate("SettingsSchema", "Network"));
    list << qMakePair(QStringLiteral("session"),
                      QCoreApplication::translate("SettingsSchema", "Saving and backups"));
    list << qMakePair(QStringLiteral("engine"),
                      QCoreApplication::translate("SettingsSchema", "Rendering and engine"));
    return list;
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
    case Directory:
        if(!value.isString()) return QVariant();
        return QVariant(value.toString());
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
    case Text:
    case Directory: return QJsonValue(value.value<QString>());
    }
    return QJsonValue();
}

QJsonObject Describe(){
    Settings &s = Application::GlobalSettings();

    QJsonArray categories;
    const QList<QPair<QString, QString> > names = Categories();
    for(int i = 0; i < names.length(); i++){
        QJsonObject category;
        category[QStringLiteral("name")]  = names[i].first;
        category[QStringLiteral("label")] = names[i].second;
        categories.append(category);
    }

    QJsonArray items;
    const QList<Item> &table = Items();
    for(int i = 0; i < table.length(); i++){
        const Item &item = table[i];
        const QString key = QString::fromLatin1(item.key);

        QJsonObject object;
        object[QStringLiteral("key")]      = key;
        object[QStringLiteral("category")] = QString::fromLatin1(item.category);
        object[QStringLiteral("label")]    =
            QCoreApplication::translate("SettingsSchema", item.label);
        if(item.hint[0])
            object[QStringLiteral("hint")] =
                QCoreApplication::translate("SettingsSchema", item.hint);
        if(item.needsRestart)
            object[QStringLiteral("needsRestart")] = true;
        if(item.appliesWhenKey && item.appliesWhenKey[0]){
            QJsonObject applies;
            applies[QStringLiteral("key")]   = QString::fromLatin1(item.appliesWhenKey);
            applies[QStringLiteral("value")] = item.appliesWhen;
            object[QStringLiteral("appliesWhen")] = applies;
        }
        if(item.picker && item.picker[0])
            object[QStringLiteral("picker")] = QString::fromLatin1(item.picker);

        switch(item.type){
        case Bool:      object[QStringLiteral("type")] = QStringLiteral("bool");      break;
        case Int:       object[QStringLiteral("type")] = QStringLiteral("int");       break;
        case Text:      object[QStringLiteral("type")] = QStringLiteral("text");      break;
        case Choice:    object[QStringLiteral("type")] = QStringLiteral("choice");    break;
        case Directory: object[QStringLiteral("type")] = QStringLiteral("directory"); break;
        case TextList:  object[QStringLiteral("type")] = QStringLiteral("textlist");  break;
        }

        if(item.type == Choice)
            object[QStringLiteral("choices")] =
                QJsonArray::fromStringList(QString::fromLatin1(item.choices)
                                           .split(QLatin1Char('|')));

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
    strings[QStringLiteral("notUsedWhileOn")] =
        QCoreApplication::translate("SettingsPage", "Not used while '%1' is on.");
    strings[QStringLiteral("notUsedWhileOff")] =
        QCoreApplication::translate("SettingsPage", "Not used while '%1' is off.");
    strings[QStringLiteral("noMatch")] =
        QCoreApplication::translate("SettingsPage", "Nothing matches.");
    strings[QStringLiteral("loadFailed")] =
        QCoreApplication::translate("SettingsPage", "Could not load the settings: ");
    return strings;
}

}
