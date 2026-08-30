#ifndef TREESERIALIZER_HPP
#define TREESERIALIZER_HPP

#include "switch.hpp"

#include <QString>

#include <functional>

class ViewNode;
class QDomElement;
class QTextStream;

namespace TreeSerializer {

class Hooks {

public:
    std::function<int(ViewNode*)> WindowIndexOf;
    std::function<void(ViewNode*, int)> RestoreIntoWindow;
    std::function<bool(ViewNode*)> KeepsSideFiles;
    std::function<void()> Tick;

    int  WindowIndex(ViewNode *nd) const { return WindowIndexOf ? WindowIndexOf(nd) : 0;}
    void Restore(ViewNode *nd, int id) const { if(RestoreIntoWindow) RestoreIntoWindow(nd, id);}
    bool KeepSideFiles(ViewNode *nd) const { return KeepsSideFiles ? KeepsSideFiles(nd) : true;}
    void Tock() const { if(Tick) Tick();}
};

bool ReadJsonFile(const QString &path, ViewNode *root, const Hooks &hooks = Hooks());

bool ReadLegacyXmlFile(const QString &path, ViewNode *root, const Hooks &hooks = Hooks());

bool WriteJsonFile(const QString &path, ViewNode *root, const Hooks &hooks = Hooks());

void ReadLegacyNode(const QDomElement &elem, ViewNode *parent, const Hooks &hooks = Hooks());
void WriteNode(ViewNode *nd, QTextStream &out, int depth, const Hooks &hooks = Hooks());
QString JsonEscape(QString str);

int ReadNodeCount();
int ReadViewCount();
void ResetReadCounters();

}

#endif
