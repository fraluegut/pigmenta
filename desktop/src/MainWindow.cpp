#include "MainWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QButtonGroup>
#include <QFrame>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QTimer>
#include <QIcon>
#include <QPixmap>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QEvent>
#include <QStyle>

#include "FilterPanel.h"
#include "DropArea.h"
#include "Engine.h"
#include "WelcomeDialog.h"
#include "DocsDialog.h"
#include "TitleBar.h"
#include "FrameContainer.h"
#include "Translator.h"

#ifndef PIGMENTA_VERSION
#define PIGMENTA_VERSION "0.0.0-dev"
#endif

namespace pigmenta::desktop {

namespace {
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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Pigmenta");
    setWindowIcon(QIcon(QPixmap(":/logo.png")));
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(1180, 760);

    buildUi();

    connect(&Translator::instance(), &Translator::languageChanged, this, &MainWindow::retranslate);

    if (WelcomeDialog::shouldShowOnStartup()) {
        QTimer::singleShot(0, this, [this]() { showWelcome(true); });
    }
}

void MainWindow::retranslate() {
    // Menu bar: rebuild in place (cheap, and QAction text can't easily be
    // mass-retranslated without keeping a parallel key map).
    const int menuIndex = m_cardLayout->indexOf(m_menuBar);
    m_cardLayout->removeWidget(m_menuBar);
    m_menuBar->deleteLater();
    m_menuBar = buildMenuBar();
    m_cardLayout->insertWidget(menuIndex, m_menuBar);

    for (auto it = m_modeButtons.constBegin(); it != m_modeButtons.constEnd(); ++it) {
        const auto mode = static_cast<CompareView::Mode>(it.key());
        const char* key = mode == CompareView::Mode::After    ? "mode.after"
                          : mode == CompareView::Mode::Before ? "mode.before"
                          : mode == CompareView::Mode::Wipe   ? "mode.wipe"
                                                              : "mode.sideBySide";
        it.value()->setText(Translator::t(key));
    }

    m_exportButton->setText(Translator::t("sidebar.exportButton"));
    if (!m_hasImage)
        m_statusLabel->setText(Translator::t("status.noImage"));

    m_compareView->update();
}

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
        updateMaximizedVisual();
}

void MainWindow::updateMaximizedVisual() {
    if (!m_windowCard)
        return;
    m_windowCard->setProperty("maximized", isMaximized());
    style()->unpolish(m_windowCard);
    style()->polish(m_windowCard);
}

QMenuBar* MainWindow::buildMenuBar() {
    auto* bar = new QMenuBar();
    bar->setObjectName("appMenuBar");

    auto* fileMenu = bar->addMenu(Translator::t("menu.file"));
    fileMenu->addAction(
        Translator::t("menu.openImage"), QKeySequence::Open, this, &MainWindow::openFileDialog);

    auto* sampleMenu = fileMenu->addMenu(Translator::t("menu.openSample"));
    for (const SampleEntry& s : kSamples) {
        const QString resource = s.resource;
        sampleMenu->addAction(Translator::t(s.labelKey), [this, resource]() { loadImage(resource); });
    }

    fileMenu->addSeparator();
    fileMenu->addAction(
        Translator::t("menu.exportImage"), QKeySequence::Save, this, &MainWindow::exportImage);
    fileMenu->addSeparator();
    fileMenu->addAction(Translator::t("menu.exit"), QKeySequence::Quit, this, &QWidget::close);

    auto* viewMenu = bar->addMenu(Translator::t("menu.view"));
    auto* group = new QActionGroup(this);
    group->setExclusive(true);
    m_modeActions.clear();

    const CompareView::Mode currentMode = m_compareView ? m_compareView->mode() : CompareView::Mode::After;

    auto addModeAction = [&](const QString& key, CompareView::Mode mode) {
        auto* action = viewMenu->addAction(Translator::t(key));
        action->setCheckable(true);
        group->addAction(action);
        connect(action, &QAction::triggered, this, [this, mode]() { setViewMode(mode); });
        m_modeActions[static_cast<int>(mode)] = action;
    };
    addModeAction("mode.after", CompareView::Mode::After);
    addModeAction("mode.before", CompareView::Mode::Before);
    addModeAction("mode.wipe", CompareView::Mode::Wipe);
    addModeAction("mode.sideBySide", CompareView::Mode::SideBySide);
    m_modeActions[static_cast<int>(currentMode)]->setChecked(true);

    auto* helpMenu = bar->addMenu(Translator::t("menu.help"));
    helpMenu->addAction(Translator::t("menu.documentation"), this, [this]() {
        auto* dialog = new DocsDialog(this);
        dialog->exec();
        dialog->deleteLater();
    });
    helpMenu->addSeparator();
    helpMenu->addAction(Translator::t("menu.about"), this, [this]() { showWelcome(false); });

    return bar;
}

