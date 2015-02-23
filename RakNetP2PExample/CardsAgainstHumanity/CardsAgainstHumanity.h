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

//defines
#define HANDSIZE 5

//information about the player/s.
struct Player
{
	//constructor. 
	Player(std::string _name = "", int _score = 0)
	{
		pName = _name;
		pScore = _score; 
		isAsker = false;
		hand = std::vector<std::string>();
	}
	//the player name.
	std::string pName;
	//the indices of the cards they have been allotted.
	std::vector<std::string> hand;
	//their score. 
	int pScore;
	//determines if they judge or play this turn. 
	bool isAsker;
};

struct Answer
{
	Answer(SystemAddress _add, std::string _card)
	{
		pAddress = _add;
		pCard = _card;
	}

	SystemAddress pAddress;
	std::string pCard;
};

//host-specific vars.

//typedefs.
typedef std::vector<std::string>::iterator stringIT;
typedef std::vector<Answer>::iterator answerIT;

class CardsAgainstHumanity
{
private:
	//networking variables. 
	NetworkHelper* netHelper;
	char command[256];
	std::thread* inputProcessingThread;

	//game vars.
	bool isQuit; 

	//cards. 
	std::string currentQuestionCard;
	std::vector<std::string> questionCards;
	std::vector<std::string> answerCards;

	std::vector<Answer> submittedAnswers;

	//xml handling of the cards. 
	bool LoadCards();
	//shuffles a given deck of cards using the fisher-yates shuffle.
	bool ShuffleCards(std::vector<std::string> &_deck);
	//displays all the cards, prefaced by a number. 
	void DisplayCards();

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

	//players. 
	Player player;

	//sets up the game (loads and shuffles cards, etc.
	void SetupGame();

	//deals out cards to each player based off of random indices in the answer vector. 
	void DealAnswerCards(int _numCards);
	//allows players to take a single card to replace one they've already had. 
	//still the hosts resposibility -> must draw the card and replace the one they used with it. 
	std::string DrawQuestionCard();
	//allows player to choose card, submits contents of the card to the host for collection. 
	void ChooseCard();
	int ChooseWinner(int _numCards);

	//collects a card for submission, used by raknet to populate submittedanswers vec. 
	void SubmitAnswer(Answer _answ);
	//shows all of the submitted answers to all players alongside the question, so all players can have a giggle
	//while they wait for the asker to decide on a winner.
	int DisplaySubmittedAnswers();

	//displays the scores thusfar, by name.
	void DisplayScores();

	///adds a score to the current game. 
	void AddScore();

	//getters/setters, if needed.
	bool GetQuit() const;
	void SetQuit(bool _new);

	void SetQuestionCard(std::string _card);

	//returns the address corresponding to a selected position. 
	std::string GetSubmittedAnswerByIndex(int _i);
	SystemAddress GetAnswerAddress(int _num);

	std::string CardsAgainstHumanity::GetScore();
};

#endif

//KW