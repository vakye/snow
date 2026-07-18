
#pragma once

typedef struct
{
    u8 ModRM;
    u8 SIB;
    ssize Disp;
    usize Immediate;
} parsed_operands;

local usize OutputInstructionHex(void* Data, usize From, usize To)
{
    usize Result = 0;

    char Buffer[2] = {0};
    char DigitMap[] = "0123456789abcdef";

    Result += PrintCharacter(' ');

    for (usize Index = From; Index < To; Index++)
    {
        u8 Byte = *((u8*)Data + Index);

        Buffer[0] = DigitMap[(Byte >> 4) & 0xF];
        Buffer[1] = DigitMap[(Byte >> 0) & 0xF];

        Result += Print(StrData(Buffer, sizeof(Buffer)));
        Result += Print(Str(" "));
    }

    usize HexPad = 40;
    while (Result < HexPad)
        Result += PrintCharacter(' ');

    return (Result);
}

local usize PrintInstructionOffset(usize Index)
{
    usize Result = 0;

    char Buffer[16] = {0};
    char DigitMap[] = "0123456789abcdef";

    Result += PrintCharacter(' ');

    for (usize Nibble = 0; Nibble < 16; Nibble++)
    {
        Buffer[15 - Nibble] = DigitMap[(Index >> (4*Nibble)) & 0x0F];
    }

    Result += Print(Str("0x"));
    Result += Print(StrData(Buffer, sizeof(Buffer)));

    usize Pad = 28;
    while (Result < Pad)
        Result += PrintCharacter(' ');

    return (Result);
}

local void PadInstruction(usize NameSize)
{
    usize InstructionPad = 12;

    for (usize Index = NameSize; NameSize < InstructionPad; NameSize++)
        PrintCharacter(' ');
}

local parsed_operands ParseModRM(u8* Bytes, usize* CurrentIndex, usize Size)
{
    parsed_operands Operands = {0};

    usize Index = *CurrentIndex;

    if (Index == Size)
    {
        Println(Str("Expected ModRM"));
        Exit(1);
    }

    Operands.ModRM = Bytes[Index++];

    u8 RM = (Operands.ModRM & 0x07);
    u8 Mode = (Operands.ModRM >> 6);

    #define ExpectSIB \
        if (Index == Size) { Println(Str("Expected SIB")); Exit(1); } \
        Operands.SIB = Bytes[Index++]

    #define ExpectDISP8 \
        if (Index + 1 > Size) { Println(Str("Expected DISP32")); Exit(1); } \
        Operands.Disp = (ssize)(*(s8*)(Bytes + Index)); Index += 1;

    #define ExpectDISP32 \
        if (Index + 4 > Size) { Println(Str("Expected DISP32")); Exit(1); } \
        Operands.Disp = (ssize)(*(s32*)(Bytes + Index)); Index += 4

    switch (Mode)
    {
        case 0:
        {
            if (RM == 0x4)
            {
                ExpectSIB;
            }
            else if (RM == 0x5)
            {
                ExpectDISP32;
            }
        } break;

        case 1:
        {
            if (RM == 0x4)
            {
                ExpectSIB;
            }

            ExpectDISP8;
        } break;

        case 2:
        {
            if (RM == 0x4)
            {
                ExpectSIB;
            }

            ExpectDISP32;
        } break;
    }

    #undef ExpectDISP32
    #undef ExpectDISP8
    #undef ExpectSIB

    *CurrentIndex = Index;

    return (Operands);
}

