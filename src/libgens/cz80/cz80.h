/********************************************************************************/
/*                                                                              */
/* CZ80 include file                                                            */
/* C Z80 emulator version 0.91 	                                                */
/* Copyright 2004 Stephane Dallongeville                                        */
/*                                                                              */
/********************************************************************************/

/*
  2011-10-18: modified for DGen/SDL.
*/

#ifndef _CZ80_H_
#define _CZ80_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif




/******************************/
/* Compiler dependant defines */
/******************************/

#ifdef _MSC_VER

// MSVC: Enable FASTCALL on i386.
// (Copied from MinGW-w64's winnt.h.)
#if defined (_M_IX86)
#define CZ80CALL __fastcall
#else
#define CZ80CALL
#endif

#else
#define CZ80CALL
#endif



/*************************************/
/* Z80 core Structures & definitions */
/*************************************/

#define CZ80_FETCH_BITS         4   // [4-12]   default = 8

#define CZ80_FETCH_SFT          (16 - CZ80_FETCH_BITS)
#define CZ80_FETCH_BANK         (1 << CZ80_FETCH_BITS)

#ifdef WORDS_BIGENDIAN
#define CZ80_LITTLE_ENDIAN 0
#else
#define CZ80_LITTLE_ENDIAN      1
#endif

// Enable Jump Table optimizations on gcc.
#ifdef __GNUC__
#define CZ80_USE_JUMPTABLE      1
#else
#define CZ80_USE_JUMPTABLE      0
#endif

// Emulator Configuration
#define CZ80_SIZE_OPT           0
#define CZ80_USE_WORD_HANDLER   0
// TODO: Remove this.
#define CZ80_DEBUG              0
// Emulate the YF and XF flags exactly.
// This also enables proper WZ register emulation.
#define CZ80_EXACT              1
// Emulate the "R" register exactly.
// Reference: http://www.z80.info/zip/z80-documented.pdf
#define CZ80_EXACT_R            1

// use zR8 for B/C/D/E/H/L registers only
// use zR16 for BC/DE/HL registers only

#if CZ80_LITTLE_ENDIAN
#define zR8(A)          CPU->creg.r8[(A) ^ 1]
#else
#define zR8(A)          CPU->creg.r8[(A)]
#endif
#define zR16(A)         CPU->creg.r16[A].W
#define pzR16(A)        &(CPU->creg.r16[A])

#define pzFA            &(CPU->creg.name.FA)
#define zFA             CPU->creg.name.FA.W
#define zlFA            CPU->creg.name.FA.B.L
#define zhFA            CPU->creg.name.FA.B.H
#define zA              zlFA
#define zF              zhFA

#define pzBC            &(CPU->creg.name.BC)
#define zBC             CPU->creg.name.BC.W
#define zlBC            CPU->creg.name.BC.B.L
#define zhBC            CPU->creg.name.BC.B.H
#define zB              zhBC
#define zC              zlBC

#define pzDE            &(CPU->creg.name.DE)
#define zDE             CPU->creg.name.DE.W
#define zlDE            CPU->creg.name.DE.B.L
#define zhDE            CPU->creg.name.DE.B.H
#define zD              zhDE
#define zE              zlDE

#define pzHL            &(CPU->creg.name.HL)
#define zHL             CPU->creg.name.HL.W
#define zlHL            CPU->creg.name.HL.B.L
#define zhHL            CPU->creg.name.HL.B.H
#define zH              zhHL
#define zL              zlHL

#define zFA2            CPU->FA2.W
#define zlFA2           CPU->FA2.B.L
#define zhFA2           CPU->FA2.B.H
#define zA2             zlFA2
#define zF2             zhFA2

#define zBC2            CPU->BC2.W
#define zDE2            CPU->DE2.W
#define zHL2            CPU->HL2.W

#define pzIX            &(CPU->IX)
#define zIX             CPU->IX.W
#define zlIX            CPU->IX.B.L
#define zhIX            CPU->IX.B.H

#define pzIY            &(CPU->IY)
#define zIY             CPU->IY.W
#define zlIY            CPU->IY.B.L
#define zhIY            CPU->IY.B.H

#define pzSP            &(CPU->SP)
#define zSP             CPU->SP.W
#define zlSP            CPU->SP.B.L
#define zhSP            CPU->SP.B.H

// WZ register. Internal register used for address calculations.
// This register must be emulated correctly in order to emulate
// flag bits 3 and 5 for the BIT n,(HL) instruction.
// NOTE: Stored as local variable for performance reasons.
#define zWZ		WZ

// Program Counter.
// NOTE: Stored as local variable for performance reasons.
#define zPC             PC

#define zI              CPU->I
#define zIM             CPU->IM

// R register.
// For performance reasons, the high bit is stored
// in zR2, since the Z80 only modifies the low 7 bits
// during the refresh cycle.
#define zwR             CPU->R.W
#define zR1             CPU->R.B.L
#define zR2             CPU->R.B.H
#define zR              zR1

