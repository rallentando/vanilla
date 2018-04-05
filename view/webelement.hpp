#ifndef WEBELEMENT_HPP
#define WEBELEMENT_HPP

#include "switch.hpp"

#include <QPixmap>
#ifdef WEBKITVIEW
#include <QWebElement>
#endif

#include <memory>

class View;

/*

  contenteditable=true: focus
  textarea: focus
  object: focus
  embed: focus
  frame: focus
  iframe: focus
  input type=text: focsu
  input type=search: focus
  input type=password: focus

  onclick: click
  href=javascript: click
  button: click
  select: click
  label: click
  role=button: click
  role=link: click
  role=menu: click
  role=checkbox: click
  role=radio: click
  role=tab: click
  input type=checkbox: clcik
  input type=radio: click
  input type=file: click
  input type=submit: click
  input type=reset: click
  input type=button: clcik

  href=*: menu

 */

class WebElement{
public:
    explicit WebElement(){
    }
    virtual ~WebElement(){
    }
    enum Action {
        Click,
        Focus,
        Hover,
        None,
    };
    virtual bool SetFocus(){ return false;}
    virtual bool ClickEvent(){ return false;}
    virtual QString TagName() const { return QString();}
    virtual QString InnerText() const { return QString();}
    virtual QUrl BaseUrl() const { return QUrl();}
    virtual QUrl LinkUrl() const { return QUrl();}
    virtual QUrl ImageUrl() const { return QUrl();}
    virtual QString LinkHtml() const { return QString();}
    virtual QString ImageHtml() const { return QString();}
    virtual QPoint Position() const { return QPoint();}
    virtual QRect Rectangle() const { return QRect();}
    virtual QRegion Region() const { return QRegion();}
    virtual void SetPosition(QPoint){}
    virtual void SetRectangle(QRect){}
    virtual void SetText(QString){}
    virtual QPixmap Pixmap(){ return QPixmap();}
    virtual bool IsNull() const { return true;}
    virtual bool IsEditableElement() const { return false;}
    virtual bool IsJsCommandElement() const { return false;}
    virtual bool IsTextInputElement() const { return false;}
    virtual bool IsQueryInputElement() const { return false;}
    virtual bool IsFrameElement() const { return false;}
    virtual bool IsLooped() const { return false;}
    virtual bool IsPaused() const { return false;}
    virtual bool IsMuted() const { return false;}
    virtual Action GetAction() const { return None;}
    virtual bool Equals(const WebElement&) const { return false;}
};

typedef std::shared_ptr<WebElement> SharedWebElement;
typedef QList<std::shared_ptr<WebElement>> SharedWebElementList;
typedef std::function<void(SharedWebElement)> WebElementCallBack;
typedef std::function<void(SharedWebElementList)> WebElementListCallBack;

Q_DECLARE_METATYPE(SharedWebElement);

class JsWebElement : public WebElement{
public:
    explicit JsWebElement();
    explicit JsWebElement(View *provider, QVariant var);
    explicit JsWebElement(View *provider, const QPoint &pos, const QUrl &linkUrl, const QUrl &imageUrl, bool isEditable);
    virtual ~JsWebElement();

