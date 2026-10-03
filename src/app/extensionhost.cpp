#include "switch.hpp"
#include "const.hpp"

#include "extensionhost.hpp"
#include "cdpshims.hpp"

#include <QBuffer>
#include <QDateTime>
#include <QElapsedTimer>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QHash>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <functional>
#ifdef WEBENGINEVIEW
#include <QAuthenticator>
#include <QWebEngineCertificateError>
#include <QWebEngineClientCertificateSelection>
#include <QWebEngineDesktopMediaRequest>
#include <QWebEngineFileSystemAccessRequest>
#include <QWebEngineFullScreenRequest>
#include <QWebEnginePage>
#include <QWebEnginePermission>
#include <QWebEngineProfile>
#include <QWebEngineRegisterProtocolHandlerRequest>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineWebAuthUxRequest>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>
#include <QWebEngineDownloadRequest>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>
#include <QCloseEvent>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QQuickWebEngineProfile>
#include <QQuickWebEngineDownloadRequest>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#endif
#include <QDir>

#include "application.hpp"
#include "extensioncontroller.hpp"
#include "extensionhostwire.hpp"
#include "gadgets.hpp"
#include "mainwindow.hpp"
#include "networkcontroller.hpp"
#include "page.hpp"
#include "treebank.hpp"
#include "view.hpp"
#include "sidepanels.hpp"
#include "dialog.hpp"

namespace {

    class ViewStamp;
    struct Stamped {
        QPointer<QObject> owner;
        View *view = nullptr;
        ViewStamp *stamp = nullptr;
    };
    QHash<quint64, Stamped> &StampedViews(){
        static QHash<quint64, Stamped> views;
        return views;
    }
    quint64 NextViewNumber(){
        static quint64 last = 0;
        return ++last;
    }
    QList<ExtensionHost*> &Hosts(){
        static QList<ExtensionHost*> hosts;
        return hosts;
    }

#ifdef WEBENGINEVIEW
    void RefuseAsking(QWebEnginePage *page){
        QObject::connect(page, &QWebEnginePage::permissionRequested, page,
                [](QWebEnginePermission permission){ permission.deny(); });
        QObject::connect(page, &QWebEnginePage::fullScreenRequested, page,
                [](QWebEngineFullScreenRequest request){ request.reject(); });
        QObject::connect(page, &QWebEnginePage::registerProtocolHandlerRequested, page,
                [](QWebEngineRegisterProtocolHandlerRequest request){ request.reject(); });
        QObject::connect(page, &QWebEnginePage::fileSystemAccessRequested, page,
                [](QWebEngineFileSystemAccessRequest request){ request.reject(); });
        QObject::connect(page, &QWebEnginePage::desktopMediaRequested, page,
                [](const QWebEngineDesktopMediaRequest &request){ request.cancel(); });
        QObject::connect(page, &QWebEnginePage::selectClientCertificate, page,
                [](QWebEngineClientCertificateSelection selection){ selection.selectNone(); });
        QObject::connect(page, &QWebEnginePage::webAuthUxRequested, page,
                [](QWebEngineWebAuthUxRequest *request){ if(request) request->cancel(); });
        QObject::connect(page, &QWebEnginePage::certificateError, page,
                [](const QWebEngineCertificateError &error){ QWebEngineCertificateError refused(error); refused.rejectCertificate(); });
        QObject::connect(page, &QWebEnginePage::authenticationRequired, page,
                [](const QUrl&, QAuthenticator *authenticator){ if(authenticator) *authenticator = QAuthenticator(); });
        QObject::connect(page, &QWebEnginePage::proxyAuthenticationRequired, page,
                [](const QUrl&, QAuthenticator *authenticator, const QString&){ if(authenticator) *authenticator = QAuthenticator(); });
    }

    class OffscreenPage : public QWebEnginePage {
    public:
        OffscreenPage(QWebEngineProfile *profile, const QUrl &url, QObject *parent)
            : QWebEnginePage(profile, parent), m_Url(url)
        {
            setAudioMuted(true);
            RefuseAsking(this);
        }
    protected:
        QWebEnginePage *createWindow(WebWindowType) Q_DECL_OVERRIDE { return nullptr; }
        void javaScriptAlert(const QUrl&, const QString&) Q_DECL_OVERRIDE {}
        bool javaScriptConfirm(const QUrl&, const QString&) Q_DECL_OVERRIDE { return false; }
        bool javaScriptPrompt(const QUrl&, const QString&, const QString&, QString*) Q_DECL_OVERRIDE { return false; }
        QStringList chooseFiles(FileSelectionMode, const QStringList&, const QStringList&) Q_DECL_OVERRIDE { return QStringList(); }
        bool acceptNavigationRequest(const QUrl &url, NavigationType, bool isMainFrame) Q_DECL_OVERRIDE {
            if(isMainFrame) return url.matches(m_Url, QUrl::RemoveFragment);
            const QString scheme = url.scheme();
            if(scheme == QStringLiteral("about") || scheme == QStringLiteral("data") || scheme == QStringLiteral("blob")) return true;
            return scheme == QStringLiteral("chrome-extension") && m_Url.scheme() == scheme && url.host() == m_Url.host();
        }
    private:
        QUrl m_Url;
    };

    class AuthPage : public QWebEnginePage {
    public:
        AuthPage(QWebEngineProfile *profile, const QString &id, bool seen,
                 std::function<void(const QUrl&)> sentBack, QObject *parent)
            : QWebEnginePage(profile, parent), m_Id(id), m_Seen(seen), m_SentBack(sentBack)
        {
            if(!m_Seen) setAudioMuted(true);
            RefuseAsking(this);
        }
        void Dispose(QObject *window){
            m_Window = window;
            if(m_Dialogs){ m_Disposed = true; return; }
            Disposed();
        }
    protected:
        QWebEnginePage *createWindow(WebWindowType) Q_DECL_OVERRIDE { return nullptr; }
        void javaScriptAlert(const QUrl &url, const QString &text) Q_DECL_OVERRIDE {
            if(!Shown()) return;
            Asking asking(this);
            QWebEnginePage::javaScriptAlert(url, text);
        }
        bool javaScriptConfirm(const QUrl &url, const QString &text) Q_DECL_OVERRIDE {
            if(!Shown()) return false;
            Asking asking(this);
            return QWebEnginePage::javaScriptConfirm(url, text);
        }
        bool javaScriptPrompt(const QUrl &url, const QString &text, const QString &value, QString *result) Q_DECL_OVERRIDE {
            if(!Shown()) return false;
            Asking asking(this);
            return QWebEnginePage::javaScriptPrompt(url, text, value, result);
        }
        QStringList chooseFiles(FileSelectionMode mode, const QStringList &old, const QStringList &types) Q_DECL_OVERRIDE {
            if(!Shown()) return QStringList();
            Asking asking(this);
            return QWebEnginePage::chooseFiles(mode, old, types);
        }
        bool acceptNavigationRequest(const QUrl &url, NavigationType, bool isMainFrame) Q_DECL_OVERRIDE {
            if(!isMainFrame || !ExtensionHostWire::IsAuthRedirect(m_Id, url)) return true;
            if(m_SentBack){
                const std::function<void(const QUrl&)> sentBack = m_SentBack;
                m_SentBack = nullptr;
                sentBack(url);
            }
            return false;
        }
    private:
        bool Shown() const { return m_Seen && QWebEngineView::forPage(this); }
        void Disposed(){
            if(m_Window) m_Window->deleteLater();
            deleteLater();
        }
        struct Asking {
            explicit Asking(AuthPage *page) : m_Page(page) { m_Page->m_Dialogs++; }
            ~Asking(){
                if(--m_Page->m_Dialogs || !m_Page->m_Disposed) return;
                m_Page->m_Disposed = false;
                m_Page->Disposed();
            }
            AuthPage *m_Page;
        };
        QString m_Id;
        bool m_Seen;
        std::function<void(const QUrl&)> m_SentBack;
        int m_Dialogs = 0;
        bool m_Disposed = false;
        QPointer<QObject> m_Window;
    };

    class AuthWindow : public QWidget {
    public:
        AuthWindow(QWebEnginePage *page, const QString &name, std::function<void()> closed)
            : QWidget(nullptr), m_Closed(closed)
        {
            m_Address = new QLineEdit(this);
            m_Address->setReadOnly(true);
            QWebEngineView *view = new QWebEngineView(this);
            view->setPage(page);
            QVBoxLayout *layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            layout->addWidget(m_Address);
            layout->addWidget(view, 1);
            const auto titled = [this, name, page](){
                const QString title = page->title();
                setWindowTitle(title.isEmpty() ? name : name + QStringLiteral(" ") + QChar(0x2014) + QStringLiteral(" ") + title);
            };
            connect(page, &QWebEnginePage::titleChanged, this, titled);
            const auto addressed = [this](const QUrl &url){
                m_Address->setText(url.toDisplayString());
                m_Address->setCursorPosition(0);
            };
            connect(page, &QWebEnginePage::urlChanged, this, addressed);
            addressed(page->url());
            titled();
            resize(500, 650);
        }
    protected:
        void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE {
            QWidget::closeEvent(event);
            if(m_Closed){
                const std::function<void()> closed = m_Closed;
                m_Closed = nullptr;
                closed();
            }
        }
    private:
        QLineEdit *m_Address;
        std::function<void()> m_Closed;
    };
#endif

    const QByteArray &ProcessSecret(){
        static const QByteArray secret = [](){
            quint32 words[8];
            QRandomGenerator::system()->fillRange(words);
            return QByteArray(reinterpret_cast<const char*>(words), sizeof words);
        }();
        return secret;
    }

#ifdef WEBENGINEVIEW
    class ViewStamp : public QWebEngineUrlRequestInterceptor {
    public:
        ViewStamp(QWebEnginePage *page, View *view, std::function<void(quint64)> gone)
            : QWebEngineUrlRequestInterceptor(page)
            , m_Gone(gone)
        {
            m_Number = NextViewNumber();
            Stamped entry;
            entry.owner = page;
            entry.view = view;
            entry.stamp = this;
            StampedViews().insert(m_Number, entry);
        }
        ~ViewStamp(){
            StampedViews().remove(m_Number);
            m_Gone(m_Number);
        }
        void interceptRequest(QWebEngineUrlRequestInfo &info) Q_DECL_OVERRIDE {
            if(info.requestUrl().scheme() != QLatin1String(ExtensionHostWire::SCHEME)) return;
            Q_ASSERT(QThread::currentThread() == qApp->thread());
            info.setHttpHeader(QByteArray(ExtensionHostWire::VIEW_HEADER),
                               ExtensionHostWire::Stamp(ProcessSecret(), m_Number));
        }
    private:
        quint64 m_Number;
        std::function<void(quint64)> m_Gone;
    };

    class QtAsk : public ExtensionHost::Ask {
    public:
        explicit QtAsk(QWebEngineUrlRequestJob *job) : m_Job(job) {}
        QByteArray Method() const Q_DECL_OVERRIDE { return m_Job ? m_Job->requestMethod() : QByteArray(); }
        QUrl Url() const Q_DECL_OVERRIDE { return m_Job ? m_Job->requestUrl() : QUrl(); }
        QUrl Initiator() const Q_DECL_OVERRIDE { return m_Job ? m_Job->initiator() : QUrl(); }
        QMap<QByteArray, QByteArray> Headers() const Q_DECL_OVERRIDE {
            return m_Job ? m_Job->requestHeaders() : QMap<QByteArray, QByteArray>();
        }
        bool Body(qint64 limit, QByteArray *body) Q_DECL_OVERRIDE {
            if(!m_Job) return false;
            QIODevice *device = m_Job->requestBody();
            if(!device) return false;
            if(!device->isOpen()) device->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
            QByteArray read;
            read.resize(2 * limit);
            const qint64 size = device->read(read.data(), limit);
            if(size < 0 || size >= limit) return false;
            read.truncate(size);
            *body = read;
            return true;
        }
        bool Alive() const Q_DECL_OVERRIDE { return !m_Job.isNull(); }
        void Reply(const QJsonObject &answer) Q_DECL_OVERRIDE {
            if(!m_Job) return;
            QBuffer *buffer = new QBuffer(m_Job);
            buffer->setData(QJsonDocument(answer).toJson(QJsonDocument::Compact));
            buffer->open(QIODevice::ReadOnly);
            m_Job->reply(QByteArrayLiteral("application/json"), buffer);
        }
        void ReplyNothing() Q_DECL_OVERRIDE {
            if(!m_Job) return;
            QBuffer *buffer = new QBuffer(m_Job);
            buffer->open(QIODevice::ReadOnly);
            m_Job->reply(QByteArrayLiteral("text/plain"), buffer);
        }
        void Fail() Q_DECL_OVERRIDE {
            if(m_Job) m_Job->fail(QWebEngineUrlRequestJob::RequestDenied);
        }
    private:
        QPointer<QWebEngineUrlRequestJob> m_Job;
    };

    class QtSchemeHandler : public QWebEngineUrlSchemeHandler {
    public:
        QtSchemeHandler(QObject *profile, ExtensionHost *host)
            : QWebEngineUrlSchemeHandler(profile), m_Host(host) {}
        void requestStarted(QWebEngineUrlRequestJob *job) Q_DECL_OVERRIDE {
            if(m_Host) m_Host->Handle(std::make_shared<QtAsk>(job), 0);
            else job->fail(QWebEngineUrlRequestJob::RequestDenied);
        }
    private:
        QPointer<ExtensionHost> m_Host;
    };
#endif

    bool DirIsOfProfile(ViewNode *parent, const QString &profileSpace){
        return ExtensionHostWire::NodeIsOfProfile(TreeBank::SettingsOf(parent),
                                                  TreeBank::NetworkSpaceOf(parent), profileSpace);
    }

    qint64 Milliseconds(const QDateTime &date){
        return date.isValid() ? date.toMSecsSinceEpoch() : 0;
    }

    bool VisibleTo(ViewNode *vn, ExtensionController *controller, const std::function<bool()> &dirIsOfProfile){
        using ExtensionHostWire::TabOwner;
        View *view = vn->GetView();
        ExtensionController *of = view ? view->Extensions() : nullptr;
        return ExtensionHostWire::TabIsTold(!of ? TabOwner::NotYet : of == controller ? TabOwner::Asking : TabOwner::Other,
                                            dirIsOfProfile);
    }

