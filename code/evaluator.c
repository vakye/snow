
#pragma once

local ssize EvaluateNode(node* Node)
{
    if (!Node) return 0;

    ssize Result = 0;

    switch (Node->Kind)
    {
        default: { Println(Str("Unimplemented node kind for evaluator")); Exit(1); } break;

        case NodeKind_Integer: Result = Node->Integer; break;

        case NodeKind_Negate:
        case NodeKind_BitwiseNot:
        case NodeKind_LogicalNot:
        {
            ssize Left = EvaluateNode(Node->Left);

            switch (Node->Kind)
            {
                default: { Println(Str("Unimplemented unary node kind for evaluator")); Exit(1); } break;

                case NodeKind_Negate:               Result = -Left; break;
                case NodeKind_BitwiseNot:           Result = ~Left; break;
                case NodeKind_LogicalNot:           Result = !Left; break;
            }
        } break;

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
        {
            ssize Left = EvaluateNode(Node->Left);
            ssize Right = EvaluateNode(Node->Right);

            switch (Node->Kind)
            {
                default: { Println(Str("Unimplemented binary node kind for evaluator")); Exit(1); } break;

                case NodeKind_Add:                  Result = Left + Right; break;
                case NodeKind_Sub:                  Result = Left - Right; break;
                case NodeKind_Mul:                  Result = Left * Right; break;
                case NodeKind_Div:                  Result = Left / Right; break;
                case NodeKind_Mod:                  Result = Left % Right; break;
                case NodeKind_Equal:                Result = Left == Right; break;
                case NodeKind_NotEqual:             Result = Left != Right; break;
                case NodeKind_Less:                 Result = Left < Right; break;
                case NodeKind_Greater:              Result = Left > Right; break;
                case NodeKind_LessEqual:            Result = Left <= Right; break;
                case NodeKind_GreaterEqual:         Result = Left >= Right; break;
                case NodeKind_ShiftLeft:            Result = Left << Right; break;
                case NodeKind_ShiftRight:           Result = Left >> Right; break;
                case NodeKind_BitwiseAnd:           Result = Left & Right; break;
                case NodeKind_BitwiseOr:            Result = Left | Right; break;
                case NodeKind_BitwiseXor:           Result = Left ^ Right; break;
            }
        }
    }

    return (Result);
}

local ssize Evaluate(node* RootNode)
{
    ssize Result = EvaluateNode(RootNode);
    return (Result);
}

