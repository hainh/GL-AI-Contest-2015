#include <ai/Game.h>
#include <ai/AI.h>
#include <time.h>

#include "../../../../AI_Impl.h"

// ==================== HOW TO RUN THIS =====================
// Call:
// "AI_Template.exe -h [host] -p [port] -k [key]"
//
// If no argument given, it'll be 127.0.0.1:3011
// key is a secret string that authenticate the bot identity
// it is not required when testing
// ===========================================================

//////////////////////////////////////////////////////////////////////////////////////
//                                                                                  //
//                                    GAME RULES                                    //
//                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////
// - Game board is an array of MAP_SIZExMAP_SIZE blocks                             //
// - 2 players starts at 2 corners of the game board                                //
// - Each player will take turn to move                                             //
// - Player can only move left/right/up/down and stay inside the game board         //
// - The game is over when one of 2 players cannot make a valid move                //
// - In a competitive match:                                                        //
//   + A player will lose if they cannot connect to server within 10 seconds        //
//   + A player will lose if they don't make a valid move within 3 seconds          //
//////////////////////////////////////////////////////////////////////////////////////


// This function is called automatically when it's your turn.
// Remember to call AI_Move() with a valid move before the time is run out.
// See <ai/Game.h> and <ai/AI.h> for supported APIs.
void AI_Update()
{
	FILETIME ft_now;
	GetSystemTimeAsFileTime(&ft_now);

	AI *p_ai = AI::GetInstance();
	int * _board = p_ai->GetBoard();	// Access block at (x, y) by using board[CONVERT_COORD(x,y)]
	Position myPos = p_ai->GetMyPosition();
	Position enemyPos = p_ai->GetEnemyPosition();

	int direction = AiMove(_board, myPos, enemyPos);

	if(direction)
	{
		printf("Move: %d", direction);

		//Remember to call AI_Move() within allowed time
		Game::GetInstance()->AI_Move(direction);
	}
	else
	{
		Game::GetInstance()->AI_Move(0);
		printf("Damn, I was trapped!\n");
	}

	FILETIME ft_now2;
	GetSystemTimeAsFileTime(&ft_now2);

	LONGLONG ll_now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);
	LONGLONG ll_now2 = (LONGLONG)ft_now2.dwLowDateTime + ((LONGLONG)(ft_now2.dwHighDateTime) << 32LL);

	printf("time = %lld\n", (ll_now2 - ll_now) / 10000);
}

#if _DEBUG
extern int deepMoveDfsIterative(board* b, const Position &pos);
extern int deepMove(board* b, int x, int y, int depth, const int maxDepth, bool returnDirection);
int deepMove_ia(unsigned int* b, int x, int y, int depth, int maxDepth, bool returnDirection);

