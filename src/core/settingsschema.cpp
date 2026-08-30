#include "switch.hpp"

#include "settingsschema.hpp"

#include <QCoreApplication>
#include <QJsonArray>

#include "application.hpp"

namespace {

    const SettingsSchema::Item ITEMS[] = {

        { "application/@ColorScheme", SettingsSchema::Choice, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Colour scheme"),
          QT_TRANSLATE_NOOP("SettingsSchema", "'Automatic' follows the desktop."),
          "Auto|Light|Dark", "Auto", false },

        { "gadgets/@Style", SettingsSchema::Choice, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tree overview style"),
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

        { "application/@Viewport", SettingsSchema::Choice, "appearance",
          QT_TRANSLATE_NOOP("SettingsSchema", "Viewport"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "How the page is composited. Change it only if drawing misbehaves."),
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

        { "treebar/@EnableFrameRate", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show the frame rate on the bar"),
          "", "", "false", false },

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

        { "application/@MaxViewCount", SettingsSchema::Int, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Pages kept loaded at once"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Tabs beyond this keep their place in the tree but drop the page. "
              "-1 for the default (10)."),
          "", "-1", false },

        { "application/@MaxTrashEntryCount", SettingsSchema::Int, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Entries kept in the wastebasket"),
          QT_TRANSLATE_NOOP("SettingsSchema", "-1 for the default (100)."),
          "", "-1", false },

        { "application/@TraverseAllView", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Next and previous walk the whole tree"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Off, they stay among the siblings of the current tab."),
          "", "false", false },

        { "application/@PurgeView", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Give the page its own window"),
          "", "", "false", true },

        { "application/@PurgeNotifier", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Give the status area its own window"),
          "", "", "false", true },

        { "application/@PurgeReceiver", SettingsSchema::Bool, "tabs",
          QT_TRANSLATE_NOOP("SettingsSchema", "Give the command line its own window"),
          "", "", "false", true },

        { "gadgets/thumblist/@EnableCloseButton", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Close button on a thumbnail"),
          "", "", "true", false },

        { "gadgets/thumblist/@EnableCloneButton", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Duplicate button on a thumbnail"),
          "", "", "true", false },

        { "gadgets/thumblist/@EnableAnimation", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Animate the overview"), "", "", "false", false },

        { "gadgets/thumblist/@EnableInPlaceNotifier", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show a panel of dates and url"),
          "", "", "true", false },

        { "gadgets/thumblist/@EnableHoveredSpotLight", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Join a hovered thumbnail to its title"),
          "", "", "true", false },

        { "gadgets/thumblist/@EnablePrimarySpotLight", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Join the current thumbnail to its title"),
          "", "", "false", false },

        { "gadgets/thumblist/@EnableLoadedSpotLight", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Join every loaded thumbnail to its title"),
          "", "", "false", false },

        { "gadgets/thumblist/@NodeCollectionType", SettingsSchema::Choice, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "What the overview collects"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'Flat' is the nodes beside the current one, 'Straight' one line "
              "from the root down, 'Recursive' everything below it, 'Foldable' "
              "the same but stopping at folded directories. This is what a new "
              "window starts with; the overview's own menu changes the window "
              "in front of you."),
          "Flat|Straight|Recursive|Foldable", "Flat", false },

        { "gadgets/thumblist/@RightClickToRenameNode", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Right click renames"), "", "", "false", false },

        { "gadgets/thumblist/@ScrollToChangeDirectory", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Wheel changes directory"), "", "", "false", false },

        { "gadgets/thumblist/@EnableFrameRate", SettingsSchema::Bool, "gadgets",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show the frame rate on the overview"),
          "", "", "false", false },

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

        { "webview/@OpenCommandOperation", SettingsSchema::Choice, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where a link opens"), "",
          "InNewViewNode|InNewDirectory|OnRoot|InNewViewNodeBackground|"
          "InNewDirectoryBackground|OnRootBackground",
          "InNewViewNode", false },

        { "webview/@ActivateNewViewDefault", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Switch to a newly opened tab"),
          "", "", "true", false },

        { "webview/@NavigationBySpaceKey", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Space scrolls the page"), "", "", "false", false },

        { "application/@EnableMiniMap", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show the minimap scroll bar"),
          QT_TRANSLATE_NOOP("SettingsSchema",
                            "A reduced model of the whole page. A scroll "
                            "on the minimap acts at the middle of the "
                            "page."),
          "", "false", true },

        { "webview/@EnableMouseGesture", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Mouse gestures"), "", "", "true", false },

        { "webview/@EnableDragGesture", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Super drag"), "", "", "false", false },

        { "webview/@DragToStartDownload", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Dragging a link out downloads it"),
          "", "", "false", false },

        { "webview/@EnableDestinationInferrer", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Guess where a link should open"),
          "", "", "false", false },

        { "webview/@InspectorInMainWindow", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Open the inspector beside the page"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "A pane of this window instead of a window of its own. Takes "
              "effect the next time the inspector is opened."),
          "", "true", false },

        { "webview/@SavePageFormat", SettingsSchema::Choice, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "How a page is saved"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "'MimeHtml' is one '.mhtml' with everything in it. "
              "'CompleteHtml' is an '.html' beside a folder of what it refers "
              "to. 'SingleHtml' is the markup alone."),
          "MimeHtmlSaveFormat|CompleteHtmlSaveFormat|SingleHtmlSaveFormat",
          "MimeHtmlSaveFormat", false },

        { "webview/@SuspendHiddenViews", SettingsSchema::Choice, "page",
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

        { "webview/preferences/JavascriptEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "JavaScript"), "", "", "true", false },

        { "webview/preferences/AutoLoadImages", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load images"), "", "", "true", false },

        { "webview/preferences/LocalStorageEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Local storage"), "", "", "true", false },

        { "webview/preferences/PluginsEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Plugins"), "", "", "true", false },

        { "webview/preferences/PdfViewerEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show PDFs in the page"), "", "", "true", false },

        { "webview/preferences/FullScreenSupportEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Allow a page to go full screen"),
          "", "", "true", false },

        { "webview/preferences/ScrollAnimatorEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Smooth scrolling"), "", "", "false", false },

        { "webview/preferences/SpatialNavigationEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Arrow keys move between links"),
          "", "", "false", false },

        { "webview/preferences/CaretBrowsingEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Caret browsing"), "", "", "false", false },

