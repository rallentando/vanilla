#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"

#include <QDir>
#include <QCryptographicHash>
#include <QLabel>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QHideEvent>
#include <QTimer>

#include <functional>
#include <memory>
#include <utility>

#include "application.hpp"
#include "extensioncopy.hpp"
#include "extensionhost.hpp"
#include "extensionhostwire.hpp"
#include "extensionui.hpp"
#include "cdpshims.hpp"
#include "devicescale.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QScopedPointer>
#include <QThread>

namespace {

    QString Failure(HRESULT hr){
        return ExtensionController::tr("WebView2 extension operation failed (0x%1).")
            .arg(static_cast<quint32>(hr), 8, 16, QLatin1Char('0'));
    }

    QString ReadItem(ICoreWebView2BrowserExtension *raw, ExtensionItem *item){
        const ComPtr<ICoreWebView2BrowserExtension> extension = raw;
        if(!extension) return Failure(E_POINTER);
        LPWSTR id = nullptr;
        LPWSTR name = nullptr;
        BOOL enabled = FALSE;
        HRESULT hr = extension->get_Id(&id);
        if(SUCCEEDED(hr)) hr = extension->get_Name(&name);
        if(SUCCEEDED(hr)) hr = extension->get_IsEnabled(&enabled);
        if(SUCCEEDED(hr)){
            item->id = QString::fromWCharArray(id);
            item->name = QString::fromWCharArray(name);
            item->enabled = enabled;
        }
        CoTaskMemFree(id);
        CoTaskMemFree(name);
        return FAILED(hr) ? Failure(hr) : QString();
    }

    ExtensionController *ControllerOf(const QString &profileName);
    class EdgeRelayView;

