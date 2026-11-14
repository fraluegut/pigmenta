#pragma once

#include <QDialog>
#include <QString>

class QCheckBox;
class QVBoxLayout;
class QWidget;

namespace pigmenta::desktop {

// Startup welcome panel: logo, blurb, sample gallery, "don't show again".
// Also reused (without persisting the checkbox) as the Help > About dialog.
class WelcomeDialog : public QDialog {
    Q_OBJECT
public:
    explicit WelcomeDialog(QWidget* parent, bool isStartup);

    // Whether the startup welcome panel should be shown (i.e. the user
    // hasn't ticked "don't show this again" in a previous run).
    static bool shouldShowOnStartup();

signals:
    void sampleRequested(const QString& resourcePath);

private:
    void buildContent();

    QVBoxLayout* m_cardLayout = nullptr;
    QWidget* m_content = nullptr;
    QCheckBox* m_dontShowAgain = nullptr;
    bool m_isStartup;
};

}  // namespace pigmenta::desktop
