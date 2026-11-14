#pragma once

namespace pigmenta::desktop {

// Obsidian-inspired dark theme: warm neutral charcoal, floating rounded
// panels with a visible gap between them, soft low-contrast borders, amber
// accent (matches the app logo) instead of Obsidian's purple.
inline const char* kStyleSheet = R"(
QWidget {
    color: #e8e3dc;
    font-family: "Inter", "Segoe UI", "Helvetica Neue", sans-serif;
    font-size: 11px;
}

QDialog {
    background-color: #141210;
}

QLabel, QCheckBox, QToolButton {
    background-color: transparent;
}

#windowCard {
    background-color: #1c1917;
    border: 1px solid #2c2722;
    border-radius: 12px;
}
#windowCard[maximized="true"] {
    border-radius: 0px;
    border: none;
}

#titleBar {
    background-color: transparent;
}
#titleBrand {
    color: #c2bbb1;
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 1.5px;
}
#winMin, #winMax, #winClose {
    border: none;
    border-radius: 0px;
    background-color: transparent;
    color: #8a8178;
    font-size: 13px;
}
#winMin:hover, #winMax:hover {
    background-color: #2c2722;
    color: #e8e3dc;
}
#winClose:hover {
    background-color: #c0392b;
    color: #ffffff;
}

#sidebar {
    background-color: #191613;
    border: none;
}
#viewer {
    background-color: #161312;
    border: none;
}

#appMenuBar {
    background-color: transparent;
    border: none;
    padding: 2px 10px 6px 10px;
}
QMenuBar {
    background-color: transparent;
    border: none;
    padding: 3px 6px;
}
QMenuBar::item {
    padding: 4px 10px;
    border-radius: 6px;
    background: transparent;
}
QMenuBar::item:selected {
    background-color: #2c2722;
    color: #e8a33d;
}
QMenu {
    background-color: #1c1917;
    border: 1px solid #242019;
    border-radius: 8px;
    padding: 6px;
}
QMenu::item {
    padding: 6px 24px 6px 12px;
    border-radius: 6px;
}
QMenu::item:selected {
    background-color: #2c2722;
    color: #e8a33d;
}
QMenu::separator {
    height: 1px;
    background: #2c2722;
    margin: 6px 6px;
}

#versionLabel {
    color: #5c544e;
    font-size: 10px;
    padding-right: 6px;
}

#brand {
    color: #e8a33d;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 2px;
}

#sectionTitle {
    color: #8a8178;
    font-size: 9px;
    font-weight: 600;
    letter-spacing: 1.5px;
    margin-top: 4px;
}

#dropArea {
    border: 1.5px dashed #3a332c;
    border-radius: 12px;
    color: #8a8178;
    padding: 8px;
    background-color: #201c19;
}
#dropArea:hover {
    border-color: #e8a33d;
    color: #e8a33d;
    background-color: #241f1a;
}

#filterList {
    background-color: #161312;
    border: 1px solid #242019;
    border-radius: 10px;
    outline: none;
    padding: 4px;
}
#filterList::item {
    padding: 7px 10px;
    border-radius: 7px;
    margin: 1px 0px;
}
#filterList::item:hover {
    background-color: #221d19;
}
#filterList::item:selected {
    background-color: #2c2722;
    color: #e8a33d;
}

#hairline {
    background-color: #2c2722;
    max-height: 1px;
    border: none;
}

#paramValue {
    color: #e8a33d;
    font-size: 10px;
}

QSlider::groove:horizontal {
    height: 3px;
    background: #2c2722;
    border-radius: 1.5px;
}
QSlider::handle:horizontal {
    width: 12px;
    height: 12px;
    margin: -5px 0;
    background: #e8a33d;
    border-radius: 6px;
}

QComboBox {
    background-color: #161312;
    border: 1px solid #242019;
    border-radius: 7px;
    padding: 4px 8px;
}
QComboBox::drop-down {
    border: none;
}
QComboBox QAbstractItemView {
    background-color: #1c1917;
    border: 1px solid #242019;
    border-radius: 8px;
    outline: none;
    selection-background-color: #2c2722;
}

QPushButton {
    background-color: #221d19;
    border: 1px solid #242019;
    border-radius: 8px;
    padding: 6px 12px;
    color: #e8e3dc;
}
QPushButton:hover {
    border-color: #e8a33d;
    color: #e8a33d;
}
QPushButton:checked {
    background-color: #e8a33d;
    color: #141210;
    border-color: #e8a33d;
    font-weight: 600;
}

#exportButton {
    background-color: #e8a33d;
    color: #141210;
    font-weight: 600;
    border-radius: 8px;
}
#exportButton:disabled {
    background-color: #221d19;
    color: #5c544e;
    border-color: #2c2722;
}

#langButton {
    padding: 0px;
    font-size: 11px;
    font-weight: 600;
    border-radius: 7px;
}

#modeButton {
    padding: 5px 12px;
    font-size: 10px;
    border-radius: 7px;
}

#statusLabel {
    color: #8a8178;
    font-size: 10px;
}

#welcomeSubtitle {
    color: #8a8178;
    font-size: 10px;
}
#welcomeBlurb {
    color: #c2bbb1;
    line-height: 1.4;
}
#sampleButton {
    background-color: #161312;
    border: 1px solid #242019;
    border-radius: 10px;
    padding: 6px;
    color: #c2bbb1;
    font-size: 9px;
}
#sampleButton:hover {
    border-color: #e8a33d;
    color: #e8a33d;
    background-color: #201c19;
}

QTextBrowser {
    background-color: #1c1917;
    border: none;
    border-radius: 10px;
    padding: 20px 24px;
    color: #e8e3dc;
}
QTextBrowser a { color: #e8a33d; }

QLabel { font-size: 11px; }

QScrollBar:vertical {
    background: transparent;
    width: 9px;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background: #3a332c;
    border-radius: 4px;
    min-height: 24px;
}
QScrollBar::handle:vertical:hover {
    background: #4a423a;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}
)";

}  // namespace pigmenta::desktop
