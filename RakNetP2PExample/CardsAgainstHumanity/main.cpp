// Cards Against Humanity (Clonetown) -- Multiplayer Systems.
// Kayla Wright, 100853637

#include "CardsAgainstHumanity.h"

int main(int argc, char* argv)
{
	CardsAgainstHumanity* cards = new CardsAgainstHumanity();
	cards->Init();

	while (cards->Run())
	{

	}
	
	cards->Shutdown();

	std::cout << "Exiting program now." << std::endl;
	std::system("pause");
	return 0;
}

//KW