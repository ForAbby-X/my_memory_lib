
#include <stdio.h>
#include "memory_arena.h"

typedef struct
{
	MemoryArena name_arena;
	
}
GameData;

inline
void
game_init(GameData* game)
{
	game->a = 0;
}

inline
void
game_destroy(GameData* game)
{
	game->a = 0;
}

int main()
{
	GameData game;
	game_init(&game);

	printf("Hello Gamer ?\n");

	game_destroy(&game);
	return 0;
}
