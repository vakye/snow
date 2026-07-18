
#pragma once

typedef enum
{
    NodeKind_Nil = 0,

    NodeKind_Integer,

    NodeKind_Add,
    NodeKind_Sub,
    NodeKind_Mul,
    NodeKind_Div,
    NodeKind_Mod,

    NodeKind_Equal,
    NodeKind_NotEqual,

    NodeKind_Less,
    NodeKind_Greater,
    NodeKind_LessEqual,
    NodeKind_GreaterEqual,

    NodeKind_ShiftLeft,
    NodeKind_ShiftRight,

    NodeKind_BitwiseAnd,
    NodeKind_BitwiseOr,
    NodeKind_BitwiseXor,

    NodeKind_Negate,
    NodeKind_BitwiseNot,
    NodeKind_LogicalNot,
    NodeKind_LogicalAnd,
    NodeKind_LogicalOr,

    NodeKind_Ternary,

    NodeKind_Return,
} node_kind;

typedef struct node node;
struct node
{
    node_kind Kind;
    node* Next;

    union
    {
        struct { node* Left; node* Right; };
        struct { usize Integer; };
        struct { node* TernaryCond; node* TernaryIf; node* TernaryElse; };
    };
};

local node* MakeNode(node_kind Kind)
{
    node* Node = Allocate(sizeof(node));

    ZeroMemory(Node, sizeof(node));
    Node->Kind = Kind;

    return (Node);
}

local node* MakeUnaryNode(node_kind Kind, node* Left)
{
    node* Node = MakeNode(Kind);

    Node->Left = Left;

    return (Node);
}

local node* MakeBinaryNode(node_kind Kind, node* Left, node* Right)
{
    node* Node = MakeNode(Kind);

    Node->Left = Left;
    Node->Right = Right;

    return (Node);
}

local node* MakeIntegerNode(usize Integer)
{
    node* Node = MakeNode(NodeKind_Integer);

    Node->Integer = Integer;

    return (Node);
}

local node* ParseStatement  (token_stream* Stream);
local node* ParseExpression (token_stream* Stream);
local node* ParseTernary    (token_stream* Stream);
local node* ParseLogicalOr  (token_stream* Stream);
local node* ParseLogicalAnd (token_stream* Stream);
local node* ParseBitwiseOr  (token_stream* Stream);
local node* ParseBitwiseXor (token_stream* Stream);
local node* ParseBitwiseAnd (token_stream* Stream);
local node* ParseEquality   (token_stream* Stream);
local node* ParseComparison (token_stream* Stream);
local node* ParseShift      (token_stream* Stream);
local node* ParseSum        (token_stream* Stream);
local node* ParseFactor     (token_stream* Stream);
local node* ParsePrefix     (token_stream* Stream);
local node* ParsePrimary    (token_stream* Stream);

local node* Parse(token_stream* Stream)
{
    node* First = ParseStatement(Stream);
    node* Last = First;

    while (!NoMoreTokens(Stream))
    {
        Last->Next = ParseStatement(Stream);

        if (Last->Next)
            Last = Last->Next;
    }

    return (First);
}

local node* ParseStatement(token_stream* Stream)
{
    node* Node = 0;

    if (MatchStringAndNextToken(Stream, Str("return")))
    {
        Node = MakeUnaryNode(NodeKind_Return, ParseExpression(Stream));

        if (!MatchAndNextToken(Stream, ';'))
        {
            Println(Str("Expected ';' at end of statement"));
            Exit(1);
        }
    }
    else
    {
        Node = ParseExpression(Stream);

        if (!MatchAndNextToken(Stream, ';'))
        {
            Println(Str("Expected ';' at end of statement"));
            Exit(1);
        }
    }

    return (Node);
}

local node* ParseExpression(token_stream* Stream)
{
    node* Node = ParseTernary(Stream);
    return (Node);
}

local node* ParseTernary(token_stream* Stream)
{
    node* Node = ParseLogicalOr(Stream);

    if (MatchAndNextToken(Stream, '?'))
    {
        node* Ternary = MakeNode(NodeKind_Ternary);

        Ternary->TernaryCond = Node;
        Ternary->TernaryIf = ParseTernary(Stream);

        if (MatchAndNextToken(Stream, ':'))
        {
            Ternary->TernaryElse = ParseTernary(Stream);
        }
        else
        {
            Println(Str("Missing ':' in ternary conditional"));
            Exit(1);
        }

        Node = Ternary;
    }

    return (Node);
}

