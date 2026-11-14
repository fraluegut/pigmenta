#include "TitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QPixmap>
#include <QMouseEvent>
#include <QWindow>

namespace pigmenta::desktop {

namespace {
QToolButton* makeWinButton(const QString& glyph, const QString& objectName) {
    auto* btn = new QToolButton();
    btn->setObjectName(objectName);
    btn->setText(glyph);
    btn->setFixedSize(32, 26);
    btn->setCursor(Qt::ArrowCursor);
    return btn;
}
}  // namespace

TitleBar::TitleBar(QWidget* parent, const QString& title, bool showMinMax) : QWidget(parent) {
    setObjectName("titleBar");
    setFixedHeight(34);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 6, 0);
    layout->setSpacing(8);

    auto* logo = new QLabel();
    logo->setPixmap(QPixmap(":/logo.png").scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(logo);

    auto* titleLabel = new QLabel(title);
    titleLabel->setObjectName("titleBrand");
    layout->addWidget(titleLabel);

    layout->addStretch(1);

    if (showMinMax) {
        auto* minBtn = makeWinButton(QString(QChar(0x2212)), "winMin");
        auto* maxBtn = makeWinButton(QString(QChar(0x25A1)), "winMax");
        connect(minBtn, &QToolButton::clicked, this, [this]() { window()->showMinimized(); });
        connect(maxBtn, &QToolButton::clicked, this, &TitleBar::toggleMaximize);
        layout->addWidget(minBtn);
        layout->addWidget(maxBtn);
    }

    auto* closeBtn = makeWinButton(QString(QChar(0x00D7)), "winClose");
    connect(closeBtn, &QToolButton::clicked, this, [this]() { window()->close(); });
    layout->addWidget(closeBtn);
}

void TitleBar::toggleMaximize() {
    QWidget* w = window();
    if (w->isMaximized())
        w->showNormal();
    else
        w->showMaximized();
}

void TitleBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (auto* handle = window()->windowHandle()) {
            handle->startSystemMove();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        toggleMaximize();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

}  // namespace pigmenta::desktop
