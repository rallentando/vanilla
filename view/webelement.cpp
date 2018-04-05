#include "switch.hpp"
#include "const.hpp"

#include "webelement.hpp"

#include <QPixmap>
#include <QPainter>
#ifdef WEBKITVIEW
#include <QWebElement>
#include <QWebFrame>
#endif

#include "view.hpp"

JsWebElement::JsWebElement()
    : WebElement()
{
    m_Provider     = 0;
    m_TagName      = QString();
    m_InnerText    = QString();
    m_BaseUrl      = QUrl();
    m_LinkUrl      = QUrl();
    m_ImageUrl     = QUrl();
    m_LinkHtml     = QString();
    m_ImageHtml    = QString();
    m_Rectangle    = QRect();
    m_IsEditable   = false;
    m_IsJsCommand  = false;
    m_IsTextInput  = false;
    m_IsQueryInput = false;
    m_IsFrame      = false;
    m_IsLooped     = false;
    m_IsPaused     = false;
    m_IsMuted      = false;
    m_XPath        = QString();
    m_Action       = QStringLiteral("None");
}

JsWebElement::JsWebElement(View *provider, QVariant var)
    : WebElement()
{
    QVariantMap map = var.toMap();
    m_Provider  = provider;
    m_TagName   = map[QStringLiteral("tagName")].toString();
    m_InnerText = map[QStringLiteral("innerText")].toString();
    m_BaseUrl   = Page::StringToUrl(map[QStringLiteral("baseUrl")].toString());
    m_LinkUrl   = Page::StringToUrl(map[QStringLiteral("linkUrl")].toString(), m_BaseUrl);
    m_ImageUrl  = Page::StringToUrl(map[QStringLiteral("imageUrl")].toString(), m_BaseUrl);
    m_LinkHtml  = map[QStringLiteral("linkHtml")].toString();
    m_ImageHtml = map[QStringLiteral("imageHtml")].toString();
    m_Rectangle = QRect(map[QStringLiteral("x")].toInt(),     map[QStringLiteral("y")].toInt(),
                        map[QStringLiteral("width")].toInt(), map[QStringLiteral("height")].toInt());
    m_IsEditable   = map[QStringLiteral("isEditable")].toBool();
    m_IsJsCommand  = map[QStringLiteral("isJsCommand")].toBool();
    m_IsTextInput  = map[QStringLiteral("isTextInput")].toBool();
    m_IsQueryInput = map[QStringLiteral("isQueryInput")].toBool();
    m_IsFrame      = map[QStringLiteral("isFrame")].toBool();
    m_IsLooped     = map[QStringLiteral("isLooped")].toBool();
    m_IsPaused     = map[QStringLiteral("isPaused")].toBool();
    m_IsMuted      = map[QStringLiteral("isMuted")].toBool();
    m_XPath        = map[QStringLiteral("xPath")].toString();
    m_Action       = map[QStringLiteral("action")].toString();

    m_Region = QRegion();
    QVariantMap regionMap = map[QStringLiteral("region")].toMap();
    QRect viewport = QRect(QPoint(), m_Provider->size());
    foreach(QString key, regionMap.keys()){
        QVariantMap m = regionMap[key].toMap();
        m_Region |= QRect(m["x"].toInt(),
                          m["y"].toInt(),
                          m["width"].toInt(),
                          m["height"].toInt()).intersected(viewport);
    }
}

JsWebElement::JsWebElement(View *provider, const QPoint &pos, const QUrl &linkUrl, const QUrl &imageUrl, bool isEditable)
    : JsWebElement()
{
    m_Provider = provider;
    m_LinkUrl = linkUrl;
    m_ImageUrl = imageUrl;
    m_IsEditable = isEditable;
    m_Rectangle = QRect(pos, QSize(1, 1));
}

JsWebElement::~JsWebElement(){
}

bool JsWebElement::SetFocus(){
    if(m_Provider){
        QMetaObject::invokeMethod(m_Provider->base(), "SetFocusToElement",
                                  Q_ARG(QString, m_XPath));
        return true;
    }
    return false;
}

