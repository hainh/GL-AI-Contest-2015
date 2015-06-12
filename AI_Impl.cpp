
#include <assert.h>

#include "AI_Impl.h"

int board::count = 0;

#if !_DEBUG
inline int at(int x, int y);
void initBoards();
inline board* allocBoards(board* copy);
inline void reclaimBoards(board* b);
#endif

inline int atPos(board* b, int x, int y)
{
	return (b->cells[x] & (1 << y)) == 0 ? 0 : 1;
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
		count = 10;
		initialized = true;
	}
	else // extend list
	{
		count = 5;
	}
	board* temp;
	for (int i = 0; i < count; ++i)
	{
		temp = new board;
		temp->next = node;
		node = temp;
	}
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
			if (b[CONVERT_COORD(j, i)] != BLOCK_EMPTY)
			{
				set0AtPos(pRet, i, j);
			}
		}
	}
	return pRet;
}

// AI implementation //
/*Heuristic function:*/
int countMoves(board* b, int x, int y);

/*Heuristic function:*/
int evaluateBoard(board* b, const Position& myPos, const Position& opPos);

/* Find available direction */
void findAvailableDir(board* b, int &d1, int &d2, int &d3, int &d4, int x, int y)
{
	d1 = y > 0 ? atPos(b, x, y - 1) : 0;
	d2 = x > 0 ? atPos(b, x - 1, y) : 0;
	d3 = y < MAP_SIZE - 1 ? atPos(b, x, y + 1) : 0;
	d4 = x < MAP_SIZE - 1 ? atPos(b, x + 1, y) : 0;
}

inline void moveMe(board* b, const int d, int &resultDir, Position &my,const Position &opp, int &alpha, int &beta, int &ev, int depth, bool &prunned)
{
	board* next = allocBoards(b);
	set0AtPos(next, my.x, my.y);
	int t = abp(next, my, opp, depth - 1, alpha, beta, false, false);
	ev = MAXXY(ev, t);
	if (ev > alpha)
	{
		alpha = ev;
		resultDir = d;
	}
	if (beta <= alpha)
	{
		prunned = true;
	}
	reclaimBoards(next);
}

inline void moveOpp(board* b, const Position &my, Position &opp, int &alpha, int &beta, int &ev, int depth, bool &prunned)
{
	board* next = allocBoards(b);
	set0AtPos(next, opp.x, opp.y);
	int t = abp(next, my, opp, depth - 1, alpha, beta, true, false);
	ev = MINXY(ev, t);
	beta = MINXY(beta, ev);
	if (beta <= alpha)
	{
		prunned = true;
	}
	reclaimBoards(next);
}

int abp(board* b, const Position myPos, const Position opPos, int depth, int alpha, int beta, bool maximizePlayer, bool returnDirection)
{
	if (depth == 0)
	{
		return evaluateBoard(b, myPos, opPos);
	}

	int x = myPos.x;
	int y = myPos.y;
	int ox = opPos.x;
	int oy = opPos.y;
	int d1 = 0, d2 = 0, d3 = 0, d4 = 0;
	int e1 = 0, e2 = 0, e3 = 0, e4 = 0;
	findAvailableDir(b, d1, d2, d3, d4, x, y);
	findAvailableDir(b, e1, e2, e3, e4, ox, oy);

	if ((d1 | d2 | d3 | d4) == 0) // terminate condition meets
	{
		return -1000;
	}

	if ((e1 | e2 | e3 | e4) == 0) // terminate condition meets
	{
		return 1000;
	}

	if (maximizePlayer)
	{
		int ev = MIN_INT;
		bool prunned = false;
		int dir = 0;
		if (d1 > 0)
		{
			moveMe(b, 1, dir, Position(x, y - 1), opPos, alpha, beta, ev, depth, prunned);
		}
		if (!prunned && d2 > 0)
		{
			moveMe(b, 2, dir, Position(x - 1, y), opPos, alpha, beta, ev, depth, prunned);
		}
		if (!prunned && d3 > 0)
		{
			moveMe(b, 3, dir, Position(x, y + 1), opPos, alpha, beta, ev, depth, prunned);
		}
		if (!prunned && d4 > 0)
		{
			moveMe(b, 4, dir, Position(x + 1, y), opPos, alpha, beta, ev, depth, prunned);
		}

		return returnDirection ? dir : alpha;
	}
	else
	{
		int ev = MAX_INT;
		bool prunned = false;
		if (e1 > 0)
		{
			moveOpp(b, myPos, Position(ox, oy - 1), alpha, beta, ev, depth, prunned);
		}
		if (!prunned && e2 > 0)
		{
			moveOpp(b, myPos, Position(ox - 1, oy), alpha, beta, ev, depth, prunned);
		}
		if (!prunned && e3 > 0)
		{
			moveOpp(b, myPos, Position(ox, oy + 1), alpha, beta, ev, depth, prunned);
		}
		if (!prunned && e4 > 0)
		{
			moveOpp(b, myPos, Position(ox + 1, oy), alpha, beta, ev, depth, prunned);
		}

		return beta;
	}
}

int evaluateBoard(board* b, const Position& myPos, const Position& opPos)
{
	return countMoves(b, myPos.x, myPos.y) - countMoves(b, opPos.x, opPos.y);
}

int countMoves(board* b, int x, int y)
{
	int res = 0;
	int a = MAXXY(0, x - 1), ma = MINXY(x + 2, MAP_SIZE);
	int bb = MAXXY(0, y - 1), mb = MINXY(y + 2, MAP_SIZE);
	for (; a < ma; ++a)
	{
		for (int i = bb; i < mb; ++i)
		{
			res += atPos(b, a, i);
		}
	}

	return res;
}