void test()
{
	//void main()
	

	int board_state[] = {
		1, 0, 0, 0, 5, 0, 0, 5, 0, 5, 5,
		0, 0, 0, 0, 5, 0, 5, 0, 0, 5, 5,
		0, 0, 0, 0, 0, 5, 0, 0, 5, 5, 5,
		0, 0, 5, 0, 0, 5, 5, 5, 5, 5, 5,
		0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 5, 5, 5, 5, 0, 5,
		5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 5,
		0, 0, 0, 0, 0, 0, 5, 5, 3, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	};//               |

	auto b = copyFrom(board_state);
	unsigned int* rBoard = b->cells;

	float total = 0;
	system("pause");

	for (int i = 0; i < 45; ++i)
	{

		FILETIME now;
		GetSystemTimeAsFileTime(&now);
		LONGLONG now1 = (LONGLONG)now.dwLowDateTime + ((LONGLONG)(now.dwHighDateTime) << 32LL);

		int maxDepth = deepMove_ia(rBoard, 8, 8, 0, 100, false);

		GetSystemTimeAsFileTime(&now);
		LONGLONG now2 = (LONGLONG)now.dwLowDateTime + ((LONGLONG)(now.dwHighDateTime) << 32LL);

		int maxDepth2 = deepMove(b, 8, 8, 0, 100, false);

		GetSystemTimeAsFileTime(&now);
		LONGLONG now3 = (LONGLONG)now.dwLowDateTime + ((LONGLONG)(now.dwHighDateTime) << 32LL);

		//printf("Max Depth iterative = %d, time = %lld\n", maxDepth, now2 - now1);
		//printf("Max Depth recursive = %d, time = %lld\n", maxDepth2, now3 - now2);
		float rate = (now2 - now1) / (float)(now3 - now2);
		//printf("Ratio iterative/recursive = %f\n", rate);
		total += rate;
	}

	printf("end test, rate = %f", total / 45);
	getchar();

	//extern int evaluateBoard(board* b, const Position& myPos, const Position& opPos);
	Position myPos = Position(0, 0);
	Position opPos = Position(8, 8);
	
	for (int y = 0; y < MAP_SIZE; ++y)
	{
		for (int x = 0; x < MAP_SIZE; ++x)
		{
			printf("%d  ", board_state[CONVERT_COORD(x, y)]);
		}
		printf("\n\n");
	}

	while (true)
	{
		FILETIME ft_now;
		GetSystemTimeAsFileTime(&ft_now);
		LONGLONG ll_now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);

		int dir = AiMove(board_state, myPos, opPos);

		GetSystemTimeAsFileTime(&ft_now);
		LONGLONG ll_now2 = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);
		printf("Player1: time = %lld, dir = %d\n", (ll_now2 - ll_now) / 10000, dir);
		board_state[CONVERT_COORD(myPos.x, myPos.y)] = BLOCK_PLAYER_1_TRAIL;
		switch (dir)
		{
		case 1:
			myPos.x--;
			break;
		case 2:
			myPos.y--;
			break;
		case 3:
			myPos.x++;
			break;
		case 4:
			myPos.y++;
			break;
		default:
			break;
		}
		board_state[CONVERT_COORD(myPos.x, myPos.y)] = BLOCK_PLAYER_1;
		for (int y = 0; y < MAP_SIZE; ++y)
		{
			for (int x = 0; x < MAP_SIZE; ++x)
			{
				printf("%d  ", board_state[CONVERT_COORD(x, y)]);
			}
			printf("\n\n");
		}

		//int charCode = 0;
		//do
		//{
		//	printf("Your move: ");
		//	charCode = getchar();
		//	while (getchar() != '\n'){}
		//}
		//while (charCode != 'a' && charCode != 's' && charCode != 'd' && charCode != 'w');
		//board_state[CONVERT_COORD(opPos.x, opPos.y)] = BLOCK_PLAYER_2_TRAIL;
		//switch (charCode)
		//{
		//case 'a':
		//	opPos.x--;
		//	break;
		//case 'w':
		//	opPos.y--;
		//	break;
		//case 'd':
		//	opPos.x++;
		//	break;
		//case 's':
		//	opPos.y++;
		//	break;
		//default:
		//	break;
		//}
		//board_state[CONVERT_COORD(opPos.x, opPos.y)] = BLOCK_PLAYER_2;

		printf("Press Enter to move next ");
		while (getchar() != '\n');

		GetSystemTimeAsFileTime(&ft_now);
		ll_now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);

		dir = AiMove(board_state, opPos, myPos);

		GetSystemTimeAsFileTime(&ft_now);
		ll_now2 = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);
		printf("Player2: time = %lld, dir = %d\n", (ll_now2 - ll_now) / 10000, dir);
		board_state[CONVERT_COORD(opPos.x, opPos.y)] = BLOCK_PLAYER_2_TRAIL;
		switch (dir)
		{
		case 1:
			opPos.x--;
			break;
		case 2:
			opPos.y--;
			break;
		case 3:
			opPos.x++;
			break;
		case 4:
			opPos.y++;
			break;
		default:
			break;
		}
		board_state[CONVERT_COORD(opPos.x, opPos.y)] = BLOCK_PLAYER_2;

		for (int y = 0; y < MAP_SIZE; ++y)
		{
			for (int x = 0; x < MAP_SIZE; ++x)
			{
				printf("%d  ", board_state[CONVERT_COORD(x, y)]);
			}
			printf("\n\n");
		}
	}

}

inline int atPos_ia(unsigned int* b, int x, int y)
{
	return (b[x] & (1 << y));
}

