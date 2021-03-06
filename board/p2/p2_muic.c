#include <common.h>
#include <asm/arch/bits.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <i2c.h>

#include <exports.h>
#include <mmc.h>

#include <cosmo_muic.h>

#ifdef CONFIG_LGE_NVDATA
#include <lge_nvdata_emmc.h>
#endif

int cable_910K_detect=0;
#ifdef CONFIG_COSMO_SU760
int cable_56K_detect=0;
int cable_open_usb_detect = 0;
#endif
#ifdef CONFIG_COSMO_SU760
#else
int download_start = 0;
#endif
int dload_mode = 0;

static DP3T_MODE_TYPE dp3t_mode = DP3T_NC;		

int get_dload_mode()
{
	return dload_mode;
}
int get_dp3t_mode(void)
{
	return dp3t_mode;
}

void ifx_vbus_ctrl(int output)
{
	omap_set_gpio_direction(IFX_USB_VBUS_EN_GPIO, 0); 	
	omap_set_gpio_dataout(IFX_USB_VBUS_EN_GPIO, output);
}

void dp3t_switch_ctrl(DP3T_MODE_TYPE mode)
{
	omap_set_gpio_direction(UART_SW1_GPIO, 0);
	omap_set_gpio_direction(UART_SW2_GPIO, 0);

	if (mode == DP3T_S1_AP_UART) {
		ifx_vbus_ctrl(0);
		omap_set_gpio_dataout(UART_SW1_GPIO, 1);
		omap_set_gpio_dataout(UART_SW2_GPIO, 0);
	} else if (mode == DP3T_S2_CP_UART) {
		ifx_vbus_ctrl(0);
		omap_set_gpio_dataout(UART_SW1_GPIO, 0);
		omap_set_gpio_dataout(UART_SW2_GPIO, 1);
	} else if (mode == DP3T_S3_CP_USB) {
		ifx_vbus_ctrl(1);
		omap_set_gpio_dataout(UART_SW1_GPIO, 1);
		omap_set_gpio_dataout(UART_SW2_GPIO, 1);
	} else {
		ifx_vbus_ctrl(0);
		omap_set_gpio_dataout(UART_SW1_GPIO, 0);
		omap_set_gpio_dataout(UART_SW2_GPIO, 0);
	}
	
	dp3t_mode = mode;
}

void usif1_switch_ctrl(USIF1_MODE_TYPE mode)
{

	*((volatile unsigned int *)0x4a100170) &= ~(0x7 << 0);
	*((volatile unsigned int *)0x4a100170) |= (0x3 << 0);

	omap_set_gpio_direction(USIF1_SW_GPIO, 0);
	
	if (mode == USIF1_UART) {
		omap_set_gpio_dataout(USIF1_SW_GPIO, 1);
	} else if (mode == USIF1_IPC) {
		omap_set_gpio_dataout(USIF1_SW_GPIO, 0);
	} else {
	  
	}
}

int muic_i2c_write_byte(unsigned char addr, unsigned char data)
{
	return i2c_write(MUIC_SLAVE_ADDR, (uint) addr, 1, &data, 1);
}

int muic_i2c_read_byte(unsigned char addr, unsigned char *value)
{
	return i2c_read(MUIC_SLAVE_ADDR, (uint) addr, 1, value, 1);
}

#ifdef CONFIG_LGE_NVDATA
void write_muic_mode_of_dload(unsigned char flag)
{
	lge_static_nvdata_emmc_write(LGE_NVDATA_STATIC_910K_DETECT_OFFSET,&flag, 1);
}

int read_muic_mode_of_dload(void)
{
	char flag;

	lge_static_nvdata_emmc_read(LGE_NVDATA_STATIC_910K_DETECT_OFFSET, &flag, 1);
	
	if (flag == 0x01)
		return 1;
	else
		return 0;
}

void write_muic_retain_mode(unsigned char flag)
{
	lge_dynamic_nvdata_emmc_write(LGE_NVDATA_DYNAMIC_MUIC_RETENTION_OFFSET, &flag, 1);
}

