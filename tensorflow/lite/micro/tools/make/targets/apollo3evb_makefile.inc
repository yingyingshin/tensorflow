export PATH := $(MAKEFILE_DIR)/downloads/gcc_embedded/bin/:$(PATH)
TARGET_ARCH := cortex-m4
TARGET_TOOLCHAIN_PREFIX := arm-none-eabi-
TARGET_TOOLCHAIN_ROOT := $(TENSORFLOW_ROOT)$(MAKEFILE_DIR)/downloads/gcc_embedded/bin/
# Download the Ambiq Apollo3 SDK and set this variable to find the header
# files:
APOLLO3_SDK := $(MAKEFILE_DIR)/downloads/$(AM_SDK_DEST)
# Need a pointer to the GNU ARM toolchain for crtbegin.o for the fp functions
# with the hard interfaces.
GCC_ARM := $(MAKEFILE_DIR)/downloads/gcc_embedded/

$(eval $(call add_third_party_download,$(GCC_EMBEDDED_URL),$(GCC_EMBEDDED_MD5),gcc_embedded,))
$(eval $(call add_third_party_download,$(CMSIS_URL),$(CMSIS_MD5),cmsis,))
$(eval $(call add_third_party_download,$(AM_SDK_URL),$(AM_SDK_MD5),$(AM_SDK_DEST),patch_am_sdk))

ifeq ($(findstring sparkfun,$(TARGET)), sparkfun)
  $(eval $(call add_third_party_download,$(SF_BSPS_URL),$(SF_BSPS_MD5),$(AM_SDK_DEST)/$(SF_BSPS_DEST),))
  # Make sure that we download the full Ambiq SDK before the SparkFun BSPs.
$(MAKEFILE_DIR)/downloads/$(AM_SDK_DEST)/$(SF_BSPS_DEST): $(MAKEFILE_DIR)/downloads/$(AM_SDK_DEST)
endif

PLATFORM_FLAGS = \
  -DPART_apollo3 \
  -DAM_PACKAGE_BGA \
  -DAM_PART_APOLLO3 \
  -DGEMMLOWP_ALLOW_SLOW_SCALAR_FALLBACK \
  -DTF_LITE_STATIC_MEMORY \
  -DNDEBUG \
  -DTF_LITE_MCU_DEBUG_LOG \
  -D __FPU_PRESENT=1 \
  -DARM_MATH_CM4 \
  -fno-rtti \
  -fmessage-length=0 \
  -fno-exceptions \
  -fno-unwind-tables \
  -ffunction-sections \
  -fdata-sections \
  -funsigned-char \
  -MMD \
  -mcpu=cortex-m4 \
  -mthumb \
  -mfpu=fpv4-sp-d16 \
  -mfloat-abi=hard \
  -std=gnu++11 \
  -Wvla \
  -Wall \
  -Wextra \
  -Wno-missing-field-initializers \
  -Wno-strict-aliasing \
  -Wno-type-limits \
  -Wno-unused-function \
  -Wno-unused-parameter \
  -fno-delete-null-pointer-checks \
  -fno-threadsafe-statics \
  -fomit-frame-pointer \
  -fno-use-cxa-atexit \
  -nostdlib \
  -ggdb \
  -O3
CXXFLAGS += $(PLATFORM_FLAGS)
CCFLAGS += $(PLATFORM_FLAGS)
LDFLAGS += \
  -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  -nostartfiles -static \
  -Wl,--gc-sections -Wl,--entry,Reset_Handler \
  -Wl,--start-group -lm -lc -lgcc -Wl,--end-group \
  -fno-exceptions \
  -nostdlib --specs=nano.specs -t -lstdc++ -lc -lnosys -lm \
  -Wl,-T,$(TENSORFLOW_ROOT)$(APOLLO3_SDK)/boards/apollo3_evb/examples/hello_world/gcc_patched/apollo3evb.ld \
  -Wl,-Map=$(TENSORFLOW_ROOT)$(MAKEFILE_DIR)/gen/$(TARGET).map,--cref
BUILD_TYPE := micro
ifeq ($(TARGET), apollo3evb)
  BOARD_BSP_PATH := $(APOLLO3_SDK)/boards/apollo3_evb/bsp
