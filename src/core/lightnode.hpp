#ifndef LIGHTNODE_HPP
#define LIGHTNODE_HPP

#include "switch.hpp"
#include "const.hpp"

#include <QList>
#include <QUrl>
#include <QImage>

#include <QCache>
#include <QMutex>
#include <QSet>
#include <QFileInfo>

#include <atomic>

class QString;
class QUrl;
class QImage;
class QDateTime;
class View;

class Node;
class ViewNode;
class LocalNode;

typedef QList<Node*>      NodeList;
typedef QList<ViewNode*>  ViewNodeList;
typedef QList<LocalNode*> LocalNodeList;

class Node {

public:
    enum NodeType {
        ViewTypeNode,
        LocalTypeNode
    } m_Type;

protected:
    enum AddNodePosition {
        RightEnd,
        LeftEnd,
        RightOfPrimary,
        LeftOfPrimary,
        TailOfRightUnreadsOfPrimary,
        HeadOfLeftUnreadsOfPrimary,
    };

    static AddNodePosition m_AddChildViewNodePosition;
    static AddNodePosition m_AddSiblingViewNodePosition;

    View *m_View;

    std::atomic_bool m_Folded;
    QString m_Title;

    Node *m_Parent;
    Node *m_Primary;
    NodeList m_Children;
    quint64 m_Serial;
    static std::atomic<quint64> m_SerialCounter;
    static QRecursiveMutex m_DataMutex;
    static bool m_Booting;
    static QSet<QString> m_AllImageFileName;
    static QSet<QString> m_AllHistoryFileName;

public:
    Node();
    virtual ~Node();

    void Delete(){ delete this;}

    static void SetBooting(bool b);

    static void LoadSettings();
    static void SaveSettings();

    bool IsRead();
    QString ReadableTitle();
    Node *ImageOwner();
    QImage VisibleImage();
    QImage VisibleLargeImage();
    QIcon GetIcon();

    virtual bool IsRoot(){ return false;}
    virtual bool IsDirectory(){ return false;}
    virtual bool IsViewNode() const { return false;}
    virtual bool IsDummy()    const { return false;}
    virtual bool HoldsView()  const { return false;}
    virtual bool TitleEditable(){ return false;}
    virtual Node *MakeChild() { return nullptr;}
    virtual Node *MakeParent(){ return nullptr;}
    virtual Node *Next()      { return nullptr;}
    virtual Node *Prev()      { return nullptr;}
    virtual ViewNode  *ToViewNode(){ return nullptr;}
    virtual LocalNode *ToLocalNode(){ return nullptr;}

    NodeType GetType() const { return m_Type;}
    View *GetView()    const { return m_View;}
    QString GetTitle() const {
        QMutexLocker locker(&m_DataMutex);
        return m_Title;
    }
    quint64 GetSerial() const { return m_Serial;}

    static constexpr quint64 SERIAL_SPAN = quint64(1) << 20;
    static constexpr quint64 SERIAL_CEILING = (quint64(1) << 31) - (quint64(1) << 21);
    static quint64 SerialStart(const QByteArray &kept);
    static quint64 SerialAfter(quint64 start, quint64 next);
    static void SeedSerials(quint64 start);
    static quint64 NextSerial(){ return m_SerialCounter.load();}

