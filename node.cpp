#include "switch.hpp"
#include "const.hpp"

#ifndef USE_LIGHTNODE

#include "node.hpp"

#include "application.hpp"
#include "treebank.hpp"
#include "view.hpp"

#include <QUuid>

bool Node::m_Booting = false;
QStringList Node::m_AllImageFileName = QStringList();
QStringList Node::m_AllHistoryFileName = QStringList();
bool Node::m_EnableDeepCopyOfNode = false;
Node::AddNodePosition Node::m_AddChildViewNodePosition = RightEnd;
Node::AddNodePosition Node::m_AddSiblingViewNodePosition = RightOfPrimary;

Node::Node()
    : QObject(0)
{
    m_Folded   = true;
    m_View     = 0;
    m_Title    = QString();
    m_Parent   = 0;
    m_Primary  = 0;
    m_Partner  = 0;
    m_Children = NodeList();
}

Node::~Node(){
    //Q_ASSERT(!m_View);

    if(m_View){
        m_View->DeleteLater();
    }
    foreach(Node *nd, GetChildren()){
        nd->deleteLater();
    }
}

void Node::SetBooting(bool b){
    if(b){
        QDir imageDir = Application::ThumbnailDirectory();
        if(imageDir.exists()){
            m_AllImageFileName = imageDir.entryList();
        } else {
            imageDir.mkpath(Application::ThumbnailDirectory());
        }

        QDir histDir = Application::HistoryDirectory();
        if(histDir.exists()){
            m_AllHistoryFileName = histDir.entryList();
        } else {
            histDir.mkpath(Application::HistoryDirectory());
        }
    } else {
        foreach(QString file, m_AllImageFileName){
            // 'QFile::remove' can't delete directory('.' or '..').
            // when trying to delete, 'remove' returns false.
            QFile::remove(Application::ThumbnailDirectory() + file);
        }
        m_AllImageFileName.clear();

        foreach(QString file, m_AllHistoryFileName){
            // 'QFile::remove' can't delete directory('.' or '..').
            // when trying to delete, 'remove' returns false.
            QFile::remove(Application::HistoryDirectory() + file);
        }
        m_AllHistoryFileName.clear();
    }
    m_Booting = b;
}

void Node::LoadSettings(){
    QSettings *s = Application::GlobalSettings();
    if(!s->group().isEmpty()) return;

    m_EnableDeepCopyOfNode = s->value(QStringLiteral("application/@EnableDeepCopyOfNode") , false).value<bool>();
    {
        QString position = s->value(QStringLiteral("application/@AddChildViewNodePosition"),
                                    QStringLiteral("RightEnd")).value<QString>();
        if(position == QStringLiteral("RightEnd"))                    m_AddChildViewNodePosition = RightEnd;
        if(position == QStringLiteral("LeftEnd"))                     m_AddChildViewNodePosition = LeftEnd;
        if(position == QStringLiteral("RightOfPrimary"))              m_AddChildViewNodePosition = RightOfPrimary;
        if(position == QStringLiteral("LeftOfPrimary"))               m_AddChildViewNodePosition = LeftOfPrimary;
        if(position == QStringLiteral("TailOfRightUnreadsOfPrimary")) m_AddChildViewNodePosition = TailOfRightUnreadsOfPrimary;
        if(position == QStringLiteral("HeadOfLeftUnreadsOfPrimary"))  m_AddChildViewNodePosition = HeadOfLeftUnreadsOfPrimary;
    }
    {
        QString position = s->value(QStringLiteral("application/@AddSiblingViewNodePosition"),
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
    QSettings *s = Application::GlobalSettings();
    if(!s->group().isEmpty()) return;

    s->setValue(QStringLiteral("application/@EnableDeepCopyOfNode"), m_EnableDeepCopyOfNode);
    {
        AddNodePosition position = m_AddChildViewNodePosition;
        if(position == RightEnd)                    s->setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("RightEnd"));
        if(position == LeftEnd)                     s->setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("LeftEnd"));
        if(position == RightOfPrimary)              s->setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("RightOfPrimary"));
        if(position == LeftOfPrimary)               s->setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("LeftOfPrimary"));
        if(position == TailOfRightUnreadsOfPrimary) s->setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("TailOfRightUnreadsOfPrimary"));
        if(position == HeadOfLeftUnreadsOfPrimary)  s->setValue(QStringLiteral("application/@AddChildViewNodePosition"), QStringLiteral("HeadOfLeftUnreadsOfPrimary"));
    }
    {
        AddNodePosition position = m_AddSiblingViewNodePosition;
        if(position == RightEnd)                    s->setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("RightEnd"));
        if(position == LeftEnd)                     s->setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("LeftEnd"));
        if(position == RightOfPrimary)              s->setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("RightOfPrimary"));
        if(position == LeftOfPrimary)               s->setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("LeftOfPrimary"));
        if(position == TailOfRightUnreadsOfPrimary) s->setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("TailOfRightUnreadsOfPrimary"));
        if(position == HeadOfLeftUnreadsOfPrimary)  s->setValue(QStringLiteral("application/@AddSiblingViewNodePosition"), QStringLiteral("HeadOfLeftUnreadsOfPrimary"));
    }
}

