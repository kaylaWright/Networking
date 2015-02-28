#include "CardsAgainstHumanity.h"

#pragma region constructors/destructors.

CardsAgainstHumanity::CardsAgainstHumanity() : isQuit(false)
{
	netHelper = new NetworkHelper(this);

	//seed that sweet, sweet random. 
	std::srand(time(NULL));

	//initialize the player to default values. 
	player = Player();
}

CardsAgainstHumanity::~CardsAgainstHumanity()
{
	questionCards.clear();
	answerCards.clear();
	submittedAnswers.clear();

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
	questionCards.clear();
	answerCards.clear();
	submittedAnswers.clear();

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

	//choose a new czar. 
	int temp = netHelper->GetNumConnections();
	temp = rand() % temp;
	netHelper->SendMessageToPeer(ID_BECOME_ASKER, netHelper->GetPlayerAddress(temp), "");

	netHelper->BroadcastMessageToPeers(GameMessages::ID_CHOOSE_CARD, "");
	ChooseCard();
}

void CardsAgainstHumanity::ResetGame()
{
	player.pScore = 0;
	player.hand.clear();

	GetReadyToPlay();
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
		player.isAsker = true;

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
void CardsAgainstHumanity::ChooseCard()
{
	system("cls");

	//display Score.
	std::cout << "Your score: " << player.pScore << std::endl << std::endl;

	//dispaly the current question card. 
	std::cout << "Playing on: " << std::endl;
	std::cout << currentQuestionCard << std::endl << std::endl;

	int temp = NULL;
	if (player.isAsker == true)
	{
		std::cout << "You're the czar this round.\nSit back and wait for some cards to come your way." << std::endl;
		netHelper->SetEvent(NetworkHelper::RE_WAITINGFORACTIONS, true);
	}
	else
	{
		std::cout << "Which card would you like to play?" << std::endl;
		DisplayCards();

		std::cin.clear();
		std::cin >> command;

		if ((command[0] - '0') <= player.hand.size())
		{
			//need to subtract the extra one because the cases are 1-5, not 0-4.
			temp = command[0] - 49;
			std::cout << std::endl << "Okay, you've chosen..." << std::endl << player.hand[temp] << std::endl;
			std::cout << "Now we're just waiting on the others..." << std::endl;
			netHelper->BroadcastMessageToPeers(ID_COLLECT_ANSWER_CARDS, player.hand[temp]);
			

			//if you're the host, send a message to yourself as well. 
			if (netHelper->GetIsHost())
			{
				netHelper->SendMessageToPeer(ID_COLLECT_ANSWER_CARDS, netHelper->GetLocalAddress(), player.hand[temp]);
			}

			//remove that card from the players hands, they don't get it back no matter what. 
			stringIT it = player.hand.begin() + temp;
			std::rotate(it, it + 1, player.hand.end());
			player.hand.pop_back();

			netHelper->SetEvent(NetworkHelper::RE_WAITINGFORACTIONS, true);
		}
		else
		{
			std::cout << "You don't own a card by that number. Wanna try again?" << std::endl;
			ChooseCard();
		}
	}
}

//allows the asker to choose the winning card. 
int CardsAgainstHumanity::ChooseWinner(int _numCards)
{
	std::cout << "All right, czar, what card do you like best?" << std::endl << std::endl;

	std::cin.clear();
	std::cin >> command;
	int card = command[0] - 49;

	if (card <= _numCards)
	{
		std::cout << "Okay, cool." << std::endl;
		netHelper->SetEvent(NetworkHelper::RE_WAITING_FOR_DECISION, true);
		return card;
	}
	else
	{
		std::cout << "Not an option, tho." << std::endl;
		ChooseWinner(_numCards);
	}
}

void CardsAgainstHumanity::StartNewRound()
{
	//choose a new czar. 
	int temp = netHelper->GetNumConnections();
	temp = rand() % temp;
	netHelper->SendMessageToPeer(ID_BECOME_ASKER, netHelper->GetPlayerAddress(temp), "");

	//decide what card will be played upon.
	DrawQuestionCard();

	//deal out cards as needed to start game. 
	DealAnswerCards(1);
	
	//clear out previous answers.
	submittedAnswers.clear();

	//get started. 
	netHelper->BroadcastMessageToPeers(GameMessages::ID_CHOOSE_CARD, "");
	ChooseCard();
}

//displays all the cards, numbered according to the first one shown. 
void CardsAgainstHumanity::DisplayCards()
{
	for (int i = 0; i < player.hand.size(); i++)
	{
		std::cout << (i + 1) << ": " << player.hand[i] << std::endl;
	}
}

//pushes an answer back into the vector of submitted answers 
void CardsAgainstHumanity::SubmitAnswer(Answer _answ)
{
	submittedAnswers.push_back(_answ);
}

//Displays all the submitted answers to all players, as well as the host. HOST ONLY.
int CardsAgainstHumanity::DisplaySubmittedAnswers()
{
	if (submittedAnswers.empty())
	{
		std::cout << "Something's wrong in display submitted answers." << std::endl;
		return 0;
	}

	//redisplay the question to everyone. 
	std::cout << currentQuestionCard << std::endl << std::endl;
	netHelper->BroadcastMessageToPeers(ID_BROADCAST_MESSAGE, (currentQuestionCard + "\n"));

	//display the selected answers to everyone, alongside a number so the asker knows what to put in. 
	int i = 1;
	for (answerIT it = submittedAnswers.begin(); it != submittedAnswers.end(); ++it, ++i)
	{
		std::string temp = std::to_string(i) + ": " + it->pCard;
		std::cout << temp << std::endl;
		netHelper->BroadcastMessageToPeers(ID_BROADCAST_MESSAGE, temp);
	}

	//extra space though. 
	netHelper->BroadcastMessageToPeers(ID_BROADCAST_MESSAGE, "\n");

	return submittedAnswers.size();
}

#pragma endregion 

#pragma region scoring.

//displays the scores.
void CardsAgainstHumanity::DisplayScores()
{
	netHelper->SendMessageToPeer(ID_SHOW_SCORE, netHelper->GetLocalAddress(), "");

	for (int i = 0; i < netHelper->GetNumConnections(); i++)
	{
		netHelper->SendMessageToPeer(ID_SHOW_SCORE, netHelper->GetPlayerAddress(0), "");
	}
}

//adds a score to the players count. 
void CardsAgainstHumanity::AddScore()
{
	player.pScore += 1;

	if (player.pScore >= 5)
	{
		std::cout << "You win!" << std::endl;
		netHelper->BroadcastMessageToPeers(ID_WIN_GAME, player.pName);
		netHelper->SendMessageToPeer(ID_WIN_GAME, netHelper->GetLocalAddress(), player.pName);
	}
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

SystemAddress CardsAgainstHumanity::GetAnswerAddress(int _num)
{
	return submittedAnswers[_num].pAddress;
}

std::string CardsAgainstHumanity::GetSubmittedAnswerByIndex(int _i)
{
	return submittedAnswers[_i].pCard;
}

std::string CardsAgainstHumanity::GetScore()
{
	std::string temp;
	temp = player.pName + ": " + std::to_string(player.pScore);
	return temp;
}

#pragma endregion 

//KW 