int read_muic_retain_mode(void)
{
	char flag;

	lge_dynamic_nvdata_emmc_read(LGE_NVDATA_DYNAMIC_MUIC_RETENTION_OFFSET, &flag, 1);

	if (flag == 0x02)
		return RETAIN_AP_USB;
	else if (flag == 0x03)
		return RETAIN_CP_USB;
	else if (flag == 0x00)
		return NO_RETAIN;
	else
		return flag;
}
#else
void write_muic_mode_of_dload(unsigned char flag) {}
int read_muic_mode_of_dload(void) { return 0; }
void write_muic_retain_mode(unsigned char flag) {}
int read_muic_retain_mode(void) { return NO_RETAIN; }
#endif

static void check_charging_mode(MUIC_MODE_TYPE *mode)
{
	s32 i2c_ret;
	unsigned char value;
	
	i2c_ret = muic_i2c_read_byte(INT_STATUS, &value);
	if (value & VBUS) {
		if ((value & IDNO) == 0x02 || 
			(value & IDNO) == 0x04 ||
			(value & IDNO) == 0x09 ||
			(value & IDNO) == 0x0a)
			*mode = MUIC_FACTORY_MODE;
		else if (value & CHGDET) 
			*mode = MUIC_CHRGER;
		else
			*mode = MUIC_USB;
	}
}

static MUIC_MODE_TYPE muic_device_ts5usba33402(int _isFactoryRest, char *cmd)
{
	unsigned char i2c_ret;
	unsigned char int_status_val;	

	MUIC_MODE_TYPE muic_mode = MUIC_OPEN;

	unsigned char status_val;

	printf("start muic_device_ti\n");

	muic_i2c_write_byte(CONTROL_1, ID_200 | SEMREN);
	muic_i2c_write_byte(CONTROL_2, CHG_TYP);

	udelay(250000);

	muic_i2c_write_byte(CONTROL_2, INT_EN | CP_AUD | CHG_TYP);
	udelay(500000);

	usif1_switch_ctrl(USIF1_UART);
			
	i2c_ret = muic_i2c_read_byte(INT_STATUS, &int_status_val);
	udelay(1000);  

	if ((int_status_val & VBUS) != 0) {

		if((int_status_val & CHGDET) != 0){
			muic_i2c_write_byte(SW_CONTROL, OPEN);
			muic_mode = MUIC_CHRGER;
			printf("Charger Detected\n");
		}
		else{
			if ((int_status_val & IDNO) == 0x02) {
				udelay(50000); 
				
				if (_isFactoryRest) {
					muic_i2c_write_byte(CONTROL_1, ID_2P2 | SEMREN);
					muic_i2c_write_byte(SW_CONTROL, UART);		
					dp3t_switch_ctrl(DP3T_S2_CP_UART);			
					muic_mode = MUIC_FACTORY_MODE;

					strcat(cmd, " muic=1");
					setenv("bootargs", cmd);
					
					printf("[MUIC] first boot, path = CP UART\n");
				} else {
					
					muic_i2c_write_byte(CONTROL_1,ID_2P2 | SEMREN);
					muic_i2c_write_byte(SW_CONTROL, UART);
					dp3t_switch_ctrl(DP3T_S3_CP_USB);
					muic_mode = MUIC_DEVELOP_MODE;
					printf("CP_USB(56K)\n");
				}
			} else if ((int_status_val & IDNO) == 0x04) {
				
				muic_i2c_write_byte(CONTROL_1, ID_2P2 | SEMREN);
				muic_i2c_write_byte(SW_CONTROL, UART);		
				dp3t_switch_ctrl(DP3T_S2_CP_UART);			
				muic_mode = MUIC_FACTORY_MODE;
				printf("CP_UART(130K)\n");
			} else if ((int_status_val & IDNO) == 0x0a || (int_status_val & IDNO) == 0x09) {
				
				mmc_init(1);

				cable_910K_detect = 1;
				
				dload_mode = read_muic_mode_of_dload();
		
				if (dload_mode == NORMAL_MODE) {
					muic_i2c_write_byte(CONTROL_1, ID_200 | SEMREN | CP_EN);
					muic_i2c_write_byte(SW_CONTROL, USB);
					muic_mode = MUIC_FACTORY_MODE;
					printf("AP_USB is set(910Kohm)\n");
				} else {
					muic_i2c_write_byte(SW_CONTROL, OPEN);
					dp3t_switch_ctrl(DP3T_NC);
					write_muic_mode_of_dload(NORMAL_MODE);
					muic_mode = MUIC_FACTORY_MODE;
					printf("CP_USB is set(910Kohm)\n");
				}
			} else {
					
					muic_i2c_write_byte(CONTROL_1, ID_2P2 | SEMREN | CP_EN);
					muic_i2c_write_byte(SW_CONTROL, USB);		
					
					muic_mode = MUIC_USB;
					printf("AP_USB\n");
					printf("### USB charger may not have enough power to boot up depends on your board condition..... ###\n");
					printf("### You'd better to use TA or PIF for power supply. ###\n");
			}
		}
	} else {
		
		if ((int_status_val & IDNO) == 0x02) {
			muic_i2c_write_byte(SW_CONTROL, UART);		
			dp3t_switch_ctrl(DP3T_S1_AP_UART);			
			
			printf("AP_UART\n");
		} else if ((int_status_val & IDNO) == 0x04) {
			
			muic_i2c_write_byte(CONTROL_1, ID_2P2| SEMREN);
			muic_i2c_write_byte(SW_CONTROL, UART);		
			dp3t_switch_ctrl(DP3T_S2_CP_UART);			
			printf("CP_UART\n");
			
		} else {
			muic_i2c_write_byte(SW_CONTROL, OPEN);	
			dp3t_switch_ctrl(DP3T_NC);
			muic_mode = MUIC_OPEN;
			printf("MUIC opened\n");
		}
	} 

	udelay(1000);		
	printf("INT_STAT = 0x%x\n", int_status_val);

	return muic_mode;
}

