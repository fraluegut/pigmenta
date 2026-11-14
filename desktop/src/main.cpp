#include <QApplication>

#include "MainWindow.h"
#include "Style.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("Pigmenta");
    app.setApplicationName("Pigmenta Desktop");
    app.setApplicationVersion(PIGMENTA_VERSION);
    app.setStyleSheet(pigmenta::desktop::kStyleSheet);

    pigmenta::desktop::MainWindow window;
    window.show();

    return app.exec();
}