void MainWindow::buildUi() {
    auto* frame = new FrameContainer();
    setCentralWidget(frame);

    auto* frameLayout = new QVBoxLayout(frame);
    const int m = FrameContainer::kMargin;
    frameLayout->setContentsMargins(m, m, m, m);
    frameLayout->setSpacing(0);

    m_windowCard = new QWidget();
    m_windowCard->setObjectName("windowCard");
    m_windowCard->setAttribute(Qt::WA_StyledBackground, true);
    frameLayout->addWidget(m_windowCard);

    auto* cardLayout = new QVBoxLayout(m_windowCard);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    m_cardLayout = cardLayout;
    cardLayout->addWidget(new TitleBar());
    m_menuBar = buildMenuBar();
    cardLayout->addWidget(m_menuBar);

    // ── Body: sidebar + viewer ──────────────────────────────────────────
    auto* body = new QWidget();
    auto* bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(10, 10, 10, 10);
    bodyLayout->setSpacing(10);

    auto* sidebar = new QWidget();
    sidebar->setObjectName("sidebar");
    sidebar->setAttribute(Qt::WA_StyledBackground, true);
    sidebar->setFixedWidth(280);
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 16, 16, 16);
    sidebarLayout->setSpacing(14);

    auto* brandRow = new QHBoxLayout();
    brandRow->setSpacing(10);
    auto* logo = new QLabel();
    logo->setPixmap(QPixmap(":/logo.png").scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    brandRow->addWidget(logo);
    auto* brand = new QLabel("PIGMENTA");
    brand->setObjectName("brand");
    brandRow->addWidget(brand);
    brandRow->addStretch(1);
    sidebarLayout->addLayout(brandRow);

    m_dropArea = new DropArea();
    sidebarLayout->addWidget(m_dropArea);
    connect(m_dropArea, &DropArea::imagePathChosen, this, &MainWindow::loadImage);

    m_filterPanel = new FilterPanel();
    sidebarLayout->addWidget(m_filterPanel, 1);
    connect(m_filterPanel, &FilterPanel::parametersChanged, this, &MainWindow::reprocess);

    m_exportButton = new QPushButton(Translator::t("sidebar.exportButton"));
    m_exportButton->setObjectName("exportButton");
    m_exportButton->setEnabled(false);
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::exportImage);
    sidebarLayout->addWidget(m_exportButton);

    bodyLayout->addWidget(sidebar);

    auto* viewerWidget = new QWidget();
    viewerWidget->setObjectName("viewer");
    viewerWidget->setAttribute(Qt::WA_StyledBackground, true);
    auto* viewerLayout = new QVBoxLayout(viewerWidget);
    viewerLayout->setContentsMargins(16, 14, 16, 16);
    viewerLayout->setSpacing(10);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    auto* group = new QButtonGroup(this);
    group->setExclusive(true);

    auto addModeButton = [&](const QString& key, CompareView::Mode mode) {
        auto* btn = new QPushButton(Translator::t(key));
        btn->setObjectName("modeButton");
        btn->setCheckable(true);
        group->addButton(btn);
        connect(btn, &QPushButton::clicked, this, [this, mode]() { setViewMode(mode); });
        toolbar->addWidget(btn);
        m_modeButtons[static_cast<int>(mode)] = btn;
    };
    addModeButton("mode.after", CompareView::Mode::After);
    addModeButton("mode.before", CompareView::Mode::Before);
    addModeButton("mode.wipe", CompareView::Mode::Wipe);
    addModeButton("mode.sideBySide", CompareView::Mode::SideBySide);
    m_modeButtons[static_cast<int>(CompareView::Mode::After)]->setChecked(true);

    toolbar->addStretch(1);
    viewerLayout->addLayout(toolbar);

    m_compareView = new CompareView();
    viewerLayout->addWidget(m_compareView, 1);

    bodyLayout->addWidget(viewerWidget, 1);

    cardLayout->addWidget(body, 1);
    cardLayout->addWidget(buildStatusRow());
}

