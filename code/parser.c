
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
} node_kind;

typedef struct node node;
struct node
{
    node_kind Kind;
    node* Left;
    node* Right;
    usize Integer;
};

local node* ParseExpression (token_stream* Stream);
local node* ParseEquality   (token_stream* Stream);
local node* ParseComparison (token_stream* Stream);
local node* ParseSum        (token_stream* Stream);
local node* ParseFactor     (token_stream* Stream);
local node* ParsePrimary    (token_stream* Stream);

local node* Parse(token_stream* Stream)
{
    node* Result = ParseExpression(Stream);
    return (Result);
}

local node* MakeNode(node_kind Kind)
{
    node* Node = Allocate(sizeof(node));

    ZeroMemory(Node, sizeof(node));
    Node->Kind = Kind;

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

local node* ParseExpression(token_stream* Stream)
{
    node* Node = ParseEquality(Stream);
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
    node* Node = ParseSum(Stream);

    for (;;)
    {
            if (MatchAndNextToken(Stream, '<'))
            {
                Node = MakeBinaryNode(NodeKind_Less, Node, ParseSum(Stream));
            }
            else if (MatchAndNextToken(Stream, '>'))
            {
                Node = MakeBinaryNode(NodeKind_Greater, Node, ParseSum(Stream));
            }
            else if (MatchAndNextToken(Stream, TokenKind_LessEqual))
            {
                Node = MakeBinaryNode(NodeKind_LessEqual, Node, ParseSum(Stream));
            }
            else if (MatchAndNextToken(Stream, TokenKind_GreaterEqual))
            {
                Node = MakeBinaryNode(NodeKind_GreaterEqual, Node, ParseSum(Stream));
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
    node* Node = ParsePrimary(Stream);

    for (;;)
    {
        if (MatchAndNextToken(Stream, '*'))
        {
            Node = MakeBinaryNode(NodeKind_Mul, Node, ParsePrimary(Stream));
        }
        else if (MatchAndNextToken(Stream, '/'))
        {
            Node = MakeBinaryNode(NodeKind_Div, Node, ParsePrimary(Stream));
        }
        else if (MatchAndNextToken(Stream, '%'))
        {
            Node = MakeBinaryNode(NodeKind_Mod, Node, ParsePrimary(Stream));
        }
        else
        {
            break;
        }
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
    else
    {
        Println(Str("Syntax error"));
        Exit(1);
    }

    return (Node);
}