        { "webview/preferences/DeveloperExtrasEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Developer tools"), "", "", "false", false },

        { "webview/preferences/WebGLEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "WebGL"), "", "", "true", false },

        { "webview/preferences/ScreenCaptureEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Allow a page to capture the screen"),
          "", "", "false", false },

        { "webview/preferences/NotificationsEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Allow a page to send notifications"),
          "", "", "true", false },

        { "webview/preferences/PlaybackRequiresUserGesture", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Media needs a click before it plays"),
          "", "", "true", false },

        { "webview/preferences/PrintElementBackgrounds", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Print backgrounds"), "", "", "true", false },

        { "webview/preferences/ZoomTextOnly", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Zoom text only"), "", "", "false", false },

        { "webview/preferences/ShowScrollBars", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Show scroll bars"), "", "", "true", false },

        { "webview/preferences/FocusOnNavigationEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Focus the page when it navigates"),
          "", "", "false", false },

        { "webview/preferences/LinksIncludedInFocusChain", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tab key stops on links"), "", "", "true", false },

        { "webview/preferences/AutoLoadIconsForPage", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load favicons"), "", "", "true", false },

        { "webview/preferences/TouchIconsEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Load touch icons"), "", "", "false", false },

        { "webview/preferences/NavigateOnDropEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Dropping a link on a page opens it there"),
          "", "", "true", false },

        { "webview/preferences/ReadingFromCanvasEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Let a page read back its own canvas"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Reading a canvas back is one of the ways a site fingerprints "
              "the machine. Off, pages which use a canvas do not work."),
          "", "true", false },

        { "webview/preferences/ForceDarkMode", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Darken pages which have no dark theme"),
          "", "", "false", false },

        { "webview/preferences/BackForwardCacheEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Keep the previous page in memory"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Going back then shows the page as it was, without loading it "
              "again."),
          "", "false", false },

        { "webview/preferences/TouchEventsApiEnabled", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Tell pages this machine has a touch screen"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Until this is changed, the engine answers by whether a touch "
              "screen was found."),
          "", "false", false },

        { "webview/preferences/TrimAccessibilityIdentifiers", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Leave element ids out of the accessibility tree"),
          "", "", "false", false },

        { "webview/preferences/PrintHeaderAndFooter", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Print the page title and address in the margin"),
          "", "", "false", false },

        { "webview/preferences/PreferCSSMarginsForPrinting", SettingsSchema::Bool, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Let the page choose the printed margins"),
          "", "", "false", false },

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

        { "application/@ChromiumFlags", SettingsSchema::Text, "page",
          QT_TRANSLATE_NOOP("SettingsSchema", "Chromium switches"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Passed to the engine at startup, separated by spaces, each "
              "beginning with '--'. A switch it will not take can stop it "
              "starting, and the way back is to edit 'data/config.json'."),
          "", "", true },

        { "gadgets/accesskey/@EnableMultiStroke", SettingsSchema::Bool, "accesskey",
          QT_TRANSLATE_NOOP("SettingsSchema", "Allow multi stroke labels"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Every link gets a label of its own, typed one key at a time, "
              "instead of blocks of one key each."),
          "", "false", false },

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

        { "application/@DownloadPolicy", SettingsSchema::Choice, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where downloads go"), "",
          "Undefined|FixedLocale|DownloadFolder|AskForEachDownload",
          "Undefined", false },

        { "application/@FileSaveDirectory", SettingsSchema::Directory, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Download folder"), "", "", "", false },

        { "application/@FileOpenDirectory", SettingsSchema::Directory, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Upload folder"), "", "", "", false },

        { "application/@SslErrorPolicy", SettingsSchema::Choice, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "On a certificate error"), "",
          "Undefined|BlockAccess|IgnoreSslErrors|AskForEachAccess|"
          "AskForEachHost|AskForEachCertificate",
          "Undefined", false },

        { "application/@SaveSessionCookie", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Keep session cookies between launches"),
          "", "", "false", true },

        { "application/@AcceptLanguage", SettingsSchema::Text, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Accept-Language"), "", "", "en-US", false },

        { "application/@EnableGoogleSuggest", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Suggestions from Google while typing a url"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Sends what is typed in the address bar to Google."),
          "", "false", false },

        { "application/@AllowedHosts", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Hosts always allowed"),
          QT_TRANSLATE_NOOP("SettingsSchema", "One per line. Empty means every host."),
          "", "", false },

        { "application/@BlockedHosts", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Hosts always blocked"),
          QT_TRANSLATE_NOOP("SettingsSchema", "One per line."),
          "", "", false },

        { "application/@AllowedCertificates", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Certificates always allowed"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One host:SHA-256 identity per line. Remove one to ask again."),
          "", "", false },

        { "application/@BlockedCertificates", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Certificates always blocked"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One host:SHA-256 identity per line. Remove one to ask again."),
          "", "", false },

        { "network/@HttpCacheType", SettingsSchema::Choice, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Where the page cache is kept"),
          QT_TRANSLATE_NOOP("SettingsSchema", "A private tab always uses memory."),
          "DiskHttpCache|MemoryHttpCache|NoCache", "DiskHttpCache", true },

        { "network/@HttpCacheMaximumSize", SettingsSchema::Int, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Cache size limit (MB)"),
          QT_TRANSLATE_NOOP("SettingsSchema", "Zero lets the engine decide."),
          "", "0", true },

        { "network/@RememberPermissions", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Remember what a site was allowed"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "An answer to a permission request is kept for that site instead "
              "of being asked for again at the next launch."),
          "", "true", true },

        { "network/@SpellCheckLanguages", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Spell checking dictionaries"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One name per line, as the '.bdic' file next to the executable "
              "is called without its extension ('en-US'). Empty turns spell "
              "checking off."),
          "", "", true },

        { "network/@EnablePushService", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Let a site send push messages"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "A site allowed to send notifications can then reach this "
              "machine while its page is not open."),
          "", "false", true },

        { "network/@UnloadHangoutsExtension", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Remove the built-in Google Hangouts extension"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "The engine ships an extension which only Google's own sites "
              "can use. Off, it is kept, which Google Meet may want for "
              "sharing a screen."),
          "", "true", true },

        { "network/@Extensions", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Extensions to load"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One directory per line, each holding an unpacked extension: "
              "the folder its 'manifest.json' sits in. A line starting with "
              "'#' is ignored, and only manifest version 3 is accepted.\n"
              "A line added here reaches the views made after it. Private "
              "windows and QML views have no extensions."),
          "", "", true },

        { "network/@BlockedUrlPatterns", SettingsSchema::TextList, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Requests never made"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One wildcard per line, matched against the whole address "
              "('*doubleclick.net*'). A line starting with '#' is ignored. "
              "What is typed into the address bar is never blocked."),
          "", "", false },

        { "network/@SendDoNotTrack", SettingsSchema::Bool, "network",
          QT_TRANSLATE_NOOP("SettingsSchema", "Ask not to be tracked"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "Adds the 'DNT' and 'Sec-GPC' headers. Whether a site honours "
              "them is up to the site."),
          "", "false", false },

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

        { "application/@ExternalCommands", SettingsSchema::TextList, "commands",
          QT_TRANSLATE_NOOP("SettingsSchema", "Commands to open a page with"),
          QT_TRANSLATE_NOOP("SettingsSchema",
              "One per line, 'name = command line'. The address goes where "
              "'%u' is, or at the end when there is none."),
          "", "", false },

        { "application/@EnableAutoSave", SettingsSchema::Bool, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Save the session automatically"),
          "", "", "true", false },

        { "application/@AutoSaveInterval", SettingsSchema::Int, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Save every (ms)"), "", "", "300000", false },

        { "application/@EnableAutoLoad", SettingsSchema::Bool, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Notice a session written by another window"),
          "", "", "true", false },

        { "application/@AutoLoadInterval", SettingsSchema::Int, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Check every (ms)"), "", "", "1000", false },

        { "application/@MaxBackUpGenerationCount", SettingsSchema::Int, "session",
          QT_TRANSLATE_NOOP("SettingsSchema", "Backups kept"), "", "", "5", false },

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
                      QCoreApplication::translate("SettingsSchema", "Tree overview"));
    list << qMakePair(QStringLiteral("files"),
                      QCoreApplication::translate("SettingsSchema", "File browser"));
    list << qMakePair(QStringLiteral("page"),
                      QCoreApplication::translate("SettingsSchema", "Pages"));
    list << qMakePair(QStringLiteral("accesskey"),
                      QCoreApplication::translate("SettingsSchema", "Access keys"));
    list << qMakePair(QStringLiteral("network"),
                      QCoreApplication::translate("SettingsSchema", "Network"));
    list << qMakePair(QStringLiteral("session"),
                      QCoreApplication::translate("SettingsSchema", "Session"));
    list << qMakePair(QStringLiteral("commands"),
                      QCoreApplication::translate("SettingsSchema", "External commands"));
    return list;
}

namespace {

    const struct { const char *key, *was, *now; } RENAMED[] = {
        { "webview/@SuspendHiddenViews", "Off", "Active" },
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
    case Choice:   return QJsonValue(Canonical(item, value.value<QString>()));
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
