
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "dsc.h"

/*
struct KeyVal {
	struct String KeyName;
	struct Variant Data;
	bool (*Destructor)();
	struct KeyVal *NextKey;
};
*/

struct KeyVal *KeyVal_New(bool (*const dtor)())
{
	struct KeyVal *node = calloc(1, sizeof *node);
	if( node ) {
		node->Destructor = dtor;
	}
	return node;
}

struct KeyVal *KeyVal_NewS(char *restrict strKeyName, bool (*const dtor)())
{
	struct KeyVal *node = calloc(1, sizeof *node);
	if( node ) {
		String_CopyStr(&node->KeyName, strKeyName);
		node->Destructor = dtor;
	}
	return node;
}

bool KeyVal_Free(struct KeyVal **kv)
{
	if( !*kv )
		return false;
	
	struct KeyVal *iter = *kv;
	String_Del(&iter->KeyName);
	
	if( iter->Destructor )
		(*iter->Destructor)(&iter->Data.Val.Ptr);
	
	if( iter->NextKey )
		KeyVal_Free(&iter->NextKey);
	free(*kv); *kv=NULL;
	iter=NULL;
	return true;
}

void KeyVal_Init(struct KeyVal *const kv)
{
	if( !kv )
		return;
	else *kv = (struct KeyVal){0};
}


void KeyVal_Print(struct KeyVal *const kv)
{
	if( !kv )
		return;
	
	printf("KeyVal_Print :: Key Name: %s\n", kv->KeyName.CStr);
	if( kv->Data.Type==TypeKeyval ) {
		puts("Printing SubKey");
		KeyVal_Print(kv->Data.Val.Keyval);
	}
	else if( kv->Data.Type==TypeStr )
		printf("KeyVal_Print :: String Value: %s\n", kv->Data.Val.Str->CStr);
	else printf("KeyVal_Print :: Int Value: %llu\n", kv->Data.Val.UInt64);
	
	if( kv->NextKey ) {
		puts("Printing NextKey");
		KeyVal_Print(kv->NextKey);
	}
}

struct Variant KeyVal_Get(struct KeyVal *const kv)
{
	return !kv ? (struct Variant){0} : kv->Data;
}

struct String *KeyVal_GetStr(struct KeyVal *const kv)
{
	return !kv or kv->Data.Type != TypeStr ? NULL : kv->Data.Val.Str;
}

struct KeyVal *KeyVal_GetSubKey(struct KeyVal *const kv)
{
	return !kv or kv->Data.Type != TypeKeyval ? NULL : kv->Data.Val.Keyval;
}

struct KeyVal *KeyVal_GetNextKey(struct KeyVal *const kv)
{
	return !kv ? NULL : kv->NextKey;
}

char *KeyVal_GetKeyName(struct KeyVal *const restrict kv)
{
	return !kv ? NULL : kv->KeyName.CStr;
}


void KeyVal_Set(struct KeyVal *const restrict kv, const struct Variant val)
{
	if( !kv )
		return;
	
	kv->Data = val;
}

void KeyVal_SetSubKey(struct KeyVal *const restrict kv, struct KeyVal *const subkey)
{
	if( !kv or !subkey )
		return;
	
	kv->Data.Val.Keyval = subkey;
	kv->Data.Type = TypeKeyval;
}

void KeyVal_SetNextKey(struct KeyVal *const restrict kv, struct KeyVal *const nextkey)
{
	if( !kv or !nextkey )
		return;
	
	kv->NextKey = nextkey;
}

void KeyVal_SetKeyName(struct KeyVal *const restrict kv, char *restrict cstr)
{
	if( !kv or !cstr )
		return;
	
	String_CopyStr(&kv->KeyName, cstr);
}

struct KeyVal *KeyVal_GetFirstSubKey(struct KeyVal *const kv)
{
	if( !kv )
		return NULL;
	
	struct KeyVal *Ret = kv->Data.Val.Keyval;
	while( Ret and Ret->Data.Type != TypeKeyval )
		Ret = Ret->NextKey;
	
	return Ret;
}

struct KeyVal *KeyVal_GetNextSubKey(struct KeyVal *const kv)
{
	if( !kv )
		return NULL;
	
	struct KeyVal *Ret = kv->NextKey;
	while( Ret and Ret->Data.Type != TypeKeyval )
		Ret = Ret->NextKey;
	
	return Ret;
}

struct KeyVal *KeyVal_FindLastSubKey(struct KeyVal *const kv)
{
	if( !kv )
		return NULL;
	else if( kv->Data.Type==TypeKeyval and !kv->Data.Val.Keyval )
		return NULL;
	
