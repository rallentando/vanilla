#include "switch.hpp"
#include "const.hpp"

#include "lightnode.hpp"

#include "application.hpp"
#include "treebank.hpp"
#include "view.hpp"

#include <QCoreApplication>
#include <QUuid>
#include <QtConcurrent/QtConcurrent>

std::atomic<quint64> Node::m_SerialCounter(1);
QRecursiveMutex Node::m_DataMutex;
bool Node::m_Booting = false;
QSet<QString> Node::m_AllImageFileName = QSet<QString>();
QSet<QString> Node::m_AllHistoryFileName = QSet<QString>();
Node::AddNodePosition Node::m_AddChildViewNodePosition = RightEnd;
Node::AddNodePosition Node::m_AddSiblingViewNodePosition = RightOfPrimary;

quint64 Node::SerialStart(const QByteArray &kept){
    bool ok = false;
    const quint64 start = kept.trimmed().toULongLong(&ok);
    return ok && start >= 1 && start < SERIAL_CEILING ? start : 1;
}

quint64 Node::SerialAfter(quint64 start, quint64 next){
    const quint64 after = qMax(next, start + SERIAL_SPAN);
    return after < SERIAL_CEILING ? after : 1;
}

void Node::SeedSerials(quint64 start){
    Q_ASSERT(m_SerialCounter.load() == 1);
    m_SerialCounter.store(qMax<quint64>(start, 1));
}

Node::Node()
{
    m_Folded   = true;
    m_View     = nullptr;
    m_Title    = QString();
    m_Parent   = nullptr;
    m_Primary  = nullptr;
    m_Children = NodeList();
    m_Serial   = m_SerialCounter++;
}

Node::~Node(){

    if(m_View){
        m_View->DeleteLater();
    }
    foreach(Node *nd, GetChildren()){
        delete nd;
    }
}

void Node::SetBooting(bool b){
    if(b){
        QDir imageDir = Application::ThumbnailDirectory();
        if(imageDir.exists()){
            const QStringList files = imageDir.entryList();
            m_AllImageFileName = QSet<QString>(files.cbegin(), files.cend());
        } else {
            imageDir.mkpath(Application::ThumbnailDirectory());
        }

        QDir histDir = Application::HistoryDirectory();
        if(histDir.exists()){
            const QStringList files = histDir.entryList();
            m_AllHistoryFileName = QSet<QString>(files.cbegin(), files.cend());
        } else {
            histDir.mkpath(Application::HistoryDirectory());
        }
    } else {
        foreach(QString file, m_AllImageFileName){
            QFile::remove(Application::ThumbnailDirectory() + file);
        }
        m_AllImageFileName.clear();

        foreach(QString file, m_AllHistoryFileName){
            QFile::remove(Application::HistoryDirectory() + file);
        }
        m_AllHistoryFileName.clear();
    }
    m_Booting = b;
}

void Node::LoadSettings(){
    Settings &s = Application::GlobalSettings();

    {
        QString position = s.value(QStringLiteral("application/@AddChildViewNodePosition"),
                                    QStringLiteral("RightEnd")).value<QString>();
        if(position == QStringLiteral("RightEnd"))                    m_AddChildViewNodePosition = RightEnd;
        if(position == QStringLiteral("LeftEnd"))                     m_AddChildViewNodePosition = LeftEnd;
        if(position == QStringLiteral("RightOfPrimary"))              m_AddChildViewNodePosition = RightOfPrimary;
        if(position == QStringLiteral("LeftOfPrimary"))               m_AddChildViewNodePosition = LeftOfPrimary;
        if(position == QStringLiteral("TailOfRightUnreadsOfPrimary")) m_AddChildViewNodePosition = TailOfRightUnreadsOfPrimary;
        if(position == QStringLiteral("HeadOfLeftUnreadsOfPrimary"))  m_AddChildViewNodePosition = HeadOfLeftUnreadsOfPrimary;
    }
    {
        QString position = s.value(QStringLiteral("application/@AddSiblingViewNodePosition"),
                                    QStringLiteral("RightOfPrimary")).value<QString>();
        if(position == QStringLiteral("RightEnd"))                    m_AddSiblingViewNodePosition = RightEnd;
        if(position == QStringLiteral("LeftEnd"))                     m_AddSiblingViewNodePosition = LeftEnd;
        if(position == QStringLiteral("RightOfPrimary"))              m_AddSiblingViewNodePosition = RightOfPrimary;
        if(position == QStringLiteral("LeftOfPrimary"))               m_AddSiblingViewNodePosition = LeftOfPrimary;
        if(position == QStringLiteral("TailOfRightUnreadsOfPrimary")) m_AddSiblingViewNodePosition = TailOfRightUnreadsOfPrimary;
        if(position == QStringLiteral("HeadOfLeftUnreadsOfPrimary"))  m_AddSiblingViewNodePosition = HeadOfLeftUnreadsOfPrimary;
    }
}

