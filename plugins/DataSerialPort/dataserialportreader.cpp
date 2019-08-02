#include <math.h>
#include <QByteArray>
#include <QDebug>
#include <QErrorMessage>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QSerialPort>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <mutex>
#include <thread>
#include "dataserialportreader.h"

DataSerialPortReader::DataSerialPortReader() : running_(false), serial_port_(), namespace_("")
{
}

DataSerialPortReader::~DataSerialPortReader()
{
  shutdown();
}

bool DataSerialPortReader::start(QStringList*)
{
  running_ = true;
  bool ok;
  QString port("/dev/ttyS0");
  port = QInputDialog::getText(nullptr, tr(""), tr("Enter serial port:"), QLineEdit::Normal, port, &ok);
  running_ &= ok;
  ok = false;
  int baudrate = 115200;
  baudrate = QInputDialog::getInt(nullptr, tr(""), tr("Enter baudrate:"), baudrate, 0, 115200, 100, &ok);
  running_ &= ok;
  ok = false;
  namespace_ =
	  QInputDialog::getText(nullptr, tr(""), tr("Enter namespace (or leave blank):"), QLineEdit::Normal, "", &ok);
  running_ &= ok;
  if (running_)
  {
	QErrorMessage error_msg;
	serial_port_.setPortName(port);

	if (serial_port_.setBaudRate(baudrate))
	{
	  qDebug() << "Baudrate set to " << baudrate;
	  if (serial_port_.open(QIODevice::ReadOnly))
	  {
		qDebug() << "Serial port reading on " << port;
		connect(&serial_port_, &QSerialPort::readyRead, this, &DataSerialPortReader::handleReadyRead);
		connect(&serial_port_, &QSerialPort::errorOccurred, this, &DataSerialPortReader::handleError);
	  }
	  else
	  {
		qDebug() << "Couldn't open serial port " << serial_port_.portName();
		running_ = false;
		error_msg.showMessage("Couldn't open the serial port " + serial_port_.portName());
		error_msg.exec();
	  }
	}
	else
	{
	  qDebug() << "Baudrate couldn't be set to " << baudrate;
	  running_ = false;
	  error_msg.showMessage("Baudrate couldn't be set to " + baudrate);
	}
  }
  return running_;
}

void DataSerialPortReader::shutdown()
{
  if (running_)
  {
	serial_port_.close();
	remaining_data_.clear();
	running_ = false;
	qDebug() << "Shutdown Serial port";
  }
}

void DataSerialPortReader::handleReadyRead()
{
  std::lock_guard<std::mutex> lock(mutex());

  QByteArray data = serial_port_.readAll();
  data.prepend(remaining_data_);
  QString s_data(data);
  QStringList lst = s_data.split('\n');

  if (s_data.endsWith("\n"))
  {
	remaining_data_.clear();
  }
  else
  {
	remaining_data_ = QByteArray(lst.last().toStdString().c_str(), lst.last().size());
	lst.removeLast();
  }

  for (auto&& line : lst)
  {
	bool start_with_namespace = line.startsWith(namespace_);
	if (start_with_namespace)
	{
	  QStringList line_list = line.split(':');
	  if (line_list.size() == 3 && namespace_.compare("") || line_list.size() == 4)
	  {
		if (namespace_.compare("") != 0)
		{
		  line_list.removeAt(0);
		}
		QString key = line_list.at(0);
		double time = line_list.at(1).toDouble();
		double value = line_list.at(2).toDouble();

		auto& numeric_plots = dataMap().numeric;
		const std::string name_str = key.toStdString();
		auto plotIt = numeric_plots.find(name_str);

		if (plotIt == numeric_plots.end())
		{
		  plotIt = dataMap().addNumeric(name_str);
		}
		plotIt->second.pushBack({ time, value });
	  }
	}
  }
}
void DataSerialPortReader::handleError(QSerialPort::SerialPortError serialPortError)
{
  if (serialPortError == QSerialPort::ReadError)
  {
	qDebug() << "An I/O error occurred while reading, the data from port" << serial_port_.portName()
			 << "error: " << serial_port_.errorString();
  }
}
