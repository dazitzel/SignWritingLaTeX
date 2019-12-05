all: fswtotex extractgloss sortenu

fswtotex: fswtotex.cpp
	g++ -Wall fswtotex.cpp -o fswtotex

extractgloss: extractgloss.cpp
	g++ -Wall extractgloss.cpp -o extractgloss

sortenu: sortenu.cpp
	g++ -Wall sortenu.cpp -o sortenu