void Node::SaveSettings(){
    Settings &s = Application::GlobalSettings();

    {
        AddNodePosition position = m_AddChildViewNodePosition;
        if(position == RightEnd)                    s.setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("RightEnd"));
        if(position == LeftEnd)                     s.setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("LeftEnd"));
        if(position == RightOfPrimary)              s.setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("RightOfPrimary"));
        if(position == LeftOfPrimary)               s.setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("LeftOfPrimary"));
        if(position == TailOfRightUnreadsOfPrimary) s.setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("TailOfRightUnreadsOfPrimary"));
        if(position == HeadOfLeftUnreadsOfPrimary)  s.setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("HeadOfLeftUnreadsOfPrimary"));
    }
    {
        AddNodePosition position = m_AddSiblingViewNodePosition;
        if(position == RightEnd)                    s.setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("RightEnd"));
        if(position == LeftEnd)                     s.setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("LeftEnd"));
        if(position == RightOfPrimary)              s.setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("RightOfPrimary"));
        if(position == LeftOfPrimary)               s.setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("LeftOfPrimary"));
        if(position == TailOfRightUnreadsOfPrimary) s.setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("TailOfRightUnreadsOfPrimary"));
        if(position == HeadOfLeftUnreadsOfPrimary)  s.setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("HeadOfLeftUnreadsOfPrimary"));
    }
}

bool Node::IsRead(){
    return GetCreateDate() != GetLastAccessDate();
}

QString Node::ReadableTitle(){
    QString title = GetTitle();
    if(title.isEmpty()){
        const QUrl url = GetUrl();
        if(url.isEmpty()){
            if(IsDirectory()){
                title = QCoreApplication::translate("Node", "Directory");
            } else {
                title = QCoreApplication::translate("Node", "No Title");
            }
        } else {
            title = url.toString();
        }
    } else if(IsDirectory()){
        title = title.split(QStringLiteral(";")).first();
    }
    return title;
}

Node *Node::ImageOwner(){
    if(!GetImage().isNull()) return this;

    Node *nd = this;
    if(nd->IsViewNode() && nd->IsDirectory()){
        while(!nd->HasNoChildren()){
            if(nd->GetPrimary()){
                nd = nd->GetPrimary();
            } else {
                nd = nd->GetFirstChild();
            }
        }
        if(!nd->GetImage().isNull()) return nd;
    }
    return nullptr;
}

QImage Node::VisibleImage(){
    Node *owner = ImageOwner();
    return owner ? owner->GetImage() : QImage();
}

QImage Node::VisibleLargeImage(){
    Node *owner = ImageOwner();
    return owner ? owner->GetLargeImage() : QImage();
}

QIcon Node::GetIcon(){
    if(GetUrl().isEmpty()) return QIcon();
    return Application::GetIcon(GetUrl().host());
}

ViewNode::ViewNode()
    : Node()
{
    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetCreateDate(current);
        SetLastUpdateDate(current);
        SetLastAccessDate(current);
    }
    m_HoldView = false;
    m_Url      = QUrl();
    m_Image    = QImage();
    m_ImageFileName = QString();
    m_NeedToSaveImage = false;
    m_HistoryData = QByteArray();
    m_HistoryFileName = QString();
    m_NeedToSaveHistory = false;
    m_ScrollX = 0;
    m_ScrollY = 0;
    m_Zoom    = 1.0;
