#include "switch.hpp"

#include "webengineextensions.hpp"

#ifdef WEBENGINEVIEW

#include "extensioncontroller.hpp"
#include "extensioncopy.hpp"
#include "extensionhost.hpp"
#include "extensionhostwire.hpp"
#include "webenginepage.hpp"
#include "application.hpp"
#include "devicescale.hpp"
#include "networkcontroller.hpp"
#include "extensionui.hpp"
#include "view.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"

#include <functional>

#include <QChildEvent>
#include <QContextMenuEvent>
#include <QLocale>
#include <QMenu>
#include <QTimer>
#include <QWebEngineScript>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineNewWindowRequest>
#include <QQuickWebEngineProfile>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QtWebEngineCore/qtwebenginecoreglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0) && QT_CONFIG(webengine_extensions)
#  define VANILLA_WEBENGINE_EXTENSIONS 1
#  include <QWebEngineExtensionManager>
#  include <QWebEngineExtensionInfo>
#endif

#ifdef VANILLA_WEBENGINE_EXTENSIONS
namespace {

    ExtensionItem Item(const QWebEngineExtensionInfo &info){
        ExtensionItem item;
        item.id = info.id();
        item.name = info.name();
        item.path = ExtensionCopy::SourceOf(info.path());
        item.enabled = info.isEnabled();
        item.popup = info.actionPopupUrl();
        return item;
    }

    class QtExtensions : public ExtensionController {
    public:
        QtExtensions(QObject *profile, QWebEngineExtensionManager *manager)
            : ExtensionController(profile)
            , m_Manager(manager)
        {
            SetMenusFile(Application::StateDirectory() + ExtensionUi::MenusFileName(NetworkController::ProfileKey(profile)));
            SetRulesFile(Application::StateDirectory() + ExtensionUi::RulesFileName(NetworkController::ProfileKey(profile)));
            SetInstalledFile(Application::StateDirectory() + ExtensionUi::InstalledFileName(NetworkController::ProfileKey(profile)));
        }

    protected:
        bool WorkerIsOfThisRun(const QString &id) const Q_DECL_OVERRIDE { return m_Refreshed.contains(id); }
        bool HandlesCommands() const Q_DECL_OVERRIDE { return true; }

        void Snapshot(SnapshotDone done) Q_DECL_OVERRIDE {
            QList<ExtensionItem> result;
            foreach(const QWebEngineExtensionInfo &info, m_Manager->extensions())
                result.append(Item(info));
            done(result, QString());
        }

        QString EngineNote() const Q_DECL_OVERRIDE {
            if(!ExtensionHost::ShimsOn()) return QString();
            return ExtensionController::tr(
                "In this view extensions run from a copy with a compatibility layer added. "
                "Your folder is not changed.\n"
                "Some extensions may not work fully.");
        }

        QSet<QString> ShimmedIds() const Q_DECL_OVERRIDE {
            QSet<QString> ids;
            foreach(const QWebEngineExtensionInfo &info, m_Manager->extensions())
                if(info.isEnabled() && ExtensionCopy::SourceOf(info.path()) != info.path()) ids.insert(info.id());
            return ids;
        }
        QString RunFolderOf(const QString &id) const Q_DECL_OVERRIDE {
            foreach(const QWebEngineExtensionInfo &info, m_Manager->extensions())
                if(info.id() == id && info.isEnabled() && ExtensionCopy::SourceOf(info.path()) != info.path()) return info.path();
            return QString();
        }

    protected:

