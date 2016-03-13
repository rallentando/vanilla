#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "switch.hpp"

#include <QApplication>
#include <QSettings>
#include <QKeySequence>
#include <QKeyEvent>
#include <QCryptographicHash>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

class QIODevice;
class QThread;

class MainWindow;
class TreeBank;
class NetworkController;
class AutoSaver;
class ModelessDialogFrame;

typedef QMap<int, MainWindow*> WinMap;

template <class Key, class T>
QMap<Key, T>& operator<<(QMap<Key, T>& map, const QPair<Key, T>& data){
    map.insert(data.first, data.second);
    return map;
}

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    ~Application();

    bool notify(QObject *receiver, QEvent *ev) DECL_OVERRIDE;

    static void BootApplication(int &argc, char **argv, Application *instance);
    static void Import(TreeBank *tb);
    static void Export(TreeBank *tb);
    static void AboutVanilla(QWidget *widget);
    static void AboutQt(QWidget *widget);
    static void Quit();

    static NetworkController *GetNetworkController(){
        return m_NetworkController;
    }
    static AutoSaver *GetAutoSaver(){
        return m_AutoSaver;
    }
    static Application *GetInstance(){
        return m_Instance;
    }
private:
    static Application *m_Instance;
    static NetworkController *m_NetworkController;
    static AutoSaver *m_AutoSaver;
    static QThread *m_SaverThread;
    static int m_DelayFileCount;


    // for settings.

public:
    static QSettings *GlobalSettings();
    static void SaveGlobalSettings();
    static void LoadGlobalSettings();
    static void Reconfigure();
    static bool EnableGoogleSuggest();
    static bool EnableFramelessWindow();
    static bool EnableTransparentBar();
    static bool EnableAutoSave();
    static bool EnableAutoLoad();
    static double WheelScrollRate();
private:
    static QSettings::Format XMLFormat;
    static QSettings *m_GlobalSettings;
    static bool m_EnableGoogleSuggest;
    static bool m_EnableFramelessWindow;
    static bool m_EnableTransparentBar;
    static bool m_EnableAutoSave;
    static bool m_EnableAutoLoad;
    static int m_AutoSaveInterval;
    static int m_AutoLoadInterval;
    static double m_WheelScrollRate;


    // uploading path and downloading path.

public:
    static void SetDownloadDirectory(QString);
    static QString GetDownloadDirectory();
    static void SetUploadDirectory(QString);
    static QString GetUploadDirectory();

    static void AppendChosenFile(QString file);
    static void RemoveChosenFile(QString file);
    static QStringList ChosenFiles();
private:
    static int m_MaxBackUpGenerationCount;
    static QString m_DownloadDirectory;
    static QString m_UploadDirectory;
    static QStringList m_ChosenFiles;


    // network configuration.

public:
    enum SslErrorPolicy {
        Undefined,
        BlockAccess,
        IgnoreSslErrors,
        AskForEachAccess,
        AskForEachHost,
        AskForEachCertificate
    };

    enum DownloadPolicy {
        Undefined_,
        FixedLocale,
        DownloadFolder,
        AskForEachDownload
    };

    static bool SaveSessionCookie();
    static QString GetAcceptLanguage();
    static void SetAcceptLanguage(QString);
    static QStringList GetAllowedHosts();
    static void AppendToAllowedHosts(QString);
    static void RemoveFromAllowedHosts(QString);
    static QStringList GetBlockedHosts();
    static void AppendToBlockedHosts(QString);
    static void RemoveFromBlockedHosts(QString);
    static SslErrorPolicy GetSslErrorPolicy();
    static void AskSslErrorPolicyIfNeed();
    static DownloadPolicy GetDownloadPolicy();
    static void AskDownloadPolicyIfNeed();
    static QString LocalServerName();
    static QString SharedMemoryKey();
    static int EventKey();
private:
    static bool m_SaveSessionCookie;
    static QString m_AcceptLanguage;
    static QStringList m_AllowedHosts;
    static QStringList m_BlockedHosts;
    static SslErrorPolicy m_SslErrorPolicy;
    static DownloadPolicy m_DownloadPolicy;


    // window.

