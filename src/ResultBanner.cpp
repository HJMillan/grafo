#include "ResultBanner.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

namespace {
QColor colorFor(ResultBanner::Kind kind) {
    switch (kind) {
    case ResultBanner::Success: return Theme::success();
    case ResultBanner::Warning: return Theme::warning();
    case ResultBanner::Error:   return Theme::destructive();
    case ResultBanner::Info:
    default:                    return Theme::accent();
    }
}

QString glyphFor(ResultBanner::Kind kind) {
    switch (kind) {
    case ResultBanner::Success: return QStringLiteral("✓");
    case ResultBanner::Warning: return QStringLiteral("!");
    case ResultBanner::Error:   return QStringLiteral("×");
    case ResultBanner::Info:
    default:                    return QStringLiteral("i");
    }
}

// Círculo del color del tipo con su símbolo en blanco.
QPixmap iconFor(ResultBanner::Kind kind, qreal dpr) {
    const int size = 22;
    QPixmap pm(QSize(size, size) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(colorFor(kind));
    p.drawEllipse(QRectF(0, 0, size, size));
    QFont f = Theme::headline();
    f.setPointSizeF(kind == ResultBanner::Error ? 14 : 11);
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, glyphFor(kind));
    return pm;
}
}

ResultBanner::ResultBanner(QWidget *parent) : QFrame(parent) {
    setObjectName("resultBanner");
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    m_icon = new QLabel(this);
    m_icon->setFixedSize(22, 22);

    m_title = new QLabel(this);
    m_title->setWordWrap(true);

    m_text = new QLabel(this);
    m_text->setWordWrap(true);
    m_text->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *texts = new QVBoxLayout;
    texts->setSpacing(2);
    texts->addWidget(m_title);
    texts->addWidget(m_text);

    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(Theme::SpacingBezeled + 2, Theme::SpacingBezeled,
                            Theme::SpacingBezeled + 2, Theme::SpacingBezeled);
    row->setSpacing(Theme::SpacingBezeled);
    row->addWidget(m_icon, 0, Qt::AlignTop);
    row->addLayout(texts, 1);

    setMessage(Info, QString());
}

void ResultBanner::setMessage(Kind kind, const QString &title, const QString &text) {
    m_kind = kind;
    m_title->setText(title);
    m_text->setText(text);
    m_text->setVisible(!text.isEmpty());
    refreshStyle();
}

void ResultBanner::refreshStyle() {
    // Fondo: el color del tipo muy atenuado, para que se lea como estado y no como botón.
    QColor wash = colorFor(m_kind);
    wash.setAlpha(Theme::isDark() ? 40 : 28);
    setStyleSheet(QString("#resultBanner { background-color: rgba(%1, %2, %3, %4);"
                          " border: none; border-radius: 12px; }"
                          "#resultBanner QLabel { background: transparent; }")
                          .arg(wash.red()).arg(wash.green()).arg(wash.blue()).arg(wash.alpha()));
    m_title->setFont(Theme::headline());
    m_text->setFont(Theme::body());
    m_icon->setPixmap(iconFor(m_kind, devicePixelRatioF()));
}

QString ResultBanner::title() const { return m_title->text(); }
QString ResultBanner::text() const { return m_text->text(); }
