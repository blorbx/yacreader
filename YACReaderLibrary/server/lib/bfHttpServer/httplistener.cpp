/**
  @file
  @author Stefan Frings
*/

#include "httplistener.h"
#include "httpconnectionhandler.h"
#include "httpconnectionhandlerpool.h"
#include <QCoreApplication>

HttpListener::HttpListener(QSettings* settings, HttpRequestHandler* requestHandler, QObject *parent)
    : QTcpServer(parent)
{
    Q_ASSERT(settings!=0);
    // Reqister type of socketDescriptor for signal/slot handling
    qRegisterMetaType<tSocketDescriptor>("tSocketDescriptor");
    // Create connection handler pool
    this->settings=settings;
    pool=new HttpConnectionHandlerPool(settings,requestHandler);
    // Start listening
    int port=settings->value("port",8080).toInt();
    listen(QHostAddress::Any, port);
	//Cambiado
	int i = 0;
    while (!isListening() && i < 1000) {
        listen(QHostAddress::Any, (rand() % 45535)+20000);
		i++;
    }
	if(!isListening())
	{
		qCritical("HttpListener: Cannot bind on port %i: %s",port,qPrintable(errorString()));
	}
    else {
        qDebug("HttpListener: Listening on port %i",port);
    }
}

HttpListener::~HttpListener() {
    close();
    qDebug("HttpListener: closed");
    delete pool;
    qDebug("HttpListener: destroyed");
}

void HttpListener::incomingConnection(tSocketDescriptor socketDescriptor) {
#ifdef SUPERVERBOSE
    qDebug("HttpListener: New connection");
#endif
    HttpConnectionHandler* freeHandler=pool->getConnectionHandler();

    // Let the handler process the new connection.
    if (freeHandler) {
        // The descriptor is passed via signal/slot because the handler lives in another
        // thread and cannot open the socket when called by another thread.
        connect(this,SIGNAL(handleConnection(tSocketDescriptor)),freeHandler,SLOT(handleConnection(tSocketDescriptor)));
        emit handleConnection(socketDescriptor);
        disconnect(this,SIGNAL(handleConnection(tSocketDescriptor)),freeHandler,SLOT(handleConnection(tSocketDescriptor)));
    }
    else {
        // Reject the connection
        qDebug("HttpListener: Too many connections");
        QTcpSocket* socket=new QTcpSocket(this);
        socket->setSocketDescriptor(socketDescriptor);
        connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
        socket->write("HTTP/1.1 503 too many connections\r\nConnection: close\r\n\r\nToo many connections\r\n");
        socket->disconnectFromHost();
    }
}
