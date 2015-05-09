#ifndef _WIMAXDBG_H
#define _WIMAXDBG_H

extern int mmc_wimax_set_netlog_status(int on);
extern int mmc_wimax_set_cliam_host_status(int on);
/* extern int sqn_sdio_get_sdc_clocks(void); */
/* extern void sqn_sdio_set_sdc_clocks(int on); */
extern int mmc_wimax_set_busclk_pwrsave(int on);
extern int mmc_wimax_set_sdcclk_highspeed(int on);
extern int mmc_wimax_set_CMD53_timeout_trigger_counter(int counter);
extern int mmc_wimax_get_CMD53_timeout_trigger_counter(void);

extern int sqn_sdio_notify_host_wakeup(void);
extern int mmc_wimax_set_thp_log(int on);
extern int mmc_wimax_set_irq_log(int on);
extern int mmc_wimax_set_sdio_hw_reset(int on);
extern int mmc_wimax_set_packet_filter(int on);

extern int mmc_wimax_set_netlog_withraw_status(int on);
extern int mmc_wimax_set_sdio_interrupt_log(int on);

extern int mmc_wimax_set_sdio_wakelock_log(int on);
extern int mmc_wimax_get_sdio_wakelock_log(void);
extern int mmc_wimax_set_sdio_lsp_log(int on);
extern int mmc_wimax_get_sdio_lsp_log(void);
extern int mmc_wimax_set_sdio_wakeup_lite_dump(int on);
extern int mmc_wimax_get_sdio_wakeup_lite_dump(void);

extern int mmc_wimax_trigger_RD_FIFO_LEVEL_ERROR(int on);

extern int mmc_wimax_set_wimax_FW_freeze_WK_RX(int on);
extern int mmc_wimax_get_wimax_FW_freeze_WK_RX(void);
extern int mmc_wimax_set_wimax_FW_freeze_WK_TX(int on);
extern int mmc_wimax_get_wimax_FW_freeze_WK_TX(void);


extern int mmc_wimax_set_disable_irq_config(int on);

#endif  /* _WIMAXDBG_H */
