#include "Theme.h"

#include <QFontDatabase>
#include <QGuiApplication>
#include <QList>
#include <QPair>
#include <QPalette>
#include <algorithm>

namespace {

QString css(const QColor &c) {
    return QString("rgba(%1, %2, %3, %4)")
            .arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}

QColor withAlpha(QColor c, int alpha) {
    c.setAlpha(alpha);
    return c;
}

QFont baseFont() {
    const QStringList preferred = {
        QStringLiteral("SF Pro Text"),
        QStringLiteral("SF Pro Display"),
        QStringLiteral("Segoe UI Variable"),
        QStringLiteral("Segoe UI"),
    };
    const QStringList available = QFontDatabase::families();
    for (const QString &name : preferred) {
        for (const QString &fam : available) {
            if (fam.compare(name, Qt::CaseInsensitive) == 0) {
                QFont f(fam);
                f.setStyleHint(QFont::SansSerif);
                f.setHintingPreference(QFont::PreferFullHinting);
                return f;
            }
        }
    }
    return QGuiApplication::font();
}

} // namespace

namespace Theme {

bool isDark() {
    return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
}

QColor systemBackground()          { return isDark() ? QColor("#000000") : QColor("#F2F2F7"); }
QColor secondarySystemBackground() { return isDark() ? QColor("#1C1C1E") : QColor("#FFFFFF"); }
QColor tertiarySystemBackground()  { return isDark() ? QColor("#2C2C2E") : QColor("#E5E5EA"); }

QColor canvasBackground() { return QColor("#0B0F14"); }

QColor label()          { return isDark() ? QColor("#FFFFFF") : QColor("#000000"); }
QColor secondaryLabel() { return isDark() ? QColor(235, 235, 245, 179) : QColor(60, 60, 67, 179); }
QColor separator()      { return isDark() ? QColor(84, 84, 88, 165)   : QColor(60, 60, 67, 74); }

QColor accent()      { return isDark() ? QColor("#0A84FF") : QColor("#007AFF"); }
QColor destructive() { return isDark() ? QColor("#FF453A") : QColor("#FF3B30"); }
QColor success()     { return isDark() ? QColor("#30D158") : QColor("#34C759"); }
QColor warning()     { return isDark() ? QColor("#FF9F0A") : QColor("#FF9500"); }

QColor nodeFill()          { return QColor("#F2F2F7"); }
QColor nodeStroke()        { return QColor("#3A3A3C"); }
QColor nodeLabel()         { return QColor("#1C1C1E"); }
QColor edge()              { return QColor(235, 235, 245, 160); }
QColor pathHighlight()     { return QColor("#64D2FF"); }
QColor cycleHighlight()    { return QColor("#FF453A"); } // el lienzo es oscuro siempre
QColor pillFill()          { return QColor(28, 28, 30, 210); }
QColor pillStroke()        { return QColor(255, 255, 255, 36); }
QColor pillLabel()         { return QColor(245, 247, 255); }
QColor emptyStateLabel()   { return QColor(235, 235, 245, 160); }
QColor emptyStateCaption() { return QColor(235, 235, 245, 110); }

QFont title() {
    QFont f = baseFont();
    f.setPointSizeF(22.0);
    f.setWeight(QFont::Bold);
    return f;
}

QFont headline() {
    QFont f = baseFont();
    f.setPointSizeF(13.0);
    f.setWeight(QFont::DemiBold);
    return f;
}

QFont body() {
    QFont f = baseFont();
    f.setPointSizeF(12.0);
    f.setWeight(QFont::Normal);
    return f;
}

QFont caption() {
    QFont f = baseFont();
    f.setPointSizeF(10.5);
    f.setWeight(QFont::Normal);
    return f;
}

QString styleSheet() {
    // Tokens con nombre: QString::arg("%10") se confunde con "%1", así que no
    // se pueden usar marcadores numerados más allá de %9.
    QString sheet = QString::fromUtf8(R"(
QWidget {
    color: @label;
}

QMainWindow, QWidget#centralwidget {
    background-color: @systemBackground;
}

QFrame#panelSidebar {
    background-color: @secondaryBackground;
    border: 1px solid @separator;
    border-radius: 16px;
}
QScrollArea#scrollSide, QWidget#panelSide {
    background: transparent;
    border: none;
}

QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 4px 2px 4px 0;
}
QScrollBar::handle:vertical {
    background: @separator;
    border-radius: 4px;
    min-height: 24px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }

QFrame[role="separator"] {
    background-color: @separator;
    border: none;
    max-height: 1px;
    min-height: 1px;
}

