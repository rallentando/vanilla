#include "switch.hpp"
#include "const.hpp"

#include "notifier.hpp"

#include <QWidget>
#include <QPaintEvent>
#include <QTimerEvent>
#include <QTimer>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QMenu>

#if defined(Q_OS_WIN)
#  include <QtWinExtras/QWinTaskbarButton>
#  include <QtWinExtras/QWinTaskbarProgress>
#endif

#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "networkcontroller.hpp"

Notifier::Notifier(TreeBank *parent, bool purge)
    : QWidget(purge ? 0 : parent)
    , m_TreeBank(parent)
    , m_HotSpot(QPoint())
    , m_Position(SouthWest)
#if defined(Q_OS_WIN)
    , m_Button(0)
    , m_Progress(0)
#endif
{
    if(purge){
        setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    // display test.
    //m_UploadItemTable["ulfile"] = 50;
    //m_DownloadItemTable["dlfile"] = 50;

    ResizeNotify(parent->size());
    show();
    raise();
}

Notifier::~Notifier(){}

bool Notifier::IsPurged(){
    return !parent();
}

void Notifier::Purge(){
    bool v = isVisible();
    setParent(0);
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
    if(v) show();
    else hide();
}

void Notifier::Join(){
    bool v = isVisible();
    setParent(m_TreeBank);
    if(v) show();
    else hide();
}

void Notifier::ResizeNotify(QSize size){
    int dlen = m_DownloadItemTable.size();
    int ulen = m_UploadItemTable.size();
    int len = dlen + ulen;
    int w = qMax(size.width() * NOTIFIER_WIDTH_PERCENTAGE / 100,
                 NOTIFIER_MINIMUM_WIDTH);
    int h = NOTIFIER_HEIGHT + len * TRANSFER_ITEM_HEIGHT;
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
        if(// overwritable?
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
    DownloadItem *item = qobject_cast<DownloadItem*>(sender());
    if(!item) return;

    if(total == -1){
        m_DownloadItemTable[item] = 0;
    } else {
        bool resizeflag = !m_DownloadItemTable.contains(item);

        m_DownloadItemTable[item] = 100 * ((float)received/(float)total);

        if(received == 100 && total == 100){
            RemoveDownloadItem(item);
            resizeflag = true;
        }

        if(resizeflag){
            ResizeNotify(m_TreeBank->size());
        }
    }
    repaint();

#if defined(Q_OS_WIN)
    UpdateTaskbarProgress();
#endif
}

void Notifier::SetOpenProgress(QString file, qint64 sent, qint64 total){
    UploadItem *item = qobject_cast<UploadItem*>(sender());
    if(!item) return;

    if(total == -1){
        m_UploadItemTable[item] = 0;
    } else {
        bool resizeflag = !m_UploadItemTable.contains(item);

        m_UploadItemTable[item] = 100 * ((float)sent/(float)total);

        if(sent == 100 && total == 100){
            RemoveUploadItem(item);
            resizeflag = true;
        }

        if(resizeflag){
            ResizeNotify(m_TreeBank->size());
        }
    }
    repaint();

#if defined(Q_OS_WIN)
    UpdateTaskbarProgress();
#endif
}

#if defined(Q_OS_WIN)
void Notifier::UpdateTaskbarProgress(){
    int num = 0;
    int sum = 0;

    foreach(int value,
            m_UploadItemTable.values() +
            m_DownloadItemTable.values()){
        num += 1;
        sum += value;
    }

    int progress = num ? sum/num : 100;

    MainWindow *win = m_TreeBank->GetMainWindow();

    if(!m_Button){
        if(progress == 0) return;
        m_Button = new QWinTaskbarButton(win);
        m_Button->setWindow(win->windowHandle());
        m_Progress = m_Button->progress();
    }
    if(progress == 0){
        m_Progress->setVisible(false);
        return;
    }

    if(!m_Progress->isVisible())
        m_Progress->setVisible(true);

    m_Progress->setValue(progress);

    if(progress == 100)
        m_Progress->setVisible(false);
}
#endif

bool Notifier::EmitScrollRequest(QPoint pos){
    if(!m_HotSpot.isNull()) return false;

    int x = pos.x();
    int y = pos.y()
        - (m_DownloadItemTable.size() * TRANSFER_ITEM_HEIGHT)
        - (m_UploadItemTable.size() * TRANSFER_ITEM_HEIGHT);
    if(x <= SCROLL_AREA_WIDTH  && x >= 0 &&
       y <= SCROLL_AREA_HEIGHT && y >= 0){
        if(x == 0) x = 1;
        if(x == SCROLL_AREA_WIDTH)  x = SCROLL_AREA_WIDTH  - 1;
        if(y == 0) y = 1;
        if(y == SCROLL_AREA_HEIGHT) y = SCROLL_AREA_HEIGHT - 1;
        emit ScrollRequest(QPointF((float)(x-1) / (SCROLL_AREA_WIDTH  - 2),
                                   (float)(y-1) / (SCROLL_AREA_HEIGHT - 2)));
        return true;
    }
    return false;
}

void Notifier::timerEvent(QTimerEvent *ev){
    Q_UNUSED(ev);
    /* do nothing. */
}

void Notifier::paintEvent(QPaintEvent *ev){
    const int dlen = m_DownloadItemTable.size();
    const int ulen = m_UploadItemTable.size();
    const int len = dlen + ulen;
    const QList<DownloadItem*> dkeys = m_DownloadItemTable.keys();
    const QList<UploadItem*> ukeys = m_UploadItemTable.keys();
    const int offset = (dlen + ulen) * TRANSFER_ITEM_HEIGHT;

    static const QColor white255 = QColor(255, 255, 255, 255);
    static const QColor white200 = QColor(255, 255, 255, 200);
    static const QColor white128 = QColor(255, 255, 255, 128);
    static const QColor white100 = QColor(255, 255, 255, 100);

    QPainter painter(this);
    painter.setFont(NOTIFIER_FONT);

    {   // background.
        static const QBrush b = QBrush(QColor(0, 0, 0, 128));
        painter.setPen(Qt::NoPen);
        painter.setBrush(b);
        painter.drawRect(-1, -1, width()+1, height()+1);
    }

    {   // transfer item.
        const int progress_offset = (width() * (100 - TRANSFER_PROGRESS_PERCENTAGE) / 100);
        const int progress_width = (width() * TRANSFER_PROGRESS_PERCENTAGE / 100);
        const int filename_width = progress_offset;
        {
            static const QPen p = QPen(QColor(255, 255, 255, 128));
            painter.setPen(p);
            painter.setBrush(Qt::NoBrush);
            for(int i = 0; i < dlen; i++){
                painter.drawRect(progress_offset + 2, i * TRANSFER_ITEM_HEIGHT + 3,
                                 progress_width - 5, TRANSFER_ITEM_HEIGHT - 6);
            }
            for(int i = dlen; i < len; i++){
                painter.drawRect(progress_offset + 2, i * TRANSFER_ITEM_HEIGHT + 3,
                                 progress_width - 5, TRANSFER_ITEM_HEIGHT - 6);
            }
        }

        painter.setPen(Qt::NoPen);
        //#4169e1 65 150 225 misty blue
        //#6478ff 100 120 255 dark blue
        static const QBrush dlbrush = QBrush(QColor(100, 120, 255, 200));
        painter.setBrush(dlbrush);
        for(int i = 0; i < dlen; i++){
            painter.drawRect(progress_offset + 3, i * TRANSFER_ITEM_HEIGHT + 4,
                             (progress_width - 6) * m_DownloadItemTable[dkeys[i]] / 100,
                             TRANSFER_ITEM_HEIGHT - 7);
        }
        //#9932cc 153 50 204 purple
        //#c87864 200 120 100 dark pink
        static const QBrush ulbrush = QBrush(QColor(200, 120, 100, 200));
        painter.setBrush(ulbrush);
        for(int i = dlen; i < len; i++){
            painter.drawRect(progress_offset + 3, i * TRANSFER_ITEM_HEIGHT + 4,
                             (progress_width - 6) * m_UploadItemTable[ukeys[i-dlen]] / 100,
                             TRANSFER_ITEM_HEIGHT - 7);
        }

        {
            static const QPen p = QPen(QColor(255, 255, 255, 200));
            painter.setPen(p);
            painter.setBrush(Qt::NoBrush);
            for(int i = 0; i < dlen; i++){
                QRect f = QRect(5, i * TRANSFER_ITEM_HEIGHT + 4,
                                filename_width - 6, TRANSFER_ITEM_HEIGHT);
                QRect p = QRect(progress_offset + 3, i * TRANSFER_ITEM_HEIGHT + 4,
                                progress_width - 6, TRANSFER_ITEM_HEIGHT - 7);
                painter.drawText(f, Qt::AlignLeft, dkeys[i]->GetPath().split(QStringLiteral("/")).last());
                painter.drawText(p, Qt::AlignCenter, QStringLiteral("%1%").arg(m_DownloadItemTable[dkeys[i]]));
            }
            for(int i = dlen; i < len; i++){
                QRect f = QRect(5, i * TRANSFER_ITEM_HEIGHT + 4,
                                filename_width - 6, TRANSFER_ITEM_HEIGHT);
                QRect p = QRect(progress_offset + 3, i * TRANSFER_ITEM_HEIGHT + 4,
                                progress_width - 6, TRANSFER_ITEM_HEIGHT - 7);
                painter.drawText(f, Qt::AlignLeft, ukeys[i-dlen]->GetPath().split(QStringLiteral("/")).last());
                painter.drawText(p, Qt::AlignCenter, QStringLiteral("%1%").arg(m_UploadItemTable[ukeys[i-dlen]]));
            }
        }
    }

    {   // status area.
        {
            static const QPen p = QPen(QColor(255, 255, 255, 128));
            painter.setPen(p);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(0, offset, SCROLL_AREA_WIDTH, SCROLL_AREA_HEIGHT - 1);
        }

        {
            static const QPen   p = QPen(QColor(255, 255, 255, 200));
            static const QBrush b = QBrush(QColor(255, 255, 255, 200));
            painter.setPen(p);
            painter.setBrush(b);
            painter.drawEllipse(m_ScrollPos.x() * SCROLL_AREA_WIDTH - 2,
                                m_ScrollPos.y() * SCROLL_AREA_HEIGHT - 2 + offset,
                                4, 4);
        }

        {
            static const QBrush b = QBrush(QColor(255, 255, 255, 100));
            painter.setPen(Qt::NoPen);
            painter.setBrush(b);
            painter.drawRect(0, m_ScrollPos.y() * SCROLL_AREA_HEIGHT + offset,
                             SCROLL_AREA_WIDTH, 1);
            painter.drawRect(m_ScrollPos.x() * SCROLL_AREA_WIDTH, offset,
                             1, SCROLL_AREA_HEIGHT);
        }

        {
            static const QPen   p = QPen(QColor(255, 255, 255, 255));
            static const QBrush b = QBrush(QColor(255, 255, 255, 100));
            painter.setPen(p);
            painter.setBrush(b);
            painter.drawText(QRect(SCROLL_AREA_WIDTH + 5, offset + 4,
                                   width()-5, NOTIFIER_HEIGHT/2), Qt::AlignLeft,
                             m_UpperText.replace(QStringLiteral("\n"), QStringLiteral(" ")));
            painter.drawText(QRect(SCROLL_AREA_WIDTH + 5, offset + 4 + (NOTIFIER_HEIGHT/2),
                                   width()-5, NOTIFIER_HEIGHT/2), Qt::AlignLeft,
                             m_LowerText.replace(QStringLiteral("\n"), QStringLiteral(" ")));
        }
    }
    painter.end();

    ev->setAccepted(true);
}

void Notifier::mousePressEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){
        if(!EmitScrollRequest(ev->pos())){
            m_HotSpot = ev->pos();
            ev->setAccepted(true);
        }
    } else if(ev->button() == Qt::RightButton){
        QMenu *menu = m_TreeBank->CreateGlobalContextMenu();
        menu->exec(ev->globalPos());
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
            QPoint center2 = QPoint(m_TreeBank->size().width()/2,
                                    m_TreeBank->size().height()/2);
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
        }
    } else if(ev->buttons() & Qt::RightButton){
        // do nothing.
    }
}

void Notifier::mouseReleaseEvent(QMouseEvent *ev){
    m_HotSpot = QPoint();
    ev->setAccepted(true);
    if(IsPurged()) m_TreeBank->GetMainWindow()->SetFocus();
}
