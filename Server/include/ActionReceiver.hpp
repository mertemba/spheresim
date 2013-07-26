
#ifndef _ACTIONRECEIVER_HPP_
#define _ACTIONRECEIVER_HPP_

#include <Actions.hpp>

#include <QObject>

class QTcpSocket;
class QHostAddress;
class SphereManager;

namespace SphereSim{
	
	/*
	 * class ActionReceiver:
	 * Takes client socket and replies to client requests.
	 */
	class ActionReceiver:QObject{
		Q_OBJECT
		
	private:
		QTcpSocket* socket;
		
		/* QByteArray requestData:
		 * Collected data from a client request. */
		QByteArray requestData;
		
		/* bool collectingRequestData:
		 * Flag if currently data from a client request is being collected.
		 * If true, no new requests are accepted. */
		bool collectingRequestData;
		
		/* method processData:
		 * Processes received request data. */
		void processData(QByteArray arr);
		
		/* method processRequest:
		 * Processes and answers received request. */
		void processRequest();
		
		/* method handleAction:
		 * Handles any action request. */
		void handleAction(const char actionGroup, const char action, const QByteArray data);
		
		/* method handleBasicAction:
		 * Handles basic action requests. */
		void handleBasicAction(const char actionGroup, const char action, const QByteArray data);
		
		/* method handleUnknownAction:
		 * Handles unknown action requests. */
		void handleUnknownAction(const char actionGroup, const char action, const QByteArray data);
		
		/* method sendReply:
		 * Sends encoded reply to client. */
		void sendReply(const QByteArray& data);
		
		/* SphereManager sphMan:
		 * Stores sphere data. */
		SphereManager* sphMan;
		
	public:
		/* constructor:
		 * Takes client socket. */
		ActionReceiver(QTcpSocket* sock);
		
	public slots:
		/* slot readData:
		 * Reads data from client. */
		void readData();
		
		/* slot disconnected:
		 * Executed when connection was released. */
		void disconnected();
	};
	
}

#endif
