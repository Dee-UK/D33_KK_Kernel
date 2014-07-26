cmd_drivers/net/wireless/rkusbwifi/rtl8188eu/core/rtw_btcoex.o := arm-linux-gnueabihf-gcc -Wp,-MD,drivers/net/wireless/rkusbwifi/rtl8188eu/core/.rtw_btcoex.o.d  -nostdinc -isystem /usr/lib/gcc-cross/arm-linux-gnueabihf/4.8/include -I/home/darrell/android/rockchip/rk3188/arch/arm/include -Iarch/arm/include/generated -Iinclude  -include include/generated/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-rk3188/include -Iarch/arm/plat-rk/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -marm -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO -O1 -Wno-unused-variable -Wno-unused-value -Wno-unused-label -Wno-unused-parameter -Wno-unused-function -Wno-unused -Idrivers/net/wireless/rkusbwifi/rtl8188eu/include -DCONFIG_RTL8188E -DCONFIG_MP_INCLUDED -DCONFIG_POWER_SAVING -DCONFIG_TRAFFIC_PROTECT -DCONFIG_LOAD_PHY_PARA_FROM_FILE -DCONFIG_LITTLE_ENDIAN -DCONFIG_PLATFORM_ANDROID -DCONFIG_PLATFORM_ROCKCHIPS -DCONFIG_MINIMAL_MEMORY_USAGE    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(rtw_btcoex)"  -D"KBUILD_MODNAME=KBUILD_STR(8188eu)" -c -o drivers/net/wireless/rkusbwifi/rtl8188eu/core/rtw_btcoex.o drivers/net/wireless/rkusbwifi/rtl8188eu/core/rtw_btcoex.c

source_drivers/net/wireless/rkusbwifi/rtl8188eu/core/rtw_btcoex.o := drivers/net/wireless/rkusbwifi/rtl8188eu/core/rtw_btcoex.c

deps_drivers/net/wireless/rkusbwifi/rtl8188eu/core/rtw_btcoex.o := \
    $(wildcard include/config/bt/coexist.h) \
    $(wildcard include/config/concurrent/mode.h) \
    $(wildcard include/config/error/reset.h) \

drivers/net/wireless/rkusbwifi/rtl8188eu/core/rtw_btcoex.o: $(deps_drivers/net/wireless/rkusbwifi/rtl8188eu/core/rtw_btcoex.o)

$(deps_drivers/net/wireless/rkusbwifi/rtl8188eu/core/rtw_btcoex.o):