bool JsWebElement::ClickEvent(){
    if(m_Provider){
        QMetaObject::invokeMethod(m_Provider->base(), "FireClickEvent",
                                  Q_ARG(QString, m_XPath),
                                  Q_ARG(QPoint, Position()));
        return true;
    }
    return false;
}

QString JsWebElement::TagName() const {
    return m_TagName;
}

QString JsWebElement::InnerText() const {
    return m_InnerText;
}

QUrl JsWebElement::BaseUrl() const {
    return m_BaseUrl;
}

QUrl JsWebElement::LinkUrl() const {
    return m_LinkUrl;
}

QUrl JsWebElement::ImageUrl() const {
    return m_ImageUrl;
}

QString JsWebElement::LinkHtml() const {
    if(!m_LinkHtml.isEmpty()) return m_LinkHtml;
    return QStringLiteral("<a href=\"") + m_LinkUrl.toString() + QStringLiteral("\"></a>");
}

QString JsWebElement::ImageHtml() const {
    if(!m_ImageHtml.isEmpty()) return m_ImageHtml;
    return QStringLiteral("<img src=\"") + m_ImageUrl.toString() + QStringLiteral("\">");
}

QPoint JsWebElement::Position() const {
    return m_Rectangle.center();
}

QRect JsWebElement::Rectangle() const {
    return m_Rectangle;
}

QRegion JsWebElement::Region() const {
    return m_Region;
}

void JsWebElement::SetPosition(QPoint pos){
    m_Rectangle.moveCenter(pos);
}

void JsWebElement::SetRectangle(QRect rect){
    m_Rectangle = rect;
}

void JsWebElement::SetText(QString text){
    if(m_Provider){
        QMetaObject::invokeMethod(m_Provider->base(), "SetTextValue",
                                  Q_ARG(QString, m_XPath),
                                  Q_ARG(QString, text));
    }
}

QPixmap JsWebElement::Pixmap(){
    if(!m_Provider || IsNull()) return QPixmap();
    QPixmap pixmap(m_Provider->size());
    QPainter painter(&pixmap);
    QRect r;
    if(!Region().isNull()){
        m_Provider->Render(&painter, Region());
        r = Region().boundingRect();
    } else if(!Rectangle().isNull()){
        m_Provider->Render(&painter, Rectangle());
        r = Rectangle();
    }
    painter.end();
    return pixmap.copy(r);
}

bool JsWebElement::IsNull() const {
    return Position().isNull() || Rectangle().isNull();
}

bool JsWebElement::IsEditableElement() const {
    return m_IsEditable;
}

bool JsWebElement::IsJsCommandElement() const {
    return m_IsJsCommand;
}

bool JsWebElement::IsTextInputElement() const {
    return m_IsTextInput;
}

bool JsWebElement::IsQueryInputElement() const {
    return m_IsQueryInput;
}

bool JsWebElement::IsFrameElement() const {
    return m_IsFrame;
}

bool JsWebElement::IsLooped() const {
    return m_IsLooped;
}

bool JsWebElement::IsPaused() const {
    return m_IsPaused;
}

bool JsWebElement::IsMuted() const {
    return m_IsMuted;
}

WebElement::Action JsWebElement::GetAction() const {
    if(m_Action == QStringLiteral("Focus")) return Focus;
    if(m_Action == QStringLiteral("Click")) return Click;
    if(m_Action == QStringLiteral("Hover")) return Hover;
    return None;
}

bool JsWebElement::Equals(const WebElement &other) const {
    return m_XPath == static_cast<const JsWebElement*>(&other)->m_XPath;
}

#ifdef WEBKITVIEW
WebKitElement::WebKitElement()
    : WebElement()
{
    m_Element = QWebElement();
    m_IsEditable = false;
    m_LinkUrl = QUrl();
    m_ImageUrl = QUrl();
    m_Pixmap = QPixmap();
    m_CoordinateOverridden = false;
    m_OverriddenRectangle = QRect();
}

WebKitElement::WebKitElement(QWebElement elem)
    : WebElement()
{
    m_Element = elem;
    m_IsEditable = false;
    m_LinkUrl = QUrl();
    m_ImageUrl = QUrl();
    m_Pixmap = QPixmap();
    m_CoordinateOverridden = false;
    m_OverriddenRectangle = QRect();

    while(!elem.isNull()){
        if(elem.attribute(QStringLiteral("contenteditable")) == QStringLiteral("true")){
            m_IsEditable = true;
            break;
        }
        elem = elem.parent();
    }
}