public:
    static MainWindow *ShadeWindow(MainWindow *win = 0);
    static MainWindow *UnshadeWindow(MainWindow *win = 0);
    static MainWindow *NewWindow(int id = 0);
    static MainWindow *CloseWindow(MainWindow *win = 0);
    static MainWindow *SwitchWindow(bool next = true);
    static MainWindow *NextWindow();
    static MainWindow *PrevWindow();
    static void RemoveWindow(MainWindow *win);
    static void RemoveWindow(int id);
    static void SetCurrentWindow(MainWindow *win);
    static void SetCurrentWindow(int id);

    static int WindowID(MainWindow *win);
    static MainWindow *Window(int id);
    static int GetCurrentWindowID();
    static MainWindow *GetCurrentWindow();
    static QWidget *CurrentWidget();

    static WinMap GetMainWindows();

    static ModelessDialogFrame *MakeTemporaryDialogFrame();
    static ModelessDialogFrame *GetTemporaryDialogFrame();
    static void SetTemporaryDialogFrame(ModelessDialogFrame *);
private:
    static WinMap m_MainWindows;
    static MainWindow *m_CurrentWindow;
    static ModelessDialogFrame *m_TemporaryDialogFrame;


    // xml files and it's backup.

public:
    static void SetMaxBackUpGenerationCount(int);
    static int GetMaxBackUpGenerationCount();

    static QStringList BackUpFileFilters();

    static QString BaseDirectory();
    static QString DataDirectory();
    static QString ThumbnailDirectory();
    static QString HistoryDirectory();
    static QString TemporaryDirectory();
    static QString BackUpPreposition();
    static void ClearTemporaryDirectory();

    static QString PrimaryTreeFileName(bool tmp = false);
    static QString SecondaryTreeFileName(bool tmp = false);
    static QString CookieFileName(bool tmp = false);
    static QString GlobalSettingsFileName();

protected:
    void timerEvent(QTimerEvent *ev) DECL_OVERRIDE;
private:
    static int m_AutoSaveTimerID;
    static int m_AutoLoadTimerID;
    static void CreateBackUpFiles();
    void SaveAllDataAsync();
public:
    static QThread* GetSaverThread();
    static void StartAutoSaveTimer();
    static void StartAutoLoadTimer();
    static void StopAutoSaveTimer();
    static void StopAutoLoadTimer();
    static void RestartAutoSaveTimer();
    static void RestartAutoLoadTimer();
signals:
    void SaveAllDataRequest();
    void SaveCookieRequest();
    void SaveTreeRequest();


    // user agent.

public:
    static QString UserAgent_IE();
    static QString UserAgent_Edge();
    static QString UserAgent_FF();
    static QString UserAgent_Opera();
    static QString UserAgent_OPR();
    static QString UserAgent_Safari();
    static QString UserAgent_Chrome();
    static QString UserAgent_Sleipnir();
    static QString UserAgent_Vivaldi();
    static QString UserAgent_NetScape();
    static QString UserAgent_SeaMonkey();
    static QString UserAgent_iCab();
    static QString UserAgent_Camino();
    static QString UserAgent_Gecko();
    static QString UserAgent_Custom();

private:
    static QString m_UserAgent_IE;
    static QString m_UserAgent_Edge;
    static QString m_UserAgent_FF;
    static QString m_UserAgent_Opera;
    static QString m_UserAgent_OPR;
    static QString m_UserAgent_Safari;
    static QString m_UserAgent_Chrome;
    static QString m_UserAgent_Sleipnir;
    static QString m_UserAgent_Vivaldi;
    static QString m_UserAgent_NetScape;
    static QString m_UserAgent_SeaMonkey;
    static QString m_UserAgent_iCab;
    static QString m_UserAgent_Camino;
    static QString m_UserAgent_Gecko;
    static QString m_UserAgent_Custom;


    // other browser path.

