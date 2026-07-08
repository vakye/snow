
#include "shared.c"

local usize WriteStdOut(void* Bytes, usize Size)
{
    ssize Result = 0;

    __asm__ volatile (
        "syscall" :
        "=a"(Result) :
        "a"(1), // NOTE(vak): write syscall
        "D"(1), // NOTE(vak): STDOUT_FILENO
        "S"(Bytes),
        "d"(Size) :
        "memory", "rcx", "r11"
    );

    if (Result < 0) Result = 0;

    return (Result);
}

local usize PrintCharacter(char Character)
{
    return WriteStdOut(&Character, 1);
}

local usize Print(string Message)
{
    return WriteStdOut(Message.Data, Message.Size);
}

local usize Println(string Message)
{
    usize Result = 0;

    Result += Print(Message);
    Result += PrintCharacter('\n');

    return (Result);
}

local usize PrintUSize(usize Value)
{
    char Buffer[32] = {0};
    usize DigitCount = 0;
    usize DigitIndex = sizeof(Buffer);

    do
    {
        char Digit = (Value % 10) + '0';
        Value /= 10;

        DigitCount++;
        DigitIndex--;

        Buffer[DigitIndex] = Digit;
    } while (Value);

    usize Result = Print(StrData(Buffer + DigitIndex, DigitCount));

    return (Result);
}

local usize PrintSSize(ssize Value)
{
    usize Result = 0;

    if (Value < 0)
    {
        Result += PrintCharacter('-');
        Value = -Value;
    }

    Result += PrintUSize(Value);

    return (Result);
}

local void Exit(unsigned char ExitCode)
{
    __asm__ volatile (
        "syscall" ::
        "a"(60), // NOTE(vak): exit syscall
        "D"(ExitCode) :
        "memory", "rcx", "r11"
    );
}

local void* MapExecutableMemory(void* Code, usize Size)
{
    register s32 Flags __asm__("r10") = 0x22; // NOTE(vak): MAP_PRIVATE | MAP_ANONYMOUS
    register s32 FileDescriptor __asm__("r8") = -1;
    register s32 Offset __asm__("r9") = 0;

    ssize ExecutableBase = 0;

    __asm__ volatile (
        "syscall" :
        "=a"(ExecutableBase) :
        "a"(9), // NOTE(vak): mmap syscall
        "D"(0),
        "S"(Size),
        "d"(0x7), // NOTE(vak): PROT_READ|PROT_WRITE|PROT_EXEC
        "r"(Flags),
        "r"(FileDescriptor),
        "r"(Offset) :
        "memory", "rcx", "r11"
    );

    if (ExecutableBase < 0)
    {
        Println(Str("Failed to map executable mmeory"));
        Exit(1);
    }

    if (Code)
    {
        CopyMemory((void*)(ExecutableBase), Code, Size);
    }

    return (void*)(ExecutableBase);
}

local void UnmapExecutableMemory(void* Memory, usize Size)
{
    s32 UnmapResult = 0;

    __asm__ volatile (
        "syscall" :
        "=a"(UnmapResult) :
        "a"(11), // NOTE(vak): munmap syscall
        "D"(Memory),
        "S"(Size) :
        "memory", "rcx", "r11"
    );

    if (UnmapResult < 0)
    {
        Println(Str("Failed to unmap executable memory"));
        Exit(1);
    }
}

local void* LinuxMemoryBase = 0;
local usize LinuxMemoryUsed = 0;
local usize LinuxMemoryCommited = 0;
local usize LinuxMemoryReserved = 0;

