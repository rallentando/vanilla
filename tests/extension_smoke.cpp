#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QCheckBox>
#include <QLabel>
#include <QScreen>
#include <QDir>
#include <QTranslator>
#include <QWebEngineView>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QtWebEngineQuick>
#include <QQmlExpression>
#include <QQmlContext>
#include <QJsonDocument>
#include <QJsonArray>
#include "application.hpp"
#include "networkcontroller.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include <QMenu>
#include "edgewebview.hpp"
#include "extensioncontroller.hpp"
#include "extensionbar.hpp"
#include "settingspage.hpp"
#include "theme.hpp"
#include "lightnode.hpp"
#include "treebank.hpp"
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {
SharedView Make(const QString &kind, const QString &id, bool privateMode = false) {
    const QStringList settings = privateMode ? QStringList{"Private"} : QStringList{};
    SharedView view;
    if (kind == "widget") view = std::make_shared<WebEngineView>(nullptr, id, settings);
    if (kind == "quick") view = std::make_shared<QuickWebEngineView>(nullptr, id, settings);
#ifdef EDGEWEBVIEW
    if (kind == "edge") view = std::make_shared<EdgeWebView>(nullptr, id, settings);
#endif
    if (view) view->SetThis(view);
    return view;
}

QString Script(View *view, const QString &script) {
    auto result = std::make_shared<QString>();
    QEventLoop loop;
    QPointer<QEventLoop> waiting(&loop);
    bool completed = false;
    view->CallWithEvaluatedJavaScriptResult(script, [result, waiting, &completed](QVariant value) {
        *result = value.toString();
        if (waiting) { completed = true; waiting->quit(); }
    });
    if (!completed) {
        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        loop.exec();
    }
    return *result;
}

void Capture(QWidget *widget, const QString &name) {
    const QString directory = qEnvironmentVariable("VANILLA_EXTENSION_SCREENSHOTS");
    if (directory.isEmpty()) return;
    QDir().mkpath(directory);
    widget->raise(); QTest::qWait(250);
    const QPoint origin = widget->mapToGlobal(QPoint());
    widget->screen()->grabWindow(0, origin.x(), origin.y(), widget->width(), widget->height())
        .save(directory + QLatin1Char('/') + name + QStringLiteral(".png"));
}
}

