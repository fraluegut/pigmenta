#pragma once

#include <QObject>
#include <QString>

namespace pigmenta::desktop {

enum class Lang { En, Es };

// Lightweight runtime translator (not Qt Linguist) — a flat key -> {en, es}
// table. Persistent widgets connect to languageChanged() to retranslate
// their own text in place; ephemeral dialogs just read t() when built.
class Translator : public QObject {
    Q_OBJECT
public:
    static Translator& instance();

    Lang language() const { return m_lang; }
    void setLanguage(Lang lang);

    // Looks up `key` in the table for the current language. Falls back to
    // the key itself (visibly broken, easy to spot) if missing.
    static QString t(const QString& key);

signals:
    void languageChanged();

private:
    Translator();
    Lang m_lang = Lang::En;
};

}  // namespace pigmenta::desktop
