#ifndef _AI_IMPL_H
#define _AI_IMPL_H

#include <stdint.h>
#include "ai/AI.h"

#define MAXXY(x, y) (x > y ? x : y)
#define MINXY(x, y) (x < y ? x : y)
#define MAX_INT (1 << 30)
#define MIN_INT (-MAX_INT)

// Memory management

struct board
{
	unsigned int cells[MAP_SIZE];
	board* next;
#if _DEBUG
	static int count;
	board()
	{
		++count;
	}
	~board()
	{
		--count;
	}
#endif
};

//int countMoves(int x, int y);

void copyToSearchBoard(board* b);

void initBoards();

//typedef _board board;
#if _DEBUG
int at(int x, int y);
board* allocBoards(board* copy);
void reclaimBoards(board* b);
#endif

board* copyFrom(int *origin);

// AI implementation \\

/*
Implementation of game's AI using alpha-beta prunning
*/
int abp(board* b, const Position &myPos, const Position &opPos, int depth, int alpha, int beta, bool maximizePlayer, bool returnDirection);

int AiMove(int* origBoard, const Position &myPos, const Position &opPos);

#endif
