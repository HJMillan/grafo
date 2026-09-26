#include "ResultBanner.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

namespace {
struct KindStyle {
    const char *accent;
    const char *background;
    QStyle::StandardPixmap icon;
};

KindStyle styleFor(ResultBanner::Kind kind) {
    switch (kind) {
    case ResultBanner::Success: return {"#2e9e5b", "rgba(46, 158, 91, 0.14)", QStyle::SP_DialogApplyButton};
    case ResultBanner::Warning: return {"#d18b00", "rgba(209, 139, 0, 0.14)", QStyle::SP_MessageBoxWarning};
    case ResultBanner::Error:   return {"#d64545", "rgba(214, 69, 69, 0.14)", QStyle::SP_MessageBoxCritical};
    case ResultBanner::Info:
    default:                    return {"#3a7bd5", "rgba(58, 123, 213, 0.12)", QStyle::SP_MessageBoxInformation};
    }
}
}

ResultBanner::ResultBanner(QWidget *parent) : QFrame(parent) {
    setObjectName("resultBanner");
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    m_icon = new QLabel(this);
    m_icon->setFixedSize(24, 24);
    m_icon->setAlignment(Qt::AlignTop);

    m_title = new QLabel(this);
    m_title->setWordWrap(true);
    QFont f = m_title->font();
    f.setBold(true);
    m_title->setFont(f);

    m_text = new QLabel(this);
    m_text->setWordWrap(true);
    m_text->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *texts = new QVBoxLayout;
    texts->setSpacing(2);
    texts->addWidget(m_title);
    texts->addWidget(m_text);

    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(12, 8, 12, 8);
    row->setSpacing(10);
    row->addWidget(m_icon, 0, Qt::AlignTop);
    row->addLayout(texts, 1);

    setMessage(Info, QString());
}

void ResultBanner::setMessage(Kind kind, const QString &title, const QString &text) {
    m_kind = kind;
    const KindStyle s = styleFor(kind);
    setStyleSheet(QString("#resultBanner { background: %1; border: 1px solid %2;"
                          " border-left: 5px solid %2; border-radius: 4px; }")
                          .arg(s.background, s.accent));
    m_icon->setPixmap(style()->standardIcon(s.icon).pixmap(20, 20));
    m_title->setText(title);
    m_text->setText(text);
    m_text->setVisible(!text.isEmpty());
}

QString ResultBanner::title() const { return m_title->text(); }
QString ResultBanner::text() const { return m_text->text(); }
