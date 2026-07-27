/* include/stm32mp157_gic.h */
#ifndef __STM32MP1XX_GIC_H__
#define __STM32MP1XX_GIC_H__ 

typedef struct {
    volatile unsigned int CTLR;
    volatile unsigned int TYPER;
    volatile unsigned int IIDR;
    volatile unsigned int RES1[29];
    volatile unsigned int IGROUPR[9];
    volatile unsigned int RES2[23];
    volatile unsigned int ISENABLER[9];
    volatile unsigned int RES3[23];
    volatile unsigned int ICENABLER[9];
    volatile unsigned int RES4[23];
    volatile unsigned int ISPENDR[9];
    volatile unsigned int RES5[23];
    volatile unsigned int ICPENDR[9];
    volatile unsigned int RES6[23];
    volatile unsigned int ISACTIVER[9];
    volatile unsigned int RES7[23];
    volatile unsigned int ICACTIVER[9];
    volatile unsigned int RES8[23];
    volatile unsigned int IPRIORITYR[72];
    volatile unsigned int RES9[184];
    volatile unsigned int ITARGETSR[72];
    volatile unsigned int RES10[184];   
    volatile unsigned int ICFGR[18];
    volatile unsigned int RES11[46];
}gicd_t;

#define GICD  ((gicd_t*)0xA0021000)

typedef struct {
    volatile unsigned int CTLR;
    volatile unsigned int PMR;
    volatile unsigned int BPR;
    volatile unsigned int IAR;
    volatile unsigned int EOIR;
    volatile unsigned int RPR;
    volatile unsigned int HPPIR;
    volatile unsigned int ABPR;
    volatile unsigned int AIAR;
    volatile unsigned int AEOIR;
    volatile unsigned int AHPPIR;
    volatile unsigned int RES1[41];
    volatile unsigned int APR0;
    volatile unsigned int RES2[3];
    volatile unsigned int NSAPR0;
    volatile unsigned int RES3[6];
    volatile unsigned int IIDR;
    volatile unsigned int RES4[960];
    volatile unsigned int DIR;
}gicc_t;

#define GICC  ((gicc_t*)0xA0022000) 

#endif 