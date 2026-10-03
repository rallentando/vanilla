#ifndef MINIMAP_HPP
#define MINIMAP_HPP

#include "switch.hpp"

#include "devicescale.hpp"

#include <QWidget>
#include <QPointer>
#include <QTimer>
#include <QHash>
#include <QUrl>

class TreeBank;
class View;

class MiniMap : public QWidget {
    Q_OBJECT

    friend class tst_minimap;

public:
    explicit MiniMap(TreeBank *parent);
    ~MiniMap() Q_DECL_OVERRIDE;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    void SetView(View *view);
    void SetShelved(bool shelved);
    bool IsActive() const;
    int MapWidth() const;
    void ResizeNotify(QSize size);

    enum BlockKind {
        TextBlock, MediaBlock, ControlBlock, FrameBlock,
        PositionedBackgroundBlock
    };
    struct Block {
        QRectF rect;
        int kind;
        bool followsViewport;
    };

    static qreal SlideOffset(qreal drawingHeight, qreal stripHeight,
                             qreal scrollRatio);
    static qreal IndicatorTravel(qreal drawingHeight, qreal stripHeight,
                                 qreal indicatorHeight);
    static qreal JumpRatio(qreal y, qreal scale, qreal offset,
                           qreal viewportHeight, qreal contentsHeight);
    static qreal DragRatio(qreal y, qreal grabOffset, qreal travel);
    static qreal IndicatorHeight(qreal viewportHeight, qreal scale,
                                 qreal minimum);
    static QList<Block> ParseBlocks(const QVariant &answer, QSizeF *pageSize,
                                    bool *overflowed);

private slots:
    void OnViewDestroyed(QObject *base);

private:
    static QString CollectBlocksJsCode();
    static QRectF PaintedBlockRect(const Block &block, qreal conversion,
                                   qreal scale, qreal offset,
                                   qreal indicatorTop);
    struct Snapshot {
        QList<Block> blocks;
        QSizeF pageSize;
        bool overflowed;
        QUrl url;
    };

    static QObject *SupportedBase(View *view);
    void WireCurrentView();

    void WatchCached(QObject *base);
    void ForgetCached(QObject *base);
    void GoDormant();
    void ClearStrip();
    void EndDrag();
    void DropPage();
    void Grown();
    void Scrolled();
    void GeometryReported();
    void RequestBlocks();
    void ApplyRatio(qreal ratio, const QRectF &viewport,
                    const QSizeF &contents);
    bool Geometry(qreal *scale, QSizeF *contents, QRectF *viewport) const;

    TreeBank *m_TreeBank;
    View *m_View;
    QPointer<QObject> m_ViewGuard;
    QObject *m_WiredBase;
    QList<QMetaObject::Connection> m_Connections;
    QHash<QObject*, QList<QMetaObject::Connection>> m_CacheWatch;

    QTimer m_RecollectTimer;

    quint64 m_Generation;
    quint64 m_Request;

    QList<Block> m_Blocks;
    QSizeF m_PageSize;
    bool m_Shelved;
    bool m_Overflowed;
    bool m_Dragging;
    qreal m_GrabOffset;
    QSizeF m_LastGeometryContents;
    QHash<QObject*, Snapshot> m_Cache;

protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
};

#endif
