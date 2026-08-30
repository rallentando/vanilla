#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "devicescale.hpp"

#include "application.hpp"
#include <VERSION>

#include <QCoreApplication>
#include <QDomDocument>
#include <QDomElement>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QDateTime>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QOpenGLContext>
#include <QAuthenticator>
#include <QFileSystemModel>
#include <QDesktopServices>
#include <QProcess>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QtConcurrent/QtConcurrent>
#include <QScreen>
#include <QOperatingSystemVersion>
#include <QStyleHints>
#include <QProxyStyle>
#include <QStyleOptionMenuItem>

#ifdef WEBENGINEVIEW
#  if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#    include <QWebEngineGlobalSettings>
#  endif
#endif

#include <functional>
#include <stdlib.h>
#include <time.h>

#if defined(Q_OS_WIN)
#  include <QSharedMemory>
#endif

#include "gadgets.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "treebank.hpp"
#include "bookmarkio.hpp"
#include "settingsio.hpp"
#include "certificatepolicy.hpp"
#include "notifier.hpp"
#include "saver.hpp"
#include "transmitter.hpp"
#include "receiver.hpp"
#include "localview.hpp"
#include "dialog.hpp"

#ifdef QT_NO_PROCESS
namespace QProcess {
    static bool startDetached(const QString &program, const QStringList &args,
                              const QString &dir = QString(), qint64 *pid = 0){
        Q_UNUSED(program) Q_UNUSED(args) Q_UNUSED(dir) Q_UNUSED(pid)
        return false;
    }
    static QStringList splitCommand(const QString &command){
        Q_UNUSED(command)
        return QStringList();
    }
}
#endif

Application *Application::m_Instance = nullptr;
int Application::m_DelayFileCount = 0;

static SettingsIO::Hooks SettingsHooks(){
    SettingsIO::Hooks hooks;
    hooks.LegacyNameOf = [](QString name){ return Application::LegacyFileName(name);};
    hooks.BackUpFiltersOf = [](){ return Application::BackUpFileFilters();};
    hooks.BackUpPrepositionOf = [](){ return Application::BackUpPreposition();};
    hooks.RestoredFromBackUp = [](QString backup){
        ModelessDialog::Information
            (Application::tr("Restored from a back up file")+ QStringLiteral(" [") + backup + QStringLiteral("]."),
             Application::tr("Because of a failure to read the latest file, it was restored from a backup file."));
    };
    return hooks;
}

NetworkController* Application::m_NetworkController = nullptr;
AutoSaver*  Application::m_AutoSaver         = nullptr;
bool        Application::m_Quitting          = false;
bool        Application::m_TakenDown         = false;
bool        Application::m_WaitedForDialog   = false;
quint64     Application::m_QuietGeneration   = 0;
Settings    Application::m_GlobalSettings    = Settings();
Settings    Application::m_IconTable         = Settings();

bool        Application::m_EnableGoogleSuggest = false;
bool        Application::m_EnableFramelessWindow = false;
bool        Application::m_EnableTransparentBar = false;
QString     Application::m_ColorScheme = QStringLiteral("Auto");
bool        Application::m_ReassertingColorScheme = false;
bool        Application::m_EnableAutoSave    = false;
bool        Application::m_EnableAutoLoad    = false;
int         Application::m_AutoSaveInterval  = 0;
int         Application::m_AutoLoadInterval  = 0;
int         Application::m_AutoSaveTimerId   = 0;
int         Application::m_AutoLoadTimerId   = 0;
int         Application::m_MaxBackUpGenerationCount = 0;
QString     Application::m_DownloadDirectory = QString();
QString     Application::m_UploadDirectory   = QString();
QStringList Application::m_ChosenFiles       = QStringList();
bool        Application::m_SaveSessionCookie = false;
QString     Application::m_AcceptLanguage    = QString();
QStringList Application::m_AllowedHosts      = QStringList();
QStringList Application::m_BlockedHosts      = QStringList();
QStringList Application::m_AllowedCertificates = QStringList();
QStringList Application::m_BlockedCertificates = QStringList();
Application::SslErrorPolicy Application::m_SslErrorPolicy = Application::Undefined;
Application::DownloadPolicy Application::m_DownloadPolicy = Application::Undefined_;

WindowLedger<MainWindow> &Application::Windows(){
    static WindowLedger<MainWindow> ledger;
    static bool hooked = false;
    if(!hooked){
        hooked = true;
        ledger.Focus  = [](MainWindow *win){ win->SetFocus();};
        ledger.IsBusy = [](){ return static_cast<bool>(mouseButtons() & Qt::LeftButton);};
    }
    return ledger;
}
ModelessDialogFrame *Application::m_TemporaryDialogFrame = nullptr;

UserAgent::Map Application::m_UserAgents = UserAgent::Map();

namespace {

    class MenuStyle : public QProxyStyle {
    public:
        int pixelMetric(PixelMetric metric, const QStyleOption *option,
                        const QWidget *widget) const Q_DECL_OVERRIDE {
            const int base = QProxyStyle::pixelMetric(metric, option, widget);
            switch(metric){
            case PM_MenuVMargin:
            case PM_MenuHMargin: return base + DeviceScale::Primary(MENU_MARGIN);
            default:             return base;
            }
        }

        QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                               const QSize &size, const QWidget *widget) const Q_DECL_OVERRIDE {
            QSize contents = QProxyStyle::sizeFromContents(type, option, size, widget);
            if(type == CT_MenuItem){
                const QStyleOptionMenuItem *item =
                    qstyleoption_cast<const QStyleOptionMenuItem*>(option);
                if(!item || item->menuItemType != QStyleOptionMenuItem::Separator)
                    contents.setHeight(contents.height()
                                       + DeviceScale::Primary(MENU_ITEM_EXTRA_HEIGHT));
            }
            return contents;
        }
    };
}

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setStyle(new MenuStyle());
    m_NetworkController = nullptr;
    m_AutoSaver = nullptr;
    srand(static_cast<unsigned int>(time(nullptr)));
}

Application::~Application(){
    if(m_NetworkController){
        m_NetworkController->deleteLater();
    }
    if(m_AutoSaver){
        m_AutoSaver->deleteLater();
    }
}