bool Node::IsRead(){
    return GetCreateDate() != GetLastAccessDate();
}

QString Node::ReadableTitle(){
    QString title = m_Title;
    if(title.isEmpty()){
        const QUrl url = GetUrl();
        if(url.isEmpty()){
            if(IsDirectory()){
                title = QStringLiteral("Directory");
            } else {
                title = QStringLiteral("No Title");
            }
        } else {
            title = url.toString();
        }
    } else if(IsDirectory()){
        title = QStringLiteral("Dir - ") + title.split(QStringLiteral(";")).first();
    }
    return title;
}

void Node::SetCreateDateToCurrent(){
    SetCreateDate(QDateTime::currentDateTime());
}

void Node::SetLastUpdateDateToCurrent(){
    SetLastUpdateDate(QDateTime::currentDateTime());
}

void Node::SetLastAccessDateToCurrent(){
    SetLastAccessDate(QDateTime::currentDateTime());
}

HistNode::HistNode()
    : Node()
{
    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetCreateDate(current);
        SetLastUpdateDate(current);
        SetLastAccessDate(current);
        if(m_Partner){
            m_Partner->SetLastUpdateDate(current);
            m_Partner->SetLastAccessDate(current);
        }
    }
    m_Url     = QUrl();
    m_Image   = QImage();
    m_ImageFileName = QString();
    m_NeedToSaveImage = false;
    m_HistoryData = QByteArray();
    m_HistoryFileName = QString();
    m_NeedToSaveHistory = false;
    m_ScrollX = 0;
    m_ScrollY = 0;
    m_Zoom    = 1.0;
    m_Type    = HistTypeNode;
}

HistNode::~HistNode(){
    if(!m_Booting && m_Partner)
        m_Partner->SetLastUpdateDate(QDateTime::currentDateTime());

    // 'GetPartner' and 'SetPartner' is
    // need to be inline method(if virtual),
    // because here is destructor(perhaps...).
    if(m_Partner && m_Partner->GetPartner() == this)
        m_Partner->SetPartner(0);

    //if(view)
    //    view->DeleteLater();
}

bool HistNode::IsRoot(){
    return m_Parent == 0 || m_Parent->GetParent() == 0;
}

bool HistNode::IsDirectory(){
    return false;
}

bool HistNode::IsHistNode() const {
    return true;
}

bool HistNode::IsViewNode() const {
    return false;
}

bool HistNode::TitleEditable(){
    return false;
}

HistNode *HistNode::MakeChild(){
    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
        if(m_Partner){
            m_Partner->SetLastUpdateDate(current);
        }
    }
    HistNode *child = new HistNode();
    child->SetParent(this);
    child->SetZoom(m_Zoom);
    AppendChild(child);
    return child;
}

