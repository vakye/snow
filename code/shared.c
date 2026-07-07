
#pragma once

#define local static
#define persist static

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

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

