#ifndef VEXTRYN_AIR_RTL8852_DLE_H
#define VEXTRYN_AIR_RTL8852_DLE_H

#include <stdint.h>

// ---------------------------------------------------------
// DLE Data Structures (Mapped from Linux rtl8852au driver)
// ---------------------------------------------------------

typedef struct {
    uint16_t pge_size;
    uint16_t lnk_pge_num;
    uint16_t unlnk_pge_num;
} vxair_dle_size_t;

typedef struct {
    uint16_t hif;
    uint16_t wcpu;
    uint16_t pkt_in;
    uint16_t cpu_io;
} vxair_wde_quota_t;

typedef struct {
    uint16_t cma0_tx;
    uint16_t cma1_tx;
    uint16_t c2h;
    uint16_t h2c;
    uint16_t wcpu;
    uint16_t mpdu_proc;
    uint16_t cma0_dma;
    uint16_t cma1_dma;
    uint16_t bb_rpt;
    uint16_t wd_rel;
    uint16_t cpu_io;
    uint16_t tx_rpt;
} vxair_ple_quota_t;

// ---------------------------------------------------------
// DLE Profile: USB 2.0 High-Speed for 8852B CCV
// (Equivalent to dle_mem_usb2_8852b)
// ---------------------------------------------------------

// Page sizes constants mapped from Linux driver
#define MAC_AX_WDE_PG_64  0
#define MAC_AX_WDE_PG_128 1
#define MAC_AX_WDE_PG_256 2

#define MAC_AX_PLE_PG_64  0
#define MAC_AX_PLE_PG_128 1
#define MAC_AX_PLE_PG_256 2

static const vxair_dle_size_t dle_wde_size25 = {
    MAC_AX_WDE_PG_64, /* pge_size */
    242,              /* lnk_pge_num */
    14                /* unlnk_pge_num */
};

static const vxair_dle_size_t dle_ple_size27 = {
    MAC_AX_PLE_PG_128, /* pge_size */
    1402,              /* lnk_pge_num */
    6                  /* unlnk_pge_num */
};

static const vxair_wde_quota_t dle_wde_qt25 = {
    190, /* hif */
    44,  /* wcpu */
    0,   /* pkt_in */
    8    /* cpu_io */
};

static const vxair_ple_quota_t dle_ple_qt61 = {
    780, /* cma0_tx */
    0,   /* cma1_tx */
    16,  /* c2h */
    48,  /* h2c */
    88,  /* wcpu */
    13,  /* mpdu_proc */
    370, /* cma0_dma */
    0,   /* cma1_dma */
    32,  /* bb_rpt */
    14,  /* wd_rel */
    8,   /* cpu_io */
    0    /* tx_rpt */
};

static const vxair_ple_quota_t dle_ple_qt62 = {
    780, /* cma0_tx */
    0,   /* cma1_tx */
    32,  /* c2h */
    48,  /* h2c */
    121, /* wcpu */
    13,  /* mpdu_proc */
    403, /* cma0_dma */
    0,   /* cma1_dma */
    65,  /* bb_rpt */
    14,  /* wd_rel */
    24,  /* cpu_io */
    0    /* tx_rpt */
};

#endif // VEXTRYN_AIR_RTL8852_DLE_H
