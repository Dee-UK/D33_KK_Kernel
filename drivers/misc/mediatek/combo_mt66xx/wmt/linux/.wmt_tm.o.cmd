cmd_drivers/misc/mediatek/combo_mt66xx/wmt/linux/wmt_tm.o := arm-linux-gnueabihf-gcc -Wp,-MD,drivers/misc/mediatek/combo_mt66xx/wmt/linux/.wmt_tm.o.d  -nostdinc -isystem /usr/lib/gcc-cross/arm-linux-gnueabihf/4.8/include -I/home/darrell/android/rockchip/rk3188/arch/arm/include -Iarch/arm/include/generated -Iinclude  -include include/generated/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-rk3188/include -Iarch/arm/plat-rk/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -marm -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO -Idrivers/misc/mediatek/combo_mt66xx/wmt/linux/include -Idrivers/misc/mediatek/combo_mt66xx/wmt/core/include -Idrivers/misc/mediatek/combo_mt66xx/wmt/include -Idrivers/misc/mediatek/combo_mt66xx/wmt/ -DWMT_PLAT_APEX=1 -DWMT_PLAT_ALPS=0 -DWMT_UART_RX_MODE_WORK=1 -DREMOVE_MK_NODE=1 -DMTK_CMB_SDIO_EINT    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(wmt_tm)"  -D"KBUILD_MODNAME=KBUILD_STR(mtk_stp_wmt)" -c -o drivers/misc/mediatek/combo_mt66xx/wmt/linux/wmt_tm.o drivers/misc/mediatek/combo_mt66xx/wmt/linux/wmt_tm.c

source_drivers/misc/mediatek/combo_mt66xx/wmt/linux/wmt_tm.o := drivers/misc/mediatek/combo_mt66xx/wmt/linux/wmt_tm.c

deps_drivers/misc/mediatek/combo_mt66xx/wmt/linux/wmt_tm.o := \
    $(wildcard include/config/thermal.h) \
    $(wildcard include/config/thermal/open.h) \

drivers/misc/mediatek/combo_mt66xx/wmt/linux/wmt_tm.o: $(deps_drivers/misc/mediatek/combo_mt66xx/wmt/linux/wmt_tm.o)

$(deps_drivers/misc/mediatek/combo_mt66xx/wmt/linux/wmt_tm.o):
