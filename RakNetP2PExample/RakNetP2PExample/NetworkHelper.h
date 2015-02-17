#ifndef NETWORKHELPER
#define NETWORKHELPER

#include <string>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "ReadyEvent.h"
#include "SocketLayer.h"
#include "FullyConnectedMesh2.h"
#include "ConnectionGraph2.h"

using namespace RakNet;

class NetworkHelper
{
public:
	enum READYEVENTIDs { RE_START = 0, RE_WAITINGFORPLAYERS, RE_WAITINGFORACTIONS, RE_FINISH };

	NetworkHelper();
	~NetworkHelper() {}

	void Init(bool isHost);
	void Destroy();
	bool EstablishConnection(const char* _connection);
	void Update();

	void SetIsMaster(bool isMaster) { m_IsMaster = isMaster; }
	bool IsMaster();

	void SetEvent(READYEVENTIDs eventId, bool isReady);

	void HandleEventAllSet(int eventId);

	//ask clients to connect
	//wait for connections
private:

	static const unsigned int MAX_CONNECTIONS = 8;
	static const unsigned int PORT = 60000;

	bool m_IsMaster;
	bool m_IsHost;

	RakPeerInterface *m_RakPeer;
	ReadyEvent m_ReadyEventPlugin;

	// These two plugins are just to automatically create a fully connected mesh so I don't have to call connect more than once
	FullyConnectedMesh2 m_FCM2;
	ConnectionGraph2 m_ConnectedGraph2;
protected:
	//a container of connected clients

};


#endif