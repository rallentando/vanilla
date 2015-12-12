#include "switch.hpp"
#include "const.hpp"

#include "accessiblewebelement.hpp"

#include <QGraphicsItem>
#include <QUrl>
#include <QPainter>
#include <QFontMetrics>
#include <QKeySequence>
#include <QMap>
#include <QtConcurrent/QtConcurrent>

#include "view.hpp"
#include "gadgets.hpp"
#include "gadgetsstyle.hpp"

/*
  element type
  current block or not.
  selected or not.
  frame or link.

  standard link : small char chip only.
  selected link : it's url and explanation of key bind.
  standard frame or iframe : large char chip only.
  selected frame or iframe : it's url and explanation of key bind.
 */

QFontMetrics AccessibleWebElement::m_InfoMetrics(ACCESSKEY_INFO_FONT);
QFontMetrics AccessibleWebElement::m_SmallMetrics(ACCESSKEY_CHAR_CHIP_S_FONT);
QFontMetrics AccessibleWebElement::m_MediumMetrics(ACCESSKEY_CHAR_CHIP_M_FONT);
QFontMetrics AccessibleWebElement::m_LargeMetrics(ACCESSKEY_CHAR_CHIP_L_FONT);

AccessibleWebElement::AccessibleWebElement(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    m_Gadgets = static_cast<Gadgets*>(parent);
    m_Index = -1;
    m_Element = SharedWebElement();
    m_Pos = QPoint();

    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setZValue(MAIN_CONTENTS_LAYER);
}

AccessibleWebElement::~AccessibleWebElement(){}

void AccessibleWebElement::Initialize(){
    // settings
}

QRectF AccessibleWebElement::boundingRect() const{
    if(m_Pos.isNull() || !m_Element || m_Element->IsNull() || m_Index == -1)
        return QRectF();

    QUrl url = m_Element->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    if(!IsSelected()){
        return QRectF(CharChipRect());
    } else {
        int width = m_InfoMetrics.boundingRect(str).width();
        // is there any good way?
        width = width + 15 - str.length()*0.4;

        if(width > ACCESSKEY_INFO_MAX_WIDTH)
            width = ACCESSKEY_INFO_MAX_WIDTH;

        QPoint base(m_Pos - QPoint(width/2, ACCESSKEY_INFO_HEIGHT/2));
        QRect rect = QRect(base, QSize(width, ACCESSKEY_INFO_HEIGHT));

        QMap<QString, QRect> keyrectmap = KeyRects();
        QMap<QString, QRect> exprectmap = ExpRects();

        foreach(QString action, m_Gadgets->GetAccessKeyKeyMap().values().toSet()){
            rect = rect.united(keyrectmap[action]);
            rect = rect.united(exprectmap[action]);
        }
        return QRectF(rect);
    }
    return QRectF();
}

QPainterPath AccessibleWebElement::shape() const{
    QPainterPath path;
    if(m_Pos.isNull() || !m_Element || m_Element->IsNull()) return path;

    QUrl url = m_Element->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    if(!IsSelected()){
        path.addRect(CharChipRect());
    } else {
        int width = m_InfoMetrics.boundingRect(str).width();
        // is there any good way?
        width = width + 15 - str.length()*0.4;

        if(width > ACCESSKEY_INFO_MAX_WIDTH)
            width = ACCESSKEY_INFO_MAX_WIDTH;

        QPoint base(m_Pos - QPoint(width/2, ACCESSKEY_INFO_HEIGHT/2));
        path.addRect(QRect(base, QSize(width, ACCESSKEY_INFO_HEIGHT)));

        QMap<QString, QRect> keyrectmap = KeyRects();
        QMap<QString, QRect> exprectmap = ExpRects();

        foreach(QString action, m_Gadgets->GetAccessKeyKeyMap().values().toSet()){
            path.addRect(keyrectmap[action]);
            path.addRect(exprectmap[action]);
        }
    }
    return path;
}

void AccessibleWebElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);
    m_Gadgets->GetStyle()->Render(this, painter);
}

void AccessibleWebElement::SetIndex(int index){
    m_Index = index;
}

void AccessibleWebElement::SetBoundingPos(QPoint pos){
    m_Pos = pos;
}

void AccessibleWebElement::SetElement(SharedWebElement elem){
    m_Element = elem;
    m_Gadgets->GetStyle()->OnSetElement(this, elem);
}

int AccessibleWebElement::GetIndex() const{
    return m_Index;
}

QPoint AccessibleWebElement::GetBoundingPos() const{
    return m_Pos;
}

SharedWebElement AccessibleWebElement::GetElement() const{
    return m_Element;
}

QPoint AccessibleWebElement::KeyExplanationBasePos() const{
    return QPoint(m_Pos - QPoint(40, ACCESSKEY_INFO_HEIGHT/2));
}