local void LinuxInitMemory(void)
{
    LinuxMemoryReserved = GB(64);
    LinuxMemoryCommited = KB(64);

    ssize MemoryBase = 0;

    register s32 Flags __asm__("r10") = 0x22; // NOTE(vak): MAP_PRIVATE | MAP_ANONYMOUS
    register s32 FileDescriptor __asm__("r8") = -1;
    register s32 Offset __asm__("r9") = 0;

    __asm__ volatile (
        "syscall" :
        "=a"(MemoryBase) :
        "a"(9), // NOTE(vak): mmap syscall
        "D"(0),
        "S"(LinuxMemoryReserved),
        "d"(0), // NOTE(vak): PROT_NONE
        "r"(Flags),
        "r"(FileDescriptor),
        "r"(Offset) :
        "memory", "rcx", "r11"
    );

    if (MemoryBase <= 0)
    {
        Println(Str("Failed to initialize memory"));
        Exit(1);
    }

    LinuxMemoryBase = (void*)MemoryBase;

    ssize ProtectResult = 0;

    __asm__ volatile (
        "syscall" :
        "=a"(ProtectResult) :
        "a"(10), // NOTE(vak): mprotect syscall
        "D"(LinuxMemoryBase),
        "S"(LinuxMemoryCommited),
        "d"(0x03) : // NOTE(vak): PROT_READ | PROT_WRITE
        "memory", "rcx", "r11"
    );

    if (ProtectResult < 0)
    {
        Println(Str("Failed to commit memory"));
        Exit(1);
    }
}

local void LinuxMoreMemory(usize Size)
{
    usize ExpandSize = (LinuxMemoryUsed + Size) - LinuxMemoryCommited;

    usize CommitSize = Align(ExpandSize, KB(64));
    void* CommitDest = (u8*)LinuxMemoryBase + LinuxMemoryCommited;

    if (LinuxMemoryCommited + CommitSize > LinuxMemoryReserved)
    {
        Println(Str("Ran out of memory"));
        Exit(1);
    }

    ssize ProtectResult = 0;

    __asm__ volatile (
        "syscall" :
        "=a"(ProtectResult) :
        "a"(10), // NOTE(vak): mprotect syscall
        "D"(CommitDest),
        "S"(CommitSize),
        "d"(0x03) : // NOTE(vak): PROT_READ | PROT_WRITE
        "memory", "rcx", "r11"
    );

    if (ProtectResult < 0)
    {
        Println(Str("Failed to commit memory"));
        Exit(1);
    }
}

local void ResetMemory(void)
{
    LinuxMemoryUsed = 0;
}

local void* Allocate(usize Size)
{
    if (!LinuxMemoryBase)
        LinuxInitMemory();

    if (LinuxMemoryUsed + Size > LinuxMemoryCommited)
        LinuxMoreMemory(Size);

    void* Result = (u8*)LinuxMemoryBase + LinuxMemoryUsed;
    LinuxMemoryUsed += Size;

    return (Result);
}

#include "lexer.c"
#include "parser.c"
#include "generator.c"
#include "disassembler.c"
#include "evaluator.c"

