#ifndef _CARDSAGAINST_H_
#define _CARDSAGAINST_H_

//standard includes
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <time.h>
#include <algorithm>

//networking includes
#include "..\RakNetP2PExample\NetworkHelper.h"

//xml reading includes. 
#include "..\PugiXML\src\pugixml.hpp"

//game includes

//some useful definitions.
//keeps track of what step of the game we're at. 
enum GameStates { WAITING_PLAYERS, WAITING_CARDS, DECIDING_WINNER };

//information about the player/s.
struct Player
{
	//constructor. 
	Player(std::string _name = "", int _score = 0)
	{
		pName = _name;
		pScore = _score; 
		isAsker = false;
		for (int i = 0; i < 5; i++)
		{
			hand[i] = "";
		}
	}
	//the player name.
	std::string pName;
	//the indices of the cards they have been allotted.
	std::string hand[5];
	//their score. 
	int pScore;
	//determines if they judge or play this turn. 
	bool isAsker;
};

//typedefs.
typedef std::vector<std::string>::iterator stringIT;
typedef std::vector<Player>::iterator playerIT;


class CardsAgainstHumanity
{
private:
	//networking variables. 
	NetworkHelper* netHelper = 0;
	char command[256];
	std::thread* inputProcessingThread;

	//game vars.
	bool isQuit; 

	//cards. 
	std::vector<std::string> questionCards;
	std::vector<std::string> answerCards;

	//players. 
	std::vector<Player> players;
	Player player;

	//sets up the game (loads and shuffles cards, etc.
	void SetupGame();

	//xml handling of the cards. 
	bool LoadCards();
	//shuffles a given deck of cards using the fisher-yates shuffle.
	bool ShuffleCards(std::vector<std::string> &_deck);
	//deals out cards to each player based off of random indices in the answer vector. 
	void DealCards();
	//allows players to take a single card to replace one they've already had. 
	//still the hosts resposibility -> must draw the card and replace the one they used with it. 
	std::string DrawCard();

	//ready events. 
	void GetReadyToPlay();

	//input functions
	void GetHost();
	void GetConnection();
	void GetName();
protected:
public:
	//constructor/destructor.
	CardsAgainstHumanity();
	~CardsAgainstHumanity();

	//
	void Init();
	bool Run();
	void Shutdown();

	//getters/setters, if needed.
	bool GetQuit() const;
	void SetQuit(bool _new);
};

#endif

//KW