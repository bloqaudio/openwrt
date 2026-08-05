// SPDX-License-Identifier: GPL-2.0-only
/*
 * Realtek thermal sensor driver
 *
 * Copyright (C) 2025 Bjørn Mork <bjorn@mork.no>>
 */

#include <linux/bitfield.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include "thermal_hwmon.h"

#define RTL8380_THERMAL_METER_CTRL0	0x98
#define RTL8380_THERMAL_METER_CTRL1	0x9c
#define RTL8380_THERMAL_METER_CTRL2	0xa0
#define RTL8380_THERMAL_METER_RESULT	0xa4
#define RTL8380_TM_ENABLE		BIT(0)
#define RTL8380_TEMP_VALID		BIT(8)
#define RTL8380_TEMP_OUT_MASK		GENMASK(6, 0)

#define RTL8390_THERMAL_METER0_CTRL0	0x274
#define RTL8390_THERMAL_METER0_CTRL1	0x278
#define RTL8390_THERMAL_METER0_CTRL2	0x27c
#define RTL8390_THERMAL_METER0_RESULT	0x280
#define RTL8390_THERMAL_METER1_CTRL0	0x284
#define RTL8390_THERMAL_METER1_CTRL1	0x288
#define RTL8390_THERMAL_METER1_CTRL2	0x28c
#define RTL8390_THERMAL_METER1_RESULT	0x290
#define RTL8390_TM_ENABLE		BIT(0)
#define RTL8390_TEMP_VALID		BIT(8)
#define RTL8390_TEMP_OUT_MASK		GENMASK(6, 0)

#define RTL9300_THERMAL_METER_CTRL0	0x60
#define RTL9300_THERMAL_METER_CTRL1	0x64
#define RTL9300_THERMAL_METER_CTRL2	0x68
#define RTL9300_THERMAL_METER_RESULT0	0x6c
#define RTL9300_THERMAL_METER_RESULT1	0x70
#define RTL9300_TM_ENABLE		BIT(16)
#define RTL9300_TEMP_VALID		BIT(24)
#define RTL9300_TEMP_OUT_MASK		GENMASK(23, 16)
#define RTL9300_SAMPLE_DLY_SHIFT	(16)
#define RTL9300_SAMPLE_DLY_MASK		GENMASK(RTL9300_SAMPLE_DLY_SHIFT + 15, RTL9300_SAMPLE_DLY_SHIFT)
#define RTL9300_COMPARE_DLY_SHIFT	(0)
#define RTL9300_COMPARE_DLY_MASK	GENMASK(RTL9300_COMPARE_DLY_SHIFT + 15, RTL9300_COMPARE_DLY_SHIFT)

#define RTL9310_TM0_CTRL0		0x80
#define RTL9310_TM0_CTRL1		0x84
#define RTL9310_TM0_CTRL2		0x88
#define RTL9310_TM0_CTRL4		0x90
#define RTL9310_TM0_RESULT0		0xa0
#define RTL9310_TM0_RESULT_LATCH	0xb0
#define RTL9310_TM_RSTB			BIT(31)
#define RTL9310_TM_DIGT_ORDER_SEL	BIT(28)
#define RTL9310_TM_REG_A		0x7467000
#define RTL9310_TM_REG_B_MASK		GENMASK(31, 10)
#define RTL9310_TM_REG_B		0x380fff
#define RTL9310_TM_ADC_OSR_SEL_MASK	GENMASK(2, 0)
#define RTL9310_TM_ADC_OSR_SEL		1
#define RTL9310_TM_CT_OUT_MASK		GENMASK(18, 0)
#define RTL9310_TM_CT_SIGN		BIT(18)

struct realtek_thermal_priv {
	struct regmap *regmap;
	bool enabled;
};

static void rtl8380_thermal_init(struct realtek_thermal_priv *priv)
{
	priv->enabled = !regmap_update_bits(priv->regmap, RTL8380_THERMAL_METER_CTRL0, RTL8380_TM_ENABLE, RTL8380_TM_ENABLE);
}

static int rtl8380_get_temp(struct thermal_zone_device *tz, int *res)
{
	struct realtek_thermal_priv *priv = thermal_zone_device_priv(tz);
	int offset = thermal_zone_get_offset(tz);
	int slope = thermal_zone_get_slope(tz);
	u32 val;
	int ret;

	if (!priv->enabled)
		rtl8380_thermal_init(priv);

	ret = regmap_read(priv->regmap, RTL8380_THERMAL_METER_RESULT, &val);
	if (ret)
		return ret;

	if (!(val & RTL8380_TEMP_VALID))
		return -EAGAIN;

	*res = FIELD_GET(RTL8380_TEMP_OUT_MASK, val) * slope + offset;
	return 0;
}

static const struct thermal_zone_device_ops rtl8380_ops = {
	.get_temp = rtl8380_get_temp,
};

static void rtl8390_thermal_init(struct realtek_thermal_priv *priv)
{
	priv->enabled = !regmap_update_bits(priv->regmap, RTL8390_THERMAL_METER0_CTRL0, RTL8390_TM_ENABLE, RTL8390_TM_ENABLE);
}

static int rtl8390_get_temp(struct thermal_zone_device *tz, int *res)
{
	struct realtek_thermal_priv *priv = thermal_zone_device_priv(tz);
	int offset = thermal_zone_get_offset(tz);
	int slope = thermal_zone_get_slope(tz);
	u32 val;
	int ret;

	if (!priv->enabled)
		rtl8390_thermal_init(priv);
	/* assume sensor0 is the CPU, both sensor0 & sensor1 report same values +/- 1 degree C */
	ret = regmap_read(priv->regmap, RTL8390_THERMAL_METER0_RESULT, &val);
	if (ret)
		return ret;

	if (!(val & RTL8390_TEMP_VALID))
		return -EAGAIN;

	*res = FIELD_GET(RTL8390_TEMP_OUT_MASK, val) * slope + offset;
	return 0;
}