local string GetRegisterName(u8 REX, u8 Register, u8 Size)
{
    persist string RegisterNames8_NO_REX[] =
    {
        StaticStr("al"),
        StaticStr("cl"),
        StaticStr("dl"),
        StaticStr("bl"),
        StaticStr("ah"),
        StaticStr("ch"),
        StaticStr("dh"),
        StaticStr("bh"),
        StaticStr("r8l"),
        StaticStr("r9l"),
        StaticStr("r10l"),
        StaticStr("r11l"),
        StaticStr("r12l"),
        StaticStr("r13l"),
        StaticStr("r14l"),
        StaticStr("r15l"),
    };

    persist string RegisterNames8_REX[] =
    {
        StaticStr("al"),
        StaticStr("cl"),
        StaticStr("dl"),
        StaticStr("bl"),
        StaticStr("spl"),
        StaticStr("bpl"),
        StaticStr("sil"),
        StaticStr("dil"),
        StaticStr("r8l"),
        StaticStr("r9l"),
        StaticStr("r10l"),
        StaticStr("r11l"),
        StaticStr("r12l"),
        StaticStr("r13l"),
        StaticStr("r14l"),
        StaticStr("r15l"),
    };

    persist string RegisterNames16[] =
    {
        StaticStr("ax"),
        StaticStr("cx"),
        StaticStr("dx"),
        StaticStr("bx"),
        StaticStr("sp"),
        StaticStr("bp"),
        StaticStr("si"),
        StaticStr("di"),
        StaticStr("r8w"),
        StaticStr("r9w"),
        StaticStr("r10w"),
        StaticStr("r11w"),
        StaticStr("r12w"),
        StaticStr("r13w"),
        StaticStr("r14w"),
        StaticStr("r15w"),
    };

    persist string RegisterNames32[] =
    {
        StaticStr("eax"),
        StaticStr("ecx"),
        StaticStr("edx"),
        StaticStr("ebx"),
        StaticStr("esp"),
        StaticStr("ebp"),
        StaticStr("esi"),
        StaticStr("edi"),
        StaticStr("r8d"),
        StaticStr("r9d"),
        StaticStr("r10d"),
        StaticStr("r11d"),
        StaticStr("r12d"),
        StaticStr("r13d"),
        StaticStr("r14d"),
        StaticStr("r15d"),
    };

    persist string RegisterNames64[] =
    {
        StaticStr("rax"),
        StaticStr("rcx"),
        StaticStr("rdx"),
        StaticStr("rbx"),
        StaticStr("rsp"),
        StaticStr("rbp"),
        StaticStr("rsi"),
        StaticStr("rdi"),
        StaticStr("r8"),
        StaticStr("r9"),
        StaticStr("r10"),
        StaticStr("r11"),
        StaticStr("r12"),
        StaticStr("r13"),
        StaticStr("r14"),
        StaticStr("r15"),
    };

    string RegisterName = {0};

    if (Register >= 16)
    {
        Println(Str("Invalid register index"));
        Exit(1);
    }

    switch (Size)
    {
        default: { Println(Str("Invalid register size")); Exit(1); } break;

        case 8:  RegisterName = (REX) ? (RegisterNames8_REX[Register]) : (RegisterNames8_NO_REX[Register]);
        case 16: RegisterName = RegisterNames16[Register];
        case 32: RegisterName = RegisterNames32[Register];
        case 64: RegisterName = RegisterNames64[Register];
    }

    return (RegisterName);
}

local void PrintNormalReg(u8 REX, u8 Reg, u8 Size)
{
    Print(GetRegisterName(REX, Reg, Size));
}

local void PrintDerefReg(u8 REX, u8 Reg, u8 Size)
{
    PrintCharacter('[');
    PrintNormalReg(REX, Reg, Size);
    PrintCharacter(']');
}

local void PrintSIB(u8 REX, u8 SIB, u8 Size)
{
    u8 REX_B = (REX)      & 0x1;
    u8 REX_X = (REX >> 1) & 0x1;

    u8 Base  = (SIB)      & 0x7 + 8*REX_B;
    u8 Index = (SIB >> 3) & 0x7 + 8*REX_X;
    u8 Scale = (SIB >> 6) & 0x3;

    u8 Multiply = (1 << Scale);

    PrintCharacter('[');
    PrintNormalReg(REX, Base, Size);
    Print(Str(" + "));

    if (Multiply > 1)
    {
        PrintUSize(Multiply);
        PrintCharacter('*');
    }

    PrintNormalReg(REX, Index, Size);
    PrintCharacter(']');
}

local void PrintRIPDisp(ssize Disp, u8 Size)
{
    Print(Str("[rip"));

    if (Disp == 0)
    {
        Print(Str("]"));
    }
    else if (Disp < 0)
    {
        Print(Str(" - "));
        PrintUSize(-Disp);
        Print(Str("]"));
    }
    else
    {
        Print(Str(" + "));
        PrintUSize(Disp);
        Print(Str("]"));
    }
}

