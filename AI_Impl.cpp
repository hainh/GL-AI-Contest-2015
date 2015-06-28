
#include <assert.h>
#include <stack>

#include "AI_Impl.h"

#define POSITIVE -1000
#define DISTANCE_SQR(x1, y1, x2, y2) (((x1) - (x2)) * ((x1) - (x2)) + ((y1) - (y2)) * ((y1) - (y2)))

#if _DEBUG
int board::count = 0;
int callCountPosMoves = 0;
#endif

#if !_DEBUG
inline int at(int x, int y);
void initBoards();
inline board* allocBoards(board* copy);
inline void reclaimBoards(board* b);
#endif

int searchBoard[MAP_SIZE][MAP_SIZE];
board* originSearchBoard = nullptr;
int stuckPosCount;

bool followEnemy = true;
bool enemyAtUpper = false;
Position enemyPrevPos(-1, -1);

int countPosibleMoves(int x, int y, const bool &nonRecursiveCall);
void copyToSearchBoard(board* b);

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

void DDALine(int x1, int y1, int x2, int y2, int *board)
{
	float dX, dY, iSteps;
	float xInc, yInc, iCount, x, y;

	dX = x1 - x2;
	dY = y1 - y2;

	if (fabs(dX) > fabs(dY))
	{
		iSteps = fabs(dX);
	}
	else
	{
		iSteps = fabs(dY);
	}

	xInc = dX / iSteps;
	yInc = dY / iSteps;

	x = x1;
	y = y1;

	for (iCount = 1; iCount <= iSteps; iCount++)
	{
		board[CONVERT_COORD((int)floor(x + .5f), (int)floor(y + .5f))] = 7;
		x -= xInc;
		y -= yInc;
	}
}

inline int atPos(board* &b, int x, int y)
{
	return (b->cells[x] & (1 << y));
}

inline void set0AtPos(board* &b, int x, int y)
{
	b->cells[x] = b->cells[x] & (~(1 << y));
}

inline void makeAvaiAtPos(board* &b, int x, int y)
{
	b->cells[x] = b->cells[x] | (1 << y);
}

board* node = nullptr;
static int initialized = false;