local void PrintNode(node* Node)
{
    persist usize Level = 0;

    if (!Node) return;

    for (usize Index = 0; Index < Level; Index++)
        Print(Str("  "));

    switch (Node->Kind)
    {
        default:
        {
            Println(Str("Unknown node"));
            Exit(1);
        } break;

        case NodeKind_Integer:
        {
            Print(Str("Integer: "));
            PrintUSize(Node->Integer);
            PrintCharacter('\n');
        } break;

        case NodeKind_Negate:
        case NodeKind_BitwiseNot:
        case NodeKind_LogicalNot:
        {
            switch (Node->Kind)
            {
                default: {} break;

                case NodeKind_Negate:       Println(Str("Negate:")); break;
                case NodeKind_BitwiseNot:   Println(Str("BitwiseNot:")); break;
                case NodeKind_LogicalNot:   Println(Str("LogicalNot:")); break;
            }

            Level++;

            PrintNode(Node->Left);

            Level--;
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
            switch (Node->Kind)
            {
                default: {} break;

                case NodeKind_Add:          Println(Str("Add:")); break;
                case NodeKind_Sub:          Println(Str("Sub:")); break;
                case NodeKind_Mul:          Println(Str("Mul:")); break;
                case NodeKind_Div:          Println(Str("Div:")); break;
                case NodeKind_Mod:          Println(Str("Mod:")); break;
                case NodeKind_Equal:        Println(Str("Equal:")); break;
                case NodeKind_NotEqual:     Println(Str("NotEqual:")); break;
                case NodeKind_Less:         Println(Str("Less:")); break;
                case NodeKind_Greater:      Println(Str("Greater:")); break;
                case NodeKind_LessEqual:    Println(Str("LessEqual:")); break;
                case NodeKind_GreaterEqual: Println(Str("GreaterEqual:")); break;
                case NodeKind_ShiftLeft:    Println(Str("ShiftLeft:")); break;
                case NodeKind_ShiftRight:   Println(Str("ShiftRight:")); break;
                case NodeKind_BitwiseAnd:   Println(Str("BitwiseAnd:")); break;
                case NodeKind_BitwiseOr:    Println(Str("BitwiseOr:")); break;
                case NodeKind_BitwiseXor:   Println(Str("BitwiseXor:")); break;
            }

            Level++;

            PrintNode(Node->Left);
            PrintNode(Node->Right);

            Level--;
        } break;
    }
}

typedef ssize program_entry(void);

local ssize CompileAndRunDetailed(string Code)
{
    ResetMemory();

    token_stream TokenStream    = MakeTokenStream(Code);
    node* RootNode              = Parse(&TokenStream);
    generated Generated         = Generate(RootNode);

    Print(Str("\n"));

    Println(Str("============================================================================================"));
    Println(Str("AST:"));
    Println(Str("============================================================================================"));
    PrintNode(RootNode);

    Print(Str("\n\n"));

    Println(Str("============================================================================================"));
    Println(Str("Disassembly:"));
    Println(Str("============================================================================================"));
    Disassemble(Generated.Code, Generated.Size);

    Print(Str("\n\n"));

    program_entry* ProgramEntry = (program_entry*)
        MapExecutableMemory(Generated.Code, Generated.Size);

    ssize ProgramResult = ProgramEntry();

    Println(Str("============================================================================================"));
    Print(Str("Execution result (RAX): "));
    PrintSSize(ProgramResult);
    PrintCharacter('\n');
    Println(Str("============================================================================================"));

    Print(Str("\n"));

    UnmapExecutableMemory((void*)ProgramEntry, Generated.Size);
    ResetMemory();

    return (ProgramResult);
}

local ssize CompileAndRun(string Code)
{
    ResetMemory();

    token_stream TokenStream    = MakeTokenStream(Code);
    node* RootNode              = Parse(&TokenStream);
    generated Generated         = Generate(RootNode);

    program_entry* ProgramEntry = (program_entry*)
        MapExecutableMemory(Generated.Code, Generated.Size);

    ssize ProgramResult = ProgramEntry();

    UnmapExecutableMemory((void*)ProgramEntry, Generated.Size);

    ResetMemory();

    return (ProgramResult);
}

local ssize ParseAndEvaluateDetailed(string Code)
{
    ResetMemory();

    token_stream TokenStream    = MakeTokenStream(Code);
    node* RootNode              = Parse(&TokenStream);

    ssize Result = Evaluate(RootNode);

    Print(Str("\n"));

    Println(Str("============================================================================================"));
    Println(Str("AST:"));
    Println(Str("============================================================================================"));
    PrintNode(RootNode);

    Print(Str("\n\n"));

    Println(Str("============================================================================================"));
    Print(Str("Evaluation Result: "));
    PrintUSize(Result);
    PrintCharacter('\n');
    Println(Str("============================================================================================"));

    Print(Str("\n"));

    ResetMemory();

    return (Result);
}

local ssize ParseAndEvaluate(string Code)
{
    ResetMemory();

    token_stream TokenStream    = MakeTokenStream(Code);
    node* RootNode              = Parse(&TokenStream);

    ssize Result = Evaluate(RootNode);

    ResetMemory();

    return (Result);
}