static MUIC_MODE_TYPE muic_device_max14526(int _isFactoryRest, char *cmd)
{
	unsigned char i2c_ret;
	unsigned char status_val;			
	unsigned char int_status_val;		

	MUIC_MODE_TYPE muic_mode = MUIC_OPEN;

	i2c_ret = muic_i2c_write_byte(CONTROL_1, ID_200 | ADC_EN);	
	i2c_ret = muic_i2c_write_byte(CONTROL_2, 0x00); 		

	usif1_switch_ctrl(USIF1_UART);

	udelay(70000);		

	i2c_ret = muic_i2c_read_byte(INT_STATUS, &int_status_val);
	printf("INT_STAT = 0x%x\n", int_status_val);

	if ((int_status_val & VBUS) != 0) {
		
		if ((int_status_val & IDNO) == 0x02) {
#ifdef CONFIG_COSMO_SU760
			cable_56K_detect = 1;
#endif
			if (_isFactoryRest) {
				muic_i2c_write_byte(SW_CONTROL, UART);		
				dp3t_switch_ctrl(DP3T_S2_CP_UART);			
				muic_mode = MUIC_FACTORY_MODE;

				strcat(cmd, " muic=1");
				setenv("bootargs", cmd);
				
				printf("[MUIC] first boot, path = CP UART\n");
			} else {
				
				muic_i2c_write_byte(SW_CONTROL, OPEN);
				dp3t_switch_ctrl(DP3T_NC);
				muic_mode = MUIC_FACTORY_MODE;
				printf("CP_USB\n");
			}
		} else if ((int_status_val & IDNO) == 0x04) {
			
			muic_i2c_write_byte(SW_CONTROL, UART);		
			dp3t_switch_ctrl(DP3T_S2_CP_UART);			
			muic_mode = MUIC_FACTORY_MODE;
			printf("CP_UART\n");
		} else if ((int_status_val & IDNO) == 0x0a || (int_status_val & IDNO) == 0x09) {
			
			mmc_init(1);

			cable_910K_detect = 1;
			dload_mode = read_muic_mode_of_dload();
	
			if (dload_mode == NORMAL_MODE) {
				muic_i2c_write_byte(CONTROL_1, ID_200 | ADC_EN | CP_EN);
				muic_i2c_write_byte(SW_CONTROL, USB);
				muic_mode = MUIC_FACTORY_MODE;
				printf("AP_USB is set(910Kohm)\n");
			} else {
				muic_i2c_write_byte(SW_CONTROL, OPEN);
				dp3t_switch_ctrl(DP3T_NC);
				write_muic_mode_of_dload(NORMAL_MODE);
				muic_mode = MUIC_FACTORY_MODE;
				printf("CP_USB is set(910Kohm)\n");
			}
		}else {
			
			muic_i2c_write_byte(SW_CONTROL, COMP2_TO_HZ | COMN1_TO_C1COMP);
			udelay(2000); 
			i2c_ret = muic_i2c_read_byte(STATUS, &status_val);
			printf("STATUS = 0x%x\n", status_val);

			if (status_val & C1COMP) {
				
				muic_i2c_write_byte(SW_CONTROL, OPEN);
				dp3t_switch_ctrl(DP3T_NC);
				muic_mode = MUIC_CHRGER;
				printf("Charger Detected\n");
			} else {
				muic_i2c_write_byte(CONTROL_1, ID_200 | ADC_EN | CP_EN);
				muic_i2c_write_byte(SW_CONTROL, USB);		
				dp3t_switch_ctrl(DP3T_NC);
				
#ifdef CONFIG_COSMO_SU760
				cable_open_usb_detect = 1;
#endif
				muic_mode = MUIC_USB;

				printf("\n### USB charger may not have enough power to boot up depends on your board condition..... ###\n");
				printf("\n### You'd better to use TA or PIF for power supply. ###\n");
				printf("[MUIC] AP USB\n");
			}
		}	
	} else {
		
		if ((int_status_val & IDNO) == 0x02) {
			muic_i2c_write_byte(CONTROL_1, ID_200 | ADC_EN | CP_EN);
			muic_i2c_write_byte(SW_CONTROL, UART);		
			dp3t_switch_ctrl(DP3T_S1_AP_UART);			
			
			printf("AP_UART\n");
		} else if ((int_status_val & IDNO) == 0x04) {
			
			muic_i2c_write_byte(CONTROL_1, ID_200 | ADC_EN);
			muic_i2c_write_byte(SW_CONTROL, UART);		
			dp3t_switch_ctrl(DP3T_S2_CP_UART);			
			printf("CP_UART\n");
			
		} else {
			muic_i2c_write_byte(SW_CONTROL, OPEN);	
			dp3t_switch_ctrl(DP3T_NC);
			muic_mode = MUIC_OPEN;
			printf("MUIC opened\n");
		}
	}	

	printf("INT_STAT = 0x%x\n", int_status_val);

	return muic_mode;
}