QPushButton {
    background-color: @tertiaryBackground;
    color: @label;
    border: none;
    border-radius: @radiuspx;
    padding: 4px @spacingTightpx;
    min-height: @controlHeightpx;
}
QPushButton:hover   { background-color: @separator; }
QPushButton:pressed { background-color: @accentPressed; }
QPushButton:disabled {
    color: @secondaryLabel;
    background-color: @tertiaryBackground;
}

QPushButton[role="primary"] {
    background-color: @accent;
    color: #FFFFFF;
    font-weight: 600;
    min-height: @primaryHeightpx;
}
QPushButton[role="primary"]:hover    { background-color: @accentHover; }
QPushButton[role="primary"]:pressed  { background-color: @accentPressed; }
QPushButton[role="primary"]:disabled { background-color: @tertiaryBackground; color: @secondaryLabel; }

QPushButton[role="destructive"] {
    background-color: transparent;
    color: @destructive;
    border: 1px solid @destructive;
}
QPushButton[role="destructive"]:hover   { background-color: @destructiveWash; }
QPushButton[role="destructive"]:pressed { background-color: @destructiveWash; }
QPushButton[role="destructive"]:disabled {
    color: @secondaryLabel;
    border-color: @separator;
}

QLineEdit, QComboBox {
    background-color: @tertiaryBackground;
    color: @label;
    border: none;
    border-radius: @radiuspx;
    padding: 2px @spacingTightpx;
    min-height: @controlHeightpx;
}
QLineEdit:focus, QComboBox:focus {
    border: 2px solid @accent;
    padding: 0px @focusPadpx;
}
QLineEdit[error="true"], QLineEdit[error="true"]:focus {
    border: 2px solid @destructive;
    padding: 0px @focusPadpx;
}
QLineEdit:disabled, QComboBox:disabled {
    color: @secondaryLabel;
}
QComboBox::drop-down {
    border: none;
    width: 22px;
}
QComboBox QAbstractItemView {
    background-color: @secondaryBackground;
    color: @label;
    border: 1px solid @separator;
    border-radius: 8px;
    selection-background-color: @accent;
    selection-color: #FFFFFF;
    outline: none;
    padding: 4px;
}

QLabel[role="secondary"] { color: @secondaryLabel; }
QLabel[role="error"]     { color: @destructive; }
QLabel[role="warning"] {
    background-color: @warningWash;
    border-left: 3px solid @warning;
    border-radius: 6px;
    padding: 6px 8px;
}

QCheckBox, QRadioButton {
    color: @label;
    spacing: 6px;
    min-height: @minControlpx;
}
QCheckBox::indicator, QRadioButton::indicator {
    width: 16px;
    height: 16px;
    border: 1.5px solid @separator;
    background-color: @tertiaryBackground;
}
QRadioButton::indicator { border-radius: 8px; }
QCheckBox::indicator    { border-radius: 4px; }
QCheckBox::indicator:checked,
QRadioButton::indicator:checked {
    background-color: @accent;
    border-color: @accent;
}
QCheckBox::indicator:hover,
QRadioButton::indicator:hover { border-color: @accent; }

QToolTip {
    background-color: @secondaryBackground;
    color: @label;
    border: 1px solid @separator;
    border-radius: 6px;
    padding: 4px 6px;
}
)");

    QList<QPair<QString, QString>> tokens = {
        {"@label", css(label())},
        {"@systemBackground", css(systemBackground())},
        {"@secondaryBackground", css(secondarySystemBackground())},
        {"@tertiaryBackground", css(tertiarySystemBackground())},
        {"@separator", css(separator())},
        {"@secondaryLabel", css(secondaryLabel())},
        {"@accentPressed", css(accent().darker(118))},
        {"@accentHover", css(accent().lighter(112))},
        {"@accent", css(accent())},
        {"@destructiveWash", css(withAlpha(destructive(), 36))},
        {"@destructive", css(destructive())},
        {"@warningWash", css(withAlpha(warning(), 36))},
        {"@warning", css(warning())},
        {"@spacingTight", QString::number(SpacingTight)},
        {"@radius", QString::number(CornerRadius)},
        {"@controlHeight", QString::number(ControlHeight)},
        {"@primaryHeight", QString::number(PrimaryControlHeight)},
        {"@focusPad", QString::number(SpacingTight - 2)},
        {"@minControl", QString::number(MinControlSize)},
    };
    // Los tokens largos primero, para que «@accent» no pise «@accentHover».
    std::sort(tokens.begin(), tokens.end(),
              [](const auto &a, const auto &b) { return a.first.size() > b.first.size(); });
    for (const auto &token : tokens) sheet.replace(token.first, token.second);
    return sheet;
}

} // namespace Theme