#ifdef MEDIATIME
    m_MediaTime = 0.0f;
#endif
    m_Type    = ViewTypeNode;
}

ViewNode::~ViewNode(){
}

bool ViewNode::IsRoot(){
    return m_Parent == nullptr;
}

bool ViewNode::IsDirectory(){
    return !m_HoldView;
}

bool ViewNode::IsViewNode() const {
    return true;
}

bool ViewNode::HoldsView() const {
    return m_HoldView;
}

bool ViewNode::TitleEditable(){
    return IsDirectory();
}

void ViewNode::SetHoldView(bool b){
    m_HoldView = b;
}

ViewNode *ViewNode::MakeChild(int position){
    if(position == -1) return MakeChild();

    ViewNode *child = new ViewNode();
    child->SetParent(this);
    InsertChild(qBound(0, position, ChildrenLength()), child);
    return child;
}

ViewNode *ViewNode::MakeChild(){
    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
    }
    ViewNode *child = new ViewNode();
    child->SetParent(this);

    int primaryIndex = m_Primary ? ChildrenIndexOf(m_Primary) : -1;

    if(m_Booting){
        AppendChild(child);
        return child;
    }

    switch(m_AddChildViewNodePosition){
    case RightEnd: AppendChild(child);  break;
    case LeftEnd:  PrependChild(child); break;

    case RightOfPrimary:

        if(m_Primary && !HasNoChildren()){
            InsertChild(primaryIndex+1, child);
        } else {
            AppendChild(child);
        }
        break;

    case LeftOfPrimary:

        if(m_Primary && !HasNoChildren()){
            InsertChild(primaryIndex, child);
        } else {
            PrependChild(child);
        }
        break;

    case TailOfRightUnreadsOfPrimary:

        if(!m_Primary || HasNoChildren() ||
           primaryIndex == ChildrenLength()-1){

            AppendChild(child);
            break;
        }
        for(int i = primaryIndex+1; i < ChildrenLength(); i++){
            if(i == ChildrenLength()-1){
                AppendChild(child);
                break;
            } else if(GetChildAt(i)->IsRead()){
                InsertChild(i, child);
                break;
            }
        }
        break;

    case HeadOfLeftUnreadsOfPrimary:

        if(!m_Primary || HasNoChildren() ||
           primaryIndex == 0){

            PrependChild(child);
            break;
        }
        for(int i = primaryIndex-1; i >= 0; i--){
            if(i == 0){
                PrependChild(child);
                break;
            } else if(GetChildAt(i)->IsRead()){
                InsertChild(i+1, child);
                break;
            }
        }
        break;
    }
    return child;
}

ViewNode *ViewNode::MakeParent(){
    if(!GetParent()) return nullptr;

    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
    }
    GetParent()->RemoveChild(this);
    if(GetParent()->GetPrimary() == this){
        if(SiblingsLength() == 0)
            GetParent()->SetPrimary(nullptr);
        else
            GetParent()->SetPrimary(GetFirstSibling());
    }
    ViewNode *parent = new ViewNode();

    GetParent()->AppendChild(parent);
    parent->SetParent(GetParent());
    parent->AppendChild(this);
    SetParent(parent);
    return parent;
}

