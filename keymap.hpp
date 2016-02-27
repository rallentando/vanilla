
#define TREEBANK_KEYMAP                                                 \
    m_KeyMap[QKeySequence(QKeySequence::Copy)]          = QStringLiteral("Copy"); \
    m_KeyMap[QKeySequence(QKeySequence::Cut)]           = QStringLiteral("Cut"); \
    m_KeyMap[QKeySequence(QKeySequence::Paste)]         = QStringLiteral("Paste"); \
    m_KeyMap[QKeySequence(QKeySequence::Open)]          = QStringLiteral("Load"); \
    m_KeyMap[QKeySequence(QKeySequence::Save)]          = QStringLiteral("Save"); \
    m_KeyMap[QKeySequence(QKeySequence::Print)]         = QStringLiteral("Print"); \
    m_KeyMap[QKeySequence(QKeySequence::Quit)]          = QStringLiteral("Quit"); \
    m_KeyMap[QKeySequence(QKeySequence::Undo)]          = QStringLiteral("Undo"); \
    m_KeyMap[QKeySequence(QKeySequence::Redo)]          = QStringLiteral("Redo"); \
    m_KeyMap[QKeySequence(QKeySequence::Back)]          = QStringLiteral("Back"); \
    m_KeyMap[QKeySequence(QKeySequence::Forward)]       = QStringLiteral("Forward"); \
    m_KeyMap[QKeySequence(QKeySequence::Refresh)]       = QStringLiteral("Reload"); \
    m_KeyMap[QKeySequence(QKeySequence::ZoomIn)]        = QStringLiteral("ZoomIn"); \
    m_KeyMap[QKeySequence(QKeySequence::ZoomOut)]       = QStringLiteral("ZoomOut"); \
    m_KeyMap[QKeySequence(QKeySequence::NextChild)]     = QStringLiteral("NextView"); \
    m_KeyMap[QKeySequence(QKeySequence::PreviousChild)] = QStringLiteral("PrevView"); \
    m_KeyMap[QKeySequence(QKeySequence::Find)]          = QStringLiteral("OpenTextSeeker"); \
    m_KeyMap[QKeySequence(QKeySequence::SelectAll)]     = QStringLiteral("SelectAll"); \
    m_KeyMap[QKeySequence(QKeySequence::AddTab)]        = QStringLiteral("NewViewNode"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_F4)]    = QStringLiteral("Quit"); \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_Left)]  = QStringLiteral("Back"); \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_Right)] = QStringLiteral("Forward"); \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_Up)]    = QStringLiteral("DigView"); \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_Down)]  = QStringLiteral("BuryView"); \
    m_KeyMap[QKeySequence(Qt::Key_Backspace)]       = QStringLiteral("Back"); \
    m_KeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Backspace)] = QStringLiteral("Forward"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_Q)] = QStringLiteral("Quit"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_W)] = QStringLiteral("Close"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_C)] = QStringLiteral("Copy"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_X)] = QStringLiteral("Cut"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_V)] = QStringLiteral("Paste"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_O)] = QStringLiteral("Load"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_S)] = QStringLiteral("Save"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_P)] = QStringLiteral("Print"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_Z)] = QStringLiteral("Undo"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_Y)] = QStringLiteral("Redo"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_F)] = QStringLiteral("OpenTextSeeker"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_Q)]        = QStringLiteral("Close"); \
    m_KeyMap[QKeySequence(Qt::Key_W)]        = QStringLiteral("Restore"); \
    m_KeyMap[QKeySequence(Qt::Key_E)]        = QStringLiteral("UpDirectory"); \
    m_KeyMap[QKeySequence(Qt::Key_R)]        = QStringLiteral("Home");  \
    m_KeyMap[QKeySequence(Qt::Key_T)]        = QStringLiteral("End");   \
    m_KeyMap[QKeySequence(Qt::Key_Y)]        = QStringLiteral("End");   \
    m_KeyMap[QKeySequence(Qt::Key_U)]        = QStringLiteral("Home");  \
    m_KeyMap[QKeySequence(Qt::Key_I)]        = QStringLiteral("UpDirectory"); \
    m_KeyMap[QKeySequence(Qt::Key_O)]        = QStringLiteral("Restore"); \
    m_KeyMap[QKeySequence(Qt::Key_P)]        = QStringLiteral("Close"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_A)]        = QStringLiteral("DisplayAccessKey"); \
    m_KeyMap[QKeySequence(Qt::Key_S)]        = QStringLiteral("OpenQueryEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_D)]        = QStringLiteral("OpenUrlEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_F)]        = QStringLiteral("DisplayViewTree"); \
    m_KeyMap[QKeySequence(Qt::Key_G)]        = QStringLiteral("DisplayHistTree"); \
    m_KeyMap[QKeySequence(Qt::Key_H)]        = QStringLiteral("DisplayHistTree"); \
    m_KeyMap[QKeySequence(Qt::Key_J)]        = QStringLiteral("DisplayViewTree"); \
    m_KeyMap[QKeySequence(Qt::Key_K)]        = QStringLiteral("OpenUrlEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_L)]        = QStringLiteral("OpenQueryEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_Semicolon)] = QStringLiteral("DisplayAccessKey"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_Z)]        = QStringLiteral("Back");  \
    m_KeyMap[QKeySequence(Qt::Key_X)]        = QStringLiteral("Forward"); \
    m_KeyMap[QKeySequence(Qt::Key_C)]        = QStringLiteral("PrevView"); \
    m_KeyMap[QKeySequence(Qt::Key_V)]        = QStringLiteral("NextView"); \
    m_KeyMap[QKeySequence(Qt::Key_B)]        = QStringLiteral("Reload"); \
    m_KeyMap[QKeySequence(Qt::Key_N)]        = QStringLiteral("PrevView"); \
    m_KeyMap[QKeySequence(Qt::Key_M)]        = QStringLiteral("NextView"); \
    m_KeyMap[QKeySequence(Qt::Key_Comma)]    = QStringLiteral("Back");  \
    m_KeyMap[QKeySequence(Qt::Key_Period)]   = QStringLiteral("Forward"); \
    m_KeyMap[QKeySequence(Qt::Key_Slash)]    = QStringLiteral("NoAction"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_F1)]       = QStringLiteral("OpenUrlEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_F2)]       = QStringLiteral("Load");  \
    m_KeyMap[QKeySequence(Qt::Key_F3)]       = QStringLiteral("NewWindow"); \
    m_KeyMap[QKeySequence(Qt::Key_F4)]       = QStringLiteral("CloseWindow"); \
    m_KeyMap[QKeySequence(Qt::Key_F5)]       = QStringLiteral("Reload"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_F5)] = QStringLiteral("ReloadAndBypassCache"); \
    m_KeyMap[QKeySequence(Qt::Key_F6)]       = QStringLiteral("PrevView"); \
    m_KeyMap[QKeySequence(Qt::Key_F7)]       = QStringLiteral("NextView"); \
    m_KeyMap[QKeySequence(Qt::Key_F8)]       = QStringLiteral("SwitchWindow"); \
    m_KeyMap[QKeySequence(Qt::Key_F9)]       = QStringLiteral("DisplayViewTree"); \
    m_KeyMap[QKeySequence(Qt::Key_F10)]      = QStringLiteral("DisplayHistTree"); \
    m_KeyMap[QKeySequence(Qt::Key_F11)]      = QStringLiteral("ToggleFullScreen"); \
    m_KeyMap[QKeySequence(Qt::Key_F12)]      = QStringLiteral("DisplayAccessKey"); \
    m_KeyMap[QKeySequence(Qt::Key_Escape)]   = QStringLiteral("Stop");  \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_Up)]       = QStringLiteral("Up");    \
    m_KeyMap[QKeySequence(Qt::Key_Down)]     = QStringLiteral("Down");  \
    m_KeyMap[QKeySequence(Qt::Key_Right)]    = QStringLiteral("Right"); \
    m_KeyMap[QKeySequence(Qt::Key_Left)]     = QStringLiteral("Left");  \
    m_KeyMap[QKeySequence(Qt::Key_PageUp)]   = QStringLiteral("PageUp"); \
    m_KeyMap[QKeySequence(Qt::Key_PageDown)] = QStringLiteral("PageDown"); \
    m_KeyMap[QKeySequence(Qt::Key_Home)]     = QStringLiteral("Home");  \
    m_KeyMap[QKeySequence(Qt::Key_End)]      = QStringLiteral("End");   \
                                                                        \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_X)] = QStringLiteral("OpenCommand");

#define WEBVIEW_KEYMAP                                                  \
    m_KeyMap[QKeySequence(QKeySequence::Copy)]          = QStringLiteral("Copy"); \
    m_KeyMap[QKeySequence(QKeySequence::Cut)]           = QStringLiteral("Cut"); \
    m_KeyMap[QKeySequence(QKeySequence::Paste)]         = QStringLiteral("Paste"); \
    m_KeyMap[QKeySequence(QKeySequence::Open)]          = QStringLiteral("Load"); \
    m_KeyMap[QKeySequence(QKeySequence::Save)]          = QStringLiteral("Save"); \
    m_KeyMap[QKeySequence(QKeySequence::Print)]         = QStringLiteral("Print"); \
    m_KeyMap[QKeySequence(QKeySequence::Quit)]          = QStringLiteral("Quit"); \
    m_KeyMap[QKeySequence(QKeySequence::Undo)]          = QStringLiteral("Undo"); \
    m_KeyMap[QKeySequence(QKeySequence::Redo)]          = QStringLiteral("Redo"); \
    m_KeyMap[QKeySequence(QKeySequence::Back)]          = QStringLiteral("Back"); \
    m_KeyMap[QKeySequence(QKeySequence::Forward)]       = QStringLiteral("Forward"); \
    m_KeyMap[QKeySequence(QKeySequence::Refresh)]       = QStringLiteral("Reload"); \
    m_KeyMap[QKeySequence(QKeySequence::ZoomIn)]        = QStringLiteral("ZoomIn"); \
    m_KeyMap[QKeySequence(QKeySequence::ZoomOut)]       = QStringLiteral("ZoomOut"); \
    m_KeyMap[QKeySequence(QKeySequence::NextChild)]     = QStringLiteral("NextView"); \
    m_KeyMap[QKeySequence(QKeySequence::PreviousChild)] = QStringLiteral("PrevView"); \
    m_KeyMap[QKeySequence(QKeySequence::Find)]          = QStringLiteral("OpenTextSeeker"); \
    m_KeyMap[QKeySequence(QKeySequence::SelectAll)]     = QStringLiteral("SelectAll"); \
    m_KeyMap[QKeySequence(QKeySequence::AddTab)]        = QStringLiteral("NewViewNode"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_F4)]    = QStringLiteral("Quit"); \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_Left)]  = QStringLiteral("Back"); \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_Right)] = QStringLiteral("Forward"); \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_Up)]    = QStringLiteral("DigView"); \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_Down)]  = QStringLiteral("BuryView"); \
    m_KeyMap[QKeySequence(Qt::Key_Backspace)]       = QStringLiteral("Back"); \
    m_KeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Backspace)] = QStringLiteral("Forward"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_Q)] = QStringLiteral("Quit"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_W)] = QStringLiteral("Close"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_C)] = QStringLiteral("Copy"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_X)] = QStringLiteral("Cut"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_V)] = QStringLiteral("Paste"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_O)] = QStringLiteral("Load"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_S)] = QStringLiteral("Save"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_P)] = QStringLiteral("Print"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_Z)] = QStringLiteral("Undo"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_Y)] = QStringLiteral("Redo"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_F)] = QStringLiteral("OpenTextSeeker"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_Q)]        = QStringLiteral("Close"); \
    m_KeyMap[QKeySequence(Qt::Key_W)]        = QStringLiteral("Restore"); \
    m_KeyMap[QKeySequence(Qt::Key_E)]        = QStringLiteral("UpDirectory"); \
    m_KeyMap[QKeySequence(Qt::Key_R)]        = QStringLiteral("Home");  \
    m_KeyMap[QKeySequence(Qt::Key_T)]        = QStringLiteral("End");   \
    m_KeyMap[QKeySequence(Qt::Key_Y)]        = QStringLiteral("End");   \
    m_KeyMap[QKeySequence(Qt::Key_U)]        = QStringLiteral("Home");  \
    m_KeyMap[QKeySequence(Qt::Key_I)]        = QStringLiteral("UpDirectory"); \
    m_KeyMap[QKeySequence(Qt::Key_O)]        = QStringLiteral("Restore"); \
    m_KeyMap[QKeySequence(Qt::Key_P)]        = QStringLiteral("Close"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_A)]        = QStringLiteral("DisplayAccessKey"); \
    m_KeyMap[QKeySequence(Qt::Key_S)]        = QStringLiteral("OpenQueryEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_D)]        = QStringLiteral("OpenUrlEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_F)]        = QStringLiteral("DisplayViewTree"); \
    m_KeyMap[QKeySequence(Qt::Key_G)]        = QStringLiteral("DisplayHistTree"); \
    m_KeyMap[QKeySequence(Qt::Key_H)]        = QStringLiteral("DisplayHistTree"); \
    m_KeyMap[QKeySequence(Qt::Key_J)]        = QStringLiteral("DisplayViewTree"); \
    m_KeyMap[QKeySequence(Qt::Key_K)]        = QStringLiteral("OpenUrlEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_L)]        = QStringLiteral("OpenQueryEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_Semicolon)] = QStringLiteral("DisplayAccessKey"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_Z)]        = QStringLiteral("Back");  \
    m_KeyMap[QKeySequence(Qt::Key_X)]        = QStringLiteral("Forward"); \
    m_KeyMap[QKeySequence(Qt::Key_C)]        = QStringLiteral("PrevView"); \
    m_KeyMap[QKeySequence(Qt::Key_V)]        = QStringLiteral("NextView"); \
    m_KeyMap[QKeySequence(Qt::Key_B)]        = QStringLiteral("Reload"); \
    m_KeyMap[QKeySequence(Qt::Key_N)]        = QStringLiteral("PrevView"); \
    m_KeyMap[QKeySequence(Qt::Key_M)]        = QStringLiteral("NextView"); \
    m_KeyMap[QKeySequence(Qt::Key_Comma)]    = QStringLiteral("Back");  \
    m_KeyMap[QKeySequence(Qt::Key_Period)]   = QStringLiteral("Forward"); \
    m_KeyMap[QKeySequence(Qt::Key_Slash)]    = QStringLiteral("NoAction"); \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_F1)]       = QStringLiteral("OpenUrlEditor"); \
    m_KeyMap[QKeySequence(Qt::Key_F2)]       = QStringLiteral("Load");  \
    m_KeyMap[QKeySequence(Qt::Key_F3)]       = QStringLiteral("NewWindow"); \
    m_KeyMap[QKeySequence(Qt::Key_F4)]       = QStringLiteral("CloseWindow"); \
    m_KeyMap[QKeySequence(Qt::Key_F5)]       = QStringLiteral("Reload"); \
    m_KeyMap[QKeySequence(Qt::CTRL + Qt::Key_F5)] = QStringLiteral("ReloadAndBypassCache"); \
    m_KeyMap[QKeySequence(Qt::Key_F6)]       = QStringLiteral("PrevView"); \
    m_KeyMap[QKeySequence(Qt::Key_F7)]       = QStringLiteral("NextView"); \
    m_KeyMap[QKeySequence(Qt::Key_F8)]       = QStringLiteral("SwitchWindow"); \
    m_KeyMap[QKeySequence(Qt::Key_F9)]       = QStringLiteral("DisplayViewTree"); \
    m_KeyMap[QKeySequence(Qt::Key_F10)]      = QStringLiteral("DisplayHistTree"); \
    m_KeyMap[QKeySequence(Qt::Key_F11)]      = QStringLiteral("ToggleFullScreen"); \
    m_KeyMap[QKeySequence(Qt::Key_F12)]      = QStringLiteral("DisplayAccessKey"); \
    m_KeyMap[QKeySequence(Qt::Key_Escape)]   = QStringLiteral("Stop");  \
                                                                        \
    m_KeyMap[QKeySequence(Qt::Key_Up)]       = QStringLiteral("Up");    \
    m_KeyMap[QKeySequence(Qt::Key_Down)]     = QStringLiteral("Down");  \
    m_KeyMap[QKeySequence(Qt::Key_Right)]    = QStringLiteral("Right"); \
    m_KeyMap[QKeySequence(Qt::Key_Left)]     = QStringLiteral("Left");  \
    m_KeyMap[QKeySequence(Qt::Key_PageUp)]   = QStringLiteral("PageUp"); \
    m_KeyMap[QKeySequence(Qt::Key_PageDown)] = QStringLiteral("PageDown"); \
    m_KeyMap[QKeySequence(Qt::Key_Home)]     = QStringLiteral("Home");  \
    m_KeyMap[QKeySequence(Qt::Key_End)]      = QStringLiteral("End");   \
                                                                        \
    m_KeyMap[QKeySequence(Qt::ALT + Qt::Key_X)] = QStringLiteral("OpenCommand");

