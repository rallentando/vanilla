#ifndef NODE_HPP
#define NODE_HPP

#include "switch.hpp"
#include "const.hpp"

#ifndef USE_LIGHTNODE

#include <QList>
#include <QUrl>
#include <QImage>

#include <QCache>
#include <QMutex>
#include <QFileInfo>

#include <atomic>

class QString;
class QUrl;
class QImage;
class QDateTime;
class View;

class Node;
class ViewNode;
class HistNode;
class LocalNode;

typedef QList<Node*>      NodeList;
typedef QList<ViewNode*>  ViewNodeList;
typedef QList<HistNode*>  HistNodeList;
typedef QList<LocalNode*> LocalNodeList;

/*

 ;;;;;;;;;;;;;;;;;;;; | ;;;;;;;;;;;;;;;;;;;;
 ;; ViewNode World ;; | ;; HistNode World ;;
 ;;;;;;;;;;;;;;;;;;;; | ;;;;;;;;;;;;;;;;;;;;
                      |
  Root                |     DummyRoot
   +-Folder           |      ...+-Root
   |  +-View .........|.......  |  +-Node
   |  +-Folder        |         |     +-Node
   |     +-View ......|.........+-Root
   |                  |         |  +-Node
   +-Folder           |         |  |  +-Node
   |  +-Folder        |         |  |  +-Node
   |  |  +-View ......|.......  |  +-Node
   |  |  +-View ......|....  .  |     +-Node
   |  +-Folder        |   .  ...+-Root
   |     +-View       |   .     |  +-Node
   .                  |   .     |     +-Node
   .                  |   ......+-Root
   .                  |         |  +-Node
                      |         .     +-Node
                      |         .
                      |         .

  relation of ViewNode <-> HistNode.
  ViewNode(not folder) <-> HistNode(Root or it's descendant).

  'Next' and 'Prev' method of the 'ViewNode' object is like that of DepthFirstSearch.
  'Next' method of 'HistNode' returns its primary child.
  'Prev' method of 'HistNode' returns its parent.
 */

// 'QObject' for deletion of 'Node' on 'Gadgets'.
class Node : public QObject{
    Q_OBJECT

public:
    enum NodeType {
        ViewTypeNode, HistTypeNode,
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

    static bool m_EnableDeepCopyOfNode;
    static AddNodePosition m_AddChildViewNodePosition;
    static AddNodePosition m_AddSiblingViewNodePosition;

    View *m_View;

    std::atomic_bool m_Folded;
    QString m_Title;

    Node *m_Parent;
    Node *m_Primary;
    Node *m_Partner;
    NodeList m_Children;
    static bool m_Booting;
    static QStringList m_AllImageFileName;
    static QStringList m_AllHistoryFileName;

public:
    Node();
    virtual ~Node();

    void Delete(){ deleteLater();}

    static void SetBooting(bool b);

    static void LoadSettings();
    static void SaveSettings();

    bool IsRead();
    QString ReadableTitle();

    virtual bool IsRoot(){ return false;}
    virtual bool IsDirectory(){ return false;}
    virtual bool IsHistNode() const { return false;}
    virtual bool IsViewNode() const { return false;}
    virtual bool IsDummy()    const { return false;}
    virtual bool TitleEditable(){ return false;}
    virtual Node *MakeChild() { return 0;}
    virtual Node *MakeParent(){ return 0;}
    virtual Node *Next()      { return 0;}
    virtual Node *Prev()      { return 0;}
    virtual HistNode  *ToHistNode(){ return 0;}
    virtual ViewNode  *ToViewNode(){ return 0;}
    virtual LocalNode *ToLocalNode(){ return 0;}

    NodeType GetType() const { return m_Type;}
    View *GetView()    const { return m_View;}
    QString GetTitle() const { return m_Title;}

