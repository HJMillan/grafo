#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QFont>
#include <QString>

// Tokens de diseño nombrados por su función (Apple HIG). Los valores se
// resuelven según la apariencia clara u oscura del sistema: nunca se elige
// un token por el color que produce.
namespace Theme {

bool isDark();

QColor systemBackground();
QColor secondarySystemBackground();
QColor tertiarySystemBackground();
QColor canvasBackground();

QColor label();
QColor secondaryLabel();
QColor separator();

QColor accent();
QColor destructive();
QColor success();
QColor warning();

// Lienzo (siempre oscuro, en ambas apariencias)
QColor nodeFill();
QColor nodeStroke();
QColor nodeLabel();
QColor edge();
QColor pathHighlight();
QColor cycleHighlight();
QColor pillFill();
QColor pillStroke();
QColor pillLabel();
QColor emptyStateLabel();
QColor emptyStateCaption();

// Controles con tamaño de escritorio (28 px; la acción principal, 36 px).
constexpr int PrimaryControlHeight = 36;
constexpr int ControlHeight        = 28;
constexpr int MinControlSize       = 28;
constexpr int SpacingTight         = 8;
constexpr int SpacingBezeled       = 12;
constexpr int SpacingUnbezeled     = 24;
constexpr int CornerRadius         = 10;
constexpr int CanvasRadius         = 16;
constexpr int NodeRadius           = 22;

QFont title();
QFont headline();
QFont body();
QFont caption();

QString styleSheet();

} // namespace Theme

#endif // THEME_H
