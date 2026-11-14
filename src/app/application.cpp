#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "blurbehind.hpp"
#include "ownedraise.hpp"
#include "devicescale.hpp"
#include "settingsschema.hpp"

#include "application.hpp"
#include "downloadhistory.hpp"
#include "extensioncontroller.hpp"
#include "extensionbar.hpp"
#include "downloadbar.hpp"
#include "siteinfo.hpp"
#include <VERSION>

#include <algorithm>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QSaveFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QDateTime>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QAuthenticator>
#include <QFileSystemModel>
#include <QDesktopServices>
#include <QProcess>
#include <QElapsedTimer>
#include <QThread>
#if !defined(Q_OS_WIN)
#  include <signal.h>
#endif
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QScreen>
#include <QOperatingSystemVersion>
#include <QStyleHints>
#include <QProxyStyle>
#include <QToolBar>
#include <QMenu>
#include <QEvent>
#include <QPainter>
#include <QStyleOptionMenuItem>

#ifdef WEBENGINEVIEW
#  include <QtWebEngineCore/qtwebenginecoreglobal.h>
#  if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#    include <QWebEngineGlobalSettings>
#  endif
#endif

#include <functional>
#include <stdlib.h>
#include <time.h>

#if defined(Q_OS_WIN)
#  include <QSharedMemory>
#  include <QAbstractNativeEventFilter>
#  include <windows.h>
#endif

#include "gadgets.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"
#include "systemmenu.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "treebank.hpp"
#include "sidepanels.hpp"
#include "bookmarkio.hpp"
#include "settingsio.hpp"
#include "certificatepolicy.hpp"
#include "notifier.hpp"
#include "saver.hpp"
#include "transmitter.hpp"
#include "translationorder.hpp"
#include "qtlog.hpp"
#include "receiver.hpp"
#include "localview.hpp"
#include "dialog.hpp"

#ifdef Q_OS_WIN
#include <QtGui/private/qguiapplication_p.h>
#endif
#ifdef EDGEWEBVIEW
#  include "edgewebview.hpp"
#endif

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

namespace {
    QProcessEnvironment BOOT_ENVIRONMENT;
}

Application *Application::m_Instance = nullptr;
bool Application::m_RestoringWindows = false;

static SettingsIO::Hooks SettingsHooks(){
    SettingsIO::Hooks hooks;
    hooks.BackUpFiltersOf = [](){ return Application::BackUpFileFilters();};
    hooks.BackUpPrepositionOf = [](){ return Application::BackUpPreposition();};
    hooks.RestoredFromBackUp = [](QString backup){
        ModelessDialog::Information
            (Application::tr("Restored from a backup file")+ QStringLiteral("\n[") + backup + QStringLiteral("]."),
             Application::tr("Because of a failure to read the latest file, it was restored from a backup file."));
    };
    return hooks;
}

NetworkController* Application::m_NetworkController = nullptr;
AutoSaver*  Application::m_AutoSaver         = nullptr;
quint64     Application::m_SerialStart       = 1;
bool        Application::m_Quitting          = false;
ShutdownFlow Application::m_ShutdownFlow([](){ Application::QueueTakeDown();});
Settings    Application::m_GlobalSettings    = Settings();
Settings    Application::m_IconTable         = Settings();

