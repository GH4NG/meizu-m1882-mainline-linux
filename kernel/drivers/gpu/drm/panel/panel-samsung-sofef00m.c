// SPDX-License-Identifier: GPL-2.0-only

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/backlight.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct sofef00m_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static const struct regulator_bulk_data sofef00m_supplies[] = {
	{ .supply = "vddio" },
	{ .supply = "vdd" },
};

static inline 
struct sofef00m_panel *to_sofef00m_panel(struct drm_panel *panel)
{
	return container_of(panel, struct sofef00m_panel, panel);
}

#define sofef00m_test_key_on_lvl2(ctx) \
	mipi_dsi_dcs_write_seq_multi(ctx, 0xf0, 0x5a, 0x5a)
#define sofef00m_test_key_off_lvl2(ctx) \
	mipi_dsi_dcs_write_seq_multi(ctx, 0xf0, 0xa5, 0xa5)
#define sofef00m_test_key_on_lvl3(ctx) \
	mipi_dsi_dcs_write_seq_multi(ctx, 0xfc, 0x5a, 0x5a)
#define sofef00m_test_key_off_lvl3(ctx) \
	mipi_dsi_dcs_write_seq_multi(ctx, 0xfc, 0xa5, 0xa5)

static void sofef00m_panel_reset(struct sofef00m_panel *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int sofef00m_panel_on(struct sofef00m_panel *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 21);

	sofef00m_test_key_on_lvl2(&dsi_ctx);
	sofef00m_test_key_on_lvl3(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd2, 0x9e);
	sofef00m_test_key_off_lvl2(&dsi_ctx);
	sofef00m_test_key_off_lvl3(&dsi_ctx);
	sofef00m_test_key_on_lvl2(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_TEAR_ON, 0x00);
	sofef00m_test_key_off_lvl2(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_PAGE_ADDRESS,
				     0x00, 0x00, 0x08, 0x6f);
	sofef00m_test_key_on_lvl2(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbb, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xef, 0x33, 0x31, 0x14);
	sofef00m_test_key_off_lvl2(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x28);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_DISPLAY_BRIGHTNESS,
				     0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x55, 0x00);
	sofef00m_test_key_on_lvl2(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb1, 0x03);
	sofef00m_test_key_off_lvl2(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe2,
				     0xb0, 0x0c, 0x04, 0x3c, 0xd3, 0x12, 0x07,
				     0x04, 0xae, 0x47, 0xe8, 0xcb, 0xc4, 0x11,
				     0xc1, 0xe9, 0xe9, 0x17, 0xff, 0xff, 0xff,
				     0xef, 0x11, 0x05, 0x00, 0xc4, 0x00, 0x07,
				     0x04, 0xb5, 0x00, 0xda, 0xc1, 0xff, 0x16,
				     0xc6, 0xe8, 0xe8, 0x0e, 0xff, 0xff, 0xff,
				     0xbb, 0x03, 0x00, 0x12, 0xdc, 0x01, 0x06,
				     0x04, 0xa6, 0x0d, 0xf2, 0xc8, 0xc4, 0x0a,
				     0xde, 0xd4, 0xed, 0x05, 0xff, 0xff, 0xff);
	sofef00m_test_key_off_lvl2(&dsi_ctx);
	sofef00m_test_key_on_lvl2(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe2, 0x01);
	sofef00m_test_key_off_lvl2(&dsi_ctx);
	sofef00m_test_key_on_lvl2(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd5, 0x02, 0x00, 0x14, 0x14);
	sofef00m_test_key_off_lvl2(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 110);

	return dsi_ctx.accum_err;
}

static int sofef00m_enable(struct drm_panel *panel)
{
	struct sofef00m_panel *ctx = to_sofef00m_panel(panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);

	return dsi_ctx.accum_err;
}

static int sofef00m_panel_off(struct sofef00m_panel *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 40);

	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int sofef00m_panel_prepare(struct drm_panel *panel)
{
	struct sofef00m_panel *ctx = to_sofef00m_panel(panel);
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(sofef00m_supplies), ctx->supplies);
	if (ret < 0)
		return ret;

	sofef00m_panel_reset(ctx);

	ret = sofef00m_panel_on(ctx);
	if (ret < 0) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(sofef00m_supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int sofef00m_disable(struct drm_panel *panel)
{
	struct sofef00m_panel *ctx = to_sofef00m_panel(panel);

	sofef00m_panel_off(ctx);

	return 0;
}

static int sofef00m_panel_unprepare(struct drm_panel *panel)
{
	struct sofef00m_panel *ctx = to_sofef00m_panel(panel);

	regulator_bulk_disable(ARRAY_SIZE(sofef00m_supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode sofef00m_panel_mode = {
	.clock = (1080 + 128 + 24 + 60) * (2160 + 8 + 4 + 12) * 60 / 1000,

	.hdisplay = 1080,
	.hsync_start = 1080 + 128,
	.hsync_end = 1080 + 128 + 24,
	.htotal = 1080 + 128 + 24 + 60,

	.vdisplay = 2160,
	.vsync_start = 2160 + 8,
	.vsync_end = 2160 + 8 + 4,
	.vtotal = 2160 + 8 + 4 + 12,

	.width_mm = 68,
	.height_mm = 136,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static int sofef00m_panel_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &sofef00m_panel_mode);
}

static const struct drm_panel_funcs sofef00m_panel_panel_funcs = {
	.prepare = sofef00m_panel_prepare,
	.enable = sofef00m_enable,
	.disable = sofef00m_disable,
	.unprepare = sofef00m_panel_unprepare,
	.get_modes = sofef00m_panel_get_modes,
};

static int sofef00m_panel_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int err;
	u16 brightness = (u16)backlight_get_brightness(bl);

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	err = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
	if (err < 0)
		return err;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

static const struct backlight_ops sofef00m_panel_bl_ops = {
	.update_status = sofef00m_panel_bl_update_status,
};

static struct backlight_device *
sofef00m_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_PLATFORM,
		.brightness = 128,
		.max_brightness = 1023,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &sofef00m_panel_bl_ops, &props);
}

static int sofef00m_panel_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct sofef00m_panel *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct sofef00m_panel, panel,
				   &sofef00m_panel_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev,
					    ARRAY_SIZE(sofef00m_supplies),
					    sofef00m_supplies,
					    &ctx->supplies);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = sofef00m_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static void sofef00m_panel_remove(struct mipi_dsi_device *dsi)
{
	struct sofef00m_panel *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id sofef00m_panel_of_match[] = {
	{ .compatible = "samsung,sofef00m" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sofef00m_panel_of_match);

static struct mipi_dsi_driver sofef00m_panel_driver = {
	.probe = sofef00m_panel_probe,
	.remove = sofef00m_panel_remove,
	.driver = {
		.name = "panel-samsung-sofef00m",
		.of_match_table = sofef00m_panel_of_match,
	},
};

module_mipi_dsi_driver(sofef00m_panel_driver);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("DRM driver for Samsung SOFEF00M DDIC");
MODULE_LICENSE("GPL v2");
