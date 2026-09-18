#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QFont>
#include <QString>

// Design tokens named by purpose, following Apple HIG.
// Values resolve from the system light/dark appearance — never pick a token
// for the color it happens to produce.
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

QColor nodeFill();
QColor nodeStroke();
QColor nodeLabel();
QColor edge();
QColor edgeHighlight();
QColor pillFill();
QColor pillStroke();
QColor pillLabel();
QColor emptyStateLabel();
QColor emptyStateCaption();

// Desktop follows macOS control sizing (28pt default, 20pt minimum).
// The primary action is 36px so it reads as the visual anchor on desktop.
// That is not the 44pt iOS touch minimum — this is a pointer-first layout.
constexpr int PrimaryControlHeight = 36;
constexpr int ControlHeight        = 28;
constexpr int MinControlSize       = 28;
constexpr int SpacingTight         = 8;
constexpr int SpacingBezeled       = 12;
constexpr int SpacingUnbezeled     = 24;
constexpr int CornerRadius         = 10;
constexpr int NodeRadius           = 22;

QFont title();
QFont headline();
QFont body();
QFont caption();

QString styleSheet();

} // namespace Theme

#endif // THEME_H
