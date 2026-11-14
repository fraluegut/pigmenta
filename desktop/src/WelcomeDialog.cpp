#include "WelcomeDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSettings>
#include <QPixmap>
#include <QToolButton>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include <QFrame>
#include <QPalette>
#include <QColor>
#include <QButtonGroup>
#include "TitleBar.h"
#include "FrameContainer.h"
#include "Translator.h"

#ifndef PIGMENTA_VERSION
#define PIGMENTA_VERSION "0.0.0-dev"
#endif

namespace pigmenta::desktop {

namespace {
constexpr const char* kSettingsKey = "welcome/dontShowAgain";
constexpr const char* kGitHubUrl = "https://github.com/fraluegut/pigmenta";
constexpr const char* kArqueoBytesUrl = "https://arqueobytes.es";

struct SampleEntry {
    const char* resource;
    const char* labelKey;
};
constexpr SampleEntry kSamples[] = {
    {":/samples/huashan_1.jpg", "sample.huashan1"},
    {":/samples/zuojiang_huashan_2.jpg", "sample.zuojiang"},
    {":/samples/thailand_petroglyph.jpg", "sample.thailand"},
    {":/samples/lorito_picada_paraguay.jpg", "sample.paraguay"},
};
}  // namespace

bool WelcomeDialog::shouldShowOnStartup() {
    QSettings settings;
    return !settings.value(kSettingsKey, false).toBool();
}

WelcomeDialog::WelcomeDialog(QWidget* parent, bool isStartup) : QDialog(parent), m_isStartup(isStartup) {
    setModal(true);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(560);

    auto* dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(0, 0, 0, 0);

    auto* frame = new FrameContainer();
    dialogLayout->addWidget(frame);
    auto* frameLayout = new QVBoxLayout(frame);
    const int m = FrameContainer::kMargin;
    frameLayout->setContentsMargins(m, m, m, m);

    auto* card = new QWidget();
    card->setObjectName("windowCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    frameLayout->addWidget(card);

    m_cardLayout = new QVBoxLayout(card);
    m_cardLayout->setContentsMargins(0, 0, 0, 0);
    m_cardLayout->setSpacing(0);
    m_cardLayout->addWidget(new TitleBar(nullptr, Translator::t("welcome.title"), false));

    buildContent();
    connect(&Translator::instance(), &Translator::languageChanged, this, &WelcomeDialog::buildContent);

    connect(this, &QDialog::finished, this, [this]() {
        if (m_isStartup && m_dontShowAgain) {
            QSettings settings;
            settings.setValue(kSettingsKey, m_dontShowAgain->isChecked());
        }
    });
}

void WelcomeDialog::buildContent() {
    const bool wasChecked = m_dontShowAgain && m_dontShowAgain->isChecked();

    if (m_content) {
        m_cardLayout->removeWidget(m_content);
        m_content->deleteLater();
        m_dontShowAgain = nullptr;
    }

    m_content = new QWidget();
    m_cardLayout->addWidget(m_content);

    auto* root = new QVBoxLayout(m_content);
    root->setContentsMargins(28, 24, 28, 20);
    root->setSpacing(16);

    auto* header = new QHBoxLayout();
    auto* logo = new QLabel();
    logo->setPixmap(QPixmap(":/logo.png").scaled(84, 84, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    header->addWidget(logo);

    auto* titleBox = new QVBoxLayout();
    auto* title = new QLabel(Translator::t("welcome.brand"));
    title->setObjectName("brand");
    auto* subtitle = new QLabel(Translator::t("welcome.subtitle"));
    subtitle->setObjectName("welcomeSubtitle");
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);
    header->addLayout(titleBox);
    header->addStretch(1);

    // Language switch (EN / ES).
    auto* langGroup = new QButtonGroup(m_content);
    langGroup->setExclusive(true);
    auto* enBtn = new QPushButton("EN");
    auto* esBtn = new QPushButton("ES");
    for (auto* btn : {enBtn, esBtn}) {
        btn->setObjectName("langButton");
        btn->setCheckable(true);
        btn->setFixedSize(40, 26);
        langGroup->addButton(btn);
    }
    enBtn->setChecked(Translator::instance().language() == Lang::En);
    esBtn->setChecked(Translator::instance().language() == Lang::Es);
    connect(enBtn, &QPushButton::clicked, this, []() { Translator::instance().setLanguage(Lang::En); });
    connect(esBtn, &QPushButton::clicked, this, []() { Translator::instance().setLanguage(Lang::Es); });
    header->addWidget(enBtn);
    header->addWidget(esBtn);

    root->addLayout(header);

    auto* blurb = new QLabel(Translator::t("welcome.blurb"));
    blurb->setWordWrap(true);
    blurb->setObjectName("welcomeBlurb");
    root->addWidget(blurb);

    auto* samplesTitle = new QLabel(Translator::t("welcome.examples"));
    samplesTitle->setObjectName("sectionTitle");
    root->addWidget(samplesTitle);

    auto* samplesRow = new QHBoxLayout();
    samplesRow->setSpacing(10);
    for (const SampleEntry& s : kSamples) {
        auto* btn = new QToolButton();
        btn->setObjectName("sampleButton");
        btn->setIcon(QIcon(
            QPixmap(s.resource).scaled(96, 96, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)));
        btn->setIconSize(QSize(96, 72));
        btn->setText(Translator::t(s.labelKey));
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        const QString resource = s.resource;
        connect(btn, &QToolButton::clicked, this, [this, resource]() {
            emit sampleRequested(resource);
            accept();
        });
        samplesRow->addWidget(btn);
    }
    root->addLayout(samplesRow);

    if (!m_isStartup) {
        auto* sep = new QFrame();
        sep->setFrameShape(QFrame::HLine);
        sep->setObjectName("hairline");
        root->addWidget(sep);

        auto* infoTitle = new QLabel(Translator::t("welcome.about"));
        infoTitle->setObjectName("sectionTitle");
        root->addWidget(infoTitle);

        auto* info = new QLabel(
            Translator::t("about.version").arg(PIGMENTA_VERSION, __DATE__) + "<br>" +
            Translator::t("about.createdBy") + "<a href=\"https://github.com/fraluegut\">fraluegut</a><br>" +
            Translator::t("about.projectBy") +
            QString("<a href=\"%1\">ArqueoBytes</a> — %1<br>").arg(kArqueoBytesUrl) +
            Translator::t("about.license") + "<br>" + Translator::t("about.source") +
            QString("<a href=\"%1\">github.com/fraluegut/pigmenta</a><br>").arg(kGitHubUrl) +
            Translator::t("about.credits") +
            QString("<a href=\"%1/blob/master/desktop/samples/CREDITS.md\">%2</a>")
                .arg(kGitHubUrl, Translator::t("about.creditsLink")));
        info->setObjectName("welcomeBlurb");
        info->setTextFormat(Qt::RichText);
        info->setOpenExternalLinks(true);
        QPalette pal = info->palette();
        pal.setColor(QPalette::Link, QColor(0xe8, 0xa3, 0x3d));
        info->setPalette(pal);
        info->setWordWrap(true);
        root->addWidget(info);
    }

    auto* footer = new QHBoxLayout();
    if (m_isStartup) {
        m_dontShowAgain = new QCheckBox(Translator::t("welcome.dontShowAgain"));
        m_dontShowAgain->setChecked(wasChecked);
        footer->addWidget(m_dontShowAgain);
    }
    footer->addStretch(1);

    auto* startButton =
        new QPushButton(m_isStartup ? Translator::t("welcome.getStarted") : Translator::t("welcome.close"));
    startButton->setObjectName("exportButton");
    startButton->setDefault(true);
    connect(startButton, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(startButton);
    root->addLayout(footer);
}

}  // namespace pigmenta::desktop
