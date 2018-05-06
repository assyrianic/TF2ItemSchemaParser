#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <iso646.h>

struct Variant;

union Value {
	bool Bool, *BoolPtr, (*BoolFunc)(), *(*BoolPtrFunc)();
	int8_t Int8, *Int8Ptr, (*Int8Func)(), *(*Int8PtrFunc)();
	int16_t Int16, *Int16Ptr, (*Int16Func)(), *(*Int16PtrFunc)();
	int32_t Int32, *Int32Ptr, (*Int32Func)(), *(*Int32PtrFunc)();
	int64_t Int64, *Int64Ptr, (*Int64Func)(), *(*Int64PtrFunc)();
	
	uint8_t UInt8, *UInt8Ptr, (*UInt8Func)(), *(*UInt8PtrFunc)();
	uint16_t UInt16, *UInt16Ptr, (*UInt16Func)(), *(*UInt16PtrFunc)();
	uint32_t UInt32, *UInt32Ptr, (*UInt32Func)(), *(*UInt32PtrFunc)();
	uint64_t UInt64, *UInt64Ptr, (*UInt64Func)(), *(*UInt64PtrFunc)();
	
	float Float, *FloatPtr, (*FloatFunc)(), *(*FloatPtrFunc)();
	double Double, *DoublePtr, (*DoubleFunc)(), *(*DoublePtrFunc)();
	
	void *Ptr, **PtrPtr, (*VoidFunc)(), *(*VoidPtrFunc)();
	struct String *Str;
	struct KeyVal *Keyval;
	
	union Value *SelfPtr, (*SelfFunc)(), *(*SelfPtrFunc)();
};

typedef enum ValType {
	TypeInvalid=0,
	// integer types
	TypeBool, TypeInt8, TypeInt16, TypeInt32, TypeInt64,
	TypeUInt8, TypeUInt16, TypeUInt32, TypeUInt64,
	TypeBoolPtr, TypeInt8Ptr, TypeInt16Ptr, TypeInt32Ptr, TypeInt64Ptr,
	TypeUInt8Ptr, TypeUInt16Ptr, TypeUInt32Ptr, TypeUInt64Ptr,
	// floating point types
	TypeFloat, TypeDouble,
	TypeFloatPtr, TypeDoublePtr,
	// misc.
	TypePtr, TypePtrPtr,
	TypeStr, TypeKeyval,
	TypeSelf, TypeSelfFunc,
	
	// func ptrs
	TypeBoolFunc, TypeInt8Func, TypeInt16Func, TypeInt32Func, TypeInt64Func,
	TypeUInt8Func, TypeUInt16Func, TypeUInt32Func, TypeUInt64Func,
	TypeBoolPtrFunc, TypeInt8PtrFunc, TypeInt16PtrFunc, TypeInt32PtrFunc, TypeInt64PtrFunc,
	TypeUInt8PtrFunc, TypeUInt16PtrFunc, TypeUInt32PtrFunc, TypeUInt64PtrFunc,
	
	TypeFloatFunc, TypeDoubleFunc,
	TypeFloatPtrFunc, TypeDoublePtrFunc,
	TypePtrFunc, TypeVoidFunc,
} ValType;

// discriminated union type!
struct Variant {
	union Value Val;
	enum ValType Type;
};

/************* C++ Style String *************/
/* Purpose:
 * to automate string data and handling.
 */
struct String {
	char *CStr;
	size_t Len;
};

struct String *String_New(void);
void String_Del(struct String *);
void String_Free(struct String **);
void String_Init(struct String *);
void String_AddChar(struct String *, char);
void String_Add(struct String *, const struct String *);
void String_AddStr(struct String *, const char *);
char *String_GetStr(const struct String *);
size_t String_Len(const struct String *);
void String_Copy(struct String *, const struct String *);
void String_CopyStr(struct String *, const char *);
int32_t String_StrCmpCStr(const struct String *, const char *);
int32_t String_StrCmpStr(const struct String *, const struct String *);
int32_t String_StrnCmpCStr(const struct String *, const char *, size_t);
int32_t String_StrnCmpStr(const struct String *, const struct String *, size_t);
bool String_IsEmpty(const struct String *);
/***************/

/************* Binary Tree *************/

struct KeyVal {
	struct String KeyName;
	struct Variant Data;
	bool (*Destructor)();
	struct KeyVal *NextKey;
};

struct KeyVal *KeyVal_New(bool (*)());
struct KeyVal *KeyVal_NewS(char *, bool (*)());
void KeyVal_Free(struct KeyVal **);
void KeyVal_Init(struct KeyVal *);
void KeyVal_Print(struct KeyVal *);

struct Variant KeyVal_Get(struct KeyVal *);
struct KeyVal *KeyVal_GetSubKey(struct KeyVal *);
struct KeyVal *KeyVal_GetNextKey(struct KeyVal *);
char *KeyVal_GetKeyName(struct KeyVal *);

void KeyVal_Set(struct KeyVal *, struct Variant);
void KeyVal_SetSubKey(struct KeyVal *, struct KeyVal *);
void KeyVal_SetNextKey(struct KeyVal *, struct KeyVal *);
void KeyVal_SetKeyName(struct KeyVal *, char *);

struct KeyVal *KeyVal_GetFirstSubKey(struct KeyVal *);
struct KeyVal *KeyVal_GetNextSubKey(struct KeyVal *);
struct KeyVal *KeyVal_FindLastSubKey(struct KeyVal *);

struct KeyVal *KeyVal_FindByKeyName(struct KeyVal *, char *);
bool KeyVal_HasKey(struct KeyVal *, char *);
bool KeyVal_ReadItemSchema(struct KeyVal *);

/***************/

#ifdef __cplusplus
}
#endif
