
#include <assert.h>

#include "AI_Impl.h"

#define POSITIVE -1000
#define DISTANCE_SQR(x1, y1, x2, y2) (((x1) - (x2)) * ((x1) - (x2)) + ((y1) - (y2)) * ((y1) - (y2)))

#if _DEBUG
int board::count = 0;
#endif

#if !_DEBUG
inline int at(int x, int y);
void initBoards();
inline board* allocBoards(board* copy);
inline void reclaimBoards(board* b);
#endif

inline void sortFour(int &a, int &b, int &c, int &d, int &aa, int &bb, int &cc, int &dd)
{
	int t;
	if (a > b)
	{
		t = a; a = b; b = t;
		t = aa; aa = bb; bb = t;
	}

	if (c > d)
	{
		t = c; c = d; d = t;
		t = cc; cc = dd; dd = t;
	}

	// lowest = a, middle1 = c
	if (a > c)
	{
		t = a; a = c; c = t;
		t = aa; aa = cc; cc = t;
	}

	// highest = d, middle2 = b
	if (b > d)
	{
		t = b; b = d; d = t;
		t = bb; bb = dd; dd = t;
	}

	// sort middle1 and middle2
	if (b > c)
	{
		t = b; b = c; c = t;
		t = bb; bb = cc; cc = t;
	}
}

inline int atPos(board* b, int x, int y)
{
	return (b->cells[x] & (1 << y));
}

inline void set0AtPos(board* b, int x, int y)
{
	b->cells[x] = b->cells[x] & (~(1 << y));
}

board* node = nullptr;
static int initialized = false;

void initBoards()
{
	int count;
	if (!initialized)
	{
		count = 100;
		initialized = true;
	}
	else // extend list
	{
		count = 50;
	}
	board* temp;
	for (int i = 0; i < count; ++i)
	{
		temp = new board;
		temp->next = node;
		node = temp;
	}
#if _DEBUG
	printf("Init %d objects", board::count);
#endif
}

board* allocBoards(board* copy)
{
	if (node == nullptr)
	{
		initBoards();
	}

	board* ret = node;
	node = node->next;

	if (copy)
	{
		ret->cells[0] = copy->cells[0];
		ret->cells[1] = copy->cells[1];
		ret->cells[2] = copy->cells[2];
		ret->cells[3] = copy->cells[3];
		ret->cells[4] = copy->cells[4];
		ret->cells[5] = copy->cells[5];
		ret->cells[6] = copy->cells[6];
		ret->cells[7] = copy->cells[7];
		ret->cells[8] = copy->cells[8];
		ret->cells[9] = copy->cells[9];
		ret->cells[10] = copy->cells[10];
	}

	return ret;
}

void reclaimBoards(board* b)
{
	b->next = node;
	node = b;
}

board* copyFrom(int* b)
{
	board* pRet = allocBoards(nullptr);
	for (size_t i = 0; i < MAP_SIZE; i++)
	{
		pRet->cells[i] = 0x00000fff;
	}

	for (size_t i = 0; i < MAP_SIZE; i++)
	{
		for (size_t j = 0; j < MAP_SIZE; j++)
		{
#if _DEBUG
			//int v = b[CONVERT_COORD(j, i)];
			//printf("%d ", v);
#endif
			if (b[CONVERT_COORD(j, i)] != BLOCK_EMPTY)
			{
				set0AtPos(pRet, i, j);
			}
		}
#if _DEBUG
		//printf("\n");
#endif
	}
	return pRet;
}

// AI implementation //
/*Heuristic function:*/
int countPosibleMoves(board* b, int x, int y);

/*Heuristic function:*/
inline int evaluateBoard(board* b, const Position& myPos, const Position& opPos, const bool & maximizePlayer);

/* Find available direction */
inline void findAvailableDir(board* b, int &d1, int &d2, int &d3, int &d4, int x, int y)
{
	d1 = y > 0 ? (atPos(b, x, y - 1) == 0 ? 0 : 1) : 0;
	d2 = x > 0 ? (atPos(b, x - 1, y) == 0 ? 0 : 2) : 0;
	d3 = y < MAP_SIZE - 1 ? (atPos(b, x, y + 1) == 0 ? 0 : 3) : 0;
	d4 = x < MAP_SIZE - 1 ? (atPos(b, x + 1, y) == 0 ? 0 : 4) : 0;
}

