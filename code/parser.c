
#pragma once

typedef enum
{
    NodeKind_Nil = 0,

    NodeKind_Integer,
    NodeKind_Identifier,
    NodeKind_Variable,

    NodeKind_Declare,
    NodeKind_Assign,

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

    NodeKind_Block,
    NodeKind_If,
    NodeKind_Return,
} node_kind;

typedef struct node node;
struct node
{
    node_kind Kind;
    node* Next;

    union
    {
        struct { node* Left; node* Right; };                                    // NOTE(vak): Unary/Binary
        struct { usize Integer; };                                              // NOTE(vak): Integer
        struct { string Identifier; };                                          // NOTE(vak): Identifier
        struct { usize StackOffset; };                                          // NOTE(vak): Variable
        struct { node* TernaryCond; node* TernaryIf; node* TernaryElse; };      // NOTE(vak): Ternary conditional
        struct { node* IfCond; node* IfThen; node* IfElse; };                   // NOTE(vak): If/Else
        struct { node* First; node* Last; };                                    // NOTE(vak): Block
    };
};

typedef struct
{
    node* Root;
    usize StackSize;
} program;

typedef struct symbol symbol;
struct symbol
{
    string Identifier;
    usize StackOffset;
    symbol* Next;
};

typedef struct
{
    usize StackOffset;           // NOTE(vak): Current stack offset

    // NOTE(vak): Linked-list stack for variables
    symbol* First;              // NOTE(vak): First variable
    symbol* Last;               // NOTE(vak): Last variable
} symbol_tracker;

local symbol* MakeVariableSymbol(program* Program, symbol_tracker* Tracker, string Identifier)
{
    symbol* Result = Allocate(sizeof(symbol));

    Tracker->StackOffset += 8;
    Program->StackSize = Maximum(Program->StackSize, Tracker->StackOffset);

    Result->Identifier = Identifier;
    Result->StackOffset = Tracker->StackOffset;
    Result->Next = 0;

    if (!Tracker->First)
    {
        Tracker->First = Result;
        Tracker->Last = Result;
    }
    else
    {
        Tracker->Last->Next = Result;
        Tracker->Last = Result;
    }

    return (Result);
}

local symbol* LookupSymbol(symbol_tracker* Tracker, string Identifier)
{
    symbol* Result = 0;

    for (
        symbol* Scan = Tracker->First;
        Scan;
        Scan = Scan->Next
    )
    {
        if (StringIsEqual(Scan->Identifier, Identifier))
        {
            Result = Scan;
            break;
        }
    }

    return (Result);
}

local void AnalyzeSymbols(symbol_tracker* Tracker, program* Program, node* Node)
{
    if (!Node)
        return;

    switch (Node->Kind)
    {
        default: { Println(Str("Unknown node kind in AnalyzeSymbols()")); Exit(1); } break;

        case NodeKind_Integer:
        {
        } break;

        case NodeKind_Block:
        {
            symbol* SavedLast = Tracker->Last;
            usize SavedStack = Tracker->StackOffset;

            AnalyzeSymbols(Tracker, Program, Node->First);

            Tracker->StackOffset = SavedStack;

            if (SavedLast)
            {
                SavedLast->Next = 0;
                Tracker->Last = SavedLast;
            }
            else
            {
                Tracker->First = 0;
                Tracker->Last = 0;
            }
        } break;

        case NodeKind_Identifier:
        {
            symbol* Symbol = LookupSymbol(Tracker, Node->Identifier);

            if (!Symbol)
            {
                Print(Str("Use of undeclared identifier '"));
                Print(Node->Identifier);
                Print(Str("'\n"));
                Exit(1);
            }

            Node->Kind = NodeKind_Variable;
            Node->StackOffset = Symbol->StackOffset;
        } break;

        case NodeKind_Declare:
        {
            node* Target = Node->Left;

            symbol* Symbol = 0;

            if (Target && (Target->Kind == NodeKind_Identifier))
            {
                Symbol = LookupSymbol(Tracker, Target->Identifier);

                if (Symbol)
                {
                    Print(Str("Redeclaration of previously declared variable '"));
                    Print(Target->Identifier);
                    Print(Str("'\n"));
                    Exit(1);
                }
                else
                {
                    Symbol = MakeVariableSymbol(Program, Tracker, Target->Identifier);
                }
            }
            else
            {
                Println(Str("Invalid variable name"));
                Exit(1);
            }

            Node->Kind = NodeKind_Variable;
            Node->StackOffset = Symbol->StackOffset;
        } break;

        case NodeKind_Negate:
        case NodeKind_BitwiseNot:
        case NodeKind_LogicalNot:
        case NodeKind_Return:
        {
            AnalyzeSymbols(Tracker, Program, Node->Left);
        } break;

        case NodeKind_Assign:
        case NodeKind_Add:
        case NodeKind_Sub:
        case NodeKind_Mul:
        case NodeKind_Div:
        case NodeKind_Mod:
        case NodeKind_Equal:
        case NodeKind_NotEqual:
        case NodeKind_Less:
        case NodeKind_Greater:
        case NodeKind_LessEqual:
        case NodeKind_GreaterEqual:
        case NodeKind_ShiftLeft:
        case NodeKind_ShiftRight:
        case NodeKind_BitwiseAnd:
        case NodeKind_BitwiseOr:
        case NodeKind_BitwiseXor:
        case NodeKind_LogicalAnd:
        case NodeKind_LogicalOr:
        {
            AnalyzeSymbols(Tracker, Program, Node->Left);
            AnalyzeSymbols(Tracker, Program, Node->Right);
        } break;

        case NodeKind_Ternary:
        {
            AnalyzeSymbols(Tracker, Program, Node->TernaryCond);
            AnalyzeSymbols(Tracker, Program, Node->TernaryIf);
            AnalyzeSymbols(Tracker, Program, Node->TernaryElse);
        } break;

        case NodeKind_If:
        {
            AnalyzeSymbols(Tracker, Program, Node->IfCond);
            AnalyzeSymbols(Tracker, Program, Node->IfThen);
            AnalyzeSymbols(Tracker, Program, Node->IfElse);
        } break;
    }

    AnalyzeSymbols(Tracker, Program, Node->Next);
}

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