#define zIFF            CPU->IFF.W
#define zIFF1           CPU->IFF.B.L
#define zIFF2           CPU->IFF.B.H

#define CZ80_SF_SFT     7
#define CZ80_ZF_SFT     6
#define CZ80_YF_SFT     5
#define CZ80_HF_SFT     4
#define CZ80_XF_SFT     3
#define CZ80_PF_SFT     2
#define CZ80_VF_SFT     2
#define CZ80_NF_SFT     1
#define CZ80_CF_SFT     0

#define CZ80_SF         (1 << CZ80_SF_SFT)
#define CZ80_ZF         (1 << CZ80_ZF_SFT)
#define CZ80_YF         (1 << CZ80_YF_SFT)
#define CZ80_HF         (1 << CZ80_HF_SFT)
#define CZ80_XF         (1 << CZ80_XF_SFT)
#define CZ80_PF         (1 << CZ80_PF_SFT)
#define CZ80_VF         (1 << CZ80_VF_SFT)
#define CZ80_NF         (1 << CZ80_NF_SFT)
#define CZ80_CF         (1 << CZ80_CF_SFT)

#define CZ80_IFF_SFT    CZ80_PF_SFT
#define CZ80_IFF        CZ80_PF

#define CZ80_HAS_INT    CZ80_IFF
#define CZ80_HAS_NMI    0x08

#define CZ80_RUNNING    0x10
#define CZ80_HALTED     0x20
#define CZ80_FAULTED    0x80
#define CZ80_DISABLE    0x40


typedef uint8_t CZ80CALL CZ80_READ(void *ctx, uint16_t adr);
typedef void CZ80CALL CZ80_WRITE(void *ctx, uint16_t adr, uint8_t data);

#if CZ80_USE_WORD_HANDLER
typedef uint16_t CZ80CALL CZ80_READ_WORD(void *ctx, uint16_t adr);
typedef void CZ80CALL CZ80_WRITE_WORD(void *ctx, uint16_t adr, uint16_t data);
#endif

typedef void CZ80CALL CZ80_RETI_CALLBACK(void *ctx);
typedef uint8_t CZ80CALL CZ80_INT_CALLBACK(void *ctx, uint8_t param);

typedef union
{
        struct
        {
#if CZ80_LITTLE_ENDIAN
                uint8_t L;
                uint8_t H;
#else
                uint8_t H;
                uint8_t L;
#endif
        } B;
        uint16_t W;
} union16;

typedef struct
{
    union
    {
        uint8_t r8[8];
        union16 r16[4];
        struct
        {
            union16 BC;
            union16 DE;
            union16 HL;
            union16 FA;
        } name;
    } creg;

    union16 IX;
    union16 IY;
    union16 SP;

#if CZ80_EXACT
    uint16_t WZ;
#endif
    uint16_t PC;
    
    union16 BC2;
    union16 DE2;
    union16 HL2;
    union16 FA2;

        union16 R;
        union16 IFF;
        
        uint8_t I;
        uint8_t IM;
        uint8_t IntVect;
        uint8_t Status;

        uintptr_t BasePC;
        int CycleIO;
        
        int CycleToDo;
        int CycleSup;

	void *ctx;

        CZ80_READ *Read_Byte;
        CZ80_WRITE *Write_Byte;
#if CZ80_USE_WORD_HANDLER
        CZ80_READ_WORD *Read_Word;
        CZ80_WRITE_WORD *Write_Word;
#endif

        CZ80_READ *IN_Port;
        CZ80_WRITE *OUT_Port;

        CZ80_RETI_CALLBACK *RetI;
        CZ80_INT_CALLBACK *Interrupt_Ack;

        uint8_t *Fetch[CZ80_FETCH_BANK];
} cz80_struc;


/*************************/
/* Publics Z80 functions */
/*************************/

void    Cz80_Init(cz80_struc *cpu);
uint8_t Cz80_Reset(cz80_struc *cpu);
uint8_t Cz80_Soft_Reset(cz80_struc *cpu);

void    Cz80_Set_Fetch(cz80_struc *cpu, uint16_t low_adr, uint16_t high_adr, void *fetch_adr);

void    Cz80_Set_Ctx(cz80_struc *cpu, void *ctx);
void    Cz80_Set_ReadB(cz80_struc *cpu, CZ80_READ *Func);
void    Cz80_Set_WriteB(cz80_struc *cpu, CZ80_WRITE *Func);
#if CZ80_USE_WORD_HANDLER
void    Cz80_Set_ReadW(cz80_struc *cpu, CZ80_READ_WORD *Func);
void    Cz80_Set_WriteW(cz80_struc *cpu, CZ80_WRITE_WORD *Func);
#endif

void    Cz80_Set_INPort(cz80_struc *cpu, CZ80_READ *Func);
void    Cz80_Set_OUTPort(cz80_struc *cpu, CZ80_WRITE *Func);

