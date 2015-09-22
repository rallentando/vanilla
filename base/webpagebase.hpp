#ifndef WEBPAGEBASE_HPP
#define WEBPAGEBASE_HPP

#include "switch.hpp"

#include <QWebPageBase>
#include <QWebSettingsBase>

//[[!WEV]]
#include <QWebFrameBase>
//[[/!WEV]]

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

class WebPageBase : public QWebPageBase{
    Q_OBJECT

public:
    WebPageBase(NetworkAccessManager *nam, QObject *parent = 0);
    ~WebPageBase();
    View* GetView();
    WebPageBase* createWindow(WebWindowType type) DECL_OVERRIDE;
    void triggerAction(WebAction action, bool checked = false) DECL_OVERRIDE;

    //[[!WEV]]
    bool acceptNavigationRequest(QWebFrameBase *frame,
                                 const QNetworkRequest &request,
                                 NavigationType type) DECL_OVERRIDE;

    QObject* createPlugin(const QString &id, const QUrl &url,
                          const QStringList &names,
                          const QStringList &values) DECL_OVERRIDE;

    QString userAgentForUrl(const QUrl &url) const DECL_OVERRIDE;
    QString chooseFile(QWebFrameBase *parentframe, const QString &suggested) DECL_OVERRIDE;
    bool extension(Extension extension, const ExtensionOption *option = 0, ExtensionReturn *output = 0) DECL_OVERRIDE;
    bool supportsExtension(Extension extension) const DECL_OVERRIDE;

    void javaScriptAlert(QWebFrameBase *frame, const QString &msg) DECL_OVERRIDE {
        //QWebPageBase::javaScriptAlert(frame, msg);
        Q_UNUSED(frame);
        ModalDialog::Information(QStringLiteral("Javascript Alert"), msg);
    }
    bool javaScriptConfirm(QWebFrameBase *frame, const QString & msg) DECL_OVERRIDE {
        //return QWebPageBase::javaScriptConfirm(frame, msg);
        Q_UNUSED(frame);
        return ModalDialog::Question(QStringLiteral("Javascript Confirm"), msg);
    }
    bool javaScriptPrompt(QWebFrame *frame, const QString &msg, const QString &defaultValue, QString *result) DECL_OVERRIDE {
        //return QWebPageBase::javaScriptPrompt(frame, msg, defaultValue, result);
        Q_UNUSED(frame);
        bool ok = true;
        if(result) *result = ModalDialog::GetText(QStringLiteral("Javascript Prompt"), msg, defaultValue, &ok);
        return ok;
    }
    void javaScriptConsoleMessage(const QString &msg, int lineNumber, const QString &sourceID) DECL_OVERRIDE {
        QWebPageBase::javaScriptConsoleMessage(msg, lineNumber, sourceID);
    }
    //[[/!WEV]]
    //[[WEV]]
    QStringList chooseFiles(FileSelectionMode mode, const QStringList &oldFiles,
                            const QStringList &acceptedMimeTypes) DECL_OVERRIDE;
    bool certificateError(const QWebEngineCertificateError& error) DECL_OVERRIDE;

    void javaScriptAlert(const QUrl &securityOrigin, const QString &msg) DECL_OVERRIDE {
        //QWebPageBase::javaScriptAlert(securityOrigin, msg);
        Q_UNUSED(securityOrigin);
        ModalDialog::Information(QStringLiteral("Javascript Alert"), msg);
    }
    bool javaScriptConfirm(const QUrl &securityOrigin, const QString &msg) DECL_OVERRIDE {
        //return QWebPageBase::javaScriptConfirm(securityOrigin, msg);
        Q_UNUSED(securityOrigin);
        return ModalDialog::Question(QStringLiteral("Javascript Confirm"), msg);
    }
    bool javaScriptPrompt(const QUrl &securityOrigin, const QString &msg,
                          const QString &defaultValue, QString *result) DECL_OVERRIDE {
        //return QWebPageBase::javaScriptPrompt(securityOrigin, msg, defaultValue, result);
        Q_UNUSED(securityOrigin);
        bool ok = true;
        if(result) *result = ModalDialog::GetText(QStringLiteral("Javascript Prompt"), msg, defaultValue, &ok);
        return ok;
    }
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &msg,
                                  int lineNumber, const QString &sourceID) DECL_OVERRIDE {
        QWebPageBase::javaScriptConsoleMessage(level, msg, lineNumber, sourceID);
    }

    QNetworkAccessManager *networkAccessManager() const;
    void setNetworkAccessManager(QNetworkAccessManager *nam);
    QString userAgentForUrl(const QUrl &url) const;
    //[[/WEV]]

    void DisplayContextMenu(QWidget *parent, SharedWebElement elem,
                            QPoint localPos, QPoint globalPos);

    void TriggerAction(QWebPageBase::WebAction);
    void TriggerAction(Page::CustomAction, QVariant = QVariant());
    QAction *Action(QWebPageBase::WebAction);
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

    //[[WEV]]
    void OnLinkHovered(const QString &);
    void HandleFeaturePermission(const QUrl&, QWebEnginePage::Feature);
    void HandleAuthentication(const QUrl&, QAuthenticator*);
    void HandleProxyAuthentication(const QUrl&, QAuthenticator*, const QString&);
    //[[/WEV]]

    void HandleUnsupportedContent(QNetworkReply *reply);

    void AddJsObject();

    void CleanUpHtml();

    View *OpenInNew(QUrl url){ return m_Page->OpenInNew(url);}
    View *OpenInNew(QList<QUrl> urls){ return m_Page->OpenInNew(urls);}
    View *OpenInNew(QString query){ return m_Page->OpenInNew(query);}
    View *OpenInNew(QString key, QString query){ return m_Page->OpenInNew(key, query);}

    void SetSource(const QUrl &url){ m_Page->SetSource(url);}
    void SetSource(const QByteArray &html){ m_Page->SetSource(html);}
    void SetSource(const QString &html){ m_Page->SetSource(html);}

    void Print();
    void AddSearchEngine(QPoint);
    void AddBookmarklet(QPoint);
    void InspectElement();
    void ReloadAndBypassCache();

    void CloseLater();

signals:
    //[[WEV]]
    void statusBarMessage(const QString&);
    //[[/WEV]]
    void statusBarMessage2(const QString&, const QString&);
    //[[WEV]]
    void linkHovered(const QString&, const QString&, const QString&);
    //[[/WEV]]
    void ViewChanged();
    void ScrollChanged(QPointF);
    void ButtonCleared();
    void RenderFinished();
    //[[!WEV]]
    void urlChanged(const QUrl&);
    void titleChanged(const QString&);
    //[[/!WEV]]

public slots:
    void DownloadSuggest(const QUrl&);
signals:
    void SuggestResult(const QByteArray&);

private:
    View *m_View;
    Page *m_Page;
    //[[WEV]]
    QNetworkAccessManager *m_NetworkAccessManager;
    //[[/WEV]]
};

#endif