local node* MakeIdentifierNode(string Identifier)
{
    node* Node = MakeNode(NodeKind_Identifier);

    Node->Identifier = Identifier;

    return (Node);
}

local node* ParseBlock      (token_stream* Stream);
local node* ParseStatement  (token_stream* Stream);
local node* ParseExpression (token_stream* Stream);
local node* ParseAssign     (token_stream* Stream);
local node* ParseDeclare    (token_stream* Stream);
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

local void PostParsePass(program* Program)
{
    symbol_tracker SymbolTracker = {0};

    AnalyzeSymbols(&SymbolTracker, Program, Program->Root);
}

local program Parse(token_stream* Stream)
{
    node* First = ParseStatement(Stream);
    node* Last = First;

    while (!NoMoreTokens(Stream))
    {
        Last->Next = ParseStatement(Stream);

        if (Last->Next)
            Last = Last->Next;
    }

    program Program =
    {
        .Root = First,
    };

    PostParsePass(&Program);

    return (Program);
}

local node* ParseBlock(token_stream* Stream)
{
    node* Block = MakeNode(NodeKind_Block);

    if (!MatchAndNextToken(Stream, '{'))
    {
        Println(Str("Missing '{' to denote start of block"));
        Exit(1);
    }

    while (!NoMoreTokens(Stream))
    {
        if (MatchToken(Stream, '}'))
            break;

        if (!Block->First)
        {
            Block->First = ParseStatement(Stream);
            Block->Last = Block->First;
        }
        else
        {
            Block->Last->Next = ParseStatement(Stream);

            if (Block->Last->Next)
                Block->Last = Block->Last->Next;
        }
    }

    if (!MatchAndNextToken(Stream, '}'))
    {
        Println(Str("Missing '}' to match '{'"));
        Exit(1);
    }

    return (Block);
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
    else if (MatchStringAndNextToken(Stream, Str("if")))
    {
        Node = MakeNode(NodeKind_If);

        Node->IfCond = ParseExpression(Stream);
        Node->IfThen = ParseBlock(Stream);

        if (MatchStringAndNextToken(Stream, Str("else")))
        {
            Node->IfElse = ParseStatement(Stream);
        }
    }
    else if (MatchToken(Stream, '{'))
    {
        Node = ParseBlock(Stream);
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
    node* Node = ParseAssign(Stream);
    return (Node);
}

local node* ParseAssign(token_stream* Stream)
{
    node* Node = ParseDeclare(Stream);

    if (MatchAndNextToken(Stream, '='))
    {
        Node = MakeBinaryNode(NodeKind_Assign, Node, ParseAssign(Stream));
    }

    return (Node);
}

local node* ParseDeclare(token_stream* Stream)
{
    node* Node = ParseTernary(Stream);

    if (MatchAndNextToken(Stream, TokenKind_ColonEqual))
    {
        Node = MakeBinaryNode(NodeKind_Declare, Node, 0);
        Node = MakeBinaryNode(NodeKind_Assign, Node, ParseDeclare(Stream));
    }

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
    else if (MatchToken(Stream, TokenKind_Identifier))
    {
        Node = MakeIdentifierNode(GetTokenString(Stream, GetCurrentToken(Stream)));

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
    else if (MatchToken(Stream, ';') || MatchToken(Stream, '{'))
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

