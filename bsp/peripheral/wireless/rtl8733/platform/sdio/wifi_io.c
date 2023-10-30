#include <stdio.h>
#include <drivers/pin.h>
#include <drivers/sdio.h>
#include <drivers/mmcsd_card.h>
#include <drivers/mmcsd_core.h>
#include <aic_core.h>
#include <aic_drv.h>
#include "card.h"
#include "wifi_io.h"
#include "wifi/wifi_conf.h"

/* test wifi driver */
#define ADDR_MASK       0x10000
#define LOCAL_ADDR_MASK 0x00000

#define RTL8733_PROBE_TEST

extern int rtl8733bs_reset(void);
extern int rtl8733bs_power_on(void);
extern int rtl8733bs_power_off(void);
void wifi_fake_driver_probe_rtlwifi(struct sdio_func *func);

struct rt_mmcsd_card *rtt_mmc_card;
struct rt_sdio_function *rtt_sdio_func;
struct sdio_func *wifi_sdio_func;
struct mmc_card *wifi_mmc_card;

void (*wifi_irq_handler)(struct sdio_func *);

static void copy_rtt_sdio_func(struct sdio_func *func, struct rt_sdio_function *rt_func)
{
    //func->irq_handler = rt_func->irq_handler;
    func->max_blksize = rt_func->max_blk_size;
    func->cur_blksize = rt_func->cur_blk_size;
    func->enable_timeout = rt_func->enable_timeout_val;
    func->num = rt_func->num;
    func->vendor = rt_func->manufacturer;
    func->device = rt_func->product;
    func->num_info = 0;
#ifdef CONFIG_READ_CIS
    func->tuples = NULL;
#endif
    func->drv_priv = rt_func->priv;
}

static void copy_rtt_mmc_card(struct mmc_card *card, struct rt_mmcsd_card *rt_card)
{
    int i = 0;

    card->host = (struct mmc_host *)rt_card->host;

    //cccr
    card->cccr.sdio_vsn = rt_card->cccr.sdio_version;
    card->cccr.sd_vsn = rt_card->cccr.sd_version;
    card->cccr.multi_block = rt_card->cccr.multi_block;
    card->cccr.low_speed = rt_card->cccr.low_speed;
    card->cccr.wide_bus = rt_card->cccr.bus_width;
    card->cccr.high_power = rt_card->cccr.power_ctrl;
    card->cccr.high_speed = rt_card->cccr.high_speed;
    card->cccr.disable_cd = rt_card->cccr.cd_disable;

    //cis
    card->cis.vendor = rt_card->cis.manufacturer;
    card->cis.device = rt_card->cis.product;
    card->cis.blksize = rt_card->cis.func0_blk_size;
    card->cis.max_dtr = rt_card->cis.max_tran_speed;

    //sdio_func
    card->sdio_funcs = rt_card->sdio_function_num;
    for(i=0; i<7; i++)
        card->sdio_func[i] = NULL;

    card->rca = rt_card->rca;
    card->type = rt_card->card_type;
#ifdef CONFIG_READ_CIS
    card->tuples = NULL;
#endif
}

extern void cmd_wifi_reorder_scan(int argc, char **argv);
extern void cmd_wifi_scan(int argc, char **argv);


int wifi_sdio_probe(struct rt_mmcsd_card *card)
{
    /* rtt card & function */
    rtt_mmc_card = card;
    rtt_sdio_func = card->sdio_function[1];

    /* rtl function */
    wifi_sdio_func = aicos_malloc(MEM_DEFAULT, sizeof(struct sdio_func));
    if (wifi_sdio_func == NULL)
        return -ENOMEM;
    copy_rtt_sdio_func(wifi_sdio_func, rtt_sdio_func);

    /* rtl card */
    wifi_mmc_card = aicos_malloc(MEM_DEFAULT, sizeof(struct mmc_card));
    if (wifi_mmc_card == NULL)
        return -ENOMEM;
    copy_rtt_mmc_card(wifi_mmc_card, rtt_mmc_card);
    wifi_mmc_card->sdio_func[1] = wifi_sdio_func;
    wifi_sdio_func->card = wifi_mmc_card;

    sdio_enable_func(rtt_sdio_func);
    sdio_set_block_size(rtt_sdio_func, 512);
#ifdef RTL8733_PROBE_TEST
    wifi_fake_driver_probe_rtlwifi(wifi_sdio_func);
#endif

	if(wifi_on(1)<0){
		pr_err("ERROR: Wifi on failed!\n");
	}

    cmd_wifi_scan(0, NULL);

    return 0;
}

