//  http://codereview.stackexchange.com/questions/47167/conways-game-of-life-in-c

#include "game_of_life3.h"

#include "quark.h"
#include <iostream>
#include <cstdlib>
#include <vector>


namespace game_of_life3 {

	std::string to_string(const grid_t& grid){
		std::string result;
		for(int a = 0; a < gridsize; a++){
			for(int b = 0; b < gridsize; b++){
				if(grid._entries[a][b] == true){
					result = result + "*";
				}
				else{
					result = result + " ";
				}
			}
			result = result + "\n";
		}
		return result;
	}

	grid_t liveOrDie(const grid_t& grid0){
		grid_t grid1 = grid0;

		for(int a = 0; a < gridsize; a++){
			for(int b = 0; b < gridsize; b++){
				int life = 0;
				for(int c = -1; c < 2; c++){
					for(int d = -1; d < 2; d++){
						int x2 = a + c;
						int y2 = b + d;
						if(x2 == a && y2 == b){
						}
						else if(x2 < 0 || x2 >= gridsize || y2 < 0 || y2 >= gridsize){
						}
						else{
							if(grid0._entries[x2][y2]){
								++life;
							}
						}
					}
				}
				if(life < 2) {
					grid1._entries[a][b] = false;
				}
				else if(life == 3){
					grid1._entries[a][b] = true;
				}
				else if(life > 3){
					grid1._entries[a][b] = false;
				}
			}
		}
		return grid1;
	}

	grid_t make_init(){
		grid_t result;
		result._entries[gridsize/2][gridsize/2] = true;
		result._entries[gridsize/2-1][gridsize/2] = true;
		result._entries[gridsize/2][gridsize/2+1] = true;
		result._entries[gridsize/2][gridsize/2-1] = true;
		result._entries[gridsize/2+1][gridsize/2+1] = true;
		return result;
	}

	void game_of_life(int generations){
		grid_t grid = make_init();
		for(int i = 0 ; i < generations ; i++){
		std::cout << to_string(grid);
			grid = liveOrDie(grid);
		}
	}

}

QUARK_UNIT_TESTQ("game_of_life3", "10 generations"){
	using namespace game_of_life3;

	grid_t grid = make_init();
	for(int i = 0 ; i < 10 ; i++){
		std::cout << to_string(grid);
		grid = liveOrDie(grid);
	}
}