ViewNode *ViewNode::MakeSibling(){
    ViewNode *young = new ViewNode();
    young->SetParent(m_Parent);

    int primaryIndex = SiblingsIndexOf(this);

    if(m_Booting){
        InsertSibling(primaryIndex+1, young);
        return young;
    }

    switch(m_AddSiblingViewNodePosition){
    case RightEnd: AppendSibling(young);  break;
    case LeftEnd:  PrependSibling(young); break;
    case RightOfPrimary: InsertSibling(primaryIndex+1, young); break;
    case LeftOfPrimary:  InsertSibling(primaryIndex, young);   break;

    case TailOfRightUnreadsOfPrimary:

        if(primaryIndex == SiblingsLength()-1){
            AppendSibling(young);
            break;
        }
        for(int i = primaryIndex+1; i < SiblingsLength(); i++){
            if(GetSiblingAt(i)->IsRead()){
                InsertSibling(i, young);
                break;
            } else if(i == SiblingsLength()-1){
                AppendSibling(young);
                break;
            }
        }
        break;

    case HeadOfLeftUnreadsOfPrimary:

        if(primaryIndex == 0){
            PrependSibling(young);
            break;
        }
        for(int i = primaryIndex-1; i >= 0; i--){
            if(GetSiblingAt(i)->IsRead()){
                InsertSibling(i+1, young);
                break;
            } else if(i == 0){
                PrependSibling(young);
                break;
            }
        }
        break;
    }
    return young;
}

ViewNode *ViewNode::NewDir(){
    return MakeSibling()->MakeChild();
}

ViewNode *ViewNode::Next(){
    Node *nd = this;
    if(!nd->HasNoChildren())
        return nd->GetFirstChild()->ToViewNode();
    while(nd->GetParent() && nd->GetLastSibling() == nd)
        nd = nd->GetParent();
    if(nd->IsRoot()) return nullptr;
    NodeList sibling = nd->GetSiblings();
    return sibling[sibling.indexOf(nd) + 1]->ToViewNode();
}

ViewNode *ViewNode::Prev(){
    Node *nd = this;
    if(nd->IsRoot()) return nullptr;
    if(nd->GetFirstSibling() == nd)
        return nd->GetParent()->ToViewNode();
    NodeList sibling = nd->GetSiblings();
    nd = sibling[sibling.indexOf(nd) - 1];
    while(!nd->HasNoChildren())
        nd = nd->GetLastChild();
    return nd->ToViewNode();
}

ViewNode *ViewNode::New(){
    if(m_Booting) return nullptr;
    ViewNode *vn = MakeSibling();
    vn->m_HoldView = true;
    {   QMutexLocker locker(&m_DataMutex);
        vn->m_Url = BLANK_URL;
    }
    return vn;
}

ViewNode *ViewNode::Clone(ViewNode *parent){
    if(m_Booting) return nullptr;
    if(!parent) parent = m_Parent->ToViewNode();
    ViewNode *clone = m_Parent == parent ? MakeSibling() : parent->MakeChild();
    clone->m_Type = m_Type;
    clone->m_Folded.store(m_Folded.load());
    {   QMutexLocker locker(&m_DataMutex);
        clone->m_Title = m_Title;
    }

    if(IsDirectory()){
        foreach(Node *child, GetChildren()){
            ViewNode *vn = child->ToViewNode()->Clone(clone);
            if(m_Primary == child) clone->m_Primary = vn;
        }
    } else {
        clone->m_HoldView = true;
        QString imageFrom, historyFrom;
        {   QMutexLocker locker(&m_DataMutex);
            clone->m_Url = m_Url;
            clone->m_Image = QImage(m_Image);
            imageFrom = m_ImageFileName;
            if(!imageFrom.isEmpty())
                clone->m_ImageFileName = QUuid::createUuid().toString() + QStringLiteral(".jpg");
            clone->m_HistoryData = QByteArray(m_HistoryData);
            historyFrom = m_HistoryFileName;
            if(!historyFrom.isEmpty())
                clone->m_HistoryFileName = QUuid::createUuid().toString() + QStringLiteral(".dat");
        }
        if(!imageFrom.isEmpty())
            QFile::copy(Application::ThumbnailDirectory() + imageFrom,
                        Application::ThumbnailDirectory() + clone->GetImageFileName());
        if(!historyFrom.isEmpty())
            QFile::copy(Application::HistoryDirectory() + historyFrom,
                        Application::HistoryDirectory() + clone->GetHistoryFileName());
        clone->m_ScrollX.store(m_ScrollX.load());
        clone->m_ScrollY.store(m_ScrollY.load());
        clone->m_Zoom.store(m_Zoom.load());
#ifdef MEDIATIME
        clone->m_MediaTime.store(m_MediaTime.load());
#endif
    }
    return clone;
}

