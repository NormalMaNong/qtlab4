#include "chatserver.h"
#include "serverworker.h"
#include <QJsonValue>
#include <QJsonObject>

ChatServer::ChatServer(QObject *parent):QTcpServer(parent) {}

void ChatServer::incomingConnection(qintptr socketDescriptor)
{
    ServerWorker *worker = new ServerWorker(this);
    if(!worker->setSocketDescriptor(socketDescriptor)){
        worker->deleteLater();
        return;
    }
    connect(worker, &ServerWorker::logMessage, this, &ChatServer::logMessage);
    connect(worker, &ServerWorker::jsonReceived, this, &ChatServer::jsonReceived);
    m_clients.append(worker);
    emit logMessage("新用户连接上了");
}

void ChatServer::broadcast(const QJsonObject &message, ServerWorker *exclude)
{
    for(ServerWorker *worker : m_clients){
        worker->sendJson(message);
    }
}

void ChatServer::stopServer()
{
    close();
}

void ChatServer::jsonReceived(ServerWorker *sender, const QJsonObject &docObj)
{
    const QJsonValue typeVal = docObj.value("type");
    if(typeVal.isNull() || !typeVal.isString())
        return;
    if(typeVal.toString().compare("message", Qt::CaseInsensitive) == 0){
        const QJsonValue textVal = docObj.value("text");
        if(textVal.isNull() || !textVal.isString())
            return;
        const QString text = textVal.toString().trimmed();
        if(text.isEmpty())
            return;
        QJsonObject message;
        message[QStringLiteral("type")] = "message";
        message[QStringLiteral("text")] = text;
        message[QStringLiteral("sender")] = sender->userName();

        broadcast(message, sender);
    } else if(typeVal.toString().compare("登录") == 0){
        const QJsonValue userNameVal = docObj.value("text");
        if(userNameVal.isNull() || !userNameVal.isString())
            return;
        sender->setUserName(userNameVal.toString());
        QJsonObject connectedMessage;
        connectedMessage[QStringLiteral("type")] = "newuser";
        connectedMessage[QStringLiteral("username")] = userNameVal.toString();

        broadcast(connectedMessage, sender);
    }
}


