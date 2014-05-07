

static int rk30_vmac_register_set(void)
{
	//config rk30 vmac as rmii
	writel_relaxed(0x3 << 16 | 0x2, RK30_GRF_BASE + GRF_SOC_CON1);
	int val = readl_relaxed(RK30_GRF_BASE + GRF_IO_CON3);
	writel_relaxed(val | 0xf << 16 | 0xf, RK30_GRF_BASE + GRF_IO_CON3);
	return 0;
}

static int rk30_rmii_io_init(void)
{
	struct regulator *ldo_33 = NULL;

	iomux_set(RMII_TXEN);
	iomux_set(RMII_TXD1);
	iomux_set(RMII_TXD0);
	iomux_set(RMII_RXD0);
	iomux_set(RMII_RXD1);
#if defined (CONFIG_RK29_VMAC_EXT_CLK)
	iomux_set(RMII_CLKIN);
#else
	iomux_set(RMII_CLKOUT);
#endif
	iomux_set(RMII_RXERR);
	iomux_set(RMII_CRS);
	iomux_set(RMII_MD);
	iomux_set(RMII_MDCLK);
	iomux_set(GPIO3_D2);

	gpio_request(RK30_PIN3_PD2, "rmii_int");
	gpio_direction_input(RK30_PIN3_PD2);

#if defined (CONFIG_REGULATOR_ACT8846)
	if (pmic_is_act8846())
		ldo_33 = regulator_get(NULL, "act_ldo5");
#endif
#if defined (CONFIG_MFD_RK808)
	if (pmic_is_rk808())
		ldo_33 = regulator_get(NULL, "rk_ldo2");
#endif
	if (ldo_33 == NULL)
		return 0;
	if (regulator_is_enabled(ldo_33) > 0)
		regulator_disable(ldo_33);
	regulator_put(ldo_33);

	return 0;
}

static int rk30_rmii_io_deinit(void)
{
	struct regulator *ldo_33 = NULL;

	gpio_free(RK30_PIN3_PD2);

#if defined (CONFIG_REGULATOR_ACT8846)
	if(pmic_is_act8846())
		ldo_33 = regulator_get(NULL, "act_ldo5");
#endif
#if defined (CONFIG_MFD_RK808)
	if(pmic_is_rk808())
		ldo_33 = regulator_get(NULL, "rk_ldo2");
#endif
	if (ldo_33 == NULL)
		return 0;
	if (regulator_is_enabled(ldo_33) > 0)
		regulator_disable(ldo_33);
	regulator_put(ldo_33);

	return 0;
}

static int rk30_rmii_power_control(int enable)
{
	struct regulator *ldo_33 = NULL;

#if defined (CONFIG_REGULATOR_ACT8846)
	if(pmic_is_act8846())
		ldo_33 = regulator_get(NULL, "act_ldo5");
#endif
#if defined (CONFIG_MFD_RK808)
	if(pmic_is_rk808())
		ldo_33 = regulator_get(NULL, "rk_ldo2");
#endif
	if (ldo_33 == NULL)
		return 0;

	if (enable) {
		//enable phy power
		printk("power on phy\n");

		if (regulator_is_enabled(ldo_33) == 0)
			regulator_enable(ldo_33);
	} else {
		if (regulator_is_enabled(ldo_33) > 0)
			regulator_disable(ldo_33);
	}

	regulator_put(ldo_33);

	return 0;
}

#define BIT_EMAC_SPEED      (1 << 1)
static int rk29_vmac_speed_switch(int speed)
{
	//printk("%s--speed=%d\n", __FUNCTION__, speed);
	if (10 == speed) {
	    writel_relaxed(readl_relaxed(RK30_GRF_BASE + GRF_SOC_CON1) & (~BIT_EMAC_SPEED) | (BIT_EMAC_SPEED << 16), RK30_GRF_BASE + GRF_SOC_CON1);
	} else {
	    writel_relaxed(readl_relaxed(RK30_GRF_BASE + GRF_SOC_CON1) | ( BIT_EMAC_SPEED) | (BIT_EMAC_SPEED << 16), RK30_GRF_BASE + GRF_SOC_CON1);
	}
}

struct rk29_vmac_platform_data board_vmac_data = {
	.vmac_register_set = rk30_vmac_register_set,
	.rmii_io_init = rk30_rmii_io_init,
	.rmii_io_deinit = rk30_rmii_io_deinit,
	.rmii_power_control = rk30_rmii_power_control,
	.rmii_speed_switch = rk29_vmac_speed_switch,
};