QUrl ViewNode::GetUrl(){
    QMutexLocker locker(&m_DataMutex);
    return m_Url;
}

QImage ViewNode::GetImage(){
    QMutexLocker locker(&m_DataMutex);
    if(m_Image.isNull() && !m_ImageFileName.isEmpty()){
        const QString filename = m_ImageFileName;
        QtConcurrent::run([this, filename](){
            QImage image = QImage(Application::ThumbnailDirectory() + filename);
            if(!image.isNull() && image.size() != RESIDENT_THUMBNAIL_SIZE)
                image = image.scaled(RESIDENT_THUMBNAIL_SIZE,
                                     Qt::KeepAspectRatioByExpanding,
                                     Qt::SmoothTransformation);
            QMutexLocker locker(&m_DataMutex);
            if(m_Image.isNull()) m_Image = image;
        });
        return QImage();
    }
    if(m_Image.isNull()) m_ImageFileName = QString();
    return m_Image;
}

QImage ViewNode::GetLargeImage(){
    QString filename;
    QImage resident;
    {   QMutexLocker locker(&m_DataMutex);
        filename = m_ImageFileName;
        resident = m_Image;
    }
    if(!filename.isEmpty()){
        QImage image = QImage(Application::ThumbnailDirectory() + filename);
        if(!image.isNull()) return image;
    }
    return resident;
}

int ViewNode::GetScrollX(){
    return m_ScrollX;
}

int ViewNode::GetScrollY(){
    return m_ScrollY;
}

float ViewNode::GetZoom(){
    return m_Zoom;
}

#ifdef MEDIATIME
float ViewNode::GetMediaTime(){
    return m_MediaTime;
}
#endif

QDateTime ViewNode::GetCreateDate(){
    QMutexLocker locker(&m_DataMutex);
    return m_CreateDate;
}

QDateTime ViewNode::GetLastUpdateDate(){
    QMutexLocker locker(&m_DataMutex);
    return m_LastUpdateDate;
}

QDateTime ViewNode::GetLastAccessDate(){
    QMutexLocker locker(&m_DataMutex);
    return m_LastAccessDate;
}

void ViewNode::SetTitle(const QString &title){
    if(!m_Booting && IsDirectory()){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
    }
    Node::SetTitle(title);
}

void ViewNode::SetUrl(const QUrl &u){
    QMutexLocker locker(&m_DataMutex);
    if(u != m_Url && !m_Booting)
        SetLastUpdateDate(QDateTime::currentDateTime());
    m_Url = u;
}

void ViewNode::SetCreateDate(QDateTime dt){
    QMutexLocker locker(&m_DataMutex);
    m_CreateDate = dt;
}

void ViewNode::SetLastUpdateDate(QDateTime dt){
    QMutexLocker locker(&m_DataMutex);
    m_LastUpdateDate = dt;
}

void ViewNode::SetLastAccessDate(QDateTime dt){
    QMutexLocker locker(&m_DataMutex);
    m_LastAccessDate = dt;
}

void ViewNode::SetCreateDateToCurrent(){
    SetCreateDate(QDateTime::currentDateTime());
}

void ViewNode::SetLastUpdateDateToCurrent(){
    QDateTime current = QDateTime::currentDateTime();
    SetLastUpdateDate(current);
    Node *nd = this;
    while(nd && !nd->IsRoot()){
        nd->SetLastAccessDate(current);
        nd = nd->GetParent();
    }
}

void ViewNode::SetLastAccessDateToCurrent(){
    QDateTime current = QDateTime::currentDateTime();
    Node *nd = this;
    while(nd && !nd->IsRoot()){
        nd->SetLastAccessDate(current);
        nd = nd->GetParent();
    }
}