HistNode *HistNode::MakeParent(){
    if(!GetParent()) return 0;

    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
        if(m_Partner){
            m_Partner->SetLastUpdateDate(current);
        }
    }

    GetParent()->RemoveChild(this);
    if(GetParent()->GetPrimary() == this){
        if(SiblingsLength() == 0)
            GetParent()->SetPrimary(0);
        else
            GetParent()->SetPrimary(GetFirstSibling());
    }
    HistNode *parent = new HistNode();

    // root doesn't have zoom factor.
    GetParent()->AppendChild(parent);
    parent->SetParent(GetParent());
    parent->SetZoom(m_Zoom);
    parent->AppendChild(this);
    SetParent(parent);
    return parent;
}

HistNode *HistNode::Next(){
    if(HasNoChildren()) return 0;
    return (m_Primary ? m_Primary : GetLastChild())->ToHistNode();
}

HistNode *HistNode::Prev(){
    if(this->IsRoot()) return 0;
    return m_Parent->ToHistNode();
}

HistNode *HistNode::New(){
    if(m_Booting) return 0;
    HistNode *hn = MakeChild();
    hn->m_Partner = m_Partner;
    hn->m_Url = QUrl(QStringLiteral("about:blank"));
    return hn;
}

HistNode *HistNode::Clone(HistNode *parent, ViewNode *partner){
    if(m_Booting) return 0;
    if(!parent)  parent  = this;
    if(!partner) partner = m_Partner->ToViewNode();
    NodeList children = m_Children; // copy.
    HistNode *clone = parent->MakeChild();
    // Node's member.
    clone->m_Type = m_Type;
    clone->m_Folded = m_Folded;
    clone->m_Title = m_Title;
    clone->m_Partner = partner;
    // set or replace partner.
    if(m_Partner->GetPartner() == this)
        partner->m_Partner = clone;
    // HistNode's member.
    clone->m_Url = m_Url;
    clone->m_Image = QImage(m_Image);
    if(!m_ImageFileName.isEmpty()){
        clone->m_ImageFileName = QUuid::createUuid().toString() + QStringLiteral(".jpg");
        QFile::copy(Application::ThumbnailDirectory() + m_ImageFileName,
                    Application::ThumbnailDirectory() + clone->m_ImageFileName);
    }
    clone->m_HistoryData = QByteArray(m_HistoryData);
    if(!m_HistoryFileName.isEmpty()){
        clone->m_HistoryFileName = QUuid::createUuid().toString() + QStringLiteral(".dat");
        QFile::copy(Application::HistoryDirectory() + m_HistoryFileName,
                    Application::HistoryDirectory() + clone->m_HistoryFileName);
    }
    clone->m_ScrollX = m_ScrollX;
    clone->m_ScrollY = m_ScrollY;
    clone->m_Zoom = m_Zoom;
    if(m_EnableDeepCopyOfNode){
        // Node::m_Children, Node::m_Primary
        foreach(Node *child, children){
            HistNode *hn = child->ToHistNode()->Clone(clone, partner);
            if(m_Primary == child) clone->m_Primary = hn;
        }
    } else {
        partner->m_Partner = clone;
    }
    return clone;
}

QUrl HistNode::GetUrl(){
    return m_Url;
}

QImage HistNode::GetImage(){
    if(m_Image.isNull() && !m_ImageFileName.isEmpty()){
        QTimer::singleShot(0, [this](){
                QImage image = QImage(Application::ThumbnailDirectory() + m_ImageFileName);
                if(m_Image.isNull()) m_Image = image;
            });
        return QImage();
    }
    if(m_Image.isNull()) m_ImageFileName = QString();
    return m_Image;
}

int HistNode::GetScrollX(){
    return m_ScrollX;
}

int HistNode::GetScrollY(){
    return m_ScrollY;
}

float HistNode::GetZoom(){
    return m_Zoom;
}

QDateTime HistNode::GetCreateDate(){
    return m_CreateDate;
}

QDateTime HistNode::GetLastUpdateDate(){
    return m_LastUpdateDate;
}

QDateTime HistNode::GetLastAccessDate(){
    return m_LastAccessDate;
}

void HistNode::SetUrl(const QUrl &u){
    if(u != m_Url && !m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
        if(m_Partner){
            m_Partner->SetLastUpdateDate(current);
        }
    }
    m_Url = u;
}

void HistNode::SetCreateDate(QDateTime dt){
    m_CreateDate = dt;
}

