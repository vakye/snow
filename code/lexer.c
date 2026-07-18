
#pragma once

typedef enum
{
    TokenKind_EOF = 0,

    TokenKind_Integer = 128,
    TokenKind_Identifier,

    TokenKind_DoubleEqual,      // NOTE(vak): ==
    TokenKind_ExclamEqual,      // NOTE(vak): !=
    TokenKind_LessEqual,        // NOTE(vak): <=
    TokenKind_GreaterEqual,     // NOTE(vak): >=

    TokenKind_DoubleLess,       // NOTE(vak): <<
    TokenKind_DoubleGreater,    // NOTE(vak): >>

    TokenKind_DoubleAmpersand,  // NOTE(vak): &&
    TokenKind_DoubleBar,        // NOTE(vak): ||

    TokenKind_ColonEqual,       // NOTE(vak): :=
} token_kind;

typedef struct
{
    token_kind Kind;
    usize From;
    usize Size;
} token;

typedef struct
{
    char* Data;
    usize Size;
    usize At;

    token Current;
} token_stream;

local b32 IsPrintable(char Character)
{
    b32 Result = ((Character >= 32) && (Character <= 126));
    return (Result);
}

local b32 IsWhitespace(char Character)
{
    b32 Result =
        (Character == ' ') ||
        (Character == '\t') ||
        (Character == '\r') ||
        (Character == '\n');

    return (Result);
}

local b32 IsDigit(char Character)
{
    b32 Result =
        (Character >= '0') &&
        (Character <= '9');

    return (Result);
}

local b32 IsLowercase(char Character)
{
    b32 Result =
        (Character >= 'a') &&
        (Character <= 'z');

    return (Result);
}

local b32 IsUppercase(char Character)
{
    b32 Result =
        (Character >= 'A') &&
        (Character <= 'Z');

    return (Result);
}

local b32 IsAlphabet(char Character)
{
    b32 Result = IsLowercase(Character) || IsUppercase(Character);
    return (Result);
}

local b32 IsIdentifierStart(char Character)
{
    b32 Result = (Character == '_') || IsAlphabet(Character);
    return (Result);
}

local b32 IsIdentifier(char Character)
{
    b32 Result = IsIdentifierStart(Character) || IsDigit(Character);
    return (Result);
}

local token TokenizeDigit(token_stream* Stream)
{
    token Result =
    {
        .Kind = TokenKind_Integer,
        .From = Stream->At,
        .Size = 0,
    };

    Stream->At++;

    while (Stream->At < Stream->Size)
    {
        char Character = Stream->Data[Stream->At];

        if (!IsDigit(Character))
            break;

        Stream->At++;
    }

    Result.Size = Stream->At - Result.From;

    return (Result);
}

local token TokenizeIdentifier(token_stream* Stream)
{
    token Result =
    {
        .Kind = TokenKind_Identifier,
        .From = Stream->At,
        .Size = 0,
    };

    Stream->At++;

    while (Stream->At < Stream->Size)
    {
        char Character = Stream->Data[Stream->At];

        if (!IsIdentifier(Character))
            break;

        Stream->At++;
    }

    Result.Size = Stream->At - Result.From;

    return (Result);
}

local token TokenizePunctuation(token_stream* Stream)
{
    char Character = Stream->Data[Stream->At];

    token Result =
    {
        .Kind = (token_kind)Character,
        .From = Stream->At,
        .Size = 1
    };

    if (Stream->At + 2 <= Stream->Size)
    {
        char C0 = Character;
        char C1 = Stream->Data[Stream->At + 1];

        u16 Compare = ((u16)C1 << 8) | (C0);

        switch (Compare)
        {
            #define MatchToTokenKind(C0, C1, TokenKind) \
                case ((C1 << 8) | (C0)): Result.Size = 2; Result.Kind = TokenKind; break;

            MatchToTokenKind('=', '=', TokenKind_DoubleEqual)
            MatchToTokenKind('!', '=', TokenKind_ExclamEqual)
            MatchToTokenKind('<', '=', TokenKind_LessEqual)
            MatchToTokenKind('>', '=', TokenKind_GreaterEqual)

            MatchToTokenKind('<', '<', TokenKind_DoubleLess)
            MatchToTokenKind('>', '>', TokenKind_DoubleGreater)

            MatchToTokenKind('&', '&', TokenKind_DoubleAmpersand)
            MatchToTokenKind('|', '|', TokenKind_DoubleBar)

            MatchToTokenKind(':', '=', TokenKind_ColonEqual)

            #undef MatchToTokenKind
        }
    }

    Stream->At += Result.Size;

    return (Result);
}

local void NextToken(token_stream* Stream)
{
    while (Stream->At < Stream->Size)
    {
        char Character = Stream->Data[Stream->At];

        if (!IsWhitespace(Character))
            break;

        Stream->At++;
    }

    token Token = {0};

    if (Stream->At < Stream->Size)
    {
        char Character = Stream->Data[Stream->At];

        if (IsDigit(Character))
        {
            Token = TokenizeDigit(Stream);
        }
        else if (IsIdentifierStart(Character))
        {
            Token = TokenizeIdentifier(Stream);
        }
        else if (IsPrintable(Character))
        {
            Token = TokenizePunctuation(Stream);
        }
        else
        {
            Print(Str("Unknown character '"));
            PrintCharacter(Character);
            Print(Str("'\n"));
            Exit(1);
        }
    }
    else
    {
        Token.Kind = TokenKind_EOF;
        Token.From = Stream->At;
        Token.Size = 0;
    }

    Stream->Current = Token;
}

local token_stream MakeTokenStream(string String)
{
    token_stream Stream =
    {
        .Data = String.Data,
        .Size = String.Size,
    };

    NextToken(&Stream);

    return (Stream);
}

local b32 NoMoreTokens(token_stream* Stream)
{
    b32 Result = (Stream->Current.Kind == TokenKind_EOF);
    return (Result);
}

local token GetCurrentToken(token_stream* Stream)
{
    return Stream->Current;
}

local string GetTokenString(token_stream* Stream, token Token)
{
    if (Token.From + Token.Size > Stream->At)
    {
        Println(Str("Token string overflow"));
        Exit(1);
    }

    string Result = StrData(Stream->Data + Token.From, Token.Size);
    return (Result);
}

local usize GetTokenInteger(token_stream* Stream, token Token)
{
    if (Token.Kind != TokenKind_Integer)
    {
        Println(Str("Invalid token passed into GetTokenInteger()"));
        Exit(1);
    }

    usize Result = 0;

    string String = GetTokenString(Stream, Token);
    for (usize Index = 0; Index < String.Size; Index++)
    {
        Result *= 10;
        Result += (String.Data[Index] - '0');
    }

    return (Result);
}

local b32 MatchToken(token_stream* Stream, token_kind Kind)
{
    b32 Result = (Stream->Current.Kind == Kind);
    return (Result);
}

local b32 MatchAndNextToken(token_stream* Stream, token_kind Kind)
{
    b32 Result = (Stream->Current.Kind == Kind);

    if (Result)
        NextToken(Stream);

    return (Result);
}

local b32 MatchStringAndNextToken(token_stream* Stream, string Match)
{
    string String = GetTokenString(Stream, Stream->Current);

    b32 Result = StringIsEqual(String, Match);

    if (Result)
        NextToken(Stream);

    return (Result);
}