int muic_init(int isFactoryReset)
{
	unsigned int retain_mode = NO_RETAIN;
			
	unsigned char i2c_ret;
	
	unsigned char reg_val;			
	unsigned char muic_device;		
	
	MUIC_MODE_TYPE muic_mode = MUIC_OPEN;

	char cmd_line[256];

	unsigned char iloop = 5;

	strcpy(cmd_line, getenv("bootargs"));

#if defined(CONFIG_COSMO_EVB_B)  || defined(CONFIG_COSMO_EVB_C)
	printf("muic chip : I2C4\n");
	select_bus(I2C4 , OMAP_I2C_STANDARD);
#else
	printf("muic chip : I2C3\n");
	select_bus(I2C3 , OMAP_I2C_STANDARD);
#endif 

	i2c_init(OMAP_I2C_STANDARD, MUIC_SLAVE_ADDR);

retry_read_byte:

	i2c_ret = muic_i2c_read_byte(DEVICE_ID, &reg_val);
	printf("reg_val = 0x%x\n", reg_val);
	if ((reg_val & 0xf0) == 0x10) 
		muic_device = TS5USBA33402;		
	else if ((reg_val & 0xf0) == 0x20)
		muic_device = MAX14526;			
	else if ((reg_val & 0xf0) == ANY_VANDOR_ID)
		muic_device = ANY_VANDOR;

	else{
		if(iloop <= 0){
			printf("muic_init muic vendor check fail. unlimit looping \n", iloop);
			while(1);
		}
		else{
			printf("muic_init muic vendor check loop = %d\n", iloop);
			udelay(250000);

			iloop--;
			goto retry_read_byte;
		}
	}

	if (downloadkey()){
		write_muic_retain_mode(NO_RETAIN);
		printf("[MUIC] No retain\n");
	} 

	retain_mode = read_muic_retain_mode();
	
	printf("muic_init muic vendor : %d, reg_val:0x%x, retain_mode = %d\n", muic_device, reg_val, retain_mode);

	if (muic_device == TS5USBA33402) {
		printf("muic chip : TS5USBA33402\n");
		if (retain_mode == RETAIN_AP_USB) {
			muic_i2c_write_byte(CONTROL_1, ID_200 | SEMREN | CP_EN);
			muic_i2c_write_byte(SW_CONTROL, USB);		
			
			muic_mode = MUIC_USB;
			check_charging_mode(&muic_mode);

			printf("[MUIC] retain AP_USB\n");
			printf("\n###[MUIC] USB charger may not have enough power to boot up depends on your board condition..... ###\n");
			printf("\n###[MUIC] You'd better to use TA or PIF for power supply. ###\n");

			strcat(cmd_line, " muic=2");
			setenv("bootargs", cmd_line);
		} else if (retain_mode == RETAIN_CP_USB) {
			muic_i2c_write_byte(SW_CONTROL, OPEN);
			dp3t_switch_ctrl(DP3T_NC);
			muic_mode = MUIC_USB;
			check_charging_mode(&muic_mode);

			printf("[MUIC] retain CP_USB\n");

			strcat(cmd_line, " muic=3");
			setenv("bootargs", cmd_line);
		} else
			muic_mode = muic_device_ts5usba33402(isFactoryReset, cmd_line);
		
	} else if (muic_device == MAX14526) {
		printf("muic chip : MAX14526\n");
		if (retain_mode == RETAIN_AP_USB) {
			muic_i2c_write_byte(CONTROL_1, ID_200 | ADC_EN | CP_EN);
			muic_i2c_write_byte(SW_CONTROL, USB);		
			
			muic_mode = MUIC_USB;
			check_charging_mode(&muic_mode);
			
			printf("[MUIC] retain AP_USB\n");
			printf("\n###[MUIC] USB charger may not have enough power to boot up depends on your board condition..... ###\n");
			printf("\n###[MUIC] You'd better to use TA or PIF for power supply. ###\n");

			strcat(cmd_line, " muic=2");
			setenv("bootargs", cmd_line);
		} else if (retain_mode == RETAIN_CP_USB) {
			muic_i2c_write_byte(SW_CONTROL, OPEN);
			dp3t_switch_ctrl(DP3T_NC);
			muic_mode = MUIC_USB;
			check_charging_mode(&muic_mode);
			
			printf("[MUIC] retain CP_USB\n");

			strcat(cmd_line, " muic=3");
			setenv("bootargs", cmd_line);
		} else
			muic_mode = muic_device_max14526(isFactoryReset, cmd_line);
	} else
		printf("[MUIC] ANY VENDOR, Not supported\n");

	select_bus(I2C1, OMAP_I2C_STANDARD); 

	return muic_mode;
}

void muic_for_download(int mode)
{
	int ret;
if(mode == 0)
{
	select_bus(I2C3 , OMAP_I2C_STANDARD);

	usif1_switch_ctrl(USIF1_IPC);
	
		dp3t_switch_ctrl(2);

	ret = muic_i2c_write_byte(SW_CONTROL, DP_UART | DM_UART);
}
else
		dp3t_switch_ctrl(DP3T_S3_CP_USB);
	printf("CP_USB is set(910Kohm)\n");

}

void muic_AP_USB(void)
{
	
	select_bus(I2C3 , OMAP_I2C_STANDARD);

	muic_i2c_write_byte(CONTROL_1, ID_200 | SEMREN | CP_EN);
	muic_i2c_write_byte(SW_CONTROL, USB);		
	
	printf("AP_USB\n");

	select_bus(I2C1, OMAP_I2C_STANDARD);
}