    void Collect(ViewNode *parent, ExtensionController *controller, const QString &profileSpace,
                 ViewNode *current, bool dated, QList<ExtensionHostWire::Tab> *tabs){
        bool asked = false, ofProfile = false;
        foreach(Node *child, parent->GetChildren()){
            ViewNode *vn = child->ToViewNode();
            if(!vn) continue;
            if(vn->IsDirectory()){
                Collect(vn, controller, profileSpace, current, dated, tabs);
                continue;
            }
            View *view = vn->GetView();
            if(!VisibleTo(vn, controller, [&](){
                if(!asked){ asked = true; ofProfile = DirIsOfProfile(parent, profileSpace); }
                return ofProfile;
            })) continue;
            ExtensionHostWire::Tab tab;
            tab.id = static_cast<qint64>(vn->GetSerial());
            tab.url = view ? view->url() : vn->GetUrl();
            tab.title = view ? view->GetTitle() : vn->GetTitle();
            if(tab.title.isEmpty()) tab.title = vn->GetTitle();
            tab.active = vn == current;
            tab.discarded = !view;
            tab.audible = view && view->RecentlyAudible();
            tab.muted = view && view->IsAudioMuted();
            tab.loading = view && view->IsLoading();
            tab.loads = view ? view->LoadSerial() : 0;
            tab.zoom = vn->GetZoom();
            if(dated){
                tab.added = Milliseconds(vn->GetCreateDate());
                tab.visited = Milliseconds(vn->GetLastAccessDate());
            }
            tabs->append(tab);
        }
    }

    ExtensionHostWire::Bookmark Gather(ViewNode *dir, ExtensionController *controller, const QString &profileSpace){
        ExtensionHostWire::Bookmark folder;
        folder.id = static_cast<qint64>(dir->GetSerial());
        folder.folder = true;
        folder.told = DirIsOfProfile(dir, profileSpace);
        folder.title = dir->GetTitle();
        folder.added = Milliseconds(dir->GetCreateDate());
        foreach(Node *child, dir->GetChildren()){
            ViewNode *vn = child->ToViewNode();
            if(!vn) continue;
            if(vn->IsDirectory()){
                folder.children.append(Gather(vn, controller, profileSpace));
                continue;
            }
            View *view = vn->GetView();
            if(!VisibleTo(vn, controller, [&](){ return folder.told; })) continue;
            ExtensionHostWire::Bookmark leaf;
            leaf.id = static_cast<qint64>(vn->GetSerial());
            leaf.told = true;
            leaf.url = view ? view->url() : vn->GetUrl();
            leaf.title = view ? view->GetTitle() : vn->GetTitle();
            if(leaf.title.isEmpty()) leaf.title = vn->GetTitle();
            leaf.added = Milliseconds(vn->GetCreateDate());
            folder.children.append(leaf);
        }
        return folder;
    }

    TreeBank *CurrentBank(){
        MainWindow *window = Application::GetCurrentWindow();
        return window ? window->GetTreeBank() : nullptr;
    }

    TreeBank *LiveBankOf(View *view){
        TreeBank *said = view ? view->GetTreeBank() : nullptr;
        return TreeBank::IsLive(said) ? said : CurrentBank();
    }

    bool OverviewIsUp(){
        foreach(MainWindow *window, Application::GetMainWindows()){
            TreeBank *bank = window ? window->GetTreeBank() : nullptr;
            if(bank && bank->GetGadgets() && bank->GetGadgets()->IsActive()) return true;
        }
        return false;
    }

    const int LINE_LIMIT = 64;
    const int COME_BACK_AFTER = 50;

    const int LOOK_SOON = 50;
    const int LOOK_AGAIN = 2000;

    typedef ExtensionHost::HeldAsk HeldAsk;
    bool StillThere(const HeldAsk &ask){ return ask && ask->Alive(); }

    qint64 Now(){
        static QElapsedTimer clock;
        if(!clock.isValid()) clock.start();
        return clock.elapsed();
    }

    bool AnyFrameActive(){
        foreach(MainWindow *window, Application::GetMainWindows())
            if(window && window->IsFrameActive()) return true;
        return false;
    }

}

struct ExtensionHost::Pending {
    HeldAsk ask;
    QPointer<ExtensionHost> host;
    ExtensionHostWire::Sight sight;
    ExtensionHostWire::Act act;
    bool ownCleanup = false;
};

namespace {
    ExtensionHostWire::OneAtATime<ExtensionHost::Pending> &Line(){
        static ExtensionHostWire::OneAtATime<ExtensionHost::Pending> line(LINE_LIMIT);
        return line;
    }
}

ExtensionHost::ExtensionHost(QObject *owner, ExtensionController *controller, const QString &profileSpace)
    : QObject(owner)
    , m_Owner(owner)
    , m_ProfileSpace(profileSpace)
    , m_Controller(controller)
{
    Hosts().append(this);
    connect(controller, &ExtensionController::WorkerEvent, this, &ExtensionHost::Fire);
    connect(controller, &ExtensionController::Changed, this, &ExtensionHost::CloseOffscreenOfOthers);
    connect(controller, &ExtensionController::Changed, this, &ExtensionHost::KeepMainScripts);
    connect(controller, &ExtensionController::Ready, this, &ExtensionHost::KeepMainScripts);
    connect(controller, &ExtensionController::Unlisted, this, [this](const QString &id){
        m_UserChecked.remove(id);
        if(m_UserStore.entries.contains(id)){
            m_UserStore.Drop(id);
            WriteUserStoreSoon();
        }
        if(!m_MainStore.entries.contains(id)) return;
        m_MainStore.Drop(id);
        WriteMainStore();
    });
    connect(controller, &ExtensionController::Changed, this, &ExtensionHost::KeepUserScripts);
    connect(controller, &ExtensionController::Ready, this, &ExtensionHost::KeepUserScripts);
    connect(controller, &QObject::destroyed, this, [this](){ CloseNotices(); CloseOffscreen(false); });
    static bool listening = false;
    if(!listening){
        listening = true;
        TreeBank::WhenSomethingChanges([](){ ExtensionHost::Changed(); });
    }
}

namespace {
    void WriteKept(const QString &path, const ExtensionMainScripts::Store &store, QByteArray *written);
}

ExtensionHost::~ExtensionHost(){
    Hosts().removeAll(this);
    CloseNotices();
    CloseOffscreen(true);
    Line().Prune([this](const Pending &pending){ return pending.host != this; });
    if(m_UserStoreSoon) WriteKept(m_UserStoreFile, m_UserStore, &m_UserStoreWritten);
    foreach(const auto &one, m_UserScriptMessages.Drop()) if(StillThere(one.waiter)) one.waiter->Reply(one.answer);
}

bool ExtensionHost::HasOffscreenOf(QObject *profile, const QUrl &origin){
    const QString id = ExtensionHostWire::ExtensionIdOf(origin);
    if(id.isEmpty()) return false;
    foreach(ExtensionHost *host, Hosts())
        if(host->m_Owner == profile && host->m_Offscreen.contains(id) && !host->m_Offscreen.value(id).page.isNull()) return true;
    return false;
}

void ExtensionHost::CloseOffscreenOf(QObject *profile){
    foreach(ExtensionHost *host, QList<ExtensionHost*>(Hosts()))
        if(Hosts().contains(host) && host->m_Owner == profile) host->CloseOffscreen(true);
}

#ifdef WEBENGINEVIEW
void ExtensionHost::StampRequestsOf(QWebEnginePage *page, View *view){
    if(!page || !view) return;
    bool hosted = false;
    foreach(ExtensionHost *host, Hosts()) hosted = hosted || host->m_Owner == page->profile();
    if(!hosted) return;

    ViewStamp *stamp = nullptr;
    foreach(const Stamped &entry, StampedViews())
        if(entry.owner == page) stamp = entry.stamp;
    if(!stamp)
        stamp = new ViewStamp(page, view, [](quint64 number){
            ExtensionHost::ForgetView(number);
        });
    page->setUrlRequestInterceptor(stamp);
}
#endif

quint64 ExtensionHost::NumberView(QObject *owner, View *view){
    if(!owner || !view) return 0;
    for(auto it = StampedViews().constBegin(); it != StampedViews().constEnd(); ++it)
        if(it.value().owner == owner && it.value().view == view) return it.key();
    const quint64 number = NextViewNumber();
    Stamped entry;
    entry.owner = owner;
    entry.view = view;
    StampedViews().insert(number, entry);
    return number;
}

void ExtensionHost::ForgetView(quint64 number){
    if(!number) return;
    StampedViews().remove(number);
    foreach(ExtensionHost *host, Hosts()) host->m_Documents.Forget(number);
}

View *ExtensionHost::ViewOf(quint64 number) const {
    if(!number || !m_Controller) return nullptr;
    const Stamped entry = StampedViews().value(number);
    if(!entry.owner || !entry.view) return nullptr;
    return entry.view->Extensions() == m_Controller ? entry.view : nullptr;
}

void ExtensionHost::Bind(const HeldAsk &ask, quint64 view){
    const QMap<QByteArray, QByteArray> headers = ask->Headers();
    const QByteArray nonce = ExtensionHostWire::HeaderOf(headers, ExtensionHostWire::NONCE_HEADER);
    if(!ExtensionHostWire::IsBindRequest(ask->Method(), nonce)){
        ask->Fail();
        return;
    }
    const quint64 number = view ? view
        : ExtensionHostWire::ViewOfStamp(ProcessSecret(), ExtensionHostWire::HeaderOf(headers, ExtensionHostWire::VIEW_HEADER));
    if(ViewOf(number)) m_Documents.Bind(number, nonce);
    ask->ReplyNothing();
}

#ifdef WEBENGINEVIEW
namespace {
    QWebEngineScript EngineScriptOf(const ExtensionMainScripts::Script &s){
        QWebEngineScript script;
        script.setName(s.name);
        script.setSourceCode(s.source);
        script.setWorldId(s.world);
        script.setRunsOnSubFrames(s.subFrames);
        script.setInjectionPoint(s.runAt == ExtensionMainScripts::DocumentStart ? QWebEngineScript::DocumentCreation
                                 : s.runAt == ExtensionMainScripts::DocumentEnd ? QWebEngineScript::DocumentReady
                                 : QWebEngineScript::Deferred);
        return script;
    }
}

void ExtensionHost::RegisterScheme(){
    QWebEngineUrlScheme scheme{ExtensionHostWire::SCHEME};
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
    scheme.setDefaultPort(QWebEngineUrlScheme::PortUnspecified);
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme |
                    QWebEngineUrlScheme::CorsEnabled |
                    QWebEngineUrlScheme::FetchApiAllowed |
                    QWebEngineUrlScheme::ServiceWorkersAllowed);
    QWebEngineUrlScheme::registerScheme(scheme);
}

void ExtensionHost::Install(QWebEngineProfile *profile, ExtensionController *controller){
    if(!profile || !controller || profile->isOffTheRecord()) return;
    ExtensionHost *host = Install(static_cast<QObject*>(profile), controller,
                                  ExtensionHostWire::SpaceOfProfileKey(NetworkController::ProfileKey(profile)));
    if(!host) return;
    profile->installUrlSchemeHandler(ExtensionHostWire::SCHEME, new QtSchemeHandler(profile, host));
    host->m_Qt = true;
    QPointer<QWebEngineProfile> owner = profile;
    host->m_ApplyMainScripts = [owner](const ExtensionMainScripts::Table::Change &change){
        if(!owner) return;
        foreach(const ExtensionMainScripts::Script &s, change.removed) owner->scripts()->remove(EngineScriptOf(s));
        QList<QWebEngineScript> in;
        foreach(const ExtensionMainScripts::Script &s, change.inserted) in << EngineScriptOf(s);
        if(!in.isEmpty()) owner->scripts()->insert(in);
    };
    host->ReadMainStore(Application::StateDirectory() + ExtensionMainScripts::StoreFileName(NetworkController::ProfileKey(profile)));
    host->ReadUserStore(Application::StateDirectory() + ExtensionUserScripts::StoreFileName(NetworkController::ProfileKey(profile)));
    connect(profile, &QWebEngineProfile::downloadRequested, host, &ExtensionHost::DownloadRequested);
}

void ExtensionHost::Install(QQuickWebEngineProfile *profile, ExtensionController *controller){
    if(!profile || !controller || profile->isOffTheRecord()) return;
    ExtensionHost *host = Install(static_cast<QObject*>(profile), controller,
                                  ExtensionHostWire::SpaceOfProfileKey(NetworkController::ProfileKey(profile)));
    if(!host) return;
    profile->installUrlSchemeHandler(ExtensionHostWire::SCHEME, new QtSchemeHandler(profile, host));
    host->m_Qt = true;
    QPointer<QObject> scripts = profile->property("userScripts").value<QObject*>();
    if(scripts) host->m_ApplyMainScripts = [scripts](const ExtensionMainScripts::Table::Change &change){
        if(!scripts) return;
        bool done = true;
        foreach(const ExtensionMainScripts::Script &s, change.removed)
            done = QMetaObject::invokeMethod(scripts, "remove", Q_ARG(QWebEngineScript, EngineScriptOf(s))) && done;
        foreach(const ExtensionMainScripts::Script &s, change.inserted)
            done = QMetaObject::invokeMethod(scripts, "insert", Q_ARG(QWebEngineScript, EngineScriptOf(s))) && done;
        if(!done) qWarning("extension host: the profile's user scripts could not be changed");
    };
    if(scripts) host->ReadMainStore(Application::StateDirectory() + ExtensionMainScripts::StoreFileName(NetworkController::ProfileKey(profile)));
    if(scripts) host->ReadUserStore(Application::StateDirectory() + ExtensionUserScripts::StoreFileName(NetworkController::ProfileKey(profile)));
    connect(profile, &QQuickWebEngineProfile::downloadRequested, host,
            [host](QQuickWebEngineDownloadRequest *request){ host->DownloadRequested(request); });
}
#endif

ExtensionHost *ExtensionHost::Install(QObject *owner, ExtensionController *controller, const QString &profileSpace){
    if(!owner || !controller) return nullptr;
    foreach(ExtensionHost *host, Hosts())
        if(host->m_Owner == owner) return host;
    return new ExtensionHost(owner, controller, profileSpace);
}

void ExtensionHost::Invoked(const ExtensionController *controller, const QString &id, qint64 tab){
    ExtensionHost *host = controller ? Of(controller) : nullptr;
    if(host) host->m_InvokedAt.insert(id, QDateTime::currentMSecsSinceEpoch());
    if(!host || !host->m_Controller || !host->m_Controller->HasPermission(id, QStringLiteral("activeTab"))) return;
    ViewNode *vn = host->NodeOf(tab);
    View *view = vn ? vn->GetView() : nullptr;
    if(view) host->m_ActiveTabs.Grant(id, tab, view->LoadSerial());
}

void ExtensionHost::SidePanelEvent(const ExtensionController *controller, const QString &id, bool opened, const QString &path, qint64 tab){
    ExtensionHost *host = controller ? Of(controller) : nullptr;
    if(!host) return;
    QJsonObject info;
    info[QStringLiteral("path")] = path;
    if(tab > 0) info[QStringLiteral("tabId")] = double(tab);
    info[QStringLiteral("windowId")] = 1;
    host->Fire(id, opened ? QStringLiteral("sidePanel.onOpened") : QStringLiteral("sidePanel.onClosed"), QJsonArray() << info, 0);
}

