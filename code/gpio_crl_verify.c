/*
 * gpio_crl_verify.c —— 在 PC 上验证 STM32F1 的 CRL 位域算法（不碰硬件）
 * 编译: gcc -O2 -Wall -o gpio_crl_verify gpio_crl_verify.c
 * 目的: 烧板子之前先确认"PA0 配成推挽 50MHz 到底该写哪 4 位"
 */
#include <stdio.h>
#include <stdint.h>

/* 一个引脚在 CRL/CRH 里占 4 bit: [3:2]=CNF, [1:0]=MODE */
static uint32_t nibble(int cnf, int mode)
{
    return ((uint32_t)(cnf & 0x3u) << 2) | (uint32_t)(mode & 0x3u);
}

int main(void)
{
    /* 复位值 0x44444444: 每个引脚 = 0b0100 = CNF 01 + MODE 00 = 浮空输入 */
    uint32_t crl = 0x44444444u;

    /* 第 1 步: 清掉 PA0 那 4 位（掩码必须是 0xF，不是 0x3） */
    crl &= ~(0xFu << 0);
    /* 第 2 步: 写入 CNF=00(通用推挽) + MODE=11(50MHz) */
    crl |= nibble(0, 3) << 0;
    printf("PA0 推挽50MHz : CRL = 0x%08X, PA0 = 0x%X\n", crl, (crl >> 0) & 0xF);

    /* 反例: 只清 2 位（漏了 CNF），CNF 里的旧值会残留 */
    uint32_t bad = 0x44444444u;
    bad &= ~(0x3u << 0);
    bad |= nibble(0, 3) << 0;
    printf("只清低2位的后果: CRL = 0x%08X, PA0 = 0x%X (=%s)\n",
           bad, (bad >> 0) & 0xF,
           ((bad >> 0) & 0xF) == 0x3 ? "推挽50MHz" : "不是推挽(错)");

    /* 顺手把 PA8 配成开漏 2MHz: PA8 在 CRH 的第 0 个 nibble */
    uint32_t crh = 0x44444444u;
    crh &= ~(0xFu << 0);
    crh |= nibble(1, 2) << 0;
    printf("PA8 开漏2MHz  : CRH = 0x%08X, PA8 = 0x%X\n", crh, (crh >> 0) & 0xF);

    return 0;
}