[[ noreturn ]] static void EmitErrorMessage(QObject *receiver, QEvent *ev, std::exception &e){
    qFatal("Error %s sending event %s to object %s (%s)", e.what(),
           typeid(*ev).name(), qPrintable(receiver->objectName()),
           typeid(*receiver).name());
}

[[ noreturn ]] static void EmitErrorMessage(QObject *receiver, QEvent *ev){
    qFatal("Error <unknown> sending event %s to object %s (%s)",
           typeid(*ev).name(), qPrintable(receiver->objectName()),
           typeid(*receiver).name());
}

bool Application::notify(QObject *receiver, QEvent *ev){
    try{
        return QApplication::notify(receiver, ev);
    } catch(std::exception &e){
        EmitErrorMessage(receiver, ev, e);
    } catch(...){
        EmitErrorMessage(receiver, ev);
    }

}

void Application::BootApplication(int &argc, char **argv, Application *instance){
    Q_UNUSED(argc) Q_UNUSED(argv)


    Transmitter *t = new Transmitter(instance);
    if(t->ServerAlreadyExists()){
        QStringList list;
        for(int i = 1; i < argc; i++){
            QString arg = QLatin1String(argv[i]);
            if(arg.startsWith(QStringLiteral("--"))){  break;}
            if(arg.startsWith(QStringLiteral("-"))){   break;}
            list << arg;
        }
        t->SendCommandAndQuit(list.join(QStringLiteral(" ")));
        return;
    }

#if defined(Q_OS_WIN)
    static QSharedMemory mem(SharedMemoryKey());
    if(!mem.create(1)){
        QTimer::singleShot(100, instance, &Application::quit);
        return;
    }
#endif

    m_Instance = instance;
    setApplicationName(QStringLiteral("vanilla"));
    setApplicationVersion(VANILLA_VERSION);
    setQuitOnLastWindowClosed(false);

    const QString path = applicationDirPath() + QStringLiteral("/translations");
    const QStringList locales = QLocale::system().uiLanguages();
    const QStringList prefixes = QStringList()
        << QStringLiteral("qt")     << QStringLiteral("qtbase")
        << QStringLiteral("custom") << QStringLiteral("vanilla");

    foreach(QString locale, locales){
        locale = QLocale(locale).name();
        foreach(QString prefix, prefixes){
            QTranslator *translator = new QTranslator(instance);
            translator->load(prefix + QStringLiteral("_") + locale, path);
            installTranslator(translator);
        }
    }
    LoadSettingsFile();
    LoadGlobalSettings();
    Theme::ApplyScheme(m_ColorScheme);
    connect(styleHints(), &QStyleHints::colorSchemeChanged,
            instance, [](Qt::ColorScheme){
                if(m_ReassertingColorScheme) return;
                if(Theme::ApplyScheme(m_ColorScheme)) UpdateAllWidgets();
            });
    LoadIconDatabase();

    ApplyChromiumFlags();
    ApplyGlobalWebEngineSettings();

    m_NetworkController = new NetworkController();
    m_AutoSaver = new AutoSaver();
    TreeBar::Initialize();
    ToolBar::Initialize();
    TreeBank::Initialize();
    TreeBank::LoadTree();
    if(Windows().IsEmpty()){
        MainWindow * win = NewWindow();
        win->GetTreeBank()->OpenOnSuitableNode(QUrl(QStringLiteral("https://google.com")), true);
    } else {
        Settings &s = GlobalSettings();
        QStringList keys = s.allKeys(QStringLiteral("mainwindow"));

        QList<int> ids = Windows().Ids();
        foreach(int id, ids){
            keys.removeOne(QStringLiteral("mainwindow/tableview%1").arg(id));
            keys.removeOne(QStringLiteral("mainwindow/geometry%1").arg(id));
            keys.removeOne(QStringLiteral("mainwindow/notifier%1").arg(id));
            keys.removeOne(QStringLiteral("mainwindow/receiver%1").arg(id));
            keys.removeOne(QStringLiteral("mainwindow/menubar%1").arg(id));
            keys.removeOne(QStringLiteral("mainwindow/toolbar%1").arg(id));
            keys.removeOne(QStringLiteral("mainwindow/treebar%1").arg(id));
            keys.removeOne(QStringLiteral("mainwindow/status%1").arg(id));
        }
        foreach(QString key, keys){
            s.remove(key);
        }
    }

    QTimer::singleShot(0, [=](){

        CreateBackUpFiles();
        StartAutoSaveTimer();
        StartAutoLoadTimer();

        if(MainWindow *win = Windows().Current()){
            QStringList list;
            for(int i = 1; i < argc; i++){
                QString arg = QLatin1String(argv[i]);
                if(arg.startsWith(QStringLiteral("--"))){  break;}
                if(arg.startsWith(QStringLiteral("-"))){   break;}
                list << arg;
            }
            if(Receiver *receiver = win->GetTreeBank()->GetReceiver())
                receiver->ReceiveCommand(list.join(QStringLiteral(" ")));
        }
    });
}

