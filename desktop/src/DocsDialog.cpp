#include "DocsDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QTextBrowser>
#include <QFile>
#include <QTextStream>
#include <QTextDocument>
#include "TitleBar.h"
#include "FrameContainer.h"
#include "Translator.h"

namespace pigmenta::desktop {

namespace {
struct DocEntry {
    const char* titleKey;
    const char* resource;
};
constexpr DocEntry kDocs[] = {
    {"docs.overview", ":/docs/README.md"},
    {"docs.architecture", ":/docs/ARCHITECTURE.md"},
    {"docs.building", ":/docs/BUILDING.md"},
};

QString readResource(const char* path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return Translator::t("docs.notFound");
    return QTextStream(&f).readAll();
}
}  // namespace

DocsDialog::DocsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(Translator::t("docs.title") + " — Pigmenta");
    setModal(true);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(760, 560);

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

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);
    cardLayout->addWidget(new TitleBar(nullptr, Translator::t("docs.title"), false));

    auto* content = new QWidget();
    cardLayout->addWidget(content, 1);

    auto* root = new QHBoxLayout(content);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);

    auto* list = new QListWidget();
    list->setObjectName("filterList");
    list->setFixedWidth(160);
    for (const DocEntry& d : kDocs)
        list->addItem(Translator::t(d.titleKey));
    root->addWidget(list);

    m_browser = new QTextBrowser();
    m_browser->setOpenExternalLinks(true);
    m_browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_browser->document()->setDefaultStyleSheet(R"(
        body { color: #d8d2ca; line-height: 1.55; }
        h1 { font-size: 17px; color: #e8a33d; font-weight: 600;
             border-bottom: 1px solid #2c2722; padding-bottom: 8px; margin-bottom: 14px; }
        h2 { font-size: 13px; color: #e8e3dc; font-weight: 600;
             margin-top: 22px; margin-bottom: 8px; }
        h3 { font-size: 11px; color: #c2bbb1; font-weight: 600; margin-top: 16px; }
        p { margin: 6px 0; }
        a { color: #e8a33d; text-decoration: none; }
        code { background-color: #120e0c; color: #e8a33d; padding: 1px 5px;
               border-radius: 3px; font-family: "JetBrains Mono", Consolas, monospace; font-size: 10px; }
        pre { background-color: #120e0c; border: 1px solid #2c2722; border-radius: 6px;
              padding: 10px 12px; margin: 10px 0; }
        pre code { background: transparent; padding: 0; color: #c2bbb1; }
        table { border: none; border-collapse: collapse; margin: 10px 0; }
        th, td { border: 1px solid #2c2722; padding: 5px 8px; font-size: 10px; }
        th { background-color: #201c19; color: #e8a33d; font-weight: 600; }
        tr:nth-child(even) td { background-color: #18140f; }
        ul, ol { margin: 6px 0; padding-left: 20px; }
        li { margin: 3px 0; }
    )");
    root->addWidget(m_browser, 1);

    connect(list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0)
            return;
        m_browser->setMarkdown(readResource(kDocs[static_cast<size_t>(row)].resource));
    });
    list->setCurrentRow(0);
}

}  // namespace pigmenta::desktop
