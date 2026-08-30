#include "switch.hpp"

#include "commandmap.hpp"

#include <QRegularExpression>

#include <iterator>

namespace CommandMap {

static const Entry TheEntries[] = {
    { "[uU]p",                      "Up",       Signal },
    { "[dD](?:ow)?n",               "Down",     Signal },
    { "[rR]ight",                   "Right",    Signal },
    { "[lL]eft",                    "Left",     Signal },
    { "[hH]ome",                    "Home",     Signal },
    { "[eE]nd",                     "End",      Signal },
    { "[pP](?:g|age)?[uU]p",        "PageUp",   Signal },
    { "[pP](?:g|age)?[dD](?:ow)?n", "PageDown", Signal },

    { "[iI]mport",                                 "Import",                Signal },
    { "[eE]xport",                                 "Export",                Signal },
    { "[aA]bout(?:[vV]anilla)?",                   "AboutVanilla",          Signal },
    { "[aA]bout[qQ]t",                             "AboutQt",               Signal },
    { "(?:[oO]pen)?[dD]ir(?:ectory)?[sS]ettings?", "OpenDirectorySettings", Signal },
    { "(?:[oO]pen)?[sS]ettings?",                  "OpenSettings",          Signal },
    { "[qQ]uit",                                   "Quit",                  Signal },
    { "[cC]lear[cC]ookies?",                       "ClearCookies",          Signal },
    { "[cC]lear(?:[hH]ttp)?[cC]ache",              "ClearHttpCache",        Signal },
    { "[cC]lear(?:[vV]isited)?[lL]inks?",          "ClearVisitedLinks",     Signal },

    { "(?:[tT]oggle)?[nN]otifier",                  "ToggleNotifier",   Signal },
    { "(?:[tT]oggle)?[rR]eceiver",                  "ToggleReceiver",   Signal },
    { "(?:[tT]oggle)?[mM]enu[bB]ar",                "ToggleMenuBar",    Signal },
    { "(?:[tT]oggle)?(?:[tT]ree|[tT]ab)[bB]ar",     "ToggleTreeBar",    Signal },
    { "(?:[tT]oggle)?(?:[tT]ool|[aA]ddress)[bB]ar", "ToggleToolBar",    Signal },
    { "(?:[tT]oggle)?[fF]ull[sS]creen",             "ToggleFullScreen", Signal },
    { "[tT]oggle[mM]aximized",                      "ToggleMaximized",  Signal },
    { "[mM]aximize(?:[wW]indow)?",                  "ToggleMaximized",  Signal },
    { "[tT]oggle[mM]inimized",                      "ToggleMinimized",  Signal },
    { "[mM]inimize(?:[wW]indow)?",                  "ToggleMinimized",  Signal },
    { "[tT]oggle[sS]haded",                         "ToggleShaded",     Signal },
    { "[sS]hade(?:[wW]indow)?",                     "ShadeWindow",      Signal },
    { "[uU]nshade(?:[wW]indow)?",                   "UnshadeWindow",    Signal },
    { "[nN]ew[wW]indow",                            "NewWindow",        Signal },
    { "[cC]lose[wW]indow",                          "CloseWindow",      Signal },
    { "[sS]witch(?:[wW]indow)?",                    "SwitchWindow",     Signal },
    { "[nN]ext[wW]indow",                           "NextWindow",       Signal },
    { "[pP]rev(?:ious)?[wW]indow",                  "PrevWindow",       Signal },

    { "[bB]ack(?:ward)?",                 "Back",              Signal },
    { "[fF]orward",                       "Forward",           Signal },
    { "[rR]ewind",                        "Rewind",            Signal },
    { "[fF]ast[fF]orward",                "FastForward",       Signal },
    { "[uU]pdir(?:ectory)?",              "UpDirectory",       Signal },
    { "[cC]lose",                         "Close",             Signal },
    { "[rR]estore",                       "Restore",           Signal },
    { "[rR]ecreate",                      "Recreate",          Signal },
    { "[nN]ext(?:[vV]iew)?",              "NextView",          Signal },
    { "[pP]rev(?:ious)?(?:[vV]iew)?",     "PrevView",          Signal },
    { "[bB]ury(?:[vV]iew)?",              "BuryView",          Signal },
    { "[dD]ig(?:[vV]iew)?",               "DigView",           Signal },
    { "(?:1|[fF]ir)st(?:[vV]iew)?",       "FirstView",         Signal },
    { "(?:2|[sS]eco)nd(?:[vV]iew)?",      "SecondView",        Signal },
    { "(?:3|[tT]hi)rd(?:[vV]iew)?",       "ThirdView",         Signal },
    { "(?:4|[fF]our)th(?:[vV]iew)?",      "FourthView",        Signal },
    { "(?:5|[fF]if)th(?:[vV]iew)?",       "FifthView",         Signal },
    { "(?:6|[sS]ix)th(?:[vV]iew)?",       "SixthView",         Signal },
    { "(?:7|[sS]even)th(?:[vV]iew)?",     "SeventhView",       Signal },
    { "(?:8|[eE]igh)th(?:[vV]iew)?",      "EighthView",        Signal },
    { "(?:9|[nN]in)th(?:[vV]iew)?",       "NinthView",         Signal },
    { "(?:10|[tT]en)th(?:[vV]iew)?",      "TenthView",         Signal },
    { "[lL]ast(?:[vV]iew)?",              "LastView",          Signal },
    { "[nN]ew(?:[vV]iew)?(?:[nN]ode)?",   "NewViewNode",       Signal },
    { "[cC]lone(?:[vV]iew)?(?:[nN]ode)?", "CloneViewNode",     Signal },
    { "[lL]ocal[nN]ode",                  "MakeLocalNode",     Signal },
    { "(?:[dD]isplay)?[aA]ccess[kK]ey",   "DisplayAccessKey",  Signal },
    { "(?:[dD]isplay)?[vV]iew[tT]ree",    "DisplayViewTree",   Signal },
    { "(?:[dD]isplay)?[tT]rash[tT]ree",   "DisplayTrashTree",  Signal },
    { "[rR]elease[hH]idden[vV]iew",       "ReleaseHiddenView", Signal },
    { "(?:[oO]pen)?[cC]ommand(?:[lL]ine)?", "OpenCommand",     Signal },

    { "[cC]opy",                          "Copy",                 Signal },
    { "[cC]ut",                           "Cut",                  Signal },
    { "[pP]aste",                         "Paste",                Signal },
    { "[uU]ndo",                          "Undo",                 Signal },
    { "[rR]edo",                          "Redo",                 Signal },
    { "[sS]elect[aA]ll",                  "SelectAll",            Signal },
    { "[uU]n[sS]elect",                   "Unselect",             Signal },
    { "[rR]eload",                        "Reload",               Signal },
    { "[rR]eload[aA]nd[bB]ypass[cC]ache", "ReloadAndBypassCache", Signal },
    { "[sS]top",                          "Stop",                 Signal },
    { "[sS]top[aA]nd[uU]n[sS]elect",      "StopAndUnselect",      Signal },

    { "[pP]aste[aA]nd[mM]atch[sS]tyle",   "PasteAndMatchStyle",   Signal },
    { "(?:[tT]oggle)?[bB]old",            "ToggleBold",           Signal },
    { "(?:[tT]oggle)?[iI]talic",          "ToggleItalic",         Signal },
    { "(?:[tT]oggle)?[uU]nderline",       "ToggleUnderline",      Signal },
    { "(?:[tT]oggle)?[sS]trike(?:through)?", "ToggleStrikethrough", Signal },
    { "[aA]lign[lL]eft",                  "AlignLeft",            Signal },
    { "[aA]lign[cC]enter",                "AlignCenter",          Signal },
    { "[aA]lign[rR]ight",                 "AlignRight",           Signal },
    { "[aA]lign[jJ]ustified",             "AlignJustified",       Signal },
    { "[iI]ndent",                        "Indent",               Signal },
    { "[oO]utdent",                       "Outdent",              Signal },
    { "[iI]nsert[oO]rdered[lL]ist",       "InsertOrderedList",    Signal },
    { "[iI]nsert[uU]nordered[lL]ist",     "InsertUnorderedList",  Signal },
    { "[cC]hange[tT]ext[dD]irection[lL][tT][rR]", "ChangeTextDirectionLTR", Signal },
    { "[cC]hange[tT]ext[dD]irection[rR][tT][lL]", "ChangeTextDirectionRTL", Signal },

    { "[pP]rint",          "Print",       Signal },
    { "[sS]ave",           "Save",        Signal },
    { "[zZ]oom[iI]n",      "ZoomIn",      Signal },
    { "[zZ]oom[oO]ut",     "ZoomOut",     Signal },
    { "[vV]iew[sS]ource",  "ViewSource",  Signal },
    { "[aA]pply[sS]ource", "ApplySource", Signal },

    { "[iI]nspect(?:[eE]lement)?", "InspectElement", Signal },

    { "[cC]opy[uU]rl",                      "CopyUrl",          Signal },
    { "[cC]opy[tT]itle",                    "CopyTitle",        Signal },
    { "[cC]opy[pP]age[aA]s[lL]ink",         "CopyPageAsLink",   Signal },
    { "[cC]opy[sS]elected[hH]tml",          "CopySelectedHtml", Signal },
    { "[oO]pen(?:[wW]ith|[oO]n)[dD]efault", "OpenWithDefault",  Signal },
    { "[oO]pen(?:[wW]ith|[oO]n)",           "OpenWithCommand",  OpenWith },

    { "[cC]lick(?:[eE]lement)?", "ClickElement", Element },
    { "[fF]ocus(?:[eE]lement)?", "FocusElement", Element },
    { "[hH]over(?:[eE]lement)?", "HoverElement", Element },

    { "[lL]oad[lL]ink",                            "LoadLink",            Element },
    { "[oO]pen[lL]ink",                            "OpenLink",            Element },
    { "[dD]ownload[lL]ink",                        "DownloadLink",        Element },
    { "[cC]opy[lL]ink[uU]rl",                      "CopyLinkUrl",         Element },
    { "[cC]opy[lL]ink[hH]tml",                     "CopyLinkHtml",        Element },
    { "[oO]pen[lL]ink(?:[wW]ith|[oO]n)[dD]efault", "OpenLinkWithDefault", Element },

    { "[lL]oad[iI]mage",                            "LoadImage",            Element },
    { "[oO]pen[iI]mage",                            "OpenImage",            Element },
    { "[dD]ownload[iI]mage",                        "DownloadImage",        Element },
    { "[cC]opy[iI]mage",                            "CopyImage",            Element },
    { "[cC]opy[iI]mage[uU]rl",                      "CopyImageUrl",         Element },
    { "[cC]opy[iI]mage[hH]tml",                     "CopyImageHtml",        Element },
    { "[oO]pen[iI]mage(?:[wW]ith|[oO]n)[dD]efault", "OpenImageWithDefault", Element },

    { "[lL]oad[mM]edia",                            "LoadMedia",            Element },
    { "[oO]pen[mM]edia",                            "OpenMedia",            Element },
    { "[dD]ownload[mM]edia",                        "DownloadMedia",        Element },
    { "[tT]oggle[mM]edia[cC]ontrols",               "ToggleMediaControls",  Element },
    { "[tT]oggle[mM]edia[lL]oop",                   "ToggleMediaLoop",      Element },
    { "[tT]oggle[mM]edia[pP]lay[pP]ause",           "ToggleMediaPlayPause", Element },
    { "[tT]oggle[mM]edia[mM]ute",                   "ToggleMediaMute",      Element },
    { "[cC]opy[mM]edia[uU]rl",                      "CopyMediaUrl",         Element },
    { "[cC]opy[mM]edia[hH]tml",                     "CopyMediaHtml",        Element },
    { "[oO]pen[mM]edia(?:[wW]ith|[oO]n)[dD]efault", "OpenMediaWithDefault", Element },

    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode",                 "OpenInNewViewNode",                 Element },
    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory",                   "OpenInNewDirectory",                Element },
    { "[oO]pen(?:[lL]ink)?[oO]n[rR]oot",                              "OpenOnRoot",                        Element },
    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[fF]oreground",    "OpenInNewViewNodeForeground",       Element },
    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[fF]oreground",      "OpenInNewDirectoryForeground",      Element },
    { "[oO]pen(?:[lL]ink)?[oO]n[rR]oot[fF]oreground",                 "OpenOnRootForeground",              Element },
    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[bB]ackground",    "OpenInNewViewNodeBackground",       Element },
    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[bB]ackground",      "OpenInNewDirectoryBackground",      Element },
    { "[oO]pen(?:[lL]ink)?[oO]n[rR]oot[bB]ackground",                 "OpenOnRootBackground",              Element },
    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[tT]his[wW]indow", "OpenInNewViewNodeThisWindow",       Element },
    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[tT]his[wW]indow",   "OpenInNewDirectoryThisWindow",      Element },
    { "[oO]pen(?:[lL]ink)?[oO]n[rR]oot[tT]his[wW]indow",              "OpenOnRootThisWindow",              Element },
    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[nN]ew[wW]indow",  "OpenInNewViewNodeNewWindow",        Element },
    { "[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[nN]ew[wW]indow",    "OpenInNewDirectoryNewWindow",       Element },
    { "[oO]pen(?:[lL]ink)?[oO]n[rR]oot[nN]ew[wW]indow",               "OpenOnRootNewWindow",               Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode",                     "OpenImageInNewViewNode",            Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory",                       "OpenImageInNewDirectory",           Element },
    { "[oO]pen[iI]mage[oO]n[rR]oot",                                  "OpenImageOnRoot",                   Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[fF]oreground",        "OpenImageInNewViewNodeForeground",  Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[fF]oreground",          "OpenImageInNewDirectoryForeground", Element },
    { "[oO]pen[iI]mage[oO]n[rR]oot[fF]oreground",                     "OpenImageOnRootForeground",         Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[bB]ackground",        "OpenImageInNewViewNodeBackground",  Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[bB]ackground",          "OpenImageInNewDirectoryBackground", Element },
    { "[oO]pen[iI]mage[oO]n[rR]oot[bB]ackground",                     "OpenImageOnRootBackground",         Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[tT]his[wW]indow",     "OpenImageInNewViewNodeThisWindow",  Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[tT]his[wW]indow",       "OpenImageInNewDirectoryThisWindow", Element },
    { "[oO]pen[iI]mage[oO]n[rR]oot[tT]his[wW]indow",                  "OpenImageOnRootThisWindow",         Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[nN]ew[wW]indow",      "OpenImageInNewViewNodeNewWindow",   Element },
    { "[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[nN]ew[wW]indow",        "OpenImageInNewDirectoryNewWindow",  Element },
    { "[oO]pen[iI]mage[oO]n[rR]oot[nN]ew[wW]indow",                   "OpenImageOnRootNewWindow",          Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode",                     "OpenMediaInNewViewNode",            Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory",                       "OpenMediaInNewDirectory",           Element },
    { "[oO]pen[mM]edia[oO]n[rR]oot",                                  "OpenMediaOnRoot",                   Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode[fF]oreground",        "OpenMediaInNewViewNodeForeground",  Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory[fF]oreground",          "OpenMediaInNewDirectoryForeground", Element },
    { "[oO]pen[mM]edia[oO]n[rR]oot[fF]oreground",                     "OpenMediaOnRootForeground",         Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode[bB]ackground",        "OpenMediaInNewViewNodeBackground",  Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory[bB]ackground",          "OpenMediaInNewDirectoryBackground", Element },
    { "[oO]pen[mM]edia[oO]n[rR]oot[bB]ackground",                     "OpenMediaOnRootBackground",         Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode[tT]his[wW]indow",     "OpenMediaInNewViewNodeThisWindow",  Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory[tT]his[wW]indow",       "OpenMediaInNewDirectoryThisWindow", Element },
    { "[oO]pen[mM]edia[oO]n[rR]oot[tT]his[wW]indow",                  "OpenMediaOnRootThisWindow",         Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode[nN]ew[wW]indow",      "OpenMediaInNewViewNodeNewWindow",   Element },
    { "[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory[nN]ew[wW]indow",        "OpenMediaInNewDirectoryNewWindow",  Element },
    { "[oO]pen[mM]edia[oO]n[rR]oot[nN]ew[wW]indow",                   "OpenMediaOnRootNewWindow",          Element },

    { "[dD]eactivate",                                     "Deactivate",                      Signal },
    { "[rR]efresh",                                        "Refresh",                         Signal },
    { "[rR]efresh[nN]o[sS]croll",                          "RefreshNoScroll",                 Signal },
    { "[oO]pen[nN]ode",                                    "OpenNode",                        Signal },
    { "[oO]pen[nN]ode[oO]n[nN]ew[wW]indow",                "OpenNodeOnNewWindow",             Signal },
    { "[dD]elete[nN]ode",                                  "DeleteNode",                      Signal },
    { "[dD]elete[rR]ight[nN]ode",                          "DeleteRightNode",                 Signal },
    { "[dD]elete[lL]eft[nN]ode",                           "DeleteLeftNode",                  Signal },
    { "[dD]elete[oO]ther[nN]ode",                          "DeleteOtherNode",                 Signal },
    { "[pP]aste[nN]ode",                                   "PasteNode",                       Signal },
    { "[rR]estore[nN]ode",                                 "RestoreNode",                     Signal },
    { "[nN]ew[nN]ode",                                     "NewNode",                         Signal },
    { "[cC]lone[nN]ode",                                   "CloneNode",                       Signal },
    { "[uU]p[dD]irectory",                                 "UpDirectory",                     Signal },
    { "[dD]own[dD]irectory",                               "DownDirectory",                   Signal },
    { "[mM]ake[lL]ocal[nN]ode",                            "MakeLocalNode",                   Signal },
    { "[mM]ake[dD]irectory",                               "MakeDirectory",                   Signal },
    { "[mM]ake[dD]irectory[wW]ith[sS]elected[nN]ode",      "MakeDirectoryWithSelectedNode",   Signal },
    { "[mM]ake[dD]irectory[wW]ith[sS]ame[dD]omain[nN]ode", "MakeDirectoryWithSameDomainNode", Signal },
    { "[rR]ename[nN]ode",                                  "RenameNode",                      Signal },
    { "[cC]opy[nN]ode[uU]rl",                              "CopyNodeUrl",                     Signal },
    { "[cC]opy[nN]ode[tT]itle",                            "CopyNodeTitle",                   Signal },
    { "[cC]opy[nN]ode[aA]s[lL]ink",                        "CopyNodeAsLink",                  Signal },
    { "[oO]pen[nN]ode(?:[wW]ith|[oO]n)[dD]efault",         "OpenNodeWithDefault",             Signal },
    { "[oO]pen[nN]ode(?:[wW]ith|[oO]n)",                   "OpenNodeWithCommand",             OpenNodeWith },

    { "[tT]oggle[tT]rash",                              "ToggleTrash",                     Signal },
    { "[sS]croll[uU]p",                                 "ScrollUp",                        Signal },
    { "[sS]croll[dD]own",                               "ScrollDown",                      Signal },
    { "[pP]age[uU]p",                                   "PageUp",                          Signal },
    { "[pP]age[dD]own",                                 "PageDown",                        Signal },
    { "[nN]ext[pP]age",                                 "NextPage",                        Signal },
    { "[pP]rev(?:ious)?[pP]age",                         "PrevPage",                        Signal },
    { "[mM]ove[tT]o[uU]pper[iI]tem",                    "MoveToUpperItem",                 Signal },
    { "[mM]ove[tT]o[lL]ower[iI]tem",                    "MoveToLowerItem",                 Signal },
    { "[mM]ove[tT]o[rR]ight[iI]tem",                    "MoveToRightItem",                 Signal },
    { "[mM]ove[tT]o[lL]eft[iI]tem",                     "MoveToLeftItem",                  Signal },
    { "[mM]ove[tT]o[pP]rev[pP]age",                     "MoveToPrevPage",                  Signal },
    { "[mM]ove[tT]o[nN]ext[pP]age",                     "MoveToNextPage",                  Signal },
    { "[mM]ove[tT]o[fF]irst[iI]tem",                    "MoveToFirstItem",                 Signal },
    { "[mM]ove[tT]o[lL]ast[iI]tem",                     "MoveToLastItem",                  Signal },
    { "[sS]elect[tT]o[uU]pper[iI]tem",                  "SelectToUpperItem",               Signal },
    { "[sS]elect[tT]o[lL]ower[iI]tem",                  "SelectToLowerItem",               Signal },
    { "[sS]elect[tT]o[rR]ight[iI]tem",                  "SelectToRightItem",               Signal },
    { "[sS]elect[tT]o[lL]eft[iI]tem",                   "SelectToLeftItem",                Signal },
    { "[sS]elect[tT]o[pP]rev[pP]age",                   "SelectToPrevPage",                Signal },
    { "[sS]elect[tT]o[nN]ext[pP]age",                   "SelectToNextPage",                Signal },
    { "[sS]elect[tT]o[fF]irst[iI]tem",                  "SelectToFirstItem",               Signal },
    { "[sS]elect[tT]o[lL]ast[iI]tem",                   "SelectToLastItem",                Signal },
    { "[sS]elect[iI]tem",                               "SelectItem",                      Signal },
    { "[sS]elect[rR]ange",                              "SelectRange",                     Signal },
    { "[cC]lear[sS]election",                           "ClearSelection",                  Signal },
    { "[tT]ransfer[tT]o[uU]pper",                       "TransferToUpper",                 Signal },
    { "[tT]ransfer[tT]o[lL]ower",                       "TransferToLower",                 Signal },
    { "[tT]ransfer[tT]o[rR]ight",                       "TransferToRight",                 Signal },
    { "[tT]ransfer[tT]o[lL]eft",                        "TransferToLeft",                  Signal },
    { "[tT]ransfer[tT]o[pP]rev[pP]age",                 "TransferToPrevPage",              Signal },
    { "[tT]ransfer[tT]o[nN]ext[pP]age",                 "TransferToNextPage",              Signal },
    { "[tT]ransfer[tT]o[fF]irst",                       "TransferToFirst",                 Signal },
    { "[tT]ransfer[tT]o[lL]ast",                        "TransferToLast",                  Signal },
    { "[tT]ransfer[tT]o[uU]p[dD]irectory",              "TransferToUpDirectory",           Signal },
    { "[tT]ransfer[tT]o[dD]own[dD]irectory",            "TransferToDownDirectory",         Signal },
    { "[sS]witch[nN]ode[cC]ollection[tT]ype",           "SwitchNodeCollectionType",        Signal },
    { "[sS]witch[nN]ode[cC]ollection[tT]ype[rR]everse", "SwitchNodeCollectionTypeReverse", Signal },

    { "[rR]econf(?:ig(?:ure)?)?", "Reconfigure", Signal },
    { "[bB]lank",                 "Blank",       Blank },

    { "[oO]pen",                "Open",     Open },
    { "[lL]oad",                "Load",     Load },
    { "[qQ]uery",               "Query",    Query },
    { "[dD]ownload",            "Download", Download },
    { "(?:[sS]earch|[sS]eek)",  "Seek",     Seek },
    { "[sS]et(?:tings?)?",      "Set",      Set },
    { "[uU]n[sS]et(?:tings?)?", "Unset",    Unset },
    { "[kK]ey",                 "Key",      Key },
};

const QVector<Entry> &Entries(){
    static const QVector<Entry> entries(std::begin(TheEntries), std::end(TheEntries));
    return entries;
}

static const QVector<QRegularExpression> &Patterns(){
    static const QVector<QRegularExpression> patterns = []{
        QVector<QRegularExpression> list;
        for(const Entry &entry : Entries())
            list << QRegularExpression(QStringLiteral("\\A%1\\Z")
                                       .arg(QString::fromLatin1(entry.spelling)));
        return list;
    }();
    return patterns;
}

Kind Resolve(const QString &word, QString *action){
    if(action) action->clear();
    if(word.isEmpty()) return NotACommand;

    const QVector<Entry> &entries = Entries();
    const QVector<QRegularExpression> &patterns = Patterns();

    for(int i = 0; i < entries.length(); i++){
        if(!patterns[i].match(word).hasMatch()) continue;
        if(action) *action = QString::fromLatin1(entries[i].action);
        return entries[i].kind;
    }
    return NotACommand;
}

#define COLLECT_SIGNAL_ACTION(ACTION) << QStringLiteral(#ACTION)

bool IsSignalAction(const QString &action){
    static const QStringList actions = QStringList()
        COMMANDMAP_FOR_EACH_SIGNAL_ACTION(COLLECT_SIGNAL_ACTION);
    return actions.contains(action);
}

}
