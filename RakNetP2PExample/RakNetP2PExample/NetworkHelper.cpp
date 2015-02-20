#include "NetworkHelper.h"
#include "BitStream.h"
#include <iostream>

//must include due to forward declaration of the class. 
#include "../CardsAgainstHumanity/CardsAgainstHumanity.h"

//added in a reference to the game, so we can call game functions from here. 
NetworkHelper::NetworkHelper(CardsAgainstHumanity* _game) : m_RakPeer(0), m_IsHost(false)
{
	currentGame = _game;
}

//didn't touch this. 
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
	if (m_RakPeer)
	{
		RakPeerInterface::DestroyInstance(m_RakPeer);
	}

	if (currentGame)
	{
		delete currentGame;
	}
		
}

//Establishes a connection with the host. 
//PARAM:
//	_connection - the address to which the user desires to connect.
bool NetworkHelper::EstablishConnection(const char* _connection)
{
	ConnectionAttemptResult car = m_RakPeer->Connect(_connection, PORT, 0, 0, 0);

	if(car==CONNECTION_ATTEMPT_STARTED)
	{ 
		return true;
	}
	
	return false;
}

//Returns the system address of a given player, used to loop through and feed information to specific players. 
//PARAM:
//	_num -> the index of the player to receive information. 
SystemAddress NetworkHelper::GetPlayerAddress(int _num)
{
	SystemAddress playerAddress;

	if (_num < GetNumConnections())
	{
		playerAddress = m_RakPeer->GetSystemAddressFromIndex(_num);
	}

	//so we know who to send messages to, specifically. 
	return playerAddress;
}

//returns the number of connections, so the game knows how many hands/cards to deal out. 
int NetworkHelper::GetNumConnections()
{
	unsigned short numConn; 
	m_RakPeer->GetConnectionList(0, &numConn);
	return static_cast<int>(numConn);
}

//didn't touch this, handles ready events.
void NetworkHelper::SetEvent(READYEVENTIDs eventId, bool isReady)
{
	m_ReadyEventPlugin.SetEvent(eventId, isReady);
}

//didn't touch this, handles all-ready events. 
void NetworkHelper::HandleEventAllSet(int eventId)
{
	switch (eventId)
	{
	case RE_WAITINGFORPLAYERS:
		if (GetNumConnections() < MIN_CONNECTIONS)
		{
			//tells other player this player is ready.
			std::string temp = "We still need more players!\nWe only have " + std::to_string((GetNumConnections() + 1)) + "/4.";
			temp = currentGame->player.pName + " is also ready.";
			BroadcastMessageToPeers(ID_BROADCAST_MESSAGE, temp);

			//displays to self this player is ready. 
			std::cout << "You are ready, " << currentGame->player.pName << "." << std::endl << std::endl;
		}
		else
		{
			//get the game rolling. 
			if (m_IsHost)
			{
				std::cout << "Let the games... BEGIN." << std::endl;
				currentGame->SetupGame();
			}
		}
		break;
		break;
	case RE_WAITINGFORACTIONS:
		std::cout << "everyone's ready to see the cards!" << std::endl;
		break;
	case RE_FINISH:
		break;
	}
}

//allows the game to access and send messages to peers in a simplified way. otherwise would need to access rakpeer directly in the game
//which seems kind of like a bad idea. encapsulation, ahoy. 
//PARAMS:
// _type - what kind of message to be sent to the peers (see GameMessages enum in header).
// _add - which address the message is to go to, derived generally from GetPlayerAddress.
// _card - if relevant, allows the player to receive the card they should get. Formatted as a string, because only the host has access to the vectors.
void NetworkHelper::SendMessageToPeer(GameMessages _type, SystemAddress _add, std::string _card)
{
	RakNet::BitStream bsOUT;
	bsOUT.Write((unsigned char)_type);

	//convert string to the rakstring -> apparently faster over networks than std::strings by 3x? 
	std::string tempSTD = _card;
	RakNet::RakString tempRAK(tempSTD.c_str());
	bsOUT.Write(tempRAK);

	m_RakPeer->Send(&bsOUT, HIGH_PRIORITY, RELIABLE_ORDERED, 0, _add, false);
}

