#if defined(CONFIG_MSM_RTB)
#undef __raw_writeb
#undef __raw_writew
#undef __raw_writel
#define __raw_writeb(v, a)     __raw_write_logged((v), (a), b)
#define __raw_writew(v, a)     __raw_write_logged((v), (a), w)
#define __raw_writel(v, a)     __raw_write_logged((v), (a), l)
#undef __raw_readb
#undef __raw_readw
#undef __raw_readl
#define __raw_readb(a)         __raw_read_logged((a), b, char)
#define __raw_readw(a)         __raw_read_logged((a), w, short)
#define __raw_readl(a)         __raw_read_logged((a), l, int)
#endif
