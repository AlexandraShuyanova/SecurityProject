#include <QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include "echoclient.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QCommandLineParser parser;
	parser.setApplicationDescription("QtWebSockets example: echoclient");
	parser.addHelpOption();

	QCommandLineOption dbgOption(QStringList() << "d" << "debug",
		QCoreApplication::translate("main", "Debug output [default: off]."));
	parser.addOption(dbgOption);
	parser.process(a);
	bool debug = true;

	EchoClient client(QUrl(QStringLiteral("ws://45.135.233.2:8081/ws")), debug);
	QObject::connect(&client, &EchoClient::closed, &a, &QCoreApplication::quit);

	return a.exec();
}
