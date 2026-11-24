/*
 * mem_sections_pc.c —— 在 PC 上把 "ROM / RAM 之分" 变成看得见的地址与页属性
 *
 * 编译: gcc -O1 -Wall -o mem_sections_pc.exe mem_sections_pc.c
 * 运行: ./mem_sections_pc.exe
 *
 * 目的: 面试里说 "const 存在 ROM"，在 PC 上对应的就是"落在只读段、所在内存页被标为只读"。
 *       本程序用 VirtualQuery 把每个变量的页保护属性打印出来 —— 数字来自操作系统，不是嘴说。
 */
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

/* ---- 各种"变量"分别落在哪一段 ---- */
const int  g_const_tab[3] = { 1, 2, 3 };   /* const 全局 → .rdata（只读数据段） */
int        g_init = 42;                    /* 已初始化全局 → .data（初值在镜像里，运行时在 RAM） */
int        g_zero;                         /* 未初始化全局 → .bss（不占文件体积，运行时清零） */
static int g_static_init = 7;              /* 文件内静态 → 同样在 .data */

void some_func(void) { }                   /* 函数体 → .text（代码段，只读可执行） */

static const char *prot_str(DWORD p)
{
    switch (p) {
    case PAGE_READONLY:           return "PAGE_READONLY   只读";
    case PAGE_READWRITE:          return "PAGE_READWRITE  可读写";
    case PAGE_EXECUTE_READ:       return "PAGE_EXECUTE_READ 可执行只读";
    case PAGE_EXECUTE_READWRITE:  return "PAGE_EXECUTE_READWRITE 可执行可写";
    case PAGE_WRITECOPY:          return "PAGE_WRITECOPY  写时复制";
    case PAGE_EXECUTE_WRITECOPY:  return "PAGE_EXECUTE_WRITECOPY";
    default:                      return "其它(见 WinNT.h)";
    }
}

static void show(const char *what, const void *addr, const char *note)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(addr, &mbi, sizeof mbi) == 0) {
        printf("  %-24s %p   查询失败\n", what, addr);
        return;
    }
    printf("  %-24s %p   页属性 = %-38s  %s\n",
           what, addr, prot_str(mbi.Protect), note);
}

int main(void)
{
    int   local = 1;                       /* 局部变量 → 栈 */
    int  *heap  = (int *)malloc(16);       /* 堆 */
    const char *lit = "hello";             /* 字符串字面量 → .rdata */

    printf("==== 各类变量所在地址与内存页保护属性 ====\n");
    show("函数 some_func()",  (const void *)some_func,        "代码段 .text");
    show("const g_const_tab[]", g_const_tab,                  "只读数据 .rdata  ← 对应 MCU 的 Flash");
    show("字符串字面量 \"hello\"", lit,                        "只读数据 .rdata");
    show("全局 g_init(已初始化)", &g_init,                     "数据段 .data      ← 运行时在 RAM");
    show("静态 g_static_init", &g_static_init,                 "数据段 .data");
    show("全局 g_zero(未初始化)", &g_zero,                     "BSS 段 .bss       ← 运行时在 RAM");
    show("局部 local", &local,                                 "栈 stack");
    show("malloc 返回 heap", heap,                             "堆 heap");

    printf("\n==== 栈与堆的生长方向（同一进程内两帧对比）====\n");
    {
        int inner = 2;
        printf("  外层局部变量地址 : %p\n", (void *)&local);
        printf("  内层局部变量地址 : %p   => 栈向下（地址变小）增长:%s\n",
               (void *)&inner, (char *)&inner < (char *)&local ? "是" : "否");
    }
    printf("  堆地址           : %p   => 堆向上（地址变大）增长，与栈相向而行\n", (void *)heap);

    printf("\n==== 关键结论 ====\n");
    printf("  1) const 数据落在只读页：想改它，操作系统直接抛访问违例(0xC0000005)\n");
    printf("  2) 普通全局变量落在可读写页：断电即丢，这就是 RAM\n");
    printf("  3) .bss 的页同样可读写，但它不占可执行文件体积（只有段表记个长度）\n");

    free(heap);
    return 0;
}