
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
#if _DEBUG
			int v = b[CONVERT_COORD(j, i)];
			printf("%d ", v);
#endif
			if (b[CONVERT_COORD(j, i)] != BLOCK_EMPTY)
			{
				set0AtPos(pRet, i, j);
			}
		}
#if _DEBUG
		printf("\n");
#endif
	}
	return pRet;
}

// AI implementation //
/*Heuristic function:*/
int countPosibleMoves(board* b, int x, int y);

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

inline void moveMe(board* b, const int d, int &resultDir, Position &my,const Position &opp, int &alpha, int &beta, int depth, bool &prunned)
{
	board* next = allocBoards(b);
	set0AtPos(next, my.x, my.y);
	int t = abp(next, my, opp, depth - 1, alpha, beta, false, false);
	if (t > alpha)
	{
		alpha = t;
		resultDir = d;
	}
	if (beta <= alpha)
	{
		prunned = true;
	}
	reclaimBoards(next);
}

inline void moveOpp(board* b, const Position &my, Position &opp, int &alpha, int &beta, int depth, bool &prunned)
{
	board* next = allocBoards(b);
	set0AtPos(next, opp.x, opp.y);
	int t = abp(next, my, opp, depth - 1, alpha, beta, true, false);
	beta = MINXY(beta, t);
	if (beta <= alpha)
	{
		prunned = true;
	}
	reclaimBoards(next);
}

int abp(board* b, const Position &myPos, const Position &opPos, int depth, int alpha, int beta, bool maximizePlayer, bool returnDirection)
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
		return -1000 - depth;
	}

	if ((e1 | e2 | e3 | e4) == 0) // terminate condition meets
	{
		return 1000 + depth;
	}

	if (maximizePlayer)
	{
		bool prunned = false;
		int dir = 0;
		if (d1 > 0)
		{
			moveMe(b, 1, dir, Position(x, y - 1), opPos, alpha, beta, depth, prunned);
		}
		if (!prunned && d2 > 0)
		{
			moveMe(b, 2, dir, Position(x - 1, y), opPos, alpha, beta, depth, prunned);
		}
		if (!prunned && d3 > 0)
		{
			moveMe(b, 3, dir, Position(x, y + 1), opPos, alpha, beta, depth, prunned);
		}
		if (!prunned && d4 > 0)
		{
			moveMe(b, 4, dir, Position(x + 1, y), opPos, alpha, beta, depth, prunned);
		}

		return returnDirection ? dir : alpha;
	}
	else
	{
		bool prunned = false;
		if (e1 > 0)
		{
			moveOpp(b, myPos, Position(ox, oy - 1), alpha, beta, depth, prunned);
		}
		if (!prunned && e2 > 0)
		{
			moveOpp(b, myPos, Position(ox - 1, oy), alpha, beta, depth, prunned);
		}
		if (!prunned && e3 > 0)
		{
			moveOpp(b, myPos, Position(ox, oy + 1), alpha, beta, depth, prunned);
		}
		if (!prunned && e4 > 0)
		{
			moveOpp(b, myPos, Position(ox + 1, oy), alpha, beta, depth, prunned);
		}

		return beta;
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

int evaluateBoard(board* b, const Position& myPos, const Position& opPos)
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
		copyToSearchBoard(b);
	}
	int opponentMovableCount = countPosibleMoves(opPos.x, opPos.y);

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
	b[CONVERT_COORD(x, y)] = 0;
}

int searchDirFromUpperLeftCorner(int* b_cpy, const Position &myPos, int moves)
{
	int x1 = myPos.x, y1 = myPos.y;
	int x2 = x1, y2 = y1;
	++y1; // down
	++x2; // right
	if (b_cpy[CONVERT_COORD(x1, y1)] == 0 && b_cpy[CONVERT_COORD(x2, y2)] == 0)
	{
		int d01 = x1 * x1 + y1 * y1;
		int d13 = DISTANCE_SQR(x1, y1, 5, 5);
		int d02 = x2 * x2 + y2 * y2;
		int d23 = DISTANCE_SQR(x2, y2, 5, 5);
		int s = d01 + d13 - (d02 + d23);
		if (s < 0)
		{
			return 4; // decide to go down to (x1, y1)
		}
		else if (s > 0)
		{
			return 3; // decide to go right to (x2, y2)
		}
		else
		{
			// random direction (other idea?)
			return rand() % 1000 < 500 ? 3 : 4;
		}
	}

	return b_cpy[CONVERT_COORD(x1, y1)] == 0 ? 4 : 3;
}