QJsonObject ExtensionHost::NotificationCall(const ExtensionHostWire::Call &asked, const QString &id){
    using namespace ExtensionHostWire;
    const QString api = asked.api;
    if(!NoticesMayRun(id)) return Refused(NotAvailable(api));

    if(api == QStringLiteral("notifications.getAll")){
        QJsonObject all;
        foreach(const QString &notice, m_Notices.Ids(id)) all[notice] = true;
        return Accepted(all);
    }
    if(api == QStringLiteral("notifications.getPermissionLevel")) return Accepted(QStringLiteral("granted"));
    const QJsonValue first = asked.args.isEmpty() ? QJsonValue() : asked.args.at(0);
    QString notice;
    QJsonObject given;
    if(api == QStringLiteral("notifications.create") && first.isObject()){
        given = first.toObject();
    } else {
        if(!first.isString()) return Refused(QStringLiteral("Invalid value for argument 1. Expected 'string'."));
        notice = first.toString();
        given = asked.args.size() > 1 ? asked.args.at(1).toObject() : QJsonObject();
    }

    if(api == QStringLiteral("notifications.clear")){
        const quint64 serial = m_Notices.SerialOf(id, notice);
        const bool cleared = serial && m_Notices.Take(id, notice, serial);
        if(cleared){
            DismissNotice(serial);
            Fire(id, QStringLiteral("notifications.onClosed"), QJsonArray() << notice << false, 0);
        }
        return Accepted(cleared);
    }

    const bool creating = api == QStringLiteral("notifications.create");
    if(!creating && api != QStringLiteral("notifications.update")) return Refused(NotAvailable(api));
    if(!creating && !m_Notices.SerialOf(id, notice)) return Accepted(false);
    const ExtensionRow row = m_Controller->RowOf(id);
    QString error;
    const QJsonObject options = ExtensionUi::NoticeOf(given, m_Notices.OptionsOf(id, notice), creating, &error);
    if(!error.isEmpty()) return Refused(error);
    if(notice.isEmpty()) notice = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const ExtensionUi::Notices::Added added = m_Notices.Add(id, notice, options, row.manifest.version);
    if(added.replaced) DismissNotice(added.replaced);
    foreach(const ExtensionUi::Notices::Gone &gone, added.pushedOut){
        DismissNotice(gone.serial);
        Fire(id, QStringLiteral("notifications.onClosed"), QJsonArray() << gone.id << false, 0);
    }
    ShowNotice(id, notice, added.serial);
    return Accepted(creating ? QJsonValue(notice) : QJsonValue(true));
}

void ExtensionHost::ShowNotice(const QString &id, const QString &notice, quint64 serial){
    const QJsonObject options = m_Notices.OptionsOf(id, notice);
    const QString name = m_Controller ? m_Controller->RowOf(id).manifest.name : QString();
    ModelessDialog *dialog = new ModelessDialog();
    dialog->SetTitle(ExtensionUi::NoticeTitle(name.isEmpty() ? id : name, options));
    dialog->SetCaption(ExtensionUi::NoticeText(options));
    dialog->SetButtons(Dialog::Open | Dialog::Close);
    dialog->SetDefaultValue(false);
    QPointer<ExtensionHost> self(this);
    dialog->SetCallBack([self, dialog, id, notice, serial](bool ok){
        if(!self) return;
        const Dialog::Button button = dialog->ClickedButton();
        self->NoticeEnded(id, notice, serial, ok && button == Dialog::Open, button != Dialog::NoButton);
    });
    connect(dialog, &QObject::destroyed, this, [this, id, notice, serial](){
        QTimer::singleShot(0, this, [this, id, notice, serial](){ NoticeEnded(id, notice, serial, false, false); });
    });
    m_NoticeDialogs.insert(serial, dialog);
    QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute(); });
}

void ExtensionHost::NoticeEnded(const QString &id, const QString &notice, quint64 serial, bool clicked, bool byUser){
    if(!m_Notices.Take(id, notice, serial)) return;
    m_NoticeDialogs.remove(serial);
    if(clicked) Fire(id, QStringLiteral("notifications.onClicked"), QJsonArray() << notice, 0);
    Fire(id, QStringLiteral("notifications.onClosed"), QJsonArray() << notice << byUser, 0);
}

void ExtensionHost::DismissNotice(quint64 serial){
    const QPointer<ModelessDialog> dialog = m_NoticeDialogs.take(serial);
    if(dialog) dialog->Discard();
}

bool ExtensionHost::Runs(const QString &id) const {
    if(!m_Controller) return false;
    const ExtensionRow row = m_Controller->RowOf(id);
    return row.manifest.id == id && row.registered && row.wanted;
}

bool ExtensionHost::NoticesMayRun(const QString &id) const {
    if(!m_Qt || !Runs(id)) return false;
    return m_Controller->HasPermission(id, QStringLiteral("notifications"))
        && !m_Controller->RunFolderOf(id).isEmpty();
}

void ExtensionHost::CloseNoticesOfOthers(){
    if(!m_Controller){ CloseNotices(); return; }
    foreach(const QString &id, m_Notices.Extensions()){
        if(NoticesMayRun(id) && m_Controller->RowOf(id).manifest.version == m_Notices.VersionOf(id)) continue;
        foreach(const ExtensionUi::Notices::Gone &gone, m_Notices.Drop(id)) DismissNotice(gone.serial);
    }
}

void ExtensionHost::CloseNotices(){
    foreach(const QString &id, m_Notices.Extensions())
        foreach(const ExtensionUi::Notices::Gone &gone, m_Notices.Drop(id)) DismissNotice(gone.serial);
}

QJsonObject ExtensionHost::SidePanelCall(const ExtensionHostWire::Call &asked, const QString &id){
    using namespace ExtensionHostWire;
    const QString api = asked.api;
    if(api != QStringLiteral("sidePanel.open") && api != QStringLiteral("sidePanel.close")){
        const QJsonObject reply = m_Controller->SidePanelCall(id, api, asked.args);
        if(api == QStringLiteral("sidePanel.setOptions") && reply.value(QStringLiteral("ok")).toBool()) SidePanels::UpdateAll();
        return reply;
    }
    const QJsonObject where = asked.args.size() == 1 ? asked.args.at(0).toObject() : QJsonObject();
    const QJsonValue window = where.value(QStringLiteral("windowId")), tab = where.value(QStringLiteral("tabId"));
    if(asked.args.size() != 1 || !asked.args.at(0).isObject() || (!window.isUndefined() && !window.isDouble())
       || (!tab.isUndefined() && !tab.isDouble()) || (window.isUndefined() && tab.isUndefined()))
        return Refused(QStringLiteral("Error in invocation of %1(sidePanel.%2Options options, optional function callback): No matching signature.")
                       .arg(api, api == QStringLiteral("sidePanel.open") ? QStringLiteral("Open") : QStringLiteral("Close")));
    if(api == QStringLiteral("sidePanel.close")){
        const QString error = SidePanels::Close(m_Controller, id, tab.isDouble() ? qint64(tab.toDouble()) : 0);
        return error.isEmpty() ? Done() : Refused(error);
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 closed = SidePanels::ClosedByUserAt(m_Controller, id);
    if(closed && now - closed < 10000 && now - m_InvokedAt.value(id) >= 10000)
        return Refused(QStringLiteral("sidePanel.open() may only be called in response to a user gesture."));
    const QString error = SidePanels::Open(m_Controller, id, tab.isDouble() ? qint64(tab.toDouble()) : 0);
    return error.isEmpty() ? Done() : Refused(error);
}

QJsonObject ExtensionHost::CaptureCall(const ExtensionHostWire::Call &asked, const QString &id){
    using namespace ExtensionHostWire;
    const Capture capture = ParseCapture(asked);
    if(!capture.error.isEmpty()) return Refused(capture.error);
    ViewNode *vn = CurrentNode();
    View *view = vn ? vn->GetView() : nullptr;
    if(!view) return Refused(QStringLiteral("No active web contents to capture"));
    if(OverviewIsUp()) return Refused(QStringLiteral("Failed to capture tab: view is invisible"));
    const qint64 tab = static_cast<qint64>(vn->GetSerial());
    const bool granted = m_Controller->HasPermission(id, QStringLiteral("activeTab"))
        && m_ActiveTabs.Granted(id, tab, view->LoadSerial());
    const QString refused = CaptureRefusal(view->CommittedUrl(), id, m_Controller->RowOf(id).manifest.hostPermissions, granted);
    if(!refused.isEmpty()) return Refused(refused);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if(!CaptureAllowedAt(m_LastCapture.value(id), now)) return Refused(QString::fromLatin1(CAPTURE_QUOTA));
    m_LastCapture.insert(id, now);
    const QImage image = view->CaptureVisible();
    if(image.isNull()) return Refused(QStringLiteral("Failed to capture tab: view is invisible"));
    QByteArray bytes;
    QBuffer buffer(&bytes);
    const bool png = capture.format == "png";
    if(!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, png ? "PNG" : "JPG", png ? -1 : capture.quality))
        return Refused(QStringLiteral("Failed to capture tab: unknown error"));
    return Accepted(QString::fromLatin1(QByteArray("data:image/") + capture.format + ";base64," + bytes.toBase64()));
}

void ExtensionHost::MenuPicked(const ExtensionController *controller, qint64 tab, const QString &pageUrl){
    if(ExtensionHost *host = Of(controller)) host->m_MenuPicks.Push(tab, pageUrl, Now());
}

ExtensionHost *ExtensionHost::Of(const ExtensionController *controller){
    if(!controller) return nullptr;
    foreach(ExtensionHost *host, Hosts())
        if(host->m_Controller == controller) return host;
    return nullptr;
}

QByteArray ExtensionHost::Secret(bool make){
    static QByteArray secret;
    if(secret.size() == 32) return secret;

    const QString path = Application::StateDirectory() + QStringLiteral("extension-host.key");
    auto read = [&](){
        QFile file(path);
        secret = ExtensionHostWire::SecretOf(file.open(QIODevice::ReadOnly) ? file.read(64) : QByteArray());
    };
    read();
    if(secret.size() == 32 || !make) return secret;

    QByteArray fresh;
    for(int i = 0; i < 8; i++){
        const quint32 word = QRandomGenerator::system()->generate();
        fresh.append(reinterpret_cast<const char*>(&word), sizeof word);
    }
    if(QFile::exists(path)) QFile::remove(path);
    QFile file(path);
    if(file.open(QIODevice::WriteOnly | QIODevice::NewOnly)){
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        const bool written = file.write(fresh) == fresh.size();
        file.close();
        if(!written) QFile::remove(path);
    }
    read();
    static bool said = false;
    if(secret.size() != 32 && !said){
        said = true;
        qWarning("extension host: no secret could be kept at %s; extensions will ask nobody", qPrintable(path));
    }
    return secret;
}

QByteArray ExtensionHost::KeyFor(const QString &id){
    return ExtensionHostWire::KeyFor(Secret(true), id);
}

bool ExtensionHost::ShimsOn(){
    static const bool on = Application::GlobalSettings().value(QStringLiteral("network/@ExtensionShims"), true).toBool();
    return on;
}

QString ExtensionHost::CopyRoot(){
    return Application::StateDirectory() + QStringLiteral("extension-copies");
}

