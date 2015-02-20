#include "CardsAgainstHumanity.h"

#pragma region constructors/destructors.

CardsAgainstHumanity::CardsAgainstHumanity() : isQuit(false)
{
	netHelper = new NetworkHelper(this);

	//seed that sweet, sweet random. 
	std::srand(time(NULL));

	//initialize the player to default values. 
	player = Player();
	currentState = WAITING_PLAYERS;
	LoadCards();
}

CardsAgainstHumanity::~CardsAgainstHumanity()
{
	questionCards.clear();
	answerCards.clear();

	delete netHelper; 
	netHelper = NULL;
}

#pragma endregion

#pragma region init/shutdown

void CardsAgainstHumanity::Init()
{
	std::cout << "Welcome to Cards Against Humanity (#15 in the class)!" << std::endl;
	GetName();
	GetHost();
	GetReadyToPlay();
}

bool CardsAgainstHumanity::Run()
{
	//check to see if isQuit should be made true; if so, break. 
	if (isQuit)
	{
		return false; 
	}

	netHelper->Update();

	return true;
	//play the game. 
}

void CardsAgainstHumanity::Shutdown()
{
	std::cout << "Quitter? Quitter." << std::endl;
}

#pragma endregion

#pragma region gamehandling

void CardsAgainstHumanity::SetupGame()
{
	//use an xml parser to load the cards in. 
	LoadCards();

	//shuffle the question cards.
	ShuffleCards(questionCards);
	//shuffle the answer cards. 
	ShuffleCards(answerCards);

	//decide what card will be played upon.
	DrawQuestionCard();

	//deal out cards as needed to start game. 
	DealAnswerCards(5);

	currentState = GameStates::WAITING_CARDS;
	netHelper->BroadcastMessageToPeers(GameMessages::ID_CHOOSE_CARD, "");

	ChooseCard();
}

//shuffles the cards of a given vector. 
// PARAMS: 
// std::vector<std::string> &_deck -> the vector to be shuffled.
bool CardsAgainstHumanity::ShuffleCards(std::vector<std::string>& _deck)
{
	//if the deck is empty, don't shuffle it. 
	if (_deck.empty())
	{
		return false;
	}

	//welp this made it silly to shuffle them. 
	std::random_shuffle(_deck.begin(), _deck.end());

	return true; 
}

#pragma endregion 

#pragma region inputfunctions

void CardsAgainstHumanity::GetHost()
{	
	std::cout << "Would you like to (H)ost or (J)oin the game?\n You can (Q)uit too..." << std::endl;
	std::cin.clear();
	std::cin >> command; 

	if (command[0] == 'H' || command[0] == 'h')
	{
		std::cout << "You'll host then." << std::endl;

		netHelper->Init(true);

		//only the host needs to hold onto the 'original' ordering of the cards, and will be responsible for
		//dealing out new cards as needed. All other players will receive string literals for their hands. 
		void SetupGame();
	}
	else if (command[0] == 'J' || command[0] == 'j')
	{
		std::cout << "You want to join an ongoing game." << std::endl;

		netHelper->Init(false);
		GetConnection();
	}
	else if (command[0] == 'Q' || command[0] == 'q')
	{
		isQuit = true;
	}
	else
	{
		std::cout << "Sorry, could you repeat that please? I think your fingers stuttered." << std::endl;
		GetHost();
	}
}

void CardsAgainstHumanity::GetConnection()
{
	std::cout << "Okay, who do you want to connect to? (H)ome is an option." << std::endl;
	std::cin.clear();
	std::cin >> command;
	
	if (command[0] == 'H' || command[0] == 'h')
	{
		std::cout << "Connecting to yourself, huh? You're super cool." << std::endl;
		netHelper->EstablishConnection("127.0.0.1");
		netHelper->Update();
	}
	else if (netHelper->EstablishConnection(command))
	{
		std::cout << "Cool, so you're connected. Almost ready to go, just a few more things..." << std::endl;
	}
	else
	{
		std::cout << "Yeah, we don't recognize that one. Try again." << std::endl;
		GetConnection();
		netHelper->Update();
	}
}

void CardsAgainstHumanity::GetName()
{
	std::cout << "What's your name, then?" << std::endl;
	std::cin >> player.pName;
	std::cout << "Nice to meet you " << player.pName << "." << std::endl;
}

