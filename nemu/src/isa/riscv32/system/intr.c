/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include "../local-include/reg.h"


word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  mepc = epc;
  mcause = NO;
  mstatus =  ((mstatus & MIE) << 4) | (mstatus & ~MIE); // set MPIE to MIE, then clear MIE
  return mtvec;
}

word_t isa_query_intr() {
  if ((mstatus & MIE) && cpu.INTR) {
    cpu.INTR = false;
    return IRQ_TIMER;
  }
  return INTR_EMPTY;
}
