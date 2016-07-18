#ifndef WEBENGINEPAGE_HPP
#define WEBENGINEPAGE_HPP

#include "switch.hpp"

#ifdef WEBENGINEVIEW

#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineFullScreenRequest>

#include <memory>

#include "page.hpp"
#include "application.hpp"
#include "dialog.hpp"

class QMenu;
class QObject;
class QAction;
class QMenu;
class View;
class TreeBank;
class MainWindow;

class ViewNode;
class HistNode;
class WebElement;

typedef std::shared_ptr<WebElement> SharedWebElement;
typedef QList<std::shared_ptr<WebElement>> SharedWebElementList;

class WebEnginePage : public QWebEnginePage{
    Q_OBJECT

public:
    WebEnginePage(NetworkAccessManager *nam, QObject *parent = 0);
    ~WebEnginePage();
    View* GetView();
    WebEnginePage* createWindow(WebWindowType type) Q_DECL_OVERRIDE;
    void triggerAction(WebAction action, bool checked = false) Q_DECL_OVERRIDE;

    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) Q_DECL_OVERRIDE;
    QStringList chooseFiles(FileSelectionMode mode, const QStringList &oldFiles,
                            const QStringList &acceptedMimeTypes) Q_DECL_OVERRIDE;
    bool certificateError(const QWebEngineCertificateError& error) Q_DECL_OVERRIDE;

    void javaScriptAlert(const QUrl &securityOrigin, const QString &msg) Q_DECL_OVERRIDE {
        //QWebEnginePage::javaScriptAlert(securityOrigin, msg);
        Q_UNUSED(securityOrigin);
        ModalDialog::Information(QStringLiteral("Javascript Alert"), msg);
    }
    bool javaScriptConfirm(const QUrl &securityOrigin, const QString &msg) Q_DECL_OVERRIDE {
        //return QWebEnginePage::javaScriptConfirm(securityOrigin, msg);
        Q_UNUSED(securityOrigin);
        return ModalDialog::Question(QStringLiteral("Javascript Confirm"), msg);
    }
    bool javaScriptPrompt(const QUrl &securityOrigin, const QString &msg,
                          const QString &defaultValue, QString *result) Q_DECL_OVERRIDE {
        //return QWebEnginePage::javaScriptPrompt(securityOrigin, msg, defaultValue, result);
        Q_UNUSED(securityOrigin);
        bool ok = true;
        if(result) *result = ModalDialog::GetText(QStringLiteral("Javascript Prompt"), msg, defaultValue, &ok);
        return ok;
    }
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &msg,
                                  int lineNumber, const QString &sourceId) Q_DECL_OVERRIDE;

    QNetworkAccessManager *networkAccessManager() const;
    void setNetworkAccessManager(QNetworkAccessManager *nam);
    QString userAgentForUrl(const QUrl &url) const;

    void DisplayContextMenu(QWidget *parent, SharedWebElement elem,
                            QPoint localPos, QPoint globalPos);

    void TriggerAction(Page::CustomAction, QVariant = QVariant());
    QAction *Action(Page::CustomAction, QVariant = QVariant());

public slots:
    void Download(const QNetworkRequest &req,
                  const QString &file = QString()){
        m_Page->Download(req, file);
    }
    void Download(const QUrl &target,
                  const QUrl &referer,
                  const QString &file = QString()){
        m_Page->Download(target, referer, file);
    }
    void Download(const QString &url,
                  const QString &file = QString()){
        m_Page->Download(url, file);
    }

    void OnLinkHovered(const QString &);
    void HandleFeaturePermission(const QUrl&, QWebEnginePage::Feature);
    void HandleAuthentication(const QUrl&, QAuthenticator*);
    void HandleProxyAuthentication(const QUrl&, QAuthenticator*, const QString&);
    void HandleFullScreen(QWebEngineFullScreenRequest request);
    void HandleProcessTermination(RenderProcessTerminationStatus status, int code);
#if QT_VERSION >= 0x050700
    void HandleContentsSizeChange(const QSizeF &size);
    void HandleScrollPositionChange(const QPointF &pos);
#endif

    void HandleUnsupportedContent(QNetworkReply *reply);

    void AddJsObject();

    View *OpenInNew(QUrl url){ return m_Page->OpenInNew(url);}
    View *OpenInNew(QList<QUrl> urls){ return m_Page->OpenInNew(urls);}
    View *OpenInNew(QString query){ return m_Page->OpenInNew(query);}
    View *OpenInNew(QString key, QString query){ return m_Page->OpenInNew(key, query);}
    View *OpenInNewBackground(QUrl url){ return m_Page->OpenInNewViewNodeBackground(url);}
    View *OpenInNewBackground(QList<QUrl> urls){ return m_Page->OpenInNewViewNodeBackground(urls);}
    View *OpenInNewBackground(QString query){ return m_Page->OpenInNewViewNodeBackground(query);}
    View *OpenInNewBackground(QString key, QString query){ return m_Page->OpenInNewViewNodeBackground(key, query);}

    void SetSource(const QUrl &url){ m_Page->SetSource(url);}
    void SetSource(const QByteArray &html){ m_Page->SetSource(html);}
    void SetSource(const QString &html){ m_Page->SetSource(html);}

    void InspectElement();
    void AddSearchEngine(QPoint);
    void AddBookmarklet(QPoint);

    void CloseLater();

signals:
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void linkHovered(const QString&, const QString&, const QString&);
    void ViewChanged();
    void ScrollChanged(QPointF);

public slots:
    void DownloadSuggest(const QUrl&);
signals:
    void SuggestResult(const QByteArray&);

private:
    View *m_View;
    Page *m_Page;
    QNetworkAccessManager *m_NetworkAccessManager;
};

#endif //ifdef WEBENGINEVIEW
#endif //ifndef WEBENGINEPAGE_HPP