local void PrintSIBDisp(u8 REX, u8 SIB, ssize Disp, u8 Size)
{
    u8 REX_B = (REX)      & 0x1;
    u8 REX_X = (REX >> 1) & 0x1;

    u8 Base  = (SIB)      & 0x7 + 8*REX_B;
    u8 Index = (SIB >> 3) & 0x7 + 8*REX_X;
    u8 Scale = (SIB >> 6) & 0x3;

    u8 Multiply = (1 << Scale);

    PrintCharacter('[');
    PrintNormalReg(REX, Base, Size);
    Print(Str(" + "));

    if (Multiply > 1)
    {
        PrintUSize(Multiply);
        PrintCharacter('*');
    }

    PrintNormalReg(REX, Index, Size);

    if (Disp == 0)
    {
        Print(Str("]"));
    }
    else if (Disp < 0)
    {
        Print(Str(" - "));
        PrintUSize(-Disp);
        Print(Str("]"));
    }
    else
    {
        Print(Str(" + "));
        PrintUSize(Disp);
        Print(Str("]"));
    }
}

local void PrintRMDisp(u8 REX, u8 RM, ssize Disp, u8 Size)
{
    Print(Str("["));
    PrintNormalReg(REX, RM, Size);

    if (Disp == 0)
    {
        Print(Str("]"));
    }
    else if (Disp < 0)
    {
        Print(Str(" - "));
        PrintUSize(-Disp);
        Print(Str("]"));
    }
    else
    {
        Print(Str(" + "));
        PrintUSize(Disp);
        Print(Str("]"));
    }
}

local void PrintRM(u8 REX, parsed_operands Operands, u8 Size)
{
    u8 REX_B = REX & 0x1;
    u8 RM    = 8*REX_B + (Operands.ModRM) & 0x7;
    u8 Mode  = (Operands.ModRM >> 6);

    switch (Mode)
    {
        case 0:
        {
            if ((RM & 0x7) == 0x4)
            {
                PrintSIB(REX, Operands.SIB, Size);
            }
            else if ((RM & 0x7) == 0x5)
            {
                PrintRIPDisp(Operands.Disp, Size);
            }
            else
            {
                PrintDerefReg(REX, RM, Size);
            }
        } break;

        case 1:
        case 2:
        {
            if ((RM & 0x7) == 0x4)
            {
                PrintSIBDisp(REX, Operands.SIB, Operands.Disp, Size);
            }
            else
            {
                PrintRMDisp(REX, RM, Operands.Disp, Size);
            }
        } break;

        case 3:
        {
            PrintNormalReg(REX, RM, Size);
        } break;
    }
}

local void PrintRegRM(u8 REX, parsed_operands Operands, u8 LeftSize, u8 RightSize)
{
    u8 REX_R = (REX >> 2) & 1;
    u8 Reg = 8*REX_R + (Operands.ModRM >> 3) & 0x7;

    PrintNormalReg(REX, Reg, LeftSize);
    Print(Str(", "));
    PrintRM(REX, Operands, RightSize);
}

local void PrintRMReg(u8 REX, parsed_operands Operands, u8 LeftSize, u8 RightSize)
{
    u8 REX_R = (REX >> 2) & 1;
    u8 Reg = 8*REX_R + (Operands.ModRM >> 3) & 0x7;

    PrintRM(REX, Operands, RightSize);
    Print(Str(", "));
    PrintNormalReg(REX, Reg, LeftSize);
}

