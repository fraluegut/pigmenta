#include "CompareView.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPen>
#include <algorithm>
#include "Translator.h"

namespace pigmenta::desktop {

namespace {
constexpr QColor kPanelBg(0x16, 0x13, 0x12);   // matches the viewer panel surface
constexpr QColor kCanvasBg(0x12, 0x10, 0x0e);  // sunken canvas, slightly darker
constexpr QColor kAccent(0xe8, 0xa3, 0x3d);
constexpr QColor kMuted(0x8a, 0x81, 0x78);
constexpr qreal kCornerRadius = 10.0;
}  // namespace

CompareView::CompareView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(320, 240);
    connect(&Translator::instance(), &Translator::languageChanged, this, [this]() { update(); });
}

void CompareView::setImages(const QImage& before, const QImage& after) {
    const bool newImage = m_after.size() != after.size();
    m_before = before;
    m_after = after;
    if (newImage)
        m_zoom = 1.0;
    update();
}

void CompareView::setMode(Mode mode) {
    m_mode = mode;
    update();
}

QRect CompareView::imageRect(const QSize& imgSize) const {
    if (imgSize.isEmpty())
        return rect();
    const QSize fitted = imgSize.scaled(rect().size(), Qt::KeepAspectRatio);
    const QSize zoomed = fitted * m_zoom;
    QRect r(QPoint(0, 0), zoomed);
    r.moveCenter(rect().center());
    return r;
}

void CompareView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // Seamless against the surrounding viewer card...
    p.fillRect(rect(), kPanelBg);

    // ...with a rounded, slightly sunken canvas for the actual image.
    QPainterPath canvasPath;
    canvasPath.addRoundedRect(QRectF(rect()), kCornerRadius, kCornerRadius);
    p.setClipPath(canvasPath);
    p.fillRect(rect(), kCanvasBg);

    if (!m_after.isNull() && std::abs(m_zoom - 1.0) > 0.01) {
        p.setPen(kMuted);
        QFont f = p.font();
        f.setPointSize(8);
        p.setFont(f);
        p.drawText(rect().adjusted(0, 0, -8, -6),
                   Qt::AlignBottom | Qt::AlignRight,
                   QString("%1%").arg(qRound(m_zoom * 100)));
    }

    if (m_after.isNull()) {
        p.setPen(kMuted);
        QFont f = p.font();
        f.setPointSize(9);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter, Translator::t("compareView.empty"));
        return;
    }

    switch (m_mode) {
        case Mode::After: {
            const QRect r = imageRect(m_after.size());
            p.drawImage(r, m_after);
            break;
        }
        case Mode::Before: {
            const QImage& img = m_before.isNull() ? m_after : m_before;
            const QRect r = imageRect(img.size());
            p.drawImage(r, img);
            break;
        }
        case Mode::SideBySide: {
            const QRect left(0, 0, width() / 2, height());
            const QRect right(width() / 2, 0, width() - width() / 2, height());
            const QImage& beforeImg = m_before.isNull() ? m_after : m_before;

            QSize fittedLeft = beforeImg.size().scaled(left.size(), Qt::KeepAspectRatio) * m_zoom;
            QRect leftRect(QPoint(0, 0), fittedLeft);
            leftRect.moveCenter(left.center());
            p.setClipRect(left, Qt::IntersectClip);
            p.drawImage(leftRect, beforeImg);
            p.setClipPath(canvasPath);

            QSize fittedRight = m_after.size().scaled(right.size(), Qt::KeepAspectRatio) * m_zoom;
            QRect rightRect(QPoint(0, 0), fittedRight);
            rightRect.moveCenter(right.center());
            p.setClipRect(right, Qt::IntersectClip);
            p.drawImage(rightRect, m_after);
            p.setClipPath(canvasPath);

            p.setPen(QPen(kMuted, 1));
            p.drawLine(width() / 2, 0, width() / 2, height());
            break;
        }
        case Mode::Wipe: {
            const QImage& beforeImg = m_before.isNull() ? m_after : m_before;
            const QRect r = imageRect(m_after.size());
            const int divX = r.left() + static_cast<int>(r.width() * m_divider);

            p.drawImage(r, beforeImg);

            QRect afterClip(r.left(), r.top(), divX - r.left(), r.height());
            p.setClipRect(afterClip, Qt::IntersectClip);
            p.drawImage(r, m_after);
            p.setClipPath(canvasPath);

            p.setPen(QPen(kAccent, 2));
            p.drawLine(divX, r.top(), divX, r.bottom());

            const int handleR = 6;
            p.setBrush(kAccent);
            p.drawEllipse(QPoint(divX, r.center().y()), handleR, handleR);
            break;
        }
    }
}

void CompareView::updateDividerFromX(int x) {
    const QRect r = imageRect(m_after.size());
    if (r.width() <= 0)
        return;
    m_divider = std::clamp(double(x - r.left()) / r.width(), 0.0, 1.0);
    update();
}

void CompareView::mousePressEvent(QMouseEvent* event) {
    if (m_mode == Mode::Wipe && event->button() == Qt::LeftButton) {
        m_dragging = true;
        updateDividerFromX(event->pos().x());
    }
}

void CompareView::mouseMoveEvent(QMouseEvent* event) {
    if (m_mode == Mode::Wipe && m_dragging) {
        updateDividerFromX(event->pos().x());
    }
}

void CompareView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        m_dragging = false;
}

void CompareView::wheelEvent(QWheelEvent* event) {
    const double factor = event->angleDelta().y() > 0 ? 1.1 : 1.0 / 1.1;
    m_zoom = std::clamp(m_zoom * factor, 0.2, 8.0);
    update();
}

void CompareView::mouseDoubleClickEvent(QMouseEvent*) {
    m_zoom = 1.0;
    update();
}

}  // namespace pigmenta::desktop