    QString JournalFor(const QString &profileName){
        const QByteArray key =
            (EdgeEnvironment::UserDataFolder() + QLatin1Char('/') + profileName).toUtf8();
        return Application::DataDirectory() + QStringLiteral("extension-registry/")
            + QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex())
            + QStringLiteral(".json");
    }

    QString CopyLedgerFor(const QString &profileName){
        return JournalFor(profileName).replace(QStringLiteral(".json"), QStringLiteral("-copies.json"));
    }
    QString PinnedCopy(const ExtensionManifest &manifest, QString *copy = nullptr, bool *keyless = nullptr){
        const ExtensionCopy::Made making = ExtensionCopy::Make
            (manifest.folder, ExtensionHost::CopyRoot(), manifest.id, ExtensionHost::KeyFor(manifest.id), ExtensionCopy::UiLocale());
        const QString made = making.path;
        if(copy) *copy = made == manifest.folder ? QString() : made;
        if(keyless) *keyless = made != manifest.folder && making.keyless;
        return made == manifest.folder ? made : ExtensionCopy::Pin(ExtensionHost::CopyRoot(), manifest.id, made);
    }

    class EdgeAsk : public ExtensionHost::Ask {
    public:
        EdgeAsk(ICoreWebView2WebResourceRequestedEventArgs *args, ICoreWebView2Deferral *deferral,
                ICoreWebView2Environment *environment, ICoreWebView2WebResourceRequest *request,
                const QUrl &url, QObject *owner)
            : m_Args(args), m_Deferral(deferral), m_Environment(environment)
            , m_Url(url), m_Owner(owner), m_Thread(QThread::currentThread()), m_Completed(false)
        {
            LPWSTR method = nullptr;
            if(SUCCEEDED(request->get_Method(&method)) && method){
                m_Method = QString::fromWCharArray(method).toLatin1();
                CoTaskMemFree(method);
            }
            ComPtr<ICoreWebView2HttpRequestHeaders> headers;
            if(SUCCEEDED(request->get_Headers(&headers)) && headers){
                const char *names[] = { ExtensionHostWire::KEY_HEADER, ExtensionHostWire::CALL_HEADER,
                                        ExtensionHostWire::NONCE_HEADER, "Origin" };
                for(const char *name : names){
                    LPWSTR value = nullptr;
                    const QString wide = QString::fromLatin1(name);
                    if(SUCCEEDED(headers->GetHeader(reinterpret_cast<LPCWSTR>(wide.utf16()), &value)) && value){
                        m_Headers.insert(QByteArray(name), QString::fromWCharArray(value).toUtf8());
                        CoTaskMemFree(value);
                    }
                }
            }
            m_Origin = QString::fromUtf8(m_Headers.value(QByteArrayLiteral("Origin")));
        }
        ~EdgeAsk() Q_DECL_OVERRIDE { Answer(403, L"Forbidden", QByteArray(), QByteArray()); }

        QByteArray Method() const Q_DECL_OVERRIDE { return m_Method; }
        QUrl Url() const Q_DECL_OVERRIDE { return m_Url; }
        QUrl Initiator() const Q_DECL_OVERRIDE {
            return m_Origin.isEmpty() || m_Origin == QStringLiteral("null") ? QUrl() : QUrl(m_Origin);
        }
        QMap<QByteArray, QByteArray> Headers() const Q_DECL_OVERRIDE { return m_Headers; }
        bool Body(qint64, QByteArray *) Q_DECL_OVERRIDE { return false; }
        bool Alive() const Q_DECL_OVERRIDE { return !m_Completed && !m_Owner.isNull(); }
        void Reply(const QJsonObject &answer) Q_DECL_OVERRIDE {
            Answer(200, L"OK", QByteArrayLiteral("application/json"),
                   QJsonDocument(answer).toJson(QJsonDocument::Compact));
        }
        void ReplyNothing() Q_DECL_OVERRIDE { Answer(200, L"OK", QByteArrayLiteral("text/plain"), QByteArray()); }
        void Fail() Q_DECL_OVERRIDE { Answer(403, L"Forbidden", QByteArray(), QByteArray()); }

        static QString CorsHeaders(const QString &origin){
            return QStringLiteral("Access-Control-Allow-Origin: ") + (origin.isEmpty() ? QStringLiteral("null") : origin)
                + QStringLiteral("\r\nAccess-Control-Allow-Headers: X-Vanilla-Key, X-Vanilla-Call, X-Vanilla-Nonce")
                + QStringLiteral("\r\nAccess-Control-Allow-Methods: POST")
                + QStringLiteral("\r\nAccess-Control-Max-Age: 600")
                + QStringLiteral("\r\nVary: Origin");
        }

    private:
        void Answer(int status, const wchar_t *reason, const QByteArray &type, const QByteArray &body){
            if(m_Completed) return;
            m_Completed = true;
            Q_ASSERT(QThread::currentThread() == m_Thread);
            ComPtr<ICoreWebView2WebResourceRequestedEventArgs> args = m_Args;
            ComPtr<ICoreWebView2Deferral> deferral = m_Deferral;
            ComPtr<ICoreWebView2Environment> environment = m_Environment;
            m_Args.Reset();
            m_Deferral.Reset();
            m_Environment.Reset();
            if(args && environment){
                ComPtr<IStream> stream;
                if(!body.isEmpty())
                    stream.Attach(SHCreateMemStream(reinterpret_cast<const BYTE*>(body.constData()),
                                                    static_cast<UINT>(body.size())));
                QString headers = CorsHeaders(m_Origin);
                if(!type.isEmpty()) headers = QStringLiteral("Content-Type: ") + QString::fromLatin1(type) + QStringLiteral("\r\n") + headers;
                ComPtr<ICoreWebView2WebResourceResponse> response;
                if(SUCCEEDED(environment->CreateWebResourceResponse
                             (stream.Get(), status, reason, reinterpret_cast<PCWSTR>(headers.utf16()), &response)) && response)
                    args->put_Response(response.Get());
            }
            if(deferral) deferral->Complete();
        }

        ComPtr<ICoreWebView2WebResourceRequestedEventArgs> m_Args;
        ComPtr<ICoreWebView2Deferral> m_Deferral;
        ComPtr<ICoreWebView2Environment> m_Environment;
        QUrl m_Url;
        QByteArray m_Method;
        QMap<QByteArray, QByteArray> m_Headers;
        QString m_Origin;
        QPointer<QObject> m_Owner;
        QThread *m_Thread;
        bool m_Completed;
    };

    class EdgeExtensions : public ExtensionController {
    public:
        EdgeExtensions(QObject *parent, const QString &name, const QString &space, ICoreWebView2Profile7 *profile)
            : ExtensionController(parent, JournalFor(name))
            , m_Profile(profile)
            , m_Ledger(CopyLedgerFor(name))
        {
            setProperty("edgeExtensionProfile", name);
            m_ProfileName = name;
            SetMenusMirrored(true);
            SetMenusFile(Application::StateDirectory() + ExtensionUi::MenusFileName(QStringLiteral("edge:") + name));
            connect(this, &ExtensionController::Changed, this, [this](){ ReconcileRelays(); });
            ReadLedger();
            if(ExtensionHost::ShimsOn()) ExtensionHost::Install(this, this, space);
            Start();
        }

        QSet<QString> ShimmedIds() const Q_DECL_OVERRIDE {
            QSet<QString> ids;
            if(!ExtensionHost::ShimsOn()) return ids;
            foreach(const ExtensionRow &row, Rows())
                if(row.loaded && row.enabled && m_Copies.value(row.manifest.id).copied) ids.insert(row.manifest.id);
            return ids;
        }
        bool HasKeyedShims(const QString &id) const Q_DECL_OVERRIDE {
            const Copy copy = m_Copies.value(id);
            return ExtensionHost::ShimsOn() && copy.copied && !copy.keyless;
        }

    protected:
        void Snapshot(SnapshotDone done) Q_DECL_OVERRIDE {
            List(done);
        }

        void Add(const ExtensionManifest &manifest, ItemDone done) Q_DECL_OVERRIDE {
            QPointer<EdgeExtensions> self(this);
            const ComPtr<ICoreWebView2Profile7> profile = m_Profile;
            QString from = manifest.folder;
            Copy copy;
            if(ExtensionHost::ShimsOn()){
                const ExtensionCopy::Made made = ExtensionCopy::Make
                    (manifest.folder, ExtensionHost::CopyRoot(), manifest.id, ExtensionHost::KeyFor(manifest.id), ExtensionCopy::UiLocale());
                copy.copied = made.path != manifest.folder;
                from = copy.copied ? ExtensionCopy::Pin(ExtensionHost::CopyRoot(), manifest.id, made.path) : made.path;
                if(copy.copied) copy.loaded = made.path;
                copy.keyless = made.keyless;
                Note(manifest.path, CopyNote(made.note, made.detail, made.withheld));
            } else {
                Note(manifest.path, QString());
            }
            copy.path = from;
            const bool known = m_Copies.contains(manifest.id);
            const Copy before = m_Copies.value(manifest.id);
            m_Copies.insert(manifest.id, copy);
            if(!WriteLedger()){
                if(known) m_Copies.insert(manifest.id, before); else m_Copies.remove(manifest.id);
                done(ExtensionItem(), ExtensionController::tr("The copy ledger could not be written: %1")
                     .arg(QDir::toNativeSeparators(m_Ledger)));
                return;
            }
            const QString path = QDir::toNativeSeparators(from);
            const HRESULT hr = profile->AddBrowserExtension(
                reinterpret_cast<LPCWSTR>(path.utf16()),
                Callback<ICoreWebView2ProfileAddBrowserExtensionCompletedHandler>(
                    [self, profile, done](HRESULT hr, ICoreWebView2BrowserExtension *raw) -> HRESULT {
                        if(!self) return S_OK;
                        const ComPtr<ICoreWebView2BrowserExtension> extension = raw;
                        ExtensionItem item;
                        const QString error = FAILED(hr) ? Failure(hr) : ReadItem(extension.Get(), &item);
                        if(!self) return S_OK;
                        if(error.isEmpty()) self->m_Items.insert(item.id, extension);
                        done(item, error);
                        return S_OK;
                    }).Get());
            if(FAILED(hr) && self) done(ExtensionItem(), Failure(hr));
        }

        void Remove(const QString &id, Done done) Q_DECL_OVERRIDE {
            m_Copies.remove(id);
            WriteLedger();
            RemoveFromEngine(id, done);
        }

        void Enable(const QString &id, bool enabled, ItemDone done) Q_DECL_OVERRIDE {
            Operate(EdgeExtensionOperation::Kind::Enable, id,
                    [id, enabled](ICoreWebView2BrowserExtension *extension, Answer answer) -> HRESULT {
                        const ComPtr<ICoreWebView2BrowserExtension> object = extension;
                        return extension->Enable(
                            enabled,
                            Callback<ICoreWebView2BrowserExtensionEnableCompletedHandler>(
                                [object, id, enabled, answer](HRESULT hr) -> HRESULT {
                                    ExtensionItem item;
                                    QString error = FAILED(hr) ? Failure(hr) : ReadItem(object.Get(), &item);
                                    if(error.isEmpty() && (item.id != id || item.enabled != enabled))
                                        error = ExtensionController::tr("The engine did not apply the requested enabled state.");
                                    answer(item, error);
                                    return S_OK;
                                }).Get());
                    },
                    done);
        }

        void RemoveFromEngine(const QString &id, Done done){
            Operate(EdgeExtensionOperation::Kind::Remove, id,
                    [](ICoreWebView2BrowserExtension *extension, Answer answer) -> HRESULT {
                        const ComPtr<ICoreWebView2BrowserExtension> object = extension;
                        return extension->Remove(
                            Callback<ICoreWebView2BrowserExtensionRemoveCompletedHandler>(
                                [object, answer](HRESULT hr) -> HRESULT {
                                    answer(ExtensionItem(), FAILED(hr) ? Failure(hr) : QString());
                                    return S_OK;
                                }).Get());
                    },
                    [done](ExtensionItem, QString error){ done(error);});
        }

    private:
        using Answer = std::function<void(ExtensionItem item, QString error)>;
        using Call = std::function<HRESULT(ICoreWebView2BrowserExtension *extension, Answer answer)>;

        struct Attempt {
            Attempt(EdgeExtensionOperation::Kind kind, const QString &id, Call call, ItemDone report)
                : state(kind), id(id), call(std::move(call)), report(std::move(report)) {}
            EdgeExtensionOperation state;
            QString id;
            Call call;
            ItemDone report;
            QString firstError;
        };

        static void Gone(const ItemDone &report){
            ExtensionItem item;
            item.gone = true;
            report(item, ExtensionController::tr("The extension is no longer loaded."));
        }

        void List(SnapshotDone done){
            QPointer<EdgeExtensions> self(this);
            const ComPtr<ICoreWebView2Profile7> profile = m_Profile;
            const HRESULT hr = profile->GetBrowserExtensions(
                Callback<ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler>(
                    [self, profile, done](HRESULT hr, ICoreWebView2BrowserExtensionList *raw) -> HRESULT {
                        if(!self) return S_OK;
                        const ComPtr<ICoreWebView2BrowserExtensionList> list = raw;
                        QList<ExtensionItem> items;
                        QMap<QString, ComPtr<ICoreWebView2BrowserExtension>> objects;
                        UINT count = 0;
                        QString error;
                        if(SUCCEEDED(hr)) hr = list ? list->get_Count(&count) : E_POINTER;
                        for(UINT i = 0; SUCCEEDED(hr) && i < count; ++i){
                            ComPtr<ICoreWebView2BrowserExtension> extension;
                            hr = list->GetValueAtIndex(i, &extension);
                            if(FAILED(hr)) break;
                            ExtensionItem item;
                            error = ReadItem(extension.Get(), &item);
                            if(!error.isEmpty()) break;
                            objects.insert(item.id, extension);
                            items.append(item);
                        }
                        if(error.isEmpty() && FAILED(hr)) error = Failure(hr);
                        if(!self) return S_OK;
                        if(!error.isEmpty()){
                            done(QList<ExtensionItem>(), error);
                            return S_OK;
                        }
                        self->m_Items = objects;
                        self->Migrate(items, done);
                        return S_OK;
                    }).Get());
            if(FAILED(hr) && self) done(QList<ExtensionItem>(), Failure(hr));
        }

        void Migrate(QList<ExtensionItem> items, SnapshotDone done){
            QStringList stale;
            QHash<QString, QString> behind;
            {
                QHash<QString, QString> current, copies;
                m_CopyKeyless.clear();
                foreach(const QString &path, Application::GlobalSettings().value(QStringLiteral("network/@Extensions")).toStringList()){
                    const ExtensionManifest manifest = ExtensionManifest::Read(path);
                    if(!manifest.error.isEmpty()) continue;
                    QString copy;
                    bool keyless = false;
                    current.insert(manifest.id, ExtensionHost::ShimsOn() ? PinnedCopy(manifest, &copy, &keyless) : manifest.folder);
                    copies.insert(manifest.id, copy);
                    m_CopyKeyless.insert(manifest.id, keyless);
                }
                bool rewritten = false;
                const QSet<QString> owned(OwnedByPath().cbegin(), OwnedByPath().cend());
                foreach(const ExtensionItem &item, items){
                    if(!current.contains(item.id) || !owned.contains(item.id)) continue;
                    const Copy copy = m_Copies.value(item.id);
                    const QString pinned = ExtensionCopy::PinnedPath(ExtensionHost::CopyRoot(), item.id);
                    if(ExtensionCopy::Held(ExtensionHost::ShimsOn(), copy.path, pinned, current.value(item.id))) continue;
                    if(ExtensionCopy::Stale(ExtensionHost::ShimsOn(), m_Copies.contains(item.id), copy.path, current.value(item.id)))
                        stale.append(item.id);
                    else if(item.enabled && ExtensionCopy::Behind(copy.path, pinned, copy.loaded, copies.value(item.id)))
                        behind.insert(item.id, copies.value(item.id));
                    if(stale.contains(item.id) || !m_Copies.contains(item.id)) continue;
                    const bool keyless = ExtensionCopy::KeylessOfLoaded(copy.keyless, copy.loaded, copies.value(item.id),
                                                                        m_CopyKeyless.value(item.id));
                    if(keyless != copy.keyless){
                        m_Copies[item.id].keyless = keyless;
                        rewritten = true;
                    }
                }
                if(rewritten) WriteLedger();
            }
            if(stale.isEmpty()){
                Refresh(items, behind, done);
                return;
            }
            QPointer<EdgeExtensions> self(this);
            const QString id = stale.first();
            RemoveFromEngine(id, [self, items, id, done, behind](QString error){
                if(!self) return;
                QList<ExtensionItem> rest = items;
                for(int i = rest.size() - 1; i >= 0; i--)
                    if(rest.at(i).id == id) rest.removeAt(i);
                if(!error.isEmpty()){
                    self->Refresh(items, behind, done);
                    return;
                }
                self->m_Copies.remove(id);
                self->WriteLedger();
                self->Migrate(rest, done);
            });
        }

        void Refresh(QList<ExtensionItem> items, QHash<QString, QString> behind, SnapshotDone done){
            if(behind.isEmpty()){
                done(items, QString());
                return;
            }
            const QString id = behind.constBegin().key(), copy = behind.constBegin().value();
            behind.remove(id);
            QPointer<EdgeExtensions> self(this);
            const std::function<void(QList<ExtensionItem>)> next = [self, behind, done](QList<ExtensionItem> now){
                if(self) self->Refresh(now, behind, done);
            };
            const std::function<void(const ExtensionItem*)> settle = [self, items, id, copy, next](const ExtensionItem *item){
                QList<ExtensionItem> now = items;
                for(int i = 0; i < now.size(); i++){
                    if(now.at(i).id != id) continue;
                    if(item) now[i] = *item; else now[i].enabled = false;
                }
                if(item && self){
                    Copy &entry = self->m_Copies[id];
                    entry.loaded = copy;
                    entry.keyless = ExtensionCopy::KeylessOfLoaded(entry.keyless, copy, copy,
                                                                   self->m_CopyKeyless.value(id, entry.keyless));
                    self->WriteLedger();
                }
                next(now);
            };
            const ComPtr<ICoreWebView2BrowserExtension> object = m_Items.value(id);
            if(!object){
                next(items);
                return;
            }
            const HRESULT hr = object->Enable(
                FALSE,
                Callback<ICoreWebView2BrowserExtensionEnableCompletedHandler>(
                    [self, object, id, items, next, settle](HRESULT hr) -> HRESULT {
                        if(!self) return S_OK;
                        if(FAILED(hr)){
                            next(items);
                            return S_OK;
                        }
                        const HRESULT again = object->Enable(
                            TRUE,
                            Callback<ICoreWebView2BrowserExtensionEnableCompletedHandler>(
                                [self, object, id, settle](HRESULT hr) -> HRESULT {
                                    if(!self) return S_OK;
                                    ExtensionItem item;
                                    const QString error = FAILED(hr) ? Failure(hr) : ReadItem(object.Get(), &item);
                                    if(!self) return S_OK;
                                    settle(error.isEmpty() && item.id == id && item.enabled ? &item : nullptr);
                                    return S_OK;
                                }).Get());
                        if(FAILED(again)) settle(nullptr);
                        return S_OK;
                    }).Get());
            if(FAILED(hr)) next(items);
        }

        void Operate(EdgeExtensionOperation::Kind kind, const QString &id, Call call, ItemDone report){
            const ComPtr<ICoreWebView2BrowserExtension> extension = m_Items.value(id);
            if(!extension){
                if(kind == EdgeExtensionOperation::Kind::Remove) report(ExtensionItem(), QString());
                else Gone(report);
                return;
            }
            Issue(std::make_shared<Attempt>(kind, id, std::move(call), std::move(report)),
                  extension, false);
        }

        void Issue(std::shared_ptr<Attempt> attempt, ComPtr<ICoreWebView2BrowserExtension> extension,
                   bool retry){
            QPointer<EdgeExtensions> self(this);
            const HRESULT hr = attempt->call(
                extension.Get(),
                [self, attempt, retry](ExtensionItem item, QString error){
                    if(self) self->Answered(attempt, retry, item, error);
                });
            if(FAILED(hr) && self) Answered(attempt, retry, ExtensionItem(), Failure(hr));
        }

        void Answered(std::shared_ptr<Attempt> attempt, bool retry,
                      const ExtensionItem &item, const QString &error){
            const bool succeeded = error.isEmpty();
            if(!retry && !succeeded) attempt->firstError = error;
            Follow(attempt,
                   retry ? attempt->state.RetryResult(succeeded) : attempt->state.First(succeeded),
                   item, error);
        }

        void Follow(std::shared_ptr<Attempt> attempt, EdgeExtensionOperation::Effect effect,
                    const ExtensionItem &item, const QString &error){
            switch(effect){
            case EdgeExtensionOperation::Effect::ReportSuccess:
                if(attempt->state.GetKind() == EdgeExtensionOperation::Kind::Remove)
                    m_Items.remove(attempt->id);
                attempt->report(item, QString());
                break;
            case EdgeExtensionOperation::Effect::ReportFailure:
                attempt->report(ExtensionItem(), error);
                break;
            case EdgeExtensionOperation::Effect::ReportGone:
                m_Items.remove(attempt->id);
                Gone(attempt->report);
                break;
            case EdgeExtensionOperation::Effect::Snapshot: {
                QPointer<EdgeExtensions> self(this);
                List([self, attempt](QList<ExtensionItem>, QString listError){
                    if(!self) return;
                    const ComPtr<ICoreWebView2BrowserExtension> fresh = self->m_Items.value(attempt->id);
                    const EdgeExtensionOperation::Effect next =
                        attempt->state.SnapshotResult(listError.isEmpty(), fresh != nullptr);
                    if(next == EdgeExtensionOperation::Effect::Retry) self->Issue(attempt, fresh, true);
                    else self->Follow(attempt, next, ExtensionItem(), attempt->firstError);
                });
                break;
            }
            case EdgeExtensionOperation::Effect::Retry:
            case EdgeExtensionOperation::Effect::None:
                break;
            }
        }

    private:
        struct Copy {
            bool copied = false;
            bool keyless = false;
            QString path;
            QString loaded;
        };
        void ReadLedger(){
            QFile file(m_Ledger);
            if(!file.open(QIODevice::ReadOnly)) return;
            const QJsonObject all = QJsonDocument::fromJson(file.readAll()).object();
            for(auto it = all.constBegin(); it != all.constEnd(); ++it){
                const QJsonObject entry = it.value().toObject();
                Copy copy;
                copy.copied = entry.value(QStringLiteral("copied")).toBool();
                copy.keyless = entry.value(QStringLiteral("keyless")).toBool();
                copy.path = entry.value(QStringLiteral("path")).toString();
                copy.loaded = entry.value(QStringLiteral("loaded")).toString();
                m_Copies.insert(it.key(), copy);
            }
        }
        bool WriteLedger(){
            QJsonObject all;
            for(auto it = m_Copies.constBegin(); it != m_Copies.constEnd(); ++it){
                QJsonObject entry;
                entry.insert(QStringLiteral("copied"), it.value().copied);
                entry.insert(QStringLiteral("keyless"), it.value().keyless);
                entry.insert(QStringLiteral("path"), it.value().path);
                if(!it.value().loaded.isEmpty()) entry.insert(QStringLiteral("loaded"), it.value().loaded);
                all.insert(it.key(), entry);
            }
            const QByteArray bytes = QJsonDocument(all).toJson(QJsonDocument::Compact);
            QSaveFile file(m_Ledger);
            return QDir().mkpath(QFileInfo(m_Ledger).absolutePath()) && file.open(QIODevice::WriteOnly)
                && file.write(bytes) == bytes.size() && file.commit();
        }

        ComPtr<ICoreWebView2Profile7> m_Profile;
        QMap<QString, ComPtr<ICoreWebView2BrowserExtension>> m_Items;
        QString m_Ledger;
        QString m_ProfileName;

        void ReconcileRelays();
        QHash<QString, EdgeRelayView*> m_Relays;
    public:
        bool SendToWorker(const QString &id, const QJsonArray &args) Q_DECL_OVERRIDE;
    private:
        bool m_Reconciling = false;
        bool m_ReconcileAgain = false;
        QMap<QString, Copy> m_Copies;
        QHash<QString, bool> m_CopyKeyless;
    };

    class EdgeExtensionView : public QWidget {
    public:
        EdgeExtensionView(const QString &profileName, const QUrl &url,
                          const ExtensionHostWire::Sight &sight, bool fits, WeakView source, QWidget *parent,
                          bool docked = false)
            : QWidget(parent)
            , m_ProfileName(profileName)
            , m_NamesTab(sight.Any())
            , m_Sight(sight)
            , m_Fits(fits)
            , m_Docked(docked)
            , m_Source(source)
            , m_Origin(url)
            , m_HostToken(new QObject())
        {
            setAttribute(Qt::WA_NativeWindow);
        }
        ~EdgeExtensionView() Q_DECL_OVERRIDE {
            Shutdown();
        }

        void Initialize(){
            QPointer<EdgeExtensionView> self(this);
            const HWND window = reinterpret_cast<HWND>(winId());
            const QString profileName = m_ProfileName;
            const QUrl url = m_Origin;
            ComPtr<ICoreWebView2Environment10> environment;
            ComPtr<ICoreWebView2ControllerOptions> options;
            const ComPtr<ICoreWebView2Environment> base = EdgeEnvironment::Instance()->GetEnvironment();
            HRESULT hr = base ? base.As(&environment) : E_POINTER;
            if(SUCCEEDED(hr)) hr = environment->CreateCoreWebView2ControllerOptions(&options);
            if(SUCCEEDED(hr)) hr = options->put_ProfileName(reinterpret_cast<LPCWSTR>(profileName.utf16()));
            if(SUCCEEDED(hr)) hr = options->put_IsInPrivateModeEnabled(FALSE);
            if(!self || m_State.IsClosed()) return;
            if(FAILED(hr)){
                Fail(hr);
                return;
            }
            hr = environment->CreateCoreWebView2ControllerWithOptions(
                window, options.Get(),
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [self, environment, profileName, url](HRESULT hr, ICoreWebView2Controller *controller) -> HRESULT {
                        return self ? self->Adopt(hr, controller, profileName, url)
                                    : Discard(controller);
                    }).Get());
            if(!self || m_State.IsClosed()) return;
            if(FAILED(hr)) Fail(hr);
        }

    protected:
        void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE {
            QWidget::resizeEvent(ev);
            Bounds();
        }
        void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE {
            QPointer<EdgeExtensionView> self(this);
            if(m_Docked){
                if(const ComPtr<ICoreWebView2Controller> controller = m_Controller) controller->put_IsVisible(FALSE);
            } else {
                Shutdown();
            }
            if(self) QWidget::hideEvent(ev);
        }
        void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE {
            QWidget::showEvent(ev);
            if(!m_Docked) return;
            QPointer<EdgeExtensionView> self(this);
            if(const ComPtr<ICoreWebView2Controller> controller = m_Controller) controller->put_IsVisible(TRUE);
            if(self) Bounds();
        }
        void moveEvent(QMoveEvent *ev) Q_DECL_OVERRIDE {
            QWidget::moveEvent(ev);
            if(!m_Docked) return;
            if(const ComPtr<ICoreWebView2Controller> controller = m_Controller) controller->NotifyParentWindowPositionChanged();
        }
        bool event(QEvent *ev) Q_DECL_OVERRIDE {
            if(m_Docked && ev->type() == View::SidePanelShutdownEvent()){
                Shutdown();
                return true;
            }
            if(m_Docked && ev->type() == View::SidePanelNavigate::Type()){
                const QUrl url = static_cast<View::SidePanelNavigate*>(ev)->Url();
                if(m_State.IsClosed() || !IsOwnOrigin(url)) return true;
                ev->accept();
                m_Origin = url;
                if(m_Web) Navigate(url);
                return true;
            }
            return QWidget::event(ev);
        }
        void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
            QWidget::focusInEvent(ev);
            const ComPtr<ICoreWebView2Controller> controller = m_Controller;
            if(controller) controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }

    private:
        static HRESULT Discard(ICoreWebView2Controller *controller){
            if(controller) controller->Close();
            return S_OK;
        }

        HRESULT Adopt(HRESULT hr, ICoreWebView2Controller *raw,
                      const QString &profileName, const QUrl &url){
            if(m_State.IsClosed()) return Discard(raw);
            QPointer<EdgeExtensionView> self(this);
            const ComPtr<ICoreWebView2Controller> controller = raw;
            if(FAILED(hr) || !controller){
                Discard(controller.Get());
                if(self && !m_State.IsClosed()) Fail(FAILED(hr) ? hr : E_POINTER);
                return S_OK;
            }
            ComPtr<ICoreWebView2> web;
            ComPtr<ICoreWebView2_13> web13;
            ComPtr<ICoreWebView2Profile> profile;
            LPWSTR name = nullptr;
            BOOL privateMode = TRUE;
            hr = controller->get_CoreWebView2(&web);
            if(SUCCEEDED(hr)) hr = web.As(&web13);
            if(SUCCEEDED(hr)) hr = web13->get_Profile(&profile);
            if(SUCCEEDED(hr)) hr = profile->get_ProfileName(&name);
            if(SUCCEEDED(hr)) hr = profile->get_IsInPrivateModeEnabled(&privateMode);
            const bool matches =
                SUCCEEDED(hr) && !privateMode && QString::fromWCharArray(name) == profileName;
            CoTaskMemFree(name);
            if(!self) return Discard(controller.Get());
            if(m_State.IsClosed() || !matches){
                controller->Close();
                if(self && !m_State.IsClosed()) Fail(FAILED(hr) ? hr : E_UNEXPECTED);
                return S_OK;
            }
            m_Controller = controller;
            m_Web = web;

            {
                EventRegistrationToken resourceToken{};
                const bool filtered = SUCCEEDED(web->AddWebResourceRequestedFilter
                    (L"vanilla-extension://*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL));
                if(!self || m_State.IsClosed()) return S_OK;
                QPointer<QObject> owner(m_HostToken.data());
                const QString profile = profileName;
                const bool hooked = filtered && SUCCEEDED(web->add_WebResourceRequested(
                    Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                        [self, owner, profile](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs *args) -> HRESULT {
                            if(!self || self->m_State.IsClosed() || !args) return S_OK;
                            ComPtr<ICoreWebView2WebResourceRequest> request;
                            if(FAILED(args->get_Request(&request)) || !request) return S_OK;
                            LPWSTR uri = nullptr;
                            QUrl url;
                            if(SUCCEEDED(request->get_Uri(&uri)) && uri){
                                url = QUrl(QString::fromWCharArray(uri));
                                CoTaskMemFree(uri);
                            }
                            if(url.scheme() != QLatin1String(ExtensionHostWire::SCHEME)) return S_OK;
                            return EdgeAnswerExtensionHost(ControllerOf(profile), 0, owner.data(),
                                                           args, request.Get(), url);
                        }).Get(), &resourceToken));
                if(!self || m_State.IsClosed()){
                    if(hooked) web->remove_WebResourceRequested(resourceToken);
                    return S_OK;
                }
                m_ResourceToken = resourceToken;
                m_ResourceHooked = hooked;
            }

            EventRegistrationToken titleToken{};
            const bool titleHooked = SUCCEEDED(web->add_DocumentTitleChanged(
                Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
                    [self](ICoreWebView2 *sender, IUnknown *) -> HRESULT {
                        if(!self || self->m_State.IsClosed()) return S_OK;
                        LPWSTR title = nullptr;
                        if(SUCCEEDED(sender->get_DocumentTitle(&title)) && self && !self->m_State.IsClosed())
                            self->setWindowTitle(QString::fromWCharArray(title));
                        CoTaskMemFree(title);
                        return S_OK;
                    }).Get(), &titleToken));
            if(!self || m_State.IsClosed()){
                if(titleHooked) web->remove_DocumentTitleChanged(titleToken);
                return S_OK;
            }
            m_TitleToken = titleToken;
            m_TitleHooked = titleHooked;

            EventRegistrationToken acceleratorToken{};
            hr = controller->add_AcceleratorKeyPressed(
                Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                    [self](ICoreWebView2Controller *, ICoreWebView2AcceleratorKeyPressedEventArgs *raw) -> HRESULT {
                        const ComPtr<ICoreWebView2AcceleratorKeyPressedEventArgs> args = raw;
                        if(!self || self->m_State.IsClosed() || !args) return S_OK;
                        COREWEBVIEW2_KEY_EVENT_KIND kind;
                        UINT key = 0;
                        COREWEBVIEW2_PHYSICAL_KEY_STATUS status{};
                        if(self->m_Docked) return S_OK;
                        if(FAILED(args->get_KeyEventKind(&kind)) || FAILED(args->get_VirtualKey(&key)) ||
                           key != VK_ESCAPE || kind != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN) return S_OK;
                        args->put_Handled(TRUE);
                        if(FAILED(args->get_PhysicalKeyStatus(&status)) || status.WasKeyDown ||
                           !self || self->m_CloseQueued) return S_OK;
                        self->m_CloseQueued = true;
                        QTimer::singleShot(0, self.data(), [self](){
                            if(self && !self->m_State.IsClosed()) self->window()->close();
                        });
                        return S_OK;
                    }).Get(), &acceleratorToken);
            if(!self || m_State.IsClosed()){
                if(SUCCEEDED(hr)) controller->remove_AcceleratorKeyPressed(acceleratorToken);
                return S_OK;
            }
            if(FAILED(hr)){
                Fail(hr);
                return S_OK;
            }
            m_AcceleratorToken = acceleratorToken;
            m_AcceleratorHooked = true;

            if(m_NamesTab){
                EventRegistrationToken messageToken{};
                hr = web->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [self](ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *raw) -> HRESULT {
                            const ComPtr<ICoreWebView2WebMessageReceivedEventArgs> args = raw;
                            if(!self || self->m_State.IsClosed() || !args) return S_OK;
                            LPWSTR json = nullptr;
                            LPWSTR source = nullptr;
                            const bool got = SUCCEEDED(args->get_WebMessageAsJson(&json)) && json &&
                                             SUCCEEDED(args->get_Source(&source)) && source;
                            const QString text = got ? QString::fromWCharArray(json) : QString();
                            const QUrl from = got ? QUrl(QString::fromWCharArray(source)) : QUrl();
                            CoTaskMemFree(json);
                            CoTaskMemFree(source);
                            if(!got || !self || self->m_State.IsClosed() || !self->IsOwnOrigin(from)) return S_OK;
                            const EdgeExtensionTabRequest request = EdgeExtensionTabRequest::Parse(text);
                            if(!request.IsValid()) return S_OK;
                            const QUrl arrival = self->SourceOfView();
                            if(!self || self->m_State.IsClosed()) return S_OK;
                            QTimer::singleShot(0, self.data(), [self, request, arrival](){
                                if(self) self->Answer(request, arrival);
                            });
                            return S_OK;
                        }).Get(), &messageToken);
                if(!self || m_State.IsClosed()){
                    if(SUCCEEDED(hr)) web->remove_WebMessageReceived(messageToken);
                    return S_OK;
                }
                if(FAILED(hr)){
                    Fail(hr);
                    return S_OK;
                }
                m_MessageToken = messageToken;
                m_MessageHooked = true;
            }

            Bounds();
            if(!self || m_State.IsClosed()) return S_OK;
            controller->put_IsVisible(m_Docked && !isVisible() ? FALSE : TRUE);
            if(!self || m_State.IsClosed()) return S_OK;
            if(m_NamesTab){
                const QString script = View::EdgeExtensionTabQueryJsCode();
                hr = web->AddScriptToExecuteOnDocumentCreated(
                    reinterpret_cast<LPCWSTR>(script.utf16()),
                    Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                        [self, url](HRESULT result, LPCWSTR) -> HRESULT {
                            if(self) self->ScriptRegistered(result, url);
                            return S_OK;
                        }).Get());
                if(!self || m_State.IsClosed()) return S_OK;
                if(FAILED(hr)){
                    Fail(hr);
                    return S_OK;
                }
            } else {
                Navigate(url);
                if(!self || m_State.IsClosed()) return S_OK;
            }
            if(isVisible() && hasFocus())
                controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
            return S_OK;
        }

        void ScriptRegistered(HRESULT result, const QUrl &url){
            switch(m_State.ScriptRegistered(SUCCEEDED(result))){
            case EdgeExtensionPageState::Effect::Navigate: Navigate(url); break;
            case EdgeExtensionPageState::Effect::Fail: Fail(result); break;
            default: break;
            }
        }

        void Navigate(const QUrl &url){
            const ComPtr<ICoreWebView2> web = m_Web;
            if(!web) return;
            QPointer<EdgeExtensionView> self(this);
            const QString address = (m_Docked ? m_Origin : url).toString();
            const HRESULT hr = web->Navigate(reinterpret_cast<LPCWSTR>(address.utf16()));
            if(!self || m_State.IsClosed()) return;
            if(FAILED(hr)){ Fail(hr); return; }
            if(m_Fits && !m_FitAsked){
                m_FitAsked = true;
                for(const int milliseconds : POPUP_FIT_TIMES)
                    QTimer::singleShot(milliseconds, this, [this](){ Fit(); });
            }
        }

        void Fit(){
            const ComPtr<ICoreWebView2> web = m_Web;
            if(!web || m_State.IsClosed()) return;
            QPointer<EdgeExtensionView> self(this);
            const QString script = View::ExtensionPopupSizeJsCode();
            web->ExecuteScript(reinterpret_cast<LPCWSTR>(script.utf16()),
                Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                    [self](HRESULT result, LPCWSTR json) -> HRESULT {
                        if(!self || self->m_State.IsClosed() || FAILED(result) || !json) return S_OK;
                        const QSize size = ExtensionUi::PopupSizeOf(QString::fromWCharArray(json), self->devicePixelRatioF(),
                                                                         DeviceScale::FromDpi(96, self->logicalDpiY()) / 96.0);
                        if(!size.isValid()) return S_OK;
                        self->m_FitWanted = size;
                        if(self->m_FitQueued) return S_OK;
                        self->m_FitQueued = true;
                        QTimer::singleShot(0, self.data(), [self](){
                            if(!self) return;
                            self->m_FitQueued = false;
                            const QSize wanted = self->m_FitWanted;
                            if(self->m_State.IsClosed()) return;
                            if(self->minimumSize() != wanted || self->maximumSize() != wanted)
                                self->setFixedSize(wanted);
                        });
                        return S_OK;
                    }).Get());
        }

        bool IsOwnOrigin(const QUrl &document) const {
            return document.scheme() == QStringLiteral("chrome-extension") &&
                   !document.host().isEmpty() && document.host() == m_Origin.host();
        }

        QUrl SourceOfView() const {
            if(const SharedView view = m_Source.lock()){
                if(EdgeWebView *edge = qobject_cast<EdgeWebView*>(view->base()))
                    return edge->ReportedSource();
            }
            return QUrl();
        }

        void Answer(const EdgeExtensionTabRequest &request, const QUrl &arrival){
            if(m_State.AnswerDue() != EdgeExtensionPageState::Effect::Answer || !m_Web) return;
            const ComPtr<ICoreWebView2> web = m_Web;
            QPointer<EdgeExtensionView> self(this);
            const QUrl now = SourceOfView();
            if(!self || m_State.IsClosed()) return;
            const QUrl document = ReportedSourceOf(web.Get());
            if(!self || m_State.IsClosed() || !IsOwnOrigin(document)) return;
            const QString reply = EdgeExtensionTabRequest::Reply(
                request.Document(), request.Sequence(),
                EdgeExtensionTabRequest::Answer(request.Candidates(), arrival, now, m_Sight.Sees(now)));
            web->PostWebMessageAsJson(reinterpret_cast<LPCWSTR>(reply.utf16()));
        }

        void Bounds(){
            const ComPtr<ICoreWebView2Controller> controller = m_Controller;
            if(!controller) return;
            RECT bounds;
            GetClientRect(reinterpret_cast<HWND>(winId()), &bounds);
            controller->put_Bounds(bounds);
        }

        void Shutdown(){
            m_State.Close();
            m_HostToken.reset();
            const ComPtr<ICoreWebView2Controller> controller = std::move(m_Controller);
            const ComPtr<ICoreWebView2> web = std::move(m_Web);
            const bool accelerator = m_AcceleratorHooked;
            const bool message = m_MessageHooked;
            const bool title = m_TitleHooked;
            const bool resource = m_ResourceHooked;
            const EventRegistrationToken acceleratorToken = m_AcceleratorToken;
            const EventRegistrationToken messageToken = m_MessageToken;
            const EventRegistrationToken titleToken = m_TitleToken;
            const EventRegistrationToken resourceToken = m_ResourceToken;
            m_AcceleratorHooked = m_MessageHooked = m_TitleHooked = m_ResourceHooked = false;
            if(accelerator && controller) controller->remove_AcceleratorKeyPressed(acceleratorToken);
            if(message && web) web->remove_WebMessageReceived(messageToken);
            if(title && web) web->remove_DocumentTitleChanged(titleToken);
            if(resource && web) web->remove_WebResourceRequested(resourceToken);
            if(controller) controller->Close();
        }

        void Fail(HRESULT hr){
            QPointer<EdgeExtensionView> self(this);
            Shutdown();
            if(!self) return;
            QVBoxLayout *layout = new QVBoxLayout(this);
            QLabel *label = new QLabel(Failure(hr), this);
            label->setWordWrap(true);
            layout->addWidget(label);
        }

        QString m_ProfileName;
        bool m_NamesTab;
        ExtensionHostWire::Sight m_Sight;
        bool m_Fits;
        bool m_Docked;
        bool m_FitAsked = false;
        bool m_FitQueued = false;
        QSize m_FitWanted;
        WeakView m_Source;
        QUrl m_Origin;
        EdgeExtensionPageState m_State;
        bool m_CloseQueued = false;
        bool m_TitleHooked = false;
        bool m_AcceleratorHooked = false;
        bool m_MessageHooked = false;
        bool m_ResourceHooked = false;
        EventRegistrationToken m_TitleToken{};
        EventRegistrationToken m_AcceleratorToken{};
        EventRegistrationToken m_MessageToken{};
        EventRegistrationToken m_ResourceToken{};
        ComPtr<ICoreWebView2Controller> m_Controller;
        ComPtr<ICoreWebView2> m_Web;
        QScopedPointer<QObject> m_HostToken;
    };

    class EdgeRelayView : public QObject {
    public:
        EdgeRelayView(const QString &profileName, const QUrl &url, QObject *parent)
            : QObject(parent)
            , m_ProfileName(profileName)
            , m_Url(url)
            , m_Window(new QWindow())
            , m_HostToken(new QObject())
        {
            m_Window->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus |
                               Qt::WindowStaysOnBottomHint | Qt::WindowTransparentForInput);
            m_Window->setGeometry(QRect(-30000, -30000, 8, 8));
        }
        ~EdgeRelayView() Q_DECL_OVERRIDE {
            Shutdown();
            delete m_Window;
        }

        void Initialize(){
            QPointer<EdgeRelayView> self(this);
            if(!qEnvironmentVariableIsSet("VANILLA_RELAY_HIDDEN")) m_Window->show();
            const HWND window = reinterpret_cast<HWND>(m_Window->winId());
            const QString profileName = m_ProfileName;
            const QUrl url = m_Url;
            ComPtr<ICoreWebView2Environment10> environment;
            ComPtr<ICoreWebView2ControllerOptions> options;
            const ComPtr<ICoreWebView2Environment> base = EdgeEnvironment::Instance()->GetEnvironment();
            HRESULT hr = base ? base.As(&environment) : E_POINTER;
            if(SUCCEEDED(hr)) hr = environment->CreateCoreWebView2ControllerOptions(&options);
            if(SUCCEEDED(hr)) hr = options->put_ProfileName(reinterpret_cast<LPCWSTR>(profileName.utf16()));
            if(SUCCEEDED(hr)) hr = options->put_IsInPrivateModeEnabled(FALSE);
            if(!self || m_Closed) return;
            if(FAILED(hr)){ m_Failed = true; return; }
            hr = environment->CreateCoreWebView2ControllerWithOptions(
                window, options.Get(),
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [self, environment, profileName, url](HRESULT hr, ICoreWebView2Controller *controller) -> HRESULT {
                        if(!self){ if(controller) controller->Close(); return S_OK; }
                        return self->Adopt(hr, controller, profileName, url);
                    }).Get());
            if(!self || m_Closed) return;
            if(FAILED(hr)) m_Failed = true;
        }

        bool Run(const QString &script){
            const ComPtr<ICoreWebView2> web = m_Web;
            if(m_Closed || !web) return false;
            return SUCCEEDED(web->ExecuteScript(reinterpret_cast<LPCWSTR>(script.utf16()),
                Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                    [](HRESULT, LPCWSTR) -> HRESULT { return S_OK; }).Get()));
        }

        void Shutdown(){
            m_Closed = true;
            m_HostToken.reset();
            const ComPtr<ICoreWebView2Controller> controller = std::move(m_Controller);
            const ComPtr<ICoreWebView2> web = std::move(m_Web);
            const bool resource = m_ResourceHooked;
            const EventRegistrationToken resourceToken = m_ResourceToken;
            m_ResourceHooked = false;
            if(resource && web) web->remove_WebResourceRequested(resourceToken);
            if(controller) controller->Close();
        }

    private:
        HRESULT Adopt(HRESULT hr, ICoreWebView2Controller *raw, const QString &profileName, const QUrl &url){
            const ComPtr<ICoreWebView2Controller> controller = raw;
            if(m_Closed){ if(controller) controller->Close(); return S_OK; }
            QPointer<EdgeRelayView> self(this);
            if(FAILED(hr) || !controller){
                if(controller) controller->Close();
                if(self) m_Failed = true;
                return S_OK;
            }
            ComPtr<ICoreWebView2> web;
            ComPtr<ICoreWebView2_13> web13;
            ComPtr<ICoreWebView2Profile> profile;
            LPWSTR name = nullptr;
            BOOL privateMode = TRUE;
            hr = controller->get_CoreWebView2(&web);
            if(SUCCEEDED(hr)) hr = web.As(&web13);
            if(SUCCEEDED(hr)) hr = web13->get_Profile(&profile);
            if(SUCCEEDED(hr)) hr = profile->get_ProfileName(&name);
            if(SUCCEEDED(hr)) hr = profile->get_IsInPrivateModeEnabled(&privateMode);
            const bool matches = SUCCEEDED(hr) && !privateMode && QString::fromWCharArray(name) == profileName;
            CoTaskMemFree(name);
            if(!self){ controller->Close(); return S_OK; }
            if(m_Closed || !matches){
                controller->Close();
                if(self && !m_Closed) m_Failed = true;
                return S_OK;
            }
            m_Controller = controller;
            m_Web = web;

            {
                EventRegistrationToken resourceToken{};
                const bool filtered = SUCCEEDED(web->AddWebResourceRequestedFilter
                    (L"vanilla-extension://*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL));
                if(!self || m_Closed) return S_OK;
                QPointer<QObject> owner(m_HostToken.data());
                const QString profileOf = profileName;
                const bool hooked = filtered && SUCCEEDED(web->add_WebResourceRequested(
                    Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                        [self, owner, profileOf](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs *args) -> HRESULT {
                            if(!self || self->m_Closed || !args) return S_OK;
                            ComPtr<ICoreWebView2WebResourceRequest> request;
                            if(FAILED(args->get_Request(&request)) || !request) return S_OK;
                            LPWSTR uri = nullptr;
                            QUrl url;
                            if(SUCCEEDED(request->get_Uri(&uri)) && uri){
                                url = QUrl(QString::fromWCharArray(uri));
                                CoTaskMemFree(uri);
                            }
                            if(url.scheme() != QLatin1String(ExtensionHostWire::SCHEME)) return S_OK;
                            return EdgeAnswerExtensionHost(ControllerOf(profileOf), 0, owner.data(),
                                                           args, request.Get(), url);
                        }).Get(), &resourceToken));
                if(!self || m_Closed){
                    if(hooked) web->remove_WebResourceRequested(resourceToken);
                    return S_OK;
                }
                m_ResourceToken = resourceToken;
                m_ResourceHooked = hooked;
            }
            RECT bounds{0, 0, 8, 8};
            controller->put_Bounds(bounds);
            if(!self || m_Closed) return S_OK;
            controller->put_IsVisible(qEnvironmentVariableIsSet("VANILLA_RELAY_HIDDEN") ? FALSE : TRUE);
            if(!self || m_Closed) return S_OK;
            const QString address = url.toString();
            hr = web->Navigate(reinterpret_cast<LPCWSTR>(address.utf16()));
            if(self && !m_Closed && FAILED(hr)) m_Failed = true;
            return S_OK;
        }

        QString m_ProfileName;
        QUrl m_Url;
        QWindow *m_Window;
        bool m_Closed = false;
        bool m_Failed = false;
        bool m_ResourceHooked = false;
        EventRegistrationToken m_ResourceToken{};
        ComPtr<ICoreWebView2Controller> m_Controller;
        ComPtr<ICoreWebView2> m_Web;
        QScopedPointer<QObject> m_HostToken;
    };

    void EdgeExtensions::ReconcileRelays(){
        if(m_Reconciling){ m_ReconcileAgain = true; return; }
        m_Reconciling = true;
        QPointer<EdgeExtensions> self(this);
        do {
            m_ReconcileAgain = false;
            QSet<QString> wanted;
            if(ExtensionHost::ShimsOn())
                foreach(const ExtensionRow &row, Rows())
                    if(row.loaded && row.enabled && HasKeyedShims(row.manifest.id)) wanted.insert(row.manifest.id);
            const QStringList had = m_Relays.keys();
            foreach(const QString &id, had){
                if(wanted.contains(id)) continue;
                EdgeRelayView *gone = m_Relays.take(id);
                delete gone;
                if(!self) return;
            }
            foreach(const QString &id, wanted){
                if(m_Relays.contains(id)) continue;
                const QString page = qEnvironmentVariableIsSet("VANILLA_RELAY_PAGE")
                    ? qEnvironmentVariable("VANILLA_RELAY_PAGE") : QStringLiteral("vanilla_relay.html");
                EdgeRelayView *relay = new EdgeRelayView(m_ProfileName,
                    QUrl(QStringLiteral("chrome-extension://") + id + QLatin1Char('/') + page), this);
                m_Relays.insert(id, relay);
                relay->Initialize();
                if(!self) return;
            }
        } while(m_ReconcileAgain);
        m_Reconciling = false;
    }

    bool EdgeExtensions::SendToWorker(const QString &id, const QJsonArray &args){
        EdgeRelayView *relay = m_Relays.value(id);
        if(!relay) return false;
        return relay->Run(Cdp::MenuChosenScript(args));
    }

    ExtensionController *ControllerOf(const QString &profileName){
        foreach(QObject *child, EdgeEnvironment::Instance()->children()){
            ExtensionController *manager = qobject_cast<ExtensionController*>(child);
            if(manager && manager->property("edgeExtensionProfile").toString() == profileName) return manager;
        }
        return nullptr;
    }
}

