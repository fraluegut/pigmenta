#pragma once

#include <QMainWindow>
#include <QImage>
#include <QMap>
#include "CompareView.h"

class QPushButton;
class QLabel;
class QAction;
class QMenuBar;
class QWidget;
class QVBoxLayout;

namespace pigmenta::desktop {

class FilterPanel;
class DropArea;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void loadImage(const QString& path);
    void reprocess();
    void exportImage();
    void setViewMode(CompareView::Mode mode);
    void openFileDialog();
    void showWelcome(bool isStartup);

private:
    QMenuBar* buildMenuBar();
    QWidget* buildStatusRow();
    void buildUi();
    void updateMaximizedVisual();
    void retranslate();

    FilterPanel* m_filterPanel = nullptr;
    DropArea* m_dropArea = nullptr;
    CompareView* m_compareView = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_exportButton = nullptr;

    QMap<int, QPushButton*> m_modeButtons;
    QMap<int, QAction*> m_modeActions;
    QWidget* m_windowCard = nullptr;
    QMenuBar* m_menuBar = nullptr;
    QVBoxLayout* m_cardLayout = nullptr;
    bool m_hasImage = false;

    QImage m_original;
    QImage m_processed;
    QString m_currentPath;
};

}  // namespace pigmenta::desktop
