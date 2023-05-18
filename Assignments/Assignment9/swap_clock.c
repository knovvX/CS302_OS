#include <defs.h>
#include <riscv.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_clock.h>
#include <list.h>
#include <pmm.h>

list_entry_t pra_list_head, *curr_ptr;

static int
_clock_init_mm(struct mm_struct *mm)
{
    // TODO
    list_init(&pra_list_head);
    mm->sm_priv = &pra_list_head;
    return 0;
}

static int
_clock_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    // TODO
    list_entry_t *head = (list_entry_t *)mm->sm_priv;
    list_entry_t *entry = &(page->pra_page_link);

    assert(entry != NULL && head != NULL);
    // 如果列表为空,让entry插到head后面并且形成环
    if ((head->next) == head)
    {
        head->prev=entry;
        entry->next=head;
        entry->prev=entry;
        head->next=head;
        mm->sm_priv = entry;
    }
    // 否则的话插到head前面，因为head是最近访问的
    else
    {
        list_add_before(head, entry);
    }
    return 0;
}

static int
_clock_swap_out_victim(struct mm_struct *mm, struct Page **ptr_page, int in_tick)
{
    // TODO
    list_entry_t *head = (list_entry_t *)mm->sm_priv;
    assert(head != NULL);
    assert(in_tick == 0);
    list_entry_t *entry = head;
    *ptr_page = NULL;
    // go through the entries of the list to find victim page
    while (list_next(entry) != head)
    {
        struct Page *page = le2page(entry, pra_page_link);
        pte_t *ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);
        bool accessed = *ptep & PTE_A;
        // 找到了reference bit为0的，替换
        if (!accessed)
        {
            *ptr_page = page;
            mm->sm_priv = list_next(entry);
            list_del(entry);
            break;
        }
        // 否则置0，继续往下找
        *ptep &= ~PTE_A;
        entry = list_next(entry);
    }
    // 如果一圈都没有找到，找回head
    if (*ptr_page == NULL)
    {
        *ptr_page = le2page(head, pra_page_link);
        mm->sm_priv = list_next(head);
        list_del(head);
    }

    return 0;
}

static int
_clock_check_swap(void)
{

    cprintf("---------Clock check begin----------\n");
    cprintf("write Virt Page c in clock_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num == 4);
    cprintf("write Virt Page a in clock_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 4);
    cprintf("write Virt Page d in clock_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num == 4);
    cprintf("write Virt Page b in clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num == 4);
    cprintf("write Virt Page e in clock_check_swap\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num == 5);
    cprintf("write Virt Page b in clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num == 5);
    cprintf("write Virt Page a in clock_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    cprintf("page fault num:%d\n", pgfault_num);
    assert(pgfault_num == 6);
    cprintf("write Virt Page b in clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num == 6);
    cprintf("write Virt Page c in clock_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num == 7);
    cprintf("write Virt Page d in clock_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num == 8);
    cprintf("write Virt Page e in clock_check_swap\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num == 9);
    cprintf("write Virt Page a in clock_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 9);
    cprintf("Clock check succeed!\n");

    return 0;
}

static int
_clock_init(void)
{
    return 0;
}

static int
_clock_set_unswappable(struct mm_struct *mm, uintptr_t addr)
{
    return 0;
}

static int
_clock_tick_event(struct mm_struct *mm)
{
    return 0;
}

struct swap_manager swap_manager_clock =
    {
        .name = "clock swap manager",
        .init = &_clock_init,
        .init_mm = &_clock_init_mm,
        .tick_event = &_clock_tick_event,
        .map_swappable = &_clock_map_swappable,
        .set_unswappable = &_clock_set_unswappable,
        .swap_out_victim = &_clock_swap_out_victim,
        .check_swap = &_clock_check_swap,
};