public:
    static QString BrowserPath_IE();
    static QString BrowserPath_Edge();
    static QString BrowserPath_FF();
    static QString BrowserPath_Opera();
    static QString BrowserPath_OPR();
    static QString BrowserPath_Safari();
    static QString BrowserPath_Chrome();
    static QString BrowserPath_Sleipnir();
    static QString BrowserPath_Vivaldi();
    static QString BrowserPath_Custom();

    static QIcon BrowserIcon_IE();
    static QIcon BrowserIcon_Edge();
    static QIcon BrowserIcon_FF();
    static QIcon BrowserIcon_Opera();
    static QIcon BrowserIcon_OPR();
    static QIcon BrowserIcon_Safari();
    static QIcon BrowserIcon_Chrome();
    static QIcon BrowserIcon_Sleipnir();
    static QIcon BrowserIcon_Vivaldi();
    static QIcon BrowserIcon_Custom();

    static bool OpenUrlWith_IE(QUrl url);
    static bool OpenUrlWith_Edge(QUrl url);
    static bool OpenUrlWith_FF(QUrl url);
    static bool OpenUrlWith_Opera(QUrl url);
    static bool OpenUrlWith_OPR(QUrl url);
    static bool OpenUrlWith_Safari(QUrl url);
    static bool OpenUrlWith_Chrome(QUrl url);
    static bool OpenUrlWith_Sleipnir(QUrl url);
    static bool OpenUrlWith_Vivaldi(QUrl url);
    static bool OpenUrlWith_Custom(QUrl url);

private:
    static QString m_BrowserPath_IE;
    static QString m_BrowserPath_Edge;
    static QString m_BrowserPath_FF;
    static QString m_BrowserPath_Opera;
    static QString m_BrowserPath_OPR;
    static QString m_BrowserPath_Safari;
    static QString m_BrowserPath_Chrome;
    static QString m_BrowserPath_Sleipnir;
    static QString m_BrowserPath_Vivaldi;
    static QString m_BrowserPath_Custom;


    // utilities.