void initBoards()
{
	int count;
	if (!initialized)
	{
		count = 1;
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
#if _DEBUG
	printf("Init %d objects\n", board::count);
#endif
}

#if _DEBUG
int allocCount = 0;
#endif

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
#if _DEBUG
		allocCount++;
#endif
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

inline bool isStuckPosition(board* &b, const int &x, const int &y)
{
	bool d1 = y > 0 && atPos(b, x, y - 1);
	bool d2 = x > 0 && atPos(b, x - 1, y);

	bool notStuck = d1 && d2;
	if (notStuck) return false;

	bool d3 = y < MAP_SIZE - 1 && atPos(b, x, y + 1);

	notStuck = (d1 || d2) && d3;
	if (notStuck) return false;

	bool d4 = x < MAP_SIZE - 1 && atPos(b, x + 1, y);
	
	notStuck = (d1 || d2 || d3) && d4;
	if (notStuck) return false;

	return true;
}

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

inline void moveMe(board* next, const int d, int &resultDir, Position &my,const Position &opp, int &v, int &alpha, int &beta, int depth, bool &prunned)
{
	set0AtPos(next, my.x, my.y);
	int t = abp(next, my, opp, depth - 1, alpha, beta, false, false);
	makeAvaiAtPos(next, my.x, my.y);
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
}

inline void moveOpp(board* next, const Position &my, Position &opp, int &v, int &alpha, int &beta, int depth, bool &prunned)
{
	set0AtPos(next, opp.x, opp.y);
	int t = abp(next, my, opp, depth - 1, alpha, beta, true, false);
	makeAvaiAtPos(next, opp.x, opp.y);
	v = MINXY(v, t);
	beta = MINXY(beta, v);
	if (beta <= alpha)
	{
		prunned = true;
	}
}

int deepMove(board* b, const Position &pos, int depth, const int &maxDepth, bool returnDirection)
{
	int x = pos.x, y = pos.y;
	int d1 = 0, d2 = 0, d3 = 0, d4 = 0;
	findAvailableDir(b, d1, d2, d3, d4, x, y);

	if ((d1 | d2 | d3 | d4) == 0 || depth >= maxDepth) // terminate condition meets
	{
		return depth;
	}

	int max = -1;
	int dir = 1;
	board* next = b;// allocBoards(b);

	if (d1)
	{
		set0AtPos(next, x, y - 1);
		max = deepMove(next, Position(x, y - 1), depth + 1, maxDepth, false);
		makeAvaiAtPos(next, x, y - 1);
	}

	if (d2)
	{
		set0AtPos(next, x - 1, y);
		int v = deepMove(next, Position(x - 1, y), depth + 1, maxDepth, false);
		makeAvaiAtPos(next, x - 1, y);
		if (v > max)
		{
			max = v;
			dir = 2;
		}
	}
	
	if (d3)
	{
		set0AtPos(next, x, y + 1);
		int v = deepMove(next, Position(x, y + 1), depth + 1, maxDepth, false);
		makeAvaiAtPos(next, x, y + 1);
		if (v > max)
		{
			max = v;
			dir = 3;
		}
	}

	if (d4)
	{
		set0AtPos(next, x + 1, y);
		int v = deepMove(next, Position(x + 1, y), depth + 1, maxDepth, false);
		makeAvaiAtPos(next, x + 1, y);
		if (v > max)
		{
			max = v;
			dir = 4;
		}
	}

	//reclaimBoards(next);

	return returnDirection ? dir : max;
}

int deepMoveDfsIterative(board* b, const Position &pos)
{
	int x = pos.x, y = pos.y;
	int dirsToVisit[256], backTrack[256];
	int topDir = 0, topBackTrack = 0;
	if (y > 0 && atPos(b, x, y - 1))
		dirsToVisit[topDir++] = (DIRECTION_LEFT);
	if (x > 0 && atPos(b, x - 1, y))
		dirsToVisit[topDir++] = (DIRECTION_UP);
	if (y < MAP_SIZE - 1 && atPos(b, x, y + 1))
		dirsToVisit[topDir++] = (DIRECTION_RIGHT);
	if (x < MAP_SIZE - 1 && atPos(b, x + 1, y))
		dirsToVisit[topDir++] = (DIRECTION_DOWN);

	int currentDepth = 0;
	int maxDepth = currentDepth;

	int dir;
	bool hasChild = false;

	while (topDir > 0)
	{
		dir = dirsToVisit[--topDir];
		if (dir == 0)
		{
			--currentDepth;
			makeAvaiAtPos(b, x, y);

			switch (backTrack[--topBackTrack])
			{
			case 1:
				++y;
				break;
			case 2:
				++x;
				break;
			case 3:
				--y;
				break;
			case 4:
				--x;
				break;
			default:
				break;
			}

			continue;
		}

		switch (dir)
		{
		case 1:
			--y;
			break;
		case 2:
			--x;
			break;
		case 3:
			++y;
			break;
		case 4:
			++x;
			break;
		default:
			break;
		}

		backTrack[topBackTrack++] = (dir);
		++currentDepth;

		set0AtPos(b, x, y);

		dirsToVisit[topDir++] = (0);

		if (y > 0 && atPos(b, x, y - 1))
		{
			dirsToVisit[topDir++] = (DIRECTION_LEFT);
		}
		if (x > 0 && atPos(b, x - 1, y))
		{
			dirsToVisit[topDir++] = (DIRECTION_UP);
		}
		if (y < MAP_SIZE - 1 && atPos(b, x, y + 1))
		{
			dirsToVisit[topDir++] = (DIRECTION_RIGHT);
		}
		if (x < MAP_SIZE - 1 && atPos(b, x + 1, y))
		{
			dirsToVisit[topDir++] = (DIRECTION_DOWN);
		}

		maxDepth = MAXXY(maxDepth, currentDepth);
	}

	return maxDepth;
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

	if ((d1 | d2 | d3 | d4) == 0) // terminate condition meets: lost
	{
		return -1000 - depth; // better lose fast
	}

	if (!returnDirection && (e1 | e2 | e3 | e4) == 0) // terminate condition meets: won
	{
		return 1000 + depth; // better win fast
	}

	if (!(x == ox && (y - oy == 1 || oy - y == 1)) && !(y == oy && (x - ox == 1 || ox - x == 1)))
	{
		copyToSearchBoard(b);

		searchBoard[x][y] = 0;
		searchBoard[ox][oy] = 2;

		int myMoveCount = countPosibleMoves(x, y, true);
#if _DEBUG
		++callCountPosMoves;
#endif
		if (myMoveCount > 0)
		{
			searchBoard[ox][oy] = 0;
			int oppMoveCount = countPosibleMoves(ox, oy, true);
#if _DEBUG
			++callCountPosMoves;
#endif
			// Optim 1
			int k = myMoveCount - oppMoveCount;
			if (k >= 5 || k <= -5)
			{
				int ret = myMoveCount >= oppMoveCount ? 900 + depth + myMoveCount : -900 - depth - oppMoveCount;
				return ret;
			}
			
			if (myMoveCount <= 25 && oppMoveCount <= 25)
			{
				board* sboard = allocBoards(b);
				myMoveCount = deepMove(sboard, myPos, 0, 41, false);
				oppMoveCount = deepMove(sboard, opPos, 0, 41, false);
				reclaimBoards(b);

				int ret = myMoveCount >= oppMoveCount ? 900 + depth + myMoveCount : -900 - depth - oppMoveCount;
				return ret;
			}


			// Optim 2
			//if (movableCount - 3 > oppMoveCount || movableCount < oppMoveCount)
			//{
			//	return movableCount > oppMoveCount ? 900 + depth + oppMoveCount : -900 - depth + movableCount;
			//}
		}
	}

	if (maximizePlayer)
	{
		bool prunned = false;
		int dir = 0;
		int v = -1000000;

		bool d1_prior = myPos.y > opPos.y;
		bool d2_prior = myPos.x > opPos.x;

		if (d1_prior && !prunned && d1 > 0)
		{
			moveMe(b, 1, dir, Position(x, y - 1), opPos, v, alpha, beta, depth, prunned);
		}
		if (d2_prior && !prunned && d2 > 0)
		{
			moveMe(b, 2, dir, Position(x - 1, y), opPos, v, alpha, beta, depth, prunned);
		}

		if (!d1_prior && !prunned && d3 > 0)
		{
			moveMe(b, 3, dir, Position(x, y + 1), opPos, v, alpha, beta, depth, prunned);
		}
		if (!d2_prior && !prunned && d4 > 0)
		{
			moveMe(b, 4, dir, Position(x + 1, y), opPos, v, alpha, beta, depth, prunned);
		}

		if (!d1_prior && !prunned && d1 > 0)
		{
			moveMe(b, 1, dir, Position(x, y - 1), opPos, v, alpha, beta, depth, prunned);
		}
		if (!d2_prior && !prunned && d2 > 0)
		{
			moveMe(b, 2, dir, Position(x - 1, y), opPos, v, alpha, beta, depth, prunned);
		}

		if (d1_prior && !prunned && d3 > 0)
		{
			moveMe(b, 3, dir, Position(x, y + 1), opPos, v, alpha, beta, depth, prunned);
		}
		if (d2_prior && !prunned && d4 > 0)
		{
			moveMe(b, 4, dir, Position(x + 1, y), opPos, v, alpha, beta, depth, prunned);
		}

		return returnDirection ? dir : v;
	}
	else
	{
		bool prunned = false;
		int v = 1000000;

		bool e1_prior = myPos.y < opPos.y;
		bool e2_prior = myPos.x < opPos.x;

		if (e1_prior && !prunned && e1 > 0)
		{
			moveOpp(b, myPos, Position(ox, oy - 1), v, alpha, beta, depth, prunned);
		}
		if (e2_prior && !prunned && e2 > 0)
		{
			moveOpp(b, myPos, Position(ox - 1, oy), v, alpha, beta, depth, prunned);
		}
		if (!e1_prior && !prunned && e3 > 0)
		{
			moveOpp(b, myPos, Position(ox, oy + 1), v, alpha, beta, depth, prunned);
		}
		if (!e2_prior && !prunned && e4 > 0)
		{
			moveOpp(b, myPos, Position(ox + 1, oy), v, alpha, beta, depth, prunned);
		}

		if (!e1_prior && !prunned && e1 > 0)
		{
			moveOpp(b, myPos, Position(ox, oy - 1), v, alpha, beta, depth, prunned);
		}
		if (!e2_prior && !prunned && e2 > 0)
		{
			moveOpp(b, myPos, Position(ox - 1, oy), v, alpha, beta, depth, prunned);
		}
		if (e1_prior && !prunned && e3 > 0)
		{
			moveOpp(b, myPos, Position(ox, oy + 1), v, alpha, beta, depth, prunned);
		}
		if (e2_prior && !prunned && e4 > 0)
		{
			moveOpp(b, myPos, Position(ox + 1, oy), v, alpha, beta, depth, prunned);
		}

		return v;
	}
}

void copyToSearchBoard(board* b)
{
	originSearchBoard = b;
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
int countPosibleMoves(int x, int y, const bool &nonRecursiveCall)
{
	if (nonRecursiveCall)
	{
		stuckPosCount = 0;
	}
	else if (isStuckPosition(originSearchBoard, x, y))
	{
		++stuckPosCount;
	}

	int count = 0;

	// Found opponent
	if (searchBoard[x][y] == 2)
	{
		count = POSITIVE;
	}


	searchBoard[x][y] = 0;
	if (x > 0 && searchBoard[x - 1][y])
	{
		++count;
		count += countPosibleMoves(x - 1, y, false);
		if (nonRecursiveCall && isStuckPosition(originSearchBoard, x - 1, y))
		{
			--stuckPosCount;
		}
	}

	if (y > 0 && searchBoard[x][y - 1])
	{
		++count;
		count += countPosibleMoves(x, y - 1, false);
		if (nonRecursiveCall && isStuckPosition(originSearchBoard, x, y - 1))
		{
			--stuckPosCount;
		}
	}
	if (y < MAP_SIZE - 1 && searchBoard[x][y + 1])
	{
		++count;
		count += countPosibleMoves(x, y + 1, false);
		if (nonRecursiveCall && isStuckPosition(originSearchBoard, x, y + 1))
		{
			--stuckPosCount;
		}
	}

	if (x < MAP_SIZE - 1 && searchBoard[x + 1][y])
	{
		++count;
		count += countPosibleMoves(x + 1, y, false);
		if (nonRecursiveCall && isStuckPosition(originSearchBoard, x + 1, y))
		{
			--stuckPosCount;
		}
	}

	if (nonRecursiveCall)
	{
		return stuckPosCount > 0 ? count - (stuckPosCount - 1) : count;
	}

	return count;
}

int evaluateBoard(board* b, const Position& myPos, const Position& opPos, const bool & maximizePlayer)
{
	copyToSearchBoard(b);
	searchBoard[myPos.x][myPos.y] = 0;
	searchBoard[opPos.x][opPos.y] = 2;

	int myMovableCount = countPosibleMoves(myPos.x, myPos.y, true);

#if _DEBUG
	++callCountPosMoves;
#endif
	// Reset search board if my position and opponent position are in same plane (they can be connected by some moves)
	// because searching for my moves sets all position available to 0.
	// This is applicable only as my position split all my available moves to 2 parts.
	//----

	// Me and enemy can be connected
	if (myMovableCount < 0)
	{
		//copyToSearchBoard(b);

		// Reconstruct my moves
		myMovableCount = myMovableCount - POSITIVE - 1; // minus 1: position of opponent is set to 2 before
		return myMovableCount;
	}

	searchBoard[opPos.x][opPos.y] = 0;
	int opponentMovableCount = countPosibleMoves(opPos.x, opPos.y, true);

#if _DEBUG
	++callCountPosMoves;
#endif

	// Unreachable now
	//if (myMovableCount < 0)
	//{
	//	// Reconstruct my moves
	//	myMovableCount = myMovableCount - POSITIVE - 1; // minus 1: position of opponent is set to 2 before
	//	if (myMovableCount > opponentMovableCount * 2) // I can absolutely win
	//	{
	//		return 1000;
	//	}
	//	return 0;
	//}

	return myMovableCount == opponentMovableCount ?
		0 : (myMovableCount > opponentMovableCount ? 800 + myMovableCount: -800 - opponentMovableCount);
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

	return moves;
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
	followEnemy = (followEnemy && (allMovesCount & 1)) && false; // follow if enemy move first -> allMovesCount is odd number
	
	if (followEnemy && allMovesCount == 3) // init to follow enemy
	{
		enemyAtUpper = opPos.x < 5;
		if (enemyAtUpper)
		{
			enemyPrevPos = Position(0, 0);
		}
		else
		{
			enemyPrevPos = Position(MAP_SIZE - 1, MAP_SIZE - 1);
		}
	}

	if (allMovesCount > 22)
	{
		// reset
		followEnemy = true;
		enemyPrevPos.x = enemyPrevPos.y = -1;
	}

	bool iAmPlayer1 = (allMovesCount & 1) == 0; // allMovesCount is even

	allMovesCount /= 2;
	bool useLogic = allMovesCount <= 3;
	int depth = 0;

	if (useLogic && allMovesCount >= 9)
	{
		board *b = copyFrom(origBoard);
		copyToSearchBoard(b);
		
		if (origBoard[CONVERT_COORD(4, 5)] == BLOCK_OBSTACLE || origBoard[CONVERT_COORD(5, 4)] == BLOCK_OBSTACLE)
		{
			useLogic = false;
			depth = 45;
		}
		else if (countPosibleMoves(MAP_SIZE - 2, 1, true) < 60) // play board is splitted 2 regions
		{
			useLogic = false;
			depth = 45;
		}
		else
		{
			bool upper = origBoard[0] == origBoard[CONVERT_COORD(myPos.x, myPos.y)]
				|| origBoard[0] == origBoard[CONVERT_COORD(myPos.x, myPos.y)] + 1;
			if (allMovesCount == 10 && ((upper && opPos.x == 6 && opPos.y == 6) || (!upper && opPos.x == 4 && opPos.y == 4)))
			{
				printf("checking ola, upper = %d, mypos = %d;%d, opPos = %d;%d ", upper, myPos.x, myPos.y, opPos.x, opPos.y);
				useLogic = false;
				depth = 35;
			}
		}
	}

	if (useLogic)
	{
		bool upper = origBoard[0] == origBoard[CONVERT_COORD(myPos.x, myPos.y)]
			|| origBoard[0] == origBoard[CONVERT_COORD(myPos.x, myPos.y)] + 1;
		
		bool criticalUpper = (myPos.x == 4 && myPos.y == 4 && origBoard[CONVERT_COORD(4, 5)] == BLOCK_EMPTY && origBoard[CONVERT_COORD(5, 4)] == BLOCK_EMPTY)
			|| (myPos.x == 3 && myPos.y == 3 && origBoard[CONVERT_COORD(4, 4)] != BLOCK_EMPTY && origBoard[CONVERT_COORD(3, 4)] == BLOCK_EMPTY && origBoard[CONVERT_COORD(4, 3)] == BLOCK_EMPTY);
		bool criticalLower = (myPos.x == 6 && myPos.y == 6 && origBoard[CONVERT_COORD(6, 5)] == BLOCK_EMPTY && origBoard[CONVERT_COORD(5, 6)] == BLOCK_EMPTY)
			|| (myPos.x == 7 && myPos.y == 7 && origBoard[CONVERT_COORD(6, 6)] != BLOCK_EMPTY && origBoard[CONVERT_COORD(6, 7)] == BLOCK_EMPTY && origBoard[CONVERT_COORD(7, 6)] == BLOCK_EMPTY);

		int b_cpy[MAP_SIZE*MAP_SIZE];
		memcpy(b_cpy, origBoard, sizeof(b_cpy));

		if (criticalLower || criticalUpper)
		{
			int countEmptyUpper = 0, countEmptyLower = 0;
			// draw a line to separate the playing board
			DDALine(myPos.x, myPos.y, opPos.x, opPos.y, b_cpy);
			board* board_cpy = copyFrom(b_cpy);
			copyToSearchBoard(board_cpy);
			
			countEmptyUpper = countPosibleMoves(1, MAP_SIZE - 2, true); // use algorithm coordinate
			countEmptyLower = countPosibleMoves(MAP_SIZE - 2, 1, true);

			printf("critical up = %d, low = %d\n", countEmptyUpper, countEmptyLower);
			for (int y = 0; y < MAP_SIZE; ++y)
			{
				for (int x = 0; x < MAP_SIZE; ++x)
				{
					printf("%d  ", b_cpy[CONVERT_COORD(x, y)]);
				}
				printf("\n\n");
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
		for (int k = 0; k < 3; ++k)
		{

		if (upper)
		{
			for (int x = MAP_SIZE - 1; x >= 1; --x)
			{
				for (int y = MAP_SIZE - 2; y >= 0; --y)
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
			for (int x = 0; x < MAP_SIZE - 2; ++x)
			{
				for (int y = 1; y < MAP_SIZE - 1; ++y)
				{
					if (b_cpy[CONVERT_COORD(x, y)] && b_cpy[CONVERT_COORD(x + 1, y - 1)])
					{
						b_cpy[CONVERT_COORD(x + 1, y)] = 1;
					}
				}
			}
		}
		}

		if (followEnemy)
		{
			// calculate if need to follow enemy
			if (enemyAtUpper)
			{
				// enemy move to the right && we can follow to the left
				if (enemyPrevPos.x < opPos.x && opPos.x < 5 && b_cpy[CONVERT_COORD(myPos.x - 1, myPos.y)] == BLOCK_EMPTY) 
				{
					enemyPrevPos.x = opPos.x;
					return DIRECTION_LEFT;
				}
				// enemy move down && we can follow up
				else if (enemyPrevPos.y < opPos.y && opPos.y < 5 && b_cpy[CONVERT_COORD(myPos.x, myPos.y - 1)] == BLOCK_EMPTY)
				{
					enemyPrevPos.y = opPos.y;
					return DIRECTION_UP;
				}

				// no need to follow any more if all conditions above not meet
				followEnemy = false;
			}
			else // enemy at lower
			{
				// enemy move to the left && we can follow to the right
				if (opPos.x < enemyPrevPos.x && opPos.x > 5 && b_cpy[CONVERT_COORD(myPos.x + 1, myPos.y)] == BLOCK_EMPTY)
				{
					enemyPrevPos.x = opPos.x;
					return DIRECTION_RIGHT;
				}
				// enemy move up && we can follow down
				else if (opPos.y < enemyPrevPos.y && opPos.y > 5 && b_cpy[CONVERT_COORD(myPos.x, myPos.y + 1)] == BLOCK_EMPTY)
				{
					enemyPrevPos.y = opPos.y;
					return DIRECTION_DOWN;
				}

				// no need to follow any more if all conditions above not meet
				followEnemy = false;
			}
		}

		if (myPos.x == myPos.y)
		{
			if (upper)
			{
				if (b_cpy[CONVERT_COORD(myPos.x, myPos.y + 1)] == BLOCK_EMPTY)
				{
					return DIRECTION_DOWN;
				}
			}
			else if (b_cpy[CONVERT_COORD(myPos.x, myPos.y - 1)] == BLOCK_EMPTY)
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
		int direction;
#if _DEBUG
		allocCount = 0;
		callCountPosMoves = 0;
#endif

		if (depth == 0)
		{
			copyToSearchBoard(b);
			searchBoard[myPos.y][myPos.x] = 0;
			searchBoard[opPos.y][opPos.x] = 2;
			int posibleMoves = countPosibleMoves(myPos.y, myPos.x, true);
			if (posibleMoves < 0)
			{
				//// Reconstruct my moves
				//posibleMoves = posibleMoves - POSITIVE - 1; // minus 1: position of opponent is set to 2 before

				//int distance = DISTANCE_SQR(myPos.x, myPos.y, opPos.x, opPos.y);
				//if (distance <= 2)
				//{
				//	depth = posibleMoves;
				//}

				//if (posibleMoves < 55)
				//{
				//	depth = posibleMoves;
				//}
				//else if (posibleMoves < 70)
				//{
				//	depth = 50;
				//}
				//else
				//{
				//	depth = 25;// 37;
				//}

				depth = 22;
			}
			else
			{
				int maxDepth = posibleMoves < 48 ? posibleMoves : (MINXY(80 - posibleMoves, 26));
				printf("-> self move, maxDepth = %d, posibleMoves = %d ", maxDepth, posibleMoves);
				direction = deepMove(b, Position(myPos.y, myPos.x), 0, maxDepth, true);
				return direction;
			}
		}
		printf("-> depth = %d, mypos = %d;%d, oppos = %d;%d ", depth, myPos.x, myPos.y, opPos.x, opPos.y);
		direction = abp(b, Position(myPos.y, myPos.x), Position(opPos.y, opPos.x), depth, MIN_INT, MAX_INT, true, true);
#if _DEBUG
		printf("Alloc Count = %d ", allocCount);
		printf("Calls pos moves = %d\n", callCountPosMoves);
#endif
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