class ExtensionSmoke : public QObject {
    Q_OBJECT
    Settings m_RestartSettings;
private slots:
    void initTestCase() {
        if (qEnvironmentVariable("VANILLA_EXTENSION_PHASE", "exercise") != "exercise") Application::LoadSettingsFile();
        m_RestartSettings = Application::GlobalSettings();
    }
    void cleanupTestCase() {
        QVERIFY(Application::SaveSettingsFile(Application::GlobalSettings()));
    }
    void backends_data() {
        QTest::addColumn<QString>("kind");
        QTest::newRow("widget") << QStringLiteral("widget");
        QTest::newRow("quick") << QStringLiteral("quick");
#ifdef EDGEWEBVIEW
        QTest::newRow("edge") << QStringLiteral("edge");
#endif
    }
    void backends() {
        QFETCH(QString, kind);
        const QString phase = qEnvironmentVariable("VANILLA_EXTENSION_PHASE", "exercise");
        if (phase == "history" && kind != "widget") QSKIP("Serialized history is a Widgets backend feature.");
        const QString fixture = QString::fromUtf8(VANILLA_FIXTURE_DIR);
        QStringList registrations{fixture};
        const QString extra = qEnvironmentVariable("VANILLA_EXTENSION_EXTRA");
        if (!extra.isEmpty()) registrations.append(extra);
        if (phase == "probe") registrations.append(fixture + "-notabs");
        auto &settings = Application::GlobalSettings();
        if (phase == "exercise" || phase == "history" || phase == "probe") {
            settings.setValue("network/@Extensions", registrations);
            settings.setValue("network/@DisabledExtensions", QStringList{});
            settings.setValue("network/@PinnedExtensions", registrations);
        } else {
            for (const QString &key : {QStringLiteral("network/@Extensions"), QStringLiteral("network/@DisabledExtensions"),
                                       QStringLiteral("network/@PinnedExtensions")})
                settings.setValue(key, m_RestartSettings.value(key));
            QCOMPARE(settings.value("network/@Extensions").toStringList(), phase == "removed" ? QStringList{} : registrations);
            QCOMPARE(settings.value("network/@DisabledExtensions").toStringList(), phase == "disabled" ? QStringList{fixture} : QStringList{});
        }
        settings.setValue("network/@SpellCheckLanguages", QStringList{});
        QByteArray savedHistory;
        quint16 historyPort = 0;
        const QString historyFile = Application::StateDirectory() + "extension-smoke-history.bin";
        if (phase == "history") {
            QFile file(historyFile); QVERIFY(file.open(QIODevice::ReadOnly));
            QDataStream stream(&file); stream >> historyPort >> savedHistory;
            QVERIFY(historyPort); QVERIFY(!savedHistory.isEmpty());
        }
        QTcpServer server;
        int blockedRequests = 0;
        QVERIFY(server.listen(QHostAddress::LocalHost, historyPort));
        connect(&server, &QTcpServer::newConnection, &server, [&server, &blockedRequests] {
            while (auto *socket = server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, socket, [socket, &blockedRequests, request = QByteArray(), responded = false]() mutable {
                    request += socket->readAll();
                    if (responded || !request.contains("\r\n\r\n")) return;
                    responded = true;
                    if (request.contains("/vanilla-blocked-probe")) ++blockedRequests;
                    const QByteArray body = "<!doctype html><meta charset=utf-8><title>Extension test page</title><h1>Extension content-script test</h1>";
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
        const QUrl url(QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort()));
        QWidget window;
        window.setWindowTitle("Vanilla extension smoke - " + kind);
        window.resize(900, 720);
        auto *layout = new QVBoxLayout(&window);
        auto *address = new QHBoxLayout;
        auto *field = new QLineEdit(url.toString(), &window); address->addWidget(field, 1);
        auto *bar = new ExtensionBar(&window); address->addWidget(bar); layout->addLayout(address);
        std::unique_ptr<ViewNode> historyNode;
        auto view = Make(kind, "extension-smoke-" + kind + (phase == "probe" ? QString::number(QDateTime::currentMSecsSinceEpoch()) : QString()));
        QVERIFY(view);
        const auto cleanupView = qScopeGuard([&] {
            view->SetViewNode(nullptr);
            TreeBank::RemoveFromUpdateBox(view);
        });
        auto *widget = qobject_cast<QWidget*>(view->base());
        layout->addWidget(widget, 1);
        bar->SetView(view);
        window.show();
        if (phase == "history") {
            historyNode = std::make_unique<ViewNode>(); historyNode->SetHistoryData(savedHistory);
            view->SetViewNode(historyNode.get());
            view->RestoreHistoryOrLoad(QNetworkRequest(url));
        } else view->TriggerNativeLoadAction(url);
        QTRY_VERIFY_WITH_TIMEOUT(view->Extensions(), 20000);
        ExtensionController *manager = view->Extensions();
        QTRY_VERIFY_WITH_TIMEOUT(manager->IsReady(), 20000);
        QVERIFY2(manager->Error().isEmpty(), qPrintable(manager->Error()));
        for (const auto &row : manager->Rows()) QVERIFY2(row.error.isEmpty(), qPrintable(row.error));
        QTRY_COMPARE_WITH_TIMEOUT(Script(view.get(), "document.title"), QStringLiteral("Extension test page"), 15000);
        const QString marker = "document.documentElement.dataset.vanillaExtension || 'absent'";
        if (phase == "removed") {
            QVERIFY(manager->Rows().isEmpty());
            QCOMPARE(Script(view.get(), marker), QStringLiteral("absent"));
            bar->SetView({}); return;
        }
        QCOMPARE(manager->Rows().size(), registrations.size());
        const QString id = manager->Rows().first().manifest.id;
        if (phase == "disabled") {
            QVERIFY(!manager->Rows().first().enabled);
            QCOMPARE(Script(view.get(), marker), QStringLiteral("absent"));
            ExtensionController::UnregisterPath(fixture);
            QTRY_VERIFY_WITH_TIMEOUT(manager->Rows().isEmpty(), 15000);
            bar->SetView({}); return;
        }
        QVERIFY(manager->Rows().first().enabled);
        QTRY_COMPARE_WITH_TIMEOUT(Script(view.get(), marker), id, 15000);
        if (phase == "probe") {
            Script(view.get(), "fetch('/vanilla-blocked-probe').then(() => document.documentElement.dataset.request='allowed', () => document.documentElement.dataset.request='blocked'); ''");
            QTRY_VERIFY(!Script(view.get(), "document.documentElement.dataset.request || ''").isEmpty());
            qInfo() << kind << "request" << Script(view.get(), "document.documentElement.dataset.request") << "server count" << blockedRequests;
            QCOMPARE(blockedRequests, 0);
            auto *pin = bar->findChild<QToolButton*>("PinnedExtension_" + id); QVERIFY(pin); pin->click();
            QPointer<QWidget> popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
            QPointer<QWidget> content = popup->layout()->itemAt(1)->widget(); QVERIFY(content);
            QTRY_VERIFY_WITH_TIMEOUT(content && content->windowTitle().startsWith("visited=true;opens="), 15000);
            if (kind != "edge") {
                const QString js = "document.title='probe:'+JSON.stringify({message:chrome.i18n.getMessage('probe'),language:chrome.i18n.getUILanguage(),tabsQuery:typeof chrome.tabs?.query,windows:typeof chrome.windows,frames:typeof chrome.webNavigation?.getAllFrames,dnr:typeof chrome.declarativeNetRequest?.getEnabledRulesets})";
                if (auto *web = qobject_cast<QWebEngineView*>(content)) web->page()->runJavaScript(js);
                if (auto *quick = qobject_cast<QQuickWidget*>(content)) {
                    const QString encoded = QString::fromUtf8(QJsonDocument(QJsonArray{js}).toJson(QJsonDocument::Compact));
                    QQmlExpression expression(QQmlEngine::contextForObject(quick->rootObject()), quick->rootObject(), "runJavaScript(" + encoded + "[0])");
                    expression.evaluate(); QVERIFY(!expression.hasError());
                }
                QTRY_VERIFY(content->windowTitle().startsWith("probe:"));
                qInfo() << kind << "API" << content->windowTitle();
                const auto api = QJsonDocument::fromJson(content->windowTitle().mid(6).toUtf8()).object();
                QEXPECT_FAIL("", "Qt 6.11.2 returns an empty extension message catalog (D-313).", Continue);
                QCOMPARE(api.value("message").toString(), QStringLiteral("Localized fixture"));
            } else {
                QTRY_VERIFY(!Script(view.get(), "document.documentElement.dataset.apiProbe || ''").isEmpty());
                qInfo() << kind << "API" << Script(view.get(), "document.documentElement.dataset.apiProbe");
                QTRY_VERIFY_WITH_TIMEOUT(Script(view.get(), "document.documentElement.dataset.tabsProbe || ''").contains("\"reply\""), 10000);
                const QString tabsProbe = Script(view.get(), "document.documentElement.dataset.tabsProbe");
                qInfo() << kind << "tabs" << tabsProbe;
                const QJsonObject tabs = QJsonDocument::fromJson(tabsProbe.toUtf8()).object();
                const QJsonArray current = tabs.value("current").toArray();
                QCOMPARE(current.size(), 1);
                QCOMPARE(current.first().toObject().value("url").toString(), url.toString());
                QVERIFY(current.first().toObject().value("url").toString() != tabs.value("self").toString());
                bool listsPopup = false;
                for (const auto &tab : tabs.value("all").toArray())
                    listsPopup = listsPopup || tab.toObject().value("url").toString() == tabs.value("self").toString();
                QVERIFY(listsPopup);
                QCOMPARE(tabs.value("reply").toString(), QStringLiteral("pong"));
            }
#ifdef Q_OS_WIN
            if (kind == "edge") {
                POINT point{qRound(80 * content->devicePixelRatioF()), qRound(100 * content->devicePixelRatioF())};
                ClientToScreen(reinterpret_cast<HWND>(content->winId()), &point);
                const HWND hit = WindowFromPoint(point);
                const HWND capture = GetCapture();
                const HWND receiver = capture ? capture : hit;
                wchar_t hitClass[128] = {}; wchar_t hitTitle[128] = {};
                GetClassNameW(hit, hitClass, 128); GetWindowTextW(hit, hitTitle, 128);
                qInfo() << "Native click hit/capture/receiver" << hit << capture << receiver
                        << "hit class" << QString::fromWCharArray(hitClass) << "title" << QString::fromWCharArray(hitTitle)
                        << "root" << GetAncestor(hit, GA_ROOT) << "popup" << reinterpret_cast<HWND>(popup->winId());
                QVERIFY2(!capture, "Qt must not capture the mouse away from the WebView2 native child.");
                QVERIFY(receiver);
                QVERIFY(IsChild(reinterpret_cast<HWND>(popup->winId()), receiver) || receiver == reinterpret_cast<HWND>(popup->winId()));
                ScreenToClient(receiver, &point);
                PostMessage(receiver, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point.x, point.y));
                QTest::qWait(50);
                PostMessage(receiver, WM_LBUTTONUP, 0, MAKELPARAM(point.x, point.y));
                QTRY_VERIFY_WITH_TIMEOUT(content && content->windowTitle().startsWith("clicked="), 3000);
                const HWND popupWindow = reinterpret_cast<HWND>(popup->winId());
                QCOMPARE(GetForegroundWindow(), popupWindow);
                INPUT up{}; up.type = INPUT_KEYBOARD; up.ki.wVk = VK_ESCAPE; up.ki.dwFlags = KEYEVENTF_KEYUP;
                QCOMPARE(SendInput(1, &up, sizeof(INPUT)), UINT(1));
                QTest::qWait(150); QVERIFY(popup && popup->isVisible());
                QCOMPARE(GetForegroundWindow(), popupWindow);
                INPUT escape[2]{};
                escape[0].type = INPUT_KEYBOARD; escape[0].ki.wVk = VK_ESCAPE;
                escape[1] = up;
                QCOMPARE(SendInput(2, escape, sizeof(INPUT)), UINT(2));
                QTRY_VERIFY_WITH_TIMEOUT(!popup || !popup->isVisible(), 3000);
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                pin = bar->findChild<QToolButton*>("PinnedExtension_" + id); QVERIFY(pin); pin->click();
                popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
                content = popup->layout()->itemAt(1)->widget();
                QTRY_VERIFY_WITH_TIMEOUT(content && content->windowTitle().startsWith("visited=true;opens="), 15000);
                QVERIFY(popup->isVisible());
                QTRY_VERIFY(popup->isActiveWindow());
                window.activateWindow(); field->setFocus();
                QTRY_VERIFY(!popup || !popup->isVisible());
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                pin = bar->findChild<QToolButton*>("PinnedExtension_" + id); QVERIFY(pin); pin->click();
                popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
            }
#endif
#ifdef EDGEWEBVIEW
            if (kind == "edge") {
                const QString previous = Script(view.get(), "document.documentElement.dataset.tabsProbe");
                bar->ShowList(); QTest::qWait(150);
                auto *more = bar->findChild<QToolButton*>("MoreExtension_" + id); QVERIFY(more); more->click();
                QMenu *menu = more->findChild<QMenu*>(); QVERIFY(menu); QVERIFY(!menu->actions().isEmpty());
                QAction *settings = menu->actions().first(); QVERIFY(settings->isEnabled()); settings->trigger(); menu->close();
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
                content = popup->layout()->itemAt(1)->widget(); QVERIFY(content);
                QTRY_VERIFY_WITH_TIMEOUT(Script(view.get(), "document.documentElement.dataset.tabsProbe") != previous &&
                                         Script(view.get(), "document.documentElement.dataset.tabsProbe").contains("\"reply\""), 15000);
                const QString optionsProbe = Script(view.get(), "document.documentElement.dataset.tabsProbe");
                qInfo() << kind << "options tabs" << optionsProbe;
                const QJsonObject options = QJsonDocument::fromJson(optionsProbe.toUtf8()).object();
                QCOMPARE(options.value("current").toArray().size(), 1);
                QCOMPARE(options.value("current").toArray().first().toObject().value("url").toString(), options.value("self").toString());
                popup->close(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

                const QString notabsPath = ExtensionManifest::Read(fixture + "-notabs").path;
                QString notabs;
                for (const ExtensionRow &row : manager->Rows()) {
                    if (row.manifest.path != notabsPath) continue;
                    notabs = row.manifest.id;
                    QVERIFY(!row.manifest.tabsPermission);
                }
                QVERIFY(!notabs.isEmpty()); QVERIFY(notabs != id);
                QVERIFY(manager->Rows().first().manifest.tabsPermission);
                auto *notabsPin = bar->findChild<QToolButton*>("PinnedExtension_" + notabs); QVERIFY(notabsPin); notabsPin->click();
                popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
                content = popup->layout()->itemAt(1)->widget(); QVERIFY(content);
                QTRY_VERIFY_WITH_TIMEOUT(content && content->windowTitle().startsWith("tabs:"), 15000);
                qInfo() << kind << "no-tabs popup" << content->windowTitle();
                const QJsonArray notabsTabs = QJsonDocument::fromJson(content->windowTitle().mid(5).toUtf8()).array();
                QCOMPARE(notabsTabs.size(), 1);
                QVERIFY(notabsTabs.first().toObject().contains("id"));
                const QString notabsUrl = notabsTabs.first().toObject().value("url").toString();
                QVERIFY2(notabsUrl.isEmpty() || notabsUrl == "chrome-extension://" + notabs + "/popup.html", qPrintable(notabsUrl));
                popup->close(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

                auto *edge = qobject_cast<EdgeWebView*>(view->base()); QVERIFY(edge);
                const QString probeKey = "document.documentElement.dataset.tabsProbe";
                struct AddressCase { QString name; QUrl target; QString html; };
                const QList<AddressCase> addressCases{
                    {"percent", QUrl(url.toString() + QStringLiteral("p%C3%A4th/?q=%E3%81%82%20b&r=あ#f-あ")), QString()},
                    {"idn", QUrl(QStringLiteral("http://日本語.example/")), QString()},
                    {"view-source", QUrl(QStringLiteral("view-source:") + url.toString() + QStringLiteral("source")), QString()},
                    {"string", QUrl(url.toString() + QStringLiteral("string")), QStringLiteral("<!doctype html><title>String document</title><p>string")},
                };
                for (const AddressCase &addressCase : addressCases) {
                    const QString previous = Script(view.get(), probeKey);
                    const QUrl wasAt = edge->ReportedSource();
                    if (addressCase.html.isEmpty()) view->TriggerNativeLoadAction(addressCase.target);
                    else view->setHtml(addressCase.html, addressCase.target);
                    QTRY_VERIFY_WITH_TIMEOUT(edge->ReportedSource() != wasAt, 15000);
                    QTest::qWait(500);
                    const QUrl reported = edge->ReportedSource();
                    const QString location = Script(view.get(), "location.href");
                    pin = bar->findChild<QToolButton*>("PinnedExtension_" + id); QVERIFY(pin); pin->click();
                    popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
                    content = popup->layout()->itemAt(1)->widget(); QVERIFY(content);
                    QTRY_VERIFY_WITH_TIMEOUT(content && content->windowTitle().startsWith("visited=true;opens="), 15000);
                    QTest::qWait(3000);
                    popup->close(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                    view->TriggerNativeLoadAction(url);
                    QTRY_VERIFY_WITH_TIMEOUT(Script(view.get(), probeKey) != previous && Script(view.get(), probeKey).contains("\"reply\""), 15000);
                    const QString record = Script(view.get(), probeKey);
                    qInfo() << kind << "D-335" << addressCase.name << "reported" << reported.toString() << "location" << location << "tabs" << record;
                    const QJsonObject tabs = QJsonDocument::fromJson(record.toUtf8()).object();
                    const QJsonArray current = tabs.value("current").toArray();
                    if (addressCase.name == "view-source") {
                        QVERIFY(current.isEmpty());
                        bool spelledViewSource = false;
                        for (const auto &tab : tabs.value("all").toArray())
                            spelledViewSource = spelledViewSource || tab.toObject().value("url").toString() == addressCase.target.toString();
                        QVERIFY(spelledViewSource);
                        QCOMPARE(tabs.value("reply").toString(), QStringLiteral("none"));
                    } else {
                        QCOMPARE(current.size(), 1);
                        QCOMPARE(QUrl(current.first().toObject().value("url").toString()), reported);
                        if (addressCase.name == "percent") {
                            QCOMPARE(current.first().toObject().value("url").toString(), location);
                            QCOMPARE(tabs.value("reply").toString(), QStringLiteral("pong"));
                        } else {
                            QVERIFY(tabs.value("reply").toString().startsWith("error:"));
                        }
                    }
                }
                Script(view.get(), "document.dispatchEvent(new CustomEvent('vanilla-extension-storage', {detail: JSON.stringify({timeoutProbe: 'arm'})})); ''");
                QTRY_COMPARE_WITH_TIMEOUT(Script(view.get(), "document.documentElement.dataset.timeoutProbe"), QStringLiteral("arm"), 5000);
                pin = bar->findChild<QToolButton*>("PinnedExtension_" + id); QVERIFY(pin); pin->click();
                popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
                content = popup->layout()->itemAt(1)->widget(); QVERIFY(content);
                QTRY_VERIFY_WITH_TIMEOUT(Script(view.get(), "document.documentElement.dataset.timeoutProbe").contains("\"ms\""), 10000);
                const QString timeoutRecord = Script(view.get(), "document.documentElement.dataset.timeoutProbe");
                qInfo() << kind << "D-335 timeout" << timeoutRecord;
                const QJsonObject timeout = QJsonDocument::fromJson(timeoutRecord.toUtf8()).object();
                QCOMPARE(timeout.value("count").toInt(-1), 0);
                QVERIFY(timeout.value("ms").toInt() >= 1900 && timeout.value("ms").toInt() < 4000);
                popup->close(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

                pin = bar->findChild<QToolButton*>("PinnedExtension_" + id); QVERIFY(pin); pin->click();
                popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
            }
#endif
            ExtensionController::SetEnabled(fixture, false);
            QTRY_VERIFY_WITH_TIMEOUT(!manager->Rows().first().enabled && !manager->IsBusy(), 15000);
            QVERIFY(!popup || !popup->isVisible());
            Script(view.get(), "document.documentElement.dataset.request=''; fetch('/vanilla-blocked-probe').then(() => document.documentElement.dataset.request='allowed', () => document.documentElement.dataset.request='blocked'); ''");
            QTRY_COMPARE(Script(view.get(), "document.documentElement.dataset.request"), QStringLiteral("allowed"));
            qInfo() << kind << "disabled server count" << blockedRequests;
            QCOMPARE(blockedRequests, 1);
            bar->SetView({}); return;
        }
        if (kind == "widget" && phase == "exercise") {
            QByteArray history;
            QDataStream historyStream(&history, QIODevice::WriteOnly);
            historyStream << *qobject_cast<WebEngineView*>(view->base())->history();
            QVERIFY(!history.isEmpty());
            QFile file(historyFile); QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file); stream << quint16(server.serverPort()) << history;
            QCOMPARE(stream.status(), QDataStream::Ok);
        }
        if (phase == "history") {
            QVERIFY(qobject_cast<WebEngineView*>(view->base())->history()->count() > 0);
            view->SetViewNode(nullptr);
            ExtensionController::SetEnabled(fixture, false);
            QTRY_VERIFY_WITH_TIMEOUT(!manager->Rows().first().enabled && !manager->IsBusy(), 15000);
            bar->SetView({}); return;
        }
        auto second = Make(kind, "extension-smoke-" + kind);
        QTRY_COMPARE_WITH_TIMEOUT(second->Extensions(), manager, 15000);
        second.reset();

        for (const QString &scheme : {QStringLiteral("dark"), QStringLiteral("light")}) {
            Application::SetColorScheme(scheme);
            auto *extensionsButton = bar->findChild<QToolButton*>("ExtensionsButton"); QVERIFY(extensionsButton);
            QCOMPARE(extensionsButton->icon().pixmap(24, 24).toImage(), ExtensionBar::PuzzleIcon().pixmap(24, 24).toImage());
            bar->ShowList(); QTest::qWait(150);
            auto *panel = bar->findChild<QWidget*>("ExtensionsPanel"); QVERIFY(panel);
            Capture(&window, kind + "-" + scheme + "-bar");
            Capture(panel, kind + "-" + scheme + "-list"); panel->close();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        if (!extra.isEmpty()) {
            const auto extraRow = manager->Rows().at(1);
            auto *extraPin = bar->findChild<QToolButton*>("PinnedExtension_" + extraRow.manifest.id);
            QVERIFY(extraPin); QVERIFY(extraPin->isEnabled()); extraPin->click();
            QTRY_VERIFY(bar->findChild<QWidget*>("ExtensionPopup"));
            QWidget *extraPopup = bar->findChild<QWidget*>("ExtensionPopup");
            QTest::qWait(1500);
            if (auto *engineView = qobject_cast<QWebEngineView*>(extraPopup->layout()->itemAt(1)->widget())) {
                engineView->page()->runJavaScript(QStringLiteral(
                    "JSON.stringify({ready:document.readyState,body:document.body.innerText.length,tabs:typeof chrome.tabs,"
                    "query:typeof chrome.tabs?.query,windows:typeof chrome.windows})"),
                    [](const QVariant &value) { qInfo() << "Extra extension popup API probe:" << value; });
                QTest::qWait(100);
            }
            Capture(extraPopup, kind + "-extra-popup"); extraPopup->close();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        bar->Refresh();
        auto *pin = bar->findChild<QToolButton*>("PinnedExtension_" + id);
        QVERIFY(pin); QVERIFY(pin->isEnabled()); pin->click();
        auto *earlyPopup = bar->findChild<QWidget*>("ExtensionPopup");
        QVERIFY(earlyPopup); earlyPopup->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTest::qWait(100);
        QVERIFY(!bar->findChild<QWidget*>("ExtensionPopup"));
        pin = bar->findChild<QToolButton*>("PinnedExtension_" + id);
        QVERIFY(pin); pin->click();
        QTRY_VERIFY(bar->findChild<QWidget*>("ExtensionPopup"));
        QPointer<QWidget> popup = bar->findChild<QWidget*>("ExtensionPopup");
        QPointer<QWidget> content = popup->layout()->itemAt(1)->widget(); QVERIFY(content);
        QElapsedTimer popupWait; popupWait.start();
        while (popup && content && !(content->windowTitle().startsWith("visited=true;opens=") &&
                                    (kind == "widget" || content->windowTitle().endsWith(";focused=true"))) && popupWait.elapsed() < 15000)
            QTest::qWait(100);
        const bool popupReady = content && content->windowTitle().startsWith("visited=true;opens=") &&
                                (kind == "widget" || content->windowTitle().endsWith(";focused=true"));
        if (!popupReady) {
            qInfo() << "Popup result" << kind << bool(popup) << bool(content)
                    << (popup ? popup->isVisible() : false) << (content ? content->windowTitle() : QString());
            if (content) for (auto *label : content->findChildren<QLabel*>()) qInfo() << "Popup error" << label->text();
            if (content) qInfo() << "Qt focus" << content->hasFocus() << content->focusProxy()
                                << QApplication::focusWidget() << popup->isActiveWindow();
            if (auto *quick = qobject_cast<QQuickWidget*>(content)) {
                qInfo() << "Quick focus" << quick->rootObject()->hasFocus() << quick->rootObject()->hasActiveFocus()
                        << quick->quickWindow()->activeFocusItem() << quick->quickWindow()->isActive();
                qInfo() << "Quick focus windows" << QGuiApplication::focusWindow()
                        << QQuickRenderControl::renderWindowFor(quick->quickWindow()) << popup->windowHandle();
            }
            if (popup) Capture(popup, kind + "-popup-diagnostic");
        }
        QVERIFY(popupReady);
        const int opens = content->windowTitle().section(';', 1, 1).section('=', 1, 1).toInt(); QVERIFY(opens > 0);
        if (kind != "edge") {
            QVERIFY(QApplication::focusWidget());
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_F8);
            QTRY_COMPARE(content->windowTitle(), QStringLiteral("keyboard-probe"));
        }
        Application::SetColorScheme("dark");
        QCOMPARE(popup->palette().color(QPalette::Window), Theme::Color(Theme::PreviewBackground));
        Capture(popup, kind + "-popup");
        popup->close(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        pin = bar->findChild<QToolButton*>("PinnedExtension_" + id);
        QVERIFY(pin); pin->click(); QTRY_VERIFY(bar->findChild<QWidget*>("ExtensionPopup"));
        popup = bar->findChild<QWidget*>("ExtensionPopup"); content = popup->layout()->itemAt(1)->widget();
        if (kind != "widget") {
            auto *editor = new QLineEdit(popup); popup->layout()->addWidget(editor); editor->show(); editor->setFocus();
            QTRY_COMPARE_WITH_TIMEOUT(content ? content->windowTitle() : QString(),
                QStringLiteral("visited=true;opens=%1;focused=false").arg(opens + 1), 15000);
            QVERIFY(editor->hasFocus()); content->setFocus();
        }
        if (kind == "widget") {
            QTRY_VERIFY_WITH_TIMEOUT(content && content->windowTitle().startsWith(
                QStringLiteral("visited=true;opens=%1;focused=").arg(opens + 1)), 15000);
        } else {
            QTRY_COMPARE_WITH_TIMEOUT(content ? content->windowTitle() : QString(),
                QStringLiteral("visited=true;opens=%1;focused=true").arg(opens + 1), 15000);
        }

        if (kind == "quick") {
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Escape);
            QTRY_VERIFY(!popup || !popup->isVisible());
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            pin = bar->findChild<QToolButton*>("PinnedExtension_" + id); QVERIFY(pin); pin->click();
            popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
            QTRY_VERIFY(popup->isActiveWindow());
            window.activateWindow(); field->setFocus();
            QTRY_VERIFY(!popup || !popup->isVisible());
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            pin->click(); popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
            bar->findChild<QToolButton*>("ExtensionsButton")->click();
            QVERIFY(!popup || !popup->isVisible());
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            if (auto *list = bar->findChild<QWidget*>("ExtensionsPanel")) list->close();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            pin = bar->findChild<QToolButton*>("PinnedExtension_" + id); QVERIFY(pin); pin->click();
            popup = bar->findChild<QWidget*>("ExtensionPopup"); QVERIFY(popup);
            view->ApplySpecificSettings({"Private"});
            QVERIFY(!popup || !popup->isVisible()); QVERIFY(!view->Extensions());
            view->ApplySpecificSettings({"!Private"});
            QCOMPARE(view->Extensions(), manager);
            auto initiallyPrivate = Make(kind, "extension-smoke-private-first", true);
            initiallyPrivate->TriggerNativeLoadAction(url);
            QTRY_COMPARE_WITH_TIMEOUT(Script(initiallyPrivate.get(), "document.title"), QStringLiteral("Extension test page"), 15000);
            QCOMPARE(Script(initiallyPrivate.get(), marker), QStringLiteral("absent"));
            initiallyPrivate->ApplySpecificSettings({"!Private"});
            QTRY_VERIFY_WITH_TIMEOUT(initiallyPrivate->Extensions() && initiallyPrivate->Extensions()->IsReady(), 15000);
            QTRY_COMPARE_WITH_TIMEOUT(Script(initiallyPrivate.get(), marker), id, 15000);
            initiallyPrivate.reset();
        }
        if (extra.isEmpty()) {
            ExtensionController::UnregisterPath(fixture);
            QTRY_VERIFY_WITH_TIMEOUT(manager->Rows().isEmpty(), 15000);
            ExtensionController::RegisterPath(fixture);
            QTRY_VERIFY_WITH_TIMEOUT(manager->Rows().size() == 1 && manager->Rows().first().enabled && !manager->IsBusy(), 15000);
        }
        bar->ShowList();
        auto *check = bar->findChild<QCheckBox*>("EnableExtension_" + id);
        QVERIFY(check); QVERIFY(check->isEnabled()); QVERIFY(check->isChecked()); check->click();
        QTRY_VERIFY_WITH_TIMEOUT(!manager->Rows().first().enabled && !manager->IsBusy(), 15000);
        QVERIFY(!popup || !popup->isVisible());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        check = bar->findChild<QCheckBox*>("EnableExtension_" + id);
        QVERIFY(check); QVERIFY(check->isEnabled()); QVERIFY(!check->isChecked()); check->click();
        QTRY_VERIFY_WITH_TIMEOUT(manager->Rows().first().enabled && !manager->IsBusy(), 15000);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        check = bar->findChild<QCheckBox*>("EnableExtension_" + id);
        QVERIFY(check); QVERIFY(check->isEnabled()); QVERIFY(check->isChecked()); check->click();
        QTRY_VERIFY_WITH_TIMEOUT(!manager->Rows().first().enabled && !manager->IsBusy(), 15000);
        if (auto *panel = bar->findChild<QWidget*>("ExtensionsPanel")) panel->close();
        view->TriggerNativeLoadAction(url.resolved(QUrl("/disabled")));
        QTRY_COMPARE_WITH_TIMEOUT(Script(view.get(), "location.pathname"), QStringLiteral("/disabled"), 15000);
        QCOMPARE(Script(view.get(), marker), QStringLiteral("absent"));
        auto later = Make(kind, "extension-smoke-" + kind);
        QTRY_COMPARE_WITH_TIMEOUT(later->Extensions(), manager, 15000);
        QVERIFY(!manager->Rows().first().enabled); later.reset();
        auto privateView = Make(kind, "extension-smoke-private-" + kind, true);
        privateView->TriggerNativeLoadAction(url);
        QTRY_COMPARE_WITH_TIMEOUT(Script(privateView.get(), "document.title"), QStringLiteral("Extension test page"), 15000);
        QVERIFY(!privateView->Extensions());
        QCOMPARE(Script(privateView.get(), marker), QStringLiteral("absent"));
        privateView.reset();
        bar->SetView({});
    }
};

int main(int argc, char **argv) {
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--proxy-server=127.0.0.1:1");
    qputenv("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", "--proxy-server=127.0.0.1:1");
    SettingsSchemeHandler::RegisterScheme();
    QtWebEngineQuick::initialize();
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QTranslator ja;
    const QString translation = QCoreApplication::applicationDirPath() + "/../../translations/vanilla_ja.qm";
    if (ja.load(translation)) app.installTranslator(&ja);
    ExtensionSmoke test;
    const int result = QTest::qExec(&test, argc, argv);
    for (auto *nam : NetworkController::AllNetworkAccessManager()) delete nam;
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return result;
}
#include "extension_smoke.moc"