void CardsAgainstHumanity::GetReadyToPlay()
{
	std::cout << "Let the machine know when you're (r)eady to play." << std::endl;
	std::cin.clear();
	std::cin >> command;

	while (command[0] != 'r' && command[0] != 'R')
	{
		std::cin.clear();
		std::cin >> command;
		std::cout << command[0];
	}

	netHelper->SetEvent(NetworkHelper::RE_WAITINGFORPLAYERS, true);

	//system("cls");
	std::cout << "Waiting on other players to be ready..." << std::endl;
}

#pragma endregion

#pragma region cardhandling  

//loads the cards in from an xml. 
bool CardsAgainstHumanity::LoadCards()
{
	//get the xml document, make sure it loads correctly. 
	pugi::xml_document cardDOC;
	if (!cardDOC.load_file("cards.xml"))
	{
		std::cout << "Couldn't load cards.xml" << std::endl;
		return false;
	}

	//load all of the questions.
	pugi::xml_node questions = cardDOC.child("cards").child("questions");
	for (pugi::xml_node question = questions.first_child(); question; question = question.next_sibling())
	{
		questionCards.push_back(question.child_value());
	}

	//load all of the answers.
	pugi::xml_node answers = cardDOC.child("cards").child("answers");
	for (pugi::xml_node answer = answers.first_child(); answer; answer = answer.next_sibling())
	{
		answerCards.push_back(answer.child_value());
	}

	//all's well! <3
	return true; 
}

//deals out cards to each player based off of random indices in the answer vector. 
void CardsAgainstHumanity::DealAnswerCards(int _numCards)
{
	//if we've run through the deck of answer cards, reload it in.
	if (answerCards.empty())
	{
		LoadCards();
	}

	//deal five cards to the host.
	for (int i = 0; i < _numCards; i++)
	{
		player.hand.push_back(answerCards.back());
		answerCards.pop_back();
	}

	std::cout << netHelper->GetNumConnections() << std::endl;

	//deal out cards to remaining players. 
	// i = 1 because 0 is the host. 
	for (int i = 0; i < netHelper->GetNumConnections(); i++)
	{
		for (int c = 0; c < _numCards; c++)
		{
			netHelper->SendMessageToPeer(ID_RECEIVE_CARD, netHelper->GetPlayerAddress(i), answerCards.back());
			answerCards.pop_back();
		}
	}
}

//host only, draws a new question card and passes it back to all players (except host) for displaying. 
std::string CardsAgainstHumanity::DrawQuestionCard()
{
	currentQuestionCard = questionCards.back();
	for (int i = 0; i < netHelper->GetNumConnections(); i++)
	{
		netHelper->SendMessageToPeer(ID_RECEIVE_QUESTION_CARD, netHelper->GetPlayerAddress(i), questionCards.back());
	}

	questionCards.pop_back();

	return currentQuestionCard;
}

//allows the player to choose which card they would like to play this round
//triggers a ready event. 
std::string CardsAgainstHumanity::ChooseCard()
{
	system("cls");

	//dispaly the current question card. 
	std::cout << "Play on: " << std::endl;
	std::cout << currentQuestionCard << std::endl << std::endl;

	std::cout << "Which card would you like to play?" << std::endl;
	DisplayCards();

	std::cin.clear();
	std::cin >> command;

	int temp = NULL;

	switch (command[0])
	{
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	{
		//need to subtract the extra one because the cases are 1-5, not 0-4.
		temp = command[0] - 49;
		std::cout << std::endl << "Okay, you've chosen..." << std::endl << player.hand[temp] << std::endl;
		std::cout << "Now we're just waiting on the others..." << std::endl; 
		netHelper->SetEvent(NetworkHelper::RE_WAITINGFORACTIONS, true);
		break;
	}
	default:
		std::cout << "You don't own a card by that number. Wanna try again?" << std::endl;
		ChooseCard();
		break;
	}

	return player.hand[temp];
}

//displays all the cards, numbered according to the first one shown. 
void CardsAgainstHumanity::DisplayCards()
{
	for (int i = 0; i < player.hand.size(); i++)
	{
		std::cout << (i + 1) << ": " << player.hand[i] << std::endl;
	}
}

#pragma endregion 

#pragma region scoring.

//adds a score to the players count. 
void CardsAgainstHumanity::AddScore()
{
	player.pScore += 1;
}

#pragma region

//this region is full of getters/setters. Names are self-explanatory, ditto that with any parameters. 
#pragma region get/set

bool CardsAgainstHumanity::GetQuit() const
{
	return isQuit;
}

void CardsAgainstHumanity::SetQuit(bool _new)
{
	isQuit = _new; 
}

void CardsAgainstHumanity::SetQuestionCard(std::string _card)
{
	currentQuestionCard = _card;
}

#pragma endregion 

//KW 