bool        Application::m_EnableGoogleSuggest = false;
QString     Application::m_SuggestEngine = QStringLiteral("Google");
bool        Application::m_EnableCustomTitleBar = false;
QString     Application::m_WindowEdgeColor = QStringLiteral("#000000");
bool        Application::m_EnableTransparentWindow = false;
bool        Application::m_TransparentWindowEffective = false;
bool        Application::m_EnableTranslucentPanels = false;
QString     Application::m_ColorScheme = QStringLiteral("Auto");
bool        Application::m_ReassertingColorScheme = false;
bool        Application::m_EnableAutoSave    = false;
bool        Application::m_EnableAutoLoad    = false;
int         Application::m_AutoSaveInterval  = 0;
int         Application::m_AutoLoadInterval  = 0;
int         Application::m_AutoSaveTimerId   = 0;
bool        Application::m_AutoSaveMayStart  = false;
int         Application::m_AutoLoadTimerId   = 0;
int         Application::m_MaxBackUpGenerationCount = 0;
QString     Application::m_DownloadDirectory = QString();
QString     Application::m_UploadDirectory   = QString();
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
                        const QWidget *widget) const override {
            const int base = QProxyStyle::pixelMetric(metric, option, widget);
            switch(metric){
            case PM_MenuVMargin:
            case PM_MenuHMargin: return base + DeviceScale::Primary(MENU_MARGIN);
            default:             return base;
            }
        }

        QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                               const QSize &size, const QWidget *widget) const override {
            QSize contents = QProxyStyle::sizeFromContents(type, option, size, widget);
            if(type == CT_MenuItem){
                const QStyleOptionMenuItem *item =
                    qstyleoption_cast<const QStyleOptionMenuItem*>(option);
                const bool separator =
                    item && item->menuItemType == QStyleOptionMenuItem::Separator;
                contents.setHeight(contents.height()
                                   + DeviceScale::Primary(separator ? MENU_SEPARATOR_EXTRA_HEIGHT
                                                                    : MENU_ITEM_EXTRA_HEIGHT));
                if(ShowsKeys(widget))
                    contents.setWidth(contents.width() + DeviceScale::Primary(MENU_KEY_GAP));
            }
            return contents;
        }

        static void DrawSeparator(QPainter *painter, const QRect &span, int y){
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setPen(Theme::Pen(Theme::MenuSeparator));
            painter->drawLine(span.left(), y, span.right(), y);
            painter->restore();
        }

        static void DrawSeparatorsAcross(const QWidget *widget, QPainter *painter){
            const QMenu *menu = qobject_cast<const QMenu*>(widget);
            if(!menu) return;
            for(QAction *action : menu->actions()){
                if(!action->isSeparator()) continue;
                const QRect rect = menu->actionGeometry(action);
                if(!rect.isEmpty()) DrawSeparator(painter, menu->rect(), rect.center().y());
            }
        }

        static bool ShowsKeys(const QWidget *widget){
            const QMenu *menu = qobject_cast<const QMenu*>(widget);
            if(!menu) return false;
            for(QAction *action : menu->actions())
                if(!action->shortcut().isEmpty() || action->text().contains(QLatin1Char('\t')))
                    return true;
            return false;
        }

        void drawControl(ControlElement element, const QStyleOption *option,
                         QPainter *painter, const QWidget *widget) const override {
            if(element == CE_MenuItem){
                const QStyleOptionMenuItem *item =
                    qstyleoption_cast<const QStyleOptionMenuItem*>(option);
                if(item && item->menuItemType == QStyleOptionMenuItem::Separator){
                    const QMenu *menu = qobject_cast<const QMenu*>(widget);
                    DrawSeparator(painter, menu ? menu->rect() : item->rect,
                                  item->rect.center().y());
                    return;
                }
                const qsizetype tab = item ? item->text.indexOf(QLatin1Char('\t')) : -1;
                if(item && tab >= 0 && item->menuItemType != QStyleOptionMenuItem::SubMenu){
                    QStyleOptionMenuItem narrowed(*item);
                    const int width = item->fontMetrics.horizontalAdvance(item->text.mid(tab + 1));
                    narrowed.reservedShortcutWidth = qMin(width, item->reservedShortcutWidth);
                    QProxyStyle::drawControl(element, &narrowed, painter, widget);
                    return;
                }
            }
            QProxyStyle::drawControl(element, option, painter, widget);
            if(element == CE_MenuEmptyArea) DrawSeparatorsAcross(widget, painter);
        }

        void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                           QPainter *painter, const QWidget *widget) const override {
            if(element == PE_PanelMenu && widget && widget->testAttribute(Qt::WA_TranslucentBackground)){
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->setPen(Theme::Pen(Theme::PreviewBorder));
                painter->setBrush(Theme::Brush(BlurBehind::Surface()));
                const int radius = DeviceScale::FromDpi(DIALOG_CORNER_RADIUS, static_cast<int>(widget->logicalDpiY()));
                const QRectF card = QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5);
                painter->drawRoundedRect(card, radius, radius);
                painter->restore();
                return;
            }
            if(element == PE_PanelButtonTool && OnABar(widget) &&
               (option->state & State_AutoRaise) && !(option->state & State_On)){
                const bool pressed = option->state & State_Sunken;
                if((option->state & State_Enabled) && (pressed || (option->state & State_MouseOver))){
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->setPen(Qt::NoPen);
                    const bool onField = widget->property(LineEdit::AddressHoverProperty).toBool();
                    painter->setBrush(Theme::Brush(pressed ? Theme::BarButtonPressed
                                                   : onField ? Theme::FieldButtonHovered
                                                             : Theme::BarButtonHovered));
                    const int radius = DeviceScale::FromDpi(CHIP_CORNER_RADIUS, static_cast<int>(widget->logicalDpiY()));
                    painter->drawRoundedRect(option->rect, radius, radius);
                    painter->restore();
                }
                return;
            }
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            if(element == PE_FrameMenu) DrawSeparatorsAcross(widget, painter);
        }

        static bool OnABar(const QWidget *widget){
            for(const QWidget *w = widget ? widget->parentWidget() : nullptr; w; w = w->parentWidget()){
                if(qobject_cast<const QToolBar*>(w) || w->inherits("TitleBar")) return true;
                if(w->isWindow()) return false;
            }
            return false;
        }

        void polish(QWidget *widget) override {
            QProxyStyle::polish(widget);
            if(BlurBehind::Enabled() && qobject_cast<QMenu*>(widget)){
                widget->setAttribute(Qt::WA_TranslucentBackground);
                widget->installEventFilter(this);
            }
        }

        bool eventFilter(QObject *watched, QEvent *ev) override {
            if(ev->type() == QEvent::Show)
                if(QWidget *widget = qobject_cast<QWidget*>(watched)) BlurBehind::Apply(widget);
            return QProxyStyle::eventFilter(watched, ev);
        }
    };
}

#if defined(Q_OS_WIN)
namespace {

    class PopupCloser : public QAbstractNativeEventFilter {
    public:
        bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override {
            Q_UNUSED(result)
            if(eventType != "windows_generic_MSG" || !message) return false;
            const MSG *msg = static_cast<const MSG*>(message);
            if(msg->message != WM_ACTIVATE || LOWORD(msg->wParam) != WA_INACTIVE) return false;
            if(DeactivatedByAnotherProcess(reinterpret_cast<HWND>(msg->lParam)))
                Application::CloseAllPopups();
            return false;
        }
        static bool DeactivatedByAnotherProcess(HWND activated){
            if(!activated) return true;
            DWORD pid = 0;
            GetWindowThreadProcessId(activated, &pid);
            return pid != GetCurrentProcessId();
        }
    };
}
#endif

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setStyle(new MenuStyle());
    setFont(MenuFont(), "QMenu");
    m_NetworkController = nullptr;
    m_AutoSaver = nullptr;
    srand(static_cast<unsigned int>(time(nullptr)));
#if defined(Q_OS_WIN)
    static PopupCloser popupCloser;
    installNativeEventFilter(&popupCloser);
#endif
    OwnedRaise::Install();
}

void Application::CloseAllPopups(){
    while(QWidget *popup = QApplication::activePopupWidget()){
        popup->close();
        if(QApplication::activePopupWidget() == popup) break;
    }
}

Application::~Application(){
    TreeBank::StopTellingTabs();
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
    if(SwallowsDuringRestore(receiver, ev)) return true;
    try{
        if(const std::optional<bool> byKey = ByKeyFor(ev)){
            InputScope scope(*byKey);
            return QApplication::notify(receiver, ev);
        }
        return QApplication::notify(receiver, ev);
    } catch(std::exception &e){
        EmitErrorMessage(receiver, ev, e);
    } catch(...){
        EmitErrorMessage(receiver, ev);
    }

}