void Application::Import(TreeBank *tb){
    ViewNode *root = TreeBank::GetViewRoot();

    const QString ieFavorites      = BookmarkIO::IeFavoritesDirectory();
    const QString firefoxProfile   = BookmarkIO::FirefoxProfileDirectory();
    const QString chromeBookmarks  = BookmarkIO::ChromeBookmarkFile();
    const QString oprBookmarks     = BookmarkIO::OperaBookmarkFile();
    const QString vivaldiBookmarks = BookmarkIO::VivaldiBookmarkFile();

    const bool supportsIE      = !ieFavorites.isEmpty();
    const bool supportsFirefox = QFile::exists(firefoxProfile);
    const bool supportsChrome  = QFile::exists(chromeBookmarks);
    const bool supportsOPR     = QFile::exists(oprBookmarks);
    const bool supportsVivaldi = QFile::exists(vivaldiBookmarks);

    std::function<void()> importFromIE = [&](){
        QFileDialog::Options options =
            QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;

        QString dirname =
            ModalDialog::GetExistingDirectory(QString(), ieFavorites, options);

        if(dirname.isEmpty()) return;
        BookmarkIO::ReadIeFavorites(dirname, root);
    };

    std::function<void()> importFromFirefox = [&](){
        QString filename =
            ModalDialog::GetOpenFileName_(QString(), BookmarkIO::FirefoxBookmarkBackup(),
                                          QStringLiteral("Json Files (*.json)"));

        if(filename.isEmpty()) return;
        BookmarkIO::ReadFirefoxJsonFile(filename, root);
    };

    std::function<void(QString)> importFromChromeFamily = [&](QString filename){
        if(filename.isEmpty()){
            filename =
                QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) +
                QStringLiteral("/Bookmarks");

            filename =
                ModalDialog::GetOpenFileName_(QString(), filename,
                                              QStringLiteral("Bookmarks"));
        }

        if(filename.isEmpty()) return;
        BookmarkIO::ReadChromeJsonFile(filename, root);
    };

    std::function<void(QString, QString, std::function<bool(QString, ViewNode*)>)> importFromFile =
        [&](QString filter, QString caption, std::function<bool(QString, ViewNode*)> read){

        QString filename = ModalDialog::GetOpenFileName_(QString(), filter, caption);

        if(filename.isEmpty()) return;
        read(filename, root);
    };

    QStringList list;
    if(supportsIE) list << QStringLiteral("IE");
    if(supportsFirefox) list << QStringLiteral("Firefox");
    if(supportsChrome) list << QStringLiteral("Chrome");
    if(supportsOPR) list << QStringLiteral("OPR");
    if(supportsVivaldi) list << QStringLiteral("Vivaldi");
    list << QStringLiteral("Chrome Family")
         << QStringLiteral("Internal Format")
         << QStringLiteral("Xbel")
         << QStringLiteral("Html");

    bool ok = true;
    QString which = ModalDialog::GetItem
        (tr("Import Favorites"),
         tr("Select browser or file format"),
         list, false, &ok);
    if(!ok) return;
    else if(which == QStringLiteral("IE")) importFromIE();
    else if(which == QStringLiteral("Firefox")) importFromFirefox();
    else if(which == QStringLiteral("Chrome")) importFromChromeFamily(chromeBookmarks);
    else if(which == QStringLiteral("OPR")) importFromChromeFamily(oprBookmarks);
    else if(which == QStringLiteral("Vivaldi")) importFromChromeFamily(vivaldiBookmarks);
    else if(which == QStringLiteral("Chrome Family")) importFromChromeFamily(QString());
    else if(which == QStringLiteral("Internal Format"))
        importFromFile(QStringLiteral("*.xml"), QStringLiteral("Xml Document (*.xml)"),
                       BookmarkIO::ReadInternalXmlFile);
    else if(which == QStringLiteral("Xbel"))
        importFromFile(QStringLiteral("*.xbel"), QStringLiteral("Xbel Files (*.xbel)"),
                       BookmarkIO::ReadXbelFile);
    else if(which == QStringLiteral("Html"))
        importFromFile(QStringLiteral("*.html"), QStringLiteral("Html Files (*.html)"),
                       BookmarkIO::ReadNetscapeHtmlFile);
    else return;

    if(tb){
        QTimer::singleShot(0, [tb](){
            TreeBank::EmitTreeStructureChanged();
            tb->DisplayViewTree(TreeBank::GetViewRoot());
            tb->GetGadgets()->setFocus(Qt::OtherFocusReason);
        });
    }
}

void Application::Export(TreeBank *tb){
    Q_UNUSED(tb)
    ViewNode *root = TreeBank::GetViewRoot();

    const int utcOffset = BookmarkIO::LocalUtcOffset();

    BookmarkIO::Hooks hooks;
    hooks.WindowIndexOf = [](ViewNode *nd){ return TreeBank::WinIndex(nd);};

    bool ok = true;
    QString which = ModalDialog::GetItem(tr("Export as Favorites"),
                                         tr("Select format"),
                                         QStringList()
                                         << QStringLiteral("Internal Format")
                                         << QStringLiteral("Xbel")
                                         << QStringLiteral("Html")
                                         ,
                                         false, &ok);
    if(!ok) return;

    if(which == QStringLiteral("Internal Format")){
        QString filename = ModalDialog::GetSaveFileName_
            (QString(), QStringLiteral("*.xml"), QStringLiteral("XML Document (*.xml)"));
        BookmarkIO::WriteInternalXmlFile(filename, root, hooks);

    } else if(which == QStringLiteral("Xbel")){
        QString filename = ModalDialog::GetSaveFileName_
            (QString(), QStringLiteral("*.xbel"), QStringLiteral("Xbel files (*.xbel)"));
        BookmarkIO::WriteXbelFile(filename, root, utcOffset);

    } else if(which == QStringLiteral("Html")){
        QString filename = ModalDialog::GetSaveFileName_
            (QString(), QStringLiteral("*.html"), QStringLiteral("Html Files (*.html)"));
        BookmarkIO::WriteNetscapeHtmlFile(filename, root, utcOffset);
    }
}

void Application::AboutVanilla(QWidget *parent){
    Q_UNUSED(parent)
    QString text = tr("Vanilla is a simple web browser.");
    QMessageBox::about(CurrentWidget(), QStringLiteral("Vanilla"), text);
}

void Application::AboutQt(QWidget *parent){
    Q_UNUSED(parent)
    QMessageBox::aboutQt(CurrentWidget());
}

void Application::Quit(){
    if(m_Quitting) return;
    m_Quitting = true;

    TakeDown();
}