inline void moveMe(board* b, const int d, int &resultDir, Position &my,const Position &opp, int &v, int &alpha, int &beta, int depth, bool &prunned)
{
	board* next = allocBoards(b);
	set0AtPos(next, my.x, my.y);
	int t = abp(next, my, opp, depth - 1, alpha, beta, false, false);
	v = MAXXY(v, t);
	if (v > alpha)
	{
		alpha = v;
		resultDir = d;
	}
	if (beta <= alpha)
	{
		prunned = true;
	}
	reclaimBoards(next);
}

inline void moveOpp(board* b, const Position &my, Position &opp, int &v, int &alpha, int &beta, int depth, bool &prunned)
{
	board* next = allocBoards(b);
	set0AtPos(next, opp.x, opp.y);
	int t = abp(next, my, opp, depth - 1, alpha, beta, true, false);
	v = MINXY(v, t);
	beta = MINXY(beta, v);
	if (beta <= alpha)
	{
		prunned = true;
	}
	reclaimBoards(next);
}

int deepMove(board* b, const Position &pos, int depth, bool returnDirection)
{
	int x = pos.x, y = pos.y;
	int d1 = 0, d2 = 0, d3 = 0, d4 = 0;
	findAvailableDir(b, d1, d2, d3, d4, x, y);

	if ((d1 | d2 | d3 | d4) == 0) // terminate condition meets
	{
		return depth;
	}

	int max;
	int dir = 1;

	if (d1)
	{
		board* next = allocBoards(b);
		set0AtPos(next, x, y - 1);
		max = deepMove(next, Position(x, y - 1), depth + 1, false);
		reclaimBoards(next);
	}

	if (d2)
	{
		board* next = allocBoards(b);
		set0AtPos(next, x - 1, y);
		int v = deepMove(next, Position(x - 1, y), depth + 1, false);
		if (v > max)
		{
			max = v;
			dir = 2;
		}
		reclaimBoards(next);
	}
	
	if (d3)
	{
		board* next = allocBoards(b);
		set0AtPos(next, x, y + 1);
		int v = deepMove(next, Position(x, y + 1), depth + 1, false);
		if (v > max)
		{
			max = v;
			dir = 3;
		}
		reclaimBoards(next);
	}

	if (d4)
	{
		board* next = allocBoards(b);
		set0AtPos(next, x + 1, y);
		int v = deepMove(next, Position(x + 1, y), depth + 1, false);
		if (v > max)
		{
			max = v;
			dir = 4;
		}
		reclaimBoards(next);
	}

	return returnDirection ? dir : max;
}

