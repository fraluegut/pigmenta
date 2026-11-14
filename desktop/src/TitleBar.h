#pragma once

#include <QWidget>

namespace pigmenta::desktop {

// Custom dark title bar replacing the native OS one (frameless window).
// Draggable anywhere except the window buttons; double-click toggles
// maximize.
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* parent = nullptr, const QString& title = "PIGMENTA", bool showMinMax = true);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void toggleMaximize();
};

}  // namespace pigmenta::desktop