void Application::TakeDown(){
    if(ModalDialog::AnyRunning()){
        m_WaitedForDialog = true;
        ModalDialog::AbortAll();
        QTimer::singleShot(0, GetInstance(), [](){ TakeDown();});
        return;
    }

    if(m_WaitedForDialog && ModalDialog::Generation() != m_QuietGeneration){
        m_QuietGeneration = ModalDialog::Generation();
        QTimer::singleShot(QUIT_AFTER_DIALOG_DELAY, GetInstance(), [](){ TakeDown();});
        return;
    }

    if(m_TakenDown) return;
    m_TakenDown = true;

    StopAutoLoadTimer();
    StopAutoSaveTimer();

    foreach(MainWindow *win, Windows().All().values()){
        win->hide();
    }
#ifdef LOCALVIEW
    LocalView::ClearCache();
#endif
    TreeBank::ReleaseAllView();

    const auto finish = [](){
        m_AutoSaver->disconnect(GetInstance());
        QTimer::singleShot(0, GetInstance(), &Application::quit);
    };
    connect(m_AutoSaver, &AutoSaver::Finished, GetInstance(), finish);
    connect(m_AutoSaver, &AutoSaver::Failed,   GetInstance(), finish);

    if(!m_AutoSaver->IsSaving()){
        TreeBank::DoDelete();
        m_AutoSaver->SaveAll();
    }
}

Settings &Application::GlobalSettings(){
    return m_GlobalSettings;
}

void Application::SaveGlobalSettings(){
    Settings &s = GlobalSettings();

    s.setValue(QStringLiteral("application/@EnableGoogleSuggest"),      m_EnableGoogleSuggest);
    s.setValue(QStringLiteral("application/@EnableFramelessWindow"),    m_EnableFramelessWindow);
    s.setValue(QStringLiteral("application/@EnableTransparentBar"),     m_EnableTransparentBar);
    s.setValue(QStringLiteral("application/@ColorScheme"),              m_ColorScheme);
    s.setValue(QStringLiteral("application/@EnableAutoSave"),           m_EnableAutoSave);
    s.setValue(QStringLiteral("application/@EnableAutoLoad"),           m_EnableAutoLoad);
    s.setValue(QStringLiteral("application/@AutoSaveInterval"),         m_AutoSaveInterval);
    s.setValue(QStringLiteral("application/@AutoLoadInterval"),         m_AutoLoadInterval);
    s.setValue(QStringLiteral("application/@MaxBackUpGenerationCount"), m_MaxBackUpGenerationCount);
    s.setValue(QStringLiteral("application/@FileSaveDirectory"),        m_DownloadDirectory);
    s.setValue(QStringLiteral("application/@FileOpenDirectory"),        m_UploadDirectory);
    s.setValue(QStringLiteral("application/@SaveSessionCookie"),        m_SaveSessionCookie);
    s.setValue(QStringLiteral("application/@AcceptLanguage"),           m_AcceptLanguage);
    s.setValue(QStringLiteral("application/@AllowedHosts"),             m_AllowedHosts);
    s.setValue(QStringLiteral("application/@BlockedHosts"),             m_BlockedHosts);
    s.setValue(QStringLiteral("application/@AllowedCertificates"),      m_AllowedCertificates);
    s.setValue(QStringLiteral("application/@BlockedCertificates"),      m_BlockedCertificates);

    SslErrorPolicy sslPolicy = m_SslErrorPolicy;
    if(sslPolicy == Undefined)             s.setValue(QStringLiteral("application/@SslErrorPolicy"), QStringLiteral("Undefined"));
    if(sslPolicy == BlockAccess)           s.setValue(QStringLiteral("application/@SslErrorPolicy"), QStringLiteral("BlockAccess"));
    if(sslPolicy == IgnoreSslErrors)       s.setValue(QStringLiteral("application/@SslErrorPolicy"), QStringLiteral("IgnoreSslErrors"));
    if(sslPolicy == AskForEachAccess)      s.setValue(QStringLiteral("application/@SslErrorPolicy"), QStringLiteral("AskForEachAccess"));
    if(sslPolicy == AskForEachHost)        s.setValue(QStringLiteral("application/@SslErrorPolicy"), QStringLiteral("AskForEachHost"));
    if(sslPolicy == AskForEachCertificate) s.setValue(QStringLiteral("application/@SslErrorPolicy"), QStringLiteral("AskForEachCertificate"));

    DownloadPolicy downPolicy = m_DownloadPolicy;
    if(downPolicy == Undefined_)         s.setValue(QStringLiteral("application/@DownloadPolicy"), QStringLiteral("Undefined"));
    if(downPolicy == FixedLocale)        s.setValue(QStringLiteral("application/@DownloadPolicy"), QStringLiteral("FixedLocale"));
    if(downPolicy == DownloadFolder)     s.setValue(QStringLiteral("application/@DownloadPolicy"), QStringLiteral("DownloadFolder"));
    if(downPolicy == AskForEachDownload) s.setValue(QStringLiteral("application/@DownloadPolicy"), QStringLiteral("AskForEachDownload"));

    UserAgent::Save(s, m_UserAgents);
}

