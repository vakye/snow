
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

        case NodeKind_Add:
        case NodeKind_Sub:
        case NodeKind_Mul:
        case NodeKind_Div:
        case NodeKind_Mod:
        {
            switch (Node->Kind)
            {
                default: {} break;

                case NodeKind_Add: Println(Str("Add:")); break;
                case NodeKind_Sub: Println(Str("Sub:")); break;
                case NodeKind_Mul: Println(Str("Mul:")); break;
                case NodeKind_Div: Println(Str("Div:")); break;
                case NodeKind_Mod: Println(Str("Mod:")); break;
            }

            Level++;

            PrintNode(Node->Left);
            PrintNode(Node->Right);

            Level--;
        } break;
    }
}

typedef ssize program_entry(void);

__attribute__((force_align_arg_pointer))
void LinuxEntry(void)
{
    string Code = Str("10 - 5");

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