        void Add(const ExtensionManifest &manifest, ItemDone done) Q_DECL_OVERRIDE {
            QString path = manifest.folder;
            if(ExtensionHost::ShimsOn()){
                const ExtensionCopy::Made made = ExtensionCopy::Make
                    (manifest.folder, ExtensionHost::CopyRoot(), manifest.id,
                     ExtensionHost::KeyFor(manifest.id), ExtensionCopy::UiLocale());
                path = made.path;
                Note(manifest.path, CopyNote(made.note, made.detail, made.withheld));
            } else {
                Note(manifest.path, QString());
            }
            auto connection = std::make_shared<QMetaObject::Connection>();
            *connection = connect(m_Manager, &QWebEngineExtensionManager::loadFinished, this,
                                  [path, done, connection](const QWebEngineExtensionInfo &info){
                if(ExtensionManifest::PathKey(info.path()) != ExtensionManifest::PathKey(path)) return;
                QObject::disconnect(*connection);
                done(Item(info), info.error());
            });
            m_Manager->loadExtension(path);
        }

        void Enable(const QString &id, bool enabled, ItemDone done) Q_DECL_OVERRIDE {
            foreach(const QWebEngineExtensionInfo &info, m_Manager->extensions()){
                if(info.id() != id) continue;
                m_Manager->setExtensionEnabled(info, enabled);
                if(enabled && !m_Refreshed.contains(id)){
                    m_Refreshed.insert(id);
                    m_Manager->setExtensionEnabled(info, false);
                    m_Manager->setExtensionEnabled(info, true);
                }
                done(Item(info), QString());
                return;
            }
            ExtensionItem gone;
            gone.gone = true;
            done(gone, ExtensionController::tr("The extension is no longer loaded."));
        }

        void Remove(const QString &id, Done done) Q_DECL_OVERRIDE {
            foreach(const QWebEngineExtensionInfo &info, m_Manager->extensions()){
                if(info.id() != id) continue;
                auto connection = std::make_shared<QMetaObject::Connection>();
                *connection = connect(m_Manager, &QWebEngineExtensionManager::unloadFinished, this,
                                      [this, id, done, connection](const QWebEngineExtensionInfo &result){
                    if(result.id() != id) return;
                    QObject::disconnect(*connection);
                    if(result.error().isEmpty()) m_Refreshed.remove(id);
                    done(result.error());
                });
                m_Manager->unloadExtension(info);
                return;
            }
            done(QString());
        }

    private:
        QWebEngineExtensionManager *m_Manager;
        QSet<QString> m_Refreshed;
    };

    template <typename Profile> void InstallOn(Profile *profile){
        if(profile->isOffTheRecord() || ExtensionController::Of(profile)) return;
        if(QWebEngineExtensionManager *manager = profile->extensionManager()){
            ExtensionController *controller = new QtExtensions(profile, manager);
            if(ExtensionHost::ShimsOn()) ExtensionHost::Install(profile, controller);
        }
    }
}
#endif

void WebEngineExtensions::Install(QWebEngineProfile *profile){
#ifdef VANILLA_WEBENGINE_EXTENSIONS
    InstallOn(profile);
#else
    Q_UNUSED(profile)
#endif
}

void WebEngineExtensions::Install(QQuickWebEngineProfile *profile){
#ifdef VANILLA_WEBENGINE_EXTENSIONS
    InstallOn(profile);
#else
    Q_UNUSED(profile)
#endif
}

QQuickWebEngineProfile *WebEngineExtensions::CreateQuickProfile(QObject *owner, const QVariantMap &initial){
    if(!owner) owner = QCoreApplication::instance();
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    QQmlEngine *engine = new QQmlEngine(owner);
    QQmlComponent component(engine);
    component.setData("import QtWebEngine\nWebEngineProfilePrototype {}", QUrl());
    QObject *prototype = component.createWithInitialProperties(initial);
    QQuickWebEngineProfile *profile = nullptr;
    if(prototype){
        prototype->setParent(engine);
        QMetaObject::invokeMethod(prototype, "instance", Qt::DirectConnection,
                                  Q_RETURN_ARG(QQuickWebEngineProfile*, profile));
    }
    if(profile) return profile;
    qWarning() << "Quick profile construction failed:" << component.errors();
    delete engine;
#else
    QQuickWebEngineProfile *legacy = new QQuickWebEngineProfile(owner);
    legacy->setStorageName(initial.value(QStringLiteral("storageName")).toString());
    legacy->setOffTheRecord(false);
    for(QVariantMap::const_iterator i = initial.begin(); i != initial.end(); ++i){
        if(legacy->metaObject()->indexOfProperty(i.key().toLatin1().constData()) >= 0)
            legacy->setProperty(i.key().toLatin1().constData(), i.value());
    }
    return legacy;
#endif
    QQuickWebEngineProfile *fallback = new QQuickWebEngineProfile(owner);
    fallback->setProperty("extensionProfileError", ExtensionController::tr(
        "The persistent Quick profile could not be opened. "
        "This view is temporary and extensions are unavailable."));
    return fallback;
}