void Application::LoadGlobalSettings(){
    Settings &s = GlobalSettings();

    m_EnableGoogleSuggest      = s.value(QStringLiteral("application/@EnableGoogleSuggest"), false).value<bool>();
    m_EnableFramelessWindow    = s.value(QStringLiteral("application/@EnableFramelessWindow"), false).value<bool>();
    m_EnableTransparentBar     = s.value(QStringLiteral("application/@EnableTransparentBar"), false).value<bool>();
    m_ColorScheme              = s.value(QStringLiteral("application/@ColorScheme"), QStringLiteral("Auto")).value<QString>();
    m_EnableAutoSave           = s.value(QStringLiteral("application/@EnableAutoSave"), true).value<bool>();
    m_EnableAutoLoad           = s.value(QStringLiteral("application/@EnableAutoLoad"), true).value<bool>();
    m_AutoSaveInterval         = s.value(QStringLiteral("application/@AutoSaveInterval"), 300000).value<int>();
    m_AutoLoadInterval         = s.value(QStringLiteral("application/@AutoLoadInterval"), 1000).value<int>();
    m_MaxBackUpGenerationCount = s.value(QStringLiteral("application/@MaxBackUpGenerationCount"), 5).value<int>();
    m_DownloadDirectory        = s.value(QStringLiteral("application/@FileSaveDirectory"), QString()).value<QString>();
    m_UploadDirectory          = s.value(QStringLiteral("application/@FileOpenDirectory"), QString()).value<QString>();
    m_SaveSessionCookie        = s.value(QStringLiteral("application/@SaveSessionCookie"), false).value<bool>();
    m_AcceptLanguage           = s.value(QStringLiteral("application/@AcceptLanguage"), tr("en-US")).value<QString>();
    m_AllowedHosts             = s.value(QStringLiteral("application/@AllowedHosts"), QStringList()).value<QStringList>();
    m_BlockedHosts             = s.value(QStringLiteral("application/@BlockedHosts"), QStringList()).value<QStringList>();
    m_AllowedCertificates      = s.value(QStringLiteral("application/@AllowedCertificates"), QStringList()).value<QStringList>();
    m_BlockedCertificates      = s.value(QStringLiteral("application/@BlockedCertificates"), QStringList()).value<QStringList>();

    QString sslPolicy = s.value(QStringLiteral("application/@SslErrorPolicy"), QStringLiteral("Undefined")).value<QString>();
    if(sslPolicy == QStringLiteral("Undefined"))             m_SslErrorPolicy = Undefined;
    if(sslPolicy == QStringLiteral("BlockAccess"))           m_SslErrorPolicy = BlockAccess;
    if(sslPolicy == QStringLiteral("IgnoreSslErrors"))       m_SslErrorPolicy = IgnoreSslErrors;
    if(sslPolicy == QStringLiteral("AskForEachAccess"))      m_SslErrorPolicy = AskForEachAccess;
    if(sslPolicy == QStringLiteral("AskForEachHost"))        m_SslErrorPolicy = AskForEachHost;
    if(sslPolicy == QStringLiteral("AskForEachCertificate")) m_SslErrorPolicy = AskForEachCertificate;

    QString downPolicy = s.value(QStringLiteral("application/@DownloadPolicy"), QStringLiteral("Undefined")).value<QString>();
    if(downPolicy == QStringLiteral("Undefined"))          m_DownloadPolicy = Undefined_;
    if(downPolicy == QStringLiteral("FixedLocale"))        m_DownloadPolicy = FixedLocale;
    if(downPolicy == QStringLiteral("DownloadFolder"))     m_DownloadPolicy = DownloadFolder;
    if(downPolicy == QStringLiteral("AskForEachDownload")) m_DownloadPolicy = AskForEachDownload;

    m_UserAgents = UserAgent::Load(s);
}

void Application::ApplyChromiumFlags(){
    const QString flags =
        m_GlobalSettings.value(QStringLiteral("application/@ChromiumFlags"),
                               QString()).value<QString>();

    QStringList accepted;
    foreach(const QString &word, flags.split(QRegularExpression(QStringLiteral("\\s+")),
                                             Qt::SkipEmptyParts)){
        if(word.startsWith(QStringLiteral("--"))) accepted << word;
        else qWarning() << "ignoring" << word << "in application/@ChromiumFlags:"
                        << "not a switch";
    }
    if(accepted.isEmpty()) return;

    const QByteArray existing = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
    const QByteArray added = accepted.join(QStringLiteral(" ")).toLocal8Bit();
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            existing.isEmpty() ? added : existing + ' ' + added);
}

void Application::ApplyGlobalWebEngineSettings(){
#if defined(WEBENGINEVIEW) && QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    const QString mode =
        m_GlobalSettings.value(QStringLiteral("network/@SecureDnsMode"),
                               QStringLiteral("SystemOnly")).value<QString>();
    if(mode == QStringLiteral("SystemOnly")) return;

    QWebEngineGlobalSettings::DnsMode dns;
    dns.secureMode = mode == QStringLiteral("SecureOnly")
        ? QWebEngineGlobalSettings::SecureDnsMode::SecureOnly
        : QWebEngineGlobalSettings::SecureDnsMode::SecureWithFallback;
    dns.serverTemplates =
        m_GlobalSettings.value(QStringLiteral("network/@SecureDnsServers"),
                               QStringList()).value<QStringList>();

    if(dns.serverTemplates.isEmpty()){
        qWarning() << "network/@SecureDnsMode was not applied:"
                   << "network/@SecureDnsServers is empty."
                   << "the system resolver is still in use.";
        return;
    }

    if(!QWebEngineGlobalSettings::setDnsMode(dns))
        qWarning() << "network/@SecureDnsMode was not applied:"
                   << "network/@SecureDnsServers is not a list of URI"
                   << "templates. the system resolver is still in use.";
#endif
}

void Application::SaveSettingsFile(){
    SettingsIO::Save(StateDirectory(), GlobalSettingsFileName(), m_GlobalSettings, SettingsHooks());
}

void Application::LoadSettingsFile(){
    SettingsIO::Load(StateDirectory(), GlobalSettingsFileName(), m_GlobalSettings, SettingsHooks());
}

void Application::SaveIconDatabase(){
    SettingsIO::Save(StateDirectory(), IconDatabaseFileName(), m_IconTable, SettingsHooks());
}

void Application::LoadIconDatabase(){
    SettingsIO::Load(StateDirectory(), IconDatabaseFileName(), m_IconTable, SettingsHooks());
}

void Application::RegisterIcon(QString host, QIcon icon){
    if(!host.isEmpty() && !icon.isNull()){
        QSize size = icon.availableSizes().first();
        if(size.width() > 32) size = QSize(32, 32);
        icon = QIcon(icon.pixmap(size));
        m_IconTable[host] = QVariant::fromValue(icon);
    }
}

QIcon Application::GetIcon(QString host){
    if(m_IconTable.contains(host))
        return m_IconTable[host].value<QIcon>();
    return QIcon();
}

void Application::Reconfigure(){
    LoadGlobalSettings();
    const bool schemeChanged = Theme::ApplyScheme(m_ColorScheme);
    LoadIconDatabase();
    TreeBar::LoadSettings();
    ToolBar::LoadSettings();
    TreeBank::LoadSettings();
    if(schemeChanged) UpdateAllWidgets();
}

bool Application::EnableAutoSave(){
    return m_EnableAutoSave;
}

bool Application::EnableAutoLoad(){
    return m_EnableAutoLoad;
}

bool Application::EnableGoogleSuggest(){
    return m_EnableGoogleSuggest;
}

bool Application::EnableFramelessWindow(){
    return m_EnableFramelessWindow;
}

