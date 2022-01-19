#include "echoclient.h"
#include <QDebug>
#include "windows.h"
#include "TCHAR.h"
#include "windef.h"
#include <QTimer>
#include "qvector.h"
#include <stdio.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <istream>
#include <sys/stat.h>
#include <iomanip>
#include <QDir>
#include <string>
#include "qtextcodec.h"
#include "qjsonobject.h"
#include "qjsondocument.h"
#include "qjsonarray.h"
#include "smbios.h"
#include "smbios_decode.h"
#include <sstream>

using namespace std;

QT_USE_NAMESPACE

#pragma comment(lib, "pdh.lib")
static float CalculateCPULoad();
static unsigned long long FileTimeToInt64();
float GetCPULoad();


EchoClient::EchoClient(const QUrl &url, bool debug, QObject *parent) :
	QObject(parent),
	m_url(url),
	m_debug(debug)
{
	if (m_debug)
		qDebug() << "WebSocket server:" << url;
	//connect(&m_webSocket, &QWebSocket::connected, this, &EchoClient::onConnected);
	onConnected();
	connect(&m_webSocket, &QWebSocket::disconnected, this, &EchoClient::closed);
	m_webSocket.open(QUrl(url));
}

void EchoClient::onConnected()
{
	if (m_debug)
		qDebug() << "WebSocket connected";
	connect(&m_webSocket, &QWebSocket::textMessageReceived,
		this, &EchoClient::onTextMessageReceived);

	QStringList volumeSerialNumbers;
	for(const QFileInfo& drive : QDir::drives())
	{
		volumeSerialNumbers.append(QString::fromStdString(getVolumeSerialNumber(drive.absolutePath())));
	}
	for (QString& number : volumeSerialNumbers)
	{
		number = QString::number(number.toLongLong(), 16);
	}
	getInfoBIOS();
	
	QJsonObject mainObject;
	QJsonObject deviceObject;
	deviceObject.insert("os", QSysInfo::prettyProductName());
	deviceObject.insert("volumes", fromListToJsonArray(volumeSerialNumbers));
	mainObject.insert("method", "auth");
	mainObject.insert("device", deviceObject);
	mainObject.insert("smbios", fromStringToJsonObject(infoBios));

	QJsonDocument doc(mainObject);
	qDebug().noquote() << doc.toJson(QJsonDocument::Indented);
	QString strJson(doc.toJson(QJsonDocument::Compact));
	
	QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
	QString cyrillicName = codec->toUnicode(executeCommand("dir").c_str());

	qDebug() << cyrillicName << endl;
	
	/*QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &EchoClient::sendInfo);
	timer->start(1000);	*/
}

std::string EchoClient::executeCommand(const char* cmd) {
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	char buffer[128];
	std::string result = "";
	std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) throw std::runtime_error("popen() failed!");

	while (!feof(pipe.get())) {
		if (fgets(buffer, 128, pipe.get()) != NULL)
			result += buffer;
	}
	return result;
}

std::string EchoClient::getVolumeSerialNumber(const QString& drive)
{
	DWORD VolumeSerialNumber = 0;
	GetVolumeInformation(drive.toStdWString().c_str(), NULL, NULL, &VolumeSerialNumber, NULL, NULL, NULL, NULL);
	wstring wstr = std::to_wstring(VolumeSerialNumber);
	std::string str = std::string(wstr.begin(), wstr.end());
	return str;
}
void EchoClient::sendInfo()
{
	
	/*MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;
	DWORDLONG virtualMemUsed = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
	DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
	DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;

	m_webSocket.sendTextMessage("Total Virtual Memory: " + QString(std::to_string(totalVirtualMem / 1024 / 1024).c_str()) + "Mb");
	m_webSocket.sendTextMessage("Virtual Memory Currently Used: " + QString(std::to_string(virtualMemUsed / 1024 / 1024).c_str()) + "Mb");
	m_webSocket.sendTextMessage("Total Physical Memory: " + QString(std::to_string(totalPhysMem / 1024 / 1024).c_str()) + "Mb");
	m_webSocket.sendTextMessage("Physical Memory Currently Used: " + QString(std::to_string(physMemUsed / 1024 / 1024).c_str()) + "Mb");
	m_webSocket.sendTextMessage("CPU Currently Used: " + QString(std::to_string(GetCPULoad() * 100).c_str()));*/
}