local void Disassemble(void* Data, usize Size)
{
    u8* Bytes = (u8*)Data;
    usize Index = 0;

    persist string RegisterNames8_NO_REX[] =
    {
        StaticStr("al"),
        StaticStr("cl"),
        StaticStr("dl"),
        StaticStr("bl"),
        StaticStr("ah"),
        StaticStr("ch"),
        StaticStr("dh"),
        StaticStr("bh"),
        StaticStr("r8l"),
        StaticStr("r9l"),
        StaticStr("r10l"),
        StaticStr("r11l"),
        StaticStr("r12l"),
        StaticStr("r13l"),
        StaticStr("r14l"),
        StaticStr("r15l"),
    };

    persist string RegisterNames8_REX[] =
    {
        StaticStr("al"),
        StaticStr("cl"),
        StaticStr("dl"),
        StaticStr("bl"),
        StaticStr("spl"),
        StaticStr("bpl"),
        StaticStr("sil"),
        StaticStr("dil"),
        StaticStr("r8l"),
        StaticStr("r9l"),
        StaticStr("r10l"),
        StaticStr("r11l"),
        StaticStr("r12l"),
        StaticStr("r13l"),
        StaticStr("r14l"),
        StaticStr("r15l"),
    };

    persist string RegisterNames16[] =
    {
        StaticStr("ax"),
        StaticStr("cx"),
        StaticStr("dx"),
        StaticStr("bx"),
        StaticStr("sp"),
        StaticStr("bp"),
        StaticStr("si"),
        StaticStr("di"),
        StaticStr("r8w"),
        StaticStr("r9w"),
        StaticStr("r10w"),
        StaticStr("r11w"),
        StaticStr("r12w"),
        StaticStr("r13w"),
        StaticStr("r14w"),
        StaticStr("r15w"),
    };

    persist string RegisterNames32[] =
    {
        StaticStr("eax"),
        StaticStr("ecx"),
        StaticStr("edx"),
        StaticStr("ebx"),
        StaticStr("esp"),
        StaticStr("ebp"),
        StaticStr("esi"),
        StaticStr("edi"),
        StaticStr("r8d"),
        StaticStr("r9d"),
        StaticStr("r10d"),
        StaticStr("r11d"),
        StaticStr("r12d"),
        StaticStr("r13d"),
        StaticStr("r14d"),
        StaticStr("r15d"),
    };

    persist string RegisterNames64[] =
    {
        StaticStr("rax"),
        StaticStr("rcx"),
        StaticStr("rdx"),
        StaticStr("rbx"),
        StaticStr("rsp"),
        StaticStr("rbp"),
        StaticStr("rsi"),
        StaticStr("rdi"),
        StaticStr("r8"),
        StaticStr("r9"),
        StaticStr("r10"),
        StaticStr("r11"),
        StaticStr("r12"),
        StaticStr("r13"),
        StaticStr("r14"),
        StaticStr("r15"),
    };

    while (Index < Size)
    {
        usize StartIndex = Index;

        PrintInstructionOffset(StartIndex);

        u8 Byte = Bytes[Index];
        u8 REX = 0;

        if ((Byte >= 0x40) && (Byte <= 0x4F))
        {
            REX = Byte;

            Index++;
            if (Index == Size)
            {
                Println(Str("Expected op code after REX prefix"));
                Exit(1);
            }

            Byte = Bytes[Index];
        }

        u8 REX_W = (REX >> 3) & 1;
        u8 REX_R = (REX >> 2) & 1;
        u8 REX_X = (REX >> 1) & 1;
        u8 REX_B = (REX >> 0) & 1;

        if ((Byte >= 0x50) && (Byte <= 0x57))
        {
            Index++;
            OutputInstructionHex(Data, StartIndex, Index);

            u8 Register = REX_R*8 + (Byte - 0x50);
            PadInstruction(Print(Str("push ")));
            Print(RegisterNames64[Register]);
            PrintCharacter('\n');
        }
        else if ((Byte >= 0x58) && (Byte <= 0x5F))
        {
            Index++;
            OutputInstructionHex(Data, StartIndex, Index);

            u8 Register = REX_R*8 + (Byte - 0x58);
            PadInstruction(Print(Str("pop ")));
            Print(RegisterNames64[Register]);
            PrintCharacter('\n');
        }
        else if (Byte == 0x8b)
        {
            Index++;

            parsed_operands Operands = ParseModRM(Bytes, &Index, Size);

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Size = (REX_W) ? (64) : (32);

            PadInstruction(Print(Str("mov ")));
            PrintRegRM(REX, Operands, Size, Size);
            PrintCharacter('\n');
        }
        else if (Byte == 0x89)
        {
            Index++;

            parsed_operands Operands = ParseModRM(Bytes, &Index, Size);

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Size = (REX_W) ? (64) : (32);

            PadInstruction(Print(Str("mov ")));
            PrintRMReg(REX, Operands, Size, Size);
            PrintCharacter('\n');
        }
        else if (Byte == 0x8d)
        {
            Index++;

            parsed_operands Operands = ParseModRM(Bytes, &Index, Size);

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Size = (REX_W) ? (64) : (32);

            PadInstruction(Print(Str("lea ")));
            PrintRegRM(REX, Operands, Size, Size);
            PrintCharacter('\n');
        }
        else if ((Byte >= 0xb8) && (Byte <= 0xbf))
        {
            Index++;

            usize ImmSize = (REX_W) ? (8) : (4);

            if (Index + ImmSize >= Size)
            {
                Println(Str("Expected IMM64 after MOV"));
                Exit(1);
            }

            usize Value = 0;
            if (REX_W)
            {
                Value = *(u64*)(Bytes + Index);
                Index += 8;
            }
            else
            {
                Value = *(u32*)(Bytes + Index);
                Index += 4;
            }

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Dest = REX_R*8 + (Byte - 0xb8);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            PadInstruction(Print(Str("mov ")));
            Print(RegisterNameMap[Dest]);
            Print(Str(", "));
            PrintUSize(Value);
            PrintCharacter('\n');
        }
        else if (Byte == 0x03)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected MODRM after ADD"));
                Exit(1);
            }

            u8 ModRM = Bytes[Index];
            Index++;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Dest   = REX_R*8 + ((ModRM >> 3) & 0x7);
            u8 Source = REX_B*8 + ((ModRM >> 0) & 0x7);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            PadInstruction(Print(Str("add ")));
            Print(RegisterNameMap[Dest]);
            Print(Str(", "));
            Print(RegisterNameMap[Source]);
            PrintCharacter('\n');
        }
        else if (Byte == 0x2b)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected MODRM after SUB"));
                Exit(1);
            }

            u8 ModRM = Bytes[Index];
            Index++;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Dest   = REX_R*8 + ((ModRM >> 3) & 0x7);
            u8 Source = REX_B*8 + ((ModRM >> 0) & 0x7);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            PadInstruction(Print(Str("sub ")));
            Print(RegisterNameMap[Dest]);
            Print(Str(", "));
            Print(RegisterNameMap[Source]);
            PrintCharacter('\n');
        }
        else if (Byte == 0x81)
        {
            Index++;

            parsed_operands Operands = ParseModRM(Bytes, &Index, Size);

            if (Index + 4 > Size)
            {
                Println(Str("Expected IMM32 after SUB"));
                Exit(1);
            }

            ssize Imm32 = (ssize)(*(s32*)(Bytes + Index));
            Index += 4;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Size = (REX_W) ? (64) : (32);

            u8 Select = (Operands.ModRM >> 3) & 0x7;
            if (Select == 0x5)
            {
                PadInstruction(Print(Str("sub ")));
                PrintRM(REX, Operands, Size);
                Print(Str(", "));
                PrintSSize(Imm32);
            }
            else
            {
                Println(Str("Unknown select for opcode 0x81"));
                Exit(1);
            }

            PrintCharacter('\n');
        }
        else if (Byte == 0x3b)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected MODRM after CMP"));
                Exit(1);
            }

            u8 ModRM = Bytes[Index];
            Index++;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Dest   = REX_R*8 + ((ModRM >> 3) & 0x7);
            u8 Source = REX_B*8 + ((ModRM >> 0) & 0x7);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            PadInstruction(Print(Str("cmp ")));
            Print(RegisterNameMap[Dest]);
            Print(Str(", "));
            Print(RegisterNameMap[Source]);
            PrintCharacter('\n');
        }
        else if (Byte == 0x85)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected MODRM after TEST"));
                Exit(1);
            }

            u8 ModRM = Bytes[Index];
            Index++;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Dest   = REX_B*8 + ((ModRM >> 0) & 0x7);
            u8 Source = REX_R*8 + ((ModRM >> 3) & 0x7);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            PadInstruction(Print(Str("test ")));
            Print(RegisterNameMap[Dest]);
            Print(Str(", "));
            Print(RegisterNameMap[Source]);
            PrintCharacter('\n');
        }
        else if (Byte == 0xe9)
        {
            Index++;

            if (Index + 4 > Size)
            {
                Println(Str("Expected REL32 after JMP instruction"));
                Exit(1);
            }

            s32 Displacement = *(s32*)(Bytes + Index);
            Index += 4;

            OutputInstructionHex(Data, StartIndex, Index);

            PadInstruction(Print(Str("jmp ")));
            Print(Str("rel32("));
            PrintSSize(Displacement);
            Println(Str(")"));
        }
        else if (Byte == 0x99)
        {
            Index++;
            OutputInstructionHex(Data, StartIndex, Index);

            if (REX_W)
            {
                Println(Str("cqo"));
            }
            else
            {
                Println(Str("cdq"));
            }
        }
        else if (Byte == 0x0F)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected opcode after 0x0F"));
                Exit(1);
            }

            Byte = Bytes[Index];

            if (Byte == 0xAF)
            {
                Index++;
                if (Index == Size)
                {
                    Println(Str("Expected MODRM after IMUL"));
                    Exit(1);
                }

                u8 ModRM = Bytes[Index];
                Index++;

                OutputInstructionHex(Data, StartIndex, Index);

                u8 Dest   = REX_R*8 + ((ModRM >> 3) & 0x7);
                u8 Source = REX_B*8 + ((ModRM >> 0) & 0x7);

                string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

                PadInstruction(Print(Str("imul ")));
                Print(RegisterNameMap[Dest]);
                Print(Str(", "));
                Print(RegisterNameMap[Source]);
                PrintCharacter('\n');
            }
            else if ((Byte >= 0x80) && (Byte <= 0x8F))
            {
                persist string Instruction[16] =
                {
                    StaticStr("jo"),
                    StaticStr("jno"),
                    StaticStr("jb"),
                    StaticStr("jae"),
                    StaticStr("jz"),
                    StaticStr("jnz"),
                    StaticStr("jbe"),
                    StaticStr("ja"),
                    StaticStr("js"),
                    StaticStr("jns"),
                    StaticStr("jp"),
                    StaticStr("jnp"),
                    StaticStr("jl"),
                    StaticStr("jnl"),
                    StaticStr("jle"),
                    StaticStr("jg"),
                };

                Index++;
                if (Index + 4 > Size)
                {
                    Println(Str("Expected REL32 after Jcc instruction"));
                    Exit(1);
                }

                s32 Displacement = *(s32*)(Bytes + Index);
                Index += 4;

                OutputInstructionHex(Data, StartIndex, Index);

                PadInstruction(Print(Instruction[Byte - 0x80]));
                Print(Str("rel32("));
                PrintSSize(Displacement);
                Print(Str(")"));
                PrintCharacter('\n');
            }
            else if ((Byte >= 0x90) && (Byte <= 0x9F))
            {
                persist string Instruction[16] =
                {
                    StaticStr("seto"),
                    StaticStr("setno"),
                    StaticStr("setb"),
                    StaticStr("setae"),
                    StaticStr("setz"),
                    StaticStr("setnz"),
                    StaticStr("setbe"),
                    StaticStr("seta"),
                    StaticStr("sets"),
                    StaticStr("setns"),
                    StaticStr("setp"),
                    StaticStr("setnp"),
                    StaticStr("setl"),
                    StaticStr("setnl"),
                    StaticStr("setle"),
                    StaticStr("setg"),
                };

                Index++;
                if (Index == Size)
                {
                    Println(Str("Expected MODRM after SETcc"));
                    Exit(1);
                }

                u8 ModRM = Bytes[Index];
                Index++;

                OutputInstructionHex(Data, StartIndex, Index);

                u8 Dest = REX_R*8 + ((ModRM >> 0) & 0x7);

                string* RegisterNameMap = (REX) ? RegisterNames8_REX : RegisterNames8_NO_REX;

                PadInstruction(Print(Instruction[Byte - 0x90]));
                Print(RegisterNameMap[Dest]);
                PrintCharacter('\n');
            }
            else if (Byte == 0xB6)
            {
                Index++;
                if (Index == Size)
                {
                    Println(Str("Expected MODRM after SETcc"));
                    Exit(1);
                }

                u8 ModRM = Bytes[Index];
                Index++;

                OutputInstructionHex(Data, StartIndex, Index);

                u8 Dest   = REX_R*8 + ((ModRM >> 3) & 0x7);
                u8 Source = REX_B*8 + ((ModRM >> 0) & 0x7);

                string* DestNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;
                string* SourceNameMap = (REX) ? RegisterNames8_REX : RegisterNames8_NO_REX;

                PadInstruction(Print(Str("movzx ")));
                Print(DestNameMap[Dest]);
                Print(Str(", "));
                Print(SourceNameMap[Source]);
                PrintCharacter('\n');
            }
            else
            {
                Println(Str("Expected valid opcode after 0x0F"));
                Exit(1);
            }
        }
        else if (Byte == 0xF7)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected MODRM after opcode F7"));
                Exit(1);
            }

            u8 ModRM = Bytes[Index];
            Index++;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Select = ((ModRM >> 3) & 0x7);
            u8 RM = REX_B*8 + ((ModRM >> 0) & 0x7);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            if (Select == 7)
            {
                PadInstruction(Print(Str("idiv ")));
                Print(RegisterNameMap[RM]);
                PrintCharacter('\n');
            }
            else if (Select == 3)
            {
                PadInstruction(Print(Str("neg ")));
                Print(RegisterNameMap[RM]);
                PrintCharacter('\n');
            }
            else if (Select == 2)
            {
                PadInstruction(Print(Str("not ")));
                Print(RegisterNameMap[RM]);
                PrintCharacter('\n');
            }
            else
            {
                Println(Str("Unknown select for opcode 0xF7"));
                Exit(1);
            }
        }
        else if (Byte == 0xd3)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected MODRM after IDIV"));
                Exit(1);
            }

            u8 ModRM = Bytes[Index];
            Index++;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Select = ((ModRM >> 3) & 0x7);
            u8 Dest = REX_B*8 + ((ModRM >> 0) & 0x7);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            if (Select == 4)
            {
                PadInstruction(Print(Str("sal ")));
                Print(RegisterNameMap[Dest]);
                Print(Str(", cl"));
                PrintCharacter('\n');
            }
            else if (Select == 7)
            {
                PadInstruction(Print(Str("sar ")));
                Print(RegisterNameMap[Dest]);
                Print(Str(", cl"));
                PrintCharacter('\n');
            }
            else
            {
                Println(Str("Unknown select for opcode 0xd3"));
                Exit(1);
            }
        }
        else if (Byte == 0x23)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected MODRM after AND"));
                Exit(1);
            }

            u8 ModRM = Bytes[Index];
            Index++;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Dest   = REX_R*8 + ((ModRM >> 3) & 0x7);
            u8 Source = REX_B*8 + ((ModRM >> 0) & 0x7);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            PadInstruction(Print(Str("and ")));
            Print(RegisterNameMap[Dest]);
            Print(Str(", "));
            Print(RegisterNameMap[Source]);
            PrintCharacter('\n');
        }
        else if (Byte == 0x0b)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected MODRM after OR"));
                Exit(1);
            }

            u8 ModRM = Bytes[Index];
            Index++;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Dest   = REX_R*8 + ((ModRM >> 3) & 0x7);
            u8 Source = REX_B*8 + ((ModRM >> 0) & 0x7);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            PadInstruction(Print(Str("or ")));
            Print(RegisterNameMap[Dest]);
            Print(Str(", "));
            Print(RegisterNameMap[Source]);
            PrintCharacter('\n');
        }
        else if (Byte == 0x33)
        {
            Index++;
            if (Index == Size)
            {
                Println(Str("Expected MODRM after XOR"));
                Exit(1);
            }

            u8 ModRM = Bytes[Index];
            Index++;

            OutputInstructionHex(Data, StartIndex, Index);

            u8 Dest   = REX_R*8 + ((ModRM >> 3) & 0x7);
            u8 Source = REX_B*8 + ((ModRM >> 0) & 0x7);

            string* RegisterNameMap = (REX_W) ? RegisterNames64 : RegisterNames32;

            PadInstruction(Print(Str("xor ")));
            Print(RegisterNameMap[Dest]);
            Print(Str(", "));
            Print(RegisterNameMap[Source]);
            PrintCharacter('\n');
        }
        else if (Byte == 0xc3)
        {
            Index++;
            OutputInstructionHex(Data, StartIndex, Index);
            Index++;

            Println(Str("ret"));
        }
        else
        {
            Index++;
        }
    }
}

