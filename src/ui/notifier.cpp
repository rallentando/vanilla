#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "notifier.hpp"

#include <QWidget>
#include <QPaintEvent>
#include <QTimerEvent>
#include <QTimer>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QMenu>

#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "networkcontroller.hpp"

Notifier::Notifier(TreeBank *parent, bool purge)
    : QWidget(purge ? nullptr : parent)
    , m_Position(SouthWest)
    , m_TreeBank(parent)
    , m_HotSpot(QPoint())
    , m_DownloadItemTable(QMap<DownloadItem*, int>())
    , m_UploadItemTable(QMap<UploadItem*, int>())
    , m_HoveredDownloadItem(nullptr)
    , m_HoveredUploadItem(nullptr)
    , m_CancelButtonState(NotHovered)
{
    if(purge){
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
    }
    setMouseTracking(true);

    resize(0, 0);
    show();
}

Notifier::~Notifier(){}

bool Notifier::IsPurged() const {
    return isWindow();
}

void Notifier::MakeOwnedWindow(){
    const bool v = isVisible();

    hide();
    setAttribute(Qt::WA_TranslucentBackground);
    setParent(m_TreeBank ? m_TreeBank->GetMainWindow() : nullptr,
              Qt::FramelessWindowHint | Qt::Tool);

    if(v) show();
}

void Notifier::TakeWindowOwnerIfNeed(){
    if(!isWindow() || !m_TreeBank) return;
    QWidget *win = m_TreeBank->GetMainWindow();
    if(!win || !win->isVisible() || parentWidget() == win) return;
    MakeOwnedWindow();
}

void Notifier::Purge(){
    MakeOwnedWindow();
}

void Notifier::Join(){
    bool v = isVisible();
    setParent(m_TreeBank);
    if(v) show();
    else hide();
}

void Notifier::ResizeNotify(QSize size){
    TakeWindowOwnerIfNeed();
    int dlen = m_DownloadItemTable.size();
    int ulen = m_UploadItemTable.size();
    int len = dlen + ulen;
    int w = qMax(size.width() * NOTIFIER_WIDTH_PERCENTAGE / 100,
                 ScaleByDevice(NOTIFIER_MINIMUM_WIDTH));
    int h = ScaleByDevice(NOTIFIER_HEIGHT) + len * ScaleByDevice(TRANSFER_ITEM_HEIGHT);
    QPoint pos;
    switch(m_Position){
    case NorthWest:
        pos = QPoint(0, 0);
        break;
    case NorthEast:
        pos = QPoint(size.width() - w, 0);
        break;
    case SouthWest:
        pos = QPoint(0, size.height() - h);
        break;
    case SouthEast:
        pos = QPoint(size.width() - w, size.height() - h);
        break;
    }
    if(IsPurged()) pos = m_TreeBank->mapToGlobal(pos);
    setGeometry(QRect(pos, QSize(w, h)));
}

void Notifier::RepaintIfNeed(const QRect &rect){
    ResizeNotify(m_TreeBank->size());
    if(isVisible() && rect.intersects(geometry())){
        repaint(rect.intersected(geometry()).translated(-pos()));
    }
}

void Notifier::RegisterDownload(DownloadItem *item){
    if(!item) return;
    connect(item, &DownloadItem::Progress,
            this, &Notifier::SetSaveProgress);
}

void Notifier::RegisterUpload(UploadItem *item){
    if(!item) return;
    connect(item, &UploadItem::Progress,
            this, &Notifier::SetOpenProgress);
}

void Notifier::AddDownloadItem(DownloadItem *item){
    m_DownloadItemTable[item] = 0;
    ResizeNotify(m_TreeBank->size());
    repaint();
}

void Notifier::RemoveDownloadItem(DownloadItem *item){
    if(m_HoveredDownloadItem == item){
        m_HoveredDownloadItem = nullptr;
        m_CancelButtonState = NotHovered;
    }
    m_DownloadItemTable.remove(item);
    ResizeNotify(m_TreeBank->size());
    repaint();
}

void Notifier::AddUploadItem(UploadItem *item){
    m_UploadItemTable[item] = 0;
    ResizeNotify(m_TreeBank->size());
    repaint();
}