    bool GetFolded()   const { return m_Folded;}
    Node *GetParent()  const { return m_Parent;}
    Node *GetPrimary() const { return m_Primary;}
    NodeList GetChildren() const {
        QMutexLocker locker(&m_DataMutex);
        return m_Children;
    }
    NodeList GetSiblings() const {
        static NodeList empty = NodeList();
        if(!m_Parent) return empty;
        return m_Parent->GetChildren();
    }
    NodeList GetAncestors(){
        NodeList list = NodeList();
        Node *nd = this;
        while(nd && !nd->IsRoot()){
            Node *parent = nd->GetParent();
            if(parent) list << parent;
            nd = parent;
        }
        return list;
    }
    NodeList GetDescendants(){
        NodeList list = NodeList();
        foreach(Node *nd, m_Children){
            list << nd->GetDescendants();
        }
        return m_Children + list;
    }
    Node *GetRoot(){
        Node *nd = this;
        while(nd->GetParent() && !nd->IsRoot())
            nd = nd->GetParent();
        return nd;
    }
    bool HasNoChildren() const {
        return m_Children.isEmpty();
    }
    bool HasNoSiblings() const {
        if(!m_Parent) return true;
        return m_Parent->HasNoChildren();
    }
    bool IsParentOf(Node *nd) const {
        return nd ? nd->GetParent() == this : false;
    }
    bool IsPrimaryOf(Node *nd) const {
        return nd ? nd->GetPrimary() == this : false;
    }
    bool IsChildOf(Node *nd) const {
        return nd ? nd->ChildrenContains(const_cast<Node* const>(this)) : false;
    }
    bool IsSiblingOf(Node *nd) const {
        return nd ? nd->SiblingsContains(const_cast<Node* const>(this)) : false;
    }
    bool IsAncestorOf(Node *nd) const {
        return nd ? nd->GetAncestors().contains(const_cast<Node* const>(this)) : false;
    }
    bool IsDescendantOf(Node *nd) const {
        return nd ? const_cast<Node* const>(this)->GetAncestors().contains(nd) : false;
    }
    bool IsPrimaryOfParent() const {
        return IsPrimaryOf(m_Parent);
    }

    void SetView(View *v)   { m_View = v;}
    virtual void SetTitle(const QString &s){
        QMutexLocker locker(&m_DataMutex);
        m_Title = s;
    }

    void SetFolded(bool b)   { m_Folded = b;}
    void SetParent(Node *nd) { m_Parent  = nd;}
    void SetPrimary(Node *nd){ m_Primary = nd;}

    void ResetPrimaryPath(){
        Node *nd = this;
        while(nd->m_Parent){
            nd->m_Parent->m_Primary = nd;
            nd = nd->m_Parent;
        }
    }
    void SetChildren(NodeList c){
        QMutexLocker locker(&m_DataMutex);
        m_Children = c;
    }
    void ClearChildren(){
        QMutexLocker locker(&m_DataMutex);
        m_Children.clear();
    }
    int ChildrenLength() const {
        return m_Children.length();
    }
    int ChildrenIndexOf(Node *nd) const {
        return m_Children.indexOf(nd);
    }
    bool ChildrenContains(Node *nd) const {
        return m_Children.contains(nd);
    }
    void AppendChild(Node *nd){
        QMutexLocker locker(&m_DataMutex);
        m_Children.append(nd);
    }
    void PrependChild(Node *nd){
        QMutexLocker locker(&m_DataMutex);
        m_Children.prepend(nd);
    }
    void RemoveChild(Node *nd){
        QMutexLocker locker(&m_DataMutex);
        m_Children.removeOne(nd);
    }
    void InsertChild(int i, Node *nd){
        QMutexLocker locker(&m_DataMutex);
        m_Children.insert(i, nd);
    }
    void MoveChild(int from, int to){
        QMutexLocker locker(&m_DataMutex);
        m_Children.move(from, to);
    }
    Node *GetChildAt(int i) const {
        return m_Children.at(i);
    }
    Node *GetFirstChild() const {
        if(m_Children.isEmpty()) return nullptr;
        return m_Children.first();
    }
    Node *GetLastChild() const {
        if(m_Children.isEmpty()) return nullptr;
        return m_Children.last();
    }
    Node *TakeFirstChild(){
        QMutexLocker locker(&m_DataMutex);
        if(m_Children.isEmpty()) return nullptr;
        return m_Children.takeFirst();
    }
    Node *TakeLastChild(){
        QMutexLocker locker(&m_DataMutex);
        if(m_Children.isEmpty()) return nullptr;
        return m_Children.takeLast();
    }
    int SiblingsLength() const {
        if(!m_Parent) return 0;
        return m_Parent->ChildrenLength();
    }
    int SiblingsIndexOf(Node *nd) const {
        if(!m_Parent) return 0;
        return m_Parent->ChildrenIndexOf(nd);
    }
    bool SiblingsContains(Node *nd) const {
        if(!m_Parent) return false;
        return m_Parent->ChildrenContains(nd);
    }
    void AppendSibling(Node *nd){
        if(m_Parent) m_Parent->AppendChild(nd);
    }
    void PrependSibling(Node *nd){
        if(m_Parent) m_Parent->PrependSibling(nd);
    }
    void RemoveSibling(Node *nd){
        if(m_Parent) m_Parent->RemoveChild(nd);
    }
    void InsertSibling(int i, Node *nd){
        if(m_Parent) m_Parent->InsertChild(i, nd);
    }
    void MoveSibling(int from, int to){
        if(m_Parent) m_Parent->MoveChild(from, to);
    }
    Node *GetSiblingAt(int i) const {
        if(!m_Parent) return nullptr;
        return m_Parent->GetChildAt(i);
    }
    Node *GetFirstSibling() const {
        if(!m_Parent) return nullptr;
        return m_Parent->GetFirstChild();
    }
    Node *GetLastSibling() const {
        if(!m_Parent) return nullptr;
        return m_Parent->GetLastChild();
    }
    Node *TakeFirstSibling(){
        if(!m_Parent) return nullptr;
        return m_Parent->TakeFirstChild();
    }
    Node *TakeLastSibling(){
        if(!m_Parent) return nullptr;
        return m_Parent->TakeLastChild();
    }
    int Index(){
        return SiblingsIndexOf(this);
    }

