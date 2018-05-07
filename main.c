
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
	FILE *EnumIncludeFile = fopen("items_equip_regions.inc", "w+");
	if( !EnumIncludeFile ) {
		puts("TF2 Item Schema Parser Error: **** could not create \"items_equip_regions.inc\". ****");
		return -1;
	}
	
	// create our include header.
	fprintf(EnumIncludeFile, "#if defined _tf2equip_regions_included\n\t#endinput\n#endif\n#define _tf2equip_regions_included\n\n");
	
	// create our keyvalues and read the item schema.
	uint32_t i = 1;
	struct KeyVal *items_game = KeyVal_New(NULL);
	KeyVal_ReadItemSchema(items_game);
	
	// create our enum for the equip regions since certain items can have multiple equip regions.
	fprintf(EnumIncludeFile, "\nenum {");
	fprintf(EnumIncludeFile, "\n\tRegion_Invalid = 0,");
	{
		struct KeyVal *equip_regions_list = KeyVal_FindByKeyName(items_game, "equip_regions_list");
		for( struct KeyVal *regions = KeyVal_GetSubKey(equip_regions_list) ; regions ; regions=KeyVal_GetNextKey(regions) ) {
			// some regions are shared, account for the individual regions
			if( regions->Data.Type != TypeKeyval ) {
				fprintf(EnumIncludeFile, "\n\tRegion_%s = 0x%x,", KeyVal_GetKeyName(regions), i);
			}
			// now we deal with the shared.
			else if( regions->Data.Type == TypeKeyval ) {
				for( struct KeyVal *shared = KeyVal_GetSubKey(regions) ; shared ; shared=KeyVal_GetNextKey(shared) )
					fprintf(EnumIncludeFile, "\n\tRegion_%s = 0x%x,", KeyVal_GetKeyName(shared), i);
			}
			i <<= 1;
		}
	}
	fprintf(EnumIncludeFile, "\n};\n");
	
#ifdef PROFILE
	clock_t start = clock();
#endif
	
	// create our function to retrieve what equip regions a particular item is.
	fprintf(EnumIncludeFile, "\nstock int RetrieveItemEquipRegions(const int itemindex)\n{");
	fprintf(EnumIncludeFile, "\n\tswitch( itemindex ) {");
	{
		struct KeyVal *items = KeyVal_FindByKeyName(items_game, "items");
		for( struct KeyVal *kvitem = KeyVal_GetSubKey(items) ; kvitem ; kvitem=KeyVal_GetNextKey(kvitem) ) {
			if( KeyVal_HasKey(kvitem, "equip_region") ) {
				fprintf(EnumIncludeFile, "\n\t\tcase %s: return ", KeyVal_GetKeyName(kvitem));
				struct KeyVal *i = KeyVal_FindByKeyName(kvitem, "equip_region");
				if( !i )
					continue;
				if( i->Data.Type==TypeKeyval ) {
					struct KeyVal *shared = i->Data.Val.Keyval;
					while( shared ) {
						fprintf(EnumIncludeFile, "Region_%s", KeyVal_GetKeyName(shared));
						shared=KeyVal_GetNextKey(shared);
						if( shared )
							fprintf(EnumIncludeFile, " | ");
					}
				}
				else if( i->Data.Type != TypeKeyval )
					fprintf(EnumIncludeFile, "Region_%s", KeyVal_Get(i).Val.Str->CStr);
				fprintf(EnumIncludeFile, ";");
			}
		}
		fprintf(EnumIncludeFile, "\n\t\tdefault: return Region_Invalid;");
	}
	fprintf(EnumIncludeFile, "\n\t}\n}");
	
#ifdef PROFILE
	printf("Keyvalue ReadItemSchema Profile Time: %Lf\n", (clock()-start)/(long double)CLOCKS_PER_SEC);
#endif
	
	KeyVal_Free(&items_game);
	fclose(EnumIncludeFile); EnumIncludeFile = NULL;
	printf("items_game is NULL? '%s'\n", items_game ? "no" : "yes");
}