void Notifier::RemoveUploadItem(UploadItem *item){
    if(m_HoveredUploadItem == item){
        m_HoveredUploadItem = nullptr;
        m_CancelButtonState = NotHovered;
    }
    m_UploadItemTable.remove(item);
    ResizeNotify(m_TreeBank->size());
    repaint();
}

void Notifier::SetStatus(const QString str){
    if(!str.isEmpty()){
        m_UseLinkText = false;
    }
    m_UpperText = str;
    repaint();
}

void Notifier::SetStatus(const QString str1, const QString str2){
    if(!str1.isEmpty() || !str2.isEmpty()){
        m_UseLinkText = false;
    }
    m_UpperText = str1;
    m_LowerText = str2;
    repaint();
}

void Notifier::ResetStatus(){
    m_UseLinkText = false;
    m_UpperText = QString();
    repaint();
}

void Notifier::SetLink(const QString url, const QString title, const QString text){
    if(!url.isEmpty()){
        m_LowerText = url;
        if(
           m_UpperText.isEmpty() ||
           m_UpperText == tr("Finished loading.") ||
           m_UpperText == tr("Failed to load.") ||
           m_UpperText == tr("Auto save failed.") ||
           m_UpperText.startsWith(tr("Auto save finished")) ||
           m_UpperText.startsWith(tr("Zoom factor changed to")) ||
           m_UpperText.startsWith(tr("Displaying")) ||
           m_UseLinkText){

            if(!title.isEmpty()){
                m_UseLinkText = true;
                m_UpperText = title;
            } else if(!text.isEmpty()){
                m_UseLinkText = true;
                m_UpperText = text;
            } else {
                m_UpperText = QString();
            }
        }
    }
    repaint();
}

void Notifier::ResetLink(){
    m_UseLinkText = true;
    m_LowerText = QString();
    repaint();
}

void Notifier::AutoSaveStarted(){
    m_UseLinkText = false;
    m_UpperText = tr("Auto save started.");
    repaint();
}

void Notifier::AutoSaveFailed(){
    m_UseLinkText = false;
    m_UpperText = tr("Auto save failed.");
    repaint();
}

void Notifier::AutoSaveFinished(const QString &info){
    m_UseLinkText = false;
    m_UpperText = tr("Auto save finished(%1).").arg(info);
    repaint();
}

void Notifier::SetScroll(QPointF pos){
    m_ScrollPos = pos;
    repaint();
}

void Notifier::SetSaveProgress(QString file, qint64 received, qint64 total){
    Q_UNUSED(file)
    DownloadItem *item = qobject_cast<DownloadItem*>(sender());
    if(!item) return;

    if(total == -1){
        m_DownloadItemTable[item] = 0;
    } else {
        bool resizeflag = !m_DownloadItemTable.contains(item);

        m_DownloadItemTable[item] = static_cast<int>(100 * (static_cast<float>(received)/static_cast<float>(total)));

        if(received == 100 && total == 100){
            RemoveDownloadItem(item);
            resizeflag = true;
        }

        if(resizeflag){
            ResizeNotify(m_TreeBank->size());
        }
    }
    repaint();

}

void Notifier::SetOpenProgress(QString file, qint64 sent, qint64 total){
    Q_UNUSED(file)
    UploadItem *item = qobject_cast<UploadItem*>(sender());
    if(!item) return;

    if(total == -1){
        m_UploadItemTable[item] = 0;
    } else {
        bool resizeflag = !m_UploadItemTable.contains(item);

        m_UploadItemTable[item] = static_cast<int>(100 * sent / total);

        if(sent == 100 && total == 100){
            RemoveUploadItem(item);
            resizeflag = true;
        }

        if(resizeflag){
            ResizeNotify(m_TreeBank->size());
        }
    }
    repaint();

}