void Application::BootApplication(int &argc, char **argv, Application *instance){
    BOOT_ENVIRONMENT = QProcessEnvironment::systemEnvironment();
    WaitForPredecessor(argc, argv);

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
    QtLog::Install(StateDirectory() + QStringLiteral("qt-warnings.log"));

    const QString path = applicationDirPath() + QStringLiteral("/translations");
    const QStringList locales = TranslationOrder::Locales(QLocale::system().uiLanguages());
    const QStringList prefixes = QStringList()
        << QStringLiteral("qt")     << QStringLiteral("qtbase")
        << QStringLiteral("custom") << QStringLiteral("vanilla");

    for(QString locale : locales){
        foreach(QString prefix, prefixes){
            QTranslator *translator = new QTranslator(instance);
            if(translator->load(prefix + QStringLiteral("_") + locale, path))
                installTranslator(translator);
            else delete translator;
        }
    }
    LoadSettingsFile();
    LoadGlobalSettings();
    ExtensionController::FollowVersionFolders();
#ifdef EDGEWEBVIEW
    EdgeWebView::StartExtensionCopiesAhead();
#endif
    ApplyGraphicsApi();
#ifdef Q_OS_WIN
    if(QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGL)
        if(auto *windows = qGuiApp->nativeInterface<QNativeInterface::Private::QWindowsApplication>())
            windows->setHasBorderInFullScreenDefault(true);
#endif
    Theme::ApplyScheme(m_ColorScheme);
    SystemMenu::ApplyScheme(Theme::IsDark());
    connect(styleHints(), &QStyleHints::colorSchemeChanged,
            instance, [](Qt::ColorScheme){
                if(m_ReassertingColorScheme) return;
                if(Theme::ApplyScheme(m_ColorScheme)) UpdateAllWidgets();
            });
    LoadIconDatabase();

    ApplyChromiumFlags();
    ApplyGlobalWebEngineSettings();

    SeedSerials();
    TreeBar::Initialize();
    ToolBar::Initialize();
    TreeBank::Initialize();
    m_AutoSaver = new AutoSaver();
    SetRestoringWindows(true);
    foreach(int id, SavedWindowIds(GlobalSettings())){
        MainWindow *win = NewWindow(id, QPoint(), false);
        win->ShowWithFirstFrame();
    }
#ifdef EDGEWEBVIEW
    EdgeWebView::HurryExtensionCopiesAhead();
#endif
    m_NetworkController = new NetworkController();
    const TreeBank::Restoration restored = TreeBank::LoadTree();
    DropIdleShells(restored.windowIds, restored.lastWindow);
    SetRestoringWindows(false);
    if(restored.windowIds.isEmpty()){
        MainWindow *win = Windows().Current();
        if(!win) win = NewWindow(0, QPoint(), TreeBank::PurgeView());
        win->GetTreeBank()->OpenOnSuitableNode(START_URL, true);
        win->ShowWithFirstFrame();
    }
    SweepWindowSettings(GlobalSettings(), Windows().Ids());

    QTimer::singleShot(0, [=](){

        CreateBackUpFiles();
        m_AutoSaveMayStart = true;
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
    const QString edgeBookmarks    = BookmarkIO::EdgeBookmarkFile();

    const bool supportsIE      = !ieFavorites.isEmpty();
    const bool supportsFirefox = QFile::exists(firefoxProfile);
    const bool supportsChrome  = QFile::exists(chromeBookmarks);
    const bool supportsOPR     = QFile::exists(oprBookmarks);
    const bool supportsVivaldi = QFile::exists(vivaldiBookmarks);
    const bool supportsEdge    = QFile::exists(edgeBookmarks);

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
    if(supportsEdge) list << QStringLiteral("Edge");
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
    else if(which == QStringLiteral("Edge")) importFromChromeFamily(edgeBookmarks);
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
    QStringList lines;
    lines << tr("Version %1").arg(QStringLiteral(VANILLA_VERSION));
    lines << tr("Qt %1").arg(QString::fromLatin1(qVersion()));
#ifdef WEBENGINEVIEW
    lines << tr("Qt WebEngine %1 (Chromium %2)")
        .arg(QString::fromLatin1(qWebEngineVersion()),
             QString::fromLatin1(qWebEngineChromiumVersion()));
#endif
#ifdef EDGEWEBVIEW
    const QString runtime = EdgeWebView::RuntimeVersion();
    lines << (runtime.isEmpty() ? tr("WebView2 runtime: not installed")
                                : tr("WebView2 runtime %1").arg(runtime));
#endif
    lines << tr("Data folder: %1").arg(QDir::toNativeSeparators(StateDirectory()));

    QMessageBox box(CurrentWidget());
    box.setWindowTitle(QStringLiteral("vanilla"));
    box.setIconPixmap(windowIcon().pixmap(64, 64));
    box.setText(tr("vanilla is a simple web browser."));
    box.setInformativeText(lines.join(QLatin1Char('\n')));
    box.setTextInteractionFlags(Qt::TextSelectableByMouse);
    box.setStandardButtons(QMessageBox::Ok);

    const QFontMetrics metrics(box.font());
    int widest = 0;
    for(const QString &line : std::as_const(lines))
        widest = qMax(widest, metrics.horizontalAdvance(line));
    const QScreen *screen = CurrentWidget() ? CurrentWidget()->screen() : primaryScreen();
    const int limit = screen ? screen->availableGeometry().width() * 3 / 4 : 1200;
    box.setStyleSheet(QStringLiteral("QLabel{min-width:%1px;}")
                      .arg(qMin(widest + 48, limit)));
    box.exec();
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

bool Application::m_RestartRequested = false;
bool Application::m_ByKey = false;

void Application::Restart(){
    m_RestartRequested = true;
    Quit();
}

void Application::RelaunchIfRequested(){
    if(!m_RestartRequested) return;
    m_RestartRequested = false;
#ifndef QT_NO_PROCESS
    QProcess process;
    process.setProgram(applicationFilePath());
    process.setArguments(QStringList() << QStringLiteral("--restart-after=")
                                        + QString::number(applicationPid()));
    process.setWorkingDirectory(applicationDirPath());
    process.setProcessEnvironment(BOOT_ENVIRONMENT);
    if(!process.startDetached())
        qWarning() << "restart: could not start" << applicationFilePath();
#endif
}

void Application::WaitForPredecessor(int argc, char **argv){
    const QString prefix = QStringLiteral("--restart-after=");
    for(int i = 1; i < argc; i++){
        const QString arg = QLatin1String(argv[i]);
        if(!arg.startsWith(prefix)) continue;
        const qint64 pid = arg.mid(prefix.length()).toLongLong();
        if(pid <= 0) return;
#if defined(Q_OS_WIN)
        if(HANDLE process = ::OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid))){
            ::WaitForSingleObject(process, RESTART_WAIT_LIMIT);
            ::CloseHandle(process);
        }
#else
        QElapsedTimer timer;
        timer.start();
        while(::kill(static_cast<pid_t>(pid), 0) == 0 &&
              timer.elapsed() < RESTART_WAIT_LIMIT)
            QThread::msleep(50);
#endif
        return;
    }
}

void Application::QueueTakeDown(){
    QTimer::singleShot(0, GetInstance(), [](){ TakeDown();});
}

int Application::Run(){
    return ExitHandoff::Run<Application>(this, m_ShutdownFlow);
}

void Application::RequestExit(){
    ExitHandoff::RequestExit(GetInstance(), m_ShutdownFlow);
}

void Application::WaitForPriorSave(){
    if(!m_AutoSaver->IsSaving()){
        m_ShutdownFlow.PriorSaveSettled();
        return;
    }

    const auto resume = [](){
        if(m_ShutdownFlow.CurrentStage() != ShutdownFlow::Stage::WaitingForPriorSave) return;
        m_AutoSaver->disconnect(GetInstance());
        m_ShutdownFlow.PriorSaveSettled();
    };
    connect(m_AutoSaver, &AutoSaver::Finished, GetInstance(), resume);
    connect(m_AutoSaver, &AutoSaver::Failed,   GetInstance(), resume);
    if(!m_AutoSaver->IsSaving()) resume();
}

void Application::TakeDown(){
    DownloadHistory::Instance()->SetClosing();

    if(ModalDialog::AnyRunning()){
        ModalDialog::AbortAll();
        QueueTakeDown();
        return;
    }

    if(m_ShutdownFlow.CurrentStage() == ShutdownFlow::Stage::Start){
        StopAutoLoadTimer();
        StopAutoSaveTimer();

        for(MainWindow *win : Windows().All().values()){
            win->hide();
        }

        if(m_ShutdownFlow.Begin() == ShutdownFlow::Action::WaitForPriorSave)
            WaitForPriorSave();
        return;
    }

    if(m_ShutdownFlow.CurrentStage() == ShutdownFlow::Stage::ReadyForFinalSave &&
       m_AutoSaver->IsSaving()){
        if(m_ShutdownFlow.WaitForAnotherSave()) WaitForPriorSave();
        return;
    }

    const ShutdownFlow::Action action = m_ShutdownFlow.Continue();
    if(action == ShutdownFlow::Action::None)
        return;

    if(action == ShutdownFlow::Action::CaptureMedia){
#ifdef MEDIATIME
        TreeBank::SaveMediaTimesForQuit([](){
            m_ShutdownFlow.MediaSettled();
        });
        return;
#else
        m_ShutdownFlow.MediaSettled();
        return;
#endif
    }

    if(action != ShutdownFlow::Action::FinalSave) return;
#ifdef LOCALVIEW
    LocalView::ClearCache();
#endif
#ifdef EDGEWEBVIEW

    EdgeWebView::ReleaseDownloadCarriers();
#endif
    SidePanels::ShutdownEverywhere();
    TreeBank::ReleaseAllView();

    const auto finish = [](){
        m_AutoSaver->disconnect(GetInstance());
        QTimer::singleShot(0, GetInstance(), [](){ RequestExit();});
    };
    connect(m_AutoSaver, &AutoSaver::Finished, GetInstance(), finish);
    connect(m_AutoSaver, &AutoSaver::Failed,   GetInstance(), finish);

    TreeBank::DoDelete();
    m_AutoSaver->SaveAll();
    KeepSerials(Node::SerialAfter(m_SerialStart, Node::NextSerial()));
}

static QString SerialFile(){
    return Application::StateDirectory() + QStringLiteral("serial-base");
}

void Application::SeedSerials(){
    QByteArray kept;
    {
        QFile file(SerialFile());
        if(file.open(QIODevice::ReadOnly)) kept = file.read(32);
    }
    m_SerialStart = Node::SerialStart(kept);
    Node::SeedSerials(m_SerialStart);
    KeepSerials(Node::SerialAfter(m_SerialStart, m_SerialStart));
}

void Application::KeepSerials(quint64 next){
    QSaveFile file(SerialFile());
    if(!file.open(QIODevice::WriteOnly) || file.write(QByteArray::number(next)) < 0 || !file.commit())
        qWarning() << "where the next run's serials begin could not be written to" << SerialFile();
}

Settings &Application::GlobalSettings(){
    return m_GlobalSettings;
}

void Application::SaveGlobalSettings(){
    Settings &s = GlobalSettings();

    s.setValue(QStringLiteral("application/@EnableGoogleSuggest"),      m_EnableGoogleSuggest);
    s.setValue(QStringLiteral("application/@SuggestEngine"),            m_SuggestEngine);
    s.setValue(QStringLiteral("application/@EnableCustomTitleBar"),     m_EnableCustomTitleBar);
    s.setValue(QStringLiteral("application/@WindowEdgeColor"),          m_WindowEdgeColor);
    s.setValue(QStringLiteral("application/@EnableTransparentWindow"),     m_EnableTransparentWindow);
    s.setValue(QStringLiteral("application/@EnableTranslucentPanels"),  m_EnableTranslucentPanels);
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

    const QStringList dropped = SettingsSchema::DropUnknownKeys(s);
    if(!dropped.isEmpty())
        qWarning() << "settings: dropped keys nothing reads:" << dropped;

    m_EnableGoogleSuggest      = s.value(QStringLiteral("application/@EnableGoogleSuggest"), false).value<bool>();
    m_SuggestEngine            = s.value(QStringLiteral("application/@SuggestEngine"), QStringLiteral("Google")).toString();
    m_EnableCustomTitleBar     = s.value(QStringLiteral("application/@EnableCustomTitleBar"), false).value<bool>();
    m_WindowEdgeColor          = s.value(QStringLiteral("application/@WindowEdgeColor"), QStringLiteral("#000000")).value<QString>();
    m_EnableTransparentWindow     = s.value(QStringLiteral("application/@EnableTransparentWindow"), false).value<bool>();
    m_EnableTranslucentPanels  = s.value(QStringLiteral("application/@EnableTranslucentPanels"), false).value<bool>();
    m_ColorScheme              = s.value(QStringLiteral("application/@ColorScheme"), QStringLiteral("Auto")).value<QString>();
    m_EnableAutoSave           = s.value(QStringLiteral("application/@EnableAutoSave"), true).value<bool>();
    m_EnableAutoLoad           = s.value(QStringLiteral("application/@EnableAutoLoad"), true).value<bool>();
    m_AutoSaveInterval         = s.value(QStringLiteral("application/@AutoSaveInterval"), 300000).value<int>();
    m_AutoLoadInterval         = s.value(QStringLiteral("application/@AutoLoadInterval"), 1000).value<int>();
    m_MaxBackUpGenerationCount = s.value(QStringLiteral("application/@MaxBackUpGenerationCount"), 5).value<int>();
    m_DownloadDirectory        = s.value(QStringLiteral("application/@FileSaveDirectory"), QString()).value<QString>();
    m_UploadDirectory          = s.value(QStringLiteral("application/@FileOpenDirectory"), QString()).value<QString>();
    m_SaveSessionCookie        = s.value(QStringLiteral("application/@SaveSessionCookie"), false).value<bool>();
    m_AcceptLanguage           = s.value(QStringLiteral("application/@AcceptLanguage"), QString()).value<QString>();
    if(m_AcceptLanguage == QStringLiteral("ja,en-US;q=0.8,en;q=0.6") ||
       m_AcceptLanguage == QStringLiteral("en-US"))
        m_AcceptLanguage.clear();
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
    const QStringList lines =
        m_GlobalSettings.value(QStringLiteral("application/@ChromiumFlags"),
                               QStringList()).value<QStringList>();

    QStringList ignored;
    QStringList accepted = ChromiumSwitchesIn(lines, &ignored);
    foreach(const QString &word, ignored){
        qWarning() << "ignoring" << word << "in application/@ChromiumFlags:"
                   << "not a switch";
    }

    const QByteArray existing = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");

    if(accepted.isEmpty()) return;

    const QByteArray added = JoinChromiumSwitches(accepted, false).toLocal8Bit();
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            existing.isEmpty() ? added : existing + ' ' + added);
}