int abp(board* b, const Position &myPos, const Position &opPos, int depth, int alpha, int beta, bool maximizePlayer, bool returnDirection)
{
	if (depth == 0)
	{
		return evaluateBoard(b, myPos, opPos, maximizePlayer);
	}

	int x = myPos.x;
	int y = myPos.y;
	int ox = opPos.x;
	int oy = opPos.y;
	int d1 = 0, d2 = 0, d3 = 0, d4 = 0;
	int e1 = 0, e2 = 0, e3 = 0, e4 = 0;
	findAvailableDir(b, d1, d2, d3, d4, x, y);
	findAvailableDir(b, e1, e2, e3, e4, ox, oy);

	if (maximizePlayer && (d1 | d2 | d3 | d4) == 0) // terminate condition meets
	{
		return -1000 - depth;
	}

	if (!maximizePlayer && (e1 | e2 | e3 | e4) == 0) // terminate condition meets
	{
		return 1000 - depth;
	}

	int dirOrderDistance[4];
	int dx[5] = { 0, /*start here*/0, -1, 0, 1 };
	int dy[5] = { 0, /*start here*/-1, 0, 1, 0 };

	// Prepair order of moves
	dirOrderDistance[0] = DISTANCE_SQR(x, y - 1, ox, oy);// | (1 << 16);
	dirOrderDistance[1] = DISTANCE_SQR(x - 1, y, ox, oy);// | (2 << 16);
	dirOrderDistance[2] = DISTANCE_SQR(x, y + 1, ox, oy);// | (3 << 16);
	dirOrderDistance[3] = DISTANCE_SQR(x + 1, y, ox, oy);// | (4 << 16);


	if (maximizePlayer)
	{
		bool prunned = false;
		int dir = 0;
		int v = -1000000;

		sortFour(dirOrderDistance[0], dirOrderDistance[1], dirOrderDistance[2], dirOrderDistance[3], d1, d2, d3, d4);

		if (!prunned && d1 > 0)
		{
			moveMe(b, 1, dir, Position(x + dx[d1], y + dy[d1]), opPos, v, alpha, beta, depth, prunned);
		}
		if (!prunned && d2 > 0)
		{
			moveMe(b, 2, dir, Position(x + dx[d2], y + dy[d2]), opPos, v, alpha, beta, depth, prunned);
		}
		if (!prunned && d3 > 0)
		{
			moveMe(b, 3, dir, Position(x + dx[d3], y + dy[d3]), opPos, v, alpha, beta, depth, prunned);
		}
		if (!prunned && d4 > 0)
		{
			moveMe(b, 4, dir, Position(x + dx[d4], y + dy[d4]), opPos, v, alpha, beta, depth, prunned);
		}

		return returnDirection ? dir : v;
	}
	else
	{
		bool prunned = false;
		int v = 1000000;

		sortFour(dirOrderDistance[2], dirOrderDistance[3], dirOrderDistance[0], dirOrderDistance[1], e1, e2, e3, e4);

		if (e1 > 0)
		{
			moveOpp(b, myPos, Position(ox + dx[e1], oy + dy[e1]), v, alpha, beta, depth, prunned);
		}
		if (!prunned && e2 > 0)
		{
			moveOpp(b, myPos, Position(ox + dx[e2], oy + dy[e2]), v, alpha, beta, depth, prunned);
		}
		if (!prunned && e3 > 0)
		{
			moveOpp(b, myPos, Position(ox + dx[e3], oy + dy[e3]), v, alpha, beta, depth, prunned);
		}
		if (!prunned && e4 > 0)
		{
			moveOpp(b, myPos, Position(ox + dx[e4], oy + dy[e4]), v, alpha, beta, depth, prunned);
		}

		return v;
	}
}

int searchBoard[MAP_SIZE][MAP_SIZE];

void copyToSearchBoard(board* b)
{
	for (int i = 0; i < MAP_SIZE; ++i)
	{
		for (int j = 0; j < MAP_SIZE; ++j)
		{
			searchBoard[i][j] = atPos(b, i, j) == 0 ? 0 : 1;
		}
	}
}

/*
Count all posible moves if one player's position is (x, y) using deep-first search algorithm
*/
int countPosibleMoves(int x, int y)
{
	int count = 0;

	// Found opponent
	if (searchBoard[x][y] == 2)
	{
		count = POSITIVE;
	}

	searchBoard[x][y] = 0;
	if (x > 0)
	{
		if (y > 0 && searchBoard[x - 1][y - 1])
		{
			++count;
			count += countPosibleMoves(x - 1, y - 1);
		}
		if (searchBoard[x - 1][y])
		{
			++count;
			count += countPosibleMoves(x - 1, y);
		}
		if (y < MAP_SIZE - 1 && searchBoard[x - 1][y + 1])
		{
			++count;
			count += countPosibleMoves(x - 1, y + 1);
		}
	}

	if (y > 0 && searchBoard[x][y - 1])
	{
		++count;
		count += countPosibleMoves(x, y - 1);
	}
	if (y < MAP_SIZE - 1 && searchBoard[x][y + 1])
	{
		++count;
		count += countPosibleMoves(x, y + 1);
	}

	if (x < MAP_SIZE - 1)
	{
		if (y > 0 && searchBoard[x + 1][y - 1])
		{
			++count;
			count += countPosibleMoves(x + 1, y - 1);
		}
		if (searchBoard[x + 1][y])
		{
			++count;
			count += countPosibleMoves(x + 1, y);
		}
		if (y < MAP_SIZE - 1 && searchBoard[x + 1][y + 1])
		{
			++count;
			count += countPosibleMoves(x + 1, y + 1);
		}
	}

	return count;
}

