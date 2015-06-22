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
	//direction = toStandardDirection(direction);

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

#if !_DEBUG
void test()
{
	int board_state[] = {
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0,
		0, 5, 0, 0, 0, 0, 5, 0, 0, 0, 0,
		0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0,
		0, 0, 0, 0, 5, 0, 0, 0, 0, 5, 0,
		0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	};//               |
	//extern int evaluateBoard(board* b, const Position& myPos, const Position& opPos);
	Position myPos = Position(10, 10);
	Position opPos = Position(0, 0);

	while (true)
	{
		FILETIME ft_now;
		GetSystemTimeAsFileTime(&ft_now);
		LONGLONG ll_now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);

		int dir = AiMove(board_state, myPos, opPos);

		GetSystemTimeAsFileTime(&ft_now);
		LONGLONG ll_now2 = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);
		printf("\ntime = %lld, dir = %d\n", (ll_now2 - ll_now) / 10000, dir);
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

		int charCode = 0;
		do
		{
			printf("Your move: ");
			charCode = getchar();
			while (getchar() != '\n'){}
		}
		while (charCode != 'a' && charCode != 's' && charCode != 'd' && charCode != 'w');
		
		board_state[CONVERT_COORD(opPos.x, opPos.y)] = BLOCK_PLAYER_2_TRAIL;
		switch (charCode)
		{
		case 'a':
			opPos.x--;
			break;
		case 'w':
			opPos.y--;
			break;
		case 'd':
			opPos.x++;
			break;
		case 's':
			opPos.y++;
			break;
		default:
			break;
		}
		board_state[CONVERT_COORD(opPos.x, opPos.y)] = BLOCK_PLAYER_2;

	}

	//int move = 2;
	//for (int i = 0; i < 10; ++i)
	//{
	//	++move;
	//	FILETIME ft_now;
	//	GetSystemTimeAsFileTime(&ft_now);
	//	board* b = copyFrom(board_state);
	//	std::swap(myPos.x, myPos.y);
	//	int dir = abp(b, myPos, Position(5, 5), 35, MIN_INT, MAX_INT, true, true);
	//	std::swap(myPos.x, myPos.y);
	//	FILETIME ft_now2;
	//	GetSystemTimeAsFileTime(&ft_now2);
	//	LONGLONG ll_now = (LONGLONG)ft_now.dwLowDateTime + ((LONGLONG)(ft_now.dwHighDateTime) << 32LL);
	//	LONGLONG ll_now2 = (LONGLONG)ft_now2.dwLowDateTime + ((LONGLONG)(ft_now2.dwHighDateTime) << 32LL);
	//	printf("\ntime = %lld, dir = %d\n", (ll_now2 - ll_now) / 10000, dir);
	//	switch (dir)
	//	{
	//	case 1:
	//		myPos.x--;
	//		break;
	//	case 2:
	//		myPos.y--;
	//		break;
	//	case 3:
	//		myPos.x++;
	//		break;
	//	case 4:
	//		myPos.y++;
	//		break;
	//	default:
	//		break;
	//	}
	//	board_state[CONVERT_COORD(myPos.x, myPos.y)] = move;
	//	for (int y = 0; y < MAP_SIZE; ++y)
	//	{
	//		for (int x = 0; x < MAP_SIZE; ++x)
	//		{
	//			printf("%02d ", board_state[CONVERT_COORD(x, y)]);
	//		}
	//		printf("\n");
	//	}
	//}
	//getchar();
}
#endif

////////////////////////////////////////////////////////////
//                DON'T TOUCH THIS PART                   //
////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	initBoards();

#if !_DEBUG
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