WebKitElement::WebKitElement(QWebElement elem, bool editable, QUrl link, QUrl image, QPixmap pixmap)
    : WebElement()
{
    m_Element = elem;
    m_IsEditable = editable;
    m_LinkUrl = link;
    m_ImageUrl = image;
    m_Pixmap = pixmap;
    m_CoordinateOverridden = false;
    m_OverriddenRectangle = QRect();
}

WebKitElement::~WebKitElement(){
}

bool WebKitElement::SetFocus(){
    m_Element.setFocus();
    return true;
}

bool WebKitElement::ClickEvent(){
    return false;
}

QString WebKitElement::TagName() const {
    return m_Element.tagName();
}

QString WebKitElement::InnerText() const {
    return m_Element.toPlainText();
}

QUrl WebKitElement::BaseUrl() const {
    return m_Element.webFrame()->baseUrl();
}

QUrl WebKitElement::LinkUrl() const {
    if(m_LinkUrl.isEmpty()){
        QWebElement elem = m_Element;
        while(!elem.isNull()){
            QString href = elem.attribute(QStringLiteral("href"));
            if(!href.isEmpty())
                return Page::StringToUrl(href, elem.webFrame()->baseUrl());
            elem = elem.parent();
        }
    }
    return m_LinkUrl;
}

QUrl WebKitElement::ImageUrl() const {
    if(m_ImageUrl.isEmpty()){
        QWebElement elem = m_Element;
        while(!elem.isNull()){
            QString src = elem.attribute(QStringLiteral("src"));
            if(!src.isEmpty())
                return Page::StringToUrl(src, elem.webFrame()->baseUrl());
            elem = elem.parent();
        }
    }
    return m_ImageUrl;
}

QString WebKitElement::LinkHtml() const {
    QWebElement elem = m_Element;
    while(!elem.isNull()){
        QString href = elem.attribute(QStringLiteral("href"));
        if(!href.isEmpty()) return elem.toOuterXml();
        elem = elem.parent();
    }
    return QString();
}

QString WebKitElement::ImageHtml() const {
    QWebElement elem = m_Element;
    while(!elem.isNull()){
        QString src = elem.attribute(QStringLiteral("src"));
        if(!src.isEmpty()) return elem.toOuterXml();
        elem = elem.parent();
    }
    return QString();
}

QPoint WebKitElement::Position() const {
    if(m_CoordinateOverridden)
        return m_OverriddenRectangle.center();
    return Rectangle().center();
}

QRect WebKitElement::Rectangle() const {
    if(m_CoordinateOverridden)
        return m_OverriddenRectangle;
    QRect r = m_Element.geometry();
    QWebFrame *f = m_Element.webFrame();
    while(f){
        r.translate(-f->scrollPosition());
        r.translate(f->geometry().topLeft());
        r = r.intersected(f->geometry());
        f = f->parentFrame();
    }
    return r;
}

void WebKitElement::SetPosition(QPoint pos){
    m_CoordinateOverridden = true;
    m_OverriddenRectangle.moveCenter(pos);
}

void WebKitElement::SetRectangle(QRect rect){
    m_CoordinateOverridden = true;
    m_OverriddenRectangle = rect;
}

void WebKitElement::SetText(QString text){
    m_Element.setAttribute("value", text);
}

QPixmap WebKitElement::Pixmap(){
    if(m_Pixmap.isNull()){
        QPixmap pixmap(m_Element.geometry().size());
        pixmap.fill(QColor(255,255,255,0));
        QImage before = pixmap.toImage();
        QPainter painter(&pixmap);
        m_Element.render(&painter);
        painter.end();
        if(pixmap.toImage() == before)
            return QPixmap();
        return pixmap;
    }
    return m_Pixmap;
}

bool WebKitElement::IsNull() const {
    return m_Element.isNull() || Rectangle().isNull() || Position().isNull();
}