namespace {

    void PopupMenu(QWidget *parent, std::function<void(const QString&)> act,
                   bool editable, const QString &selection, const QUrl &link, const QPoint &at){
        QMenu menu(parent);
        auto word = [](const char *text){ return QCoreApplication::translate("ExtensionPopup", text); };
        auto add = [&](const char *text, const QString &name){
            QAction *action = menu.addAction(word(text));
            QObject::connect(action, &QAction::triggered, &menu, [act, name](){ act(name); });
        };
        if(editable){
            add(QT_TRANSLATE_NOOP("ExtensionPopup", "Undo"), QStringLiteral("undo"));
            add(QT_TRANSLATE_NOOP("ExtensionPopup", "Redo"), QStringLiteral("redo"));
            menu.addSeparator();
            add(QT_TRANSLATE_NOOP("ExtensionPopup", "Cut"), QStringLiteral("cut"));
            add(QT_TRANSLATE_NOOP("ExtensionPopup", "Copy"), QStringLiteral("copy"));
            add(QT_TRANSLATE_NOOP("ExtensionPopup", "Paste"), QStringLiteral("paste"));
            menu.addSeparator();
            add(QT_TRANSLATE_NOOP("ExtensionPopup", "Select all"), QStringLiteral("selectAll"));
        } else if(!selection.isEmpty()){
            add(QT_TRANSLATE_NOOP("ExtensionPopup", "Copy"), QStringLiteral("copy"));
        }
        if(!link.isEmpty()){
            if(!menu.isEmpty()) menu.addSeparator();
            add(QT_TRANSLATE_NOOP("ExtensionPopup", "Copy link address"), QStringLiteral("copyLink"));
        }
        if(!menu.isEmpty()) menu.addSeparator();
        add(QT_TRANSLATE_NOOP("ExtensionPopup", "Reload"), QStringLiteral("reload"));
        menu.exec(at);
    }

    void OpenAsked(const WeakView &opener, ExtensionController *controller, const QUrl &made,
                   const QUrl &shown, const QUrl &asked, QWidget *popup, bool docked = false){
        bool ok = false;
        const QUrl url = ExtensionHostWire::PopupWindowUrlOf(made.host(), shown, asked.toString(QUrl::FullyEncoded), &ok);
        if(!ok || !controller) return;
        QPointer<QWidget> panel = docked ? popup : popup->window();
        QPointer<ExtensionController> of = controller;
        QTimer::singleShot(0, qApp, [opener, of, url, panel, docked](){
            if(!panel || !panel->isVisible() || !of) return;
            SharedView view;
            if(docked){
                MainWindow *window = nullptr;
                for(QWidget *up = panel.data(); up && !window; up = up->parentWidget()) window = qobject_cast<MainWindow*>(up);
                if(window && window->GetTreeBank()) view = window->GetTreeBank()->GetCurrentView();
            } else {
                panel->close();
                view = opener.lock();
            }
            if(!view || view->Extensions() != of) return;
            if(WebEnginePage *page = qobject_cast<WebEnginePage*>(view->page())) page->OpenInNew(url);
        });
    }