bool Application::EnableTransparentBar(){
    return m_EnableTransparentBar;
}

QString Application::ColorScheme(){
    return m_ColorScheme;
}

void Application::SetColorScheme(const QString &scheme){
    m_ColorScheme = scheme;
    GlobalSettings().setValue(QStringLiteral("application/@ColorScheme"), scheme);
    if(Theme::ApplyScheme(scheme)) UpdateAllWidgets();
}

void Application::ReassertColorSchemeForWeb(){
    static bool done = false;
    if(done) return;
    done = true;
    QTimer::singleShot(0, [](){
        const QString value = m_ColorScheme.trimmed().toLower();
        const bool dark  = value == QStringLiteral("dark");
        const bool light = value == QStringLiteral("light");
        if(!dark && !light) return;
        if(QStyleHints *hints = styleHints()){
            m_ReassertingColorScheme = true;
            hints->unsetColorScheme();
            hints->setColorScheme(dark ? Qt::ColorScheme::Dark
                                       : Qt::ColorScheme::Light);
            m_ReassertingColorScheme = false;
        }
    });
}

void Application::UpdateAllWidgets(){
    foreach(QWidget *widget, allWidgets()){
        if(LineEdit *edit = qobject_cast<LineEdit*>(widget)) edit->ApplyTheme();
        if(DialogLabel *label = qobject_cast<DialogLabel*>(widget)) label->ApplyTheme();
        widget->update();
    }
    foreach(MainWindow *win, Windows().All().values()){
        if(TreeBar *bar = win->GetTreeBar()) bar->ApplyTheme();
        if(TreeBank *tb = win->GetTreeBank()) tb->ApplyTheme();
    }
}

void Application::SetDownloadDirectory(QString path){
    m_DownloadDirectory = path;
}

QString Application::GetDownloadDirectory(){
    if(m_DownloadDirectory.isEmpty())
        m_DownloadDirectory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QStringLiteral("/");
    return m_DownloadDirectory;
}

void Application::SetUploadDirectory(QString path){
    m_UploadDirectory = path;
}

QString Application::GetUploadDirectory(){
    if(m_UploadDirectory.isEmpty())
        m_UploadDirectory = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + QStringLiteral("/");
    return m_UploadDirectory;
}

void Application::SetMaxBackUpGenerationCount(int count){
    m_MaxBackUpGenerationCount = count;
}

int Application::GetMaxBackUpGenerationCount(){
    return m_MaxBackUpGenerationCount;
}

QStringList Application::BackUpFileFilters(){
    static const QString date =
        QStringLiteral(
            "[0-9][0-9][0-9][0-9]-"
            "[0-9][0-9]-"
            "[0-9][0-9]-"
            "[0-9][0-9]-"
            "[0-9][0-9]-"
            "[0-9][0-9]-");
    static const QStringList filters =
        QStringList() << date + QStringLiteral("*.json")
                      << date + QStringLiteral("*.xml");
    return filters;
}

QString Application::BaseDirectory(){
    static bool checked = false;
    static QString dir;
    if(checked) return dir;

#  if defined(Q_OS_WIN)
    dir = applicationDirPath() + QStringLiteral("/");
    if(dir.startsWith(QStringLiteral("C:/Windows/")) ||
       dir.startsWith(QStringLiteral("C:/Program Files/")) ||
       dir.startsWith(QStringLiteral("C:/Program Files (x86)/"))){
        dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/");
    }
#  else
    dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/");
#  endif
    checked = true;
    return dir;
}

QString Application::DataDirectory(){
    static QString dir = BaseDirectory() + QStringLiteral("data/");
    return dir;
}

QString Application::StateDirectory(){
    static QString dir = [](){
        const QString path =
            DataDirectory()
            + QString::fromLatin1(QCryptographicHash::hash(applicationDirPath().toUtf8(),
                                                           QCryptographicHash::Md5).toHex())
            + QStringLiteral("/");
        QDir().mkpath(path);
        return path;
    }();
    return dir;
}

QString Application::ThumbnailDirectory(){
    static QString dir = StateDirectory() + QStringLiteral("image/");
    return dir;
}

QString Application::HistoryDirectory(){
    static QString dir = StateDirectory() + QStringLiteral("history/");
    return dir;
}

QString Application::TemporaryDirectory(){
    static QString dir = BaseDirectory() + QStringLiteral("temp/");
    return dir;
}

QString Application::BackUpPreposition(){
    return QStringLiteral("~");
}

void Application::ClearTemporaryDirectory(){
    QDir tmpdir = TemporaryDirectory();
    QStringList files = tmpdir.entryList(QDir::NoDotAndDotDot|QDir::AllEntries);
    if(files.isEmpty()){
        tmpdir.mkpath(TemporaryDirectory());
    } else {
        foreach(QString file, files){
            QFile::remove(TemporaryDirectory() + QStringLiteral("/") + file);
        }
    }
}

QString Application::PrimaryTreeFileName(bool tmp){
    return (tmp ? BackUpPreposition() : QString()) + QStringLiteral("main_tree.json");
}

QString Application::SecondaryTreeFileName(bool tmp){
    return (tmp ? BackUpPreposition() : QString()) + QStringLiteral("trash_tree.json");
}

QString Application::CookieFileName(bool tmp){
    return (tmp ? BackUpPreposition() : QString()) + QStringLiteral("cookie.json");
}

QString Application::GlobalSettingsFileName(bool tmp){
    return (tmp ? BackUpPreposition() : QString()) + QStringLiteral("config.json");
}

QString Application::IconDatabaseFileName(bool tmp){
    return (tmp ? BackUpPreposition() : QString()) + QStringLiteral("icondata.json");
}

QString Application::LegacyFileName(QString name){
    if(name.endsWith(QStringLiteral(".json")))
        name.chop(5);
    return name + QStringLiteral(".xml");
}

void Application::AppendChosenFile(QString file){
    m_ChosenFiles << file;
}

void Application::RemoveChosenFile(QString file){
    m_ChosenFiles.removeOne(file);
}

QStringList Application::ChosenFiles(){
    return m_ChosenFiles;
}