HRESULT EdgeAnswerExtensionHost(ExtensionController *controller, quint64 viewNumber, QObject *owner,
                                ICoreWebView2WebResourceRequestedEventArgs *args,
                                ICoreWebView2WebResourceRequest *request, const QUrl &url){
    if(!args || !request) return S_OK;
    const QPointer<ExtensionHost> host = ExtensionHost::Of(controller);
    const QPointer<QObject> asker(owner);
    const ComPtr<ICoreWebView2Environment> environment = EdgeEnvironment::Instance()->GetEnvironment();
    if(!environment) return S_OK;

    LPWSTR method = nullptr;
    QString methodText;
    if(SUCCEEDED(request->get_Method(&method)) && method){
        methodText = QString::fromWCharArray(method);
        CoTaskMemFree(method);
    }
    if(methodText == QLatin1String("OPTIONS")){
        QString origin;
        ComPtr<ICoreWebView2HttpRequestHeaders> headers;
        LPWSTR value = nullptr;
        if(SUCCEEDED(request->get_Headers(&headers)) && headers &&
           SUCCEEDED(headers->GetHeader(L"Origin", &value)) && value){
            origin = QString::fromWCharArray(value);
            CoTaskMemFree(value);
        }
        const QString cors = EdgeAsk::CorsHeaders(origin);
        ComPtr<ICoreWebView2WebResourceResponse> response;
        if(SUCCEEDED(environment->CreateWebResourceResponse
                     (nullptr, 204, L"No Content", reinterpret_cast<PCWSTR>(cors.utf16()), &response)) && response)
            args->put_Response(response.Get());
        return S_OK;
    }

    ComPtr<ICoreWebView2Deferral> deferral;
    if(!host || !asker || FAILED(args->GetDeferral(&deferral)) || !deferral){
        ComPtr<ICoreWebView2WebResourceResponse> response;
        if(SUCCEEDED(environment->CreateWebResourceResponse
                     (nullptr, 404, L"Not Found", L"", &response)) && response)
            args->put_Response(response.Get());
        return S_OK;
    }
    const std::shared_ptr<EdgeAsk> ask =
        std::make_shared<EdgeAsk>(args, deferral.Get(), environment.Get(), request, url, asker.data());
    if(!host || !asker){
        ask->Fail();
        return S_OK;
    }
    host->Handle(ask, viewNumber);
    return S_OK;
}

