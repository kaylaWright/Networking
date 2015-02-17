#include "NetworkHelper.h"
#include "BitStream.h"
#include <iostream>

NetworkHelper::NetworkHelper() : m_RakPeer(0), m_IsHost(false) ,m_IsMaster(false)
{
}

void NetworkHelper::Init(bool isHost)
{
	m_RakPeer = RakPeerInterface::GetInstance();
	m_IsHost = isHost;
	
	m_RakPeer->AttachPlugin(&m_ReadyEventPlugin);
	m_RakPeer->AttachPlugin(&m_FCM2);
	m_RakPeer->AttachPlugin(&m_ConnectedGraph2);

	m_RakPeer->SetMaximumIncomingConnections(MAX_CONNECTIONS);
	
	m_FCM2.SetAutoparticipateConnections(true);
	m_FCM2.SetConnectOnNewRemoteConnection(true, "");
	m_ConnectedGraph2.SetAutoProcessNewConnections(true);

	SocketDescriptor sd(PORT,0);
	while (IRNS2_Berkley::IsPortInUse(sd.port, sd.hostAddress, sd.socketFamily,SOCK_DGRAM)==true)
		sd.port++;

	StartupResult sr = m_RakPeer->Startup(MAX_CONNECTIONS, &sd, 1);
	RakAssert(sr==RAKNET_STARTED);
}

void NetworkHelper::Destroy()
{
	if(m_RakPeer)
		RakPeerInterface::DestroyInstance(m_RakPeer);
}

bool NetworkHelper::EstablishConnection(const char* _connection)
{
	std::cout << _connection << std::endl;
	ConnectionAttemptResult car = m_RakPeer->Connect(_connection, PORT, 0, 0, 0);
	if(car==CONNECTION_ATTEMPT_STARTED)
	{ 
		return true;
	}
	
	return false;
}

void NetworkHelper::SetEvent(READYEVENTIDs eventId, bool isReady)
{
	m_ReadyEventPlugin.SetEvent(eventId, isReady);
}

void NetworkHelper::HandleEventAllSet(int eventId)
{
	switch (eventId)
	{
	case RE_START:
		break;
	case RE_WAITINGFORPLAYERS:
		break;
	case RE_WAITINGFORACTIONS:
		break;
	case RE_FINISH:
		break;
	}

	/*if(eventId == RE_START)
	{
		system("cls");
		printf("game\n");
		printf("press 'f' to finish game\n");
	}
	if(eventId == RE_FINISH)
	{
		system("cls");
		printf("over\n");
		printf("new highscore, congrats\n");
	}*/
}

void NetworkHelper::Update()
{
	Packet *p; 
	for(p = m_RakPeer->Receive();p != NULL; m_RakPeer->DeallocatePacket(p), p=m_RakPeer->Receive())
	{
		switch (p->data[0])
		{
			//server side
		case ID_NEW_INCOMING_CONNECTION:
			printf("ID_NEW_INCOMING_CONNECTION %s \n", p->guid.ToString());
			m_ReadyEventPlugin.AddToWaitList(RE_START, p->guid);
			break;
			//client side
		case ID_CONNECTION_REQUEST_ACCEPTED:
			printf("ID_CONNECTION_REQUEST_ACCEPTED\n");

			//adding peers that you are connected to, to your event list
			m_ReadyEventPlugin.AddToWaitList(RE_START, p->guid);

			{
				RakNet::RakString rs;
				RakNet::BitStream bsIn(p->data, p->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				m_RakPeer->Send(&bsIn, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, true);
			}
			break;
			//clients recieve this
		case ID_READY_EVENT_ALL_SET:
			printf("Got ID_READY_EVENT_ALL_SET from %s\n", p->guid.ToString());
			{
				int eventId = 0;
				RakNet::BitStream bs(p->data,p->length,false);
				bs.IgnoreBytes(sizeof(MessageID));
				bs.Read(eventId);
				HandleEventAllSet(eventId);
			}
			break;
			//clients recieve
		case ID_READY_EVENT_SET:
			printf("Got ID_READY_EVENT_SET from %s\n", p->guid.ToString());
			break;
		case ID_READY_EVENT_UNSET:
			printf("Got ID_READY_EVENT_UNSET from %s\n", p->guid.ToString());
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			// Connection lost normally
			printf("ID_DISCONNECTION_NOTIFICATION\n");
			break;
		case ID_ALREADY_CONNECTED:
			// Connection lost normally
			printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", p->guid);
			break;
		case ID_INCOMPATIBLE_PROTOCOL_VERSION:
			printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
			break;
		case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
			printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n"); 
			break;
		case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
			printf("ID_REMOTE_CONNECTION_LOST\n");
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
			printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
			break;
		case ID_CONNECTION_BANNED: // Banned from this server
			printf("We are banned from this server.\n");
			break;			
		case ID_CONNECTION_ATTEMPT_FAILED:
			printf("Connection attempt failed\n");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			// Sorry, the server is full.  I don't do anything here but
			// A real app should tell the user
			printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
			break;
		case ID_INVALID_PASSWORD:
			printf("ID_INVALID_PASSWORD\n");
			break;
		case ID_CONNECTION_LOST:
			// Couldn't deliver a reliable packet - i.e. the other system was abnormally
			// terminated
			printf("ID_CONNECTION_LOST\n");
			break;
		case ID_CONNECTED_PING:
		case ID_UNCONNECTED_PING:
			printf("Ping from %s\n", p->systemAddress.ToString(true));
			break;
		}
	}		
}