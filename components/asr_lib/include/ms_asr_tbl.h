#ifndef _SP_ASR_TBL_H
#define _SP_ASR_TBL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern const char* am_pny_vocab[408];
extern const char* am_pnytone_vocab[1250];

extern char** am_vocab;
extern const char digit_char[];
//tone
extern uint32_t lm_tbl_cnt;
extern char* lm_tbl[];
extern uint16_t am2lm[];

#define MAX_VOCAB_CNT   (5000)         //最大可能的vocab数量，为了简化malloc操作，直接申请大缓存

//pny
//extern const char* lm_tbl[406];
//extern uint16_t am2lm[408];


#endif