int evaluateBoard(board* b, const Position& myPos, const Position& opPos, const bool & maximizePlayer)
{
	copyToSearchBoard(b);
	searchBoard[myPos.x][myPos.y] = 0;
	searchBoard[opPos.x][opPos.y] = 2;

	int myMovableCount = countPosibleMoves(myPos.x, myPos.y);
	// Reset search board if my position and opponent position are in same plane (they can be connected by some moves)
	// because searching for my moves sets all position available to 0.
	// This is applicable only as my position split all my available moves to 2 parts.
	if (myMovableCount < 0)
	{
		//copyToSearchBoard(b);
		return -1000;
	}
	int opponentMovableCount = countPosibleMoves(opPos.x, opPos.y);

	// Unreachable now
	if (myMovableCount < 0)
	{
		// Reconstruct my moves
		myMovableCount = myMovableCount - POSITIVE - 1; // minus 1: position of opponent is set to 2 before
		if (myMovableCount > opponentMovableCount * 2) // I can absolutely win
		{
			return 1000;
		}

		return 0;
	}

	return myMovableCount == opponentMovableCount ? 0 : (myMovableCount > opponentMovableCount ? 1000 : -1000);
}

/*
  Count all moves that I made
*/
int countMoved(const int* const board)
{
	int moves = 0;
	int stt = 0;
	for (int x = 0; x < MAP_SIZE; ++x)
	{
		for (int y = 0; y < MAP_SIZE; ++y)
		{
			stt = board[CONVERT_COORD(x, y)];
			if (stt != BLOCK_EMPTY && stt != BLOCK_OBSTACLE)
			{
				++moves;
			}
		}
	}

	return moves / 2;
}

void simulateMove(int* b, int dir, int &x, int &y)
{
	switch (dir)
	{
	case 1:
		--x;
		break;
	case 2:
		--y;
		break;
	case 3:
		++x;
		break;
	case 4:
		++y;
		break;
	default:
		break;
	}
	b[CONVERT_COORD(x, y)] = 1;
}

void undoMove(int* b, int dir, int &x, int &y)
{
	b[CONVERT_COORD(x, y)] = 0;
	switch (dir)
	{
	case 3:
		--x;
		break;
	case 4:
		--y;
		break;
	case 1:
		++x;
		break;
	case 2:
		++y;
		break;
	default:
		break;
	}
}

/*
  go to (4, 4) with downward tendency
*/
int searchDirFromUpperLeftCorner(int* b_cpy, const Position &myPos, int moves)
{
	int x1 = myPos.x, y1 = myPos.y;
	int x2 = x1, y2 = y1;
	++y1; // down
	++x2; // right
	if (b_cpy[CONVERT_COORD(x1, y1)] == 0 && b_cpy[CONVERT_COORD(x2, y2)] == 0)
	{
		if (myPos.x == myPos.y)
		{
			return DIRECTION_DOWN;
		}
		//if (myPos.y < 4)
		//{
		//	return rand() % 1000 < 700 ? DIRECTION_DOWN : DIRECTION_RIGHT; // 70% of going DOWN
		//}
		int d01 = x1 * x1 + y1 * y1;
		int d13 = DISTANCE_SQR(x1, y1, 5, 5);
		int d02 = x2 * x2 + y2 * y2;
		int d23 = DISTANCE_SQR(x2, y2, 5, 5);
		int s = d01 + d13 - (d02 + d23);
		if (s < 0)
		{
			return DIRECTION_DOWN; // decide to go down to (x1, y1)
		}
		else if (s > 0)
		{
			return DIRECTION_RIGHT; // decide to go right to (x2, y2)
		}
		else // 
		{
			printf("You say it cannot reach hear!!??");
			return rand() % 1000 < 500 ? DIRECTION_DOWN : DIRECTION_RIGHT; // 80% of going DOWN
		}
	}

	return b_cpy[CONVERT_COORD(x1, y1)] == 0 ? DIRECTION_DOWN : DIRECTION_RIGHT;
}