void ViewNode::SetImage(const QImage &image){
    if(image.isNull()){
        QString needless;
        {   QMutexLocker locker(&m_DataMutex);
            m_NeedToSaveImage = false;
            needless = m_ImageFileName;
            m_ImageFileName = QString();
            m_Image = image;
        }
        if(!needless.isEmpty())
            QFile::remove(Application::ThumbnailDirectory() + needless);
        return;
    }

    QString name;
    {   QMutexLocker locker(&m_DataMutex);
        name = m_ImageFileName.isEmpty()
            ? QUuid::createUuid().toString() + QStringLiteral(".jpg")
            : m_ImageFileName;
    }

    const bool saved = image.save(Application::ThumbnailDirectory() + name,
                                  nullptr, THUMBNAIL_JPEG_QUALITY);

    if(saved){
        const QImage resident = image.size() == RESIDENT_THUMBNAIL_SIZE
            ? image
            : image.scaled(RESIDENT_THUMBNAIL_SIZE,
                           Qt::KeepAspectRatioByExpanding,
                           Qt::SmoothTransformation);
        QMutexLocker locker(&m_DataMutex);
        m_ImageFileName = name;
        m_NeedToSaveImage = false;
        m_Image = resident;
    } else {
        QMutexLocker locker(&m_DataMutex);
        m_NeedToSaveImage = true;
        m_Image = image;
    }
}

void ViewNode::SetScrollX(int x){
    m_ScrollX = x;
}

void ViewNode::SetScrollY(int y){
    m_ScrollY = y;
}

void ViewNode::SetZoom(float z){
    m_Zoom = z;
}

#ifdef MEDIATIME
void ViewNode::SetMediaTime(float t){
    m_MediaTime = t;
}
#endif

QString ViewNode::GetImageFileName(){
    QMutexLocker locker(&m_DataMutex);
    return m_ImageFileName;
}

void ViewNode::SetImageFileName(const QString &s){
    QMutexLocker locker(&m_DataMutex);
    m_ImageFileName = s;
    if(!m_AllImageFileName.isEmpty())
        m_AllImageFileName.remove(s);
}

void ViewNode::SaveImageIfNeed(){
    QImage image;
    QString name;
    {   QMutexLocker locker(&m_DataMutex);
        if(!m_NeedToSaveImage || m_Image.isNull()) return;
        image = m_Image;
        name = m_ImageFileName.isEmpty()
            ? QUuid::createUuid().toString() + QStringLiteral(".jpg")
            : m_ImageFileName;
    }
    if(image.save(Application::ThumbnailDirectory() + name,
                  nullptr, THUMBNAIL_JPEG_QUALITY)){
        QMutexLocker locker(&m_DataMutex);
        if(m_NeedToSaveImage){
            m_ImageFileName = name;
            m_NeedToSaveImage = false;
        }
    }
}

QByteArray ViewNode::GetHistoryData(){
    QMutexLocker locker(&m_DataMutex);
    if(m_HistoryData.isEmpty() && !m_HistoryFileName.isEmpty() &&
       QFile::exists(Application::HistoryDirectory() + m_HistoryFileName)){
        QFile file(Application::HistoryDirectory() + m_HistoryFileName);
        if(file.open(QIODevice::ReadOnly))
            m_HistoryData = file.readAll();
        file.close();
    }
    if(m_HistoryData.isEmpty()) m_HistoryFileName = QString();
    return m_HistoryData;
}

void ViewNode::SetHistoryData(const QByteArray &ba){
    QString needless;
    {   QMutexLocker locker(&m_DataMutex);
        if(ba.isEmpty()){
            m_NeedToSaveHistory = false;
            needless = m_HistoryFileName;
            m_HistoryFileName = QString();
        } else {
            m_NeedToSaveHistory = true;
        }
        m_HistoryData = ba;
    }
    if(!needless.isEmpty())
        QFile::remove(Application::HistoryDirectory() + needless);
}

QString ViewNode::GetHistoryFileName(){
    QMutexLocker locker(&m_DataMutex);
    return m_HistoryFileName;
}

void ViewNode::SetHistoryFileName(const QString &s){
    QMutexLocker locker(&m_DataMutex);
    m_HistoryFileName = s;
    if(!m_AllHistoryFileName.isEmpty())
        m_AllHistoryFileName.remove(s);
}