void ExtensionHost::Handle(const HeldAsk &ask, quint64 view){
    if(!ask) return;
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    const QUrl initiator = ask->Initiator();
    const QMap<QByteArray, QByteArray> headers = ask->Headers();
    const QByteArray key = ExtensionHostWire::HeaderOf(headers, ExtensionHostWire::KEY_HEADER);
    const QByteArray call = ExtensionHostWire::HeaderOf(headers, ExtensionHostWire::CALL_HEADER);

    if(ExtensionHostWire::IsBindUrl(ask->Url())){
        Bind(ask, view);
        return;
    }
    if(ExtensionHostWire::IsUserScriptMessageUrl(ask->Url())){
        UserScriptMessage(ask, view);
        return;
    }

    QString id;
    if(m_Controller && ExtensionHostWire::IsCallUrl(ask->Url()) &&
       !ExtensionHostWire::ExtensionIdOf(initiator).isEmpty())
        id = ExtensionHostWire::Admit(ask->Method(), initiator, key, m_Controller->ShimmedIds(), Secret(false));
    if(id.isEmpty()){
        ask->Fail();
        return;
    }

    ExtensionHostWire::Call asked = ExtensionHostWire::ParseCall(call);
    if(!asked.error.isEmpty()){
        ask->Reply(ExtensionHostWire::Refused(asked.error));
        return;
    }

    if(ExtensionHostWire::TakesBody(asked.api)){
        const bool main = asked.api == QStringLiteral("vanilla.mainScripts");
        const bool user = asked.api == QStringLiteral("userScripts.register") || asked.api == QStringLiteral("userScripts.update")
            || asked.api == QStringLiteral("vanilla.userScriptReply");
        if(main ? !m_Controller->HasPermission(id, QStringLiteral("scripting"))
           : user ? !m_Controller->HasPermission(id, QStringLiteral("userScripts"))
           : (!m_Controller->HasPermission(id, QStringLiteral("declarativeNetRequest"))
              && !m_Controller->HasPermission(id, QStringLiteral("declarativeNetRequestWithHostAccess")))){
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(
                main ? QStringLiteral("scripting.registerContentScripts") : asked.api)));
            return;
        }
        QByteArray body;
        QString error = QStringLiteral("what was sent is too long, or was not sent");
        if(ask->Body(ExtensionHostWire::BODY_LIMIT, &body)) asked.args = ExtensionHostWire::ArgsOfBody(body, &error);
        if(!error.isEmpty()){
            ask->Reply(ExtensionHostWire::Refused(error));
            return;
        }
    }

    if(asked.api == QStringLiteral("vanilla.mainScripts")){
        MainScriptsCall(ask, asked.args, id);
        return;
    }
    if(UserScriptMessageCall(ask, asked, id)) return;
    if(asked.api.startsWith(QStringLiteral("userScripts."))){
        UserScriptsCall(ask, asked.api, asked.args, id);
        return;
    }

    if(ExtensionHostWire::IsAct(asked.api)){
        Pending pending;
        pending.act = ExtensionHostWire::ParseAct(asked, id);
        if(!pending.act.error.isEmpty()){
            ask->Reply(ExtensionHostWire::Refused(pending.act.error));
            return;
        }
        if((pending.act.kind == ExtensionHostWire::Act::Search && !m_Controller->HasPermission(id, QStringLiteral("search")))
           || (pending.act.kind == ExtensionHostWire::Act::Restore && !m_Controller->HasPermission(id, QStringLiteral("sessions")))){
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
            return;
        }
        pending.ask = ask;
        pending.host = this;
        pending.sight = m_Controller->SightOf(id);
        if(!Line().Push(pending)){
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotEditable()));
            return;
        }
        DrainSoon(0);
        return;
    }

    if(asked.api.startsWith(QStringLiteral("sidePanel."))){
        if(!m_Controller->HasPermission(id, QStringLiteral("sidePanel")))
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
        else {
            QPointer<ExtensionHost> self(this);
            const HeldAsk held = ask;
            const ExtensionHostWire::Call call = asked;
            QTimer::singleShot(0, this, [self, held, call, id](){
                if(self && held && held->Alive()) held->Reply(self->SidePanelCall(call, id));
            });
        }
        return;
    }
    if(asked.api == QStringLiteral("tabs.captureVisibleTab")){
        ask->Reply(CaptureCall(asked, id));
        return;
    }

    if((asked.api == QStringLiteral("tabs.getZoom") || asked.api == QStringLiteral("tabs.setZoom"))){
        const ExtensionHostWire::ZoomCall zoom = ExtensionHostWire::ParseZoom(asked);
        if(!zoom.error.isEmpty()){
            ask->Reply(ExtensionHostWire::Refused(zoom.error));
            return;
        }
        ViewNode *vn = zoom.tab ? NodeOf(zoom.tab) : CurrentNode();
        if(!vn){
            ask->Reply(ExtensionHostWire::Refused(zoom.tab ? ExtensionHostWire::NoTab(zoom.tab) : QStringLiteral("No current tab.")));
            return;
        }
        if(!zoom.set){
            if(View *view = vn->GetView()) if(view->visible()) view->SaveZoom();
            ask->Reply(ExtensionHostWire::ZoomAnswer(vn->GetZoom()));
            return;
        }
        View *view = vn->GetView();
        vn->SetZoom(view ? view->FitZoom(static_cast<float>(zoom.factor)) : static_cast<float>(zoom.factor));
        if(view) view->RestoreZoom();
        TreeBank::SomethingChanged();
        ask->Reply(ExtensionHostWire::Done());
        return;
    }

    if(asked.api == QStringLiteral("vanilla.installed")){
        bool written = true;
        const QJsonObject details = m_Controller->TakeInstalled(id, &written);
        ask->Reply(ExtensionHostWire::InstalledAnswer(written ? details : QJsonObject()));
        return;
    }
    if(asked.api == QStringLiteral("vanilla.startup")){
        ask->Reply(ExtensionHostWire::StartupAnswer(m_Controller->TakeStartup(id)));
        return;
    }

    if(asked.api == QStringLiteral("vanilla.events")){
        Subscribe(ask, asked, id);
        return;
    }
    if(asked.api == QStringLiteral("vanilla.eventNames")){
        const QByteArray token = ExtensionHostWire::TokenOfEventNames(asked);
        if(token.isEmpty()){
            ask->Reply(ExtensionHostWire::Refused(QStringLiteral("vanilla.eventNames takes the token of whoever listens, and names")));
            return;
        }
        m_Subscribers.Names(id, token, ExtensionHostWire::NamesOfEvents(asked), ExtensionHostWire::NumberOfEventNames(asked));
        ask->Reply(ExtensionHostWire::Done());
        return;
    }
    if(asked.api == QStringLiteral("vanilla.abort")){
        const QByteArray token = ExtensionHostWire::TokenOfAbort(asked);
        if(token.isEmpty()){
            ask->Reply(ExtensionHostWire::Refused(QStringLiteral("vanilla.abort takes the token of whoever listened")));
            return;
        }
        HeldAsk held;
        m_Subscribers.Abort(id, token, &held);
        ask->Reply(ExtensionHostWire::Done());
        if(held && held->Alive()) held->Reply(ExtensionHostWire::AbortedAnswer());
        return;
    }

    if(asked.api.startsWith(QStringLiteral("notifications."))){
        if(!m_Qt || !m_Controller->HasPermission(id, QStringLiteral("notifications")))
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
        else {
            QPointer<ExtensionHost> self(this);
            const HeldAsk held = ask;
            const ExtensionHostWire::Call call = asked;
            QTimer::singleShot(0, this, [self, held, call, id](){
                if(!self) return;
                const QJsonObject reply = self->NotificationCall(call, id);
                if(held && held->Alive()) held->Reply(reply);
            });
        }
        return;
    }

    if(asked.api.startsWith(QStringLiteral("action."))){
        ask->Reply(ActionCall(asked, id));
        return;
    }
    if(asked.api == QStringLiteral("identity.launchWebAuthFlow")){
        if(!m_Controller->HasPermission(id, QStringLiteral("identity")))
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
        else
            IdentityCall(ask, asked, id);
        return;
    }
    if(asked.api.startsWith(QStringLiteral("offscreen."))){
        if(!m_Controller->HasPermission(id, QStringLiteral("offscreen")))
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
        else
            OffscreenCall(ask, asked, id);
        return;
    }
    if(asked.api.startsWith(QStringLiteral("downloads."))){
        if(!m_Controller->HasPermission(id, QStringLiteral("downloads")))
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(
                asked.api == QStringLiteral("downloads.expect") ? QStringLiteral("downloads.download") : asked.api)));
        else
            ask->Reply(DownloadCall(asked, id));
        return;
    }

    if(asked.api.startsWith(QStringLiteral("declarativeNetRequest."))){
        ask->Reply(m_Controller->RulesCall(id, asked.api, asked.args));
        return;
    }

    if(asked.api.startsWith(QStringLiteral("contextMenus."))){
        if(!m_Controller->HasPermission(id, QStringLiteral("contextMenus")) || m_Controller->MenusMirrored())
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
        else
            ask->Reply(m_Controller->MenuCall(id, asked.api, asked.args));
        return;
    }
    if(asked.api == QStringLiteral("vanilla.menuMirror")){
        if(!m_Controller->HasPermission(id, QStringLiteral("contextMenus")))
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
        else
            ask->Reply(m_Controller->MirrorCall(id, asked.args));
        return;
    }

    if((asked.api == QStringLiteral("history.search") || asked.api == QStringLiteral("bookmarks.getTree")
                                 || ExtensionHostWire::IsBookmarksRead(asked.api))){
        const bool history = asked.api == QStringLiteral("history.search");
        if(!m_Controller->HasPermission(id, history ? QStringLiteral("history") : QStringLiteral("bookmarks")))
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
        else if(history)
            ask->Reply(ExtensionHostWire::History(asked, Tabs(true), QDateTime::currentMSecsSinceEpoch()));
        else if(asked.api == QStringLiteral("bookmarks.getTree"))
            ask->Reply(ExtensionHostWire::BookmarkTree(asked, Tree()));
        else
            ask->Reply(ExtensionHostWire::Bookmarks(asked, Tree()));
        return;
    }
    if(asked.api == QStringLiteral("topSites.get")){
        if(!m_Controller->HasPermission(id, QStringLiteral("topSites")))
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
        else
            ask->Reply(ExtensionHostWire::TopSites(asked, Tabs(true)));
        return;
    }

    if(asked.api == QStringLiteral("commands.getAll")){
        foreach(const QJsonValue &given, asked.args){
            if(given.isNull()) continue;
            ask->Reply(ExtensionHostWire::Refused(QStringLiteral("commands.getAll takes no argument")));
            return;
        }
        ask->Reply(ExtensionHostWire::Accepted(m_Controller->CommandsOf(id, [](const QString &key){
            const QKeySequence seq = QKeySequence::fromString(ExtensionUi::QtKeyTextOf(key), QKeySequence::PortableText);
            return View::TakesKey(seq) || TreeBank::TakesKey(seq);
        })));
        return;
    }

    if(asked.api == QStringLiteral("fontSettings.getFontList")){
        if(!m_Controller->HasPermission(id, QStringLiteral("fontSettings")))
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
        else {
            QStringList families;
            foreach(const QString &family, QFontDatabase::families())
                if(!QFontDatabase::isPrivateFamily(family)) families << family;
            ask->Reply(ExtensionHostWire::FontList(asked, families));
        }
        return;
    }

    auto placeOf = [this](qint64 tab){
        if(!tab) return -1;
        const QList<ExtensionHostWire::Tab> all = Tabs();
        for(int i = 0; i < all.size(); i++)
            if(all.at(i).id == tab) return i;
        return -1;
    };

    if(asked.api == QStringLiteral("vanilla.tabOf")){
        qint64 tab = 0;
        if(View *view = ViewOf(m_Documents.ViewOf(ExtensionHostWire::NonceOfTabOf(asked))))
            if(ViewNode *vn = view->GetViewNode()) tab = static_cast<qint64>(vn->GetSerial());
        ask->Reply(ExtensionHostWire::TabOfAnswer(tab, placeOf(tab)));
        return;
    }

    if(asked.api == QStringLiteral("vanilla.menuTab")){
        bool ok = false;
        const QString page = ExtensionHostWire::PageUrlOfMenuTab(asked, &ok);
        if(!m_Controller->HasPermission(id, QStringLiteral("contextMenus"))){
            ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
            return;
        }
        if(!ok){
            ask->Reply(ExtensionHostWire::Refused(QStringLiteral("vanilla.menuTab takes the address of a page")));
            return;
        }
        const qint64 tab = m_MenuPicks.Take(page, Now());
        ask->Reply(ExtensionHostWire::TabOfAnswer(tab, placeOf(tab)));
        return;
    }

    const bool listing =
        asked.api == QStringLiteral("tabs.query") || asked.api == QStringLiteral("tabs.get")
        || asked.api == QStringLiteral("windows.getCurrent") || asked.api == QStringLiteral("windows.getAll");
    const QList<ExtensionHostWire::Tab> tabs = listing ? Tabs() : QList<ExtensionHostWire::Tab>();
    MainWindow *window = Application::GetCurrentWindow();
    ExtensionHostWire::Window told;
    if(window){
        const QRect place = window->geometry();
        told.focused = AnyFrameActive();
        told.state = window->isMinimized() ? QStringLiteral("minimized")
                   : window->isFullScreen() ? QStringLiteral("fullscreen")
                   : window->isMaximized() ? QStringLiteral("maximized") : QStringLiteral("normal");
        told.left = place.x(); told.top = place.y(); told.width = place.width(); told.height = place.height();
    }
    ask->Reply(ExtensionHostWire::Answer(asked, tabs, m_Controller->SightOf(id), window ? &told : nullptr));
}

void ExtensionHost::Subscribe(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id){
    using namespace ExtensionHostWire;
    typedef Subscribers<HeldAsk> Table;
    const QByteArray token = TokenOfEvents(asked);
    if(token.isEmpty()){
        ask->Reply(Refused(QStringLiteral("vanilla.events takes the token of whoever listens")));
        return;
    }
    if(m_Subscribers.Retired(id, token)){
        ask->Reply(StaleAnswer());
        return;
    }
    HeldAsk old;
    switch(m_Subscribers.Take(id, token, ask, Now(), StillThere, &old, NamesOfEvents(asked), CanWake(), NumberOfEventNames(asked))){
    case Table::Refused:
        ask->Reply(Refused(TooManySubscribers()));
        return;
    case Table::Replaced:
        if(old && old != ask && old->Alive()) old->Reply(StaleAnswer());
        break;
    case Table::Held:
        break;
    }
    const QList<QJsonObject> held = m_Wakes.Subscribed(id, Now());
    CloseWaker(id);
    foreach(const QJsonObject &event, held)
        m_Subscribers.FireTo(id, QSet<QByteArray>() << token, event, StillThere, Now());
    DeliverWaitingUserScriptMessages(id, token, m_UserScriptListeners.Of(id).contains(token), true);
    LookSoon(held.isEmpty() ? LOOK_SOON : 0);
}

void ExtensionHost::Fire(const QString &id, const QString &name, const QJsonArray &args, qint64 tab){
    using namespace ExtensionHostWire;
    QJsonArray whole = args;
    if(tab > 0){
        const QList<Tab> tabs = Tabs();
        int index = -1;
        for(int i = 0; i < tabs.size(); i++)
            if(tabs.at(i).id == tab) index = i;
        if(index < 0) return;
        whole.append(TabAnswer(tab, tabs, m_Controller ? m_Controller->SightOf(id) : Sight()).value(QStringLiteral("value")));
    }
    if(name == QStringLiteral("vanilla.actionMenuClicked")){
        if(m_Controller) m_Controller->SendToWorker(id, whole);
        return;
    }
    const QJsonObject event = FiredEvent(name, whole);
    if(name == QStringLiteral("contextMenus.onClicked") || name == QStringLiteral("action.onClicked")
       || name == QStringLiteral("commands.onCommand")){
        m_Subscribers.Prune(StillThere, Now());
        if(CanWake() && !m_Subscribers.Listening(id, StillThere, Now()) && m_Wakes.Hold(id, event, Now())){
            Wake(id);
            return;
        }
    }
    else if(name.startsWith(QStringLiteral("downloads."))){
        m_Subscribers.Prune(StillThere, Now());
        if(CanWake() && !m_Subscribers.Holding(id, StillThere) && m_Subscribers.NamesOf(id).contains(name)
           && m_Wakes.Hold(id, event, Now())){
            Wake(id);
            return;
        }
    }
    if(m_Subscribers.Fire(id, event)) LookSoon(0);
}

#ifdef WEBENGINEVIEW
bool ExtensionHost::CanWake() const {
    return qobject_cast<QWebEngineProfile*>(m_Owner) && !m_Closed;
}

void ExtensionHost::Wake(const QString &id, bool wanted){
    if(!m_Wakes.ShouldWake(id, Now(), wanted || m_UserScriptMessages.HasWaiting(id))) return;
    QWebEngineProfile *profile = qobject_cast<QWebEngineProfile*>(m_Owner);
    if(!profile || m_Closed || !Runs(id)) return;
    const QUrl url(QStringLiteral("chrome-extension://") + id + QStringLiteral("/vanilla_wake.html"));
    OffscreenPage *page = new OffscreenPage(profile, url, this);
    m_Wakers.insert(id, page);
    m_Wakes.Woke(id, Now());
    page->setUrl(url);
    if(!m_WakeSweep){
        m_WakeSweep = new QTimer(this);
        m_WakeSweep->setInterval(2000);
        connect(m_WakeSweep, &QTimer::timeout, this, &ExtensionHost::SweepWakes);
    }
    if(!m_WakeSweep->isActive()) m_WakeSweep->start();
}
#else
bool ExtensionHost::CanWake() const { return false; }
void ExtensionHost::Wake(const QString &id, bool wanted){ Q_UNUSED(id); Q_UNUSED(wanted); }
#endif

void ExtensionHost::CloseWaker(const QString &id){
    const QPointer<QObject> page = m_Wakers.take(id);
    if(page) page->deleteLater();
}

void ExtensionHost::SweepWakes(){
    foreach(const QString &id, m_Wakes.Expire(Now())) CloseWaker(id);
    if(!m_Wakes.AnyWaking() && m_WakeSweep) m_WakeSweep->stop();
}

