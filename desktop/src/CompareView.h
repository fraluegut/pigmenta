#pragma once

#include <QWidget>
#include <QPixmap>

namespace pigmenta::desktop {

class CompareView : public QWidget {
    Q_OBJECT
public:
    enum class Mode { After, Before, Wipe, SideBySide };

    explicit CompareView(QWidget* parent = nullptr);

    void setImages(const QImage& before, const QImage& after);
    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QRect imageRect(const QSize& imgSize) const;
    void updateDividerFromX(int x);

    QImage m_before;
    QImage m_after;
    Mode m_mode = Mode::After;
    double m_divider = 0.5;  // 0..1, used by Wipe mode
    bool m_dragging = false;
    double m_zoom = 1.0;
};

}  // namespace pigmenta::desktop