QStringList Application::ChromiumSwitches(const QString &flags, QStringList *ignored){
    QStringList words;
    QString word;
    bool quoted = false;
    foreach(const QChar c, flags){
        if(c == QLatin1Char('"')){
            quoted = !quoted;
        } else if(c.isSpace() && !quoted){
            if(!word.isEmpty()) words << word;
            word.clear();
        } else {
            word += c;
        }
    }
    if(!word.isEmpty()) words << word;

    QStringList accepted;
    foreach(const QString &each, words){
        if(each.startsWith(QStringLiteral("--"))) accepted << each;
        else if(ignored) *ignored << each;
    }
    return accepted;
}

QStringList Application::ChromiumSwitchesIn(const QStringList &lines, QStringList *ignored){
    QStringList accepted;
    foreach(const QString &line, lines) accepted << ChromiumSwitches(line, ignored);
    return accepted;
}

QString Application::JoinChromiumSwitches(const QStringList &switches, bool forWindows){
    QStringList words;
    foreach(QString each, switches){
        each.remove(QLatin1Char('"'));
        if(std::none_of(each.cbegin(), each.cend(),
                        [](const QChar c){ return c.isSpace();})){
            words << each;
            continue;
        }
        int slashes = 0;
        if(forWindows){
            while(slashes < each.length() &&
                  each.at(each.length() - 1 - slashes) == QLatin1Char('\\')) slashes++;
        }
        words << QLatin1Char('"') + each + QString(slashes, QLatin1Char('\\')) + QLatin1Char('"');
    }
    return words.join(QLatin1Char(' '));
}