void wifi_sdio_remove(struct rt_mmcsd_card *card)
{
    if (wifi_mmc_card)
        aicos_free(MEM_DEFAULT, wifi_mmc_card);

    if (wifi_sdio_func)
        aicos_free(MEM_DEFAULT, wifi_sdio_func);

}

static int rtt_sdio_bus_probe(void)
{
    rtl8733bs_reset();

    return 0;
}
static int rtt_sdio_bus_remove(void)
{
    rtl8733bs_power_off();

    return 0;
}
static int rtt_sdio_enable_func(struct sdio_func *func)
{
    return sdio_enable_func(rtt_sdio_func);
}
static int rtt_sdio_disable_func(struct sdio_func *func)
{
    return sdio_disable_func(rtt_sdio_func);
}

static void rtt_sdio_irq_handler(struct rt_sdio_function * rt_func)
{
    wifi_irq_handler(wifi_sdio_func);
}

/**
    Register SDIO interrupt function
*/
static int rtt_sdio_claim_irq(struct sdio_func *func,
                                  void (*handler)(struct sdio_func *))
{
    wifi_irq_handler = handler;
    return sdio_attach_irq(rtt_sdio_func, rtt_sdio_irq_handler);
}

/**
    Release SDIO interrupt function
*/
static int rtt_sdio_release_irq(struct sdio_func *func)
{
    sdio_detach_irq(rtt_sdio_func);
    wifi_irq_handler = NULL;
    return 0;
}

/**
    Get the SDIO privilege before IO
*/
static void rtt_sdio_claim_host(struct sdio_func *func)
{
    mmcsd_host_lock(rtt_mmc_card->host);
}

/**
    Release the SDIO privilege after IO
*/
static void rtt_sdio_release_host(struct sdio_func *func)
{
    mmcsd_host_unlock(rtt_mmc_card->host);
}

static unsigned char rtt_sdio_readb(struct sdio_func *func,
                                        unsigned int addr, int *err_ret)
{
    return sdio_io_readb(rtt_sdio_func, addr, err_ret);
}

static unsigned short rtt_sdio_readw(struct sdio_func *func,
                                         unsigned int addr, int *err_ret)
{
    unsigned short data = 0;
    sdio_io_rw_extended_block(rtt_sdio_func, 0, addr, 1, (rt_uint8_t *)&data,
                              sizeof(unsigned short));
    return data;
}

static unsigned int rtt_sdio_readl(struct sdio_func *func,
                                       unsigned int addr, int *err_ret)
{
    unsigned int data = 0;
    sdio_io_rw_extended_block(rtt_sdio_func, 0, addr, 1, (rt_uint8_t *)&data,
                              sizeof(unsigned int));
    return data;
}

static void rtt_sdio_writeb(struct sdio_func *func, unsigned char b,
                                unsigned int addr, int *err_ret)
{
    *err_ret = sdio_io_writeb(rtt_sdio_func, addr, b);
}

static void rtt_sdio_writew(struct sdio_func *func, unsigned short b,
                                unsigned int addr, int *err_ret)
{
    *err_ret = sdio_io_rw_extended_block(rtt_sdio_func, 1, addr, 1,
                                         (rt_uint8_t *)&b,
                                         sizeof(unsigned short));
}

static void rtt_sdio_writel(struct sdio_func *func, unsigned int b,
                                unsigned int addr, int *err_ret)
{
    *err_ret = sdio_io_rw_extended_block(rtt_sdio_func, 1, addr, 1,
                                         (rt_uint8_t *)&b,
                                         sizeof(unsigned int));
}

static int rtt_sdio_memcpy_fromio(struct sdio_func *func, void *dst,
                                      unsigned int addr, int count)
{
    return sdio_io_rw_extended_block(rtt_sdio_func, 0, addr, 1, dst, count);
}

static int rtt_sdio_memcpy_toio(struct sdio_func *func,
                                    unsigned int addr, void *src, int count)
{
    int ret = 0;

    ret = sdio_io_rw_extended_block(rtt_sdio_func, 1, addr, 1, src, count);

    return ret;
}