bool Application::SaveSessionCookie(){
    return m_SaveSessionCookie;
}

QString Application::GetAcceptLanguage(){
    return m_AcceptLanguage;
}

void Application::SetAcceptLanguage(QString acceptLanguage){
    m_AcceptLanguage = acceptLanguage;
}

QStringList Application::GetAllowedHosts(){
    return m_AllowedHosts;
}

void Application::AppendToAllowedHosts(QString host){
    if(!m_AllowedHosts.contains(host))
        m_AllowedHosts << host;
}

void Application::RemoveFromAllowedHosts(QString host){
    if(m_AllowedHosts.contains(host))
        m_AllowedHosts.removeOne(host);
}

QStringList Application::GetBlockedHosts(){
    return m_BlockedHosts;
}

void Application::AppendToBlockedHosts(QString host){
    if(!m_BlockedHosts.contains(host))
        m_BlockedHosts << host;
}

void Application::RemoveFromBlockedHosts(QString host){
    if(m_BlockedHosts.contains(host))
        m_BlockedHosts.removeOne(host);
}

QStringList Application::GetAllowedCertificates(){
    return m_AllowedCertificates;
}

QStringList Application::GetBlockedCertificates(){
    return m_BlockedCertificates;
}

void Application::RememberCertificate(QString key, bool allow){
    CertificatePolicy::Remember
        (key, allow, &m_AllowedCertificates, &m_BlockedCertificates);
}

Application::SslErrorPolicy Application::GetSslErrorPolicy(){
    return m_SslErrorPolicy;
}

void Application::AskSslErrorPolicyIfNeed(){
    if(m_SslErrorPolicy == Undefined){
        QStringList policies;
        policies << tr("BlockAccess")
                 << tr("IgnoreSslErrors")
                 << tr("AskForEachAccess")
                 << tr("AskForEachHost")
                 << tr("AskForEachCertificate");
        bool ok;

        QString policy = ModalDialog::GetItem
            (tr("Ssl error policy"),
             tr("Select ssl error policy."),
             policies, false, &ok);

        if(!ok) return;

        if     (policy == tr("BlockAccess"))
            m_SslErrorPolicy = BlockAccess;
        else if(policy == tr("IgnoreSslErrors"))
            m_SslErrorPolicy = IgnoreSslErrors;
        else if(policy == tr("AskForEachAccess"))
            m_SslErrorPolicy = AskForEachAccess;
        else if(policy == tr("AskForEachHost"))
            m_SslErrorPolicy = AskForEachHost;
        else if(policy == tr("AskForEachCertificate"))
            m_SslErrorPolicy = AskForEachCertificate;
    }
}

Application::DownloadPolicy Application::GetDownloadPolicy(){
    return m_DownloadPolicy;
}

void Application::AskDownloadPolicyIfNeed(){
    if(m_DownloadPolicy == Undefined_){
        QStringList policies;
        policies << tr("FixedLocale")
                 << tr("DownloadFolder")
                 << tr("AskForEachDownload");
        bool ok;

        QString policy = ModalDialog::GetItem
            (tr("Download policy"),
             tr("Select download policy."),
             policies, false, &ok);

        if(!ok) return;

        if(policy == tr("FixedLocale")){
            QString directory =
                ModalDialog::GetExistingDirectory(QString(), GetDownloadDirectory());
            if(!directory.isEmpty()){
                SetDownloadDirectory(directory + QStringLiteral("/"));
                m_DownloadPolicy = FixedLocale;
            }
        } else if(policy == tr("DownloadFolder")){
            SetDownloadDirectory(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QStringLiteral("/"));
            m_DownloadPolicy = DownloadFolder;
        } else if(policy == tr("AskForEachDownload")){
            m_DownloadPolicy = AskForEachDownload;
        }
    }
}

QString Application::LocalServerName(){
    return VANILLA_LOCAL_SERVER_NAME_PREFIX +
        QString::fromLatin1(QCryptographicHash::hash(applicationDirPath().toUtf8(), QCryptographicHash::Md5).toHex());
}

QString Application::SharedMemoryKey(){
    return VANILLA_SHARED_MEMORY_KEY_PREFIX +
        QString::fromLatin1(QCryptographicHash::hash(applicationDirPath().toUtf8(), QCryptographicHash::Md5).toHex());
}

int Application::EventKey(){
    static int key = 0;
    while(!key) key = rand() + 1;
    return key;
}

QString Application::ProductVersion(){
    static QString version = QString();
    if(version.isNull()){
#if defined(Q_OS_MAC)
        QProcess process;
        process.start("sw_vers", QStringList() << QStringLiteral("-productVersion"));
        process.waitForFinished(-1);
        version = process.readAllStandardOutput().trimmed();
#else
        version = QSysInfo::productVersion();
#endif
    }
    return version;
}

MainWindow *Application::ShadeWindow(MainWindow *win){
    if(!win) win = GetCurrentWindow();
    if(win) win->Shade();
    return win;
}

MainWindow *Application::UnshadeWindow(MainWindow *win){
    if(!win) win = GetCurrentWindow();
    if(win) win->Unshade();
    return win;
}

MainWindow *Application::NewWindow(int id, QPoint pos){
    if(id == 0) id = Windows().UnusedId();

    TreeBank::LiftMaxViewCountIfNeed(Windows().Count());

    MainWindow *win = new MainWindow(id, pos);
    Windows().Insert(id, win);

#if defined(Q_OS_MAC)
    if(Windows().Current() && pos.isNull() && ProductVersion().startsWith(QStringLiteral("10.14"))){
        QTimer::singleShot(16, [win](){ Windows().SetCurrent(win);});
        return win;
    }
#endif
    Windows().SetCurrent(win);
    return win;
}

MainWindow *Application::CloseWindow(MainWindow *win){
    if(!win) win = GetCurrentWindow();
    if(win) win->close();
    return GetCurrentWindow();
}

MainWindow *Application::SwitchWindow(bool next){
    return Windows().Switch(next);
}

MainWindow *Application::NextWindow(){
    return SwitchWindow(true);
}

MainWindow *Application::PrevWindow(){
    return SwitchWindow(false);
}