QMap<QString, QRect> AccessibleWebElement::KeyRects() const{
    QMap<QString, QRect> map;

    int i = 0;
    foreach(QString action, m_Gadgets->GetAccessKeyKeyMap().values().toSet()){

        QStringList list;

        foreach(QKeySequence seq, m_Gadgets->GetAccessKeyKeyMap().keys(action)){
            list << seq.toString();
        }

        QString key = list.join(QStringLiteral(" or "));

        int width = m_SmallMetrics.boundingRect(key).width();

        QRect keyrect = QRect(KeyExplanationBasePos()
                              + QPoint(-20,
                                       i * (ACCESSKEY_CHAR_CHIP_S_SIZE.height()
                                            + ACCESSKEY_INFO_HEIGHT + 6)
                                       + ACCESSKEY_INFO_HEIGHT + 3),
                              // is there any good way?
                              QSize(width + 15 - key.length()*0.4,
                                    ACCESSKEY_CHAR_CHIP_S_SIZE.height()));
        map[action] = keyrect;
        i++;
    }
    return map;
}

QMap<QString, QRect> AccessibleWebElement::ExpRects() const{
    QMap<QString, QRect> map;

    int i = 0;
    foreach(QString action, m_Gadgets->GetAccessKeyKeyMap().values().toSet()){

        QString exp = action;

        int width = m_InfoMetrics.boundingRect(exp).width();

        QRect exprect = QRect(KeyExplanationBasePos()
                              + QPoint(0,
                                       i * (ACCESSKEY_CHAR_CHIP_S_SIZE.height()
                                            + ACCESSKEY_INFO_HEIGHT + 6)
                                       + ACCESSKEY_INFO_HEIGHT
                                       + ACCESSKEY_CHAR_CHIP_S_SIZE.height() + 6),
                              // is there any good way?
                              QSize(width + 15 - exp.length()*0.4,
                                    ACCESSKEY_INFO_HEIGHT));
        map[action] = exprect;
        i++;
    }
    return map;
}

bool AccessibleWebElement::IsCurrentBlock() const{
    return m_Gadgets->IsCurrentBlock(this);
}

bool AccessibleWebElement::IsSelected() const{
    return isSelected();
}

QRect AccessibleWebElement::CharChipRect() const{
    QSize size;
    int len = m_Gadgets->IndexToString(m_Index).length();

    if(m_Element->IsFrameElement()){
        if(len>1) size = ACCESSKEY_CHAR_CHIP_LS_SIZE;
        else      size = ACCESSKEY_CHAR_CHIP_L_SIZE;
    } else if(m_Element->GetAction() != WebElement::None){
        if(len>1) size = ACCESSKEY_CHAR_CHIP_MS_SIZE;
        else      size = ACCESSKEY_CHAR_CHIP_M_SIZE;
    } else {
        if(len>1) size = ACCESSKEY_CHAR_CHIP_SS_SIZE;
        else      size = ACCESSKEY_CHAR_CHIP_S_SIZE;
    }
    size.setWidth(size.width()*len);
    return QRect(m_Pos - QPoint(size.width()/2, size.height()/2), size);
}

QFont AccessibleWebElement::CharChipFont() const{
    int len = m_Gadgets->IndexToString(m_Index).length();

    if(m_Element->IsFrameElement()){
        if(len>1) return ACCESSKEY_CHAR_CHIP_LS_FONT;
        else      return ACCESSKEY_CHAR_CHIP_L_FONT;
    } else if(m_Element->GetAction() != WebElement::None){
        if(len>1) return ACCESSKEY_CHAR_CHIP_MS_FONT;
        else      return ACCESSKEY_CHAR_CHIP_M_FONT;
    } else {
        if(len>1) return ACCESSKEY_CHAR_CHIP_SS_FONT;
        else      return ACCESSKEY_CHAR_CHIP_S_FONT;
    }
}

void AccessibleWebElement::UpdateMinimal(){
    if(m_Pos.isNull() || !m_Element || m_Element->IsNull()) return;

    QUrl url = m_Element->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    if(!IsSelected()){
        m_Gadgets->Update(CharChipRect());
        return;
    } else {
        int width = m_InfoMetrics.boundingRect(str).width();
        // is there any good way?
        width = width + 15 - str.length()*0.4;

        if(width > ACCESSKEY_INFO_MAX_WIDTH)
            width = ACCESSKEY_INFO_MAX_WIDTH;

        QPoint base(m_Pos - QPoint(width/2, ACCESSKEY_INFO_HEIGHT/2));
        m_Gadgets->Update(QRect(base, QSize(width, ACCESSKEY_INFO_HEIGHT)));

        QMap<QString, QRect> keyrectmap = KeyRects();
        QMap<QString, QRect> exprectmap = ExpRects();

        foreach(QString action, m_Gadgets->GetAccessKeyKeyMap().values().toSet()){
            m_Gadgets->Update(keyrectmap[action]);
            m_Gadgets->Update(exprectmap[action]);
        }
    }
}

QVariant AccessibleWebElement::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemSelectedChange && scene()){

        if(value.toBool()){
            setZValue(DRAGGING_CONTENTS_LAYER);
        } else {
            setZValue(MAIN_CONTENTS_LAYER);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}


void AccessibleWebElement::mousePressEvent   (QGraphicsSceneMouseEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::mouseReleaseEvent (QGraphicsSceneMouseEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::mouseMoveEvent    (QGraphicsSceneMouseEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::hoverEnterEvent   (QGraphicsSceneHoverEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::hoverMoveEvent    (QGraphicsSceneHoverEvent *ev){ ev->setAccepted(false);}