local node* ParseLogicalOr(token_stream* Stream)
{
    node* Node = ParseLogicalAnd(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, TokenKind_DoubleBar))
        {
            Node = MakeBinaryNode(NodeKind_LogicalOr, Node, ParseLogicalAnd(Stream));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseLogicalAnd(token_stream* Stream)
{
    node* Node = ParseBitwiseOr(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, TokenKind_DoubleAmpersand))
        {
            Node = MakeBinaryNode(NodeKind_LogicalAnd, Node, ParseBitwiseOr(Stream));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseBitwiseOr(token_stream* Stream)
{
    node* Node = ParseBitwiseXor(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, '|'))
        {
            Node = MakeBinaryNode(NodeKind_BitwiseOr, Node, ParseBitwiseXor(Stream));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseBitwiseXor(token_stream* Stream)
{
    node* Node = ParseBitwiseAnd(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, '^'))
        {
            Node = MakeBinaryNode(NodeKind_BitwiseXor, Node, ParseBitwiseAnd(Stream));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseBitwiseAnd(token_stream* Stream)
{
    node* Node = ParseEquality(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, '&'))
        {
            Node = MakeBinaryNode(NodeKind_BitwiseAnd, Node, ParseEquality(Stream));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseEquality(token_stream* Stream)
{
    node* Node = ParseComparison(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, TokenKind_DoubleEqual))
        {
            Node = MakeBinaryNode(NodeKind_Equal, Node, ParseComparison(Stream));
        }
        else if (MatchAndNextToken(Stream, TokenKind_ExclamEqual))
        {
            Node = MakeBinaryNode(NodeKind_NotEqual, Node, ParseComparison(Stream));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseComparison(token_stream* Stream)
{
    node* Node = ParseShift(Stream);

    for (;;)
    {
            if (MatchAndNextToken(Stream, '<'))
            {
                Node = MakeBinaryNode(NodeKind_Less, Node, ParseShift(Stream));
            }
            else if (MatchAndNextToken(Stream, '>'))
            {
                Node = MakeBinaryNode(NodeKind_Greater, Node, ParseShift(Stream));
            }
            else if (MatchAndNextToken(Stream, TokenKind_LessEqual))
            {
                Node = MakeBinaryNode(NodeKind_LessEqual, Node, ParseShift(Stream));
            }
            else if (MatchAndNextToken(Stream, TokenKind_GreaterEqual))
            {
                Node = MakeBinaryNode(NodeKind_GreaterEqual, Node, ParseShift(Stream));
            }
            else
            {
                break;
            }
    }

    return (Node);
}

local node* ParseShift(token_stream* Stream)
{
    node* Node = ParseSum(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, TokenKind_DoubleLess))
        {
            Node = MakeBinaryNode(NodeKind_ShiftLeft, Node, ParseSum(Stream));
        }
        else if (MatchAndNextToken(Stream, TokenKind_DoubleGreater))
        {
            Node = MakeBinaryNode(NodeKind_ShiftRight, Node, ParseSum(Stream));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseSum(token_stream* Stream)
{
    node* Node = ParseFactor(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, '+'))
        {
            Node = MakeBinaryNode(NodeKind_Add, Node, ParseFactor(Stream));
        }
        else if (MatchAndNextToken(Stream, '-'))
        {
            Node = MakeBinaryNode(NodeKind_Sub, Node, ParseFactor(Stream));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseFactor(token_stream* Stream)
{
    node* Node = ParsePrefix(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, '*'))
        {
            Node = MakeBinaryNode(NodeKind_Mul, Node, ParsePrefix(Stream));
        }
        else if (MatchAndNextToken(Stream, '/'))
        {
            Node = MakeBinaryNode(NodeKind_Div, Node, ParsePrefix(Stream));
        }
        else if (MatchAndNextToken(Stream, '%'))
        {
            Node = MakeBinaryNode(NodeKind_Mod, Node, ParsePrefix(Stream));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParsePrefix(token_stream* Stream)
{
    node* Node = 0;

    if (MatchAndNextToken(Stream, '~'))
    {
        Node = MakeUnaryNode(NodeKind_BitwiseNot, ParsePrefix(Stream));
    }
    else if (MatchAndNextToken(Stream, '!'))
    {
        Node = MakeUnaryNode(NodeKind_LogicalNot, ParsePrefix(Stream));
    }
    else if (MatchAndNextToken(Stream, '-'))
    {
        Node = MakeUnaryNode(NodeKind_Negate, ParsePrefix(Stream));
    }
    else if (MatchAndNextToken(Stream, '+'))
    {
        Node = ParsePrefix(Stream);
    }
    else
    {
        Node = ParsePrimary(Stream);
    }

    return (Node);
}

local node* ParsePrimary(token_stream* Stream)
{
    node* Node = 0;

    if (MatchToken(Stream, TokenKind_Integer))
    {
        Node = MakeIntegerNode(GetTokenInteger(Stream, GetCurrentToken(Stream)));

        NextToken(Stream);
    }
    else if (MatchAndNextToken(Stream, '('))
    {
        Node = ParseExpression(Stream);

        if (!MatchAndNextToken(Stream, ')'))
        {
            Println(Str("Missing matching ')' for '('"));
            Exit(1);
        }
    }
    else if (MatchToken(Stream, ';'))
    {
        // NOTE(vak): Ignore
    }
    else
    {
        Println(Str("Syntax error"));
        Exit(1);
    }

    return (Node);
}