bool Notifier::EmitScrollRequest(QPoint pos){
    if(!m_HotSpot.isNull()) return false;

    const int itemHeight   = ScaleByDevice(TRANSFER_ITEM_HEIGHT);
    const int scrollWidth  = ScaleByDevice(SCROLL_AREA_WIDTH);
    const int scrollHeight = ScaleByDevice(SCROLL_AREA_HEIGHT);
    int x = pos.x();
    int y = pos.y()
        - (m_DownloadItemTable.size() * itemHeight)
        - (m_UploadItemTable.size() * itemHeight);
    if(x <= scrollWidth  && x >= 0 &&
       y <= scrollHeight && y >= 0){
        if(x == 0) x = 1;
        if(x == scrollWidth)  x = scrollWidth  - 1;
        if(y == 0) y = 1;
        if(y == scrollHeight) y = scrollHeight - 1;
        emit ScrollRequest(QPointF(static_cast<qreal>(x-1) / (scrollWidth  - 2),
                                   static_cast<qreal>(y-1) / (scrollHeight - 2)));
        return true;
    }
    return false;
}

void Notifier::timerEvent(QTimerEvent *ev){
    Q_UNUSED(ev)
}

void Notifier::paintEvent(QPaintEvent *ev){
    const int dlen = m_DownloadItemTable.size();
    const int ulen = m_UploadItemTable.size();
    const int len = dlen + ulen;
    const QList<DownloadItem*> dkeys = m_DownloadItemTable.keys();
    const QList<UploadItem*> ukeys = m_UploadItemTable.keys();
    const int itemHeight   = ScaleByDevice(TRANSFER_ITEM_HEIGHT);
    const int scrollWidth  = ScaleByDevice(SCROLL_AREA_WIDTH);
    const int scrollHeight = ScaleByDevice(SCROLL_AREA_HEIGHT);
    const int notifierHeight = ScaleByDevice(NOTIFIER_HEIGHT);
    const int offset = (dlen + ulen) * itemHeight;
    const int pad = ScaleByDevice(STATUS_TEXT_PADDING);

    QPainter painter(this);
    painter.setFont(NotifierFont());

    const auto ElidedName = [&painter](const QString &text, int width){
        return painter.fontMetrics().elidedText(text, Qt::ElideMiddle, width);
    };
    const auto ElidedText = [&painter](const QString &text, int width){
        return painter.fontMetrics().elidedText(text, Qt::ElideRight, width);
    };

    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::Brush(Theme::NotifierBackground));
        painter.drawRect(-1, -1, width()+1, height()+1);
    }

    {
        const int progress_offset = (width() * (100 - TRANSFER_PROGRESS_PERCENTAGE) / 100);
        const int progress_width = (width() * TRANSFER_PROGRESS_PERCENTAGE / 100);
        const int filename_width = progress_offset;

        const auto BarRect = [&](int i){
            return QRect(progress_offset, i * itemHeight, progress_width, itemHeight)
                .adjusted(ScaleByDevice(2), ScaleByDevice(3),
                          -ScaleByDevice(3), -ScaleByDevice(3));
        };

        {
            painter.setPen(Theme::Pen(Theme::NotifierBarBorder));
            painter.setBrush(Qt::NoBrush);
            for(int i = 0; i < len; i++){
                painter.drawRect(BarRect(i));
            }
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::Brush(Theme::NotifierDownloadBar));
        for(int i = 0; i < dlen; i++){
            const QRect inner = BarRect(i).adjusted(1, 1, -1, -1);
            painter.drawRect(QRect(inner.topLeft(),
                                   QSize(inner.width() * m_DownloadItemTable[dkeys[i]] / 100,
                                         inner.height())));
        }
        painter.setBrush(Theme::Brush(Theme::NotifierUploadBar));
        for(int i = dlen; i < len; i++){
            const QRect inner = BarRect(i).adjusted(1, 1, -1, -1);
            painter.drawRect(QRect(inner.topLeft(),
                                   QSize(inner.width() * m_UploadItemTable[ukeys[i-dlen]] / 100,
                                         inner.height())));
        }

        {
            painter.setPen(Theme::Pen(Theme::NotifierText));
            painter.setBrush(Qt::NoBrush);
            for(int i = 0; i < len; i++){
                const QRect f(pad, i * itemHeight, filename_width - pad * 2, itemHeight);
                const QString name = i < dlen
                    ? dkeys[i]->GetPath().split(QStringLiteral("/")).last()
                    : ukeys[i-dlen]->GetPath().split(QStringLiteral("/")).last();
                const int percentage = i < dlen
                    ? m_DownloadItemTable[dkeys[i]]
                    : m_UploadItemTable[ukeys[i-dlen]];

                painter.drawText(f, Qt::AlignLeft | Qt::AlignVCenter,
                                 ElidedName(name, f.width()));
                painter.drawText(BarRect(i), Qt::AlignCenter,
                                 QStringLiteral("%1%").arg(percentage));
            }

            if(m_CancelButtonState != NotHovered){
                static QPixmap close = Theme::Icon(QStringLiteral(":/resources/notifier/close.png"))
                    .pixmap(QSize(ScaleByDevice(10), ScaleByDevice(10)));
                int index = 0;
                if(m_HoveredDownloadItem) index = dkeys.indexOf(m_HoveredDownloadItem.data());
                if(m_HoveredUploadItem) index = dlen + ukeys.indexOf(m_HoveredUploadItem.data());
                painter.setBrush(Theme::Brush(
                    m_CancelButtonState == ItemHovered   ? Theme::NotifierCancelItemHovered :
                    m_CancelButtonState == ButtonHovered ? Theme::NotifierCancelHovered :
                                                           Theme::NotifierCancelPressed));
                painter.setPen(Qt::NoPen);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.drawEllipse(width() - itemHeight + ScaleByDevice(5),
                                    index * itemHeight + ScaleByDevice(6),
                                    ScaleByDevice(14), ScaleByDevice(14));
                painter.setRenderHint(QPainter::Antialiasing, false);
                painter.drawPixmap(QRect(QPoint(width() - itemHeight + ScaleByDevice(7),
                                                index * itemHeight + ScaleByDevice(8)),
                                          close.size()),
                                    close, QRect(QPoint(), close.size()));
            }
        }
    }

    {
        {
            painter.setPen(Theme::Pen(Theme::NotifierScrollBorder));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(0, offset, scrollWidth, scrollHeight - 1);
        }

        {
            const int size = ScaleByDevice(4);
            painter.setPen(Theme::Pen(Theme::NotifierScrollMarker));
            painter.setBrush(Theme::Brush(Theme::NotifierScrollMarker));
            painter.drawEllipse(static_cast<int>(m_ScrollPos.x() * scrollWidth) - size/2,
                                static_cast<int>(m_ScrollPos.y() * scrollHeight + offset) - size/2,
                                size, size);
        }

        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::Brush(Theme::NotifierScrollLine));
            painter.drawRect(0, static_cast<int>(m_ScrollPos.y() * scrollHeight + offset),
                             scrollWidth, 1);
            painter.drawRect(static_cast<int>(m_ScrollPos.x() * scrollWidth), offset,
                             1, scrollHeight);
        }

        {
            const int x = scrollWidth + pad;
            const int w = width() - x - pad;
            const QRect upper(x, offset, w, notifierHeight/2);
            const QRect lower(x, offset + notifierHeight/2, w, notifierHeight/2);
            const QString space = QStringLiteral(" ");

            painter.setPen(Theme::Pen(Theme::NotifierTextStrong));
            painter.setBrush(Theme::Brush(Theme::NotifierScrollLine));
            painter.drawText(upper, Qt::AlignLeft | Qt::AlignVCenter,
                             ElidedText(QString(m_UpperText).replace(QStringLiteral("\n"), space), w));
            painter.drawText(lower, Qt::AlignLeft | Qt::AlignVCenter,
                             ElidedText(QString(m_LowerText).replace(QStringLiteral("\n"), space), w));
        }
    }
    painter.end();

    ev->setAccepted(true);
}

