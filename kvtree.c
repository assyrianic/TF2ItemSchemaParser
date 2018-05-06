
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

/*
struct KeyVal {
	struct String KeyName;
	struct Variant Data;
	bool (*Destructor)();
	struct KeyVal *NextKey;
};
*/

struct KeyVal *KeyVal_New(bool (*dtor)())
{
	struct KeyVal *node = calloc(1, sizeof *node);
	if( node ) {
		node->Destructor = dtor;
	}
	return node;
}

struct KeyVal *KeyVal_NewS(char *restrict strKeyName, bool (*dtor)())
{
	struct KeyVal *node = calloc(1, sizeof *node);
	if( node ) {
		String_CopyStr(&node->KeyName, strKeyName);
		node->Destructor = dtor;
	}
	return node;
}

void KeyVal_Free(struct KeyVal **restrict kv)
{
	if( !*kv )
		return;
	
	struct KeyVal *iter = *kv;
	String_Del(&iter->KeyName);
	
	if( iter->Destructor )
		(*iter->Destructor)(&iter->Data.Val.Ptr);
	
	if( iter->NextKey )
		KeyVal_Free(&iter->NextKey);
	free(*kv); *kv=NULL;
	iter=NULL;
}

void KeyVal_Init(struct KeyVal *const restrict kv)
{
	if( !kv )
		return;
	
	*kv = (struct KeyVal){0};
}


void KeyVal_Print(struct KeyVal *const restrict kv)
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

struct Variant KeyVal_Get(struct KeyVal *const restrict kv)
{
	return !kv ? (struct Variant){0} : kv->Data;
}

struct String *KeyVal_GetStr(struct KeyVal *const restrict kv)
{
	return !kv or kv->Data.Type != TypeStr ? NULL : kv->Data.Val.Str;
}

struct KeyVal *KeyVal_GetSubKey(struct KeyVal *const restrict kv)
{
	return !kv or kv->Data.Type != TypeKeyval ? NULL : kv->Data.Val.Keyval;
}

struct KeyVal *KeyVal_GetNextKey(struct KeyVal *const restrict kv)
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

void KeyVal_SetSubKey(struct KeyVal *const restrict kv, struct KeyVal *const restrict subkey)
{
	if( !kv or !subkey )
		return;
	
	kv->Data.Val.Keyval = subkey;
	kv->Data.Type = TypeKeyval;
}

void KeyVal_SetNextKey(struct KeyVal *const restrict kv, struct KeyVal *const restrict nextkey)
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

struct KeyVal *KeyVal_GetFirstSubKey(struct KeyVal *const restrict kv)
{
	if( !kv )
		return NULL;
	
	struct KeyVal *Ret = kv->Data.Val.Keyval;
	while( Ret and Ret->Data.Type != TypeKeyval )
		Ret = Ret->NextKey;
	
	return Ret;
}

struct KeyVal *KeyVal_GetNextSubKey(struct KeyVal *const restrict kv)
{
	if( !kv )
		return NULL;
	
	struct KeyVal *Ret = kv->NextKey;
	while( Ret and Ret->Data.Type != TypeKeyval )
		Ret = Ret->NextKey;
	
	return Ret;
}

struct KeyVal *KeyVal_FindLastSubKey(struct KeyVal *const restrict kv)
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
	KVBuffer=0, KVIter, KVEnd,
};
bool KeyVal_RecursiveBuild(struct KeyVal *const restrict kv, char *kvdata[static 3])
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
			while( *kvdata[KVIter] != '"' and kvdata[KVIter]<=kvdata[KVEnd] )
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
				while( *kvdata[KVIter] != '"' and kvdata[KVIter]<=kvdata[KVEnd] ) {
					if( IsNum ) {
						if( *kvdata[KVIter]=='.' )
							kvdata[KVIter]++;
						i = i * 10 + *kvdata[KVIter]++ - '0';
					}
					// treat value entirely as string.
					else String_AddChar(strVal, *kvdata[KVIter]++);
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
	long int telldata = ftell(file);
	if( telldata <= -1L ) {
		puts("KeyVal_ReadItemSchema :: **** ERROR: filesize less than 1 ****");
		fclose(file), file=NULL;
		return false;
	}
	size_t filesize = (size_t)telldata;
	rewind(file);
	
	/* allocate buffer and read in file. */
	char *buffer = calloc(filesize+1, sizeof *buffer);
	if( !buffer ) {
		puts("KeyVal_ReadItemSchema :: **** ERROR: unable to allocate file buffer. ****");
		fclose(file), file=NULL;
		return false;
	}
	size_t errcheck = fread(buffer, sizeof *buffer, filesize, file);
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
	
	char *KVDatum[3]={NULL};
	KVDatum[KVBuffer] = buffer;
	KVDatum[KVIter] = buffer;
	KVDatum[KVEnd] = buffer + filesize;
	
	//printf("buffer == \n%s\n\n", buffer);
	
	/* do a recursive read and build our tree. */
	bool r = KeyVal_RecursiveBuild(root, KVDatum);
	free(buffer), buffer=NULL;
	return r;
}

// https://www.youtube.com/watch?v=oY2KnW19Tls
