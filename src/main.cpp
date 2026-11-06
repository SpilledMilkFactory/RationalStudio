#include "gui/MainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[]) {

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(
        "RationalStudio"
    );

    QCoreApplication::setApplicationName(
        "RationalStudio"
    );

    QCoreApplication::setApplicationVersion(QStringLiteral(RATIONALSTUDIO_VERSION));

    rationalstudio::MainWindow window;

    window.show();

    return app.exec();
}