void HistNode::SetLastUpdateDate(QDateTime dt){
    m_LastUpdateDate = dt;
}

void HistNode::SetLastAccessDate(QDateTime dt){
    m_LastAccessDate = dt;
}

void HistNode::SetImage(const QImage &image){
    if(image.isNull()){
        m_NeedToSaveImage = false;
        if(!m_ImageFileName.isEmpty()){
            QFile::remove(Application::ThumbnailDirectory() + m_ImageFileName);
            m_ImageFileName = QString();
        }
    } else {
        m_NeedToSaveImage = true;
    }
    m_Image = image;
}

void HistNode::SetScrollX(int x){
    m_ScrollX = x;
}

void HistNode::SetScrollY(int y){
    m_ScrollY = y;
}

void HistNode::SetZoom(float z){
    m_Zoom = z;
}

QString HistNode::GetImageFileName(){
    return m_ImageFileName;
}

void HistNode::SetImageFileName(const QString &s){
    m_ImageFileName = s;
    if(!m_AllImageFileName.isEmpty())
        m_AllImageFileName.removeOne(s);
}

void HistNode::SaveImageIfNeed(){
    if(m_NeedToSaveImage && !m_Image.isNull()){
        if(m_ImageFileName.isEmpty())
            m_ImageFileName = QUuid::createUuid().toString() + QStringLiteral(".jpg");
        m_Image.save(Application::ThumbnailDirectory() + m_ImageFileName);
        m_NeedToSaveImage = false;
    }
}

