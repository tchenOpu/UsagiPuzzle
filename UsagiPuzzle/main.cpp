
#include <SFML/Graphics.hpp>
#include <iostream>
#include <stdlib.h>

#define SIG_TIME 110
#define SIG_MOVL 103
#define SIG_MOVR 104
#define SIG_ROTL 105
#define SIG_ROTR 106
#define GROUP_LIMIT 3
#define TIMER_CLEAR 4
#define NB_COLORS 4
#define BASE_SCORE 10
#define  DISTRIB_SIZE 256
struct TableEntry
{
	unsigned int  currentState;
	unsigned char signal;
	unsigned int  nextState;
};

TableEntry const * tableBegin(void);
TableEntry const * tableEnd(void);

static const TableEntry stateTable[] =
{
	//  Current   Transition     Next
	//  State ID    Signal     State ID
	{ 0,          SIG_TIME,     1 }, //START
	{ 1,          SIG_TIME,     2 }, //CONTROL
	{ 2,          SIG_MOVL,     3 }, //MOVE LEFT
	{ 2,          SIG_MOVR,     4 }, //MOVE RIGHT
	{ 2,          SIG_ROTL,     5 }, //ROTATE LEFT
	{ 2,          SIG_ROTR,     6 }, //ROTATE RIGHT
	{ 2,          107,          7 }, //SUCCESS
	{ 2,          109,          8 }, //MISS
	{ 3,          SIG_TIME,     2 }, //TIME
	{ 4,          SIG_TIME,     2 }, //TIME
	{ 5,          SIG_TIME,     2 }, //TIME
	{ 6,          SIG_TIME,     2 }, //TIME
	{ 7,          SIG_TIME,     9 }, //CHECK
	{ 8,          SIG_TIME,     9 }, //CHECK
	{ 9,          122,          1 }, //RESET
	{ 9,          44,         10 }, //LOSE
};

static const unsigned int TABLESIZE = sizeof(stateTable) / sizeof(stateTable[0]);

TableEntry const *tableBegin(void)
{
	return &stateTable[0];
}


TableEntry const * tableEnd(void)
{
	return &stateTable[TABLESIZE];
}