    bool GetFolded()   const { return m_Folded;}
    Node *GetParent()  const { return m_Parent;}
    Node *GetPrimary() const { return m_Primary;}
    Node *GetPartner() const { return m_Partner;}
    // return copy.
    NodeList GetChildren() const { return m_Children;}
    NodeList GetSiblings() const {
        static NodeList empty = NodeList();
        if(!m_Parent) return empty;
        return m_Parent->GetChildren();
    }
    // make list.
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
        // this is slow function.
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
    bool IsPartnerOf(Node *nd) const {
        return nd ? nd->GetPartner() == this : false;
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
    bool IsPartnerOfPartner() const {
        return IsPartnerOf(m_Partner);
    }

    void SetView(View *v)   { m_View = v;}
    virtual void SetTitle(const QString &s){ m_Title = s;}

    void SetFolded(bool b)   { m_Folded = b;}
    void SetParent(Node *nd) { m_Parent  = nd;}
    void SetPrimary(Node *nd){ m_Primary = nd;}
    void SetPartner(Node *nd){ m_Partner = nd;}

    void ResetPrimaryPath(){
        Node *nd = this;
        while(nd->m_Parent){
            nd->m_Parent->m_Primary = nd;
            nd = nd->m_Parent;
        }
    }
    void SetChildren(NodeList c){
        m_Children = c;
    }
    void ClearChildren(){
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
        m_Children.append(nd);
    }
    void PrependChild(Node *nd){
        m_Children.prepend(nd);
    }
    void RemoveChild(Node *nd){
        m_Children.removeOne(nd);
    }
    void InsertChild(int i, Node *nd){
        m_Children.insert(i, nd);
    }
    void MoveChild(int from, int to){
        m_Children.move(from, to);
    }
    Node *GetChildAt(int i) const {
        return m_Children.at(i);
    }
    Node *GetFirstChild() const {
        if(m_Children.isEmpty()) return 0;
        return m_Children.first();
    }
    Node *GetLastChild() const {
        if(m_Children.isEmpty()) return 0;
        return m_Children.last();
    }
    Node *TakeFirstChild(){
        if(m_Children.isEmpty()) return 0;
        return m_Children.takeFirst();
    }
    Node *TakeLastChild(){
        if(m_Children.isEmpty()) return 0;
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
        if(!m_Parent) return 0;
        return m_Parent->GetChildAt(i);
    }
    Node *GetFirstSibling() const {
        if(!m_Parent) return 0;
        return m_Parent->GetFirstChild();
    }
    Node *GetLastSibling() const {
        if(!m_Parent) return 0;
        return m_Parent->GetLastChild();
    }
    Node *TakeFirstSibling(){
        if(!m_Parent) return 0;
        return m_Parent->TakeFirstChild();
    }
    Node *TakeLastSibling(){
        if(!m_Parent) return 0;
        return m_Parent->TakeLastChild();
    }

    virtual QUrl GetUrl()    { return QUrl();}
    virtual QImage GetImage(){ return QImage();}
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

    void SetCreateDateToCurrent();
    void SetLastUpdateDateToCurrent();
    void SetLastAccessDateToCurrent();
};

/*
  DummyRoot
   +-Root
   |  +-Node
   |  +-Node
   |     +-Node
   +-Root
   |  +-Node
   |  |  +-Node
   |  |  +-Node
   |  +-Node
   |     +-Node
   .
   .
   .

   Parent <-> Child is meaning Back <-> Forward in history.
 */
class HistNode : public Node{
    Q_OBJECT

private:
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
    QDateTime m_CreateDate;
    QDateTime m_LastUpdateDate;
    QDateTime m_LastAccessDate;

public:
    HistNode();
    virtual ~HistNode();

    bool IsRoot() DECL_OVERRIDE;
    bool IsDirectory() DECL_OVERRIDE;
    bool IsHistNode() const DECL_OVERRIDE;
    bool IsViewNode() const DECL_OVERRIDE;
    bool TitleEditable() DECL_OVERRIDE;

    HistNode *MakeChild() DECL_OVERRIDE;
    HistNode *MakeParent() DECL_OVERRIDE;
    HistNode *Next() DECL_OVERRIDE;
    HistNode *Prev() DECL_OVERRIDE;
    HistNode *ToHistNode() DECL_OVERRIDE { return this;}
    HistNode *Clone(HistNode *parent = 0, ViewNode *partner = 0);
    HistNode *New();

    QUrl GetUrl() DECL_OVERRIDE;
    QImage GetImage() DECL_OVERRIDE;
    int GetScrollX() DECL_OVERRIDE;
    int GetScrollY() DECL_OVERRIDE;
    float GetZoom() DECL_OVERRIDE;
    QDateTime GetCreateDate() DECL_OVERRIDE;
    QDateTime GetLastUpdateDate() DECL_OVERRIDE;
    QDateTime GetLastAccessDate() DECL_OVERRIDE;