    void CloseAsked(const WeakView &opener, QWidget *popup){
        QPointer<QWidget> panel = popup->window();
        QTimer::singleShot(0, qApp, [opener, panel](){
            if(!panel || !panel->isVisible() ||
               (panel->windowType() != Qt::Popup && panel->windowType() != Qt::Tool)) return;
            panel->close();
            const SharedView view = opener.lock();
            if(!view) return;
            if(QWidget *base = qobject_cast<QWidget*>(view->base())) base->window()->activateWindow();
            view->setFocus(Qt::PopupFocusReason);
        });
    }

    struct PopupFitting {
        bool asked = false;
        bool pending = false;
        void Loaded(QObject *owner, const std::function<void()> &fit){
            if(asked) return;
            asked = true;
            for(const int milliseconds : POPUP_FIT_TIMES)
                QTimer::singleShot(milliseconds, owner, fit);
        }
        void Hand(QObject *owner, const std::function<void()> &fit){
            if(!asked || pending) return;
            pending = true;
            bool first = true;
            for(const int milliseconds : POPUP_FIT_SETTLE_TIMES){
                QTimer::singleShot(milliseconds, owner, [this, first, fit](){ if(first) pending = false; fit(); });
                first = false;
            }
        }
    };

    class WidgetPopup : public QWebEngineView {
    public:
        WidgetPopup(const SharedProfile &profile, const QUrl &url, bool fits, const WeakView &opener, QWidget *parent,
                    const std::function<void()> &closed, bool docked)
            : QWebEngineView(parent)
            , m_Profile(profile)
        {
            setPage(new QWebEnginePage(profile.get(), this));
            connect(page(), &QWebEnginePage::newWindowRequested, this,
                    [this, url, opener, docked](QWebEngineNewWindowRequest &request){
                OpenAsked(opener, ExtensionController::Of(m_Profile.get()), url, this->url(), request.requestedUrl(), this, docked);
            });
            if(docked)
                connect(page(), &QWebEnginePage::windowCloseRequested, this, [closed](){ if(closed) closed(); });
            else
                connect(page(), &QWebEnginePage::windowCloseRequested, this, [this, opener](){ CloseAsked(opener, this); });
            setZoomFactor(DeviceScale::PrimaryFactor());
            connect(this, &QWebEngineView::titleChanged, this, &QWidget::setWindowTitle);
            if(fits){
                connect(this, &QWebEngineView::loadFinished, this, [this](bool){
                    m_Fitting.Loaded(this, [this](){ Fit(); });
                });
            }
        }
        ~WidgetPopup() Q_DECL_OVERRIDE {
            delete page();
        }

    protected:
        void childEvent(QChildEvent *ev) Q_DECL_OVERRIDE {
            QWebEngineView::childEvent(ev);
            if(ev->added()) ev->child()->installEventFilter(this);
        }
        bool eventFilter(QObject *watched, QEvent *ev) Q_DECL_OVERRIDE {
            if(ev->type() == QEvent::KeyRelease ||
               (ev->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent*>(ev)->button() != Qt::RightButton))
                Hand();
            return QWebEngineView::eventFilter(watched, ev);
        }
        void contextMenuEvent(QContextMenuEvent *ev) Q_DECL_OVERRIDE {
            const QWebEngineContextMenuRequest *request = lastContextMenuRequest();
            if(!request){ ev->ignore(); return; }
            static const QMap<QString, QWebEnginePage::WebAction> actions = {
                { QStringLiteral("undo"), QWebEnginePage::Undo }, { QStringLiteral("redo"), QWebEnginePage::Redo },
                { QStringLiteral("cut"), QWebEnginePage::Cut }, { QStringLiteral("copy"), QWebEnginePage::Copy },
                { QStringLiteral("paste"), QWebEnginePage::Paste }, { QStringLiteral("selectAll"), QWebEnginePage::SelectAll },
                { QStringLiteral("copyLink"), QWebEnginePage::CopyLinkToClipboard }, { QStringLiteral("reload"), QWebEnginePage::Reload }};
            QPointer<QWebEnginePage> page = this->page();
            PopupMenu(this, [page](const QString &name){ if(page && actions.contains(name)) page->triggerAction(actions.value(name)); },
                      request->isContentEditable(), request->selectedText(), request->linkUrl(), ev->globalPos());
            ev->accept();
        }