#define THUMBLIST_KEYMAP                                                \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::Paste)]         = QStringLiteral("PasteNode"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::Quit)]          = QStringLiteral("Quit"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::Back)]          = QStringLiteral("UpDirectory"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::Forward)]       = QStringLiteral("DownDirectory"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::Refresh)]       = QStringLiteral("Refresh"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::ZoomIn)]        = QStringLiteral("ZoomIn"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::ZoomOut)]       = QStringLiteral("ZoomOut"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::NextChild)]     = QStringLiteral("MoveToRightItem"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::PreviousChild)] = QStringLiteral("MoveToLeftItem"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::Find)]          = QStringLiteral("OpenTextSeeker"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::SelectAll)]     = QStringLiteral("SelectAll"); \
    m_ThumbListKeyMap[QKeySequence(QKeySequence::AddTab)]        = QStringLiteral("NewNode"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::ALT + Qt::Key_F4)]    = QStringLiteral("Quit"); \
    m_ThumbListKeyMap[QKeySequence(Qt::ALT + Qt::Key_Left)]  = QStringLiteral("UpDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::ALT + Qt::Key_Right)] = QStringLiteral("DownDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::ALT + Qt::Key_Up)]    = QStringLiteral("UpDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::ALT + Qt::Key_Down)]  = QStringLiteral("DownDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Backspace)]       = QStringLiteral("UpDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Backspace)] = QStringLiteral("DownDirectory"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Q)] = QStringLiteral("Quit"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_W)] = QStringLiteral("Close"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_V)] = QStringLiteral("PasteNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_F)] = QStringLiteral("OpenTextSeeker"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Comma)] = QStringLiteral("ScrollUp"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Period)] = QStringLiteral("ScrollDown"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Q)] = QStringLiteral("DeleteNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_W)] = QStringLiteral("RestoreNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_E)] = QStringLiteral("MoveToUpperItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_R)] = QStringLiteral("RestoreNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_T)] = QStringLiteral("ToggleTrash"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Y)] = QStringLiteral("ToggleTrash"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_U)] = QStringLiteral("RestoreNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_I)] = QStringLiteral("MoveToUpperItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_O)] = QStringLiteral("RestoreNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_P)] = QStringLiteral("DeleteNode"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_A)] = QStringLiteral("OpenNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_S)] = QStringLiteral("MoveToLeftItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_D)] = QStringLiteral("MoveToLowerItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F)] = QStringLiteral("MoveToRightItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_G)] = QStringLiteral("RenameNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_H)] = QStringLiteral("RenameNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_J)] = QStringLiteral("MoveToLeftItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_K)] = QStringLiteral("MoveToLowerItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_L)] = QStringLiteral("MoveToRightItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Semicolon)] = QStringLiteral("OpenNode"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Z)] = QStringLiteral("UpDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_X)] = QStringLiteral("DownDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_C)] = QStringLiteral("CloneNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_V)] = QStringLiteral("MakeDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_B)] = QStringLiteral("Refresh"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_N)] = QStringLiteral("MakeDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_M)] = QStringLiteral("CloneNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Comma)] = QStringLiteral("UpDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Period)] = QStringLiteral("DownDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Slash)] = QStringLiteral("NoAction"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F1)]  = QStringLiteral("OpenUrlEditor"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F2)]  = QStringLiteral("RenameNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F3)]  = QStringLiteral("NewWindow"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F4)]  = QStringLiteral("CloseWindow"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F5)]  = QStringLiteral("Refresh"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F6)]  = QStringLiteral("PrevView"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F7)]  = QStringLiteral("NextView"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F8)]  = QStringLiteral("SwitchWindow"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F9)]  = QStringLiteral("DisplayViewTree"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F10)] = QStringLiteral("DisplayHistTree"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F11)] = QStringLiteral("ToggleFullScreen"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_F12)] = QStringLiteral("DisplayAccessKey"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Escape)] = QStringLiteral("Deactivate"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Z)] = QStringLiteral("MakeLocalNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Period)] = QStringLiteral("MakeLocalNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Backslash)] = QStringLiteral("MakeLocalNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_C)] = QStringLiteral("CloneNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_M)] = QStringLiteral("CloneNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_V)] = QStringLiteral("NewNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_N)] = QStringLiteral("NewNode"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Up)]       = QStringLiteral("MoveToUpperItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Down)]     = QStringLiteral("MoveToLowerItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Right)]    = QStringLiteral("MoveToRightItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Left)]     = QStringLiteral("MoveToLeftItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Home)]     = QStringLiteral("MoveToFirstItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_End)]      = QStringLiteral("MoveToLastItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_PageUp)]   = QStringLiteral("PageUp"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_PageDown)] = QStringLiteral("PageDown"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Enter)]    = QStringLiteral("OpenNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Return)]   = QStringLiteral("OpenNode"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Delete)]   = QStringLiteral("DeleteNode"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Up)]    = QStringLiteral("SelectToUpperItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Down)]  = QStringLiteral("SelectToLowerItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Right)] = QStringLiteral("SelectToRightItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Left)]  = QStringLiteral("SelectToLeftItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_E)]     = QStringLiteral("SelectToUpperItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_D)]     = QStringLiteral("SelectToLowerItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_F)]     = QStringLiteral("SelectToRightItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_S)]     = QStringLiteral("SelectToLeftItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_I)]     = QStringLiteral("SelectToUpperItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_K)]     = QStringLiteral("SelectToLowerItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_L)]     = QStringLiteral("SelectToRightItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_J)]     = QStringLiteral("SelectToLeftItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_PageUp)] = QStringLiteral("SelectToPrevPage"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_PageDown)] = QStringLiteral("SelectToNextPage"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Home)]  = QStringLiteral("SelectToFirstItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_End)]   = QStringLiteral("SelectToLastItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::Key_Space)]             = QStringLiteral("SelectItem"); \
    m_ThumbListKeyMap[QKeySequence(Qt::SHIFT + Qt::Key_Space)] = QStringLiteral("SelectRange"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Up)]     = QStringLiteral("TransferToUpper"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Down)]   = QStringLiteral("TransferToLower"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Right)]  = QStringLiteral("TransferToRight"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Left)]   = QStringLiteral("TransferToLeft"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_PageUp)] = QStringLiteral("TransferToPrevPage"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_PageDown)] = QStringLiteral("TransferToNextPage"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Home)]   = QStringLiteral("TransferToFirst"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_End)]    = QStringLiteral("TransferToLast"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Backspace)] = QStringLiteral("TransferToUpDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Backspace)] = QStringLiteral("TransferToDownDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Z)]      = QStringLiteral("TransferToUpDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_X)]      = QStringLiteral("TransferToDownDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Comma)]  = QStringLiteral("TransferToUpDirectory"); \
    m_ThumbListKeyMap[QKeySequence(Qt::CTRL + Qt::Key_Period)] = QStringLiteral("TransferToDownDirectory"); \
                                                                        \
    m_ThumbListKeyMap[QKeySequence(Qt::ALT + Qt::Key_X)] = QStringLiteral("OpenCommand");

#define ACCESSKEY_KEYMAP                                                \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_E)] = QStringLiteral("OpenOnRootForeground"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_S)] = QStringLiteral("OpenInNewViewNodeForeground"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_F)] = QStringLiteral("OpenInNewHistNodeForeground"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_C)] = QStringLiteral("OpenInNewDirectoryForeground"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_D)] = QStringLiteral("ClickElement"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_A)] = QStringLiteral("FocusElement"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_X)] = QStringLiteral("HoverElement"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_I)] = QStringLiteral("OpenOnRootForeground"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_J)] = QStringLiteral("OpenInNewViewNodeForeground"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_L)] = QStringLiteral("OpenInNewHistNodeForeground"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_M)] = QStringLiteral("OpenInNewDirectoryForeground"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_K)] = QStringLiteral("ClickElement"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_Semicolon)] = QStringLiteral("FocusElement"); \
    m_AccessKeyKeyMap[QKeySequence(Qt::Key_Comma)] = QStringLiteral("HoverElement");
