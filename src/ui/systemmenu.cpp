#include "switch.hpp"
#include "systemmenu.hpp"

#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <QOperatingSystemVersion>

#if defined(Q_OS_WIN)
#  include <windows.h>
#  include <string>
#endif

namespace SystemMenu {

int IdOf(int index){
    if(index < 0) return -1;
    const int id = FirstId + index * IdStep;
    return id > LastId ? -1 : id;
}

int IndexOf(quintptr command){
    const quintptr id = command & 0xFFF0;
    if(id < static_cast<quintptr>(FirstId) || id > static_cast<quintptr>(LastId)) return -1;
    return static_cast<int>((id - FirstId) / IdStep);
}

bool IsOurs(int id){
    if(id == MarkId) return true;
    return id >= FirstId && id <= LastId && (id & 0xF) == 0;
}

QString Label(const QAction *action){
    QString label = action->text();
    const QString key = action->shortcut().toString(QKeySequence::NativeText);
    if(!key.isEmpty()) label += QLatin1Char('\t') + key;
    return label;
}

QList<Entry> Entries(const QMenu *menu, QList<QAction*> &actions){
    QList<Entry> entries;
    if(!menu) return entries;
    foreach(QAction *action, menu->actions()){
        if(!action->isVisible()) continue;
        Entry entry;
        entry.enabled = action->isEnabled();
        entry.checked = false;
        entry.id = -1;
        if(action->isSeparator()){
            entry.kind = Entry::Separator;
        } else if(QMenu *sub = action->menu()){
            entry.kind = Entry::Submenu;
            entry.label = Label(action);
            entry.children = Entries(sub, actions);
        } else {
            entry.kind = Entry::Command;
            entry.label = Label(action);
            entry.checked = action->isCheckable() && action->isChecked();
            entry.id = actions.size();
            actions.append(action);
        }
        entries.append(entry);
    }
    return entries;
}

#if defined(Q_OS_WIN)

namespace {
    void Fill(HMENU menu, const QList<Entry> &entries, UINT before){
        foreach(const Entry &entry, entries){
            MENUITEMINFOW info;
            ZeroMemory(&info, sizeof(info));
            info.cbSize = sizeof(info);
            std::wstring text;
            switch(entry.kind){
            case Entry::Separator:
                info.fMask = MIIM_FTYPE | MIIM_ID;
                info.fType = MFT_SEPARATOR;
                info.wID = MarkId;
                break;
            case Entry::Submenu:
                info.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID | MIIM_STATE;
                info.hSubMenu = CreatePopupMenu();
                if(!info.hSubMenu) continue;
                Fill(info.hSubMenu, entry.children, 0);
                info.wID = MarkId;
                info.fState = entry.enabled ? MFS_ENABLED : MFS_DISABLED;
                text = entry.label.toStdWString();
                info.dwTypeData = const_cast<wchar_t*>(text.c_str());
                break;
            case Entry::Command: {
                const int id = IdOf(entry.id);
                if(id < 0) continue;
                info.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
                info.wID = static_cast<UINT>(id);
                info.fState = (entry.enabled ? MFS_ENABLED : MFS_DISABLED)
                            | (entry.checked ? MFS_CHECKED : MFS_UNCHECKED);
                text = entry.label.toStdWString();
                info.dwTypeData = const_cast<wchar_t*>(text.c_str());
                break;
            }
            }
            const BOOL put = before
                ? InsertMenuItemW(menu, before, FALSE, &info)
                : InsertMenuItemW(menu, static_cast<UINT>(GetMenuItemCount(menu)), TRUE, &info);
            if(!put && info.hSubMenu) DestroyMenu(info.hSubMenu);
        }
    }

    void TakeOut(HMENU menu){
        for(int pos = GetMenuItemCount(menu) - 1; pos >= 0; pos--){
            MENUITEMINFOW info;
            ZeroMemory(&info, sizeof(info));
            info.cbSize = sizeof(info);
            info.fMask = MIIM_ID;
            if(GetMenuItemInfoW(menu, static_cast<UINT>(pos), TRUE, &info) &&
               IsOurs(static_cast<int>(info.wID)))
                DeleteMenu(menu, static_cast<UINT>(pos), MF_BYPOSITION);
        }
    }
}

void ApplyScheme(bool dark){
    static int applied = -1;
    const int wanted = dark ? 1 : 0;
    if(applied == wanted) return;
    applied = wanted;

    if(QOperatingSystemVersion::current() <
       QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 18362)) return;
    static const HMODULE uxtheme =
        LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!uxtheme) return;
    enum PreferredAppMode { Default, AllowDark, ForceDark, ForceLight };
    typedef PreferredAppMode (WINAPI *SetPreferredAppModeFn)(PreferredAppMode);
    typedef void (WINAPI *FlushMenuThemesFn)();
    const auto setMode = reinterpret_cast<SetPreferredAppModeFn>(
        GetProcAddress(uxtheme, MAKEINTRESOURCEA(135)));
    const auto flush = reinterpret_cast<FlushMenuThemesFn>(
        GetProcAddress(uxtheme, MAKEINTRESOURCEA(136)));
    if(!setMode || !flush) return;
    setMode(dark ? ForceDark : ForceLight);
    flush();
}

void Install(WId window, const QList<Entry> &entries){
    if(!window) return;
    const HMENU menu = GetSystemMenu(reinterpret_cast<HWND>(window), FALSE);
    if(!menu) return;
    TakeOut(menu);
    if(entries.isEmpty()) return;
    Fill(menu, entries, SC_CLOSE);
    MENUITEMINFOW line;
    ZeroMemory(&line, sizeof(line));
    line.cbSize = sizeof(line);
    line.fMask = MIIM_FTYPE | MIIM_ID;
    line.fType = MFT_SEPARATOR;
    line.wID = MarkId;
    InsertMenuItemW(menu, SC_CLOSE, FALSE, &line);
}

#else

void ApplyScheme(bool dark){
    Q_UNUSED(dark)
}

void Install(WId window, const QList<Entry> &entries){
    Q_UNUSED(window)
    Q_UNUSED(entries)
}

#endif

}