static const struct thermal_zone_device_ops rtl8390_ops = {
	.get_temp = rtl8390_get_temp,
};

static void rtl9300_thermal_init(struct realtek_thermal_priv *priv)
{
	/* increasing sample delay makes get_temp() succeed more often */
	regmap_update_bits(priv->regmap, RTL9300_THERMAL_METER_CTRL1, RTL9300_SAMPLE_DLY_MASK, 0x0800 << RTL9300_SAMPLE_DLY_SHIFT);
	priv->enabled = !regmap_update_bits(priv->regmap, RTL9300_THERMAL_METER_CTRL2, RTL9300_TM_ENABLE, RTL9300_TM_ENABLE);
}

static int rtl9300_get_temp(struct thermal_zone_device *tz, int *res)
{
	struct realtek_thermal_priv *priv = thermal_zone_device_priv(tz);
	int offset = thermal_zone_get_offset(tz);
	int slope = thermal_zone_get_slope(tz);
	u32 val;
	int ret;

	if (!priv->enabled)
		rtl9300_thermal_init(priv);

	ret = regmap_read(priv->regmap, RTL9300_THERMAL_METER_RESULT0, &val);
	if (ret)
		return ret;
	if (!(val & RTL9300_TEMP_VALID))
		return -EAGAIN;

	*res = FIELD_GET(RTL9300_TEMP_OUT_MASK, val) * slope + offset;
	return 0;
}

static const struct thermal_zone_device_ops rtl9300_ops = {
	.get_temp = rtl9300_get_temp,
};

/*
 * The RTL9310 thermal meter is a sigma-delta ADC whose calibration
 * constants and mode bits are not usable at their reset values (the
 * result register reads zero). The values below are the vendor's
 * "current temperature" configuration; the result is in units of
 * 1/1024 degree C with a sign flag in bit 18.
 */
static void rtl9310_thermal_init(struct realtek_thermal_priv *priv)
{
	int ret;

	ret = regmap_write(priv->regmap, RTL9310_TM0_RESULT_LATCH, 1);
	ret |= regmap_write(priv->regmap, RTL9310_TM0_CTRL0, RTL9310_TM_REG_A);
	ret |= regmap_update_bits(priv->regmap, RTL9310_TM0_CTRL1,
				  RTL9310_TM_REG_B_MASK | RTL9310_TM_ADC_OSR_SEL_MASK,
				  FIELD_PREP(RTL9310_TM_REG_B_MASK, RTL9310_TM_REG_B) |
				  FIELD_PREP(RTL9310_TM_ADC_OSR_SEL_MASK, RTL9310_TM_ADC_OSR_SEL));
	ret |= regmap_update_bits(priv->regmap, RTL9310_TM0_CTRL2,
				  RTL9310_TM_DIGT_ORDER_SEL, RTL9310_TM_DIGT_ORDER_SEL);
	ret |= regmap_update_bits(priv->regmap, RTL9310_TM0_CTRL4,
				  RTL9310_TM_RSTB, RTL9310_TM_RSTB);

	priv->enabled = !ret;
}

static int rtl9310_get_temp(struct thermal_zone_device *tz, int *res)
{
	struct realtek_thermal_priv *priv = thermal_zone_device_priv(tz);
	u32 val;
	int ret;

	if (!priv->enabled)
		rtl9310_thermal_init(priv);

	ret = regmap_read(priv->regmap, RTL9310_TM0_RESULT0, &val);
	if (ret)
		return ret;

	val &= RTL9310_TM_CT_OUT_MASK;
	if (!val)	/* ADC has no valid flag; it reads 0 until settled */
		return -EAGAIN;

	if (val & RTL9310_TM_CT_SIGN)
		*res = -((int)((val & (RTL9310_TM_CT_SIGN - 1)) ^
			       (RTL9310_TM_CT_SIGN - 1)) * 1000 / 1024);
	else
		*res = val * 1000 / 1024;

	return 0;
}

static const struct thermal_zone_device_ops rtl9310_ops = {
	.get_temp = rtl9310_get_temp,
};

static int realtek_thermal_probe(struct platform_device *pdev)
{
	struct realtek_thermal_priv *priv;
	struct thermal_zone_device *tzdev;
	struct device *dev = &pdev->dev;
	struct regmap *regmap;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	regmap = syscon_node_to_regmap(dev->of_node->parent);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	priv->regmap = regmap;
	tzdev = devm_thermal_of_zone_register(dev, 0, priv, device_get_match_data(dev));
	if (IS_ERR(tzdev))
		return PTR_ERR(tzdev);

	return devm_thermal_add_hwmon_sysfs(dev, tzdev);
}

static const struct of_device_id realtek_sensor_ids[] = {
	{ .compatible = "realtek,rtl8380-thermal", .data = &rtl8380_ops, },
	{ .compatible = "realtek,rtl8390-thermal", .data = &rtl8390_ops, },
	{ .compatible = "realtek,rtl9300-thermal", .data = &rtl9300_ops, },
	{ .compatible = "realtek,rtl9310-thermal", .data = &rtl9310_ops, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, realtek_sensor_ids);

static struct platform_driver realtek_thermal_driver = {
	.probe	= realtek_thermal_probe,
	.driver	= {
		.name		= "realtek-thermal",
		.of_match_table	= realtek_sensor_ids,
	},
};

module_platform_driver(realtek_thermal_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bjørn Mork <bjorn@mork.no>");
MODULE_DESCRIPTION("Realtek temperature sensor");
