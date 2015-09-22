#define TREEBANK_MOUSEMAP                                               \
    m_MouseMap[QStringLiteral("ExtraButton1")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("ExtraButton2")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelUp")] = QStringLiteral("ZoomIn"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelDown")] = QStringLiteral("ZoomOut"); \
    m_MouseMap[QStringLiteral("Shift+WheelUp")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Shift+WheelDown")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("RightButton+WheelUp")] = QStringLiteral("PrevView"); \
    m_MouseMap[QStringLiteral("RightButton+WheelDown")] = QStringLiteral("NextView");

#define WEBVIEW_MOUSEMAP                                                \
    m_MouseMap[QStringLiteral("ExtraButton1")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("ExtraButton2")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelUp")] = QStringLiteral("ZoomIn"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelDown")] = QStringLiteral("ZoomOut"); \
    m_MouseMap[QStringLiteral("Shift+WheelUp")] = QStringLiteral("Forward"); \
    m_MouseMap[QStringLiteral("Shift+WheelDown")] = QStringLiteral("Back"); \
    m_MouseMap[QStringLiteral("RightButton+WheelUp")] = QStringLiteral("PrevView"); \
    m_MouseMap[QStringLiteral("RightButton+WheelDown")] = QStringLiteral("NextView");

#define THUMBLIST_MOUSEMAP                                              \
    m_MouseMap[QStringLiteral("ExtraButton1")] = QStringLiteral("UpDirectory"); \
    m_MouseMap[QStringLiteral("ExtraButton2")] = QStringLiteral("DownDirectory"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelUp")] = QStringLiteral("ZoomIn"); \
    m_MouseMap[QStringLiteral("Ctrl+WheelDown")] = QStringLiteral("ZoomOut"); \
    m_MouseMap[QStringLiteral("Shift+WheelUp")] = QStringLiteral("SwitchNodeCollectionTypeReverse"); \
    m_MouseMap[QStringLiteral("Shift+WheelDown")] = QStringLiteral("SwitchNodeCollectionType");

#define WEBVIEW_RIGHTGESTURE                                            \
  /*m_RightGestureMap[QStringLiteral("U")]   = QStringLiteral("UpDirectory"); \
    m_RightGestureMap[QStringLiteral("D")]   = QStringLiteral("Reload"); \
    m_RightGestureMap[QStringLiteral("R")]   = QStringLiteral("Forward"); \
    m_RightGestureMap[QStringLiteral("L")]   = QStringLiteral("Back");  \
    m_RightGestureMap[QStringLiteral("UR")]  = QStringLiteral("DisplayHistTree"); \
    m_RightGestureMap[QStringLiteral("UL")]  = QStringLiteral("DisplayViewTree"); \
    m_RightGestureMap[QStringLiteral("DR")]  = QStringLiteral("Close"); \
    m_RightGestureMap[QStringLiteral("DL")]  = QStringLiteral("Restore"); \
    m_RightGestureMap[QStringLiteral("U,D")] = QStringLiteral("Recreate"); \
    m_RightGestureMap[QStringLiteral("D,U")] = QStringLiteral("Stop");  \
    m_RightGestureMap[QStringLiteral("U,R")] = QStringLiteral("NextView"); \
    m_RightGestureMap[QStringLiteral("U,L")] = QStringLiteral("PrevView");*/ \
    m_RightGestureMap[QStringLiteral("U")]   = QStringLiteral("UpDirectory"); \
    m_RightGestureMap[QStringLiteral("D")]   = QStringLiteral("NewViewNode"); \
    m_RightGestureMap[QStringLiteral("R")]   = QStringLiteral("Forward"); \
    m_RightGestureMap[QStringLiteral("L")]   = QStringLiteral("Back");  \
    m_RightGestureMap[QStringLiteral("U,D")] = QStringLiteral("Reload"); \
    m_RightGestureMap[QStringLiteral("D,U")] = QStringLiteral("Stop");  \
    m_RightGestureMap[QStringLiteral("U,R")] = QStringLiteral("NextView"); \
    m_RightGestureMap[QStringLiteral("U,L")] = QStringLiteral("PrevView"); \
    m_RightGestureMap[QStringLiteral("R,U")] = QStringLiteral("DisplayHistTree"); \
    m_RightGestureMap[QStringLiteral("L,U")] = QStringLiteral("DisplayViewTree"); \
    m_RightGestureMap[QStringLiteral("D,R")] = QStringLiteral("Close"); \
    m_RightGestureMap[QStringLiteral("D,L")] = QStringLiteral("Restore");

#define WEBVIEW_LEFTGESTURE                                             \
  /*m_LeftGestureMap[QStringLiteral("U")]  = QStringLiteral("OpenOnRootForeground"); \
    m_LeftGestureMap[QStringLiteral("D")]  = QStringLiteral("OpenInNewDirectoryForeground"); \
    m_LeftGestureMap[QStringLiteral("R")]  = QStringLiteral("OpenInNewHistNodeForeground"); \
    m_LeftGestureMap[QStringLiteral("L")]  = QStringLiteral("OpenInNewViewNodeForeground"); \
    m_LeftGestureMap[QStringLiteral("UR")] = QStringLiteral("OpenInNewHistNodeBackground"); \
    m_LeftGestureMap[QStringLiteral("UL")] = QStringLiteral("OpenOnRootBackground"); \
    m_LeftGestureMap[QStringLiteral("DR")] = QStringLiteral("OpenInNewDirectoryBackground"); \
    m_LeftGestureMap[QStringLiteral("DL")] = QStringLiteral("OpenInNewViewNodeBackground");*/ \
    m_LeftGestureMap[QStringLiteral("U")]   = QStringLiteral("OpenOnRootForeground"); \
    m_LeftGestureMap[QStringLiteral("D")]   = QStringLiteral("OpenInNewDirectoryForeground"); \
    m_LeftGestureMap[QStringLiteral("R")]   = QStringLiteral("OpenInNewHistNodeForeground"); \
    m_LeftGestureMap[QStringLiteral("L")]   = QStringLiteral("OpenInNewViewNodeForeground"); \
    m_LeftGestureMap[QStringLiteral("U,R")] = QStringLiteral("OpenInNewHistNodeBackground"); \
    m_LeftGestureMap[QStringLiteral("U,L")] = QStringLiteral("OpenOnRootBackground"); \
    m_LeftGestureMap[QStringLiteral("D,R")] = QStringLiteral("OpenInNewDirectoryBackground"); \
    m_LeftGestureMap[QStringLiteral("D,L")] = QStringLiteral("OpenInNewViewNodeBackground");