QJsonObject ExtensionHost::ActionCall(const ExtensionHostWire::Call &asked, const QString &id){
    using namespace ExtensionHostWire;
    using namespace ExtensionUi;
    if(!m_Controller) return Refused(NotAvailable(asked.api));
    const QString api = asked.api;
    const bool enabling = api == QStringLiteral("action.enable") || api == QStringLiteral("action.disable");
    const QJsonValue first = asked.args.isEmpty() ? QJsonValue() : asked.args.at(0);
    const QJsonObject details = first.toObject();
    const QJsonValue named = enabling ? first : details.value(QStringLiteral("tabId"));
    TabNow tab;
    if(!named.isUndefined() && !named.isNull()){
        bool whole = false;
        const qint64 wanted = TabIdOf(named, &whole);
        if(!whole) return Refused(QStringLiteral("Invalid value for argument 1. Property 'tabId': Expected integer."));
        const QList<Tab> tabs = Tabs();
        foreach(const Tab &one, tabs)
            if(one.id == wanted){ tab.id = one.id; tab.url = one.url; }
        if(tab.id <= 0) return Refused(NoTab(wanted));
    }
    const ExtensionRow row = m_Controller->RowOf(id);
    if(api == QStringLiteral("action.openPopup")){
        ViewNode *current = CurrentNode();
        TabNow now;
        if(current){
            now.id = static_cast<qint64>(current->GetSerial());
            View *view = current->GetView();
            now.url = view ? view->url() : current->GetUrl();
        }
        const QString why = WhyNotOpenPopup(!row.manifest.popup.isEmpty(), m_Controller->ActionOf(id), current ? &now : nullptr);
        if(!why.isEmpty()) return Refused(why);
        m_Controller->OpenPopup(id, static_cast<qint64>(current->GetSerial()));
        return Done();
    }
    if(api == QStringLiteral("action.getUserSettings")){
        QJsonObject settings;
        settings[QStringLiteral("isOnToolbar")] = row.pinned;
        return Accepted(settings);
    }
    if(api.startsWith(QStringLiteral("action.get")) || api == QStringLiteral("action.isEnabled")){
        Action none;
        Action *action = m_Controller->ActionOf(id);
        return GetOfAction(api, action ? *action : none, tab.id > 0 ? &tab : nullptr, row.manifest.title);
    }

    Action &action = m_Controller->ActionFor(id);
    if(api == QStringLiteral("action.setIcon")){
        Icon icon;
        if(details.contains(QStringLiteral("imageData"))){
            icon.bytes = IconBytesOf(details.value(QStringLiteral("imageData")));
            if(icon.isNull()) return Refused(QStringLiteral("Invalid value for argument 1. Property 'imageData': not a PNG of up to %1 bytes encoded.").arg(ICON_SENT_LIMIT));
        } else {
            const QString relative = IconPathOf(details.value(QStringLiteral("path")));
            const QString file = relative.isEmpty() || row.manifest.folder.isEmpty()
                ? QString() : ExtensionManifest::Resource(row.manifest.folder, relative);
            QFile reading(file);
            if(file.isEmpty() || !reading.open(QIODevice::ReadOnly) || reading.size() > ICON_BYTES_LIMIT)
                return Refused(QStringLiteral("Invalid value for argument 1. Property 'path': the icon could not be read."));
            icon.bytes = reading.readAll();
        }
        action.SetIcon(tab, icon);
    } else if(api == QStringLiteral("action.setBadgeText")){
        const QJsonValue text = details.value(QStringLiteral("text"));
        if(!text.isString() && !text.isNull() && !text.isUndefined()) return Refused(QStringLiteral("Invalid value for argument 1. Property 'text': Expected string."));
        if(text.isString()) action.SetBadgeText(tab, text.toString());
        else if(tab.id > 0) action.UnsetBadgeText(tab);
        else action.SetBadgeText(tab, QString());
    } else if(api == QStringLiteral("action.setBadgeBackgroundColor") || api == QStringLiteral("action.setBadgeTextColor")){
        const QString color = ColorOf(details.value(QStringLiteral("color")));
        if(color.isEmpty()) return Refused(QStringLiteral("Invalid value for argument 1. Property 'color': Expected a color string or [r, g, b, a]."));
        if(api == QStringLiteral("action.setBadgeBackgroundColor")) action.SetBadgeColor(tab, color);
        else action.SetTextColor(tab, color);
    } else if(api == QStringLiteral("action.setTitle")){
        const QJsonValue title = details.value(QStringLiteral("title"));
        if(!title.isString() && !title.isNull() && !title.isUndefined()) return Refused(QStringLiteral("Invalid value for argument 1. Property 'title': Expected string."));
        if(title.isString()) action.SetTitle(tab, title.toString());
        else if(tab.id > 0) action.UnsetTitle(tab);
        else action.SetTitle(tab, QString());
    } else if(enabling){
        action.SetEnabled(tab, api == QStringLiteral("action.enable"));
    } else {
        return Refused(NotAvailable(api));
    }
    m_Controller->ActionSet(id);
    return Done();
}

#ifdef WEBENGINEVIEW
void ExtensionHost::OffscreenCall(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id){
    using namespace ExtensionHostWire;
    const QString api = asked.api;
    QJsonObject reply;
    reply[QStringLiteral("ok")] = true;
    QHash<QString, Offscreen>::iterator have = m_Offscreen.find(id);
    if(have != m_Offscreen.end() && have->page.isNull()){ OffscreenGone(have, QStringLiteral("Offscreen document closed before fully loading.")); have = m_Offscreen.end(); }
    if(api == QStringLiteral("offscreen.hasDocument")){
        reply[QStringLiteral("value")] = have != m_Offscreen.end();
        ask->Reply(reply);
        return;
    }
    if(api == QStringLiteral("offscreen.contexts")){
        QJsonArray list;
        if(have != m_Offscreen.end()){
            QJsonObject one;
            one[QStringLiteral("documentUrl")] = have->url.toString(QUrl::FullyEncoded);
            one[QStringLiteral("documentId")] = have->documentId;
            one[QStringLiteral("contextId")] = have->contextId;
            list.append(one);
        }
        reply[QStringLiteral("value")] = list;
        ask->Reply(reply);
        return;
    }
    if(api == QStringLiteral("offscreen.closeDocument")){
        if(have == m_Offscreen.end()){ ask->Reply(Refused(QStringLiteral("No current offscreen document."))); return; }
        OffscreenGone(have, QStringLiteral("Offscreen document closed before fully loading."));
        ask->Reply(Done());
        return;
    }
    if(api != QStringLiteral("offscreen.createDocument")){ ask->Reply(Refused(NotAvailable(api))); return; }
    if(have != m_Offscreen.end()){ ask->Reply(Refused(QStringLiteral("Only a single offscreen document may be created."))); return; }
    const QJsonObject details = asked.args.isEmpty() ? QJsonObject() : asked.args.at(0).toObject();
    bool ok = false;
    const QUrl url = OffscreenUrlOf(id, details.value(QStringLiteral("url")).toString(), &ok);
    if(!ok){ ask->Reply(Refused(QStringLiteral("Invalid value for argument 1. Property 'url': Invalid URL."))); return; }
    QWebEngineProfile *profile = qobject_cast<QWebEngineProfile*>(m_Owner);
    if(!profile || m_Closed){ ask->Reply(Refused(QStringLiteral("chrome.offscreen.createDocument is not available in this view"))); return; }
    Offscreen made;
    OffscreenPage *page = new OffscreenPage(profile, url, this);
    made.page = page;
    made.url = url;
    made.documentId = QString::fromLatin1(QByteArray::number(QRandomGenerator::system()->generate64(), 16));
    made.contextId = QString::fromLatin1(QByteArray::number(QRandomGenerator::system()->generate64(), 16));
    made.making = ask;
    m_Offscreen.insert(id, made);
    connect(page, &QWebEnginePage::loadFinished, this, [this, id, page](bool loaded){ OffscreenLoaded(id, page, loaded); });
    page->setUrl(url);
}
#else
void ExtensionHost::OffscreenCall(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id){
    Q_UNUSED(id);
    ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
}
#endif

void ExtensionHost::OffscreenLoaded(const QString &id, QObject *page, bool loaded){
    using namespace ExtensionHostWire;
    QHash<QString, Offscreen>::iterator have = m_Offscreen.find(id);
    if(have == m_Offscreen.end() || have->page.data() != page || have->loaded) return;
    if(loaded){
        have->loaded = true;
        if(have->making && have->making->Alive()) have->making->Reply(Done());
        have->making.reset();
        return;
    }
    OffscreenGone(have, QStringLiteral("Page failed to load."));
}

QHash<QString, ExtensionHost::Offscreen>::iterator ExtensionHost::OffscreenGone(QHash<QString, Offscreen>::iterator gone, const QString &why){
    const Offscreen was = *gone;
    QHash<QString, Offscreen>::iterator next = m_Offscreen.erase(gone);
    if(was.making && was.making->Alive() && !why.isEmpty()) was.making->Reply(ExtensionHostWire::Refused(why));
    if(was.page) was.page->deleteLater();
    return next;
}

#ifdef WEBENGINEVIEW
void ExtensionHost::IdentityCall(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id){
    using namespace ExtensionHostWire;
    const ExtensionHostWire::AuthFlow details = ParseAuthFlow(asked);
    if(!details.error.isEmpty()){ ask->Reply(Refused(details.error)); return; }
    QWebEngineProfile *profile = qobject_cast<QWebEngineProfile*>(m_Owner);
    if(!profile || m_Closed){ ask->Reply(Refused(QStringLiteral("chrome.identity.launchWebAuthFlow is not available in this view"))); return; }
    QHash<QString, Flow>::iterator have = m_Flows.find(id);
    if(have != m_Flows.end()){
        if(!details.interactive && have->interactive){ ask->Reply(Refused(QString::fromLatin1(AUTH_INTERACTION))); return; }
        AuthEnd(id, have->serial, Refused(QString::fromLatin1(details.interactive ? AUTH_NOT_APPROVED : AUTH_INTERACTION)));
    }
    if(IsAuthRedirect(id, details.url)){
        ask->Reply(Accepted(details.url.toString(QUrl::FullyEncoded)));
        return;
    }
    Flow flow;
    flow.serial = ++m_FlowSerial;
    flow.ask = ask;
    flow.interactive = details.interactive;
    flow.abortOnLoad = details.abortOnLoad;
    flow.deadline = details.interactive ? qint64(25) * 60 * 1000 : details.timeoutMs;
    flow.started.start();
    const quint64 serial = flow.serial;
    AuthPage *page = new AuthPage(profile, id, details.interactive, [this, id, serial](const QUrl &url){
        AuthEnd(id, serial, ExtensionHostWire::Accepted(url.toString(QUrl::FullyEncoded)));
    }, this);
    flow.page = page;
    QTimer *watch = new QTimer(this);
    watch->setInterval(2000);
    connect(watch, &QTimer::timeout, this, [this, id, serial](){ AuthWatch(id, serial); });
    flow.watch = watch;
    m_Flows.insert(id, flow);
    connect(page, &QWebEnginePage::loadFinished, this, [this, id, serial](bool loaded){ AuthLoaded(id, serial, loaded); });
    watch->start();
    page->setUrl(details.url);
}

void ExtensionHost::AuthLoaded(const QString &id, quint64 serial, bool loaded){
    using namespace ExtensionHostWire;
    QHash<QString, Flow>::iterator have = m_Flows.find(id);
    if(have == m_Flows.end() || have->serial != serial) return;
    if(!have->interactive){
        if(have->abortOnLoad) AuthEnd(id, serial, Refused(QString::fromLatin1(loaded ? AUTH_INTERACTION : AUTH_NOT_LOADED)));
        return;
    }
    QWebEnginePage *page = qobject_cast<QWebEnginePage*>(have->page.data());
    if(have->window || !page) return;
    const QString name = m_Controller ? m_Controller->RowOf(id).manifest.name : QString();
    AuthWindow *window = new AuthWindow(page, name.isEmpty() ? id : name, [this, id, serial](){
        AuthEnd(id, serial, Refused(QString::fromLatin1(AUTH_NOT_APPROVED)));
    });
    have->window = window;
    window->show();
    window->raise();
    window->activateWindow();
}
#else
void ExtensionHost::IdentityCall(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id){
    Q_UNUSED(id);
    ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(asked.api)));
}

void ExtensionHost::AuthLoaded(const QString &id, quint64 serial, bool loaded){
    Q_UNUSED(id); Q_UNUSED(serial); Q_UNUSED(loaded);
}
#endif

void ExtensionHost::AuthWatch(const QString &id, quint64 serial){
    QHash<QString, Flow>::iterator have = m_Flows.find(id);
    if(have == m_Flows.end() || have->serial != serial) return;
    if(!have->ask || !have->ask->Alive()){ AuthEnd(id, serial, QJsonObject()); return; }
    if(have->started.elapsed() >= have->deadline)
        AuthEnd(id, serial, ExtensionHostWire::Refused(QString::fromLatin1(
            have->interactive ? ExtensionHostWire::AUTH_NOT_APPROVED : ExtensionHostWire::AUTH_INTERACTION)));
}

void ExtensionHost::AuthEnd(const QString &id, quint64 serial, const QJsonObject &answer){
    QHash<QString, Flow>::iterator have = m_Flows.find(id);
    if(have == m_Flows.end() || have->serial != serial) return;
    Flow was = *have;
    m_Flows.erase(have);
    if(!answer.isEmpty() && was.ask && was.ask->Alive()) was.ask->Reply(answer);
    AuthGone(was, false);
}

void ExtensionHost::AuthGone(Flow &flow, bool now){
    if(QTimer *watch = qobject_cast<QTimer*>(flow.watch.data())){
        watch->stop();
        if(now) delete watch; else watch->deleteLater();
    }
    QWidget *window = qobject_cast<QWidget*>(flow.window.data());
    if(window) window->hide();
    QObject *page = flow.page.data();
    if(now){
        delete window;
        delete page;
    } else {
#ifdef WEBENGINEVIEW
        if(AuthPage *login = dynamic_cast<AuthPage*>(page)) login->Dispose(window);
        else
#endif
        {
            if(window) window->deleteLater();
            if(page) page->deleteLater();
        }
    }
    flow.ask.reset();
}

void ExtensionHost::CloseFlowsOfOthers(){
    if(!m_Controller){ CloseFlows(false); return; }
    QList<QPair<QString, quint64> > gone;
    for(QHash<QString, Flow>::const_iterator i = m_Flows.constBegin(); i != m_Flows.constEnd(); ++i){
        const bool wanted = Runs(i.key()) && m_Controller->HasPermission(i.key(), QStringLiteral("identity"));
        if(!wanted) gone << qMakePair(i.key(), i->serial);
    }
    for(const QPair<QString, quint64> &one : gone)
        AuthEnd(one.first, one.second, ExtensionHostWire::Refused(QString::fromLatin1(ExtensionHostWire::AUTH_NOT_APPROVED)));
}

