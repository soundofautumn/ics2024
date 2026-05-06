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
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include "../local-include/reg.h"

#define SATP_PPN(satp) BITS(satp, 21, 0)
#define VPN1(vaddr) BITS(vaddr, 31, 22)
#define VPN0(vaddr) BITS(vaddr, 21, 12)
#define PTE_V(pte) BITS(pte, 0, 0)
#define PTE_R(pte) BITS(pte, 1, 1)
#define PTE_W(pte) BITS(pte, 2, 2)
#define PTE_X(pte) BITS(pte, 3, 3)
#define PTE_PPN1(pte) BITS(pte, 31, 22)
#define PTE_PPN0(pte) BITS(pte, 21, 10)
#define PTE_PPN(pte) BITS(pte, 31, 10)

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  // Sv32: 2-level page table, 4KB page size
  // VPN[1] | VPN[0] | page offset
  // 10      10       12
  
  word_t pte;
  paddr_t pdir = SATP_PPN(satp) << PAGE_SHIFT;
  paddr_t pte_addr = pdir + VPN1(vaddr) * sizeof(pte);
  pte = paddr_read(pte_addr, sizeof(pte));
  if (!PTE_V(pte)) return MEM_RET_FAIL;

  pdir = PTE_PPN1(pte) << PAGE_SHIFT;
  pte_addr = pdir + VPN0(vaddr) * sizeof(pte);
  pte = paddr_read(pte_addr, sizeof(pte));
  if (!PTE_V(pte)) return MEM_RET_FAIL;

  paddr_t page_offset = vaddr & PAGE_MASK;
  if (page_offset + len > PAGE_SIZE) return MEM_RET_CROSS_PAGE;
  paddr_t pa = (PTE_PPN(pte) << PAGE_SHIFT) | page_offset;
  Log("va = " FMT_VADDR ", pa = " FMT_PADDR ", pte = " FMT_WORD, vaddr, pa, pte);
  return pa;
}