typedef struct
{
    ssize Expected;
    string Code;
} test_case;

__attribute__((force_align_arg_pointer))
void LinuxEntry(void)
{
    persist test_case TestCases[] =
    {
        { 2,            StaticStr("1 + 1") },
        { 36,           StaticStr("1 + 5 * 7") },
        { 33,           StaticStr("8*4 + 2*3 / 5") },
        { 1800,         StaticStr("120 / 2*(10 + 20)") },
        { 2,            StaticStr("120 / 2*(10 + 20 - 5) % 7") },
        { 1,            StaticStr("1 / 1 == 1 + 1 % 1 - 1 + 1") },
        { 1,            StaticStr("3 != 10 + 10") },
        { 1,            StaticStr("100 - 2*2*2*2*2*2 + 2*16 - 2*24 == 10 + 10*(2 >= 10 - 9)") },
        { 0,            StaticStr("1 + 1 >= 1 * 3") },
        { 1,            StaticStr("1 + 1 == 2 * 1200 - 2398") },
        { 0,            StaticStr("1 + 2 + 3 + 4 + 5 + 6 >= 50 * 10 - 500 + 50 + 5") },
        { 1,            StaticStr("120 / 2 >= 50 == 10 * 20 - 199") },
        { 10,           StaticStr("(1 == 3) + 9 + (120/2*(10+20)<=1800)") },
        { -370,         StaticStr("180 / 3*(9 - 4*(6/8) + 1 + 1 + 1 + 1) + 100 * 3 / (13 % 7) - 1200") },
        { 287,          StaticStr("1 + 4 + 7 + 10 + 13 + 16 + 19 + 22 + 25 + 28 + 31 + 34 + 37 + 40") },
        { 38107,        StaticStr(" (((57 - 98) * 1) - 63 + 80 * 34 * 14 + 67) - 17 * (39 / 81 - 24 + 91) + 71 + 27 - 70 - (28 * 15 - (15 + 49)) / 75 * 54 + 23 + 16 * (42 + 7 * 37) + (77 / 94) + 65 * 54 - 11 / 7 / (25 / (1 + 40 / 61 / 52 * 41 - 8 + 16 + ((33 / 33 + 79 * (55 / 63)))) - (((55 * 13) + (98 - 32 + (65))) + ((70 + ((60 * 99) + 4) - 75) - 75 + 17 + (70 * 88))) / (33 + 44) * 42) - (71 * 98 + 66 * (66 / 83 / 68)) + ((94 - 59 + 91 - 91 / (57) + 39 * 80) * 95 / 86 * ((27 * 94 + 75) / 84 / 91 / 43))") },
        { -4855,        StaticStr("(((((42 - 87 + (99 / 94))) - (((21 * 15) + 92 - 87) - (93 * (23 - 71))))) - ((((9 - 16 - 19) + 63)) * (((((76 / 80) / 37 / 22) * 66)) / (85 + 82))) + ((((98 * 39 / 26) - 31) / (((20 / 42) * 41) - ((44 * (13 + 25 - 5)) / (37 * (86 / 4 + 15)) - 68) * (((58 * 23) + (35 / 34)) * (((((38)) + 55)) - 25 + 21 - 44) + 32 * (((21 - 86) - (29 / 91)) + (71 / 99))))) - 27))") },
        { -1806,        StaticStr("(((((53))) - 26 - 11 * 31 - (80 - 11 + 73)) + ((15 * 50) - 50 * (42)))") },
        { 2715,         StaticStr("(((((34 * 80) - (6)) - 13 - 3) + 17) + ((94 / 58 - 45 * (37 / 83) + 70 / 65 / 24) / ((((41) - (5 * 99) * 72) + ((23 + 70) * ((78 - 33))))) - ((3) / 23)))") },
        { 284403643073, StaticStr("(((((86 / 58) * ((61 / 11) + (40 * (44 * 4) * ((19 * 13 + 76) - (17 + 93))))) / 1 / 59) * ((43 / 61) - 85) / ((11 - 42) - 48) + ((8 * (((75 + (62 + 59)) - (19 / ((47 * 98) - 87))) - 23)) * ((97 / (70 - (79 * 6)) + ((30 + 37 * (42 * (46 * ((84 / 84 + 9) + (83 * 81))))) / 89 * 38)) - (7 + 5) * (((93 - 43) * (84)) / 12 / 52 * 79)))) + (80 / 64 / (97 - (28 + 80))))") },
        { 32,           StaticStr("1 << 5") },
        { 1024,         StaticStr("1 << 10") },
        { 1048576,      StaticStr("1 << (100 - 2*2*2*2*2*2 + 2*16 - 2*24)") },
        { 28,           StaticStr("28711 >> 10") },
        { 184032538241, StaticStr("(((((86  >> 2) * ((61 >> 2) + (40 * (44 << 2) * ((19 * 13 >= 76) - (17 + 93))))) / 1 / 59) * ((43 / 61) < 85) / ((11 - 42) - 48) + ((8 * (((75 + (62 + 59 >> 1)) - (19 / ((47 * 98) - 87))) - 23)) * ((97 / (70 - (79 * 6)) + ((30 + 37 * (42 * (46 * ((84 / 84 + 9) + (83 * 81))))) / 89 * 38)) - (7 << 5) * (((93 - 43) * (84)) / 12 / 52 * 79)))) + (80 / 64 / (97 - (28 + 80))))") },
        { 3,            StaticStr("4294967295 & 3") },
        { 10,           StaticStr("2871882 >> 3 & 10 | 3 - 2^1 & 1 + (4 % 3)^1 + 9^1 & 7") },
        { 425471,       StaticStr("43690 ^ 448341") },
        { -1,           StaticStr("-1") },
        { -137,         StaticStr("-+-+++172 + +-+---++-+309") },
        { 34856,        StaticStr("~23820247 & 65535") },
        { 485732352,    StaticStr("(485729837 + 4096 - 1) & ~4095") },
        { 0,            StaticStr("!!!-~!--~0") },
    };

    usize CasesPassed = 0;

    PrintCharacter('\n');

    for (usize Index = 0; Index < ArrayCount(TestCases); Index++)
    {
        test_case* Case = TestCases + Index;

        ssize RunResult = CompileAndRun(Case->Code);

        {
            usize SoFar = 0;

            SoFar += Print(Str("["));
            SoFar += PrintUSize(Index);
            SoFar += Print(Str("]:"));

            usize Pad = 8;
            for (; SoFar < Pad; SoFar++)
                PrintCharacter(' ');
        }

        if (RunResult != Case->Expected)
        {
            Print(Str("\033[31mFAILED\033[39m"));
            Print(Str(": '"));
            Print(Case->Code);
            Print(Str("'\n"));
        }
        else
        {
            Print(Str("\033[32mPASSED\033[39m"));
            Print(Str(": '"));
            Print(Case->Code);
            Print(Str("'\n"));
            CasesPassed++;
        }
    }

    Print(Str("Cases passed: "));
    PrintUSize(CasesPassed);
    Print(Str("/"));
    PrintUSize(ArrayCount(TestCases));
    PrintCharacter('\n');

    PrintCharacter('\n');

    Exit(0);
}

void* memset(void* DestInit, s32 Byte, usize Size)
{
    u8* Dest = (u8*)DestInit;

    while (Size--)
        *Dest++ = Byte;

    return (DestInit);
}

void* memcpy(void* DestInit, void* SourceInit, usize Size)
{
    u8* Dest = (u8*)DestInit;
    u8* Source = (u8*)SourceInit;

    while (Size--)
        *Dest++ = *Source++;

    return (DestInit);
}