endif
ifeq ($(findstring sparkfun,$(TARGET)), sparkfun)
  BOARD_BSP_PATH := $(APOLLO3_SDK)/$(SF_BSPS_DEST)/$(subst sparkfun_,,$(TARGET))/bsp
  INCLUDES+= \
    -I$(APOLLO3_SDK)/$(SF_BSPS_DEST)/common/third_party/hm01b0
endif
MICROLITE_LIBS := \
  $(BOARD_BSP_PATH)/gcc/bin/libam_bsp.a \
  $(APOLLO3_SDK)/mcu/apollo3/hal/gcc/bin/libam_hal.a \
  $(GCC_ARM)/lib/gcc/arm-none-eabi/7.3.1/thumb/v7e-m/fpv4-sp/hard/crtbegin.o \
  -lm
INCLUDES += \
  -isystem$(MAKEFILE_DIR)/downloads/cmsis/CMSIS/Core/Include/ \
  -isystem$(MAKEFILE_DIR)/downloads/cmsis/CMSIS/DSP/Include/ \
  -I$(GCC_ARM)/arm-none-eabi/ \
  -I$(APOLLO3_SDK)/mcu/apollo3/ \
  -I$(APOLLO3_SDK)/mcu/apollo3/regs \
  -I$(APOLLO3_SDK)/mcu/apollo3/hal \
  -I$(APOLLO3_SDK)/CMSIS/AmbiqMicro/Include/ \
  -I$(BOARD_BSP_PATH) \
  -I$(APOLLO3_SDK)/devices/ \
  -I$(APOLLO3_SDK)/utils/ \


# The startup_gcc.c file is an altered version of the examples/hello_world/gcc/startup_gcc.c
# file from Ambiq:
#   - Increase the stack size from 1k to 20k
#   - Change the application entry call from main() to _main()
# The am_*.c files should be copied from the Ambiq Apollo3 SDK
# _main.c contains application and target specific initialization, like
# setting clock speed, default uart setups, etc. and an implementation
# of the DebugLog interfaces.
MICROLITE_CC_SRCS += \
  $(APOLLO3_SDK)/boards/apollo3_evb/examples/hello_world/gcc_patched/startup_gcc.c \
  $(APOLLO3_SDK)/utils/am_util_delay.c \
  $(APOLLO3_SDK)/utils/am_util_faultisr.c \
  $(APOLLO3_SDK)/utils/am_util_id.c \
  $(APOLLO3_SDK)/utils/am_util_stdio.c \
  $(APOLLO3_SDK)/devices/am_devices_led.c

CMSIS_SRC_DIR := $(MAKEFILE_DIR)/downloads/cmsis/CMSIS/DSP/Source
THIRD_PARTY_CC_SRCS := \
$(CMSIS_SRC_DIR)/BasicMathFunctions/arm_dot_prod_q15.c \
$(CMSIS_SRC_DIR)/BasicMathFunctions/arm_mult_q15.c \
$(CMSIS_SRC_DIR)/TransformFunctions/arm_rfft_init_q15.c \
$(CMSIS_SRC_DIR)/TransformFunctions/arm_rfft_q15.c \
$(CMSIS_SRC_DIR)/TransformFunctions/arm_bitreversal2.c \
$(CMSIS_SRC_DIR)/TransformFunctions/arm_cfft_q15.c \
$(CMSIS_SRC_DIR)/TransformFunctions/arm_cfft_radix4_q15.c \
$(CMSIS_SRC_DIR)/CommonTables/arm_const_structs.c \
$(CMSIS_SRC_DIR)/CommonTables/arm_common_tables.c \
$(CMSIS_SRC_DIR)/StatisticsFunctions/arm_mean_q15.c \
$(CMSIS_SRC_DIR)/StatisticsFunctions/arm_max_q7.c

MICRO_SPEECH_TEST_SRCS += \
  $(AP3_MICRO_DIR)/_main.c

TEST_SCRIPT := tensorflow/lite/micro/testing/test_apollo3evb_binary.sh
# These are tests that don't currently work on the Apollo3 board.
EXCLUDED_TESTS := \
  tensorflow/lite/micro/micro_interpreter_test.cc \
  tensorflow/lite/micro/simple_tensor_allocator_test.cc
MICROLITE_TEST_SRCS := $(filter-out $(EXCLUDED_TESTS), $(MICROLITE_TEST_SRCS))