QWidget* MainWindow::buildStatusRow() {
    auto* row = new QWidget();
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(16, 6, 12, 8);

    m_statusLabel = new QLabel(Translator::t("status.noImage"));
    m_statusLabel->setObjectName("statusLabel");
    layout->addWidget(m_statusLabel);
    layout->addStretch(1);

    auto* version = new QLabel(QString("Pigmenta Desktop v%1").arg(PIGMENTA_VERSION));
    version->setObjectName("versionLabel");
    layout->addWidget(version);

    return row;
}

void MainWindow::openFileDialog() {
    const QString path = QFileDialog::getOpenFileName(
        this, Translator::t("dialog.openImage"), QString(), Translator::t("dialog.imageFilter"));
    if (!path.isEmpty())
        loadImage(path);
}

void MainWindow::showWelcome(bool isStartup) {
    auto* dialog = new WelcomeDialog(this, isStartup);
    connect(dialog, &WelcomeDialog::sampleRequested, this, &MainWindow::loadImage);
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::loadImage(const QString& path) {
    QImage img(path);
    if (img.isNull()) {
        m_statusLabel->setText(Translator::t("status.failedLoad") + QFileInfo(path).fileName());
        return;
    }
    m_original = img;
    m_currentPath = path;
    m_hasImage = true;
    m_exportButton->setEnabled(true);
    reprocess();
}

void MainWindow::reprocess() {
    if (m_original.isNull())
        return;

    m_processed = applyFilter(m_original, m_filterPanel->currentFilter(), m_filterPanel->currentValues());
    m_compareView->setImages(m_original, m_processed);

    m_statusLabel->setText(QString("%1 — %2x%3")
                               .arg(QFileInfo(m_currentPath).fileName())
                               .arg(m_original.width())
                               .arg(m_original.height()));
}

void MainWindow::setViewMode(CompareView::Mode mode) {
    m_compareView->setMode(mode);
    for (auto it = m_modeButtons.constBegin(); it != m_modeButtons.constEnd(); ++it) {
        it.value()->setChecked(it.key() == static_cast<int>(mode));
    }
    for (auto it = m_modeActions.constBegin(); it != m_modeActions.constEnd(); ++it) {
        it.value()->setChecked(it.key() == static_cast<int>(mode));
    }
}

void MainWindow::exportImage() {
    if (m_processed.isNull())
        return;
    const QString dir =
        m_currentPath.startsWith(':') ? QDir::homePath() : QFileInfo(m_currentPath).absolutePath();
    const QString suggested = dir + "/" + QFileInfo(m_currentPath).completeBaseName() + "_pigmenta.png";
    const QString path = QFileDialog::getSaveFileName(
        this, Translator::t("dialog.exportImage"), suggested, "PNG (*.png);;JPEG (*.jpg)");
    if (path.isEmpty())
        return;
    if (!m_processed.save(path)) {
        m_statusLabel->setText(Translator::t("status.failedExport"));
    } else {
        m_statusLabel->setText(Translator::t("status.exported") + QFileInfo(path).fileName());
    }
}

}  // namespace pigmenta::desktop
