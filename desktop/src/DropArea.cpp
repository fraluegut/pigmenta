#include "DropArea.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QMouseEvent>
#include "Translator.h"

namespace pigmenta::desktop {

DropArea::DropArea(QWidget* parent) : QLabel(parent) {
    setAcceptDrops(true);
    setAlignment(Qt::AlignCenter);
    setCursor(Qt::PointingHandCursor);
    setText(Translator::t("dropArea.text"));
    setObjectName("dropArea");
    setMinimumHeight(110);

    connect(&Translator::instance(), &Translator::languageChanged, this, [this]() {
        setText(Translator::t("dropArea.text"));
    });
}

void DropArea::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void DropArea::dropEvent(QDropEvent* event) {
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;
    const QString path = urls.first().toLocalFile();
    if (!path.isEmpty())
        emit imagePathChosen(path);
}

void DropArea::mousePressEvent(QMouseEvent*) {
    openFileDialog();
}

void DropArea::openFileDialog() {
    const QString path = QFileDialog::getOpenFileName(
        this, Translator::t("dialog.openImage"), QString(), Translator::t("dialog.imageFilter"));
    if (!path.isEmpty())
        emit imagePathChosen(path);
}

}  // namespace pigmenta::desktop
