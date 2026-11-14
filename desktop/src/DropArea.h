#pragma once

#include <QLabel>

namespace pigmenta::desktop {

// Small drag-and-drop target: accepts an image file dropped on it, or opens
// a file dialog on click.
class DropArea : public QLabel {
    Q_OBJECT
public:
    explicit DropArea(QWidget* parent = nullptr);

signals:
    void imagePathChosen(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void openFileDialog();
};

}  // namespace pigmenta::desktop
