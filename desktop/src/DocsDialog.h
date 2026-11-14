#pragma once

#include <QDialog>

class QTextBrowser;
class QListWidget;

namespace pigmenta::desktop {

// In-app documentation viewer (README, architecture, building) — bundled as
// Qt resources so it works offline and regardless of whether the GitHub
// remote is up to date.
class DocsDialog : public QDialog {
    Q_OBJECT
public:
    explicit DocsDialog(QWidget* parent = nullptr);

private:
    QTextBrowser* m_browser = nullptr;
};

}  // namespace pigmenta::desktop
