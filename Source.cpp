#include <vector>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <windows.h>
#include <winuser.h>
#include <thread>         
#include <chrono>      
#include <memory>
#include <optional>
#include <unordered_map>

constexpr int boardDimentions = 3;

enum class Symbol
{
	Cross,
	Circle,
	Empty
};

using Cells = std::vector<Symbol>;

auto HorizontalChecker = [](const Cells& cells, int row) -> Symbol
{
	Symbol winner = cells[row * boardDimentions];
	if (winner != Symbol::Empty)
	{
		auto beginIter = cells.begin() + row * boardDimentions;
		bool won = std::all_of(beginIter, beginIter + boardDimentions, [winner](auto val) {return val == winner; });
		if (won)
			return *beginIter;				//because it contains sign of winner
	}
	return Symbol::Empty;
};

auto VerticalChecker = [](const Cells& cells, int column) -> Symbol
{
	Symbol winner = cells[column];
	for (int i = 0; i < boardDimentions; i++)
	{
		if (cells[i * boardDimentions + column] != winner)
		{
			winner = Symbol::Empty;
			break;
		}
	}
	return winner;
};

class Board
{
public:	
	bool ProcessWinning()		//returns 1 if the game has ended
	{
		auto winner = CheckWinner();
		bool draw = CheckDraw();
		if (winner != Symbol::Empty || draw)
		{
			OnWin(winner);		//winner==empty means draw 
			return 1;
		}
		
		return 0;
	}

	void PrintCells()
	{
		for (int i = 0; i < boardDimentions; i++)
		{
			for (int j = 0; j < boardDimentions; j++)
			{
				auto curSymbol = cells[GetInvertedCellIndex(j ,i)];
				switch (curSymbol)
				{
				case Symbol::Cross:
					std::cout << "X";
					break;
				case Symbol::Circle:
					std::cout << "O";
					break;
				case Symbol::Empty:
					std::cout << " ";
					break;
				default:
					assert(false && "Unsupported symbol at Board::PrintCells()");
					break;
				}				
			}
			std::cout << '\n';
		}
		std::cout << "___________________________________________\n";
	}

	//allows us to traverse cells array in the order of numbers on num pad keys
	int GetInvertedCellIndex(int x, int y)
	{
		return x + boardDimentions * (boardDimentions - y - 1);
	}

	Cells cells{ boardDimentions * boardDimentions, Symbol::Empty };

private:
	bool CheckDraw()
	{
		return std::all_of(cells.begin(), cells.end(), [](auto symbol) {return symbol != Symbol::Empty; });
	}

	void OnWin(Symbol winner)
	{
		PrintWinningMessage(winner);
		std::cin.ignore(256, '\n');			//wait for enter press
	}
	
	Symbol CheckWinner()
	{
		for (int i = 0; i < boardDimentions; i++)
		{
			Symbol horizontalWinner = HorizontalChecker(cells, i);
			Symbol verticalWinner = VerticalChecker(cells, i);
			if (horizontalWinner != Symbol::Empty)
				return horizontalWinner;
			if (verticalWinner != Symbol::Empty)
				return verticalWinner;
		}
		return Symbol::Empty;
	}
	
	void PrintWinningMessage(Symbol winner)
	{
		switch (winner)
		{
		case Symbol::Cross:
			std::cout << "The winner is X !!!\n";
			break;
		case Symbol::Circle:
			std::cout << "The winner is O !!!\n";
			break;
		case Symbol::Empty:
			std::cout << "It's a draw!\n";
			break;
		default:
			assert(false && "unsupported Symbol at Board::PrintWinningMessage");
			break;
		}
		std::cout << "Press Enter key to play again:\n";
	}
};

class Player
{
public:
	Player(Symbol symbol) : mySymbol(symbol) {};
	virtual ~Player() = default;
	virtual void MakeTurn(Cells& cells) = 0;
protected :
	Symbol mySymbol;
};

