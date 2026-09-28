#include "Messages.h"

#include <QLocale>
#include <QStringList>

namespace {
// «1 nodo», «3 nodos».
QString count(int n, const char *singular, const char *plural) {
    return QStringLiteral("%1 %2").arg(n).arg(QString::fromUtf8(n == 1 ? singular : plural));
}
}

namespace Msg {

// ---------- Formato ----------
QString number(double value) {
    static const QLocale loc = [] {
        QLocale l(QLocale::Spanish, QLocale::Spain);
        l.setNumberOptions(QLocale::OmitGroupSeparator);
        return l;
    }();
    if (value == 0.0) value = 0.0; // evita «-0»
    return loc.toString(value, 'f', QLocale::FloatingPointShortest);
}

QString path(const Graph &g, const QVector<int> &nodes) {
    QStringList names;
    for (int i : nodes) names << g.node(i).name;
    return names.join(QStringLiteral(" → "));
}

QString edge(const Graph &g, const Graph::Edge &e) {
    const QString sep = g.isDirected() ? QStringLiteral("→") : QStringLiteral("–");
    return g.node(e.from).name + sep + g.node(e.to).name;
}

QString edgeSample(const Graph &g, const QVector<Graph::Edge> &edges, int max) {
    QStringList out;
    for (int i = 0; i < edges.size() && i < max; ++i) out << edge(g, edges[i]);
    QString s = out.join(QStringLiteral(", "));
    if (edges.size() > max) s += QStringLiteral(", …");
    return s;
}

// ---------- Campo «Peso» ----------
QString weightEmpty() {
    return QStringLiteral("Escriba un peso, por ejemplo 2,5 o -3.");
}

QString weightNotNumber(const QString &text) {
    return QStringLiteral("«%1» no es un número. Escriba, por ejemplo, 2,5 o -3.").arg(text);
}

QString weightNotFinite() {
    return QStringLiteral("El peso debe ser un número finito.");
}

QString weightOutOfRange(double limit) {
    return QStringLiteral("El peso debe estar entre %1 y %2.")
            .arg(number(-limit), number(limit));
}

QString weightNegativeUndirected(const QString &from, const QString &to, double weight) {
    return QStringLiteral("En un grafo no dirigido no se permiten pesos negativos: la arista %1–%2 "
                          "con peso %3 formaría el ciclo %1→%2→%1 y ningún camino mínimo estaría "
                          "definido. Active «Grafo dirigido» para usar pesos negativos.")
            .arg(from, to, number(weight));
}

QString weightNegativeDijkstra() {
    return QStringLiteral("Dijkstra no admite pesos negativos. Seleccione Bellman-Ford o "
                          "Floyd-Warshall para agregar esta arista.");
}

// ---------- Campo «Nombre» del nodo ----------
QString nodeNameInvalid() {
    return QStringLiteral("El nombre debe ser una sola letra de la A a la Z, sin tildes ni ñ.");
}

QString nodeNameDuplicate(const QString &name, const QString &alternative) {
    if (alternative.isEmpty())
        return QStringLiteral("Ya existe un nodo «%1». Escriba otra letra.").arg(name);
    return QStringLiteral("Ya existe un nodo «%1». Los nombres distinguen mayúsculas: "
                          "puede usar «%2».").arg(name, alternative);
}

QString nodeLimit(int max) {
    return QStringLiteral("Se alcanzó el máximo de %1 nodos. Borre alguno para agregar otro.").arg(max);
}

QString nodeNoFreeName(bool uppercase) {
    return uppercase
            ? QStringLiteral("Ya se usaron todas las letras mayúsculas. Desmarque «Mayúsculas» "
                             "o escriba un nombre.")
            : QStringLiteral("Ya se usaron todas las letras minúsculas. Marque «Mayúsculas» "
                             "o escriba un nombre.");
}

QString nodeOverlap() {
    return QStringLiteral("Ya hay un nodo en ese lugar. Haga clic en un espacio libre del lienzo.");
}

// ---------- Aristas ----------
QString edgeMissing(const QString &from, const QString &to) {
    return QStringLiteral("No existe una arista de %1 a %2.").arg(from, to);
}

QString addEdgeText(bool exists) {
    return exists ? QStringLiteral("Cambiar peso") : QStringLiteral("Agregar arista");
}

QString edgeExists(const QString &edge, double weight) {
    return QStringLiteral("La arista %1 ya existe con peso %2. «Cambiar peso» la reemplaza.")
            .arg(edge, number(weight));
}

QString edgeSameWeight(const QString &edge, double weight) {
    return QStringLiteral("La arista %1 ya tiene peso %2. Escriba otro peso para cambiarlo.")
            .arg(edge, number(weight));
}

Message edgeUpdated(const QString &edge, double oldWeight, double newWeight) {
    return {QStringLiteral("Peso de %1 cambiado: %2 → %3").arg(edge, number(oldWeight), number(newWeight)),
            QStringLiteral("Pulse «Calcular» para obtener el camino con el nuevo peso.")};
}

// ---------- Menú del nodo ----------
QString removeNodeAction(const QString &name) {
    return QStringLiteral("Eliminar nodo «%1»").arg(name);
}

// ---------- Botones desactivados ----------
QString needsNodesForEdge() {
    return QStringLiteral("Agregue al menos un nodo para crear aristas.");
}

QString needsNodesForCalculate() {
    return QStringLiteral("Agregue al menos un nodo para calcular un camino.");
}

// ---------- Aviso del grupo «Camino más corto» ----------
QString dijkstraWithNegatives(const Graph &g, const QVector<Graph::Edge> &negatives) {
    const QString count = negatives.size() == 1
            ? QStringLiteral("una arista negativa")
            : QStringLiteral("%1 aristas negativas").arg(negatives.size());
    return QStringLiteral("El grafo tiene %1 (%2). Dijkstra no puede usarse con pesos negativos: "
                          "elija Bellman-Ford o Floyd-Warshall.")
            .arg(count, edgeSample(g, negatives));
}

// ---------- Banner de resultado ----------
Message initialHint() {
    return {QStringLiteral("Sin resultados todavía"),
            QStringLiteral("Agregue nodos y aristas, elija origen y destino, y pulse «Calcular».")};
}

Message graphChanged() {
    return {QStringLiteral("El grafo cambió"),
            QStringLiteral("Pulse «Calcular» para obtener el camino con los datos actuales.")};
}

Message pathFound(const Graph &g, const QVector<int> &nodes, double distance, const QString &algorithm) {
    const QString from = g.node(nodes.first()).name;
    const QString to = g.node(nodes.last()).name;
    Message m;
    m.title = QStringLiteral("Distancia de %1 a %2: %3").arg(from, to, number(distance));
    if (nodes.size() == 1)
        m.text = QStringLiteral("El origen y el destino son el mismo nodo. · %1").arg(algorithm);
    else
        m.text = QStringLiteral("Camino: %1 · %2").arg(path(g, nodes), algorithm);
    return m;
}

Message noPath(const Graph &g, int src, int dest) {
    const QString from = g.node(src).name;
    const QString to = g.node(dest).name;
    return {QStringLiteral("No hay camino de %1 a %2").arg(from, to),
            g.isDirected()
                ? QStringLiteral("%2 no se puede alcanzar desde %1 siguiendo el sentido de las aristas.").arg(from, to)
                : QStringLiteral("%2 no se puede alcanzar desde %1: no hay aristas que los conecten.").arg(from, to)};
}

Message negativeCycle(const Graph &g, int src, int dest, const QVector<int> &cycle, double weight) {
    Message m;
    m.title = QStringLiteral("No hay camino mínimo de %1 a %2")
            .arg(g.node(src).name, g.node(dest).name);
    if (cycle.isEmpty()) {
        m.text = QStringLiteral("Hay un ciclo de peso negativo que afecta al camino, pero no se pudo "
                                "identificar qué nodos lo forman. Revise las aristas con peso negativo.");
    } else {
        m.text = QStringLiteral("El ciclo %1 (en rojo) tiene peso %2: se puede recorrer indefinidamente "
                                "y reducir la distancia sin límite. Cambie algún peso del ciclo para que "
                                "su suma no sea negativa.")
                .arg(path(g, cycle), number(weight));
    }
    return m;
}

// ---------- Deshacer ----------
QString undoAddNode(const QString &name)    { return QStringLiteral("Agregar nodo «%1»").arg(name); }
QString undoRemoveNode(const QString &name) { return QStringLiteral("Eliminar nodo «%1»").arg(name); }
QString undoMoveNode(const QString &name)   { return QStringLiteral("Mover nodo «%1»").arg(name); }
QString undoClearAll()                      { return QStringLiteral("Borrar todo"); }
QString undoAddEdge(const QString &edge)    { return QStringLiteral("Agregar arista %1").arg(edge); }
QString undoChangeWeight(const QString &edge) { return QStringLiteral("Cambiar peso de %1").arg(edge); }
QString undoRemoveEdge(const QString &edge) { return QStringLiteral("Quitar arista %1").arg(edge); }
QString undoSetDirected(bool directed) {
    return directed ? QStringLiteral("Cambiar a grafo dirigido") : QStringLiteral("Cambiar a grafo no dirigido");
}

QString undoTooltip(const QString &action) {
    return QStringLiteral("Deshacer «%1» (Ctrl+Z).").arg(action);
}

QString nothingToUndo() {
    return QStringLiteral("No hay cambios que deshacer.");
}

Message undone(const QString &action, const QString &nextAction) {
    return {QStringLiteral("Se deshizo «%1»").arg(action),
            nextAction.isEmpty()
                ? QStringLiteral("No quedan más cambios que deshacer.")
                : QStringLiteral("Pulse «Deshacer» otra vez (Ctrl+Z) para deshacer «%1».").arg(nextAction)};
}

// ---------- Confirmaciones ----------
Message confirmToUndirected(const Graph &g, const QVector<Graph::Edge> &asymmetric,
                            const QVector<Graph::Edge> &negatives) {
    QStringList reasons;
    if (asymmetric.size() == 1) {
        reasons << QStringLiteral("1 arista que solo va en un sentido o tiene otro peso a la vuelta (%1)")
                           .arg(edgeSample(g, asymmetric));
    } else if (asymmetric.size() > 1) {
        reasons << QStringLiteral("%1 aristas que solo van en un sentido o tienen otro peso a la vuelta (%2)")
                           .arg(QString::number(asymmetric.size()), edgeSample(g, asymmetric));
    }
    if (negatives.size() == 1) {
        reasons << QStringLiteral("1 arista con peso negativo (%1)").arg(edgeSample(g, negatives));
    } else if (negatives.size() > 1) {
        reasons << QStringLiteral("%1 aristas con peso negativo (%2)")
                           .arg(QString::number(negatives.size()), edgeSample(g, negatives));
    }
    return {QStringLiteral("¿Cambiar a grafo no dirigido?"),
            QStringLiteral("El grafo tiene %1. Un grafo no dirigido no puede representarlas, "
                           "así que para cambiar de modo hay que borrar todas las aristas. "
                           "Los nodos se conservan, y puede recuperar las aristas con «Deshacer» (Ctrl+Z).")
                    .arg(reasons.join(QStringLiteral(" y ")))};
}

QString confirmToUndirectedAccept() { return QStringLiteral("Borrar aristas y cambiar"); }

Message confirmClearAll(int nodes, int edges) {
    // Con varios elementos el verbo va en plural; las aristas se omiten si no hay.
    QString what = count(nodes, "nodo", "nodos");
    if (edges > 0) what += QStringLiteral(" y ") + count(edges, "arista", "aristas");
    const QString verb = nodes == 1 && edges == 0 ? QStringLiteral("Se eliminará")
                                                  : QStringLiteral("Se eliminarán");
    return {QStringLiteral("¿Borrar todo el grafo?"),
            QStringLiteral("%1 %2. Puede recuperarlo todo con «Deshacer» (Ctrl+Z).").arg(verb, what)};
}

QString confirmClearAllAccept() { return QStringLiteral("Borrar todo"); }

Message confirmRemoveNode(const QString &name, int edges) {
    return {QStringLiteral("¿Eliminar el nodo «%1»?").arg(name),
            edges == 1 ? QStringLiteral("También se eliminará su arista.")
                       : QStringLiteral("También se eliminarán sus %1 aristas.").arg(edges)};
}

QString confirmRemoveNodeAccept() { return QStringLiteral("Eliminar nodo"); }

} // namespace Msg