class player
{
private:
	char field[5][8] = { { 0,0,0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0 } };
	char isClear[5][8] = { { false } };
	char isChecked[5][8] = { { false } };
	char controlPos = -1; // 0 1 2 3 4 positions
	char controlRot = -1; // 0 1 2 3 rotations
	char bloc[DISTRIB_SIZE] = {};
	char currentState = 0;
	char nextState = 0;
	int cursor = -2;
	int timerState = 10;
	int placePos1 = 15;
	int placePos2 = 80;
	int timerClear = TIMER_CLEAR;
	int startPosX = 0;
	int score = 0;
	/* Diagramme d'etats :
								+-------------------+
								| 0 -> Etat initial |
								+-------------------+
										  | START
										  | -----
										  v
						 +---------------------------------+
						 | 1 -> Arrivee des blocs suivants |<--------------\
						 +---------------------------------+                 \
										  | CONTROL                            \
										  | -------                              \
										  v                                        \
						   +-----------------------------+                          |
			PLACE     /----| 2 -> Control des blocs (1)  |----\     MISS / TIME     |
			-----   /      +-----------------------------+      \   -----------     |
				  /    MOVE |  ^ TIME           TIME |  ^ ROTATE  \                 |
				 |     ---- |  | ----           ---- |  | ------   |                |
				 |          v  |					 v  |          |                |
				 |   +----------------+       +-----------------+  |          RESET |
				 |   |  3,4 -> Move   |       |  5,6 -> Rotate  |  |          ----- |
				 |   +----------------+       +-----------------+  |                |
				 |                                                 |                |
				 v                                                 v                |
	+------------------------------------+ +---------------------------------+      |
	| 7 -> Placement des blocs (success) | | 8 -> Placement des blocs (echec)|      |
	+------------------------------------+ +---------------------------------+      |
								 | CHECK           | CHECK                         /
								 | -----           | -----                       /
								 v                 v                           /
						  +---------------------------------+                /
						  | 9 -> Verification & resolution |--------------/
						  +---------------------------------+
										  | LOSE
										  | ----
										  v
								  +--------------+
								  | 10 -> Defeat |
								  +--------------+
	*/
public:
	void initBloc() // Initialise les blocs du joueur
	{
		for (int u = 0; u < DISTRIB_SIZE; u++)
		{
			bloc[u] = (char)((u / 64) + 2);
		}
	}
	void randomizer(int seed) // Melange les blocs du joueur
	{
		char swapMemory;
		int randomIndex;
		srand(seed);
		for (int u = 0; u < DISTRIB_SIZE; u++)
		{
			randomIndex = rand() % DISTRIB_SIZE;
			swapMemory = bloc[255 - u];
			bloc[255 - u] = bloc[randomIndex];
			bloc[randomIndex] = swapMemory;
		}
	}
	void printBloc() // std::cout du vecteur bloc
	{
		for (int u = 0; u < DISTRIB_SIZE; u++)
		{
			std::cout << (int)bloc[u] << " ";
		}
	}
	char getState()
	{
		return currentState;
	}
	int getTimerClear()
	{
		return timerClear;
	}
	void stateRefresh() // rafrachit la variable currentState avec nextState
	{
		if (currentState != nextState)
		{
			currentState = nextState;
			std::cout << "new state:" << (int)nextState << '\n';
			switch (nextState) //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			{
			case 1: //arrivee des blocs suivants
				controlPos = 2;
				controlRot = 0;
				cursor = (cursor + 2)%DISTRIB_SIZE;
				timerState = 5; //deplacer par 41 pixels (205pixels total)
				//trucs
				break;
			case 3: //move
				timerState = 5;
				if (controlPos > 0 && controlPos + rotDirection() > 0)
				{
					controlPos = controlPos - 1;
				}
				break;
			case 4:
				timerState = 5;
				if (controlPos < 4 && controlPos + rotDirection() < 4)
				{
					controlPos = controlPos + 1;
				}
				break;
			case 5: //rotate
				timerState = 5;
				controlRot = (controlRot - 1);
				while (controlRot < 0)
				{
					controlRot = controlRot + 4;
				}
				if (controlRot == 1) {
					if (controlPos == 4) {
						controlPos = 3;
					}
				}
				if (controlRot == 3) {
					if (controlPos == 0) {
						controlPos = 1;
					}
				}
				break;
			case 6:
				timerState = 5;
				controlRot = controlRot + 1 % 4;
				while (controlRot > 3)
				{
					controlRot = controlRot - 4;
				}
				if (controlRot == 1) {
					if (controlPos == 4) {
						controlPos = 3;
					}
				}
				if (controlRot == 3) {
					if (controlPos == 0) {
						controlPos = 1;
					}
				}
				break;
			case 7: //placeSUCCESS
				timerState = (8 - (std::min(getHeight(controlPos), getHeight(controlPos + rotDirection()))));
				placePos1 = 15 + 65 * rotSides();
				placePos2 = 80 - 65 * rotReversed();
				break;
			case 8:
				break;
			case 9:
				std::cout << "check defeat : height " << getHeight(controlPos) << std::endl;
				if (getHeight(controlPos) == 8 || getHeight(controlPos + rotDirection()) == 8) {
					nextState = 10; // LOSE
				}
				else {
					if (rotReversed() || rotSides()) {
						field[controlPos][getHeight(controlPos)] = bloc[cursor];
						field[controlPos + rotDirection()][getHeight(controlPos + rotDirection())] = bloc[(cursor + 1)%DISTRIB_SIZE];
					}
					else {
						field[controlPos][getHeight(controlPos)] = bloc[(cursor + 1) % DISTRIB_SIZE];
						field[controlPos + rotDirection()][getHeight(controlPos + rotDirection())] = bloc[cursor];
					}
					checkClear();
					if (setTimerClear()) {

					}
					nextState = 1;
				}
			case 10:
				break;
			}
		}
	}
	char stateChain(char signal) // Cree l'etat suivant a partir du signal
	{
		TableEntry const *  p_entry = tableBegin();
		TableEntry const * const  p_table_end = tableEnd();
		bool state_found = false;
		while ((!state_found) && (p_entry != p_table_end))
		{
			if (p_entry->currentState == currentState)
			{
				if (p_entry->signal == signal)
				{
					nextState = p_entry->nextState;
					state_found = true;
					break;
				}
			}
			++p_entry;
		}
		return nextState;
	}
	char getField(int x, int y)
	{
		return field[x][y];
	}
	char getNextBloc(int u)
	{
		return bloc[(cursor + u)%DISTRIB_SIZE];
	}
	int getTimerState()
	{
		return timerState;
	}
	std::string getScoreString()
	{
		return std::string(6 - (std::to_string(score).length()), '0') + std::to_string(score);
	}
	void setTimerState(int t)
	{
		timerState = t;
	}
	char getControlPos()
	{
		return controlPos;
	}
	char getControlRot()
	{
		return controlRot;
	}
	int getHeight(char column)
	{
		for (int u = 0; u < 8; u++)
		{
			if (field[column][u] == 0)
			{
				return u;
			}
		}
		return 8;
	}
	int getPlacePos(bool right)
	{
		if (right)
		{
			return placePos2;
		}
		return placePos1;
	}
	void addPlacePos(bool right)
	{
		if (right)
		{
			placePos2 = placePos2 + 65;
		}
		else
		{
			placePos1 = placePos1 + 65;
		}
	}
	char rotDirection()
	{
		if (controlRot == 1)
		{
			return 1;
		}
		if (controlRot == 3)
		{
			return -1;
		}
		return 0;
	}
	char rotReversed()
	{
		if (controlRot == 2)
		{
			return 1;
		}
		return 0;
	}
	char vertRotL0() //AZE
	{
		if (controlRot == 3)
		{
			return 1;
		}
		else if (controlRot == 0)
		{
			return -1;
		}
		return 0;
	}
	char vertRotR0()
	{
		if (controlRot == 1)
		{
			return 1;
		}
		else if (controlRot == 0)
		{
			return -1;
		}
		return 0;
	}
	char vertRotL1()
	{
		if (controlRot == 2)
		{
			return 1;
		}
		else if (controlRot == 1)
		{
			return -1;
		}
		return 0;
	}
	char vertRotR1()
	{
		if (controlRot == 2)
		{
			return 1;
		}
		else if (controlRot == 3)
		{
			return -1;
		}
		return 0;
	}
	char horiRotR1()
	{
		if (controlRot == 2 || controlRot == 3)
		{
			return -1;
		}
		else if (controlRot == 1 || controlRot == 0)
		{
			return +1;
		}
		return 0;
	}
	char horiRotL1()
	{
		if (controlRot == 2 || controlRot == 1)
		{
			return -1;
		}
		else if (controlRot == 3 || controlRot == 0)
		{
			return +1;
		}
		return 0;
	}
	char rotSides()
	{
		if (controlRot == 0)
		{
			return 0;
		}
		return 1;
	}
	int checkNeighbour(int x, int y, int groupSize, int color)
	{
		if (getField(x, y) == color)
		{
			isChecked[x][y] = true;
			groupSize = groupSize + 1;
			if (y < 8)
			{
				if ((getField(x, y) == getField(x, y + 1)) && (!isChecked[x][y + 1]))
				{
					groupSize = checkNeighbour(x, y + 1, groupSize, color);
				}
			}
			if (y > 0)
			{
				if ((getField(x, y) == getField(x, y - 1)) && !isChecked[x][y - 1])
				{
					groupSize = checkNeighbour(x, y - 1, groupSize, color);
				}
			}
			if (x < 8)
			{
				if ((getField(x, y) == getField(x + 1, y)) && !isChecked[x + 1][y])
				{
					groupSize = checkNeighbour(x + 1, y, groupSize, color);
				}
			}
			if (x > 0)
			{
				if ((getField(x, y) == getField(x - 1, y)) && !isChecked[x - 1][y])
				{
					groupSize = checkNeighbour(x - 1, y, groupSize, color);
				}
			}
			return groupSize;
		}
		return 0;
	}
	void checkClear()
	{
		std::cout << "checking\n";
		for (int u = 2; u < 6; u++)
		{
			for (int v = 0; v < 5; v++)
			{
				for (int w = 0; w < 8; w++)
				{
					for (int k = 0; k < 5; k++) //set unchecked
					{
						for (int l = 0; l < 8; l++)
						{
							isChecked[k][l] = false;
						}
					}
					int test = checkNeighbour(v, w, 0, u);
					if (test >= 1) { std::cout << "voisins de " << v << "," << w << ":" << test << '\n'; }
					if (test >= GROUP_LIMIT)
					{
						addClear();
						std::cout << "cleared blocs\n";
					}
				}
			}
		}
	}

	void addClear()
	{
		for (int u = 0; u < 5; u++)
		{
			for (int v = 0; v < 8; v++)
			{
				if (isChecked[u][v])
				{
					isClear[u][v] = true;
				}
			}
		}
	}

	bool getIsClear(int x, int y)
	{
		return isClear[x][y];
	}

	bool setTimerClear()
	{
		bool result = false;
		int clearedBlocCount[NB_COLORS + 2] = { 0,0,0,0,0,0 };
		int multiplicater = 1;
		int scoreGain = 0;
		for (int u = 0; u < 5; u++)
		{
			for (int v = 0; v < 8; v++)
			{
				if (isClear[u][v])
				{
					if (!result) {
						timerClear = timerClear - 1;
						result = true;
						if (timerClear > 0) {
							return result;
						}
					}
					if (timerClear == 0) {
						clearedBlocCount[(int)field[u][v]]++;
						field[u][v] = 0;
						isClear[u][v] = false;

					}
				}
			}
		}
		if (result) {
			timerClear = TIMER_CLEAR;
		}
		for (int u = 0; u < NB_COLORS + 2; u++)
		{
			if (clearedBlocCount[u] > 0)
			{
				multiplicater *= 2;
				scoreGain += BASE_SCORE * clearedBlocCount[u];
			}
			std::cout << clearedBlocCount[u] << ',';
		}
		std::cout << std::endl << clearedBlocCount << std::endl;
		std::cout << multiplicater << std::endl;
		std::cout << scoreGain << std::endl;
		score += scoreGain * multiplicater;
		return result;
	}

	void refreshPlacePos() {
		if (getState() == 7)
		{
			if (getPlacePos(false) < 145 + 65 * (7 - getHeight(getControlPos())))
			{
				addPlacePos(false);
			}
			if (getPlacePos(true) < 145 + 65 * (7 - getHeight(getControlPos() + rotDirection())))
			{
				addPlacePos(true);
			}
		}
	}
	void refreshSignal() {
		if (getTimerState() <= 0)
		{
			stateChain(SIG_TIME);
		}
		stateRefresh();
	}
	void setP2(bool player2) {
		if (player2) {
			startPosX = 628;
		}
	}
	int getStartPosX() {
		return startPosX;
	}
	void reset(int seed) {
		for (int u = 0; u < 5; u++) {
			for (int v = 0; v < 8; v++) {
				field[u][v] = 0;
				isClear[u][v] = 0;
				isChecked[u][v] = 0;
			}
		}
		initBloc();
		randomizer(seed);
		currentState = 0;
		
	}
};

