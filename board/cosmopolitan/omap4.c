#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/bits.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/clocks.h>
#include <asm/arch/mem.h>
#include <i2c.h>
#include <twl6030.h>
#include <asm/mach-types.h>
#if (CONFIG_COMMANDS & CFG_CMD_NAND) && defined(CFG_NAND_LEGACY)
#include <linux/mtd/nand_legacy.h>
#endif

static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
			  "bne 1b" : "=r" (loops) : "0"(loops));
}

#if 0

void secure_unlock_mem(void)
{
	
	#define UNLOCK_1 0xFFFFFFFF
	#define UNLOCK_2 0x00000000
	#define UNLOCK_3 0x0000FFFF

	__raw_writel(UNLOCK_1, RT_REQ_INFO_PERMISSION_1);
	__raw_writel(UNLOCK_1, RT_READ_PERMISSION_0);
	__raw_writel(UNLOCK_1, RT_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, RT_ADDR_MATCH_1);

	__raw_writel(UNLOCK_3, GPMC_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_3, OCM_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, OCM_ADDR_MATCH_2);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_1);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_1);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_1);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_2);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_2);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_2);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_3);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_3);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_3);

	__raw_writel(UNLOCK_1, SMS_RG_ATT0); 
}

void secureworld_exit(void)
{
	unsigned long i;

	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 2" : "=r" (i));
	
	__asm__ __volatile__("orr %0, %0, #0xC00" : "=r"(i));

	__asm__ __volatile__("orr %0, %0, #0x70000" : "=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 2" : "=r" (i));

	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1" : "=r" (i));
	__asm__ __volatile__("orr %0, %0, #0x40" : "=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1" : "=r" (i));

	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 0" : "=r" (i));
	__asm__ __volatile__("orr %0, %0, #0x31" : "=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 0" : "=r" (i));
}
#endif

int get_boot_type(void);
void v7_flush_dcache_all(int , int);
void setup_auxcr(int , int);

#if 0
void try_unlock_memory(void)
{
	int mode;
	int in_sdram = running_in_sdram();

	mode = get_device_type();
	if (mode == GP_DEVICE)
		secure_unlock_mem();

	if ((mode <= EMU_DEVICE) && (get_boot_type() == 0x1F)
		&& (!in_sdram)) {
		secure_unlock_mem();
		secureworld_exit();
	}

	return;
}
#endif

void s_init(void)
{
	int external_boot = 0;
	int in_sdram = running_in_sdram();

#if	defined(CONFIG_4430VIRTIO)
	in_sdram = 0;  
#endif

	watchdog_init();
	
#if 0
	external_boot = (get_boot_type() == 0x1F) ? 1 : 0;
	
	v7_flush_dcache_all(get_device_type(), external_boot);

	try_unlock_memory();
#endif
#ifndef CONFIG_ICACHE_OFF
	icache_enable();
#endif
	printf("s_init\n");

	if(!in_sdram) {
		printf("s_init mux\n");
		set_muxconf_regs();
		delay(100);
		prcm_init();
	}

#if defined(CONFIG_COSMOPOLITAN)

	set_muxconf_regs();
#endif
}

int misc_init_r(int muic_mode)
{
#ifdef CONFIG_DRIVER_OMAP44XX_I2C
	i2c_init(CFG_I2C_SPEED, CFG_I2C_SLAVE);

	twl6030_set_pcb_version();
	
#endif

	return 0;
}

void wait_for_command_complete(unsigned int wd_base)
{
	int pending = 1;
	do {
		pending = __raw_readl(wd_base + WWPS);
	} while (pending);
}

void watchdog_init(void)
{

	__raw_writel(WD_UNLOCK1, WD2_BASE + WSPR);
	wait_for_command_complete(WD2_BASE);
	__raw_writel(WD_UNLOCK2, WD2_BASE + WSPR);
}

void ether_init(void)
{
#ifdef CONFIG_DRIVER_LAN91C96
	int cnt = 20;

	__raw_writew(0x0, LAN_RESET_REGISTER);
	do {
		__raw_writew(0x1, LAN_RESET_REGISTER);
		udelay(100);
		if (cnt == 0)
			goto h4reset_err_out;
		--cnt;
	} while (__raw_readw(LAN_RESET_REGISTER) != 0x1);

	cnt = 20;

	do {
		__raw_writew(0x0, LAN_RESET_REGISTER);
		udelay(100);
		if (cnt == 0)
			goto h4reset_err_out;
		--cnt;
	} while (__raw_readw(LAN_RESET_REGISTER) != 0x0000);
	udelay(1000);

	*((volatile unsigned char *)ETH_CONTROL_REG) &= ~0x01;
	udelay(1000);

       h4reset_err_out:
	return;
#endif
}

u32 sdram_size(void)
{
	u32 section, i, total_size = 0, size, addr;
	for (i = 0; i < 4; i++) {
		section	= __raw_readl(DMM_LISA_MAP + i*4);
		addr = section & DMM_LISA_MAP_SYS_ADDR_MASK;
		
		if ((addr >= OMAP44XX_DRAM_ADDR_SPACE_START) &&
		    (addr < OMAP44XX_DRAM_ADDR_SPACE_END)) {
			size	= ((section & DMM_LISA_MAP_SYS_SIZE_MASK) >>
				    DMM_LISA_MAP_SYS_SIZE_SHIFT);
			size	= 1 << size;
			size	*= SZ_16M;
			total_size += size;
		}
	}
	return total_size;
}

int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	u32 mtype, btype;

	btype = get_board_type();
	mtype = get_mem_type();

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = sdram_size();

	printf("Load address: 0x%x\n", TEXT_BASE);
	return 0;
}

void update_mux(u32 btype, u32 mtype)
{
	
	return;

}

#if (CONFIG_COMMANDS & CFG_CMD_NAND) && defined(CFG_NAND_LEGACY)

void nand_init(void)
{
	
	return;
}
#endif