bool WebKitElement::IsJsCommandElement() const {
    QString onclick = m_Element.attribute(QStringLiteral("onclick"));
    QString href = m_Element.attribute(QStringLiteral("href")).toLower();
    QString role = m_Element.attribute(QStringLiteral("role")).toLower();
    return !onclick.isEmpty() ||
        href.startsWith(QStringLiteral("javascript:")) ||
        role == QStringLiteral("button") ||
        role == QStringLiteral("link") ||
        role == QStringLiteral("menu") ||
        role == QStringLiteral("checkbox") ||
        role == QStringLiteral("radio") ||
        role == QStringLiteral("tab");
}

bool WebKitElement::IsTextInputElement() const {
    QString tag = m_Element.tagName().toLower();
    QString type = m_Element.attribute(QStringLiteral("type")).toLower();
    return tag == QStringLiteral("textaret") ||
        (tag == QStringLiteral("input") &&
         (type == QStringLiteral("text") ||
          type == QStringLiteral("search") ||
          type == QStringLiteral("password")));
}

bool WebKitElement::IsQueryInputElement() const {
    QString tag = m_Element.tagName().toLower();
    QString type = m_Element.attribute(QStringLiteral("type")).toLower();
    return tag == QStringLiteral("input") &&
        (type == QStringLiteral("text") ||
         type == QStringLiteral("search"));
}

bool WebKitElement::IsEditableElement() const {
    return m_IsEditable
        || IsTextInputElement()
        || IsQueryInputElement();
}

bool WebKitElement::IsFrameElement() const {
    QString tag = m_Element.tagName().toLower();
    return tag == QStringLiteral("frame")
        || tag == QStringLiteral("iframe");
}

bool WebKitElement::IsLooped() const {
    return m_Element.attribute(QStringLiteral("loop")).toLower() == QStringLiteral("true");
}

bool WebKitElement::IsPaused() const {
    return m_Element.attribute(QStringLiteral("paused")).toLower() == QStringLiteral("true");
}

bool WebKitElement::IsMuted() const {
    return m_Element.attribute(QStringLiteral("muted")).toLower() == QStringLiteral("true");
}

WebElement::Action WebKitElement::GetAction() const {
    QString tag = m_Element.tagName().toLower();
    QString type = m_Element.attribute(QStringLiteral("type")).toLower();
    QString onclick = m_Element.attribute(QStringLiteral("onclick"));
    QString onhover = m_Element.attribute(QStringLiteral("onmouseover"));
    QString href = m_Element.attribute(QStringLiteral("href")).toLower();
    QString role = m_Element.attribute(QStringLiteral("role")).toLower();

    if(href.startsWith(QStringLiteral("http:")) ||
       href.startsWith(QStringLiteral("https:"))){

        return None;
    }
    if(m_IsEditable ||
       tag == QStringLiteral("textaret") ||
       tag == QStringLiteral("object") ||
       tag == QStringLiteral("embed") ||
       tag == QStringLiteral("frame") ||
       tag == QStringLiteral("iframe") ||
       (tag == QStringLiteral("input") &&
        (type == QStringLiteral("text") ||
         type == QStringLiteral("search") ||
         type == QStringLiteral("password")))){

        return Focus;
    }
    if(!onclick.isEmpty() ||
       href.startsWith(QStringLiteral("javascript:")) ||
       tag == QStringLiteral("button") ||
       tag == QStringLiteral("select") ||
       tag == QStringLiteral("label") ||
       role == QStringLiteral("button") ||
       role == QStringLiteral("link") ||
       role == QStringLiteral("menu") ||
       role == QStringLiteral("checkbox") ||
       role == QStringLiteral("radio") ||
       role == QStringLiteral("tab") ||
       (tag == QStringLiteral("input") &&
        (type == QStringLiteral("checkbox") ||
         type == QStringLiteral("radio") ||
         type == QStringLiteral("file") ||
         type == QStringLiteral("submit") ||
         type == QStringLiteral("reset") ||
         type == QStringLiteral("button")))){

        return Click;
    }
    if(!onhover.isEmpty()){

        return Hover;
    }
    return None;
}

bool WebKitElement::Equals(const WebElement &other) const {
    return m_Element == static_cast<const WebKitElement*>(&other)->m_Element;
}
#endif //ifndef WEBKITVIEW