void Notifier::enterEvent(QEnterEvent *ev)
{
    Q_UNUSED(ev)
}

void Notifier::leaveEvent(QEvent *ev){
    Q_UNUSED(ev)
    m_HoveredDownloadItem = nullptr;
    m_HoveredUploadItem = nullptr;
    m_CancelButtonState = NotHovered;
    repaint();
}

void Notifier::mousePressEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){
        if(m_CancelButtonState == ButtonHovered){
            m_CancelButtonState = ButtonPressed;
            repaint();
        } else if(!EmitScrollRequest(ev->pos())){
            m_HotSpot = ev->pos();
            ev->setAccepted(true);
        }
    } else if(ev->button() == Qt::RightButton){
        QMenu *menu = m_TreeBank->GlobalContextMenu();
        menu->exec(ev->globalPosition().toPoint());
        delete menu;
        ev->setAccepted(true);
    }
}

void Notifier::mouseMoveEvent(QMouseEvent *ev){
    if(ev->buttons() & Qt::LeftButton){
        if(!EmitScrollRequest(ev->pos()) && !m_HotSpot.isNull()){
            QRect rect = QRect((IsPurged()
                                ? mapToGlobal(ev->pos())
                                : mapToParent(ev->pos()))
                               - m_HotSpot,
                               size());
            setGeometry(rect);
            bool north = true;
            bool west = true;
            QPoint center1 = rect.center();
            QPoint center2 = QPoint(m_TreeBank->width()/2,
                                    m_TreeBank->height()/2);
            if(IsPurged()) center2 = m_TreeBank->mapToGlobal(center2);
            if(center1.x() > center2.x()) west = false;
            if(center1.y() > center2.y()) north = false;
            if(north){
                if(west){
                    m_Position = NorthWest;
                } else {
                    m_Position = NorthEast;
                }
            } else {
                if(west){
                    m_Position = SouthWest;
                } else {
                    m_Position = SouthEast;
                }
            }
            ev->setAccepted(true);
            return;
        }
    }

    const int dlen = m_DownloadItemTable.size();
    const int ulen = m_UploadItemTable.size();
    const int len = dlen + ulen;
    const QList<DownloadItem*> dkeys = m_DownloadItemTable.keys();
    const QList<UploadItem*> ukeys = m_UploadItemTable.keys();
    const int itemHeight = ScaleByDevice(TRANSFER_ITEM_HEIGHT);
    const int index = ev->pos().y() < 0 ? len : ev->pos().y() / itemHeight;
    bool shouldRepaint = false;
    if(index < dlen){
        if(m_HoveredDownloadItem != dkeys[index]){
            m_HoveredDownloadItem = dkeys[index];
            m_HoveredUploadItem = nullptr;
            shouldRepaint = true;
        }
    } else if(index < len){
        if(m_HoveredUploadItem != ukeys[index-dlen]){
            m_HoveredDownloadItem = nullptr;
            m_HoveredUploadItem = ukeys[index-dlen];
            shouldRepaint = true;
        }
    } else {
        if(m_HoveredDownloadItem != nullptr || m_HoveredUploadItem != nullptr){
            m_HoveredDownloadItem = nullptr;
            m_HoveredUploadItem = nullptr;
            shouldRepaint = true;
        }
    }
    if(index < len){
        if(ev->pos().x() > width() - itemHeight){
            if(!shouldRepaint && m_CancelButtonState == ButtonPressed){
            } else if(m_CancelButtonState != ButtonHovered){
                m_CancelButtonState = ButtonHovered;
                shouldRepaint = true;
            }
        } else {
            if(m_CancelButtonState != ItemHovered){
                m_CancelButtonState = ItemHovered;
                shouldRepaint = true;
            }
        }
    } else {
        if(m_CancelButtonState != NotHovered){
            m_CancelButtonState = NotHovered;
            shouldRepaint = true;
        }
    }
    if(shouldRepaint) repaint();
}

void Notifier::mouseReleaseEvent(QMouseEvent *ev){
    m_HotSpot = QPoint();
    ev->setAccepted(true);
    if(IsPurged()) m_TreeBank->GetMainWindow()->SetFocus();
    if(m_CancelButtonState == ButtonPressed){
        m_CancelButtonState = ButtonHovered;
        if(m_HoveredDownloadItem) m_HoveredDownloadItem->Stop();
        if(m_HoveredUploadItem) m_HoveredUploadItem->Stop();
        repaint();
    }
}
