#ifndef ECHOCLIENT_H
#define ECHOCLIENT_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <string>
#include "qjsonobject.h"
#include "qjsondocument.h"

class EchoClient : public QObject
{
	Q_OBJECT
public:
	explicit EchoClient(const QUrl &url, bool debug = false, QObject *parent = Q_NULLPTR);

Q_SIGNALS:
	void closed();

private Q_SLOTS:
	void onConnected();
	void onTextMessageReceived(QString message);

private:
	QWebSocket m_webSocket;
	QUrl m_url;
	bool m_debug;
	QString infoBios = "";
	void sendInfo();
	int getInfoBIOS();
	std::string getVolumeSerialNumber(const QString& drive);
	QJsonObject fromStringToJsonObject(const QString& out);
	QJsonArray EchoClient::fromListToJsonArray(const QStringList &volumesList);
	std::string executeCommand(const char* cmd);
};

#endif // ECHOCLIENT_H