    private:
        void Hand(){ m_Fitting.Hand(this, [this](){ Fit(); }); }
        void Fit(){
            QPointer<WidgetPopup> self(this);
            page()->runJavaScript(View::ExtensionPopupSizeJsCode(), QWebEngineScript::ApplicationWorld,
                                  [self](const QVariant &measured){
                if(!self) return;
                const QSize size = ExtensionUi::PopupSizeOf(measured.toString(), self->devicePixelRatioF(), self->zoomFactor());
                if(size.isValid() && (self->minimumSize() != size || self->maximumSize() != size))
                    self->setFixedSize(size);
            });
        }

        SharedProfile m_Profile;
        PopupFitting m_Fitting;
    };

    class QuickPopup : public QQuickWidget {
        Q_OBJECT

    public:
        QuickPopup(QQuickWebEngineProfile *profile, const QUrl &url, bool fits, const WeakView &opener, QWidget *parent)
            : QQuickWidget(parent)
            , m_Profile(profile)
            , m_Url(url)
            , m_Opener(opener)
            , m_Fits(fits)
        {
        }

    public slots:
        void Close(){ CloseAsked(m_Opener, this); }
        void Open(const QUrl &asked){
            const QUrl shown = rootObject() ? rootObject()->property("url").toUrl() : QUrl();
            OpenAsked(m_Opener, m_Profile ? ExtensionController::Of(m_Profile) : nullptr, m_Url, shown, asked, this);
        }
        void UpdateTitle(){
            if(rootObject()) setWindowTitle(rootObject()->property("title").toString());
        }
        void Loaded(){
            if(m_Fits) m_Fitting.Loaded(this, [this](){ Fit(); });
        }
        void Measured(const QString &measured){
            QQuickItem *root = rootObject();
            if(!root) return;
            const QSize size = ExtensionUi::PopupSizeOf(measured, devicePixelRatioF(), root->property("zoomFactor").toReal());
            if(size.isValid() && (minimumSize() != size || maximumSize() != size))
                setFixedSize(size);
        }
        void AskMenu(bool editable, const QString &selection, const QUrl &link, const QPointF &at){
            QPointer<QQuickItem> root = rootObject();
            PopupMenu(this, [root](const QString &name){
                if(root) QMetaObject::invokeMethod(root, "act", Q_ARG(QVariant, QVariant(name)));
            }, editable, selection, link, mapToGlobal(at.toPoint()));
        }
        void UpdateFocus(){
            if(isVisible() && hasFocus() && rootObject() &&
               QGuiApplication::focusWindow() == window()->windowHandle()){
                rootObject()->forceActiveFocus();
            }
        }

    protected:
        void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE {
            QQuickWidget::mouseReleaseEvent(ev);
            if(ev->button() != Qt::RightButton) Hand();
        }
        void keyReleaseEvent(QKeyEvent *ev) Q_DECL_OVERRIDE {
            QQuickWidget::keyReleaseEvent(ev);
            Hand();
        }

    private:
        const QPointer<QQuickWebEngineProfile> m_Profile;
        const QUrl m_Url;
        const WeakView m_Opener;
        void Hand(){ m_Fitting.Hand(this, [this](){ Fit(); }); }
        void Fit(){
            if(QQuickItem *root = rootObject())
                QMetaObject::invokeMethod(root, "measure", Q_ARG(QVariant, View::ExtensionPopupSizeJsCode()),
                                          Q_ARG(QVariant, int(QWebEngineScript::ApplicationWorld)));
        }
        bool m_Fits;
        PopupFitting m_Fitting;
    };
}

QWidget *WebEngineExtensions::CreatePopup(const SharedProfile &profile, const QUrl &url, bool fits, const WeakView &opener, QWidget *parent,
                                          const std::function<void()> &closed, bool docked){
    if(!profile || profile->isOffTheRecord()) return nullptr;
    if(ExtensionController *controller = ExtensionController::Of(profile.get())) controller->Start();
    WidgetPopup *view = new WidgetPopup(profile, url, fits, opener, parent, closed, docked);
    view->setUrl(url);
    return view;
}