void    Cz80_Set_IRQ_Callback(cz80_struc *cpu, CZ80_INT_CALLBACK *Func);
void    Cz80_Set_RETI_Callback(cz80_struc *cpu, CZ80_RETI_CALLBACK *Func);

uint8_t Cz80_Read_Byte(cz80_struc *cpu, uint16_t adr);
uint16_t Cz80_Read_Word(cz80_struc *cpu, uint16_t adr);
void    Cz80_Write_Byte(cz80_struc *cpu, uint16_t adr, uint8_t data);
void    Cz80_Write_Word(cz80_struc *cpu, uint16_t adr, uint16_t data);

int     CZ80CALL Cz80_Exec(cz80_struc *cpu, int cycles);

void    CZ80CALL Cz80_Set_IRQ(cz80_struc *cpu, uint8_t vector);
void    CZ80CALL Cz80_Set_NMI(cz80_struc *cpu);
void    CZ80CALL Cz80_Clear_IRQ(cz80_struc *cpu);
void    CZ80CALL Cz80_Clear_NMI(cz80_struc *cpu);

int     CZ80CALL Cz80_Get_CycleToDo(cz80_struc *cpu);
int     CZ80CALL Cz80_Get_CycleRemaining(cz80_struc *cpu);
int     CZ80CALL Cz80_Get_CycleDone(cz80_struc *cpu);
void    CZ80CALL Cz80_Release_Cycle(cz80_struc *cpu);
void    CZ80CALL Cz80_Add_Cycle(cz80_struc *cpu, unsigned int cycle);

uint16_t     CZ80CALL Cz80_Get_BC(cz80_struc *cpu);
uint16_t     CZ80CALL Cz80_Get_DE(cz80_struc *cpu);
uint16_t     CZ80CALL Cz80_Get_HL(cz80_struc *cpu);
uint16_t     CZ80CALL Cz80_Get_AF(cz80_struc *cpu);

uint16_t     CZ80CALL Cz80_Get_BC2(cz80_struc *cpu);
uint16_t     CZ80CALL Cz80_Get_DE2(cz80_struc *cpu);
uint16_t     CZ80CALL Cz80_Get_HL2(cz80_struc *cpu);
uint16_t     CZ80CALL Cz80_Get_AF2(cz80_struc *cpu);

uint16_t     CZ80CALL Cz80_Get_IX(cz80_struc *cpu);
uint16_t     CZ80CALL Cz80_Get_IY(cz80_struc *cpu);
uint16_t     CZ80CALL Cz80_Get_SP(cz80_struc *cpu);

#if CZ80_EXACT
uint16_t     CZ80CALL Cz80_Get_WZ(cz80_struc *cpu);
#endif
uint16_t     CZ80CALL Cz80_Get_PC(cz80_struc *cpu);

uint8_t      CZ80CALL Cz80_Get_R(cz80_struc *cpu);
uint16_t     CZ80CALL Cz80_Get_IFF(cz80_struc *cpu);
uint8_t      CZ80CALL Cz80_Get_IM(cz80_struc *cpu);
uint8_t      CZ80CALL Cz80_Get_I(cz80_struc *cpu);

void    CZ80CALL Cz80_Set_BC(cz80_struc *cpu, uint16_t value);
void    CZ80CALL Cz80_Set_DE(cz80_struc *cpu, uint16_t value);
void    CZ80CALL Cz80_Set_HL(cz80_struc *cpu, uint16_t value);
void    CZ80CALL Cz80_Set_AF(cz80_struc *cpu, uint16_t value);

void    CZ80CALL Cz80_Set_BC2(cz80_struc *cpu, uint16_t value);
void    CZ80CALL Cz80_Set_DE2(cz80_struc *cpu, uint16_t value);
void    CZ80CALL Cz80_Set_HL2(cz80_struc *cpu, uint16_t value);
void    CZ80CALL Cz80_Set_AF2(cz80_struc *cpu, uint16_t value);

void    CZ80CALL Cz80_Set_IX(cz80_struc *cpu, uint16_t value);
void    CZ80CALL Cz80_Set_IY(cz80_struc *cpu, uint16_t value);
void    CZ80CALL Cz80_Set_SP(cz80_struc *cpu, uint16_t value);

#if CZ80_EXACT
void    CZ80CALL Cz80_Set_WZ(cz80_struc *cpu, uint16_t value);
#endif
void    CZ80CALL Cz80_Set_PC(cz80_struc *cpu, uint16_t value);

void    CZ80CALL Cz80_Set_R(cz80_struc *cpu, uint8_t value);
void    CZ80CALL Cz80_Set_IFF(cz80_struc *cpu, uint16_t value);
void    CZ80CALL Cz80_Set_IM(cz80_struc *cpu, uint8_t value);
void    CZ80CALL Cz80_Set_I(cz80_struc *cpu, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif  // _CZ80_H_
