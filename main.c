
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

//#define PROFILE 1

#ifdef PROFILE
	#include <time.h>
#endif

int main()
{
	// create our include file.
	FILE *IncludeFile = fopen("items_equip_regions.inc", "w+");
	if( !IncludeFile ) {
		puts("TF2 Item Schema Parser Error: **** could not create \"items_equip_regions.inc\". ****");
		return -1;
	}
	
	// create our include header.
	fprintf(IncludeFile, "// Include File Generated by the TF2 Item Schema Parser\n"
							"// Written by Nergal (The Ashurian) aka Assyrian.\n"
							"// For Source Code, visit the parser's github @ https://github.com/assyrianic/TF2ItemSchemaParser\n"
							"#if defined _tf2equip_regions_included\n\t"
							"#endinput\n"
							"#endif\n"
							"#define _tf2equip_regions_included\n\n");
	
	// create our keyvalues and parse the item schema.
	struct KeyVal *items_game = KeyVal_New(NULL);
	KeyVal_ReadItemSchema(items_game);
	
#ifdef PROFILE
	const clock_t start = clock();
#endif
	
	// create our enum for the equip regions since certain items can have multiple equip regions.
	KeyVal_GenerateEnum(items_game, IncludeFile);
	// create our function.
	KeyVal_GenerateStockFunc(items_game, IncludeFile);
	
#ifdef PROFILE
	printf("Keyvalue ReadItemSchema Profile Time: %Lf\n", (clock()-start)/(long double)CLOCKS_PER_SEC);
#endif
	
	KeyVal_Free(&items_game);
	fclose(IncludeFile); IncludeFile = NULL;
#ifdef PROFILE
	printf("items_game is NULL? '%s'\n", items_game ? "no" : "yes");
#endif
}