int EchoClient::getInfoBIOS()
{
	std::vector<uint8_t> buffer;
	bool result = false;

#ifdef _WIN32

	result = getDMI(buffer);

#else

	const char *path = "/sys/firmware/dmi/tables";
	if (argc == 2) path = argv[1];
	std::cerr << "Using SMBIOS tables from " << path << std::endl;
	result = getDMI(path, buffer);

#endif

	if (!result)
	{
		std::cerr << "Unable to open SMBIOS tables" << std::endl;
		return 1;
	}

	smbios::Parser parser(buffer.data(), buffer.size());
	if (parser.valid())
	{
		std::stringstream stringStream;
		printSMBIOS(parser, stringStream);
		infoBios = QString::fromStdString(stringStream.str());
		//qDebug() << infoBios << endl;
		return 0;
		
	}
	else
		std::cerr << "Invalid SMBIOS data" << std::endl;
	return 1;
}

QJsonObject EchoClient::fromStringToJsonObject(const QString &out)
{
	QJsonObject recordObject;
	QJsonArray recordArray;
	QStringList elemList;
	QStringList outList = out.split(QLatin1Char('\n'), QString::SkipEmptyParts);
	
	QStringList sections = {"bios", "sysinfo", "baseboard",
							"sysenclosure", "processor", "sysslot",
							"physmem", "memory", "oemstrings"};
	
	for (const auto& section : sections) {
		recordObject.insert(section, QJsonArray());
	}

	for(const QString& elem : outList)
	{
		elemList = elem.split(':');
		if (elemList.length() > 1)
		{
			QString value = elemList.mid(1).join(':');
			
			int sectionIndex = -1;
			int valueIndex = 0;
			
			for (int i = 0; i < sections.length(); i++) {
				if (elemList[0].contains(sections[i])) {
					sectionIndex = i;
				}
			}
			
			if (sectionIndex == -1) {
				break;
			}

			QString key = elemList[0].replace("[" + sections[sectionIndex] + "] " , "");
			
			QJsonArray array = recordObject[sections[sectionIndex]].toArray();
			
			while (array.size() <= valueIndex || array[valueIndex].toObject().contains(key)) {
				if (array.size() <= valueIndex) {
					array.push_back(QJsonObject());
				} else {
					valueIndex++;
				}
			}
			
			QJsonObject jsonObject = array[valueIndex].toObject();
			jsonObject.insert(key, value);
			array[valueIndex] = jsonObject;
			recordObject.insert(sections[sectionIndex], array);
		}	
		else
		{
			//recordArray.push_back(elemList[0]);
			//recordObject.insert("Some data", recordArray);
		}
	}
	
	return recordObject;
}

QJsonArray EchoClient::fromListToJsonArray(const QStringList &volumesList)
{
	QJsonArray recordArray;
	
	for (QString volumeNum : volumesList)
	{
		recordArray.push_back(volumeNum);
	}

	return recordArray;
}

void EchoClient::onTextMessageReceived(QString message)
{
	if (m_debug)
		qDebug() << "Message received:" << message;
	//m_webSocket.close();
}

static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
	static unsigned long long _previousTotalTicks = 0;
	static unsigned long long _previousIdleTicks = 0;

	unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
	unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;


	float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

	_previousTotalTicks = totalTicks;
	_previousIdleTicks = idleTicks;
	return ret;
}

static unsigned long long FileTimeToInt64(const FILETIME & ft)
{
	return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime);
}

// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.  Returns -1.0 on error.
float GetCPULoad()
{
	FILETIME idleTime, kernelTime, userTime;
	return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
}
