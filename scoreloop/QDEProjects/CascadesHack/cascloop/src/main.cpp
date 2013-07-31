// Default empty project template
#include <bb/cascades/Application>

#include <QLocale>
#include <QTranslator>
#include "applicationui.hpp"
#include "template_ndk.hpp"

// include JS Debugger / CS Profiler enabler
// this feature is enabled by default in the debug build only
#include <Qt/qdeclarativedebug.h>

using namespace bb::cascades;

Q_DECL_EXPORT int main(int argc, char **argv)
{
	int rc = 0;

    // this is where the server is started etc
    Application app(argc, argv);

    init();
    start();

    userget();

    // localization support
    QTranslator translator;
    QString locale_string = QLocale().name();
    QString filename = QString( "cascloop_%1" ).arg( locale_string );
    if (translator.load(filename, "app/native/qm")) {
        app.installTranslator( &translator );
    }

    new ApplicationUI(&app);

    // we complete the transaction started in the app constructor and start the client event loop here
    rc = Application::exec();
    // when loop is exited the Application deletes the scene which deletes all its children (per qt rules for children)
    stop();
    kill();

    return rc;
}
