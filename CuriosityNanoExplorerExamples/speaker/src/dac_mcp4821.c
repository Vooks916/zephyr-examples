/*
 * Out-of-tree Zephyr DAC driver for the Microchip MCP4821 (12-bit, 1 channel, SPI).
 * Modelled on upstream drivers/dac/dac_ltc166x.c.
 *
 * Frame (16 bit, MSB first, SPI mode 0):
 *   b15 = 0 (write), b14 = x, b13 = GA (1 = 1x/2.048 V, 0 = 2x/4.096 V),
 *   b12 = SHDN (1 = active), b11..b0 = data.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp4821

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_mcp4821, CONFIG_DAC_LOG_LEVEL);

#define MCP4821_RESOLUTION 12
#define MCP4821_NCHANNELS  1
#define MCP4821_GA_1X      BIT(13)
#define MCP4821_SHDN_ON    BIT(12)
#define MCP4821_DATA_MASK  GENMASK(11, 0)

struct mcp4821_config {
        struct spi_dt_spec bus;
        struct gpio_dt_spec ldac;
        bool gain_1x;
};

static int mcp4821_channel_setup(const struct device *dev,
                                 const struct dac_channel_cfg *cfg)
{
        ARG_UNUSED(dev);

        if (cfg->channel_id >= MCP4821_NCHANNELS) {
                LOG_ERR("Unsupported channel %d", cfg->channel_id);
                return -ENOTSUP;
        }
        if (cfg->resolution != MCP4821_RESOLUTION) {
                LOG_ERR("Unsupported resolution %d", cfg->resolution);
                return -ENOTSUP;
        }
        if (cfg->internal) {
                LOG_ERR("Internal channels not supported");
                return -ENOTSUP;
        }
        /* Output is always buffered by the MCP4821's own amplifier; ignore cfg->buffered. */
        return 0;
}

static int mcp4821_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
        const struct mcp4821_config *config = dev->config;

        if (channel >= MCP4821_NCHANNELS) {
                return -ENOTSUP;
        }
        if (value > MCP4821_DATA_MASK) {
                return -EINVAL;
        }

        uint16_t frame = MCP4821_SHDN_ON | (uint16_t)(value & MCP4821_DATA_MASK);

        if (config->gain_1x) {
                frame |= MCP4821_GA_1X;
        }

        /* Two 8-bit words, MSB byte first (endianness-independent). */
        uint8_t tx_data[2] = { frame >> 8, frame & 0xFF };
        const struct spi_buf buf = { .buf = tx_data, .len = sizeof(tx_data) };
        const struct spi_buf_set tx = { .buffers = &buf, .count = 1 };

        return spi_write_dt(&config->bus, &tx);
}

static int mcp4821_init(const struct device *dev)
{
        const struct mcp4821_config *config = dev->config;

        if (!spi_is_ready_dt(&config->bus)) {
                LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
                return -ENODEV;
        }

        if (config->ldac.port != NULL) {
                if (!gpio_is_ready_dt(&config->ldac)) {
                        LOG_ERR("LDAC GPIO not ready");
                        return -ENODEV;
                }
                /* Physical LOW (flag GPIO_ACTIVE_HIGH in DT): output latches on CS rising edge. */
                int ret = gpio_pin_configure_dt(&config->ldac, GPIO_OUTPUT_INACTIVE);

                if (ret < 0) {
                        return ret;
                }
        }

        return 0;
}

static DEVICE_API(dac, mcp4821_driver_api) = {
        .channel_setup = mcp4821_channel_setup,
        .write_value = mcp4821_write_value,
};

#define MCP4821_DEFINE(n)                                                              \
        static const struct mcp4821_config mcp4821_config_##n = {                      \
                /* 2-arg form as used by upstream dac_ltc166x.c on main. On     \
                 * Zephyr <= v4.2 add a third arg: delay 0. Mode 0 = no CPOL/CPHA. */ \
                .bus = SPI_DT_SPEC_INST_GET(n,                                         \
                        SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB | SPI_WORD_SET(8)),  \
                .ldac = GPIO_DT_SPEC_INST_GET_OR(n, ldac_gpios, {0}),                  \
                .gain_1x = DT_INST_PROP(n, gain_1x),                                   \
        };                                                                             \
        DEVICE_DT_INST_DEFINE(n, mcp4821_init, NULL, NULL, &mcp4821_config_##n,        \
                              POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,                   \
                              &mcp4821_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCP4821_DEFINE)