int wifi_read(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
    int err;

    rtt_sdio_claim_host(func);

    err = rtt_sdio_memcpy_fromio(func, pdata, addr, cnt);
    if (err) {
        pr_err("%s: FAIL(%d)! ADDR=%#x Size=%d\n", __func__, err, addr, cnt);
    }

    rtt_sdio_release_host(func);

    return err;
}

int wifi_write(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
    int err;
    u32 size;

    rtt_sdio_claim_host(func);

    size = cnt;
    err = rtt_sdio_memcpy_toio(func, addr, pdata, size);
    if (err) {
        pr_err("%s: FAIL(%d)! ADDR=%#x Size=%d(%d)\n", __func__, err, addr,
                 cnt, size);
    }

    rtt_sdio_release_host(func);

    return err;
}

u8 wifi_readb(struct sdio_func *func, u32 addr)
{
    int err;
    u8 ret = 0;

    rtt_sdio_claim_host(func);
    ret = rtt_sdio_readb(func, ADDR_MASK | addr, &err);
    rtt_sdio_release_host(func);
    if (err)
        pr_err("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

    return ret;
}

u16 wifi_readw(struct sdio_func *func, u32 addr)
{
    int err = 0;
    u16 v;

    rtt_sdio_claim_host(func);
    v = rtt_sdio_readw(func, ADDR_MASK | addr, &err);
    rtt_sdio_release_host(func);
    if (err)
        pr_err("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

    return v;
}

u32 wifi_readl(struct sdio_func *func, u32 addr)
{
    int err;
    u32 v;

    rtt_sdio_claim_host(func);
    v = rtt_sdio_readl(func, ADDR_MASK | addr, &err);
    rtt_sdio_release_host(func);

    return v;
}

void wifi_writeb(struct sdio_func *func, u32 addr, u8 val)
{
    int err;

    rtt_sdio_claim_host(func);
    rtt_sdio_writeb(func, val, ADDR_MASK | addr, &err);
    rtt_sdio_release_host(func);
    if (err)
        pr_err("%s: FAIL!(%d) addr=0x%05x val=0x%02x\n", __func__, err, addr,
                 val);
}

void wifi_writew(struct sdio_func *func, u32 addr, u16 v)
{
    int err;

    rtt_sdio_claim_host(func);
    rtt_sdio_writew(func, v, ADDR_MASK | addr, &err);
    rtt_sdio_release_host(func);
    if (err)
        pr_err("%s: FAIL!(%d) addr=0x%05x val=0x%04x\n", __func__, err, addr,
                 v);
}

void wifi_writel(struct sdio_func *func, u32 addr, u32 v)
{
    int err;

    rtt_sdio_claim_host(func);
    rtt_sdio_writel(func, v, ADDR_MASK | addr, &err);
    rtt_sdio_release_host(func);
}

u8 wifi_readb_local(struct sdio_func *func, u32 addr)
{
    int err;
    u8 ret = 0;

    ret = rtt_sdio_readb(func, LOCAL_ADDR_MASK | addr, &err);

    return ret;
}

void wifi_writeb_local(struct sdio_func *func, u32 addr, u8 val)
{
    int err;

    rtt_sdio_writeb(func, val, LOCAL_ADDR_MASK | addr, &err);
}
extern int rtw_fake_driver_probe(struct sdio_func *func);
void wifi_fake_driver_probe_rtlwifi(struct sdio_func *func)
{
    rtw_fake_driver_probe(func);
}

SDIO_BUS_OPS rtw_sdio_bus_ops = {
    rtt_sdio_bus_probe,
    rtt_sdio_bus_remove,
    rtt_sdio_enable_func,
    rtt_sdio_disable_func,
    NULL,
    NULL,
    rtt_sdio_claim_irq,
    rtt_sdio_release_irq,
    rtt_sdio_claim_host,
    rtt_sdio_release_host,
    rtt_sdio_readb,
    rtt_sdio_readw,
    rtt_sdio_readl,
    rtt_sdio_writeb,
    rtt_sdio_writew,
    rtt_sdio_writel,
    rtt_sdio_memcpy_fromio,
    rtt_sdio_memcpy_toio,
};

