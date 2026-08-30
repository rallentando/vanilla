#ifndef MOUSEMAP_HPP
#define MOUSEMAP_HPP

#ifdef Q_OS_MAC
#  define TREEBANK_MOUSEMAP                                             \
    m_MouseMap[QStringLiteral("ExtraButton1")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("ExtraButton2")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("RightButton+LeftButton")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("LeftButton+RightButton")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelUp")] = QStringLiteral("ZoomIn"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelDown")] = QStringLiteral("ZoomOut"); \
    m_MouseMap[QStringLiteral("Shift+WheelUp")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Shift+WheelDown")] = QStringLiteral("Back");

#  define WEBVIEW_MOUSEMAP                                              \
    m_MouseMap[QStringLiteral("ExtraButton1")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("ExtraButton2")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("RightButton+LeftButton")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("LeftButton+RightButton")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelUp")] = QStringLiteral("ZoomIn"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelDown")] = QStringLiteral("ZoomOut"); \
    m_MouseMap[QStringLiteral("Shift+WheelUp")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Shift+WheelDown")] = QStringLiteral("Back");
#else
#  define TREEBANK_MOUSEMAP                                             \
    m_MouseMap[QStringLiteral("ExtraButton1")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("ExtraButton2")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("RightButton+LeftButton")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("LeftButton+RightButton")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelUp")] = QStringLiteral("ZoomIn"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelDown")] = QStringLiteral("ZoomOut"); \
    m_MouseMap[QStringLiteral("Shift+WheelUp")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Shift+WheelDown")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("RightButton+WheelUp")] = QStringLiteral("PrevView"); \
    m_MouseMap[QStringLiteral("RightButton+WheelDown")] = QStringLiteral("NextView");

#  define WEBVIEW_MOUSEMAP                                              \
    m_MouseMap[QStringLiteral("ExtraButton1")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("ExtraButton2")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("RightButton+LeftButton")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("LeftButton+RightButton")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelUp")] = QStringLiteral("ZoomIn"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelDown")] = QStringLiteral("ZoomOut"); \
    m_MouseMap[QStringLiteral("Shift+WheelUp")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Shift+WheelDown")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("RightButton+WheelUp")] = QStringLiteral("PrevView"); \
    m_MouseMap[QStringLiteral("RightButton+WheelDown")] = QStringLiteral("NextView");
#endif

#define THUMBLIST_MOUSEMAP                                              \
    m_MouseMap[QStringLiteral("ExtraButton1")] = QStringLiteral("UpDirectory"); \
    m_MouseMap[QStringLiteral("ExtraButton2")] = QStringLiteral("DownDirectory"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelUp")] = QStringLiteral("ZoomIn"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelDown")] = QStringLiteral("ZoomOut"); \
    m_MouseMap[QStringLiteral("Shift+WheelUp")] = QStringLiteral("SwitchNodeCollectionTypeReverse"); \
    m_MouseMap[QStringLiteral("Shift+WheelDown")] = QStringLiteral("SwitchNodeCollectionType");

#define WEBVIEW_RIGHTGESTURE                                            \
 \
    m_RightGestureMap[QStringLiteral("U")]   = QStringLiteral("UpDirectory"); \
    m_RightGestureMap[QStringLiteral("D")]   = QStringLiteral("NewViewNode"); \
    m_RightGestureMap[QStringLiteral("R")]   = QStringLiteral("Forward"); \
    m_RightGestureMap[QStringLiteral("L")]   = QStringLiteral("Back");  \
    m_RightGestureMap[QStringLiteral("U,D")] = QStringLiteral("Reload"); \
    m_RightGestureMap[QStringLiteral("D,U")] = QStringLiteral("Stop");  \
    m_RightGestureMap[QStringLiteral("U,R")] = QStringLiteral("NextView"); \
    m_RightGestureMap[QStringLiteral("U,L")] = QStringLiteral("PrevView"); \
    m_RightGestureMap[QStringLiteral("L,U")] = QStringLiteral("DisplayViewTree"); \
    m_RightGestureMap[QStringLiteral("R,U")] = QStringLiteral("CloneViewNode"); \
    m_RightGestureMap[QStringLiteral("D,R")] = QStringLiteral("Close"); \
    m_RightGestureMap[QStringLiteral("D,L")] = QStringLiteral("Restore");

#define WEBVIEW_DRAGGESTURE                                             \
 \
    m_DragGestureMap[QStringLiteral("U")]   = QStringLiteral("OpenOnRootForeground"); \
    m_DragGestureMap[QStringLiteral("D")]   = QStringLiteral("OpenInNewDirectoryForeground"); \
    m_DragGestureMap[QStringLiteral("L")]   = QStringLiteral("OpenInNewViewNodeForeground"); \
    m_DragGestureMap[QStringLiteral("U,L")] = QStringLiteral("OpenOnRootBackground"); \
    m_DragGestureMap[QStringLiteral("D,R")] = QStringLiteral("OpenInNewDirectoryBackground"); \
    m_DragGestureMap[QStringLiteral("D,L")] = QStringLiteral("OpenInNewViewNodeBackground");

#define WEBVIEW_SCROLLGESTURE                                           \
    m_ScrollGestureMap[QStringLiteral("U,R")] = QStringLiteral("Back"); \
    m_ScrollGestureMap[QStringLiteral("U,L")] = QStringLiteral("Forward"); \
    m_ScrollGestureMap[QStringLiteral("U,D")] = QStringLiteral("Reload"); \
    m_ScrollGestureMap[QStringLiteral("D,U")] = QStringLiteral("Stop"); \
    m_ScrollGestureMap[QStringLiteral("D,R")] = QStringLiteral("Close"); \
    m_ScrollGestureMap[QStringLiteral("D,L")] = QStringLiteral("Restore");

#endif