	// Scan for the last one
	struct KeyVal *lastKid = kv->Data.Val.Keyval;
	while( lastKid->NextKey )
		lastKid = lastKid->NextKey;
	return lastKid;
}

struct KeyVal *KeyVal_FindByKeyName(struct KeyVal *const restrict kv, char *restrict cstr)
{
	if( !kv or !cstr )
		return NULL;
	
	struct KeyVal *k=NULL;
	// iterate through our linked keys first.
	for( k=kv->Data.Val.Keyval ; k ; k=k->NextKey ) {
		if( !String_StrCmpCStr(&k->KeyName, cstr) )
			break;
	}
	return k;
}


bool KeyVal_HasKey(struct KeyVal *const restrict kv, char *restrict cstr)
{
	if( !kv or !cstr )
		return false;
	
	struct KeyVal *k=NULL;
	// iterate through our linked keys first.
	for( k=kv->Data.Val.Keyval ; k ; k=k->NextKey ) {
		if( !String_StrCmpCStr(&k->KeyName, cstr) )
			break;
	}
	return k != NULL;
}

static bool IsWhiteSpace(const char c)
{
	return( c==' ' or c=='\t' or c=='\r' or c=='\v' or c=='\f' or c=='\n' );
}


enum {
	KVBuffer, 
	KVIter, 
	KVEnd,
};

bool KeyVal_RecursiveBuild(struct KeyVal *const kv, char *kvdata[static 3])
{
	if( !kv or !kvdata )
		return false;
	
	struct KeyVal
		*kvIter = kv,	// change between pointers.
		*kvNext = NULL	// to check if we setup the next list key.
	;
	do {
		// make sure we didn't hit null terminator.
		if( !*kvdata[KVIter] ) {
			puts("KeyVal_RecursiveBuild :: Expected closing brace.");
			if( kvNext )
				KeyVal_Free(&kvNext);
			break;
		}
		// ignore whitespace.
		else if( IsWhiteSpace(*kvdata[KVIter]) ) {
			kvdata[KVIter]++;
			continue;
		}
		// handle Python and C++ style single-line comments.
		else if( *kvdata[KVIter]=='#' or (*kvdata[KVIter]=='/' and kvdata[KVIter][1]=='/') ) {
			for( ; *kvdata[KVIter] != '\n' ; kvdata[KVIter]++ );
			continue;
		}
		// Handle C style multiline comments.
		else if( *kvdata[KVIter]=='/' and kvdata[KVIter][1]=='*' ) {
			// skip past '/' and '*'
			kvdata[KVIter]++;
			do {
				kvdata[KVIter]++;
				if( !kvdata[KVIter][1] )
					break;
			} while( *kvdata[KVIter]!='*' and kvdata[KVIter][1]!='/' );
		}
		
		// ending brace? kill the loop and free our next key ptr.
		if( *kvdata[KVIter]=='}' ) {
			//puts("Found a Closing Brace!");
			if( kvNext )
				KeyVal_Free(&kvNext);
			break;
		}
		// section not closed yet, set our next key value.
		if( kvNext ) {
			kvIter->NextKey = kvNext;
			kvIter = kvNext;
			kvNext = NULL;
		}
		
		// found a starting quote.
		if( *kvdata[KVIter]=='"' ) {
			//puts("Found a Key!");
			// forward iterator to first char in keyname.
			kvdata[KVIter]++;
			
			// copy the entire name to our string object.
			while( *kvdata[KVIter] != '"' and kvdata[KVIter]<kvdata[KVEnd] )
				String_AddChar(&kvIter->KeyName, *kvdata[KVIter]++);
			//printf("KeyVal_RecursiveBuild :: KeyName == %s\n", String_GetStr(kvIter->KeyName));
			
			// forward iterator past last quote.
			kvdata[KVIter]++;
			// make sure we skip over everything else in between keyname and value.
			while( IsWhiteSpace(*kvdata[KVIter]) )
				kvdata[KVIter]++;
			
			// another quote? It's a value.
			if( *kvdata[KVIter]=='"' ) {
				//puts("found a Value!");
				kvdata[KVIter]++;
				bool IsNum = *kvdata[KVIter] >= '0' and *kvdata[KVIter] <= '9';
				struct String *strVal=NULL;
				if( !IsNum )
					strVal = String_New();
				uint64_t i = 0;
				while( *kvdata[KVIter] != '"' and kvdata[KVIter]<kvdata[KVEnd] ) {
					if( IsNum ) {
						if( *kvdata[KVIter]=='.' )
							kvdata[KVIter]++;
						i = i * 10 + *kvdata[KVIter]++ - '0';
					}
					// treat value entirely as string.
					else String_AddChar(strVal, tolower(*kvdata[KVIter]++));
				}
				
				kvIter->Data.Val.UInt64 = i;
				kvdata[KVIter]++;
				//printf("KeyVal_RecursiveBuild :: LongValue == %llu\n", kvIter->ULongVal);
				kvNext = KeyVal_New(NULL);
				if( !IsNum ) {
					kvIter->Destructor = String_Free;
					kvIter->Data.Val.Str = strVal;
					kvIter->Data.Type = TypeStr;
				}
				else kvIter->Data.Type = TypeUInt64;
			}
			// starting brace? parse subsection.
			else if( *kvdata[KVIter]=='{' ) {
				// iterate past the starting brace.
				kvdata[KVIter]++;
				//puts("found a Section!");
				// allocate subkey to build subsection.
				kvIter->Destructor = KeyVal_Free;
				kvIter->Data.Val.Keyval = KeyVal_New(NULL);
				KeyVal_RecursiveBuild(kvIter->Data.Val.Keyval, kvdata);
				kvIter->Data.Type = TypeKeyval;
				kvdata[KVIter]++;
				kvNext = KeyVal_New(NULL);
			}
		}
		kvdata[KVIter]++;
	} while( *kvdata[KVIter] );
	
	// if the next keyval is still allocated, free it cuz we're done here.
	if( kvNext )
		KeyVal_Free(&kvNext);
	return true;
}