void ExtensionHost::CloseFlows(bool now){
    QHash<QString, Flow> taken;
    taken.swap(m_Flows);
    for(QHash<QString, Flow>::iterator i = taken.begin(); i != taken.end(); ++i){
        if(i->ask && i->ask->Alive()) i->ask->Reply(ExtensionHostWire::Refused(QString::fromLatin1(ExtensionHostWire::AUTH_NOT_APPROVED)));
        AuthGone(i.value(), now);
    }
}

void ExtensionHost::CloseOffscreen(bool now){
    CloseFlows(now);
    if(now) m_Closed = true;
    foreach(const QString &id, m_Wakers.keys()){
        m_Wakes.Forget(id);
        const QPointer<QObject> page = m_Wakers.take(id);
        if(!page) continue;
        if(now) delete page.data();
        else page->deleteLater();
    }
    QObject *downloader = m_Downloader.data();
    m_Downloader.clear();
    QHash<QString, Offscreen> taken;
    taken.swap(m_Offscreen);
    const QString why = QStringLiteral("Offscreen document closed before fully loading.");
    for(QHash<QString, Offscreen>::const_iterator i = taken.constBegin(); i != taken.constEnd(); ++i){
        if(i->making && i->making->Alive()) i->making->Reply(ExtensionHostWire::Refused(why));
        if(!i->page) continue;
        if(now) delete i->page.data();
        else i->page->deleteLater();
    }
    if(downloader){
        if(now) delete downloader;
        else downloader->deleteLater();
    }
}

void ExtensionHost::MainScriptsCall(const HeldAsk &ask, const QJsonArray &args, const QString &id){
    const QString folder = m_Controller ? m_Controller->RunFolderOf(id) : QString();
    const QString root = folder.isEmpty() ? QString() : QDir(folder).canonicalPath();
    if(!m_ApplyMainScripts || root.isEmpty() || !MainScriptsWanted(id, folder)){
        ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(QStringLiteral("scripting.registerContentScripts"))));
        return;
    }
    QString error;
    const QList<ExtensionMainScripts::Registration> list = ExtensionMainScripts::Parse(args, &error);
    if(!error.isEmpty()){
        ask->Reply(ExtensionHostWire::Refused(error));
        return;
    }
    const ExtensionRow row = m_Controller->RowOf(id);
    const QList<ExtensionMainScripts::Script> scripts = ExtensionMainScripts::ScriptsOf
        (id, list, row.manifest.hostPermissions, CopyReaderOf(id), &error);
    if(!error.isEmpty()){
        ask->Reply(ExtensionHostWire::Refused(error));
        return;
    }
    const ExtensionMainScripts::Table::Change change = m_MainScripts.Put(id, folder, scripts);
    if(!change.isEmpty()) m_ApplyMainScripts(change);
    m_MainStore.Put(id, row.manifest.version, args);
    WriteMainStore();
    ask->Reply(ExtensionHostWire::Done());
}

bool ExtensionHost::ScriptsWanted(const QString &id, const QString &folder, const QString &permission) const {
    if(!m_Controller || folder.isEmpty()) return false;
    return Runs(id) && m_Controller->RowOf(id).manifest.error.isEmpty()
        && m_Controller->HasPermission(id, permission) && m_Controller->RunFolderOf(id) == folder;
}

namespace {
    void ReadKept(const QString &path, ExtensionMainScripts::Store *store, QString *file, QByteArray *written){
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly)){
            if(f.exists()) qWarning("extension host: %s could not be read; nothing is kept this run", qPrintable(path));
            else *file = path;
            return;
        }
        *file = path;
        *written = f.readAll();
        *store = ExtensionMainScripts::Store::FromJson(*written);
    }
    void WriteKept(const QString &path, const ExtensionMainScripts::Store &store, QByteArray *written){
        if(path.isEmpty()) return;
        const QByteArray bytes = store.ToJson();
        if(bytes == *written) return;
        QSaveFile file(path);
        if(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit()){
            *written = bytes;
            return;
        }
        qWarning("extension host: what is kept could not be written to %s", qPrintable(path));
    }
}

void ExtensionHost::ReadMainStore(const QString &path){
    ReadKept(path, &m_MainStore, &m_MainStoreFile, &m_MainStoreWritten);
}

void ExtensionHost::WriteMainStore(){
    WriteKept(m_MainStoreFile, m_MainStore, &m_MainStoreWritten);
}

void ExtensionHost::ReadUserStore(const QString &path){
    ReadKept(path, &m_UserStore, &m_UserStoreFile, &m_UserStoreWritten);
}

void ExtensionHost::WriteUserStoreSoon(){
    if(m_UserStoreSoon) return;
    m_UserStoreSoon = true;
    QTimer::singleShot(0, this, [this](){
        m_UserStoreSoon = false;
        WriteKept(m_UserStoreFile, m_UserStore, &m_UserStoreWritten);
    });
}

void ExtensionHost::KeepMainScripts(){
    if(!m_ApplyMainScripts) return;
    const ExtensionMainScripts::Table::Change change = m_MainScripts.Keep([this](const QString &id, const QString &folder){
        return MainScriptsWanted(id, folder);
    });
    if(!change.isEmpty()) m_ApplyMainScripts(change);

    if(!m_Controller) return;
    ExtensionMainScripts::Now now;
    now.folder = [this](const QString &id){ return m_Controller->RunFolderOf(id); };
    now.wanted = [this](const QString &id, const QString &folder){ return MainScriptsWanted(id, folder); };
    now.version = [this](const QString &id){ return m_Controller->RowOf(id).manifest.version; };
    now.hostPermissions = [this](const QString &id){ return m_Controller->RowOf(id).manifest.hostPermissions; };
    const ExtensionMainScripts::Restored restored = ExtensionMainScripts::RestoreInto(m_MainScripts, m_MainStore, now);
    if(!restored.change.isEmpty()) m_ApplyMainScripts(restored.change);
    if(restored.thrown) WriteMainStore();
}

void ExtensionHost::UserScriptsCall(const HeldAsk &ask, const QString &api, const QJsonArray &args, const QString &id){
    const QString folder = m_Controller ? m_Controller->RunFolderOf(id) : QString();
    const bool known = api == QStringLiteral("userScripts.register") || api == QStringLiteral("userScripts.update")
        || api == QStringLiteral("userScripts.unregister") || api == QStringLiteral("userScripts.getScripts")
        || api == QStringLiteral("userScripts.configureWorld") || api == QStringLiteral("userScripts.getWorldConfigurations")
        || api == QStringLiteral("userScripts.resetWorldConfiguration");
    if(!known || !m_ApplyMainScripts || m_UserStoreFile.isEmpty() || !UserScriptsWanted(id, folder)){
        ask->Reply(ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(api)));
        return;
    }
    CheckUserBook(id);
    ExtensionUserScripts::Book book = ExtensionUserScripts::Book::Of(m_UserStore.entries.value(id).list);
    const QStringList had = book.WorldIds();
    QString error;
    if(api == QStringLiteral("userScripts.getScripts")){
        const QJsonArray scripts = book.Get(args.at(0), &error);
        ask->Reply(error.isEmpty() ? ExtensionHostWire::Accepted(scripts) : ExtensionHostWire::Refused(error));
        return;
    }
    if(api == QStringLiteral("userScripts.getWorldConfigurations")){
        ask->Reply(ExtensionHostWire::Accepted(book.Worlds()));
        return;
    }
    const ExtensionUserScripts::Reader read = CopyReaderOf(id);
    const bool adding = api == QStringLiteral("userScripts.register") || api == QStringLiteral("userScripts.update");
    const bool done = api == QStringLiteral("userScripts.register") ? book.Register(args.at(0), read, &error)
                    : api == QStringLiteral("userScripts.update") ? book.Update(args.at(0), read, &error)
                    : api == QStringLiteral("userScripts.configureWorld") ? book.Configure(args.at(0), &error)
                    : api == QStringLiteral("userScripts.resetWorldConfiguration") ? book.Reset(args.at(0), &error)
                    : book.Unregister(args.at(0), &error);
    if(!done){
        ask->Reply(ExtensionHostWire::Refused(error));
        return;
    }
    QStringList fresh;
    foreach(const QString &w, book.WorldIds()) if(!had.contains(w) && !m_UserWorlds.Of(id, w)) fresh << w;
    if(adding && !fresh.isEmpty() && !m_UserWorlds.Take(id, fresh)){
        ask->Reply(ExtensionHostWire::Refused(QStringLiteral("Too many worlds of user scripts.")));
        return;
    }
    if(book.IsEmpty()) m_UserStore.Drop(id);
    else m_UserStore.entries.insert(id, ExtensionMainScripts::Store::Entry{ m_Controller->RowOf(id).manifest.version, book.Kept() });
    WriteUserStoreSoon();
    PutUserScripts(id);
    ask->Reply(ExtensionHostWire::Done());
}

ExtensionUserScripts::Reader ExtensionHost::CopyReaderOf(const QString &id) const {
    const QString folder = m_Controller ? m_Controller->RunFolderOf(id) : QString();
    const QString root = folder.isEmpty() ? QString() : QDir(folder).canonicalPath();
    return [root](const QString &name, QByteArray *bytes){
        return !root.isEmpty() && ExtensionMainScripts::ReadInside(root, name, bytes);
    };
}

void ExtensionHost::CheckUserBook(const QString &id){
    if(!m_Controller) return;
    const ExtensionRow row = m_Controller->RowOf(id);
    if(row.manifest.id != id || !row.manifest.error.isEmpty()) return;
    if(ExtensionUserScripts::ThrowKept(&m_UserChecked, id, row.manifest.version, m_UserStore.entries.contains(id),
                                       m_UserStore.entries.value(id).version)){
        m_UserStore.Drop(id);
        WriteUserStoreSoon();
    }
}

void ExtensionHost::PutUserScripts(const QString &id){
    const ExtensionUserScripts::Book book = ExtensionUserScripts::Book::Of(m_UserStore.entries.value(id).list);
    foreach(const QString &w, book.WorldIds())
        if(!m_UserWorlds.Take(id, QStringList() << w))
            qWarning("extension host: no world is left for the user scripts of %s of the world '%s'", qPrintable(id), qPrintable(w));
    foreach(const QByteArray &secret, m_WorldSecrets.keys())
        if(m_WorldSecrets.value(secret).first == id) m_WorldSecrets.remove(secret);
    const QString folder = m_Controller->RunFolderOf(id);
    const ExtensionUserScripts::Prelude prelude = [this, id, folder](const QString &worldId){
        const QByteArray secret = ExtensionHostWire::WorldSecret(ProcessSecret(), m_ProfileSpace, id, folder, worldId);
        if(secret.isEmpty()) return QString();
        m_WorldSecrets.insert(secret, qMakePair(id, worldId));
        return Cdp::UserScriptPrelude(id, secret);
    };
    QStringList skipped;
    const QList<ExtensionMainScripts::Script> scripts = ExtensionUserScripts::ScriptsOf
        (id, book, m_Controller->RowOf(id).manifest.hostPermissions, m_UserWorlds, CopyReaderOf(id), &skipped, prelude);
    foreach(const QString &why, skipped)
        qWarning("extension host: a user script of %s is not put in: %s", qPrintable(id), qPrintable(why));
    const ExtensionMainScripts::Table::Change change = m_UserScripts.Put(id, m_Controller->RunFolderOf(id), scripts);
    if(!change.isEmpty()) m_ApplyMainScripts(change);
}

void ExtensionHost::KeepUserScripts(){
    if(!m_ApplyMainScripts || !m_Controller) return;
    QList<ExtensionHostWire::UserScriptMessages<HeldAsk>::Done> dropped;
    foreach(const QString &id, m_UserScriptMessages.Extensions())
        if(!UserScriptsWanted(id, m_Controller->RunFolderOf(id))) dropped << m_UserScriptMessages.Drop(id);
    foreach(const QString &id, QSet<QString>(m_UserScriptNamedEver))
        if(!UserScriptsWanted(id, m_Controller->RunFolderOf(id))) m_UserScriptNamedEver.remove(id);
    const ExtensionMainScripts::Table::Change change = m_UserScripts.Keep([this](const QString &id, const QString &folder){
        return UserScriptsWanted(id, folder);
    });
    if(!change.isEmpty()) m_ApplyMainScripts(change);
    foreach(const QString &id, m_UserStore.entries.keys()){
        if(m_UserScripts.Has(id)) continue;
        CheckUserBook(id);
        if(m_UserStore.entries.contains(id) && UserScriptsWanted(id, m_Controller->RunFolderOf(id))) PutUserScripts(id);
    }
    AnswerUserScriptMessages(dropped);
}

