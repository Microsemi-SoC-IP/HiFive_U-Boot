# SPDX-License-Identifier: GPL-2.0+

dtb-$(CONFIG_TARGET_AX25_AE350) += ae350.dtb
# dtb-$(CONFIG_TARGET_HIFIVE_U540) += hifive_u540.dtb
dtb-$(CONFIG_NVDLA_IOFPGA) += hifive_u540_nvdla_iofpga.dtb
dtb-$(CONFIG_TARGET_MICROCHIP_LC_MPFS) += lc-mpfs.dtb
targets += $(dtb-y)

DTC_FLAGS += -R 4 -p 0x1000

PHONY += dtbs
dtbs: $(addprefix $(obj)/, $(dtb-y))
	echo $(dtb-y)
	@:

clean-files := *.dtb
