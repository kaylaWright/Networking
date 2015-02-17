#include <iostream>
#include "../RakNetP2PExample/NetworkHelper.h"
#include <string>
#include <thread>

char command[255];
std::thread* processInputThread;
NetworkHelper *netHelp = 0;

void ProcessInput();

int main()
{
	printf("Welcome to game, now. Thank you.\n");
	printf("Press 1 for Single Player\n");
	printf("Press 2 to Host a game\n");
	printf("Press 3 to Join a game\n");

	gets_s(command);

	bool isNetworked = false;

	if(command[0] == '1')
	{
		printf("Starting solo campaign, cue cut scene of space or something cool...\n");
		
	}
	else if(command[0] == '2')
	{
		printf("You Are HOST\n");
		netHelp = new NetworkHelper();
		netHelp->Init(true);
		isNetworked = true;
	}
	else if(command[0] == '3')
	{
		printf("Waiting to join game\n");
		netHelp = new NetworkHelper();
		netHelp->Init(false);
		netHelp->EstablishConnection();
		isNetworked = true;
	}
	
	if(isNetworked)
	{
		printf("press 'r' to start (the) game\n");
		printf("press 'f' to finish (the) game\n");
		printf("type 's' followd by a message and press\n");
		printf("enter(return) to send it\n");
		printf("press 'q' to quit\n");
		processInputThread = new std::thread(ProcessInput);
	}

	bool isGameOver = false;
	while(!isGameOver)
	{
		if(netHelp)
		{
			netHelp->Update();
		}
	}

	return 0;
}

void ProcessInput()
{
	do
	{
		gets_s(command);
		if(command[0] == 'r')
		{
			if(netHelp)
			{
				netHelp->SetEvent(NetworkHelper::RE_START, true);
			}
		}
	}
	while(command[0] != 'q');

	printf("thanks for playing\n");
}