void ExtensionHost::UserScriptMessage(const HeldAsk &ask, quint64 view){
    using namespace ExtensionHostWire;
    if(ask->Method() != "POST" || !m_Controller || !m_ApplyMainScripts){
        ask->Fail();
        return;
    }
    const QMap<QByteArray, QByteArray> headers = ask->Headers();
    const QByteArray secret = HeaderOf(headers, WORLD_HEADER);
    if(secret.isEmpty() || !m_WorldSecrets.contains(secret)){
        ask->Fail();
        return;
    }
    const QString id = m_WorldSecrets.value(secret).first, worldId = m_WorldSecrets.value(secret).second;
    const ExtensionUserScripts::Book book = ExtensionUserScripts::Book::Of(m_UserStore.entries.value(id).list);
    if(!UserScriptsWanted(id, m_Controller->RunFolderOf(id)) || !book.Messaging(worldId)){
        ask->Fail();
        return;
    }
    QByteArray body;
    if(!ask->Body(BODY_LIMIT, &body)){
        ask->Reply(Refused(QStringLiteral("The message is too large.")));
        return;
    }
    const QJsonObject sent = QJsonDocument::fromJson(body).object();
    const QUrl url(sent.value(QStringLiteral("url")).toString());
    const QJsonValue frame = sent.value(QStringLiteral("frame"));
    const QUrl initiator = ask->Initiator();
    if(!url.isValid() || !frame.isDouble() || frame.toDouble() < 0 || frame.toDouble() != double(qint64(frame.toDouble()))
       || initiator.scheme().isEmpty() || initiator.host().isEmpty()
       || url.scheme() != initiator.scheme() || url.host() != initiator.host() || url.port() != initiator.port()
       || !WithinHostPermissions(url, m_Controller->RowOf(id).manifest.hostPermissions)){
        ask->Reply(Refused(NoReceiver()));
        return;
    }
    QJsonObject sender;
    sender[QStringLiteral("id")] = id;
    sender[QStringLiteral("url")] = url.toString();
    sender[QStringLiteral("origin")] = initiator.toString(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::StripTrailingSlash);
    sender[QStringLiteral("frameId")] = qint64(frame.toDouble());
    if(!worldId.isEmpty()) sender[QStringLiteral("userScriptWorldId")] = worldId;
    const quint64 number = view ? view : ViewOfStamp(ProcessSecret(), HeaderOf(headers, VIEW_HEADER));
    if(View *v = ViewOf(number)){
        if(ViewNode *vn = v->GetViewNode()){
            const qint64 tab = static_cast<qint64>(vn->GetSerial());
            const QJsonValue told = TabAnswer(tab, Tabs(), m_Controller->SightOf(id)).value(QStringLiteral("value"));
            if(told.isObject()) sender[QStringLiteral("tab")] = told;
        }
    }
    m_Subscribers.Prune(StillThere, Now());
    m_UserScriptListeners.Prune(Now(), [this](const QString &extension, const QByteArray &token){ return m_Subscribers.Has(extension, token); });
    const QSet<QByteArray> listening = m_UserScriptListeners.Of(id);
    quint32 words[4];
    QRandomGenerator::system()->fillRange(words);
    const QByteArray ticket = QByteArray(reinterpret_cast<const char*>(words), sizeof words).toHex();
    const QJsonObject event = FiredEvent(QStringLiteral("runtime.onUserScriptMessage"),
                                         QJsonArray{ sent.value(QStringLiteral("message")), sender, QString::fromLatin1(ticket) });
    const QSet<QByteArray> told = m_Subscribers.FireTo(id, listening, event, StillThere, Now());
    if(told.isEmpty()){
        if(!WaitForWorker(CanWake(), m_UserScriptNamedEver.contains(id), m_Wakes.Resting(id, Now()),
                          m_Subscribers.Listening(id, StillThere, Now()))
           || !m_UserScriptMessages.Wait(ticket, id, ask, event, Now())){
            ask->Reply(Refused(NoReceiver()));
            return;
        }
        Wake(id);
    } else if(!m_UserScriptMessages.Add(ticket, id, ask, told, Now())){
        ask->Reply(Refused(QStringLiteral("Too many messages are waiting for an answer.")));
        return;
    }
    LookSoon(0);
    if(!m_UserScriptSweep){
        m_UserScriptSweep = new QTimer(this);
        m_UserScriptSweep->setInterval(2000);
        connect(m_UserScriptSweep, &QTimer::timeout, this, &ExtensionHost::SweepUserScriptMessages);
    }
    if(!m_UserScriptSweep->isActive()) m_UserScriptSweep->start();
}

bool ExtensionHost::UserScriptMessageCall(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id){
    using namespace ExtensionHostWire;
    const bool listen = asked.api == QStringLiteral("vanilla.userScriptListen");
    if(!listen && asked.api != QStringLiteral("vanilla.userScriptReply")) return false;
    if(!m_Controller->HasPermission(id, QStringLiteral("userScripts"))){
        ask->Reply(Refused(NotAvailable(asked.api)));
        return true;
    }
    const QByteArray token = asked.args.at(listen ? 0 : 1).toString().toUtf8();
    if(!IsNonce(token)){
        ask->Reply(Refused(QStringLiteral("%1 takes the token of whoever listens").arg(asked.api)));
        return true;
    }
    if(listen && m_Subscribers.Retired(id, token)){
        ask->Reply(Done());
        return true;
    }
    if(listen){
        m_Subscribers.Prune(StillThere, Now());
        m_UserScriptListeners.Name(id, token, Now(), [this](const QString &extension, const QByteArray &one){ return m_Subscribers.Has(extension, one); });
        m_UserScriptNamedEver.insert(id);
        DeliverWaitingUserScriptMessages(id, token, true, m_Subscribers.Has(id, token));
        ask->Reply(Done());
        return true;
    }
    const QString kind = asked.args.at(2).toString();
    const UserScriptMessages<HeldAsk>::Kind which = kind == QStringLiteral("value") ? UserScriptMessages<HeldAsk>::Value
        : kind == QStringLiteral("none") ? UserScriptMessages<HeldAsk>::NoAnswer : UserScriptMessages<HeldAsk>::NoListener;
    AnswerUserScriptMessages(m_UserScriptMessages.Reply(id, asked.args.at(0).toString().toUtf8(), token, which, asked.args.at(3)));
    ask->Reply(Done());
    return true;
}

void ExtensionHost::DeliverWaitingUserScriptMessages(const QString &id, const QByteArray &token, bool named, bool subscribed){
    const int handed = ExtensionHostWire::DeliverWaiting(m_UserScriptMessages, id, token, named, subscribed, Now(),
        [this, &id, &token](const QJsonObject &event){
            return !m_Subscribers.FireTo(id, QSet<QByteArray>() << token, event, StillThere, Now()).isEmpty();
        });
    if(handed) LookSoon(0);
}

void ExtensionHost::SweepUserScriptMessages(){
    m_Subscribers.Prune(StillThere, Now());
    const auto there = [this](const QString &extension, const QByteArray &token){ return m_Subscribers.Has(extension, token); };
    m_UserScriptListeners.Prune(Now(), there);
    const auto done = m_UserScriptMessages.Sweep(Now(), there, StillThere);
    if(!m_UserScriptMessages.Count() && m_UserScriptSweep) m_UserScriptSweep->stop();
    AnswerUserScriptMessages(done);
}

void ExtensionHost::AnswerUserScriptMessages(const QList<ExtensionHostWire::UserScriptMessages<HeldAsk>::Done> &done){
    foreach(const auto &one, done) if(!one.unheardOf.isEmpty()) m_UserScriptNamedEver.remove(one.unheardOf);
    foreach(const auto &one, done) if(StillThere(one.waiter)) one.waiter->Reply(one.answer);
}

void ExtensionHost::CloseOffscreenOfOthers(){
    foreach(const QString &id, m_Wakes.Extensions()){
        if(Runs(id)) continue;
        m_Wakes.Forget(id);
        CloseWaker(id);
    }
    foreach(const QString &id, m_Subscribers.Kept()){
        if(!Runs(id)) m_Subscribers.Forget(id);
    }
    CloseNoticesOfOthers();
    CloseFlowsOfOthers();
    SidePanels::UpdateAll();
    foreach(const QString &id, m_ActiveTabs.Ids()){
        if(!Runs(id)) m_ActiveTabs.Forget(id);
    }
    if(!m_Controller){ CloseOffscreen(false); return; }
    for(QHash<QString, Offscreen>::iterator i = m_Offscreen.begin(); i != m_Offscreen.end(); ){
        const bool wanted = Runs(i.key()) && m_Controller->HasPermission(i.key(), QStringLiteral("offscreen"));
        if(wanted && !i->page.isNull()){ ++i; continue; }
        i = OffscreenGone(i, QStringLiteral("Offscreen document closed before fully loading."));
    }
}

QJsonObject ExtensionHost::DownloadCall(const ExtensionHostWire::Call &asked, const QString &id){
    using namespace ExtensionHostWire;
    const QJsonObject details = asked.args.isEmpty() ? QJsonObject() : asked.args.at(0).toObject();
    QJsonObject reply;
    reply[QStringLiteral("ok")] = true;
    if(asked.api == QStringLiteral("downloads.search")){
        reply[QStringLiteral("value")] = m_Downloads.Search(id, details);
        return reply;
    }
    if(asked.api == QStringLiteral("downloads.erase")){
        QJsonArray erased;
        foreach(qint64 gone, m_Downloads.Erase(id, details)){
            if(QWebEngineDownloadRequest *request = qobject_cast<QWebEngineDownloadRequest*>(m_Requests.take(gone).data()))
                if(!request->isFinished()) request->cancel();
            erased.append(gone);
            Fire(id, QStringLiteral("downloads.onErased"), QJsonArray() << gone, 0);
        }
        reply[QStringLiteral("value")] = erased;
        return reply;
    }
    static const QStringList acts = QStringList() << QStringLiteral("downloads.cancel") << QStringLiteral("downloads.pause")
        << QStringLiteral("downloads.resume") << QStringLiteral("downloads.show");
    if(acts.contains(asked.api)){
        bool whole = false;
        const qint64 which = asked.args.isEmpty() ? 0 : ExtensionUi::TabIdOf(asked.args.at(0), &whole);
        if(!whole) return Refused(QStringLiteral("Invalid download id"));
        const ExtensionUi::DownloadRecord *record = m_Downloads.RecordOf(id, which);
        const QString why = ExtensionUi::WhyNotDownloadAct(asked.api, record);
        if(!why.isEmpty()) return Refused(why);
        QWebEngineDownloadRequest *request = record ? qobject_cast<QWebEngineDownloadRequest*>(m_Requests.value(which).data()) : nullptr;
        if(asked.api == QStringLiteral("downloads.cancel")){
            if(request && !request->isFinished()) request->cancel();
        } else if(asked.api == QStringLiteral("downloads.pause") || asked.api == QStringLiteral("downloads.resume")){
            if(!request || request->state() != QWebEngineDownloadRequest::DownloadInProgress)
                return Refused(QStringLiteral("Download must be in progress"));
            const bool pausing = asked.api == QStringLiteral("downloads.pause");
            if(pausing) request->pause(); else request->resume();
            const QJsonObject delta = m_Downloads.Paused(which, pausing);
            if(!delta.isEmpty()) Fire(id, QStringLiteral("downloads.onChanged"), QJsonArray() << delta, 0);
        } else {
            const QString folder = QFileInfo(record->filename).absolutePath();
            if(folder.startsWith(QStringLiteral("//")) || folder.startsWith(QStringLiteral("\\\\")) || !QFileInfo(folder).isDir())
                return Refused(QStringLiteral("Invalid download id"));
            QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
        }
        return reply;
    }
    if(asked.api != QStringLiteral("downloads.expect")) return Refused(NotAvailable(asked.api));
    const QString text = details.value(QStringLiteral("url")).toString();
    if(!ExtensionUi::AcceptsDownloadUrl(text)) return Refused(QStringLiteral("Invalid value for argument 1. Property 'url': Invalid URL."));
    const QUrl url(text);
    const QJsonValue filename = details.value(QStringLiteral("filename"));
    const QString name = filename.isString() ? filename.toString() : QString();
    if(!ExtensionUi::AcceptsDownloadFilename(name)) return Refused(QStringLiteral("Invalid filename"));
    QWebEnginePage *page = Downloader();
    if(!page) return Refused(QStringLiteral("chrome.downloads.download is not available in this view"));
    reply[QStringLiteral("value")] = m_Downloads.Expect(id, url, name, Now());
    page->download(url, name);
    QTimer::singleShot(ExtensionUi::DOWNLOADS_EXPECTED_MS + 500, this, [this](){
        foreach(const ExtensionUi::Downloads::Gone &gone, m_Downloads.Expired(Now()))
            Fire(gone.extension, QStringLiteral("downloads.onChanged"), QJsonArray() << gone.delta, 0);
    });
    return reply;
}

#ifdef WEBENGINEVIEW
QWebEnginePage *ExtensionHost::Downloader(){
    if(m_Downloader) return qobject_cast<QWebEnginePage*>(m_Downloader.data());
    QWebEngineProfile *profile = qobject_cast<QWebEngineProfile*>(m_Owner);
    if(!profile || m_Closed) return nullptr;
    OffscreenPage *page = new OffscreenPage(profile, QUrl(QStringLiteral("about:blank")), this);
    m_Downloader = page;
    page->setUrl(QUrl(QStringLiteral("about:blank")));
    return page;
}
#else
QWebEnginePage *ExtensionHost::Downloader(){ return nullptr; }
#endif

#ifdef WEBENGINEVIEW
void ExtensionHost::DownloadRequested(QWebEngineDownloadRequest *request){
    if(!request) return;
    if(!m_Downloader || request->page() != m_Downloader.data()) return;
    QString extension;
    const qint64 id = m_Downloads.Arrived(request->url(), Now(), &extension);
    if(!id) return;
    QPointer<QWebEngineDownloadRequest> held(request);
    m_Requests.insert(id, QPointer<QObject>(request));
    m_Downloads.Describe(id, QString(), request->mimeType(), request->totalBytes(), request->receivedBytes());
    Fire(extension, QStringLiteral("downloads.onCreated"), QJsonArray() << m_Downloads.Created(id), 0);
    auto ended = [this, id, extension, held](){
        if(!held) return;
        const QString path = QDir::cleanPath(QDir(held->downloadDirectory()).filePath(held->downloadFileName()));
        if(!held->isFinished()){
            if(held->state() == QWebEngineDownloadRequest::DownloadInProgress && !held->downloadFileName().isEmpty()){
                const QJsonObject named = m_Downloads.Named(id, path);
                if(!named.isEmpty()) Fire(extension, QStringLiteral("downloads.onChanged"), QJsonArray() << named, 0);
            }
            return;
        }
        const bool completed = held->state() == QWebEngineDownloadRequest::DownloadCompleted;
        m_Downloads.Describe(id, ExtensionUi::PathOfEndedDownload(completed, path), held->mimeType(), held->totalBytes(), held->receivedBytes());
        const QString why = completed ? QString()
            : held->state() == QWebEngineDownloadRequest::DownloadCancelled ? QStringLiteral("USER_CANCELED")
            : ExtensionUi::InterruptReasonOf(static_cast<int>(held->interruptReason()));
        const QJsonObject delta = m_Downloads.Ended(id, completed, why);
        m_Requests.remove(id);
        if(!delta.isEmpty()) Fire(extension, QStringLiteral("downloads.onChanged"), QJsonArray() << delta, 0);
    };
    if(request->isFinished()) ended();
    else {
        connect(request, &QWebEngineDownloadRequest::isFinishedChanged, this, ended);
        connect(request, &QWebEngineDownloadRequest::stateChanged, this, ended);
        connect(request, &QWebEngineDownloadRequest::isPausedChanged, this, [this, id, extension, held](){
            if(!held) return;
            const QJsonObject delta = m_Downloads.Paused(id, held->isPaused());
            if(!delta.isEmpty()) Fire(extension, QStringLiteral("downloads.onChanged"), QJsonArray() << delta, 0);
        });
    }
}
#else
void ExtensionHost::DownloadRequested(QWebEngineDownloadRequest *request){ Q_UNUSED(request); }
#endif