QWidget *WebEngineExtensions::CreatePopup(QQuickWebEngineProfile *profile, const QUrl &url, bool fits, const WeakView &opener, QWidget *parent){
    if(!profile || profile->isOffTheRecord()) return nullptr;
    if(ExtensionController *controller = ExtensionController::Of(profile)) controller->Start();
    QuickPopup *view = new QuickPopup(profile, url, fits, opener, parent);
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QQmlComponent *component = new QQmlComponent(view->engine(), view);
    component->setData(
        "import QtWebEngine\n"
        "WebEngineView {\n"
        "  focus: true\n"
        "  signal menuAsked(bool editable, string selection, url link, point at)\n"
        "  signal openAsked(url address)\n"
        "  signal closeAsked()\n"
        "  signal loaded()\n"
        "  signal measured(string value)\n"
        "  onLoadingChanged: function(info) {\n"
        "    if (info.status === WebEngineView.LoadSucceededStatus || info.status === WebEngineView.LoadFailedStatus ||\n"
        "        info.status === WebEngineView.LoadStoppedStatus) loaded();\n"
        "  }\n"
        "  function measure(script, world) {\n"
        "    runJavaScript(script, world, function(result) {\n"
        "      measured(result === null || result === undefined ? \"\" : String(result));\n"
        "    });\n"
        "  }\n"
        "  onContextMenuRequested: function(request) {\n"
        "    request.accepted = true;\n"
        "    menuAsked(request.isContentEditable, request.selectedText, request.linkUrl, request.position);\n"
        "  }\n"
        "  onNewWindowRequested: function(request) { openAsked(request.requestedUrl); }\n"
        "  onWindowCloseRequested: closeAsked()\n"
        "  function act(name) {\n"
        "    var table = { undo: WebEngineView.Undo, redo: WebEngineView.Redo, cut: WebEngineView.Cut, copy: WebEngineView.Copy,\n"
        "                  paste: WebEngineView.Paste, selectAll: WebEngineView.SelectAll,\n"
        "                  copyLink: WebEngineView.CopyLinkToClipboard, reload: WebEngineView.Reload };\n"
        "    if (name in table) triggerWebAction(table[name]);\n"
        "  }\n"
        "}\n", QUrl());
    QObject *root = component->createWithInitialProperties({
        {QStringLiteral("profile"), QVariant::fromValue(profile)},
        {QStringLiteral("zoomFactor"), DeviceScale::PrimaryFactor()},
        {QStringLiteral("url"), url}});
    if(!root){
        qWarning() << component->errors();
        delete view;
        return nullptr;
    }
    view->setContent(QUrl(), component, root);
    QObject::connect(root, SIGNAL(titleChanged()), view, SLOT(UpdateTitle()));
    QObject::connect(root, SIGNAL(menuAsked(bool,QString,QUrl,QPointF)), view, SLOT(AskMenu(bool,QString,QUrl,QPointF)));
    QObject::connect(root, SIGNAL(openAsked(QUrl)), view, SLOT(Open(QUrl)));
    QObject::connect(root, SIGNAL(closeAsked()), view, SLOT(Close()));
    QObject::connect(root, SIGNAL(loaded()), view, SLOT(Loaded()));
    QObject::connect(root, SIGNAL(measured(QString)), view, SLOT(Measured(QString)));
    QObject::connect(root, SIGNAL(loadingChanged(QWebEngineLoadingInfo)), view, SLOT(UpdateFocus()));
    QObject::connect(qGuiApp, &QGuiApplication::focusWindowChanged, view, [view](){ view->UpdateFocus();});
    QObject::connect(profile, &QObject::destroyed, view, [view](){ view->window()->close();});
    return view;
}

#include "webengineextensions.moc"

#endif
