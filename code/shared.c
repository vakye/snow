
#pragma once

#define local static
#define persist static

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define KB(Amount) ((ssize)(Amount) << 10)
#define MB(Amount) ((ssize)(Amount) << 20)
#define GB(Amount) ((ssize)(Amount) << 30)
#define TB(Amount) ((ssize)(Amount) << 40)

#define Align(Value, PowerOf2) (((Value) + (PowerOf2) - 1) & ~((PowerOf2) - 1))

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef s64 ssize;
typedef u64 usize;

typedef u8 b8;
typedef u32 b32;

#define true 1
#define false 0

#define S8Min  ((s8 )(0x80))
#define S16Min ((s16)(0x8000))
#define S32Min ((s32)(0x80000000))
#define S64Min ((s64)(0x8000000000000000))

#define S8Max  ((s8 )(0x7F))
#define S16Max ((s16)(0x7FFF))
#define S32Max ((s32)(0x7FFFFFFF))
#define S64Max ((s64)(0x7FFFFFFFFFFFFFFF))

#define U8Max  ((u8 )(0xFF))
#define U16Max ((u16)(0xFFFF))
#define U32Max ((u32)(0xFFFFFFFF))
#define U64Max ((u64)(0xFFFFFFFFFFFFFFFF))

#define SSizeMin S64Min
#define SSizeMax S64Max
#define USizeMax U64Max

local void CopyMemory(void* DestInit, void* SourceInit, usize Size)
{
    u8* Dest = (u8*)DestInit;
    u8* Source = (u8*)SourceInit;

    while (Size--)
        *Dest++ = *Source++;
}

local void ZeroMemory(void* DestInit, usize Size)
{
    u8* Dest = (u8*)DestInit;

    while (Size--) *Dest++ = 0;
}

typedef struct
{
    char* Data;
    usize Size;
} string;

#define StaticStr(Literal)  {Literal, sizeof(Literal) - 1}
#define Str(Literal)        (string){Literal, sizeof(Literal) - 1}
#define StrData(Data, Size) (string){Data, Size}

local b32 StringIsEqual(string A, string B)
{
    b32 Result = (A.Size == B.Size);

    if (Result)
    {
        for (usize Index = 0; Index < A.Size; Index++)
        {
            if (A.Data[Index] != B.Data[Index])
            {
                Result = false;
                break;
            }
        }
    }

    return (Result);
}

