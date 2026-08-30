#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QDateTime>
#include <QUrl>

#include "lightnode.hpp"
#include "treeserializer.hpp"

#include "testsupport.hpp"

namespace {

class Booting {
public:
    Booting(){ Node::SetBooting(true);}
    ~Booting(){ Node::SetBooting(false);}
};

const int BRANCHING = 8;
const int DEEP = 100;
const int WIDE_AT = 5;

const int WARMUP = 2000;
const int SMALL  = 10000;
const int LARGE  = 50000;

const int SMALL_PASSES = 3;
const int LARGE_PASSES = 2;

const double LINEAR_SLACK = 2.0;

const int SHALLOW = 8;

const double READ_OVER_WRITE = 6.0;

const QDateTime &Created(){
    static const QDateTime date =
        QDateTime::fromString(QStringLiteral("20260101120000"), NODE_DATETIME_FORMAT);
    return date;
}

class Builder {

public:
    explicit Builder(int target) : m_Target(target), m_Made(0) {}

    int Made() const { return m_Made;}

    void Build(ViewNode *root){
        const int deep = m_Target / 10;
        const int wide = m_Target / 10;
        Even(root, m_Target - deep - wide);
        Wide(root, wide);
        Deep(root, deep);
    }

private:
    ViewNode *MakeNode(ViewNode *parent){
        ViewNode *vn = parent->MakeChild();
        vn->SetTitle(QStringLiteral("node %1").arg(m_Made));
        vn->SetCreateDate(Created());
        vn->SetLastUpdateDate(Created().addSecs(60));
        vn->SetLastAccessDate(Created().addSecs(120));
        m_Made++;
        return vn;
    }

    ViewNode *MakeFolder(ViewNode *parent){
        return MakeNode(parent);
    }

    ViewNode *MakeTab(ViewNode *parent){
        ViewNode *vn = MakeNode(parent);
        vn->SetHoldView(true);
        vn->SetUrl(QUrl(QStringLiteral("https://example.com/page%1").arg(m_Made)));
        vn->SetScrollY(m_Made % 4096);
        return vn;
    }

    void Even(ViewNode *parent, int count){
        if(count <= 0) return;
        const int here = qMin(count, BRANCHING);
        const int rest = count - here;
        for(int i = 0; i < here; i++){
            const int share = rest / here + (i < rest % here ? 1 : 0);
            if(share > 0) Even(MakeFolder(parent), share);
            else          MakeTab(parent);
        }
    }

    void Wide(ViewNode *root, int count){
        if(count <= 0) return;
        ViewNode *at = root;
        for(int i = 0; i < WIDE_AT; i++){
            if(!at->ChildrenLength()) break;
            ViewNode *next = at->GetChildAt(0)->ToViewNode();
            if(!next || next->HoldsView()) break;
            at = next;
        }
        ViewNode *folder = MakeFolder(at);
        for(int i = 1; i < count; i++) MakeTab(folder);
    }

    void Deep(ViewNode *root, int count){
        while(count > 0){
            const int here = qMin(count, DEEP + 1);
            ViewNode *at = root;
            for(int i = 1; i < here; i++) at = MakeFolder(at);
            MakeTab(at);
            count -= here;
        }
    }

    int m_Target;
    int m_Made;
};

int CountNodes(Node *nd){
    int count = 0;
    foreach(Node *child, nd->GetChildren()) count += 1 + CountNodes(child);
    return count;
}

int MaxDepth(Node *nd){
    int deepest = 0;
    foreach(Node *child, nd->GetChildren()) deepest = qMax(deepest, 1 + MaxDepth(child));
    return deepest;
}

int WidestFolder(Node *nd){
    int widest = nd->ChildrenLength();
    foreach(Node *child, nd->GetChildren()) widest = qMax(widest, WidestFolder(child));
    return widest;
}

int NodesWithin(Node *nd, int depth){
    if(depth <= 0) return 0;
    int count = 0;
    foreach(Node *child, nd->GetChildren()) count += 1 + NodesWithin(child, depth - 1);
    return count;
}

struct Sample {
    int    target = 0;
    int    nodes  = 0;
    int    reread = 0;
    qint64 bytes  = 0;
    qint64 buildNs = 0;
    qint64 writeNs = 0;
    qint64 readNs  = 0;
    qint64 freeNs  = 0;
};

double PerNode(qint64 ns, int nodes){
    return nodes > 0 ? double(ns) / double(nodes) : 0.0;
}

QString Row(const Sample &s){
    return QStringLiteral("%1 nodes | %2 MB | build %3 ms (%4 ns/node) | "
                          "write %5 ms (%6 ns/node) | read %7 ms (%8 ns/node) | "
                          "free %9 ms (%10 ns/node)")
        .arg(s.nodes)
        .arg(double(s.bytes) / (1024.0 * 1024.0), 0, 'f', 1)
        .arg(s.buildNs / 1000000).arg(PerNode(s.buildNs, s.nodes), 0, 'f', 0)
        .arg(s.writeNs / 1000000).arg(PerNode(s.writeNs, s.nodes), 0, 'f', 0)
        .arg(s.readNs  / 1000000).arg(PerNode(s.readNs,  s.nodes), 0, 'f', 0)
        .arg(s.freeNs  / 1000000).arg(PerNode(s.freeNs,  s.nodes), 0, 'f', 0);
}

}

class tst_treescale : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_Dir;

    QString Path(const QString &name) const {
        return m_Dir.path() + QStringLiteral("/") + name;
    }

    Sample Measure(int target);
    Sample Best(int target, int passes);

    static double Growth(qint64 largeNs, int largeNodes, qint64 smallNs, int smallNodes){
        const double small = PerNode(smallNs, smallNodes);
        if(small <= 0.0) return 1.0;
        return PerNode(largeNs, largeNodes) / small;
    }