QSGRendererInterface::GraphicsApi Application::GraphicsApiFor(const QString &value){
    const QString word = value.toLower();
    if(word == QStringLiteral("software")) return QSGRendererInterface::Software;
    if(word == QStringLiteral("opengl"))   return QSGRendererInterface::OpenGL;
    if(word == QStringLiteral("direct3d11") ||
       word == QStringLiteral("d3d11"))    return QSGRendererInterface::Direct3D11;
    if(word == QStringLiteral("direct3d12") ||
       word == QStringLiteral("d3d12"))    return QSGRendererInterface::Direct3D12;
    if(word == QStringLiteral("vulkan"))   return QSGRendererInterface::Vulkan;
    if(word == QStringLiteral("metal"))    return QSGRendererInterface::Metal;
    return QSGRendererInterface::Unknown;
}

bool Application::GraphicsApiRunsHere(QSGRendererInterface::GraphicsApi api){
    switch(api){
    case QSGRendererInterface::Software:
    case QSGRendererInterface::OpenGL:
        return true;
    case QSGRendererInterface::Direct3D11:
    case QSGRendererInterface::Direct3D12:
#if defined(Q_OS_WIN)
        return true;
#else
        return false;
#endif
    case QSGRendererInterface::Metal:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        return true;
#else
        return false;
#endif
    case QSGRendererInterface::Vulkan:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        return false;
#else
        return true;
#endif
    default:
        return false;
    }
}