    virtual QUrl GetUrl()    { return QUrl();}
    virtual QImage GetImage(){ return QImage();}
    virtual QImage GetLargeImage(){ return GetImage();}
    virtual int GetScrollX() { return 0;}
    virtual int GetScrollY() { return 0;}
    virtual float GetZoom()  { return 1.0;}
    virtual QDateTime GetCreateDate(){ return QDateTime();}
    virtual QDateTime GetLastUpdateDate(){ return QDateTime();}
    virtual QDateTime GetLastAccessDate(){ return QDateTime();}

    virtual void SetUrl(const QUrl&){}
    virtual void SetCreateDate(QDateTime){}
    virtual void SetLastUpdateDate(QDateTime){}
    virtual void SetLastAccessDate(QDateTime){}

    virtual void SetCreateDateToCurrent(){}
    virtual void SetLastUpdateDateToCurrent(){}
    virtual void SetLastAccessDateToCurrent(){}
};

class ViewNode : public Node {

private:
    std::atomic_bool m_HoldView;

    QUrl m_Url;

    QImage m_Image;
    QString m_ImageFileName;
    std::atomic_bool m_NeedToSaveImage;

    QByteArray m_HistoryData;
    QString m_HistoryFileName;
    std::atomic_bool m_NeedToSaveHistory;

    std::atomic_int m_ScrollX;
    std::atomic_int m_ScrollY;
    std::atomic<float> m_Zoom;
#ifdef MEDIATIME
    std::atomic<float> m_MediaTime;
#endif

    QDateTime m_CreateDate;
    QDateTime m_LastUpdateDate;
    QDateTime m_LastAccessDate;

public:
    ViewNode();
    virtual ~ViewNode() Q_DECL_OVERRIDE;

    bool IsRoot() Q_DECL_OVERRIDE;
    bool IsDirectory() Q_DECL_OVERRIDE;
    bool IsViewNode() const Q_DECL_OVERRIDE;
    bool HoldsView() const Q_DECL_OVERRIDE;
    bool TitleEditable() Q_DECL_OVERRIDE;