//similar to SendMessageToPeer, broadcasts a message to ALL peers. 
void NetworkHelper::BroadcastMessageToPeers(GameMessages _type, std::string _message)
{
	RakNet::BitStream bsOUT;
	bsOUT.Write((unsigned char)_type);

	//convert string to the rakstring -> apparently faster over networks than std::strings by 3x? 
	std::string tempSTD = _message;
	RakNet::RakString tempRAK(tempSTD.c_str());
	bsOUT.Write(tempRAK);

	m_RakPeer->Send(&bsOUT, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
}

//checks for packets from peers. 
void NetworkHelper::Update()
{
	Packet *p; 
	for(p = m_RakPeer->Receive(); p != NULL; m_RakPeer->DeallocatePacket(p), p = m_RakPeer->Receive())
	{
		//create a new rakstring to hold the data.
		RakNet::RakString rs;
		RakNet::BitStream bsIN(p->data, p->length, false);

		switch (p->data[0])
		{
		//CAH specific.
		//deals a whole hand to players -> in case one would like to restart a second round. 
		case ID_DEAL_HAND:
			if (m_IsHost)
			{
				//if you're the host, deal out cards to all of the players. 
				currentGame->DealAnswerCards(5);
			}
			break;
		//deals a single card to all players.
		case ID_DEAL_CARD:
			if (m_IsHost)
			{
				//if the host, you may draw a card to send to a player. 
				currentGame->DealAnswerCards(1);
			}
			break;
		//allows players to choose a card. 
		case ID_CHOOSE_CARD:
			currentGame->ChooseCard();
			break;
		case ID_START_ROUND:
			if (m_IsHost)
			{
				//deal out cards.
				currentGame->DealAnswerCards(5);
				//choose a czar, save that index. 
			}
			break;
		case ID_RECEIVE_CARD:
			if (m_IsHost == false)
			{
				//ignore the message sent in. 
				bsIN.IgnoreBytes(sizeof(RakNet::MessageID));
				bsIN.Read(rs);

				std::cout << rs << " " << static_cast<std::string>(rs) << std::endl;
				//convert the string to an std::string and send it to the player receiving it. 
				currentGame->player.hand.push_back(static_cast<std::string>(rs));
			}
			break;
		//tells players what the current question card is 
		case ID_RECEIVE_QUESTION_CARD:
			bsIN.IgnoreBytes(sizeof(RakNet::MessageID));
			bsIN.Read(rs);
			currentGame->SetQuestionCard(static_cast<std::string>(rs));
			break;
		//broadcasts message to all players. 
		case ID_BROADCAST_MESSAGE:
			bsIN.IgnoreBytes(sizeof(RakNet::MessageID));
			bsIN.Read(rs);
			std::cout << rs.C_String() << std::endl;
			break;
		//general networking. 
		case ID_NEW_INCOMING_CONNECTION:
			printf("A new player is joining...\n");
			//necessary, or adding in a player twice? 
			m_ReadyEventPlugin.AddToWaitList(RE_WAITINGFORPLAYERS, p->guid);
			m_ReadyEventPlugin.AddToWaitList(RE_WAITINGFORACTIONS, p->guid);
			m_ReadyEventPlugin.AddToWaitList(RE_FINISH, p->guid);

			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			printf("Your connection request accepted!\n");
			//adding peers that you are connected to, to your event list
			m_ReadyEventPlugin.AddToWaitList(RE_WAITINGFORPLAYERS, p->guid);
			m_ReadyEventPlugin.AddToWaitList(RE_WAITINGFORACTIONS, p->guid);
			m_ReadyEventPlugin.AddToWaitList(RE_FINISH, p->guid);
			
			bsIN.IgnoreBytes(sizeof(RakNet::MessageID));
			m_RakPeer->Send(&bsIN, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, true);
			break;
		case ID_CONNECTION_ATTEMPT_FAILED:
			printf("Connection attempt failed\n");
			break;
		case ID_READY_EVENT_ALL_SET:
			{
				int eventId = 0;
				bsIN.IgnoreBytes(sizeof(MessageID));
				bsIN.Read(eventId);
				HandleEventAllSet(eventId);
			}
			break;
		case ID_READY_EVENT_SET:
			break;
		case ID_READY_EVENT_UNSET:
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			printf("ID_DISCONNECTION_NOTIFICATION\n");
			break;
		case ID_CONNECTION_LOST:
			printf("ID_CONNECTION_LOST\n");
			break;
		}
	}		
}