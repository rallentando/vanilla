#ifndef GADGETSSTYLE_HPP
#define GADGETSSTYLE_HPP

#include "switch.hpp"

#include "view.hpp" // for SharedWebElement.

class QGraphicsRectItem;
class GraphicsTableView;
class Thumbnail;
class NodeTitle;
class SpotLight;
class InPlaceNotifier;
class UpDirectoryButton;
class AccessibleWebElement;

class GadgetsStyle{
public:
    explicit GadgetsStyle(){}
    virtual ~GadgetsStyle(){}

    virtual QString StyleName() = 0;

    virtual void ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) = 0;
    virtual void RenderBackground(GraphicsTableView *gtv, QPainter *painter) = 0;
    virtual void Render(Thumbnail *thumb, QPainter *painter) = 0;
    virtual void Render(NodeTitle *title, QPainter *painter) = 0;
    virtual void Render(SpotLight *light, QPainter *painter) = 0;
    virtual void Render(InPlaceNotifier *notifier, QPainter *painter) = 0;
    virtual void Render(UpDirectoryButton *button, QPainter *painter) = 0;
    virtual void Render(AccessibleWebElement *awe, QPainter *painter) = 0;
    virtual void OnSetNest(Thumbnail *thumb, int nest) = 0;
    virtual void OnSetNest(NodeTitle *title, int nest) = 0;
    virtual void OnSetPrimary(Thumbnail *thumb, bool primary) = 0;
    virtual void OnSetPrimary(NodeTitle *title, bool primary) = 0;
    virtual void OnSetHovered(Thumbnail *thumb, bool hovered) = 0;
    virtual void OnSetHovered(NodeTitle *thumb, bool hovered) = 0;
    virtual void OnSetHovered(UpDirectoryButton *button, bool hovered) = 0;
    virtual void OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) = 0;
    virtual void OnReshow(QGraphicsRectItem *gri) = 0;
    virtual QRectF ThumbnailAreaRect(GraphicsTableView *gtv) = 0;
    virtual QRectF NodeTitleAreaRect(GraphicsTableView *gtv) = 0;
    virtual QRectF ScrollBarAreaRect(GraphicsTableView *gtv) = 0;
    virtual QGraphicsRectItem *CreateSelectRect(GraphicsTableView *gtv, QPointF pos) = 0;

    virtual bool ScrollToChangeDirectory(bool value) = 0;
    virtual bool RightClickToRenameNode(bool value) = 0;
    virtual bool UseGraphicsItemUpdate() = 0;

    virtual bool NodeTitleDrawBorder() = 0;
    virtual int NodeTitleHeight() = 0;
    virtual int InPlaceNotifierWidth() = 0;
    virtual int InPlaceNotifierHeight() = 0;
};

class GlassStyle : public GadgetsStyle{
public:
    explicit GlassStyle(){}
    virtual ~GlassStyle(){}

    QString StyleName() DECL_OVERRIDE { return QStringLiteral("GlassStyle");}

    static QFont m_ThumbnailTitleFont;

    static int m_ThumbnailPaddingX;
    static int m_ThumbnailPaddingY;
    static int m_ThumbnailTitleHeight;
    static int m_ThumbnailWidthPercentage;
    static int m_ThumbnailDefaultColumnCount;
    static int m_ThumbnailAreaWidthPercentage;

    static bool m_ThumbnailDrawBorder;

    static QSize m_DefaultThumbnailWholeSize;
    static QSize m_MinimumThumbnailWholeSize;

    static QFont m_NodeTitleFont;

    static int m_NodeTitleHeight;
    static bool m_NodeTitleDrawBorder;

    static int m_InPlaceNotifierWidth;
    static int m_InPlaceNotifierHeight;
    static bool m_InPlaceNotifierDrawBorder;

