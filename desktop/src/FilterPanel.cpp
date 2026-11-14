#include "FilterPanel.h"

#include <QListWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QFrame>
#include <cmath>
#include <algorithm>
#include "Translator.h"

namespace pigmenta::desktop {

FilterPanel::FilterPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_filtersTitle = new QLabel(Translator::t("filterPanel.filters"));
    m_filtersTitle->setObjectName("sectionTitle");
    layout->addWidget(m_filtersTitle);

    m_list = new QListWidget();
    m_list->setObjectName("filterList");
    for (const FilterSpec& spec : filterSpecs()) {
        m_list->addItem(spec.name);
    }
    m_list->setCurrentRow(0);
    m_list->setMaximumHeight(170);
    layout->addWidget(m_list);

    auto* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("hairline");
    layout->addWidget(sep);

    m_paramsTitle = new QLabel(Translator::t("filterPanel.parameters"));
    m_paramsTitle->setObjectName("sectionTitle");
    layout->addWidget(m_paramsTitle);

    m_paramsContainer = new QWidget();
    m_paramsForm = new QFormLayout(m_paramsContainer);
    m_paramsForm->setSpacing(8);
    m_paramsForm->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_paramsContainer);
    layout->addStretch(1);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0)
            return;
        m_currentId = filterSpecs()[static_cast<size_t>(row)].id;
        rebuildParamsForm();
        emit parametersChanged();
    });

    rebuildParamsForm();

    connect(&Translator::instance(), &Translator::languageChanged, this, &FilterPanel::retranslate);
}

void FilterPanel::retranslate() {
    m_filtersTitle->setText(Translator::t("filterPanel.filters"));
    m_paramsTitle->setText(Translator::t("filterPanel.parameters"));

    const int row = m_list->currentRow();
    m_list->blockSignals(true);
    m_list->clear();
    for (const FilterSpec& spec : filterSpecs())
        m_list->addItem(spec.name);
    m_list->setCurrentRow(row);
    m_list->blockSignals(false);

    rebuildParamsForm();
}

void FilterPanel::clearParamsForm() {
    while (m_paramsForm->rowCount() > 0) {
        m_paramsForm->removeRow(0);
    }
}

void FilterPanel::rebuildParamsForm() {
    clearParamsForm();
    m_values.clear();

    const FilterSpec* spec = nullptr;
    for (const FilterSpec& s : filterSpecs()) {
        if (s.id == m_currentId) {
            spec = &s;
            break;
        }
    }
    if (!spec)
        return;

    for (const ParamSpec& p : spec->params) {
        m_values[p.name] = p.defaultValue;

        if (p.kind == ParamKind::Choice) {
            auto* combo = new QComboBox();
            combo->addItems(p.choices);
            combo->setCurrentIndex(static_cast<int>(p.defaultValue));
            connect(combo, &QComboBox::currentIndexChanged, this, [this, name = p.name](int idx) {
                m_values[name] = idx;
                emit parametersChanged();
            });
            m_paramsForm->addRow(p.label, combo);
            continue;
        }

        const int steps = std::max(1, static_cast<int>(std::round((p.max - p.min) / p.step)));
        auto* row = new QWidget();
        auto* rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(2);

        auto* valueLabel = new QLabel(QString::number(p.defaultValue, 'f', 2));
        valueLabel->setObjectName("paramValue");

        auto* slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, steps);
        const int defaultStep = static_cast<int>(std::round((p.defaultValue - p.min) / p.step));
        slider->setValue(defaultStep);

        connect(slider,
                &QSlider::valueChanged,
                this,
                [this, valueLabel, name = p.name, lo = p.min, step = p.step](int idx) {
                    const double value = lo + idx * step;
                    m_values[name] = value;
                    valueLabel->setText(QString::number(value, 'f', 2));
                    emit parametersChanged();
                });

        rowLayout->addWidget(slider);
        rowLayout->addWidget(valueLabel);
        m_paramsForm->addRow(p.label, row);
    }
}

}  // namespace pigmenta::desktop
