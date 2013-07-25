
#include <ActionServer.hpp>
#include <Connection.hpp>

#include <QCoreApplication>

using namespace SphereSim;

/*
 * method main:
 * Creates instance of ActionServer to listen to incoming client connections.
 * Starts QCoreApplication.
 */

int main(int argc, char** argv){
	QCoreApplication app(argc, argv);
	ActionServer actSvr(Connection::defaultAddress, Connection::defaultPort);
	app.exec();
	
	return 0;
}