int searchDirFromBottomRightCorner(int* b_cpy, const Position &myPos, int moves)
{
	int x1 = myPos.x, y1 = myPos.y;
	int x2 = x1, y2 = y1;
	--y1; // p1 -> up
	--x2; // p2 -> left
	if (b_cpy[CONVERT_COORD(x1, y1)] == 0 && b_cpy[CONVERT_COORD(x2, y2)] == 0)
	{
		int d01 = DISTANCE_SQR(x1, y1, MAP_SIZE - 1, MAP_SIZE - 1);
		int d13 = DISTANCE_SQR(x1, y1, 5, 5);
		int d02 = DISTANCE_SQR(x2, y2, MAP_SIZE - 1, MAP_SIZE - 1);
		int d23 = DISTANCE_SQR(x2, y2, 5, 5);
		int s = d01 + d13 - (d02 + d23);
		if (s < 0)
		{
			return 2; // decide to go up to (x1, y1)
		}
		else if (s > 0)
		{
			return 1; // decide to go left to (x2, y2)
		}
		else
		{
			// random direction (other idea?)
			return rand() % 1000 < 500 ? 3 : 4;
		}
	}

	return b_cpy[CONVERT_COORD(x1, y1)] == 0 ? 2 : 1;
}

int searchBestDirFromUpperLeft(int* board, const Position &myPos)
{
	int x = myPos.x, y = myPos.y;
	if (x == y && board[CONVERT_COORD(x, y + 1)] == 0 && board[CONVERT_COORD(x + 1, y)] == 0)
	{
		int length;
		int moves[5];
		int count = 0;
		// test go down
		moves[count++] = 4;
		simulateMove(board, 4, x, y);
		length = DISTANCE_SQR(0, 0, x, y + 1) + DISTANCE_SQR(x, y + 1, 5, 5);

		moves[count++] = searchDirFromUpperLeftCorner(board, Position(x, y));
		simulateMove(board, moves[count - 1], x, y);
		length += DISTANCE_SQR(0, 0, x, y + 1) + DISTANCE_SQR(x, y + 1, 5, 5);

		moves[count++] = searchDirFromUpperLeftCorner(board, Position(x, y));
		simulateMove(board, moves[count - 1], x, y);
		length += DISTANCE_SQR(0, 0, x, y + 1) + DISTANCE_SQR(x, y + 1, 5, 5);

		moves[count++] = searchDirFromUpperLeftCorner(board, Position(x, y));
		simulateMove(board, moves[count - 1], x, y);
		length += DISTANCE_SQR(0, 0, x, y + 1) + DISTANCE_SQR(x, y + 1, 5, 5);
		// test go right
	}

	return searchDirFromUpperLeftCorner(board, myPos);
}

int searchBestDirFromUpperLeft(int* board, const Position &myPos)
{

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

	if (allMovesCount <= 10)
	{
		int b_cpy[MAP_SIZE*MAP_SIZE];
		memcpy(b_cpy, origBoard, sizeof(b_cpy));
		bool upper = origBoard[0] == origBoard[CONVERT_COORD(myPos.x, myPos.y)]
			|| origBoard[0] == origBoard[CONVERT_COORD(myPos.x, myPos.y)] + 1;
		int dx = upper ? 1 : -1;
		int dy = dx;

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

		int x1 = myPos.x, y1 = myPos.y;
		int x2 = x1, y2 = y1;
		if (upper) // go down or right only
		{
			++y1; // down
			++x2; // right
			if (b_cpy[CONVERT_COORD(x1, y1)] == 0 && b_cpy[CONVERT_COORD(x2, y2)] == 0)
			{
				int d01 = x1 * x1 + y1 * y1;
				int d13 = DISTANCE_SQR(x1, y1, 5, 5);
				int d02 = x2 * x2 + y2 * y2;
				int d23 = DISTANCE_SQR(x2, y2, 5, 5);
				int s = d01 + d13 - (d02 + d23);
				if (s < 0)
				{
					return 4; // decide to go down to (x1, y1)
				}
				else if (s > 0)
				{
					return 3; // decide to go right to (x2, y2)
				}
				else
				{
					// random direction (other idea?)
					return rand() % 1000 < 500 ? 3 : 4;
				}
			}
			
			return b_cpy[CONVERT_COORD(x1, y1)] == 0 ? 4 : 3;
		}
		else // go up or left only
		{
			--y1; // p1 -> up
			--x2; // p2 -> left
			if (b_cpy[CONVERT_COORD(x1, y1)] == 0 && b_cpy[CONVERT_COORD(x2, y2)] == 0)
			{
				int d01 = DISTANCE_SQR(x1, y1, MAP_SIZE - 1, MAP_SIZE - 1);
				int d13 = DISTANCE_SQR(x1, y1, 5, 5);
				int d02 = DISTANCE_SQR(x2, y2, MAP_SIZE - 1, MAP_SIZE - 1);
				int d23 = DISTANCE_SQR(x2, y2, 5, 5);
				int s = d01 + d13 - (d02 + d23);
				if (s < 0)
				{
					return 2; // decide to go up to (x1, y1)
				}
				else if (s > 0)
				{
					return 1; // decide to go left to (x2, y2)
				}
				else
				{
					// random direction (other idea?)
					return rand() % 1000 < 500 ? 3 : 4;
				}
			}

			return b_cpy[CONVERT_COORD(x1, y1)] == 0 ? 2 : 1;
		}
	}
	else // Oke then use smart algorithm to solve enemy
	{
		board *b = copyFrom(origBoard);

		int direction = abp(b, Position(myPos.y, myPos.x), Position(opPos.y, opPos.x), 50, MIN_INT, MAX_INT, true, true);
		return direction;
	}
}