/*
go to (4, 4) with downward tendency
*/
int searchDirFromBottomRightCorner(int* b_cpy, const Position &myPos, int moves)
{
	int x1 = myPos.x, y1 = myPos.y;
	int x2 = x1, y2 = y1;
	--y1; // p1 -> up
	--x2; // p2 -> left
	if (b_cpy[CONVERT_COORD(x1, y1)] == 0 && b_cpy[CONVERT_COORD(x2, y2)] == 0)
	{
		if (myPos.x == myPos.y)
		{
			return DIRECTION_UP;
		}
		//if (myPos.y > 6)
		//{
		//	return rand() % 1000 < 700 ? DIRECTION_UP : DIRECTION_LEFT; // 70% of going UP
		//}

		int d01 = DISTANCE_SQR(x1, y1, MAP_SIZE - 1, MAP_SIZE - 1);
		int d13 = DISTANCE_SQR(x1, y1, 5, 5);
		int d02 = DISTANCE_SQR(x2, y2, MAP_SIZE - 1, MAP_SIZE - 1);
		int d23 = DISTANCE_SQR(x2, y2, 5, 5);
		int s = d01 + d13 - (d02 + d23);
		if (s < 0)
		{
			return DIRECTION_UP; // decide to go up to (x1, y1)
		}
		else if (s > 0)
		{
			return DIRECTION_LEFT; // decide to go left to (x2, y2)
		}
		else
		{
			printf("You say it cannot reach hear!!??");
			return rand() % 1000 < 800 ? DIRECTION_UP : DIRECTION_LEFT; // 80% of going UP
		}
	}

	return b_cpy[CONVERT_COORD(x1, y1)] == 0 ? DIRECTION_UP : DIRECTION_LEFT;
}

int searchBestDirFromUpperLeft(int* board, const Position &myPos, int moves)
{
	int x = myPos.x, y = myPos.y;
	if (x == y && board[CONVERT_COORD(x, y + 1)] == 0 && board[CONVERT_COORD(x + 1, y)] == 0)
	{
		int length;
		int pre_moves[5];
		int count = 0;
		// test go down
		pre_moves[count++] = DIRECTION_DOWN;
		simulateMove(board, DIRECTION_DOWN, x, y);
		length = DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		pre_moves[count++] = searchDirFromUpperLeftCorner(board, Position(x, y), moves);
		simulateMove(board, pre_moves[count - 1], x, y);
		length += DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		pre_moves[count++] = searchDirFromUpperLeftCorner(board, Position(x, y), moves);
		simulateMove(board, pre_moves[count - 1], x, y);
		length += DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		pre_moves[count++] = searchDirFromUpperLeftCorner(board, Position(x, y), moves);
		simulateMove(board, pre_moves[count - 1], x, y);
		length += DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		pre_moves[count++] = searchDirFromUpperLeftCorner(board, Position(x, y), moves);
		simulateMove(board, pre_moves[count - 1], x, y);
		length += DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		undoMove(board, pre_moves[--count], x, y);
		undoMove(board, pre_moves[--count], x, y);
		undoMove(board, pre_moves[--count], x, y);
		undoMove(board, pre_moves[--count], x, y);
		undoMove(board, pre_moves[--count], x, y);

		// test go right
		simulateMove(board, DIRECTION_RIGHT, x, y);
		length -= DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		simulateMove(board, searchDirFromUpperLeftCorner(board, Position(x, y), moves), x, y);
		length -= DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		simulateMove(board, searchDirFromUpperLeftCorner(board, Position(x, y), moves), x, y);
		length -= DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		simulateMove(board, searchDirFromUpperLeftCorner(board, Position(x, y), moves), x, y);
		length -= DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		simulateMove(board, searchDirFromUpperLeftCorner(board, Position(x, y), moves), x, y);
		length -= DISTANCE_SQR(0, 0, x, y) + DISTANCE_SQR(x, y, 5, 5);

		// Oke then check shorter move
		if (length > 0) // go right is shorter
		{
			return DIRECTION_RIGHT;
		}
		return DIRECTION_DOWN; // prefer going down if equal
	}

	return searchDirFromUpperLeftCorner(board, myPos, moves);
}