    void SetHoldView(bool b);

    ViewNode *MakeChild(int);
    ViewNode *MakeChild() Q_DECL_OVERRIDE;
    ViewNode *MakeParent() Q_DECL_OVERRIDE;
    ViewNode *MakeSibling();
    ViewNode *NewDir();
    ViewNode *Next() Q_DECL_OVERRIDE;
    ViewNode *Prev() Q_DECL_OVERRIDE;
    ViewNode *ToViewNode() Q_DECL_OVERRIDE { return this;}
    ViewNode *Clone(ViewNode *parent = nullptr);
    ViewNode *New();

    QUrl GetUrl() Q_DECL_OVERRIDE;
    QImage GetImage() Q_DECL_OVERRIDE;
    QImage GetLargeImage() Q_DECL_OVERRIDE;
    int GetScrollX() Q_DECL_OVERRIDE;
    int GetScrollY() Q_DECL_OVERRIDE;
    float GetZoom() Q_DECL_OVERRIDE;
#ifdef MEDIATIME
    float GetMediaTime();
#endif
    QDateTime GetCreateDate() Q_DECL_OVERRIDE;
    QDateTime GetLastUpdateDate() Q_DECL_OVERRIDE;
    QDateTime GetLastAccessDate() Q_DECL_OVERRIDE;

    void SetTitle(const QString &title) Q_DECL_OVERRIDE;
    void SetUrl(const QUrl &u) Q_DECL_OVERRIDE;
    void SetCreateDate(QDateTime) Q_DECL_OVERRIDE;
    void SetLastUpdateDate(QDateTime) Q_DECL_OVERRIDE;
    void SetLastAccessDate(QDateTime) Q_DECL_OVERRIDE;

    void SetCreateDateToCurrent() Q_DECL_OVERRIDE;
    void SetLastUpdateDateToCurrent() Q_DECL_OVERRIDE;
    void SetLastAccessDateToCurrent() Q_DECL_OVERRIDE;

    void SetImage(const QImage &i);
    void SetScrollX(int x);
    void SetScrollY(int y);
    void SetZoom(float z);
#ifdef MEDIATIME
    void SetMediaTime(float t);
#endif

    QString GetImageFileName();
    void SetImageFileName(const QString &path);

    void SaveImageIfNeed();

    QByteArray GetHistoryData();
    void SetHistoryData(const QByteArray &data);

    QString GetHistoryFileName();
    void SetHistoryFileName(const QString &path);

    void SaveHistoryIfNeed();
};

class LocalNode : public Node {

public:

    QUrl m_Url;
    bool m_Checked;
    bool m_DirFlag;
    static QMutex m_DiskAccessMutex;
    static QCache<QString, QImage> m_FileImageCache;

    QDateTime m_CreateDate;
    QDateTime m_LastUpdateDate;
    QDateTime m_LastAccessDate;

    LocalNode();
    virtual ~LocalNode() Q_DECL_OVERRIDE;

    bool IsRoot() Q_DECL_OVERRIDE;
    bool IsDirectory() Q_DECL_OVERRIDE;
    bool IsViewNode() const Q_DECL_OVERRIDE;
    bool TitleEditable() Q_DECL_OVERRIDE;
    void SetTitle(const QString&) Q_DECL_OVERRIDE;

    LocalNode *ToLocalNode() Q_DECL_OVERRIDE { return this;}

    QUrl GetUrl() Q_DECL_OVERRIDE;
    void SetUrl(const QUrl&) Q_DECL_OVERRIDE;

    QImage GetImage() Q_DECL_OVERRIDE;

    QDateTime GetCreateDate() Q_DECL_OVERRIDE;
    QDateTime GetLastUpdateDate() Q_DECL_OVERRIDE;
    QDateTime GetLastAccessDate() Q_DECL_OVERRIDE;
};

#endif
