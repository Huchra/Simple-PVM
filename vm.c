#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define OPC(i) ((i) >> 12)
#define NOPS (16)
#define SEXTIMM(i) sext(IMM(i), 5)
#define FIMM(i) ((i >> 5) & i)
#define DR(i) (((i) >> 9) & 0x7)
#define SR1(i) (((i >> 6)) & 0x7)
#define SR2(i) ((i) & 0x7)
#define P0FF9(i) sext((i) & 0x1FF, 9)
#define IMM(i) ((i) & 0X1F)
#define P0FF(i) sext((i) & 0X3F, 6)
#define FL(i) (((i) >> 9) & 1)
#define BR(i) (((i) >> 6) & 0x7)
#define POFF11(i) sext((i)&0x7FF, 11)
#define FCND(i) (((i) >> 9) & 0x7)
#define TRP(i) ((i) & 0XFF)


enum regi { R0 = 0, R1, R2, R3, R4, R5, R6, R7, RPC, RCND, RCNT };
enum flags { FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2 };


bool running = true;
uint16_t PC_START = 0x3000;
uint16_t reg[RCNT] = {0};
uint16_t mem[UINT16_MAX] = {0};


static inline uint16_t sext(uint16_t n, int b)
{
    return ((n>>(b-1)) & 1) ?
        (n | (0XFFFF << b)) : n;
}

typedef void (*op_ex_f)(uint16_t instruction);

static inline void add(uint16_t i)
{
    reg[DR(i)] = reg[SR1(i)] +
        (FIMM(i) ?
            SEXTIMM(i) :
            reg[SR2(i)]);
    
}
static inline void and(uint16_t i)
{

}


static inline void res(uint16_t i)
{

}
static inline uint16_t mr(uint16_t address)
{
    return mem[address];
}

static inline void mw(uint16_t address, uint16_t val)
{
    mem[address] = val;
}


static inline void uf(enum regi r){
    if (reg[r] == 0) reg[RCND] = FZ; 
    else if (reg[r] >> 15) reg[RCND] = FN;
    else reg[RCND] = FP;
}

static inline void ld(uint16_t i)
{
    reg[DR(i)] = mr(reg[RPC] + P0FF9(i));
    uf(DR(i));
}

static inline void ldi(uint16_t i)
{
    reg[DR(i)] = mr(mr(reg[RPC] + P0FF9(i)));
    uf(DR(i));
}

static inline void ldr(uint16_t i)
{
    reg[DR(i)] = mr(reg[SR1(i)] + P0FF(i));
    uf(DR(i));
}

static inline void lea(uint16_t i)
{
    reg[DR(i)] = reg[RPC] + P0FF9(i);
    uf(DR(i));
}

static inline void not(uint16_t i)
{
    reg[DR(i)] = ~(SR1(i));
    uf(DR(i));
}

static inline void st(uint16_t i)
{
    mw(reg[RPC] + P0FF9(i), DR(i));
}

static inline void sti(uint16_t i)
{
    mw(mr(reg[RPC] + P0FF9(i)), DR(i));
}

static inline void str(uint16_t i)
{
    mw(reg[SR1(i)] + P0FF9(i), DR(i));
}

static inline void jmp(uint16_t i)
{
    reg[RPC] = reg[BR(i)];
}

static inline void jsr(uint16_t i)
{
    reg[R7] = reg[RPC];
    reg[RPC] = (FL(i)) ?
        reg[RPC] + POFF11(i) :
        BR(i);
}

static inline void br(uint16_t i)
{
    if (reg[RCND] & FCND(i)) {
        reg[RPC] += P0FF9(i);
    }
}


static inline void tgetc() 
{
    reg[R0] = getchar();
}

static inline void tout()
{
    fprintf(stdout, "%c", (char)reg[R0]); 
}

static inline void tputs()
{
    uint16_t *p = mem + reg[R0];
    while(*p) {
        fprintf(stdout, "%c", (char)*p);
        p++;
    }
}

static inline void tin()
{
    reg[R0] = getchar();
    fprintf(stdout, "%c", reg[R0]);
}

static inline void tputsp()
{

}

static inline void thalt()
{
    running = false;
}

static inline void tinu16()
{
    fscanf(stdin, "%hu\n", &reg[R0]);
}

static inline void toutu16()
{
    fprintf(stdout, "%hu\n", reg[R0]);
}




enum { trp_offset = 0x20 };
typedef void (*trp_ex_f)();
trp_ex_f trp_ex[8] = { tgetc, tout, tputs, tin, tputsp, thalt, tinu16, toutu16 };

static inline void trap(uint16_t i)
{
    trp_ex[TRP(i) - trp_offset]();
}

op_ex_f op_ex[NOPS] =
{
    br, add, ld, st, jsr, and, ldr, str, sti, not, ldi, sti, jmp, res, lea, trap
};


void start(uint16_t offset)
{
    reg[RPC] = PC_START + offset;
    while(running)
    {
        uint16_t i = mr(reg[RPC]++);

        op_ex[OPC(i)](i);
    }
}

void ld_img(char *fname, uint16_t offset)
{
    FILE *in = fopen(fname, "rb");
    if (NULL == in)
    {
        fprintf(stderr, "Cannot open file %s.\n", fname);
        exit(1);
    }

    uint16_t *p = mem + PC_START + offset;

    fread(p, sizeof(uint16_t), (UINT16_MAX - PC_START), in);
    fclose(in);
}

int main(int argc, char **argv)
{
    ld_img(argv[1], 0x0);
    start(0x0);
    return 0;
}











