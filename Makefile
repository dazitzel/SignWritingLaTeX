all: fswtotex extractgloss

fswtotex: fswtotex.cpp
	g++ -Wall fswtotex.cpp -o fswtotex

extractgloss: extractgloss.cpp
	g++ -Wall extractgloss.cpp -o extractgloss