private slots:
    void initTestCase();

    void theTreeHasTheShapeTheMeasurementClaims();
    void everyCostStaysProportionalToTheNodeCount();
};


void tst_treescale::initTestCase(){
    TestSupport::SilenceDebugOutput();
    QVERIFY2(m_Dir.isValid(), "could not create a temporary directory.");
}

Sample tst_treescale::Measure(int target){
    Sample sample;
    sample.target = target;

    QElapsedTimer timer;

    ViewNode *root = new ViewNode();
    {
        Booting booting;
        Builder builder(target);
        timer.start();
        builder.Build(root);
        sample.buildNs = timer.nsecsElapsed();
        sample.nodes = builder.Made();
    }

    const QString path = Path(QStringLiteral("scale_%1.json").arg(target));

    timer.start();
    const bool written = TreeSerializer::WriteJsonFile(path, root);
    sample.writeNs = timer.nsecsElapsed();
    sample.bytes = written ? QFileInfo(path).size() : -1;

    ViewNode *reread = new ViewNode();
    {
        Booting booting;
        timer.start();
        const bool read = TreeSerializer::ReadJsonFile(path, reread);
        sample.readNs = timer.nsecsElapsed();
        if(!read) sample.readNs = -1;
    }
    sample.reread = CountNodes(reread);

    timer.start();
    delete reread;
    delete root;
    sample.freeNs = timer.nsecsElapsed();

    QFile::remove(path);
    return sample;
}

Sample tst_treescale::Best(int target, int passes){
    Sample best = Measure(target);
    for(int i = 1; i < passes; i++){
        const Sample again = Measure(target);
        if(again.bytes <= 0 || again.readNs < 0 || again.reread != again.nodes) continue;
        best.buildNs = qMin(best.buildNs, again.buildNs);
        best.writeNs = qMin(best.writeNs, again.writeNs);
        best.readNs  = qMin(best.readNs,  again.readNs);
        best.freeNs  = qMin(best.freeNs,  again.freeNs);
    }
    return best;
}

void tst_treescale::theTreeHasTheShapeTheMeasurementClaims(){
    ViewNode *root = new ViewNode();
    Builder builder(SMALL);
    {
        Booting booting;
        builder.Build(root);
    }

    QCOMPARE(builder.Made(), SMALL);
    QCOMPARE(CountNodes(root), SMALL);

    QVERIFY2(MaxDepth(root) >= DEEP,
             qPrintable(QStringLiteral("the deepest node sits at %1, not %2.")
                        .arg(MaxDepth(root)).arg(DEEP)));

    QCOMPARE(WidestFolder(root), SMALL / 10 - 1);

    const int shallow = NodesWithin(root, SHALLOW);
    QVERIFY2(shallow >= SMALL * 3 / 4,
             qPrintable(QStringLiteral("only %1 of %2 nodes sit within %3 levels.")
                        .arg(shallow).arg(SMALL).arg(SHALLOW)));

    delete root;
}

void tst_treescale::everyCostStaysProportionalToTheNodeCount(){
    Measure(WARMUP);

    const Sample small = Best(SMALL, SMALL_PASSES);
    const Sample large = Best(LARGE, LARGE_PASSES);

    qInfo().noquote() << "\n" << Row(small) << "\n" << Row(large);

    QVERIFY2(small.bytes > 0 && large.bytes > 0, "the tree could not be written.");
    QVERIFY2(small.readNs >= 0 && large.readNs >= 0, "the tree could not be read back.");
    QCOMPARE(small.reread, small.nodes);
    QCOMPARE(large.reread, large.nodes);

    const double smallBytes = double(small.bytes) / small.nodes;
    const double largeBytes = double(large.bytes) / large.nodes;
    QVERIFY2(largeBytes <= smallBytes * LINEAR_SLACK,
             qPrintable(QStringLiteral("bytes per node went from %1 to %2.")
                        .arg(smallBytes, 0, 'f', 0).arg(largeBytes, 0, 'f', 0)));

    struct Phase {
        const char *name;
        double growth;
    };
    const Phase phases[] = {
        {"build", Growth(large.buildNs, large.nodes, small.buildNs, small.nodes)},
        {"write", Growth(large.writeNs, large.nodes, small.writeNs, small.nodes)},
        {"read",  Growth(large.readNs,  large.nodes, small.readNs,  small.nodes)},
        {"free",  Growth(large.freeNs,  large.nodes, small.freeNs,  small.nodes)},
    };

    const double readOverWrite = PerNode(large.readNs, large.nodes)
                               / PerNode(large.writeNs, large.nodes);
    QVERIFY2(readOverWrite <= READ_OVER_WRITE,
             qPrintable(QStringLiteral("reading a node costs %1 times writing one; "
                                       "%2 is the most it may cost.")
                        .arg(readOverWrite, 0, 'f', 2)
                        .arg(READ_OVER_WRITE, 0, 'f', 2)));

    for(const Phase &phase : phases){
        QVERIFY2(phase.growth <= LINEAR_SLACK,
                 qPrintable(QStringLiteral("the cost of one node in '%1' grew %2 times "
                                           "between %3 and %4 nodes; %5 is the most a "
                                           "cost proportional to the node count can grow.")
                            .arg(QLatin1String(phase.name))
                            .arg(phase.growth, 0, 'f', 2)
                            .arg(small.nodes).arg(large.nodes)
                            .arg(LINEAR_SLACK, 0, 'f', 2)));
    }
}

QTEST_MAIN(tst_treescale)
#include "tst_treescale.moc"