public:
    static inline QKeySequence MakeKeySequence(QKeyEvent *ev){
        int assign = ev->key();
        Qt::Key key = static_cast<Qt::Key>(assign);

        if(key == Qt::Key_unknown){

            // unknown key.
            return QKeySequence();
        }

        if(key == Qt::Key_Shift   ||
           key == Qt::Key_Control ||
           key == Qt::Key_Meta    ||
           key == Qt::Key_Alt){

            // only modifier.
            return QKeySequence();
        }

        Qt::KeyboardModifiers modifiers = ev->modifiers();
        QString keyText = ev->text();

        if(modifiers & Qt::ShiftModifier)   assign += Qt::SHIFT;
        if(modifiers & Qt::ControlModifier) assign += Qt::CTRL;
        if(modifiers & Qt::AltModifier)     assign += Qt::ALT;
        if(modifiers & Qt::MetaModifier)    assign += Qt::META;

        return QKeySequence(assign);
    }

    static inline QKeySequence MakeKeySequence(QString str){
        return QKeySequence(str);
    }

    static inline bool HasAnyModifier(QKeyEvent *ev){
        return
            // ignore ShiftModifier.
            ev->modifiers() & Qt::ControlModifier ||
            ev->modifiers() & Qt::MetaModifier ||
            ev->modifiers() & Qt::AltModifier;
    }

    static inline bool HasNoModifier(QKeyEvent *ev){
        return ev->modifiers() == Qt::NoModifier;
    }

    static inline bool IsOnlyModifier(QKeyEvent *ev){
        return
            ev->key() == Qt::Key_Control ||
            ev->key() == Qt::Key_Shift ||
            ev->key() == Qt::Key_Alt ||
            ev->key() == Qt::Key_Meta;
    }

    static inline bool IsFunctionKey(QKeyEvent *ev){
        return
            ev->key() >= Qt::Key_F1 &&
            ev->key() <= Qt::Key_F35;
    }

    static inline int JsKeyToQtKey(int key){
        if(0x41 <= key && key <= 0x5a) // A ~ Z
            return key;
        if(0x30 <= key && key <= 0x39) // 0 ~ 9
            return key;
        if(0x60 <= key && key <= 0x69) // tenkey 0 ~ 9
            return key - (0x60 - 0x30);
        if(0x70 <= key && key <= 0x7F) // F1 ~ F16
            return key + (0x01000030 - 0x70);

        switch(key){
        case 0x08: return Qt::Key_Backspace;
        case 0x09: return Qt::Key_Tab;
        case 0x0D: return Qt::Key_Return;
        case 0x10: return Qt::Key_Shift;
        case 0x11: return Qt::Key_Control;
        case 0x12: return Qt::Key_Alt;
        case 0x13: return Qt::Key_Pause;
        case 0x14: return Qt::Key_CapsLock;
        case 0x1B: return Qt::Key_Escape;
        case 0x20: return Qt::Key_Space;
        case 0x21: return Qt::Key_PageUp;
        case 0x22: return Qt::Key_PageDown;
        case 0x23: return Qt::Key_End;
        case 0x24: return Qt::Key_Home;
        case 0x25: return Qt::Key_Left;
        case 0x26: return Qt::Key_Up;
        case 0x27: return Qt::Key_Right;
        case 0x28: return Qt::Key_Down;
        case 0x2D: return Qt::Key_Insert;
        case 0x2E: return Qt::Key_Delete;
        case 0x5B: return Qt::Key_Meta;
        case 0x5C: return Qt::Key_Meta;
        case 0x6A: return Qt::Key_Asterisk;
        case 0x6B: return Qt::Key_Plus;
        case 0x6D: return Qt::Key_Minus;
        case 0x6E: return Qt::Key_Period;
        case 0x6F: return Qt::Key_Slash;
        case 0x90: return Qt::Key_NumLock;
        case 0x91: return Qt::Key_ScrollLock;
        case 0xBA: return Qt::Key_Colon;
        case 0xBB: return Qt::Key_Semicolon;
        case 0xBC: return Qt::Key_Comma;
        case 0xBD: return Qt::Key_Minus;
        case 0xBE: return Qt::Key_Period;
        case 0xBF: return Qt::Key_Slash;
        case 0xC0: return Qt::Key_At;
        case 0xDB: return Qt::Key_BracketLeft;
        case 0xDC: return Qt::Key_Backslash;
        case 0xDD: return Qt::Key_BracketRight;
        case 0xDE: return Qt::Key_AsciiCircum;
        case 0xE2: return Qt::Key_Backslash;
        case 0xF0: return Qt::Key_CapsLock;
        }
        return key;
    }

    static inline int QtKeyToJsKey(int key){
        if(0x41 <= key && key <= 0x5a) // A ~ Z
            return key;
        if(0x30 <= key && key <= 0x39) // 0 ~ 9
            return key;
        if(0x30 <= key && key <= 0x39) // tenkey 0 ~ 9
            return key + (0x60 - 0x30);
        if(0x01000030 <= key && key <= 0x0100003f) // F1 ~ F16
            return key - (0x01000030 - 0x70);

        switch(key){
        case Qt::Key_Backspace:    return 0x08;
        case Qt::Key_Tab:          return 0x09;
        case Qt::Key_Return:       return 0x0D;
        case Qt::Key_Shift:        return 0x10;
        case Qt::Key_Control:      return 0x11;
        case Qt::Key_Alt:          return 0x12;
        case Qt::Key_Pause:        return 0x13;
        case Qt::Key_CapsLock:     return 0x14;
        case Qt::Key_Escape:       return 0x1B;
        case Qt::Key_Space:        return 0x20;
        case Qt::Key_PageUp:       return 0x21;
        case Qt::Key_PageDown:     return 0x22;
        case Qt::Key_End:          return 0x23;
        case Qt::Key_Home:         return 0x24;
        case Qt::Key_Left:         return 0x25;
        case Qt::Key_Up:           return 0x26;
        case Qt::Key_Right:        return 0x27;
        case Qt::Key_Down:         return 0x28;
        case Qt::Key_Insert:       return 0x2D;
        case Qt::Key_Delete:       return 0x2E;
        case Qt::Key_Meta:         return 0x5B;
        case Qt::Key_Asterisk:     return 0x6A;
        case Qt::Key_Plus:         return 0x6B;
        case Qt::Key_Minus:        return 0x6D;
        case Qt::Key_Period:       return 0x6E;
        case Qt::Key_Slash:        return 0x6F;
        case Qt::Key_NumLock:      return 0x90;
        case Qt::Key_ScrollLock:   return 0x91;
        case Qt::Key_Colon:        return 0xBA;
        case Qt::Key_Semicolon:    return 0xBB;
        case Qt::Key_Comma:        return 0xBC;
        case Qt::Key_At:           return 0xC0;
        case Qt::Key_BracketLeft:  return 0xDB;
        case Qt::Key_Backslash:    return 0xDC;
        case Qt::Key_BracketRight: return 0xDD;
        case Qt::Key_AsciiCircum:  return 0xDE;
        }
        return key;
    }

    static void AddModifiersToString(QString &str, Qt::KeyboardModifiers modifiers){
        static const QMap<Qt::KeyboardModifier, QString> table = QMap<Qt::KeyboardModifier, QString>()
            << qMakePair(Qt::ControlModifier, QStringLiteral("Ctrl"))
            << qMakePair(Qt::AltModifier,     QStringLiteral("Alt"))
            << qMakePair(Qt::MetaModifier,    QStringLiteral("Meta"))
            << qMakePair(Qt::ShiftModifier,   QStringLiteral("Shift"))
            << qMakePair(Qt::KeypadModifier,  QStringLiteral("Keypad"));
        foreach(Qt::KeyboardModifier modifier, table.keys()){
            if(modifiers & modifier){
                if(str.isEmpty()) str += table[modifier];
                else str += QStringLiteral("+") + table[modifier];
            }
        }
    }

    static void AddMouseButtonToString(QString &str, Qt::MouseButton button){
        QString pre = str.isEmpty() ? QString() : QStringLiteral("+");
        switch(button){
        case Qt::LeftButton:    str += pre + QStringLiteral("LeftButton");    break;
        case Qt::RightButton:   str += pre + QStringLiteral("RightButton");   break;
        case Qt::MidButton:     str += pre + QStringLiteral("MidButton");     break;
        case Qt::ExtraButton1:  str += pre + QStringLiteral("ExtraButton1");  break;
        case Qt::ExtraButton2:  str += pre + QStringLiteral("ExtraButton2");  break;
        case Qt::ExtraButton3:  str += pre + QStringLiteral("ExtraButton3");  break;
        case Qt::ExtraButton4:  str += pre + QStringLiteral("ExtraButton4");  break;
        case Qt::ExtraButton5:  str += pre + QStringLiteral("ExtraButton5");  break;
        case Qt::ExtraButton6:  str += pre + QStringLiteral("ExtraButton6");  break;
        case Qt::ExtraButton7:  str += pre + QStringLiteral("ExtraButton7");  break;
        case Qt::ExtraButton8:  str += pre + QStringLiteral("ExtraButton8");  break;
        case Qt::ExtraButton9:  str += pre + QStringLiteral("ExtraButton9");  break;
        case Qt::ExtraButton10: str += pre + QStringLiteral("ExtraButton10"); break;
        case Qt::ExtraButton11: str += pre + QStringLiteral("ExtraButton11"); break;
        case Qt::ExtraButton12: str += pre + QStringLiteral("ExtraButton12"); break;
        case Qt::ExtraButton13: str += pre + QStringLiteral("ExtraButton13"); break;
        case Qt::ExtraButton14: str += pre + QStringLiteral("ExtraButton14"); break;
        case Qt::ExtraButton15: str += pre + QStringLiteral("ExtraButton15"); break;
        case Qt::ExtraButton16: str += pre + QStringLiteral("ExtraButton16"); break;
        case Qt::ExtraButton17: str += pre + QStringLiteral("ExtraButton17"); break;
        case Qt::ExtraButton18: str += pre + QStringLiteral("ExtraButton18"); break;
        case Qt::ExtraButton19: str += pre + QStringLiteral("ExtraButton19"); break;
        case Qt::ExtraButton20: str += pre + QStringLiteral("ExtraButton20"); break;
        case Qt::ExtraButton21: str += pre + QStringLiteral("ExtraButton21"); break;
        case Qt::ExtraButton22: str += pre + QStringLiteral("ExtraButton22"); break;
        case Qt::ExtraButton23: str += pre + QStringLiteral("ExtraButton23"); break;
        case Qt::ExtraButton24: str += pre + QStringLiteral("ExtraButton24"); break;
        }
    }

    static void AddMouseButtonsToString(QString &str, Qt::MouseButtons buttons){
        static const QMap<Qt::MouseButton, QString> table = QMap<Qt::MouseButton, QString>()
            << qMakePair(Qt::LeftButton,    QStringLiteral("LeftButton"))
            << qMakePair(Qt::RightButton,   QStringLiteral("RightButton"))
            << qMakePair(Qt::MidButton,     QStringLiteral("MidButton"))
            << qMakePair(Qt::ExtraButton1,  QStringLiteral("ExtraButton1"))
            << qMakePair(Qt::ExtraButton2,  QStringLiteral("ExtraButton2"))
            << qMakePair(Qt::ExtraButton3,  QStringLiteral("ExtraButton3"))
            << qMakePair(Qt::ExtraButton4,  QStringLiteral("ExtraButton4"))
            << qMakePair(Qt::ExtraButton5,  QStringLiteral("ExtraButton5"))
            << qMakePair(Qt::ExtraButton6,  QStringLiteral("ExtraButton6"))
            << qMakePair(Qt::ExtraButton7,  QStringLiteral("ExtraButton7"))
            << qMakePair(Qt::ExtraButton8,  QStringLiteral("ExtraButton8"))
            << qMakePair(Qt::ExtraButton9,  QStringLiteral("ExtraButton9"))
            << qMakePair(Qt::ExtraButton10, QStringLiteral("ExtraButton10"))
            << qMakePair(Qt::ExtraButton11, QStringLiteral("ExtraButton11"))
            << qMakePair(Qt::ExtraButton12, QStringLiteral("ExtraButton12"))
            << qMakePair(Qt::ExtraButton13, QStringLiteral("ExtraButton13"))
            << qMakePair(Qt::ExtraButton14, QStringLiteral("ExtraButton14"))
            << qMakePair(Qt::ExtraButton15, QStringLiteral("ExtraButton15"))
            << qMakePair(Qt::ExtraButton16, QStringLiteral("ExtraButton16"))
            << qMakePair(Qt::ExtraButton17, QStringLiteral("ExtraButton17"))
            << qMakePair(Qt::ExtraButton18, QStringLiteral("ExtraButton18"))
            << qMakePair(Qt::ExtraButton19, QStringLiteral("ExtraButton19"))
            << qMakePair(Qt::ExtraButton20, QStringLiteral("ExtraButton20"))
            << qMakePair(Qt::ExtraButton21, QStringLiteral("ExtraButton21"))
            << qMakePair(Qt::ExtraButton22, QStringLiteral("ExtraButton22"))
            << qMakePair(Qt::ExtraButton23, QStringLiteral("ExtraButton23"))
            << qMakePair(Qt::ExtraButton24, QStringLiteral("ExtraButton24"));
        foreach(Qt::MouseButton button, table.keys()){
            if(buttons & button){
                if(str.isEmpty()) str += table[button];
                else str += QStringLiteral("+") + table[button];
            }
        }
    }

    static void AddWheelDirectionToString(QString &str, bool up){
        QString pre = str.isEmpty() ? QString() : QStringLiteral("+");
        if(up) str += pre + QStringLiteral("WheelUp");
        else   str += pre + QStringLiteral("WheelDown");
    }
};

#endif
