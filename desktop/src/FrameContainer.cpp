#include "FrameContainer.h"

#include <QMouseEvent>
#include <QWindow>

namespace pigmenta::desktop {

FrameContainer::FrameContainer(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
}

Qt::Edges FrameContainer::edgesAt(const QPoint& pos) const {
    Qt::Edges edges;
    if (pos.x() <= kMargin)
        edges |= Qt::LeftEdge;
    if (pos.x() >= width() - kMargin)
        edges |= Qt::RightEdge;
    if (pos.y() <= kMargin)
        edges |= Qt::TopEdge;
    if (pos.y() >= height() - kMargin)
        edges |= Qt::BottomEdge;
    return edges;
}

void FrameContainer::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || window()->isMaximized()) {
        QWidget::mousePressEvent(event);
        return;
    }
    const Qt::Edges edges = edgesAt(event->pos());
    if (edges != Qt::Edges()) {
        if (auto* handle = window()->windowHandle()) {
            handle->startSystemResize(edges);
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void FrameContainer::mouseMoveEvent(QMouseEvent* event) {
    if (window()->isMaximized()) {
        setCursor(Qt::ArrowCursor);
        QWidget::mouseMoveEvent(event);
        return;
    }
    const Qt::Edges edges = edgesAt(event->pos());
    if ((edges & Qt::LeftEdge && edges & Qt::TopEdge) || (edges & Qt::RightEdge && edges & Qt::BottomEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if ((edges & Qt::RightEdge && edges & Qt::TopEdge) ||
               (edges & Qt::LeftEdge && edges & Qt::BottomEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (edges & Qt::LeftEdge || edges & Qt::RightEdge) {
        setCursor(Qt::SizeHorCursor);
    } else if (edges & Qt::TopEdge || edges & Qt::BottomEdge) {
        setCursor(Qt::SizeVerCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QWidget::mouseMoveEvent(event);
}

}  // namespace pigmenta::desktop