void EdgeEnvironment::PinExtensionCopies(){
    if(!ExtensionHost::ShimsOn()) return;
    foreach(const QString &path, Application::GlobalSettings().value(QStringLiteral("network/@Extensions")).toStringList()){
        const ExtensionManifest manifest = ExtensionManifest::Read(path);
        if(manifest.error.isEmpty()) PinnedCopy(manifest);
    }
}

void EdgeWebView::SetupExtensions(){
    if(!m_Impl->m_ProfileMeasured || m_Impl->m_ActualPrivate || !m_Impl->m_WebView) return;
    EdgeEnvironment *owner = EdgeEnvironment::Instance();
    const QString name = m_Impl->m_ActualProfileName;
    foreach(QObject *child, owner->children()){
        ExtensionController *manager = qobject_cast<ExtensionController*>(child);
        if(manager && manager->property("edgeExtensionProfile").toString() == name){
            m_Impl->m_Extensions = manager;
            break;
        }
    }
    if(!m_Impl->m_Extensions){
        ComPtr<ICoreWebView2_13> web;
        ComPtr<ICoreWebView2Profile> profile;
        ComPtr<ICoreWebView2Profile7> profile7;
        if(SUCCEEDED(m_Impl->m_WebView.As(&web)) && SUCCEEDED(web->get_Profile(&profile)) &&
           SUCCEEDED(profile.As(&profile7)))
            m_Impl->m_Extensions = new EdgeExtensions(owner, name, m_Impl->m_Space, profile7.Get());
    }
    if(m_Impl->m_Extensions){
        connect(m_Impl->m_Extensions, &ExtensionController::Ready, this, [this](){
            if(!m_Impl->m_State.IsRetired()) TryFlushPendingNavigation();
        });
    }
    emit ExtensionContextChanged();
}

