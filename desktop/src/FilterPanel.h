#pragma once

#include <QWidget>
#include <QMap>
#include "Engine.h"

class QListWidget;
class QFormLayout;
class QVBoxLayout;
class QLabel;

namespace pigmenta::desktop {

// Filter list + dynamically-built parameter controls for the selected filter.
class FilterPanel : public QWidget {
    Q_OBJECT
public:
    explicit FilterPanel(QWidget* parent = nullptr);

    FilterId currentFilter() const { return m_currentId; }
    QMap<QString, double> currentValues() const { return m_values; }

signals:
    // Emitted whenever the selected filter or any of its parameters change.
    void parametersChanged();

private:
    void rebuildParamsForm();
    void clearParamsForm();
    void retranslate();

    QListWidget* m_list = nullptr;
    QWidget* m_paramsContainer = nullptr;
    QFormLayout* m_paramsForm = nullptr;
    QLabel* m_filtersTitle = nullptr;
    QLabel* m_paramsTitle = nullptr;

    FilterId m_currentId = FilterId::Original;
    QMap<QString, double> m_values;
};

}  // namespace pigmenta::desktop