//int WinMain()
int main()
{
	sf::RenderWindow window(sf::VideoMode(1280, 720), "UsagiPuzzle");

	int number = 0;

	//Load & set backgroundField
	sf::Texture backgroundTexture;
	if (!backgroundTexture.loadFromFile("testField.png"))
	{
		printf("debugField:not_loaded\n");
	}
	sf::Sprite background;
	background.setTexture(backgroundTexture);

	//Load spriteSheet
	sf::Texture spriteTexture;
	if (!spriteTexture.loadFromFile("spriteSheet.png"))
	{
		printf("spriteSheet:not_loaded\n");
	}

	//Load font
	sf::Font font;
	if (!font.loadFromFile("fixedsys.ttf")) {
		std::cout << "Failed to load font \"fixedsys.ttf\"" << std::endl;
	}
	sf::Text textScore1;
	textScore1.setFont(font);
	textScore1.setString("000000");
	sf::Text textScore2;
	textScore2.setFont(font);
	textScore2.setString("000000");

	//Load defeat image
	sf::Texture defeatTexture;
	if (!defeatTexture.loadFromFile("defeat.png"))
	{
		printf("defeatImg:not_loaded\n");
	}
	sf::Sprite defeatImg;
	defeatImg.setTexture(defeatTexture);

	//Set bloc sprites
	sf::Sprite tile[6] = {};
	int u;
	for (u = 0; u < 6; u++)
	{
		tile[u].setTexture(spriteTexture);
		tile[u].setTextureRect(sf::IntRect(65 * u, 0, 65, 65));
	}

	//Initialisation des joueurs
	int seed = 1;
	player p1;
	p1.initBloc();
	p1.randomizer(seed);

	player p2;
	p2.initBloc();
	p2.randomizer(seed);
	p2.setP2(true);

	//Initialisation horloge
	window.setFramerateLimit(60);
	int passedMS = 0;
	sf::Clock clock;
	sf::Time elapsedTime = clock.getElapsedTime();


	while (window.isOpen())
	{
		elapsedTime = clock.restart();
		passedMS = passedMS + elapsedTime.asMilliseconds();
		while (passedMS > 16)
		{
			passedMS = passedMS - 16;
			//frame operation
			p1.setTimerState(p1.getTimerState() - 1);
			p2.setTimerState(p2.getTimerState() - 1);
			p1.refreshPlacePos();
			p2.refreshPlacePos();
			p1.refreshSignal();
			p2.refreshSignal();
		}
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape) {
					std::cout << "the escape key was pressed, resetting" << std::endl;
					seed = seed + 1;
					p1.reset(seed);
					p2.reset(seed);
				}
				if (event.key.code == sf::Keyboard::Up) { std::cout << "the up key was pressed" << std::endl; }
				if (event.key.code == sf::Keyboard::Down)
				{
					std::cout << "the down key was pressed" << std::endl;
					if (1)
					{
						p1.stateChain(107);//%%%%%%%%%%%%%%%%%%%%%%%%%%%
					}
				}
				if (event.key.code == sf::Keyboard::Left)
				{
					std::cout << "the left key was pressed" << std::endl;
					p1.stateChain(SIG_MOVL); // MOVE LEFT
				}
				if (event.key.code == sf::Keyboard::Right)
				{
					std::cout << "the right key was pressed" << std::endl;
					p1.stateChain(SIG_MOVR); // MOVE RIGHT
				}
				if (event.key.code == sf::Keyboard::Z)
				{
					std::cout << "the Z key was pressed" << std::endl;
					p1.stateChain(SIG_ROTL); // ROTATE LEFT
				}
				if (event.key.code == sf::Keyboard::X)
				{
					std::cout << "the X key was pressed" << std::endl;
					p1.stateChain(SIG_ROTR); // ROTATE RIGHT
				}
				//p2
				if (event.key.code == sf::Keyboard::S)
				{
					std::cout << "the S key was pressed" << std::endl;
					if (1)
					{
						p2.stateChain(107);//%%%%%%%%%%%%%%%%%%%%%%%%%%%
					}
				}
				if (event.key.code == sf::Keyboard::A)
				{
					std::cout << "the A key was pressed" << std::endl;
					p2.stateChain(SIG_MOVL); // MOVE LEFT
				}
				if (event.key.code == sf::Keyboard::D)
				{
					//std::cout << "the D key was pressed" << std::endl;
					p2.stateChain(SIG_MOVR); // MOVE RIGHT
					for (int v = 0; v < 8; v++)
					{
						for (int u = 0; u < 5; u++)
						{
							std::cout << (int)p1.getField(u, v);
						}
						std::cout << std::endl;
					}
				}
				if (event.key.code == sf::Keyboard::I)
				{
					std::cout << "the I key was pressed" << std::endl;
					p2.stateChain(SIG_ROTL); // ROTATE LEFT
				}
				if (event.key.code == sf::Keyboard::O)
				{
					std::cout << "the O key was pressed" << std::endl;
					p2.stateChain(SIG_ROTR); // ROTATE RIGHT
				}
			}
		}

		window.clear();
		window.draw(background);

		//Affichage du board p1
		for (int u = 0; u < 5; u++)
		{
			for (int v = 0; v < 8; v++)
			{
				int t = p1.getField(u, v);
				tile[t].setPosition((float)(p1.getStartPosX() + 105 + (u * 65)), (float)(600 - (v * 65)));
				window.draw(tile[t]);
			}
		}

		//Affichage du board p2
		for (int u = 0; u < 5; u++)
		{
			for (int v = 0; v < 8; v++)
			{
				int t = p2.getField(u, v);
				tile[t].setPosition((float)p2.getStartPosX() + 105 + (u * 65), (float)600 - (v * 65));
				window.draw(tile[t]);
			}
		}

		//Affichage des blocs effacables p1
		for (int u = 0; u < 5; u++)
		{
			for (int v = 0; v < 8; v++)
			{
				if (p1.getIsClear(u, v))
				{
					tile[1].setPosition((float)p1.getStartPosX() + 105 + (u * 65), (float)600 - (v * 65));
					tile[1].setColor(sf::Color(255, 255, 255, 128 - 20 * p1.getTimerClear()));
					window.draw(tile[1]);
				}
			}
		}

		//Affichage des blocs effacables p2
		for (int u = 0; u < 5; u++)
		{
			for (int v = 0; v < 8; v++)
			{
				if (p2.getIsClear(u, v))
				{
					tile[1].setPosition((float)p2.getStartPosX() + 105 + (u * 65), (float)600 - (v * 65));
					tile[1].setColor(sf::Color(255, 255, 255, 128 - 20 * p2.getTimerClear()));
					window.draw(tile[1]);
				}
			}
		}

		//Affichage des nextBloc p1
		for (int u = 0; u < 10; u++)
		{
			int t = (int)p1.getNextBloc(u + 2);
			tile[t].setPosition((float)p1.getStartPosX() + 440 + (37 * ((u / 2) % 2)), (float)65 * u + ((((u + 1) % 2) + (u + 1) / 2) * 5));
			window.draw(tile[t]);
		}

		//Affichage des nextBloc p2
		for (int u = 0; u < 10; u++)
		{
			int t = (int)p2.getNextBloc(u + 2);
			tile[t].setPosition((float)p2.getStartPosX() + 440 + (37 * ((u / 2) % 2)), (float)65 * u + ((((u + 1) % 2) + (u + 1) / 2) * 5));
			window.draw(tile[t]);
		}

		//Affichage des blocs arrivants
		if (p1.getState() == 1)
		{
			int t = (int)p1.getNextBloc(0);
			tile[t].setPosition((float)p1.getStartPosX() + 235 + 41 * p1.getTimerState(), (float)5);
			window.draw(tile[t]);
			t = (int)p1.getNextBloc(1);
			tile[t].setPosition((float)p1.getStartPosX() + 235 + 41 * p1.getTimerState(), (float)70);
			window.draw(tile[t]);
		}
		if (p2.getState() == 1)
		{
			int t = (int)p2.getNextBloc(0);
			tile[t].setPosition((float)p2.getStartPosX() + 235 + 41 * p2.getTimerState(), (float)5);
			window.draw(tile[t]);
			t = (int)p2.getNextBloc(1);
			tile[t].setPosition((float)p2.getStartPosX() + 235 + 41 * p2.getTimerState(), (float)70);
			window.draw(tile[t]);
		}

		//Affichage des blocs controlles
		if (p1.getState() == 2)
		{
			int t = (int)p1.getNextBloc(0);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + 65 * (p1.getControlPos()), (float)5 + 65 * (p1.rotSides()));
			window.draw(tile[t]);
			t = (int)p1.getNextBloc(1);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + (65 * p1.getControlPos()) + (65 * p1.rotDirection()), (float)70 - (65 * p1.rotReversed()));
			window.draw(tile[t]);
		}
		if (p2.getState() == 2)
		{
			int t = (int)p2.getNextBloc(0);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + 65 * (p2.getControlPos()), (float)5 + 65 * (p2.rotSides()));
			window.draw(tile[t]);
			t = (int)p2.getNextBloc(1);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + (65 * p2.getControlPos()) + (65 * p2.rotDirection()), (float)70 - (65 * p2.rotReversed()));
			window.draw(tile[t]);
		}

		//Affichage des blocs en deplacement
		if (p1.getState() == 3)
		{
			int t = (int)p1.getNextBloc(0);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + (65 * (p1.getControlPos())) + (13 * p1.getTimerState()), (float)5 + (65 * p1.rotSides()));
			window.draw(tile[t]);
			t = (int)p1.getNextBloc(1);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + (65 * (p1.getControlPos())) + (13 * p1.getTimerState()) + (65 * p1.rotDirection()), (float)70 - (65 * p1.rotReversed()));
			window.draw(tile[t]);
		}
		if (p1.getState() == 4)
		{
			int t = (int)p1.getNextBloc(0);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + (65 * (p1.getControlPos())) - (13 * p1.getTimerState()), (float)5 + (65 * p1.rotSides()));
			window.draw(tile[t]);
			t = (int)p1.getNextBloc(1);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + (65 * (p1.getControlPos())) - (13 * p1.getTimerState()) + (65 * p1.rotDirection()), (float)70 - (65 * p1.rotReversed()));
			window.draw(tile[t]);
		}
		if (p2.getState() == 3)
		{
			int t = (int)p2.getNextBloc(0);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + (65 * (p2.getControlPos())) + (13 * p2.getTimerState()), (float)5 + (65 * p2.rotSides()));
			window.draw(tile[t]);
			t = (int)p2.getNextBloc(1);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + (65 * (p2.getControlPos())) + (13 * p2.getTimerState()) + (65 * p2.rotDirection()), (float)70 - (65 * p2.rotReversed()));
			window.draw(tile[t]);
		}
		if (p2.getState() == 4)
		{
			int t = (int)p2.getNextBloc(0);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + (65 * (p2.getControlPos())) - (13 * p2.getTimerState()), (float)5 + (65 * p2.rotSides()));
			window.draw(tile[t]);
			t = (int)p2.getNextBloc(1);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + (65 * (p2.getControlPos())) - (13 * p2.getTimerState()) + (65 * p2.rotDirection()), (float)70 - (65 * p2.rotReversed()));
			window.draw(tile[t]);
		}

		//Affichage des blocs en rotation
		if (p1.getState() == 5)
		{
			int t = (int)p1.getNextBloc(0);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + 65 * (p1.getControlPos()), (float)5 + 65 * (p1.rotSides()) - (13 * p1.getTimerState() * p1.vertRotL0()));
			window.draw(tile[t]);
			t = (int)p1.getNextBloc(1);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + (65 * p1.getControlPos()) + (65 * p1.rotDirection()) + (13 * p1.getTimerState() * p1.horiRotL1()), (float)70 - (65 * p1.rotReversed()) + (13 * p1.getTimerState() * p1.vertRotL1()));
			window.draw(tile[t]);
		}
		if (p1.getState() == 6)
		{
			int t = (int)p1.getNextBloc(0);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + 65 * (p1.getControlPos()), (float)5 + 65 * (p1.rotSides()) - (13 * p1.getTimerState() * p1.vertRotR0())); //AZE
			window.draw(tile[t]);
			t = (int)p1.getNextBloc(1);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + (65 * p1.getControlPos()) + (65 * p1.rotDirection()) - (13 * p1.getTimerState() * p1.horiRotR1()), (float)70 - (65 * p1.rotReversed()) + (13 * p1.getTimerState() * p1.vertRotR1()));
			window.draw(tile[t]);
		}
		if (p2.getState() == 5)
		{
			int t = (int)p2.getNextBloc(0);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + 65 * (p2.getControlPos()), (float)5 + 65 * (p2.rotSides()) - (13 * p2.getTimerState() * p2.vertRotL0()));
			window.draw(tile[t]);
			t = (int)p2.getNextBloc(1);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + (65 * p2.getControlPos()) + (65 * p2.rotDirection()) + (13 * p2.getTimerState() * p2.horiRotL1()), (float)70 - (65 * p2.rotReversed()) + (13 * p2.getTimerState() * p2.vertRotL1()));
			window.draw(tile[t]);
		}
		if (p2.getState() == 6)
		{
			int t = (int)p2.getNextBloc(0);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + 65 * (p2.getControlPos()), (float)5 + 65 * (p2.rotSides()) - (13 * p2.getTimerState() * p2.vertRotR0())); //AZE
			window.draw(tile[t]);
			t = (int)p2.getNextBloc(1);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + (65 * p2.getControlPos()) + (65 * p2.rotDirection()) - (13 * p2.getTimerState() * p2.horiRotR1()), (float)70 - (65 * p2.rotReversed()) + (13 * p2.getTimerState() * p2.vertRotR1()));
			window.draw(tile[t]);
		}

		//Affichage des blocs en placement
		if (p1.getState() == 7)
		{
			int t = (int)p1.getNextBloc(0);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + (65 * (p1.getControlPos())), (float)p1.getPlacePos(false));
			window.draw(tile[t]);
			t = (int)p1.getNextBloc(1);
			tile[t].setPosition((float)p1.getStartPosX() + 105 + (65 * (p1.getControlPos())) + (65 * p1.rotDirection()), (float)p1.getPlacePos(true));
			window.draw(tile[t]);
		}
		if (p2.getState() == 7)
		{
			int t = (int)p2.getNextBloc(0);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + (65 * (p2.getControlPos())), (float)p2.getPlacePos(false));
			window.draw(tile[t]);
			t = (int)p2.getNextBloc(1);
			tile[t].setPosition((float)p2.getStartPosX() + 105 + (65 * (p2.getControlPos())) + (65 * p2.rotDirection()), (float)p2.getPlacePos(true));
			window.draw(tile[t]);
		}

		//Affichage du score p1
		textScore1.setPosition((float)p1.getStartPosX() + 100, (float)680);
		std::string a = std::to_string(12);
		textScore1.setString(p1.getScoreString());
		window.draw(textScore1);
		//Affichage du score p2
		textScore2.setPosition((float)p2.getStartPosX() + 100, (float)680);
		window.draw(textScore2);

		//Affichage defaite p1
		if (p1.getState() == 10) {
			defeatImg.setPosition((float)p1.getStartPosX() + 105, (float)342);
			window.draw(defeatImg);
		}

		//Affichage defaite p2
		if (p2.getState() == 10) {
			defeatImg.setPosition((float)p2.getStartPosX() + 105, (float)342);
			window.draw(defeatImg);
		}

		window.display();
	}

	return 0;
}