void Application::RemoveWindow(MainWindow *win){
    Windows().Remove(win);
}

void Application::RemoveWindow(int id){
    Windows().Remove(id);
}

void Application::SetCurrentWindow(MainWindow *win){
    Windows().SetCurrent(win);
}

void Application::SetCurrentWindow(int id){
    Windows().SetCurrent(id);
}

int Application::WindowId(MainWindow *win){
    return Windows().IdOf(win);
}

MainWindow *Application::Window(int id){
    return Windows().At(id);
}

int Application::GetCurrentWindowId(){
    return Windows().CurrentId();
}

MainWindow *Application::GetCurrentWindow(){
    return Windows().Current();
}

QWidget *Application::CurrentWidget(){
    MainWindow *win = Windows().Current();
    if(!win) return nullptr;

    TreeBank *tb = win->GetTreeBank();
    if(tb->IsDisplayingTableView()) return tb;

    if(SharedView view = tb->GetCurrentView()){
        if(QWidget *w = qobject_cast<QWidget*>(view->base())) return w;
        return tb;
    }
    return win;
}

WinMap Application::GetMainWindows(){
    return Windows().All();
}

ModelessDialogFrame *Application::MakeTemporaryDialogFrame(){
    m_TemporaryDialogFrame = new ModelessDialogFrame();
    if(screens().length()){
        QRect rect = primaryScreen()->geometry();

        m_TemporaryDialogFrame->setGeometry
            (rect.x() + rect.width()  / 6,
             rect.y() + rect.height() / 6,
             rect.width() * 2 / 3, rect.height() * 2 / 3);
    }
    m_TemporaryDialogFrame->show();
    return m_TemporaryDialogFrame;
}

ModelessDialogFrame *Application::GetTemporaryDialogFrame(){
    return m_TemporaryDialogFrame;
}

void Application::SetTemporaryDialogFrame(ModelessDialogFrame *frame){
    m_TemporaryDialogFrame = frame;
}

QString Application::UserAgentFor(const QString &name){
    return m_UserAgents.value(name);
}

bool Application::OpenUrlWithDefaultBrowser(QUrl url){
    if(url.isEmpty()) return false;
    return QDesktopServices::openUrl(url);
}

QList<QPair<QString, QString> > Application::ExternalCommands(){
    QList<QPair<QString, QString> > list;
    const QStringList lines =
        m_GlobalSettings.value(QStringLiteral("application/@ExternalCommands"),
                               QStringList()).value<QStringList>();
    foreach(QString line, lines){
        const int equal = line.indexOf(QLatin1Char('='));
        if(equal == -1) continue;
        const QString name = line.left(equal).trimmed();
        const QString command = line.mid(equal + 1).trimmed();
        if(name.isEmpty() || command.isEmpty()) continue;
        list << qMakePair(name, command);
    }
    return list;
}

bool Application::RunExternalCommand(QString name, QUrl url){
    if(url.isEmpty() || name.isEmpty()) return false;
    const QList<QPair<QString, QString> > commands = ExternalCommands();
    for(int i = 0; i < commands.length(); i++){
        if(commands[i].first != name) continue;
        QStringList args = QProcess::splitCommand(commands[i].second);
        if(args.isEmpty()) return false;
        const QString program = args.takeFirst();
        const QString text = QString::fromUtf8(url.toEncoded());
        bool substituted = false;
        for(int j = 0; j < args.length(); j++){
            if(!args[j].contains(QStringLiteral("%u"))) continue;
            args[j].replace(QStringLiteral("%u"), text);
            substituted = true;
        }
        if(!substituted) args << text;
        return QProcess::startDetached(program, args);
    }
    return false;
}

void Application::timerEvent(QTimerEvent *ev){
    if(ev->timerId() == m_AutoSaveTimerId){
        if(!m_AutoSaver->IsSaving()){
            TreeBank::DoDelete();
            TreeBank::UpdateCurrentThumbnails();
            QtConcurrent::run(&AutoSaver::SaveAll, m_AutoSaver);
        }
    }
    else if(ev->timerId() == m_AutoLoadTimerId)
        TreeBank::AutoLoad();
    else
        QApplication::timerEvent(ev);
}

void Application::CreateBackUpFiles(){

    QString date = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd-hh-mm-ss-"));

    QStringList backupfiles =
        QStringList() << GlobalSettingsFileName()
                      << IconDatabaseFileName()
                      << PrimaryTreeFileName()
                      << SecondaryTreeFileName()
                      << CookieFileName();

    foreach(QString file, backupfiles){
        QString original = StateDirectory() + file;
        QString backup   = StateDirectory() + date + file;

        if(QFile::exists(backup)) QFile::remove(backup);
        QFile::copy(original, backup);
    }

    QDir dir = QDir(StateDirectory());

    QStringList list = dir.entryList(BackUpFileFilters(), QDir::NoFilter, QDir::Name);

    while(list.length() > m_MaxBackUpGenerationCount*backupfiles.length()){
        QFile::remove(StateDirectory() + list.takeFirst());
    }
}

void Application::StartAutoSaveTimer(){
    if(m_EnableAutoSave && !m_AutoSaveTimerId)
        m_AutoSaveTimerId = m_Instance->startTimer(m_AutoSaveInterval);
}

void Application::StartAutoLoadTimer(){
    if(m_EnableAutoLoad && !m_AutoLoadTimerId)
        m_AutoLoadTimerId = m_Instance->startTimer(m_AutoLoadInterval);
}

void Application::StopAutoSaveTimer(){
    if(m_AutoSaveTimerId){
        m_Instance->killTimer(m_AutoSaveTimerId);
        m_AutoSaveTimerId = 0;
    }
}

void Application::StopAutoLoadTimer(){
    if(m_AutoLoadTimerId){
        m_Instance->killTimer(m_AutoLoadTimerId);
        m_AutoLoadTimerId = 0;
    }
}

void Application::RestartAutoSaveTimer(){
    StopAutoSaveTimer();
    StartAutoSaveTimer();
}

void Application::RestartAutoLoadTimer(){
    StopAutoLoadTimer();
    StartAutoLoadTimer();
}