bool KeyVal_ReadItemSchema(struct KeyVal *const restrict root)
{
	if( !root )
		return false;
	
	/*
	 * control structures is the double quotes "" and {} brackets.
	 * assuming the root ptr is ready to use, let's read in the file to a buffer.
	 */
	FILE *restrict file = fopen("items_game.txt", "r+");
	if( !file ) {
		puts("KeyVal_ReadItemSchema :: **** ERROR: file \'items_game.txt\' not found ****");
		return false;
	}
	
	/* get file Len so we can allocate our char* buffer. */
	fseek(file, 0, SEEK_END);
	const long int telldata = ftell(file);
	if( telldata <= -1L ) {
		puts("KeyVal_ReadItemSchema :: **** ERROR: filesize less than 1 ****");
		fclose(file), file=NULL;
		return false;
	}
	const size_t filesize = (size_t)telldata;
	rewind(file);
	
	/* allocate buffer and read in file. */
	char *buffer = calloc(filesize+2, sizeof *buffer);
	if( !buffer ) {
		puts("KeyVal_ReadItemSchema :: **** ERROR: unable to allocate file buffer. ****");
		fclose(file), file=NULL;
		return false;
	}
	const size_t errcheck = fread(buffer, sizeof *buffer, filesize, file);
	fclose(file), file=NULL;
	if( errcheck != filesize ) {
		puts("KeyVal_ReadItemSchema :: **** ERROR: error reading file to buffer. ****");
		free(buffer), buffer=NULL;
		return false;
	}
	buffer[filesize] = 0;
	
	/*
	 * buffer fully read in, use an iterator to lex and parse the .ini
	 * make sure we don't overrun the buffer's boundary.
	 */
	char *KVDatum[3]={ buffer, buffer, buffer+filesize };
	//printf("buffer == \n%s\n\n", buffer);
	
	/* do a recursive read and build our tree. */
	bool r = KeyVal_RecursiveBuild(root, KVDatum);
	free(buffer), buffer=NULL;
	return r;
}

void KeyVal_GenerateEnum(struct KeyVal *const kv, FILE *const file)
{
	if( !kv or !file )
		return;
	
	uint32_t i = 1;
	// create our enum for the equip regions since certain items can have multiple equip regions.
	fprintf(file, "\nenum {");
	fprintf(file, "\n\tRegion_Invalid = 0,");
	struct KeyVal *equip_regions_list = KeyVal_FindByKeyName(kv, "equip_regions_list");
	for( struct KeyVal *regions = KeyVal_GetSubKey(equip_regions_list) ; regions ; regions=KeyVal_GetNextKey(regions) ) {
		// some regions are shared, account for the individual regions
		if( regions->Data.Type != TypeKeyval ) {
			fprintf(file, "\n\tRegion_%s = 0x%x,", KeyVal_GetKeyName(regions), i);
		}
		// now we deal with the shared.
		else if( regions->Data.Type == TypeKeyval ) {
			for( struct KeyVal *shared = KeyVal_GetSubKey(regions) ; shared ; shared=KeyVal_GetNextKey(shared) )
				fprintf(file, "\n\tRegion_%s = 0x%x,", KeyVal_GetKeyName(shared), i);
		}
		i <<= 1;
	}
	fprintf(file, "\n};\n");
}