    void ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) DECL_OVERRIDE;
    void RenderBackground(GraphicsTableView *gtv, QPainter *painter) DECL_OVERRIDE;
    void Render(Thumbnail *thumb, QPainter *painter) DECL_OVERRIDE;
    void Render(NodeTitle *title, QPainter *painter) DECL_OVERRIDE;
    void Render(SpotLight *light, QPainter *painter) DECL_OVERRIDE;
    void Render(InPlaceNotifier *notifier, QPainter *painter) DECL_OVERRIDE;
    void Render(UpDirectoryButton *button, QPainter *painter) DECL_OVERRIDE;
    void Render(AccessibleWebElement *awe, QPainter *painter) DECL_OVERRIDE;
    void OnSetNest(Thumbnail *thumb, int nest) DECL_OVERRIDE;
    void OnSetNest(NodeTitle *title, int nest) DECL_OVERRIDE;
    void OnSetPrimary(Thumbnail *thumb, bool primary) DECL_OVERRIDE;
    void OnSetPrimary(NodeTitle *title, bool primary) DECL_OVERRIDE;
    void OnSetHovered(Thumbnail *thumb, bool hovered) DECL_OVERRIDE;
    void OnSetHovered(NodeTitle *title, bool hovered) DECL_OVERRIDE;
    void OnSetHovered(UpDirectoryButton *button, bool hovered) DECL_OVERRIDE;
    void OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) DECL_OVERRIDE;
    void OnReshow(QGraphicsRectItem *gri) DECL_OVERRIDE;
    QRectF ThumbnailAreaRect(GraphicsTableView *gtv) DECL_OVERRIDE;
    QRectF NodeTitleAreaRect(GraphicsTableView *gtv) DECL_OVERRIDE;
    QRectF ScrollBarAreaRect(GraphicsTableView *gtv) DECL_OVERRIDE;
    QGraphicsRectItem *CreateSelectRect(GraphicsTableView *gtv, QPointF pos) DECL_OVERRIDE;

    bool ScrollToChangeDirectory(bool value) DECL_OVERRIDE { return value;}
    bool RightClickToRenameNode(bool value) DECL_OVERRIDE { return value;}
    bool UseGraphicsItemUpdate() DECL_OVERRIDE { return false;}

    bool NodeTitleDrawBorder() DECL_OVERRIDE { return m_NodeTitleDrawBorder;}
    int NodeTitleHeight() DECL_OVERRIDE { return m_NodeTitleHeight;}
    int InPlaceNotifierWidth() DECL_OVERRIDE { return m_InPlaceNotifierWidth;}
    int InPlaceNotifierHeight() DECL_OVERRIDE { return m_InPlaceNotifierHeight;}
};

class FlatStyle : public GadgetsStyle{
public:
    explicit FlatStyle(){}
    virtual ~FlatStyle(){}

    QString StyleName() DECL_OVERRIDE { return QStringLiteral("FlatStyle");}

    static QFont m_ThumbnailTitleFont;

    static int m_ThumbnailPaddingX;
    static int m_ThumbnailPaddingY;
    static int m_ThumbnailTitleHeight;
    static int m_ThumbnailWidthPercentage;
    static int m_ThumbnailDefaultColumnCount;
    static int m_ThumbnailAreaWidthPercentage;

    static bool m_ThumbnailDrawBorder;

    static QSize m_DefaultThumbnailWholeSize;
    static QSize m_MinimumThumbnailWholeSize;

    void ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) DECL_OVERRIDE;
    void RenderBackground(GraphicsTableView *gtv, QPainter *painter) DECL_OVERRIDE;
    void Render(Thumbnail *thumb, QPainter *painter) DECL_OVERRIDE;
    void Render(NodeTitle *title, QPainter *painter) DECL_OVERRIDE;
    void Render(SpotLight *light, QPainter *painter) DECL_OVERRIDE;
    void Render(InPlaceNotifier *notifier, QPainter *painter) DECL_OVERRIDE;
    void Render(UpDirectoryButton *button, QPainter *painter) DECL_OVERRIDE;
    void Render(AccessibleWebElement *awe, QPainter *painter) DECL_OVERRIDE;
    void OnSetNest(Thumbnail *thumb, int nest) DECL_OVERRIDE;
    void OnSetNest(NodeTitle *title, int nest) DECL_OVERRIDE;
    void OnSetPrimary(Thumbnail *thumb, bool primary) DECL_OVERRIDE;
    void OnSetPrimary(NodeTitle *title, bool primary) DECL_OVERRIDE;
    void OnSetHovered(Thumbnail *thumb, bool hovered) DECL_OVERRIDE;
    void OnSetHovered(NodeTitle *title, bool hovered) DECL_OVERRIDE;
    void OnSetHovered(UpDirectoryButton *button, bool hovered) DECL_OVERRIDE;
    void OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) DECL_OVERRIDE;
    void OnReshow(QGraphicsRectItem *gri) DECL_OVERRIDE;
    QRectF ThumbnailAreaRect(GraphicsTableView *gtv) DECL_OVERRIDE;
    QRectF NodeTitleAreaRect(GraphicsTableView *gtv) DECL_OVERRIDE;
    QRectF ScrollBarAreaRect(GraphicsTableView *gtv) DECL_OVERRIDE;
    QGraphicsRectItem *CreateSelectRect(GraphicsTableView *gtv, QPointF pos) DECL_OVERRIDE;

    bool ScrollToChangeDirectory(bool) DECL_OVERRIDE { return false;}
    bool RightClickToRenameNode(bool) DECL_OVERRIDE { return false;}
    bool UseGraphicsItemUpdate() DECL_OVERRIDE { return true;}

    bool NodeTitleDrawBorder() DECL_OVERRIDE { return false;}
    int NodeTitleHeight() DECL_OVERRIDE { return 20;} // dummy value.
    int InPlaceNotifierWidth() DECL_OVERRIDE { return 706;} // dummy value.
    int InPlaceNotifierHeight() DECL_OVERRIDE { return 154;} // dummy value.
};

#endif