    void SetUrl(const QUrl &u) DECL_OVERRIDE;
    void SetCreateDate(QDateTime) DECL_OVERRIDE;
    void SetLastUpdateDate(QDateTime) DECL_OVERRIDE;
    void SetLastAccessDate(QDateTime) DECL_OVERRIDE;

    void SetImage(const QImage &i);
    void SetScrollX(int x);
    void SetScrollY(int y);
    void SetZoom(float z);

    QString GetImageFileName();
    void SetImageFileName(const QString &path);

    void SaveImageIfNeed();

    QByteArray GetHistoryData();
    void SetHistoryData(const QByteArray &data);

    QString GetHistoryFileName();
    void SetHistoryFileName(const QString &path);

    void SaveHistoryIfNeed();

    friend class ViewNode;
};

/*
  Root
   +-Node
   |  +-Node
   |  +-Node
   |     +-Node
   +-Node
   |  +-Node
   |  |  +-Node
   |  |  +-Node
   |  +-Node
   |     +-Node
   .
   .
   .

   Parent <-> Child is meaning Folder <-> SubFolder or File.
 */
class ViewNode : public Node{
    Q_OBJECT

private:
    QDateTime m_CreateDate;
    QDateTime m_LastUpdateDate;
    QDateTime m_LastAccessDate;

public:
    ViewNode();
    virtual ~ViewNode();

    bool IsRoot() DECL_OVERRIDE;
    bool IsDirectory() DECL_OVERRIDE;
    bool IsHistNode() const DECL_OVERRIDE;
    bool IsViewNode() const DECL_OVERRIDE;
    bool TitleEditable() DECL_OVERRIDE;

    ViewNode *MakeChild(bool);
    ViewNode *MakeChild() DECL_OVERRIDE;
    ViewNode *MakeParent() DECL_OVERRIDE;
    ViewNode *MakeSibling();
    ViewNode *NewDir();
    ViewNode *Next() DECL_OVERRIDE;
    ViewNode *Prev() DECL_OVERRIDE;
    ViewNode *ToViewNode() DECL_OVERRIDE { return this;}
    ViewNode *Clone(ViewNode *parent = 0);
    ViewNode *New();

    QUrl GetUrl() DECL_OVERRIDE;
    QImage GetImage() DECL_OVERRIDE;
    int GetScrollX() DECL_OVERRIDE;
    int GetScrollY() DECL_OVERRIDE;
    float GetZoom() DECL_OVERRIDE;
    QDateTime GetCreateDate() DECL_OVERRIDE;
    QDateTime GetLastUpdateDate() DECL_OVERRIDE;
    QDateTime GetLastAccessDate() DECL_OVERRIDE;

    void SetTitle(const QString &title) DECL_OVERRIDE;
    void SetUrl(const QUrl &u) DECL_OVERRIDE;
    void SetCreateDate(QDateTime) DECL_OVERRIDE;
    void SetLastUpdateDate(QDateTime) DECL_OVERRIDE;
    void SetLastAccessDate(QDateTime) DECL_OVERRIDE;

    friend class HistNode;
};

class LocalNode : public Node{
    Q_OBJECT

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
    virtual ~LocalNode();

    bool IsRoot() DECL_OVERRIDE;
    bool IsDirectory() DECL_OVERRIDE;
    bool IsHistNode() const DECL_OVERRIDE;
    bool IsViewNode() const DECL_OVERRIDE;
    bool TitleEditable() DECL_OVERRIDE;
    void SetTitle(const QString&) DECL_OVERRIDE;

    LocalNode *ToLocalNode() DECL_OVERRIDE { return this;}

    QUrl GetUrl() DECL_OVERRIDE;
    void SetUrl(const QUrl&) DECL_OVERRIDE;

    // no instance specific object.
    QImage GetImage() DECL_OVERRIDE;

    QDateTime GetCreateDate() DECL_OVERRIDE;
    QDateTime GetLastUpdateDate() DECL_OVERRIDE;
    QDateTime GetLastAccessDate() DECL_OVERRIDE;
};

#endif // ifndef USE_LIGHTNODE
#endif
