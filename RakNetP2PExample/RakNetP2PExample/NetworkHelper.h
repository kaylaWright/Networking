#ifndef NETWORKHELPER
#define NETWORKHELPER

#include <string>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "ReadyEvent.h"
#include "SocketLayer.h"
#include "FullyConnectedMesh2.h"
#include "ConnectionGraph2.h"

//
class CardsAgainstHumanity; 

using namespace RakNet;

//extension of the built-in messages that come with Raknet. 
enum GameMessages
{
	ID_DEAL_HAND = ID_USER_PACKET_ENUM + 1,
	ID_DEAL_CARD = ID_USER_PACKET_ENUM + 2,
	ID_CHOOSE_CARD = ID_USER_PACKET_ENUM + 3,
	ID_START_ROUND = ID_USER_PACKET_ENUM + 4,
	ID_RECEIVE_CARD = ID_USER_PACKET_ENUM + 5,
	ID_BROADCAST_MESSAGE = ID_USER_PACKET_ENUM + 6
};

class NetworkHelper
{
private:
	//maximum of eight player game (counting host) at zero index.
	static const unsigned int MAX_CONNECTIONS = 7;
	//minimum of four player game (counting host) from zero index. 
	static const unsigned int MIN_CONNECTIONS = 3;
	//i don't really know what this means. 
	static const unsigned int PORT = 60000;

	bool m_IsHost;
	SystemAddress systemAddresses[MAX_CONNECTIONS];

	RakPeerInterface *m_RakPeer;
	ReadyEvent m_ReadyEventPlugin;

	// These two plugins are just to automatically create a fully connected mesh so I don't have to call connect more than once
	FullyConnectedMesh2 m_FCM2;
	ConnectionGraph2 m_ConnectedGraph2;

	//game vars. 
	CardsAgainstHumanity* currentGame;
protected:
	//
public:
	enum READYEVENTIDs { RE_WAITINGFORPLAYERS = 0, RE_WAITINGFORACTIONS, RE_FINISH };

	NetworkHelper(CardsAgainstHumanity* _game);
	~NetworkHelper() {}

	void Init(bool isHost);
	void Update();
	void Destroy();

	bool EstablishConnection(const char* _connection);
	SystemAddress GetPlayerAddress(int _num);

	//peer messages.
	void SendMessageToPeer(GameMessages _type, SystemAddress _add, std::string _card);
	void BroadcastMessageToPeers(GameMessages _type, std::string _message);

	//ready events
	void SetEvent(READYEVENTIDs eventId, bool isReady);
	void HandleEventAllSet(int eventId);

	//get/sets
	int GetNumConnections();

	bool GetIsHost()
	{	return m_IsHost; }
	void SetIsHost(bool _new)
	{	m_IsHost = _new;	}
};


#endif