QByteArray HistNode::GetHistoryData(){
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

void HistNode::SetHistoryData(const QByteArray &ba){
    if(ba.isEmpty()){
        m_NeedToSaveHistory = false;
        if(!m_HistoryFileName.isEmpty()){
            QFile::remove(Application::HistoryDirectory() + m_HistoryFileName);
            m_HistoryFileName = QString();
        }
    } else {
        m_NeedToSaveHistory = true;
    }
    m_HistoryData = ba;
}

QString HistNode::GetHistoryFileName(){
    return m_HistoryFileName;
}

void HistNode::SetHistoryFileName(const QString &s){
    m_HistoryFileName = s;
    if(!m_AllHistoryFileName.isEmpty())
        m_AllHistoryFileName.removeOne(s);
}

void HistNode::SaveHistoryIfNeed(){
    if(m_NeedToSaveHistory && !m_HistoryData.isEmpty()){
        if(m_HistoryFileName.isEmpty())
            m_HistoryFileName = QUuid::createUuid().toString() + QStringLiteral(".dat");

        QFile file(Application::HistoryDirectory() + m_HistoryFileName);
        if(file.open(QIODevice::WriteOnly))
            file.write(m_HistoryData);
        file.close();
        m_NeedToSaveHistory = false;
    }
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
    m_Type = ViewTypeNode;
}

ViewNode::~ViewNode(){
    if(m_Partner){
        Node *nd = m_Partner;

        while(nd->GetParent() && !nd->IsRoot())
            nd = nd->GetParent();

        nd->deleteLater();
    }
}

bool ViewNode::IsRoot(){
    return m_Parent == 0;
}

bool ViewNode::IsDirectory(){
    return !m_Partner;
}

bool ViewNode::IsHistNode() const {
    return false;
}

bool ViewNode::IsViewNode() const {
    return true;
}

bool ViewNode::TitleEditable(){
    return IsDirectory();
}

ViewNode *ViewNode::MakeChild(bool forceAppend){
    if(!forceAppend) return MakeChild();

    ViewNode *child = new ViewNode();
    child->SetParent(this);
    AppendChild(child);
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
    if(!GetParent()) return 0;

    if(!m_Booting){
        QDateTime current = QDateTime::currentDateTime();
        SetLastUpdateDate(current);
    }
    GetParent()->RemoveChild(this);
    if(GetParent()->GetPrimary() == this){
        if(SiblingsLength() == 0)
            GetParent()->SetPrimary(0);
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

// 'Next' and 'Prev' don't create infinite loop.
// at begin or end of tree, they return 0.

ViewNode *ViewNode::Next(){
    Node *nd = this;
    if(!nd->HasNoChildren())
        return nd->GetFirstChild()->ToViewNode();
    while(nd->GetParent() && nd->GetLastSibling() == nd)
        nd = nd->GetParent();
    if(nd->IsRoot()) return 0;
    NodeList sibling = nd->GetSiblings();
    return sibling[sibling.indexOf(nd) + 1]->ToViewNode();
}

ViewNode *ViewNode::Prev(){
    Node *nd = this;
    if(nd->IsRoot()) return 0;
    if(nd->GetFirstSibling() == nd)
        return nd->GetParent()->ToViewNode();
    NodeList sibling = nd->GetSiblings();
    nd = sibling[sibling.indexOf(nd) - 1];
    while(!nd->HasNoChildren())
        nd = nd->GetLastChild();
    return nd->ToViewNode();
}

ViewNode *ViewNode::New(){
    if(m_Booting) return 0;
    ViewNode *vn = MakeSibling();
    HistNode *hn = TreeBank::GetHistRoot()->MakeChild();
    vn->m_Partner = hn;
    hn->m_Partner = vn;
    hn->m_Url = QUrl(QStringLiteral("about:blank"));
    return vn;
}

ViewNode *ViewNode::Clone(ViewNode *parent){
    if(m_Booting) return 0;
    if(!parent) parent = m_Parent->ToViewNode();
    ViewNode *clone = m_Parent == parent ? MakeSibling() : parent->MakeChild();
    // Node's member.
    clone->m_Type = m_Type;
    clone->m_Folded = m_Folded;
    clone->m_Title = m_Title;

    if(m_EnableDeepCopyOfNode){
        // HistNode::Clone sets ViewNode::m_Partner automatically.
        if(IsDirectory()){
            foreach(Node *child, GetChildren()){
                ViewNode *vn = child->ToViewNode()->Clone(clone);
                if(m_Primary == child) clone->m_Primary = vn;
            }
        } else {
            HistNode *root = m_Partner->GetRoot()->ToHistNode();
            root->Clone(TreeBank::GetHistRoot(), clone);
        }
    } else {
        if(!IsDirectory()){
            m_Partner->ToHistNode()->Clone(TreeBank::GetHistRoot(), clone);
        }
    }
    return clone;
}

QUrl ViewNode::GetUrl(){
    return m_Partner ? m_Partner->GetUrl() : QUrl();
}

QImage ViewNode::GetImage(){
    return m_Partner ? m_Partner->GetImage() : QImage();
}

int ViewNode::GetScrollX(){
    return m_Partner ? m_Partner->GetScrollX() : 0;
}

int ViewNode::GetScrollY(){
    return m_Partner ? m_Partner->GetScrollY() : 0;
}

float ViewNode::GetZoom(){
    return m_Partner ? m_Partner->GetZoom() : 1.0;
}

QDateTime ViewNode::GetCreateDate(){
    return m_CreateDate;
}

QDateTime ViewNode::GetLastUpdateDate(){
    return m_LastUpdateDate;
}

QDateTime ViewNode::GetLastAccessDate(){
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
    if(m_Partner)
        m_Partner->SetUrl(u);
}

void ViewNode::SetCreateDate(QDateTime dt){
    m_CreateDate = dt;
}

void ViewNode::SetLastUpdateDate(QDateTime dt){
    m_LastUpdateDate = dt;
}

void ViewNode::SetLastAccessDate(QDateTime dt){
    m_LastAccessDate = dt;
}

QCache<QString, QImage> LocalNode::m_FileImageCache(DEFAULT_LOCALVIEW_MAX_FILEIMAGE);
QMutex LocalNode::m_DiskAccessMutex(QMutex::NonRecursive);

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

bool LocalNode::IsHistNode() const {
    return false;
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
    return m_CreateDate = QFileInfo(m_Url.toLocalFile()).created();
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

// not instance specific object.
QImage LocalNode::GetImage(){
    if(QImage *i = m_FileImageCache.object(m_Url.toLocalFile()))
        return *i;
    return QImage();
}

#endif //ifndef USE_LIGHTNODE