class AI : public Player
{
public:
	AI(Symbol symbol) :Player(symbol) {};
	void MakeTurn(Cells& cells) override
	{
		//if there is a guaranteed win - take it
		auto winningTurn = CheckWinningTurns(cells);
		if (winningTurn)
		{
			cells[winningTurn.value()] = mySymbol;
			return;
		}
		//if middle is empty - place there
		int middle = boardDimentions * boardDimentions / 2;
		if (cells[middle] == Symbol::Empty)
		{
			cells[middle] = mySymbol;
			return;
		}
		//if enemy is about to win - block them
		auto blockingTurn = CheckBlockingTurns(cells);
		if (blockingTurn)
		{
			cells[blockingTurn.value()] = mySymbol;
			return;
		}
		//place in a corner
		auto corner = FindEmptyCorner(cells);
		if (corner)
		{
			cells[corner.value()] = mySymbol;
			return;
		}
		//place anywhere
		for (auto& cell : cells)
		{
			if (cell == Symbol::Empty) cell = mySymbol;
			break;
		}
	}
	std::optional<int> CheckWinningTurns(Cells& cells)
	{
		return CheckTurns(cells, [this](std::unordered_map<Symbol, int>& lineData) {return lineData[this->mySymbol] == boardDimentions - 1 && lineData[Symbol::Empty] == 1; });
	}
	std::optional<int> CheckBlockingTurns(Cells& cells)
	{
		return CheckTurns(cells, [this](std::unordered_map<Symbol, int>& lineData) {return lineData[this->mySymbol] == 0 && lineData[Symbol::Empty] == 1; });
	}
	std::optional<int>CheckTurns(Cells& cells, auto DecidingPredicate)			//DecidingPredicate checks if there is a good placing spot in this row/column, or not
	{
		for (int i = 0; i < boardDimentions; i++)
		{
			std::unordered_map<Symbol, int> horizontalSymbolCounts;
			std::unordered_map<Symbol, int> verticalSymbolCounts;
			for (int j = 0; j < boardDimentions; j++)
			{
				horizontalSymbolCounts[cells[j + i * boardDimentions]]++;
				verticalSymbolCounts[cells[i + j * boardDimentions]]++;
			}
			if (DecidingPredicate(horizontalSymbolCounts))			//placing here will result in a win
			{
				for (int j = 0; j < boardDimentions; j++)
				{
					if (cells[j + i * boardDimentions] == Symbol::Empty)return { j + i * boardDimentions };
				}
			}
			if (DecidingPredicate(verticalSymbolCounts))			//placing here will result in a win
			{
				for (int j = 0; j < boardDimentions; j++)
				{
					if (cells[i + j * boardDimentions] == Symbol::Empty)return { i + j * boardDimentions };
				}
			}
		}
		return{};
	}
	std::optional<int>FindEmptyCorner(Cells& cells)
	{
		if (cells[0] == Symbol::Empty)										return{ 0 };
		if (cells[boardDimentions - 1] == Symbol::Empty)					return { boardDimentions - 1 };
		if(cells[(boardDimentions - 1) * boardDimentions] == Symbol::Empty) return { (boardDimentions - 1) * boardDimentions };
		if (cells[boardDimentions * boardDimentions - 1] == Symbol::Empty)  return { boardDimentions * boardDimentions - 1 };
																			return{};
	}
};

class Human : public Player
{
public:
	Human(Symbol symbol) :Player(symbol) {};
	void MakeTurn(Cells& cells) override
	{
		for (;;)
		{
			int keyPressed = 0;
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			for (int i = VK_NUMPAD1; i <= VK_NUMPAD9; i++)
			{
				if (GetKeyState(i) < -1)
					keyPressed = i - VK_NUMPAD1 + 1;
			}
			if (keyPressed != 0)
			{
				if (cells[keyPressed - 1] == Symbol::Empty)
				{
					cells[keyPressed - 1] = mySymbol;
					break;
				}
			}
		}
	}
};

int main()
{
	std::cout << "Use num pad keys to place crosses\n";
	std::cout << "Good luck!\n";
	std::vector<std::unique_ptr<Player>> players;
	players.push_back(std::make_unique<Human>(Symbol::Cross));
	players.push_back(std::make_unique<AI>(Symbol::Circle));
	
	for (;;)		//outer loop allows to play infinitely
	{
		Board board;
		for (;;)		//this loop restarts the next loop untill the game is over
		{
			board.PrintCells();
			for (auto& player : players)
			{
				player->MakeTurn(board.cells);
				board.PrintCells();
				if (board.ProcessWinning())
					goto GameEnd;
			}
		}
	GameEnd:;
	}
	return 0;
}