ExtensionController *EdgeWebView::Extensions() const {
    return m_Impl->m_State.IsRetired() ? nullptr : m_Impl->m_Extensions.data();
}

QString EdgeWebView::ExtensionStatus() const {
    if(!m_Impl->m_WebView && !m_Impl->m_State.IsRetired())
        return tr("Waiting for WebView2 to initialize.");
    return View::ExtensionStatus();
}

QWidget *EdgeWebView::CreateExtensionView(const QUrl &url, ExtensionPage page, QWidget *parent, const std::function<void()> &closed){
    Q_UNUSED(closed)
    ExtensionController *controller = Extensions();
    if(!controller) return nullptr;
    if(page == ExtensionSidePanelPage){
        QPointer<EdgeExtensionView> host =
            new EdgeExtensionView(m_Impl->m_ActualProfileName, url, ExtensionHostWire::Sight(), false, GetThis(), parent, true);
        host->Initialize();
        return host.data();
    }
    ExtensionHostWire::Sight sight;
    if(page == ExtensionActionPage && !controller->HasKeyedShims(url.host())){
        foreach(const ExtensionRow &row, controller->Rows()){
            if(row.manifest.id == url.host()){
                const ExtensionManifest manifest = ExtensionManifest::Read(row.manifest.path);
                sight = ExtensionHostWire::Sight::Of(manifest.tabsPermission, url.host(), manifest.hostPermissions);
                break;
            }
        }
    }
    QPointer<EdgeExtensionView> host =
        new EdgeExtensionView(m_Impl->m_ActualProfileName, url, sight, page == ExtensionActionPage, GetThis(), parent);
    host->Initialize();
    return host.data();
}

#endif