void KeyVal_GenerateStockFunc(struct KeyVal *const kv, FILE *const file)
{
	if( !kv or !file )
		return;
	
	// generate our function header to retrieve what equip regions a particular item occupies.
	fprintf(file, "\nstock int GetItemEquipRegions(const int itemindex)\n{");
	
	// generate the switch
	fprintf(file, "\n\tswitch( itemindex ) {");
	{
		struct KeyVal *items = KeyVal_FindByKeyName(kv, "items");
		// sometimes, a cosmetic doesn't have any equip regions
		// but is based on a prefab, which MIGHT have equip regions...
		struct KeyVal *prefabs = KeyVal_FindByKeyName(kv, "prefabs");
		
		for( struct KeyVal *kvitem = KeyVal_GetSubKey(items) ; kvitem ; kvitem=KeyVal_GetNextKey(kvitem) ) {
			// make sure the item actually has an equipment region.
			// some items have "equip_regions", adjust for that as well.
			struct String switchbody = (struct String){0};
			bool has_region = KeyVal_HasKey(kvitem, "equip_region");
			bool has_regions = KeyVal_HasKey(kvitem, "equip_regions");
			bool has_prefab = KeyVal_HasKey(kvitem, "prefab");
			uint8_t writeflags = 0;
			
			#define WROTE_REGION	1
			#define WROTE_BIT_OR	2
			
			String_AddStr(&switchbody, "\n\t\tcase ");
			String_AddStr(&switchbody, KeyVal_GetKeyName(kvitem));
			String_AddStr(&switchbody, ": return ");
			
			struct KeyVal *regiondata = NULL;
			if( has_region )
				regiondata = KeyVal_FindByKeyName(kvitem, "equip_region");
			else if( has_regions )
				regiondata = KeyVal_FindByKeyName(kvitem, "equip_regions");
			
			// some items take up multiple regions which is why I implemented regions as flags.
			if( regiondata and regiondata->Data.Type==TypeKeyval ) {
				// gotta use a while-loop for this one, we gotta check if the next keyvalue pointer
				// is null or not so we know whether to add the bitwise-OR or not.
				struct KeyVal *shared = regiondata->Data.Val.Keyval;
				while( shared ) {
					writeflags |= WROTE_REGION;
					writeflags &= ~WROTE_BIT_OR;
					String_AddStr(&switchbody, "Region_");
					String_AddStr(&switchbody, KeyVal_GetKeyName(shared));
					shared=KeyVal_GetNextKey(shared);
					if( shared ) {
						String_AddStr(&switchbody, " | ");
						writeflags |= WROTE_BIT_OR;
					}
				}
			}
			else if( regiondata and regiondata->Data.Type != TypeKeyval ) {
				writeflags |= WROTE_REGION;
				String_AddStr(&switchbody, "Region_");
				String_AddStr(&switchbody, KeyVal_Get(regiondata).Val.Str->CStr);
			}
			
			// this one is tricky dicky.
			if( has_prefab ) {
				struct KeyVal *item_prefab = KeyVal_FindByKeyName(kvitem, "prefab");
				regiondata = KeyVal_FindByKeyName(prefabs, KeyVal_Get(item_prefab).Val.Str->CStr);
				while( regiondata ) {
					if( KeyVal_HasKey(regiondata, "equip_region") ) {
						if( writeflags & WROTE_REGION )
							String_AddStr(&switchbody, " | ");
						writeflags |= WROTE_REGION;
						writeflags &= ~WROTE_BIT_OR;
						String_AddStr(&switchbody, "Region_");
						String_AddStr(&switchbody, KeyVal_Get(KeyVal_FindByKeyName(regiondata, "equip_region")).Val.Str->CStr);
					}
					
					// check if the prefab has a nested prefab.
					item_prefab = KeyVal_FindByKeyName(regiondata, "prefab");
					if( !item_prefab ) {
						regiondata=NULL;
						break;
					}
					regiondata = KeyVal_FindByKeyName(prefabs, KeyVal_Get(item_prefab).Val.Str->CStr);
					if( regiondata and KeyVal_HasKey(regiondata, "equip_region") /*(writeflags & WROTE_REGION)*/ ) {
						writeflags |= WROTE_BIT_OR;
						writeflags &= ~WROTE_REGION;
						String_AddStr(&switchbody, " | ");
					}
				}
			}
			if( writeflags )
				fprintf(file, "%s;", switchbody.CStr);
			String_Del(&switchbody);
		}
		// add a default case for good measure.
		fprintf(file, "\n\t\tdefault: return Region_Invalid;");
	}
	// EDIT: ughhh, have to add this for the end of the function even though default handles the remaining cases -_-.
	// why can't I do plugins in C?
	fprintf(file, "\n\t}\n\treturn Region_Invalid;\n}");
}

// https://www.youtube.com/watch?v=oY2KnW19Tls
