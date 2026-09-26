#ifndef RESULTBANNER_H
#define RESULTBANNER_H

#include <QFrame>

class QLabel;

// Panel de resultado con cuatro tipos (información, éxito, aviso, error).
// Cada tipo tiene su color e icono, un título corto y una explicación.
class ResultBanner : public QFrame {
    Q_OBJECT
public:
    enum Kind { Info, Success, Warning, Error };

    explicit ResultBanner(QWidget *parent = nullptr);

    void setMessage(Kind kind, const QString &title, const QString &text = {});
    Kind kind() const { return m_kind; }
    QString title() const;
    QString text() const;

private:
    Kind m_kind = Info;
    QLabel *m_icon;
    QLabel *m_title;
    QLabel *m_text;
};

#endif // RESULTBANNER_H