void Application::ApplyGraphicsApi(){
    const SettingsSchema::Item *item = SettingsSchema::Find(QStringLiteral("application/@GraphicsApi"));
    const QString value =
        m_GlobalSettings.value(QStringLiteral("application/@GraphicsApi"),
                               item ? SettingsSchema::Fallback(*item)
                                    : QVariant(QStringLiteral("Auto"))).value<QString>();
    QSGRendererInterface::GraphicsApi api = GraphicsApiFor(value);

    const bool variable = qEnvironmentVariableIsSet("QSG_RHI_BACKEND");
    if(variable && api != QSGRendererInterface::Unknown){
        qWarning() << "application/@GraphicsApi" << value
                   << "ignored: QSG_RHI_BACKEND is set to"
                   << qgetenv("QSG_RHI_BACKEND");
        api = QSGRendererInterface::Unknown;
    }
    if(api != QSGRendererInterface::Unknown && !GraphicsApiRunsHere(api)){
        qWarning() << "application/@GraphicsApi" << value
                   << "ignored: this platform cannot draw with it";
        api = QSGRendererInterface::Unknown;
    }
    if(api != QSGRendererInterface::Unknown)
        QQuickWindow::setGraphicsApi(api);

    ApplyWidgetsRhi(
        m_GlobalSettings.value(QStringLiteral("application/@EnableMainWindowRhi"),
                               false).value<bool>(),
        api, variable);

    m_TransparentWindowEffective =
        m_EnableTransparentWindow &&
        BlurBehind::Available() &&
        BlurBehind::MainWindowMayBeTranslucent(qEnvironmentVariableIntValue("QT_WIDGETS_RHI"),
                                               qEnvironmentVariableIntValue("QT_WIDGETS_HIGHDPI_DOWNSCALE"),
                                               qgetenv("QT_WIDGETS_RHI_BACKEND"));
}

void Application::ApplyWidgetsRhi(bool enabled, QSGRendererInterface::GraphicsApi api, bool variable){
    if(!enabled) return;
    if(qEnvironmentVariableIsSet("QT_WIDGETS_RHI")) return;
    if(api == QSGRendererInterface::Software) return;
    QByteArray backend = WidgetsRhiBackendFor(api);
    if(api == QSGRendererInterface::Unknown && variable){
        backend = qgetenv("QSG_RHI_BACKEND").toLower();
        if(backend == QByteArrayLiteral("software")) return;
        if(WidgetsRhiBackendFor(GraphicsApiFor(QString::fromLatin1(backend))).isEmpty())
            backend.clear();
    }
    qputenv("QT_WIDGETS_RHI", "1");
    if(!backend.isEmpty() && !qEnvironmentVariableIsSet("QT_WIDGETS_RHI_BACKEND"))
        qputenv("QT_WIDGETS_RHI_BACKEND", backend);
}