int searchBestDirFromBottomRight(int* board, const Position &myPos, int moves)
{
	int x = myPos.x, y = myPos.y;
	if (x == y && board[CONVERT_COORD(x, y - 1)] == 0 && board[CONVERT_COORD(x - 1, y)] == 0)
	{
		int length;
		int pre_moves[5];
		int count = 0;
		// test go UP
		pre_moves[count++] = DIRECTION_UP;
		simulateMove(board, DIRECTION_UP, x, y);
		length = DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		pre_moves[count++] = searchDirFromBottomRightCorner(board, Position(x, y), moves);
		simulateMove(board, pre_moves[count - 1], x, y);
		length += DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		pre_moves[count++] = searchDirFromBottomRightCorner(board, Position(x, y), moves);
		simulateMove(board, pre_moves[count - 1], x, y);
		length += DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		pre_moves[count++] = searchDirFromBottomRightCorner(board, Position(x, y), moves);
		simulateMove(board, pre_moves[count - 1], x, y);
		length += DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		pre_moves[count++] = searchDirFromBottomRightCorner(board, Position(x, y), moves);
		simulateMove(board, pre_moves[count - 1], x, y);
		length += DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		undoMove(board, pre_moves[--count], x, y);
		undoMove(board, pre_moves[--count], x, y);
		undoMove(board, pre_moves[--count], x, y);
		undoMove(board, pre_moves[--count], x, y);
		undoMove(board, pre_moves[--count], x, y);

		// test go LEFT
		simulateMove(board, DIRECTION_LEFT, x, y);
		length -= DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		simulateMove(board, searchDirFromBottomRightCorner(board, Position(x, y), moves), x, y);
		length -= DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		simulateMove(board, searchDirFromBottomRightCorner(board, Position(x, y), moves), x, y);
		length -= DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		simulateMove(board, searchDirFromBottomRightCorner(board, Position(x, y), moves), x, y);
		length -= DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		simulateMove(board, searchDirFromBottomRightCorner(board, Position(x, y), moves), x, y);
		length -= DISTANCE_SQR(MAP_SIZE - 1, MAP_SIZE - 1, x, y) + DISTANCE_SQR(x, y, 5, 5);

		// Oke then check shorter move
		if (length > 0) // go left is shorter
		{
			return DIRECTION_LEFT;
		}
		return DIRECTION_UP; // prefer going up if equal
	}

	return searchDirFromBottomRightCorner(board, myPos, moves);
}