void ViewNode::SaveHistoryIfNeed(){
    QByteArray data;
    QString name;
    {   QMutexLocker locker(&m_DataMutex);
        if(!m_NeedToSaveHistory || m_HistoryData.isEmpty()) return;
        if(m_HistoryFileName.isEmpty())
            m_HistoryFileName = QUuid::createUuid().toString() + QStringLiteral(".dat");
        name = m_HistoryFileName;
        data = m_HistoryData;
        m_NeedToSaveHistory = false;
    }
    QFile file(Application::HistoryDirectory() + name);
    if(file.open(QIODevice::WriteOnly))
        file.write(data);
    file.close();
}

QCache<QString, QImage> LocalNode::m_FileImageCache(DEFAULT_LOCALVIEW_MAX_FILEIMAGE);
QMutex LocalNode::m_DiskAccessMutex = QMutex();

LocalNode::LocalNode()
    : Node()
{
    m_Url = QUrl();
    m_Type = LocalTypeNode;
    m_Checked = false;
    m_DirFlag = false;
}

LocalNode::~LocalNode(){}

bool LocalNode::IsRoot(){
#ifdef Q_OS_WIN
    return m_Url.toString().endsWith(QStringLiteral(":/"))
        || m_Url.toString() == QStringLiteral("file:///");
#else
    return m_Url.toString() == QStringLiteral("file:///");
#endif
}

bool LocalNode::IsDirectory(){
    if(m_Checked) return m_DirFlag;
    m_Checked = true;
    m_DirFlag = QFileInfo(m_Url.toLocalFile()).isDir();
    return m_DirFlag;
}

bool LocalNode::IsViewNode() const {
    return false;
}

bool LocalNode::TitleEditable(){
#ifdef Q_OS_WIN
    return !m_Title.endsWith(QStringLiteral(":/"));
#else
    return true;
#endif
}

void LocalNode::SetTitle(const QString &title){
    if(!m_Title.isEmpty()){
        QString path = m_Url.toLocalFile();
        QStringList list = path.split(QStringLiteral("/"));
        QString name = list.takeLast();

#ifdef Q_OS_WIN
        Q_ASSERT(m_Title.endsWith(QStringLiteral(":/")) || m_Title == name);
#else
        Q_ASSERT(m_Title == name);
#endif
        list << title;
        QString newPath = list.join(QStringLiteral("/"));

        if(!QFile::rename(path, newPath)) return;

        SetUrl(QUrl::fromLocalFile(newPath));
    }
    Node::SetTitle(title);
}

QUrl LocalNode::GetUrl(){
    return m_Url;
}

void LocalNode::SetUrl(const QUrl &u){
    m_Url = u;
    QString path = u.toLocalFile();
#ifdef Q_OS_WIN
    if(path.endsWith(QStringLiteral(":/"))){
        m_Title = path.right(3);
    } else
#endif
    if(path.endsWith(QStringLiteral("/"))){
        QStringList list = path.split(QStringLiteral("/"));
        list.removeLast();
        m_Title = list.last() + QStringLiteral("/");
    } else {
        m_Title = path.split(QStringLiteral("/")).last();
    }
}

QDateTime LocalNode::GetCreateDate(){
    if(m_CreateDate.isValid())
        return m_CreateDate;
    return m_CreateDate = QFileInfo(m_Url.toLocalFile()).birthTime();
}

QDateTime LocalNode::GetLastUpdateDate(){
    if(m_LastUpdateDate.isValid())
        return m_LastUpdateDate;
    return m_LastUpdateDate = QFileInfo(m_Url.toLocalFile()).lastModified();
}

QDateTime LocalNode::GetLastAccessDate(){
    if(m_LastAccessDate.isValid())
        return m_LastAccessDate;
    return m_LastAccessDate = QFileInfo(m_Url.toLocalFile()).lastRead();
}

QImage LocalNode::GetImage(){
    if(QImage *i = m_FileImageCache.object(m_Url.toLocalFile()))
        return *i;
    return QImage();
}

