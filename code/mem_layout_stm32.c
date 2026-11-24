/*
 * mem_layout_stm32.c —— STM32F103：什么住在 Flash，什么住在 SRAM，什么根本不是"存储器"
 *
 * 用途 : 裸寄存器写法，Keil MDK (AC5 / AC6) 可直接加进工程编译
 *       分区判定函数可用 -DHOST_TEST 在 PC 上单独验证（见文件末尾）
 *
 * 记住一条主线：
 *   烧录时  —— 代码 + const 数据 + 全局变量的"初值"一起进 Flash
 *   上电时  —— 启动代码把"初值"搬进 SRAM，把 .bss 清零，然后才跳 main
 *   运行时  —— SRAM 里的东西随便改；Flash 里的东西改之前必须先擦、且按页擦
 */
#include <stdint.h>

/* ============================================================
 * 1. 烧进 Flash 的：const 量 + 代码本身
 *    地址在 0x0800_0000 之后，掉电不丢，运行中不可直接写
 * ============================================================ */
const uint8_t  seg7_map[10] = {           /* 数码管段码表：全生命周期只读 */
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};
const char     fw_ver[]     = "v1.0.3";  /* 字符串常量，同样在 Flash */
const int16_t  sin_lut[8]   = { 0, 707, 1000, 707, 0, -707, -1000, -707 }; /* 查表省算力 */

/* ============================================================
 * 2. 有初值的全局 → .data
 *    注意：它的"初值"另存一份在 Flash 里，上电时由启动代码拷到 SRAM
 * ============================================================ */
volatile uint32_t g_tick  = 0;      /* 显式写了 = 0，编译器仍归 .data */
uint16_t          g_cal_k = 1024;

/* ============================================================
 * 3. 无初值的全局 → .bss
 *    只在段表里记长度，不占 Flash 体积；上电时整段清零
 * ============================================================ */
uint16_t g_dma_buf[512];            /* 1KB，全在 SRAM 里 */
uint32_t g_err_cnt;

/* ============================================================
 * 4. 外设寄存器：不是"存储器"，但被映射进了地址空间
 *    所以能用指针访问 —— 这就是"内存映射 I/O"
 *    必须 volatile：否则编译器会把连续两次读优化成一次
 * ============================================================ */
#define RCC_APB2ENR  (*(volatile uint32_t *)0x40021018UL)
#define GPIOC_CRH    (*(volatile uint32_t *)0x40011004UL)
#define GPIOC_ODR    (*(volatile uint32_t *)0x4001100CUL)

void led_pc13_init(void)
{
    RCC_APB2ENR |= (1UL << 4);            /* 先开 GPIOC 时钟（bit4 = IOPCEN） */
    GPIOC_CRH   &= ~(0xFUL << 20);        /* PC13 在 CRH 的第 (13-8)=5 个 4 位组 */
    GPIOC_CRH   |=  (0x3UL << 20);        /* CNF=00 推挽 + MODE=11 50MHz */
}

void led_pc13_set(int on)
{
    /* 用 BSRR 更规范，这里为演示 ODR 直写 */
    if (on) GPIOC_ODR &= ~(1UL << 13);    /* PC13 低电平点亮（板上 LED 通常接 VCC） */
    else    GPIOC_ODR |=  (1UL << 13);
}

/* ============================================================
 * 5. 分区判定：纯整数比较，不碰硬件 —— PC 上也能跑，可做单元验证
 * ============================================================ */
typedef struct {
    const char *name;
    uintptr_t   base;
    uintptr_t   end;      /* 开区间上界 */
} region_t;

static const region_t REGIONS[] = {
    /* 名字用纯 ASCII：armcc 默认按本地代码页读源文件，中文字符串常量会乱码 */
    { "Flash  (ROM role)", 0x08000000UL, 0x08010000UL },  /* F103C8T6: 64KB */
    { "SRAM   (RAM role)", 0x20000000UL, 0x20005000UL },  /* F103C8T6: 20KB */
    { "PERIPH MMIO      ", 0x40000000UL, 0x40024000UL },  /* APB1/APB2 一段 */
    { "CORE PRIVATE     ", 0xE0000000UL, 0xE0100000UL },  /* NVIC / SCB / SysTick */
};

const char *region_of(uintptr_t addr)
{
    unsigned i;
    for (i = 0; i < sizeof(REGIONS) / sizeof(REGIONS[0]); i++) {
        if (addr >= REGIONS[i].base && addr < REGIONS[i].end)
            return REGIONS[i].name;
    }
    return "UNKNOWN";
}

/* ============================================================
 * 6. 反例：往 Flash 地址直接赋值会怎样
 *    不会"悄悄改掉"，也不会像 PC 那样返回一个错误码 —— 是总线错误 → HardFault
 *    （Flash 控制器同时会在 FLASH_SR 里置 PGERR / WRPRTERR 标志，位定义见 RM0008）
 *    另：Flash 只能"整页擦除后写"，不支持像 SRAM 那样按字节改
 * ============================================================ */
#if 0
void never_do_this(void)
{
    *(volatile uint8_t *)0x08000000UL = 0x00;   /* 触发 HardFault */
}
#endif

/* ============================================================
 * PC 侧验证（gcc -DHOST_TEST mem_layout_stm32.c）
 * ============================================================ */
#ifdef HOST_TEST
#include <stdio.h>

int main(void)
{
    const uintptr_t cases[] = {
        0x08000000UL,   /* Flash 首地址 */
        0x0800FFFFUL,   /* Flash 最后一个字节 */
        0x08010000UL,   /* 刚越过 Flash 末尾 —— 应判未知 */
        0x20000000UL,   /* SRAM 首地址 */
        0x20004FFFUL,   /* SRAM 最后一字节 */
        0x4001080CUL,   /* GPIOA_ODR */
        0xE000E010UL,   /* SysTick */
    };
    unsigned i;
    printf("addr         -> region\n");
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
        printf("0x%08lX   -> %s\n", (unsigned long)cases[i], region_of(cases[i]));
    return 0;
}
#endif
