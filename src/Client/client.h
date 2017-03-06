#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTimer>

#include "../includes/config.h"
#include "../includes/fileserver.h"
#include "../includes/fileclient.h"

class Client : public QObject
{
    Q_OBJECT
public:
    Client();
    ~Client();

signals:
    void exSignal(QString);

private slots:
    bool getOnline();
    void getNewFile(const QString& path, const QString& ip);

private:
    void update();
    void getNewConfig(const QString& path);


    QString name;
    QTimer* onlineTimer;
    QTimer* screenTimer;
    Config* config;
    FileServer* fileServer;
    FileClient* fileClient;
};

#endif // CLIENT_H