    virtual bool SetFocus() Q_DECL_OVERRIDE;
    virtual bool ClickEvent() Q_DECL_OVERRIDE;
    virtual QString TagName() const Q_DECL_OVERRIDE;
    virtual QString InnerText() const Q_DECL_OVERRIDE;
    virtual QUrl BaseUrl() const Q_DECL_OVERRIDE;
    virtual QUrl LinkUrl() const Q_DECL_OVERRIDE;
    virtual QUrl ImageUrl() const Q_DECL_OVERRIDE;
    virtual QString LinkHtml() const Q_DECL_OVERRIDE;
    virtual QString ImageHtml() const Q_DECL_OVERRIDE;
    virtual QPoint Position() const Q_DECL_OVERRIDE;
    virtual QRect Rectangle() const Q_DECL_OVERRIDE;
    virtual QRegion Region() const Q_DECL_OVERRIDE;
    virtual void SetPosition(QPoint) Q_DECL_OVERRIDE;
    virtual void SetRectangle(QRect) Q_DECL_OVERRIDE;
    virtual void SetText(QString) Q_DECL_OVERRIDE;
    virtual QPixmap Pixmap() Q_DECL_OVERRIDE;
    virtual bool IsNull() const Q_DECL_OVERRIDE;
    virtual bool IsEditableElement() const Q_DECL_OVERRIDE;
    virtual bool IsJsCommandElement() const Q_DECL_OVERRIDE;
    virtual bool IsTextInputElement() const Q_DECL_OVERRIDE;
    virtual bool IsQueryInputElement() const Q_DECL_OVERRIDE;
    virtual bool IsFrameElement() const Q_DECL_OVERRIDE;
    virtual bool IsLooped() const Q_DECL_OVERRIDE;
    virtual bool IsPaused() const Q_DECL_OVERRIDE;
    virtual bool IsMuted() const Q_DECL_OVERRIDE;
    virtual Action GetAction() const Q_DECL_OVERRIDE;
    virtual bool Equals(const WebElement&) const Q_DECL_OVERRIDE;

protected:
    View *m_Provider;
    QString m_TagName;
    QString m_InnerText;
    QUrl m_BaseUrl;
    QUrl m_LinkUrl;
    QUrl m_ImageUrl;
    QString m_LinkHtml;
    QString m_ImageHtml;
    QRect m_Rectangle;
    QRegion m_Region;
    bool m_IsJsCommand;
    bool m_IsTextInput;
    bool m_IsQueryInput;
    bool m_IsEditable;
    bool m_IsFrame;
    bool m_IsLooped;
    bool m_IsPaused;
    bool m_IsMuted;
    QString m_XPath;
    QString m_Action;
};

#ifdef WEBKITVIEW
class WebKitElement : public WebElement{
public:
    explicit WebKitElement();
    explicit WebKitElement(QWebElement elem);
    explicit WebKitElement(QWebElement elem, bool editable, QUrl link, QUrl image, QPixmap pixmap);
    virtual ~WebKitElement();

    bool SetFocus() Q_DECL_OVERRIDE;
    bool ClickEvent() Q_DECL_OVERRIDE;
    QString TagName() const Q_DECL_OVERRIDE;
    QString InnerText() const Q_DECL_OVERRIDE;
    QUrl BaseUrl() const Q_DECL_OVERRIDE;
    QUrl LinkUrl() const Q_DECL_OVERRIDE;
    QUrl ImageUrl() const Q_DECL_OVERRIDE;
    QString LinkHtml() const Q_DECL_OVERRIDE;
    QString ImageHtml() const Q_DECL_OVERRIDE;
    QPoint Position() const Q_DECL_OVERRIDE;
    QRect Rectangle() const Q_DECL_OVERRIDE;
    void SetPosition(QPoint pos) Q_DECL_OVERRIDE;
    void SetRectangle(QRect rect) Q_DECL_OVERRIDE;
    void SetText(QString text) Q_DECL_OVERRIDE;
    QPixmap Pixmap() Q_DECL_OVERRIDE;
    bool IsNull() const Q_DECL_OVERRIDE;
    bool IsJsCommandElement() const Q_DECL_OVERRIDE;
    bool IsTextInputElement() const Q_DECL_OVERRIDE;
    bool IsQueryInputElement() const Q_DECL_OVERRIDE;
    bool IsEditableElement() const Q_DECL_OVERRIDE;
    bool IsFrameElement() const Q_DECL_OVERRIDE;
    bool IsLooped() const Q_DECL_OVERRIDE;
    bool IsPaused() const Q_DECL_OVERRIDE;
    bool IsMuted() const Q_DECL_OVERRIDE;
    Action GetAction() const Q_DECL_OVERRIDE;
    bool Equals(const WebElement &other) const Q_DECL_OVERRIDE;

private:
    QWebElement m_Element;
    bool m_IsEditable;
    QUrl m_LinkUrl;
    QUrl m_ImageUrl;
    QPixmap m_Pixmap;
    bool m_CoordinateOverridden;
    QRect m_OverriddenRectangle;
};
#endif //ifndef WEBKITVIEW

#endif //ifndef WEBELEMENT_HPP
