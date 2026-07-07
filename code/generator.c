
#pragma once

typedef struct
{
    void* Base;
    usize Size;
    usize At;
} generator;

typedef struct
{
    u8* Code;
    usize Size;
} generated;

local void GenBytes(generator* Gen, void* Data, usize Size)
{
    if (Gen->At + Size > Gen->Size)
    {
        Println(Str("Code generator buffer overflow"));
        Exit(1);
    }

    CopyMemory((u8*)Gen->Base + Gen->At, Data, Size);
    Gen->At += Size;
}

local void GenU8 (generator* Gen, u8  Value) { GenBytes(Gen, &Value, 1); }
local void GenU16(generator* Gen, u16 Value) { GenBytes(Gen, &Value, 2); }
local void GenU24(generator* Gen, u32 Value) { GenBytes(Gen, &Value, 3); }
local void GenU32(generator* Gen, u32 Value) { GenBytes(Gen, &Value, 4); }
local void GenU40(generator* Gen, u64 Value) { GenBytes(Gen, &Value, 5); }
local void GenU48(generator* Gen, u64 Value) { GenBytes(Gen, &Value, 6); }
local void GenU56(generator* Gen, u64 Value) { GenBytes(Gen, &Value, 7); }
local void GenU64(generator* Gen, u64 Value) { GenBytes(Gen, &Value, 8); }

local void GenerateNode(generator* Gen, node* Node);

local generated Generate(node* RootNode)
{
    generator Gen = {0};
    Gen.Size = KB(64);
    Gen.Base = Allocate(Gen.Size);

    generated Result =
    {
        .Code = Gen.Base,
        .Size = 0,
    };

    {
        // NOTE(vak):
        // 55           push rbp
        // 48 8b ec     mov rbp, rsp
        GenU32(&Gen, 0xec8b4855);

        GenerateNode(&Gen, RootNode);

        // NOTE(vak):
        // 48 8b e5     mov rsp, rbp
        // 5d           pop rbp
        // c3           ret
        GenU40(&Gen, 0xc35de58b48);
    }

    Result.Size = Gen.At;

    return (Result);
}

local void GenerateNode(generator* Gen, node* Node)
{
    if (!Node) return;

    if (Node->Kind == NodeKind_Integer)
    {
        // NOTE(vak):
        // 48 b8 (u64) mov rax, (u64)

        GenU16(Gen, 0xb848);
        GenU64(Gen, Node->Integer);
    }
    else
    {
        GenerateNode(Gen, Node->Right);
        GenU8(Gen, 0x50); // NOTE(vak): push rax

        GenerateNode(Gen, Node->Left);
        GenU8(Gen, 0x59); // NOTE(vak): pop rcx

        // NOTE(vak):
        //     rax = Node->Left
        //     rcx = Node->Right

        switch (Node->Kind)
        {
            default:
            {
                Println(Str("Unimplemented node kind"));
                Exit(1);
            } break;

            case NodeKind_Add:
            {
                // NOTE(vak):
                // 48 03 c1 add rax, rcx
                GenU24(Gen, 0xc10348);
            } break;

            case NodeKind_Sub:
            {
                // NOTE(Vak):
                // 48 2b c1 sub rax, rcx
                GenU24(Gen, 0xc12b48);
            } break;

            case NodeKind_Mul:
            {
                // NOTE(vak):
                // 48 0f af  c1 imul rax, rcx
                GenU32(Gen, 0xc1af0f48);
            } break;

            case NodeKind_Div:
            {
                // NOTE(vak):
                // 48 99        cqo
                // 48 f7 f9     idiv rcx

                GenU40(Gen, 0xf9f7489948);
            } break;

            case NodeKind_Mod:
            {
                // NOTE(vak):
                // 48 99        cqo
                // 48 f7 f9     idiv rcx
                // 48 8b c2     mov rax, rcx
                GenU64(Gen, 0xc28b48f9f7489948);
            } break;
        }
    }
}

