#pragma once

#include <QWidget>

namespace pigmenta::desktop {

// Transparent margin around the rounded window card, wide enough to grab
// for resizing a frameless window (there's no native border to drag).
class FrameContainer : public QWidget {
    Q_OBJECT
public:
    explicit FrameContainer(QWidget* parent = nullptr);

    static constexpr int kMargin = 6;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    Qt::Edges edgesAt(const QPoint& pos) const;
};

}  // namespace pigmenta::desktop
