#ifndef MOUSEMAP_HPP
#define MOUSEMAP_HPP

#include <QtGlobal>

struct DefaultGesture {
    const char *input;
    const char *action;
};

#ifdef Q_OS_MAC
inline constexpr DefaultGesture TREEBANK_MOUSEMAP[] = {
    { "ExtraButton1", "Back" },
    { "ExtraButton2", "Forward" },
    { "RightButton+LeftButton", "Back" },
    { "LeftButton+RightButton", "Forward" },
    { "Ctrl+WheelUp", "ZoomIn" },
    { "Ctrl+WheelDown", "ZoomOut" },
    { "Shift+WheelUp", "Forward" },
    { "Shift+WheelDown", "Back" },
};
#else
inline constexpr DefaultGesture TREEBANK_MOUSEMAP[] = {
    { "ExtraButton1", "Back" },
    { "ExtraButton2", "Forward" },
    { "RightButton+LeftButton", "Back" },
    { "LeftButton+RightButton", "Forward" },
    { "Ctrl+WheelUp", "ZoomIn" },
    { "Ctrl+WheelDown", "ZoomOut" },
    { "Shift+WheelUp", "Forward" },
    { "Shift+WheelDown", "Back" },
    { "RightButton+WheelUp", "PrevView" },
    { "RightButton+WheelDown", "NextView" },
};
#endif

inline constexpr DefaultGesture THUMBLIST_MOUSEMAP[] = {
    { "ExtraButton1", "UpDirectory" },
    { "ExtraButton2", "DownDirectory" },
    { "Ctrl+WheelUp", "ZoomIn" },
    { "Ctrl+WheelDown", "ZoomOut" },
    { "Shift+WheelUp", "SwitchNodeCollectionTypeReverse" },
    { "Shift+WheelDown", "SwitchNodeCollectionType" },
};

inline constexpr DefaultGesture WEBVIEW_RIGHTGESTURE[] = {
    { "U", "UpDirectory" },
    { "D", "NewViewNode" },
    { "R", "Forward" },
    { "L", "Back" },
    { "U,D", "Reload" },
    { "D,U", "Stop" },
    { "U,R", "NextView" },
    { "U,L", "PrevView" },
    { "L,U", "DisplayViewTree" },
    { "R,U", "CloneViewNode" },
    { "D,R", "Close" },
    { "D,L", "Restore" },
    { "L,R", "OpenCommand" },
    { "R,L", "OpenCommand" },
};

inline constexpr DefaultGesture WEBVIEW_DRAGGESTURE[] = {
    { "U", "OpenOnRootForeground" },
    { "D", "OpenInNewDirectoryForeground" },
    { "L", "OpenInNewViewNodeForeground" },
    { "U,L", "OpenOnRootBackground" },
    { "D,R", "OpenInNewDirectoryBackground" },
    { "D,L", "OpenInNewViewNodeBackground" },
};

inline constexpr DefaultGesture WEBVIEW_SCROLLGESTURE[] = {
    { "U,R", "Back" },
    { "U,L", "Forward" },
    { "U,D", "Reload" },
    { "D,U", "Stop" },
    { "D,R", "Close" },
    { "D,L", "Restore" },
};

#endif