qint64 ExtensionHost::Publish(){
    const bool watch = CanWake() && m_Subscribers.Watches();
    if(!m_Subscribers.AnyWaiting(StillThere) && !watch){
        m_Subscribers.Prune(StillThere, Now());
        return -1;
    }
    if(!m_Controller) return -1;
    bool dated = false;
    foreach(const ExtensionHostWire::Subscribers<HeldAsk>::Entry &entry, m_Subscribers.Entries())
        if(m_Controller->HasPermission(entry.extension, QStringLiteral("history"))){ dated = true; break; }
    if(watch) foreach(const QString &id, m_Subscribers.Watched())
        if(m_Controller->HasPermission(id, QStringLiteral("history"))){ dated = true; break; }
    const QList<ExtensionHostWire::Tab> tabs = Tabs(dated);
    const bool focused = AnyFrameActive();
    const auto sees = [this](const QString &id){ return m_Controller ? m_Controller->SightOf(id) : ExtensionHostWire::Sight(); };
    const QStringList wake = watch ? m_Subscribers.Watch(tabs, focused, Now(), StillThere, sees) : QStringList();
    const QPointer<ExtensionHost> self(this);
    m_Subscribers.Publish(tabs, focused, Now(), StillThere, sees,
        [](const HeldAsk &ask, const QJsonObject &reply){ if(ask && ask->Alive()) ask->Reply(reply); });
    if(!self) return -1;
    qint64 again = -1;
    foreach(const QString &id, wake){
        Wake(id, true);
        const qint64 after = m_Wakes.Resting(id, Now()) ? m_Wakes.RestLeft(id, Now()) + 1 : LOOK_AGAIN;
        if(again < 0 || after < again) again = after;
    }
    return again;
}

void ExtensionHost::Changed(){
    foreach(ExtensionHost *host, Hosts()){
        if(!host->m_Subscribers.Count() && !(host->CanWake() && host->m_Subscribers.Watches())) continue;
        LookSoon(LOOK_SOON);
        return;
    }
}

void ExtensionHost::LookSoon(int milliseconds){
    if(!qApp || QCoreApplication::closingDown()) return;
    static QPointer<QTimer> timer;
    if(!timer){
        timer = new QTimer(qApp);
        timer->setSingleShot(true);
        QObject::connect(timer.data(), &QTimer::timeout, qApp, [](){ ExtensionHost::Look(); });
    }
    if(timer->isActive() && timer->remainingTime() <= milliseconds) return;
    timer->start(milliseconds);
}

void ExtensionHost::Look(){
    if(TreeBank::ChangeScope::Depth() != 0){
        LookSoon(LOOK_SOON);
        return;
    }
    bool waiting = false;
    qint64 owed = -1;
    foreach(ExtensionHost *host, QList<ExtensionHost*>(Hosts())){
        if(!Hosts().contains(host)) continue;
        const QPointer<ExtensionHost> kept(host);
        const qint64 again = host->Publish();
        if(again >= 0 && (owed < 0 || again < owed)) owed = again;
        if(kept) waiting = waiting || host->m_Subscribers.AnyWaiting(StillThere);
    }
    if(waiting) LookSoon(LOOK_AGAIN);
    else if(owed >= 0) LookSoon(static_cast<int>(owed));
}

void ExtensionHost::DrainSoon(int milliseconds){
    if(!Line().Reserve()) return;
    QTimer::singleShot(milliseconds, qApp, [](){
        Line().Fired();
        ExtensionHost::Drain();
    });
}

void ExtensionHost::Drain(){
    const auto outcome = Line().Drain(
        [](const Pending &pending){
            if(!pending.ownCleanup && !StillThere(pending.ask)) return;
            if(!pending.host) return;
            const QJsonObject answer = pending.host->Perform(pending);
            if(!answer.isEmpty() && StillThere(pending.ask)) pending.ask->Reply(answer);
        },
        [](){
            if(!TreeBank::MayChangeFromOutside()) return false;
            const Pending *next = Line().First();
            return !(next && next->ownCleanup && OverviewIsUp());
        });
    if(outcome != ExtensionHostWire::OneAtATime<Pending>::Declined) return;
    Line().Prune([](const Pending &pending){ return pending.ownCleanup || StillThere(pending.ask); });
    if(Line().Count()) DrainSoon(COME_BACK_AFTER);
}

QList<ExtensionHostWire::Tab> ExtensionHost::Tabs(bool dated) const {
    QList<ExtensionHostWire::Tab> tabs;
    TreeBank *bank = CurrentBank();
    if(ViewNode *root = TreeBank::GetViewRoot())
        Collect(root, m_Controller, m_ProfileSpace,
                bank ? bank->GetCurrentViewNode() : nullptr, dated, &tabs);
    return tabs;
}

ExtensionHostWire::Bookmark ExtensionHost::Tree() const {
    if(ViewNode *root = TreeBank::GetViewRoot())
        return Gather(root, m_Controller, m_ProfileSpace);
    return ExtensionHostWire::Bookmark();
}

ViewNode *ExtensionHost::NodeOf(qint64 id) const {
    ViewNode *vn = id > 0 && m_Controller ? TreeBank::TabOfSerial(static_cast<quint64>(id)) : nullptr;
    ViewNode *parent = vn && vn->GetParent() ? vn->GetParent()->ToViewNode() : nullptr;
    if(!vn || !parent) return nullptr;
    return VisibleTo(vn, m_Controller, [&](){ return DirIsOfProfile(parent, m_ProfileSpace); }) ? vn : nullptr;
}

ViewNode *ExtensionHost::CurrentNode() const {
    TreeBank *bank = CurrentBank();
    ViewNode *vn = bank ? bank->GetCurrentViewNode() : nullptr;
    return vn ? NodeOf(static_cast<qint64>(vn->GetSerial())) : nullptr;
}

QJsonObject ExtensionHost::Perform(const Pending &pending){
    using namespace ExtensionHostWire;
    if(!m_Controller && !pending.ownCleanup) return Refused(NoCurrentWindow());
    if(OverviewIsUp() && !pending.ownCleanup) return Refused(NotEditable());
    switch(pending.act.kind){
    case Act::Create: return Create(pending.act, pending.sight);
    case Act::Update: return Update(pending.act, pending.sight);
    case Act::Remove: return Remove(pending.act, pending.ownCleanup);
    case Act::Reload: return Reload(pending.act);
    case Act::Search: return Search(pending.act, pending.sight);
    case Act::Restore: return Restore(pending.sight);
    case Act::Duplicate: return Duplicate(pending.act, pending.sight);
    case Act::Move: return Move(pending.act, pending.sight);
    default: return Refused(QStringLiteral("nothing was asked"));
    }
}

QJsonObject ExtensionHost::Create(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight){
    using namespace ExtensionHostWire;
    ViewNode *anchor = act.opener ? NodeOf(act.opener) : CurrentNode();
    if(act.opener && !anchor) return Refused(NoTab(act.opener));
    View *beside = anchor ? anchor->GetView() : nullptr;
    if(!beside || beside->Extensions() != m_Controller) return Refused(NoCurrentWindow());
    TreeBank *bank = CurrentBank();
    if(!bank) return Refused(NoCurrentWindow());

    QPointer<ExtensionHost> self(this);
    const SharedView made = bank->OpenInNewViewNode(act.url, act.activate, anchor);
    if(!self) return QJsonObject();
    ViewNode *vn = made ? made->GetViewNode() : nullptr;
    if(!vn) return Refused(QStringLiteral("The tab was closed before it could be told of."));
    const qint64 id = static_cast<qint64>(vn->GetSerial());
    ExtensionController *of = made->Extensions();
    const bool ours = of ? of == m_Controller : TreeBank::NetworkSpaceOf(vn) == TreeBank::NetworkSpaceOf(anchor);
    if(!ours){
        Pending cleanup;
        cleanup.host = this;
        cleanup.ownCleanup = true;
        cleanup.act.kind = Act::Remove;
        cleanup.act.tabs << id;
        if(!Line().Push(cleanup))
            qWarning("extension host: no room to close tab %lld, which is not the asking extension's", static_cast<long long>(id));
        DrainSoon(0);
        return Refused(QStringLiteral("The tab could not be made for this extension."));
    }
    return TabAnswer(id, Tabs(), sight);
}

QJsonObject ExtensionHost::Update(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight){
    using namespace ExtensionHostWire;
    ViewNode *vn = act.current ? CurrentNode() : NodeOf(act.tab);
    if(!vn) return Refused(act.current ? NoCurrentWindow() : NoTab(act.tab));
    const qint64 id = static_cast<qint64>(vn->GetSerial());
    if((act.hasUrl || act.hasMuted) && !vn->GetView() && !act.activate)
        return Refused(QStringLiteral("The tab is not loaded."));

    QPointer<ExtensionHost> self(this);
    if(act.activate){
        TreeBank *bank = CurrentBank();
        if(!bank) return Refused(NoCurrentWindow());
        if(bank->GetCurrentViewNode() != vn && !bank->SetCurrent(vn)){
            return self ? Refused(NotEditable()) : QJsonObject();
        }
        if(!self) return QJsonObject();
    }
    if(act.hasUrl){
        vn = NodeOf(id);
        const SharedView view = vn && vn->GetView() ? vn->GetView()->GetThis().lock() : SharedView();
        if(!view) return Refused(NoTab(id));
        view->Load(act.url);
        if(!self) return QJsonObject();
    }
    if(act.hasMuted){
        vn = NodeOf(id);
        const SharedView view = vn && vn->GetView() ? vn->GetView()->GetThis().lock() : SharedView();
        if(!view) return Refused(NoTab(id));
        view->SetAudioMuted(act.muted);
        if(!self) return QJsonObject();
    }
    return TabAnswer(id, Tabs(), sight);
}

QJsonObject ExtensionHost::Search(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight){
    using namespace ExtensionHostWire;
    const SearchEngine engine = Page::PrimarySearchEngine();
    const QString format = engine.isEmpty() ? QString() : engine.at(0);
    const QUrl url = Page::CreateQueryUrl(engine, act.text);
    const QString why = WhyNotSearchable(format, url);
    if(!why.isEmpty()) return Refused(QStringLiteral("search.query: ") + why);

    Act go;
    go.hasUrl = true;
    go.url = url;
    if(act.newTab){
        go.kind = Act::Create;
        go.activate = true;
    } else {
        go.kind = Act::Update;
        go.current = act.current;
        go.tab = act.tab;
    }
    const QJsonObject answer = act.newTab ? Create(go, sight) : Update(go, sight);
    if(answer.isEmpty() || !answer.value(QStringLiteral("ok")).toBool()) return answer;
    return Done();
}

QJsonObject ExtensionHost::Restore(const ExtensionHostWire::Sight &sight){
    using namespace ExtensionHostWire;
    TreeBank *bank = CurrentBank();
    if(!bank || !CurrentNode()) return Refused(NoCurrentWindow());
    ViewNode *trash = TreeBank::GetTrashRoot();
    if(!trash || trash->HasNoChildren()) return Refused(QStringLiteral("There are no recently closed tabs."));
    Node *first = trash->GetFirstChild();
    ViewNode *newest = first ? first->ToViewNode() : nullptr;
    if(!newest) return Refused(NotEditable());
    const qint64 id = static_cast<qint64>(newest->GetSerial());
    const QDateTime when = newest->GetLastUpdateDate();
    const qint64 modified = when.isValid() ? when.toSecsSinceEpoch() : 0;
    QPointer<ExtensionHost> self(this);
    bank->Restore();
    if(!self) return QJsonObject();
    return SessionAnswer(modified, id, Tabs(), sight);
}

QJsonObject ExtensionHost::Duplicate(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight){
    using namespace ExtensionHostWire;
    ViewNode *vn = NodeOf(act.tab);
    if(!vn) return Refused(NoTab(act.tab));
    TreeBank *bank = CurrentBank();
    if(!bank) return Refused(NoCurrentWindow());
    QPointer<ExtensionHost> self(this);
    ViewNode *clone = bank->CloneViewNode(vn);
    if(!self) return QJsonObject();
    if(!TreeBank::IsLive(bank) || !clone) return Refused(NotEditable());
    const qint64 id = static_cast<qint64>(clone->GetSerial());
    return TabAnswer(id, Tabs(), sight);
}

QJsonObject ExtensionHost::Move(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight){
    using namespace ExtensionHostWire;
    QPointer<ExtensionHost> self(this);
    qint64 place = act.index;
    foreach(qint64 id, act.tabs){
        ViewNode *vn = NodeOf(id);
        if(!vn) return Refused(NoTab(id));
        const QList<Tab> tabs = Tabs();
        ViewNode *dir = TreeBank::GetViewRoot();
        int to = -1, from = -1;
        for(int i = 0; i < tabs.size(); i++) if(tabs.at(i).id == id) from = i;
        if(place >= 0 && place < tabs.size()){
            ViewNode *target = NodeOf(tabs.at(place).id);
            ViewNode *parent = target && target->GetParent() ? target->GetParent()->ToViewNode() : nullptr;
            if(parent){
                dir = parent;
                to = parent->ChildrenIndexOf(target);
                const bool across = vn->GetParent() != dir;
                if(across && from >= 0 && place == from + 1) ;
                else if(across && from >= 0 && place == from - 1) to++;
                else if(across && from >= 0 && from < place) to++;
            }
        }
        if(!dir) return Refused(NotEditable());
        for(Node *above = dir; above; above = above->GetParent())
            if(above == vn) return Refused(QStringLiteral("tabs.move: a tab cannot be moved into itself"));
        if(vn->GetParent() == dir){
            const int from = dir->ChildrenIndexOf(vn);
            const int final = to < 0 ? dir->ChildrenLength() - 1 : to;
            if(from != final){
                dir->MoveChild(from, final);
                TreeBank::EmitTreeStructureChanged();
            }
        } else {
            TreeBank::MoveNode(vn, dir, to);
            TreeBank::EmitTreeStructureChanged();
        }
        if(!self) return QJsonObject();
        if(place >= 0) place++;
    }
    if(act.many) return TabsAnswer(act.tabs, Tabs(), sight);
    return TabAnswer(act.tabs.first(), Tabs(), sight);
}

QJsonObject ExtensionHost::Remove(const ExtensionHostWire::Act &act, bool whatever){
    using namespace ExtensionHostWire;
    auto node = [&](qint64 id){
        return whatever ? (id > 0 ? TreeBank::TabOfSerial(static_cast<quint64>(id)) : nullptr) : NodeOf(id);
    };
    foreach(qint64 id, act.tabs)
        if(!node(id)) return Refused(NoTab(id));

    QPointer<ExtensionHost> self(this);
    foreach(qint64 id, act.tabs){
        ViewNode *vn = node(id);
        if(!vn) continue;
        TreeBank *bank = vn->GetView() ? LiveBankOf(vn->GetView()) : CurrentBank();
        if(!bank) return Refused(NoCurrentWindow());
        bank->DeleteNode(vn);
        if(!self) return QJsonObject();
    }
    return Done();
}

QJsonObject ExtensionHost::Reload(const ExtensionHostWire::Act &act){
    using namespace ExtensionHostWire;
    ViewNode *vn = act.current ? CurrentNode() : NodeOf(act.tab);
    if(!vn) return Refused(act.current ? NoCurrentWindow() : NoTab(act.tab));
    const SharedView view = vn->GetView() ? vn->GetView()->GetThis().lock() : SharedView();
    if(!view) return Done();
    QPointer<ExtensionHost> self(this);
    view->TriggerAction(act.bypassCache ? Page::_ReloadAndBypassCache : Page::_Reload);
    return self ? Done() : QJsonObject();
}