inline void set0AtPos_ia(unsigned int* b, int x, int y)
{
	b[x] = b[x] & (~(1 << y));
}

inline void makeAvaiAtPos_ia(unsigned int* b, int x, int y)
{
	b[x] = b[x] | (1 << y);
}

inline void findAvailableDir_ia(unsigned int* b, int &d1, int &d2, int &d3, int &d4, int x, int y)
{
	d1 = y > 0 ? (atPos_ia(b, x, y - 1) == 0 ? 0 : 1) : 0;
	d2 = x > 0 ? (atPos_ia(b, x - 1, y) == 0 ? 0 : 2) : 0;
	d3 = y < MAP_SIZE - 1 ? (atPos_ia(b, x, y + 1) == 0 ? 0 : 3) : 0;
	d4 = x < MAP_SIZE - 1 ? (atPos_ia(b, x + 1, y) == 0 ? 0 : 4) : 0;
}

int deepMove_ia(unsigned int* b, int x, int y, int depth, int maxDepth, bool returnDirection)
{
	int d1 = 0, d2 = 0, d3 = 0, d4 = 0;
	findAvailableDir_ia(b, d1, d2, d3, d4, x, y);

	if ((d1 | d2 | d3 | d4) == 0 || depth >= maxDepth) // terminate condition meets
	{
		return depth;
	}

	int max = -1;
	int dir = 1;

	if (d1)
	{
		set0AtPos_ia(b, x, y - 1);
		max = deepMove_ia(b, x, y - 1, depth + 1, maxDepth, false);
		makeAvaiAtPos_ia(b, x, y - 1);
	}

	if (d2)
	{
		set0AtPos_ia(b, x - 1, y);
		int v = deepMove_ia(b, x - 1, y, depth + 1, maxDepth, false);
		makeAvaiAtPos_ia(b, x - 1, y);
		if (v > max)
		{
			max = v;
			dir = 2;
		}
	}

	if (d3)
	{
		set0AtPos_ia(b, x, y + 1);
		int v = deepMove_ia(b, x, y + 1, depth + 1, maxDepth, false);
		makeAvaiAtPos_ia(b, x, y + 1);
		if (v > max)
		{
			max = v;
			dir = 3;
		}
	}

	if (d4)
	{
		set0AtPos_ia(b, x + 1, y);
		int v = deepMove_ia(b, x + 1, y, depth + 1, maxDepth, false);
		makeAvaiAtPos_ia(b, x + 1, y);
		if (v > max)
		{
			max = v;
			dir = 4;
		}
	}

	//reclaimBoards(next);

	return returnDirection ? dir : max;
}

void test2()
{
	char buf[10];

	gets(buf);

	int k = atoi(buf);

	FILETIME ft_now;
	GetSystemTimeAsFileTime(&ft_now);
	LONGLONG ll_now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);

	//nonRecursive1(k);

	GetSystemTimeAsFileTime(&ft_now);
	LONGLONG ll_now2 = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);

	printf("Use for loop t = %lld\n", ll_now2 - ll_now);

	GetSystemTimeAsFileTime(&ft_now);
	ll_now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);

	//recursive1(k);

	GetSystemTimeAsFileTime(&ft_now);
	ll_now2 = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);

	printf("Use recursive t = %lld\n", ll_now2 - ll_now);
	getchar();
}

#endif

////////////////////////////////////////////////////////////
//                DON'T TOUCH THIS PART                   //
////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
#if _DEBUG
	test();
#endif

	srand(clock());
	
#ifdef _WIN32
    INT rc;
    WSADATA wsaData;

    rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc) {
        printf("WSAStartup Failed.\n");
        return 1;
    }
#endif

	Game::CreateInstance();
	Game * p_Game = Game::GetInstance();
	
	// Create connection
	if (p_Game->Connect(argc, argv) == -1)
	{
		LOG("Failed to connect to server!\n");
		return -1;
	}

	// Set up function pointer
	AI::GetInstance()->Update = &AI_Update;
	
	// Polling every 100ms until the connection is dead
    p_Game->PollingFromServer(100);

	Game::DestroyInstance();

#ifdef _WIN32
    WSACleanup();
#endif
	return 0;
}