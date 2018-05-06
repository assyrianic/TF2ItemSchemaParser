
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

//#define PROFILE 1

#ifdef PROFILE
	#include <time.h>
#endif

int main()
{
	struct KeyVal *items_game = KeyVal_New(NULL);
	
#ifdef PROFILE
	clock_t start = clock();
#endif
	
	KeyVal_ReadItemSchema(items_game);
	//KeyVal_Print(items_game);
	
	struct KeyVal *items = KeyVal_FindByKeyName(items_game, "items");
	for( struct KeyVal *kvitem = KeyVal_GetSubKey(items) ; kvitem ; kvitem=KeyVal_GetNextKey(kvitem) ) {
		//printf("subkey name of item: '%s' | %u\n", KeyVal_GetKeyName(kvitem));
		//if( KeyVal_HasKey(kvitem, "equip_region") )
		if( KeyVal_HasKey(kvitem, "name") ) {
			struct KeyVal *i = KeyVal_FindByKeyName(kvitem, "name");
			if( !i )
				continue;
			printf("item name: '%s'\n", KeyVal_Get(i).Val.Str->CStr);
		}
	}
	
#ifdef PROFILE
	printf("Keyvalue ReadItemSchema Profile Time: %Lf\n", (clock()-start)/(long double)CLOCKS_PER_SEC);
#endif
	
	KeyVal_Free(&items_game);
	printf("items_game is NULL? '%s'\n", items_game ? "no" : "yes");
}