QByteArray Application::WidgetsRhiBackendFor(QSGRendererInterface::GraphicsApi api){
    switch(api){
    case QSGRendererInterface::OpenGL:     return QByteArrayLiteral("opengl");
    case QSGRendererInterface::Direct3D11: return QByteArrayLiteral("d3d11");
    case QSGRendererInterface::Direct3D12: return QByteArrayLiteral("d3d12");
    case QSGRendererInterface::Vulkan:     return QByteArrayLiteral("vulkan");
    case QSGRendererInterface::Metal:      return QByteArrayLiteral("metal");
    default:                               return QByteArray();
    }
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

bool Application::SaveSettingsFile(const Settings &snapshot){
    return SettingsIO::Save(StateDirectory(), GlobalSettingsFileName(), snapshot, SettingsHooks());
}

void Application::LoadSettingsFile(){
    SettingsIO::Load(StateDirectory(), GlobalSettingsFileName(), m_GlobalSettings, SettingsHooks());
}

Settings Application::IconDatabaseSnapshot(){
    return m_IconTable;
}

bool Application::SaveIconDatabase(const Settings &snapshot){
    return SettingsIO::Save(StateDirectory(), IconDatabaseFileName(), snapshot, SettingsHooks());
}

void Application::LoadIconDatabase(){
    SettingsIO::Load(StateDirectory(), IconDatabaseFileName(), m_IconTable, SettingsHooks());
}

void Application::RegisterIcon(QString host, QIcon icon, bool offTheRecord){
    if(offTheRecord) return;
    if(!host.isEmpty() && !icon.isNull()){
        QSize size = icon.availableSizes().first();
        if(size.width() > 32) size = QSize(32, 32);
        icon = QIcon(icon.pixmap(size));
        m_IconTable[host] = QVariant::fromValue(icon);
        TreeBank::TabsLookChanged();
    }
}

QIcon Application::GetIcon(QString host){
    if(m_IconTable.contains(host))
        return m_IconTable[host].value<QIcon>();
    return QIcon();
}

QIcon Application::GetIcon(const QUrl &url){
    return GetIcon(url, false);
}

QIcon Application::GetIcon(const QUrl &url, bool loaded){
    if(url.scheme() == QStringLiteral("vanilla") && url.host() == QStringLiteral("settings")){
        const QString name = loaded ? QStringLiteral(":/resources/settings/settings-loaded")
                                    : QStringLiteral(":/resources/settings/settings");
        QIcon icon;
        icon.addPixmap(Theme::Pixmap(name + QStringLiteral(".png"), Theme::BarIcon));
        icon.addPixmap(Theme::Pixmap(name + QStringLiteral("@2x.png"), Theme::BarIcon));
        return icon;
    }
    return GetIcon(url.host());
}

void Application::Reconfigure(){
    const bool wasAutoSave = m_EnableAutoSave;
    const int  wasInterval = m_AutoSaveInterval;
    const QString wasEdgeColor = m_WindowEdgeColor;
    LoadGlobalSettings();
    if(m_WindowEdgeColor != wasEdgeColor){
        for(MainWindow *win : Windows().All().values())
            win->RepaintWindowEdge();
    }
    if(m_AutoSaveMayStart && !IsQuitting() &&
       (m_EnableAutoSave != wasAutoSave || m_AutoSaveInterval != wasInterval))
        RestartAutoSaveTimer();
    const bool schemeChanged = Theme::ApplyScheme(m_ColorScheme);
    LoadIconDatabase();
    TreeBar::LoadSettings();
    ToolBar::LoadSettings();
    TreeBank::LoadSettings();
#ifdef WEBENGINEVIEW
    NetworkController::ApplyAcceptLanguage();
#endif
    ExtensionController::ReloadAll();
    for(MainWindow *win : Windows().All().values()){
        TreeBank *tb = win->GetTreeBank();
        if(Notifier *notifier = tb->GetNotifier()){
            notifier->ResizeNotify(tb->size());
            notifier->update();
        }
    }
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

QString Application::SuggestEngine(){
    return m_SuggestEngine;
}

bool Application::EnableCustomTitleBar(){
    return m_EnableCustomTitleBar;
}

QString Application::WindowEdgeColor(){
    return m_WindowEdgeColor;
}

bool Application::EnableTransparentWindow(){
    return m_TransparentWindowEffective;
}

void Application::TranslucentWindowUnsupported(){
    if(!m_TransparentWindowEffective) return;
    m_TransparentWindowEffective = false;
    UpdateAllWidgets();
}

bool Application::EnableTranslucentPanels(){
    return m_EnableTranslucentPanels;
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
    SystemMenu::ApplyScheme(Theme::IsDark());
    foreach(QWidget *widget, allWidgets()){
        if(LineEdit *edit = qobject_cast<LineEdit*>(widget)) edit->ApplyTheme();
        if(ModalDialog *dialog = qobject_cast<ModalDialog*>(widget)) dialog->ApplyTheme();
        if(ModelessDialog *dialog = qobject_cast<ModelessDialog*>(widget)) dialog->ApplyTheme();
        if(ExtensionBar *bar = qobject_cast<ExtensionBar*>(widget)) bar->ApplyTheme();
        if(DownloadBar *bar = qobject_cast<DownloadBar*>(widget)) bar->ApplyTheme();
        if(SiteButton *button = qobject_cast<SiteButton*>(widget)) button->ApplyTheme();
        if(DownloadPanel *panel = qobject_cast<DownloadPanel*>(widget)) panel->ApplyTheme();
        widget->update();
    }
    for(MainWindow *win : Windows().All().values()){
        win->ApplyTheme();
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

bool Application::SaveSessionCookie(){
    return m_SaveSessionCookie;
}

QString Application::GetAcceptLanguage(){
    if(m_AcceptLanguage.isEmpty())
        return UserAgent::DefaultAcceptLanguage(QLocale::system().uiLanguages());
    return m_AcceptLanguage;
}

QStringList Application::GetAllowedHosts(){
    return m_AllowedHosts;
}

void Application::AppendToAllowedHosts(QString host, bool offTheRecord){
    if(offTheRecord) return;
    if(!m_AllowedHosts.contains(host))
        m_AllowedHosts << host;
}

QStringList Application::GetBlockedHosts(){
    return m_BlockedHosts;
}

void Application::AppendToBlockedHosts(QString host, bool offTheRecord){
    if(offTheRecord) return;
    if(!m_BlockedHosts.contains(host))
        m_BlockedHosts << host;
}

QStringList Application::GetAllowedCertificates(){
    return m_AllowedCertificates;
}

QStringList Application::GetBlockedCertificates(){
    return m_BlockedCertificates;
}

void Application::RememberCertificate(QString key, bool allow, bool offTheRecord){
    if(offTheRecord) return;
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
            (tr("SSL error policy"),
             tr("Select an SSL error policy."),
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

bool Application::IsQuitting(){
    return m_Quitting || m_ShutdownFlow.CurrentStage() != ShutdownFlow::Stage::Start;
}

QString Application::LocalServerName(){
    return VANILLA_LOCAL_SERVER_NAME_PREFIX +
        QString::fromLatin1(QCryptographicHash::hash(applicationDirPath().toUtf8(), QCryptographicHash::Md5).toHex());
}

QString Application::SharedMemoryKey(){
    return VANILLA_SHARED_MEMORY_KEY_PREFIX +
        QString::fromLatin1(QCryptographicHash::hash(applicationDirPath().toUtf8(), QCryptographicHash::Md5).toHex());
}

namespace {
    QString RandomToken(){
        QString token;
        quint32 words[4];
        QRandomGenerator::system()->fillRange(words);
        for(quint32 word : words)
            token += QStringLiteral("%1").arg(word, 8, 16, QLatin1Char('0'));
        return token;
    }
}

QString Application::EventToken(){
    static const QString token = RandomToken();
    return token;
}

bool Application::PageKeyFromMessage(const QString &msg, PageKey &key){
    static const QString flags = QStringLiteral(",([0-9]+),(true|false),(true|false),(true|false),(true|false)");
    if(ExactMatch(QStringLiteral("keyPressEvent") + EventToken() + flags, msg))
        key.type = QEvent::KeyPress;
    else if(ExactMatch(QStringLiteral("keyReleaseEvent") + EventToken() + flags, msg))
        key.type = QEvent::KeyRelease;
    else return false;
    const QStringList args = msg.split(QStringLiteral(","));
    key.key = JsKeyToQtKey(args[1].toInt());
    key.modifiers = Qt::NoModifier;
    if(args[2] == QStringLiteral("true")) key.modifiers |= Qt::ShiftModifier;
    if(args[3] == QStringLiteral("true")) key.modifiers |= Qt::ControlModifier;
    if(args[4] == QStringLiteral("true")) key.modifiers |= Qt::AltModifier;
    if(args[5] == QStringLiteral("true")) key.modifiers |= Qt::MetaModifier;
    return true;
}

QString Application::EdgeEventToken(){
    static const QString token = RandomToken();
    return token;
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

MainWindow *Application::NewWindow(int id, QPoint pos, bool showNow){
    if(id == 0) id = Windows().UnusedId();

    TreeBank::LiftMaxViewCountIfNeed(Windows().Count());

    MainWindow *win = new MainWindow(id, pos, showNow);
    Windows().Insert(id, win);

#if defined(Q_OS_MAC)
    if(Windows().Current() && pos.isNull() && ProductVersion().startsWith(QStringLiteral("10.14"))){
        QTimer::singleShot(16, win, [win](){
            if(IsRestoringWindows()) return;
            Windows().SetCurrent(win);
            TreeBank::SomethingChanged();
        });
        return win;
    }
#endif
    Windows().SetCurrent(win);
    TreeBank::SomethingChanged();
    return win;
}

bool Application::SwallowsDuringRestore(QObject *receiver, QEvent *ev){
    if(!m_RestoringWindows) return false;
    switch(ev->type()){
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
    case QEvent::Shortcut:
    case QEvent::ContextMenu:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::NativeGesture:
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
    case QEvent::Drop:
        break;
    default:
        return false;
    }
    QWidget *widget = qobject_cast<QWidget*>(receiver);
    if(!widget) return false;
    QWidget *window = widget->window();
    return qobject_cast<MainWindow*>(window) ||
           qobject_cast<TitleBar*>(window) ||
           qobject_cast<MainWindowEdgeWidget*>(window);
}

void Application::SweepWindowSettings(Settings &s, const QList<int> &ids){
    QStringList keys = s.allKeys(QStringLiteral("mainwindow"));
    foreach(int id, ids){
        keys.removeOne(QStringLiteral("mainwindow/tableview%1").arg(id));
        keys.removeOne(QStringLiteral("mainwindow/geometry%1").arg(id));
        keys.removeOne(QStringLiteral("mainwindow/minimap%1").arg(id));
        keys.removeOne(QStringLiteral("mainwindow/menubar%1").arg(id));
        keys.removeOne(QStringLiteral("mainwindow/toolbar%1").arg(id));
        keys.removeOne(QStringLiteral("mainwindow/treebar%1").arg(id));
        keys.removeOne(QStringLiteral("mainwindow/status%1").arg(id));
        keys.removeOne(QStringLiteral("mainwindow/titlebartoolbar%1").arg(id));
        keys.removeOne(QStringLiteral("mainwindow/stripexpanded%1").arg(id));
    }
    foreach(QString key, keys){
        s.remove(key);
    }
}

QList<int> Application::SavedWindowIds(const Settings &s){
    static const QRegularExpression key
        (QStringLiteral("^mainwindow/(?:tableview|geometry|notifier|receiver|menubar|toolbar|treebar|status)(\\d+)$"));
    QSet<int> ids;
    for(auto it = s.constBegin(); it != s.constEnd(); ++it){
        const QRegularExpressionMatch m = key.match(it.key());
        if(!m.hasMatch()) continue;
        const int id = m.captured(1).toInt();
        if(id > 0) ids << id;
    }
    QList<int> list = ids.values();
    std::sort(list.begin(), list.end());
    return list;
}

void Application::DropIdleShells(const QSet<int> &restored, MainWindow *lastRestored){
    if(lastRestored) Windows().SetCurrent(lastRestored);
    MainWindow *keep = restored.isEmpty() ? Windows().Current() : nullptr;
    for(MainWindow *win : Windows().All().values()){
        if(win == keep) continue;
        if(restored.contains(Windows().IdOf(win))) continue;
        if(win->GetTreeBank()->GetCurrentViewNode()) continue;
        RemoveWindow(win);
        foreach(ModelessDialog *dialog, win->DialogFrame()->Dialogs()){
            ModelessDialogFrame::DeregisterDialog(dialog);
            ModelessDialogFrame::RegisterDialog(dialog);
        }
        delete win;
    }
}

MainWindow *Application::CloseWindow(MainWindow *win){
    if(!win) win = GetCurrentWindow();
    if(win) win->close();
    return GetCurrentWindow();
}

MainWindow *Application::SwitchWindow(bool next){
    MainWindow *win = Windows().Switch(next);
    TreeBank::SomethingChanged();
    return win;
}

MainWindow *Application::NextWindow(){
    return SwitchWindow(true);
}

MainWindow *Application::PrevWindow(){
    return SwitchWindow(false);
}

void Application::RemoveWindow(MainWindow *win){
    Windows().Remove(win);
    TreeBank::SomethingChanged();
}

void Application::RemoveWindow(int id){
    Windows().Remove(id);
    TreeBank::SomethingChanged();
}

void Application::SetCurrentWindow(MainWindow *win){
    Windows().SetCurrent(win);
    TreeBank::SomethingChanged();
}

void Application::SetCurrentWindow(int id){
    Windows().SetCurrent(id);
    TreeBank::SomethingChanged();
}

MainWindow *Application::Window(int id){
    return Windows().At(id);
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

        const int width = rect.width() / MODELESS_DIALOG_WIDTH_DIVISOR;
        m_TemporaryDialogFrame->setGeometry
            (rect.x() + rect.width() - width,
             rect.y() + rect.height() / 6,
             width, rect.height() * 2 / 3);
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

bool Application::MayOpenWithDefaultBrowser(const QUrl &url){
    if(!url.isValid() || url.isEmpty()) return false;
    const QString scheme = url.scheme().toLower();
    if(scheme == QStringLiteral("http") || scheme == QStringLiteral("https") ||
       scheme == QStringLiteral("ftp")) return true;
    if(scheme != QStringLiteral("file") || !url.host().isEmpty()) return false;
    const QString path = url.toLocalFile();
    if(path.isEmpty() || path.startsWith(QStringLiteral("//")) ||
       path.contains(QLatin1Char('\\'))) return false;
    for(const QChar c : path)
        if(c.unicode() < 0x20 || c.unicode() == 0x7f) return false;
#if defined(Q_OS_WIN)
    if(path.indexOf(QLatin1Char(':'), 2) >= 0) return false;
    for(const QString &segment : path.split(QLatin1Char('/')))
        if(segment.endsWith(QLatin1Char('.')) || segment.endsWith(QLatin1Char(' '))) return false;
#endif
    return !DownloadHistory::IsRunnable(path);
}

bool Application::OpenUrlWithDefaultBrowser(QUrl url){
    if(!MayOpenWithDefaultBrowser(url)) return false;
    return QDesktopServices::openUrl(url);
}

QList<QPair<QString, QString> > Application::ExternalCommands(){
    QList<QPair<QString, QString> > list;
    const QStringList lines =
        m_GlobalSettings.value(QStringLiteral("application/@ExternalCommands"),
                               QStringList()).value<QStringList>();
    for(QString line : lines){
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
        if(!m_AutoSaver->IsSaving()) m_AutoSaver->SaveAllAsync();
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

    for(QString file : std::as_const(backupfiles)){
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
        m_AutoSaveTimerId = m_Instance->startTimer(qMax(m_AutoSaveInterval, 1000));
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