/*
 Split the square to 2 triangle with a diagonal through start positions of 2 player. We have upper triangle is one with a
 corner is at top right, the other called lower triangle.
 Move with strategy:
   In 10 first moves:
     All moves are in the diagonal or closest to the diagonal.
     Minimize blank positions of triangle where enemy stand on.
   Next moves: Use our AI algorithm.
*/
int AiMove(int* origBoard, const Position &myPos, const Position &opPos)
{
	int allMovesCount = countMoved(origBoard);
	bool useLogic = allMovesCount <= 10;
	int depth = 0;

	if (useLogic && allMovesCount >= 9)
	{
		board *b = copyFrom(origBoard);
		copyToSearchBoard(b);
		int posibleMoves = countPosibleMoves(MAP_SIZE - 2, 1);
		if (posibleMoves < 60) // play board is splitted 2 regions
		{
			useLogic = false;
			depth = 40;//MAXXY(posibleMoves, MAP_SIZE * MAP_SIZE - 10 - allMovesCount * 2 - 1 - posibleMoves);
		}
	}

	if (useLogic)
	{
		bool upper = origBoard[0] == origBoard[CONVERT_COORD(myPos.x, myPos.y)]
			|| origBoard[0] == origBoard[CONVERT_COORD(myPos.x, myPos.y)] + 1;
		
		bool criticalUpper = myPos.x == 4 && myPos.y == 4 && origBoard[CONVERT_COORD(4, 5)] == BLOCK_EMPTY && origBoard[CONVERT_COORD(5, 4)] == BLOCK_EMPTY;
		bool criticalLower = myPos.x == 6 && myPos.y == 6 && origBoard[CONVERT_COORD(4, 5)] == BLOCK_EMPTY && origBoard[CONVERT_COORD(5, 4)] == BLOCK_EMPTY;

		if (criticalLower || criticalUpper)
		{
			int countEmptyUpper = 0, countEmptyLower = 0;
			for (int x = 0; x < MAP_SIZE; ++x)
			{
				for (int y = 0; y < MAP_SIZE; ++y)
				{
					if (x < y && origBoard[CONVERT_COORD(x, y)] == BLOCK_EMPTY)
					{
						++countEmptyLower;
					}
					else if (x > y && origBoard[CONVERT_COORD(x, y)] == BLOCK_EMPTY)
					{
						++countEmptyUpper;
					}
				}
			}

			if (criticalUpper)
			{
				if (countEmptyUpper > countEmptyLower)
				{
					return DIRECTION_RIGHT;
				}
				return DIRECTION_DOWN;
			}
			else // if criticalLower is true
			{
				if (countEmptyLower > countEmptyUpper)
				{
					return DIRECTION_LEFT;
				}
				return DIRECTION_UP;
			}
		}

		int b_cpy[MAP_SIZE*MAP_SIZE];
		memcpy(b_cpy, origBoard, sizeof(b_cpy));

		//for (int y = 0; y < MAP_SIZE; ++y)
		//{
		//	for (int x = 0; x < MAP_SIZE; ++x)
		//	{
		//		if (upper)
		//		{
		//			if (x > 6 || y > 6)
		//			{
		//				b_cpy[CONVERT_COORD(x, y)] = 1;
		//			}
		//		}
		//		else
		//		{
		//			if (x < 4 || y < 4)
		//			{
		//				b_cpy[CONVERT_COORD(x, y)] = 1;
		//			}
		//		}
		//	}
		//}
		//if (upper)
		//{
		//	b_cpy[CONVERT_COORD(6, 6)] = 1;
		//}
		//else
		//{
		//	b_cpy[CONVERT_COORD(4, 4)] = 1;
		//}

		// Patch dead position (cannot move forward if stand on this position)
		if (upper)
		{
			for (int x = 5; x > 0; --x)
			{
				for (int y = 5; y > 0; --y)
				{
					if (b_cpy[CONVERT_COORD(x, y)] && b_cpy[CONVERT_COORD(x - 1, y + 1)])
					{
						b_cpy[CONVERT_COORD(x - 1, y)] = 1;
					}
				}
			}
		}
		else
		{
			for (int x = 6; x < MAP_SIZE - 1; ++x)
			{
				for (int y = 6; y < MAP_SIZE - 1; ++y)
				{
					if (b_cpy[CONVERT_COORD(x, y)] && b_cpy[CONVERT_COORD(x - 1, y + 1)])
					{
						b_cpy[CONVERT_COORD(x, y + 1)] = 1;
					}
				}
			}
		}


		if (myPos.x == myPos.y)
		{
			if (upper)
			{
				if (b_cpy[CONVERT_COORD(myPos.x + 1, myPos.y + 1)] == BLOCK_EMPTY)
				{
					return DIRECTION_DOWN;
				}
			}
			else if (b_cpy[CONVERT_COORD(myPos.x - 1, myPos.y - 1)] == BLOCK_EMPTY)
			{
				return DIRECTION_UP;
			}
		}

		if (upper)
		{
			return searchBestDirFromUpperLeft(b_cpy, myPos, allMovesCount);
		}
		else
		{
			return searchBestDirFromBottomRight(b_cpy, myPos, allMovesCount);
		}
	}
	else // Oke then use smart algorithm to finish enemy
	{
		board *b = copyFrom(origBoard);

		if (depth == 0)
		{
			copyToSearchBoard(b);
			searchBoard[myPos.y][myPos.x] = 0;
			searchBoard[opPos.y][opPos.x] = 2;
			int posibleMoves = countPosibleMoves(myPos.y, myPos.x);
			if (posibleMoves < 0)
			{
				// Reconstruct my moves
				posibleMoves = posibleMoves - POSITIVE - 1; // minus 1: position of opponent is set to 2 before

				int distance = DISTANCE_SQR(myPos.x, myPos.y, opPos.x, opPos.y);
				if (distance <= 2)
				{
					depth = posibleMoves;
				}

				if (posibleMoves < 60)
				{
					depth = 50;
				}
				else
				{
					depth = 35;
				}
			}
			else
			{
				depth = posibleMoves;
			}
		}
		printf(", depth = %d", depth);
		int direction = abp(b, Position(myPos.y, myPos.x), Position(opPos.y, opPos.x), depth, MIN_INT, MAX_INT, true, true);
		return direction;
	}
}

void printBoard(board* b)
{
	printf("\n");
	for (int x = 0; x < MAP_SIZE; ++x)
	{
		for (int y = 0; y < MAP_SIZE; ++y)
		{
			printf("%d  ", atPos(b, x, y) == 0 ? 1 : 0);
		}
		printf("\n\n");
	}
}
