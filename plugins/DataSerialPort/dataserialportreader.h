#ifndef DATASERIALPORTREADER_h
#define DATASERIALPORTREADER_h

#include <QSerialPort>

#include <QString>
#include <QtPlugin>
#include <QByteArray>
#include "PlotJuggler/datastreamer_base.h"


class  DataSerialPortReader: public DataStreamer
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "plotjugglerplugins.DataSerialPortReader")
    Q_INTERFACES(DataStreamer)

public:

    DataSerialPortReader();

    virtual bool start() override;

    virtual void shutdown() override;

    virtual bool isRunning() const override { return running_; }

    virtual ~DataSerialPortReader();

    virtual const char* name() const override { return "SerialPortReader"; }

    virtual bool isDebugPlugin() override { return false; }

private:
    QSerialPort serial_port_;
    QString namespace_;
    QByteArray remaining_data_;

    bool running_;

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
};

#endif // DATASERIALPORTREADER_H
