XBE_TITLE = WOWX
OUTPUT_DIR = $(CURDIR)/build/xbox
GEN_XISO = build/wowx.iso
SRCS = $(CURDIR)/src/main.c $(CURDIR)/src/pack.c $(CURDIR)/src/controller.c $(CURDIR)/src/animation.c $(CURDIR)/src/platform_xbox.c
SRCS += $(CURDIR)/src/network_xbox.cpp $(CURDIR)/src/auth_client.cpp $(CURDIR)/src/big_num.cpp $(CURDIR)/src/crypto_sha1.cpp
SRCS += $(CURDIR)/upstream/wowee/src/auth/srp.cpp $(CURDIR)/upstream/wowee/src/auth/vanilla_crypt.cpp
SRCS += $(CURDIR)/src/world_client.cpp $(CURDIR)/upstream/wowee/src/network/packet.cpp
SRCS += $(CURDIR)/src/input.c
SRCS += $(CURDIR)/src/looks.c
SRCS += $(CURDIR)/src/font.c $(CURDIR)/src/font_xbox.c
SRCS += $(CURDIR)/src/ui.c $(CURDIR)/src/ui_xbox.c
SRCS += $(CURDIR)/src/hud.c $(CURDIR)/src/portrait.c $(CURDIR)/src/icons.c $(CURDIR)/src/actionbar.c $(CURDIR)/src/cooldown.c $(CURDIR)/src/cooldown_catalog.c
SRCS += $(CURDIR)/src/cast.c $(CURDIR)/src/cast_ui.c
SRCS += $(CURDIR)/src/lighting.c $(CURDIR)/src/light_palette.c $(CURDIR)/src/sky.c $(CURDIR)/src/world_clock.c
SRCS += $(CURDIR)/src/weather.c
SRCS += $(CURDIR)/src/actions.c $(CURDIR)/src/fog.c $(CURDIR)/src/fog_xbox.c
SRCS += $(CURDIR)/src/backdrop.c $(CURDIR)/src/backdrop_effects.c $(CURDIR)/src/backdrop_xbox.c
SRCS += $(CURDIR)/src/map.c $(CURDIR)/src/map_xbox.c
SRCS += $(CURDIR)/src/death.c
SRCS += $(CURDIR)/src/death_scenario.c
SRCS += $(CURDIR)/src/soak_scenario.c
SRCS += $(CURDIR)/src/utility.c $(CURDIR)/src/utility_xbox.c
SRCS += $(CURDIR)/src/movement.c
SRCS += $(CURDIR)/src/collision.c
SRCS += $(CURDIR)/src/region.c
SRCS += $(CURDIR)/src/stream_scenario.c
SRCS += $(CURDIR)/src/character.c
SRCS += $(CURDIR)/src/realm.c
SRCS += $(CURDIR)/src/login.c $(CURDIR)/src/login_xbox.c
SRCS += $(CURDIR)/src/login_replay.c
SRCS += $(CURDIR)/src/character_ui.c $(CURDIR)/src/character_ui_xbox.c
SRCS += $(CURDIR)/src/appearance.c
SRCS += $(CURDIR)/src/avatar.c
SRCS += $(CURDIR)/src/preview.c
SRCS += $(CURDIR)/src/outfit.c
SRCS += $(CURDIR)/src/entities.c $(CURDIR)/src/update_inflate.c
SRCS += $(CURDIR)/src/game.c
SRCS += $(CURDIR)/src/journal.c $(CURDIR)/src/journal_xbox.c
SRCS += $(CURDIR)/src/spellbook.c $(CURDIR)/src/spellbook_xbox.c $(CURDIR)/src/spellbook_scenario.c
SRCS += $(CURDIR)/src/dialog.c $(CURDIR)/src/text.c
SRCS += $(CURDIR)/src/scenario.c $(CURDIR)/src/journey.c $(CURDIR)/src/trail.c
SRCS += $(CURDIR)/src/inventory.c
SRCS += $(CURDIR)/src/inventory_ui.c
SRCS += $(CURDIR)/src/replay.c $(CURDIR)/src/telemetry_xbox.c
SRCS += $(wildcard $(CURDIR)/upstream/libtommath/bn_*.c)
SRCS += $(CURDIR)/upstream/stormlib/src/libtomcrypt/src/hashes/sha1.c $(CURDIR)/upstream/stormlib/src/libtomcrypt/src/misc/crypt_argchk.c
SHADER_OBJS = src/vs.inl src/ps.inl src/font.inl src/backdrop.inl
NXDK_SDL = y
NXDK_NET = y
NXDK_CXX = y
ifndef NXDK_DIR
$(error Set NXDK_DIR to your nxdk checkout, or use scripts/build-xbox.ps1)
endif
export NXDK_DIR
export PATH := $(NXDK_DIR)/bin:$(PATH)
CFLAGS += -I$(CURDIR)/include -std=c11 -O2 -Wall -Wextra
AUTH_FLAGS = -DWX_XBOX -DLTC_NO_TEST -I$(CURDIR)/tools/compat -I$(CURDIR)/include -I$(CURDIR)/upstream/wowee/include -I$(CURDIR)/upstream/libtommath -I$(CURDIR)/upstream/stormlib/src/libtomcrypt/src/headers
MATH_FLAGS = -DLTM_NOTHING -DBN_MP_INIT_C= -DBN_MP_SET_U32_C= -DBN_MP_FROM_UBIN_C= -DBN_MP_COPY_C= -DBN_MP_CLEAR_C= -DBN_MP_ADD_C= -DBN_MP_SUB_C= -DBN_MP_MUL_C= -DBN_MP_MOD_C= -DBN_MP_EXPTMOD_C= -DBN_MP_CMP_C= -DBN_MP_UBIN_SIZE_C= -DBN_MP_TO_UBIN_C= -DBN_MP_TO_RADIX_C= -DBN_MP_READ_RADIX_C=
CFLAGS += $(AUTH_FLAGS) $(MATH_FLAGS)
CXXFLAGS += $(AUTH_FLAGS) $(MATH_FLAGS) -std=c++20 -O2 -Wall -Wextra
CXXFLAGS += -include $(CURDIR)/include/wx_bit_compat.hpp
MATH_FLAGS += -DBN_CUTOFFS_C= -DBN_MP_RADIX_SMAP_C=
include $(NXDK_DIR)/Makefile
# Windows clang writes drive-letter targets, whereas MSYS make uses Unix paths.
# Those .d targets do not invalidate the corresponding objects. Explicitly
# rebuild project modules on public-header changes to avoid mixed structure ABIs.
$(filter $(CURDIR)/src/%,$(OBJS)): $(wildcard $(CURDIR)/include/*.h $(CURDIR)/include/*.hpp)
# Generated shader includes must be ready before compiling their owner. The
# Windows .d paths also fail to rebuild main.obj for shader-only edits.
$(CURDIR)/src/main.obj: $(SHADER_OBJS) $(CURDIR)/src/portrait_fixture.h $(CURDIR)/src/cooldown_fixture.h src/action_fixture.h src/fog_fixture.h src/stream_fixture.h src/actor_stream_fixture.h
$(CURDIR)/src/font_xbox.obj: src/font.inl
$(CURDIR)/src/map_xbox.obj: src/font.inl
$(CURDIR)/src/ui_xbox.obj: src/font.inl
$(CURDIR)/src/backdrop_xbox.obj: src/backdrop.inl
$(CURDIR)/src/font.obj: $(CURDIR)/src/font_data.h
# Build flag changes still invalidate all object files.
$(OBJS): $(CURDIR)/Makefile
$(GEN_XISO): $(OUTPUT_DIR)/world.wxp
$(GEN_XISO): $(OUTPUT_DIR)/actors.wxp
$(GEN_XISO): $(OUTPUT_DIR)/entropy.bin $(OUTPUT_DIR)/testauth.bin
$(GEN_XISO): $(OUTPUT_DIR)/input.rpl
$(GEN_XISO): $(OUTPUT_DIR)/scenario.bin
$(GEN_XISO): $(OUTPUT_DIR)/CAMP.RTE
$(GEN_XISO): $(OUTPUT_DIR)/TESTCHAR.BIN
$(GEN_XISO): $(OUTPUT_DIR)/FONT.BIN
$(GEN_XISO): $(OUTPUT_DIR)/SOAK.BIN
$(GEN_XISO): $(OUTPUT_DIR)/LOGIN.BIN
$(GEN_XISO): $(OUTPUT_DIR)/MAPS.WMI $(wildcard $(OUTPUT_DIR)/Z*.WMP)
$(GEN_XISO): $(OUTPUT_DIR)/SPELLS.WXS
$(GEN_XISO): $(OUTPUT_DIR)/INTERFACE.WUI
$(GEN_XISO): $(OUTPUT_DIR)/TITLE.WXB
$(GEN_XISO): $(wildcard $(OUTPUT_DIR)/*.WXB)
$(GEN_XISO): $(OUTPUT_DIR)/OUTFITS.WXO
$(GEN_XISO): $(wildcard $(OUTPUT_DIR)/*.WXI $(OUTPUT_DIR)/*.wxi $(OUTPUT_DIR)/*.WXP)
$(GEN_XISO): $(wildcard $(OUTPUT_DIR)/A*.WXA)
$(GEN_XISO): $(wildcard $(OUTPUT_DIR)/L*.WXL)

$(GEN_XISO): $(OUTPUT_DIR)/PORTRAIT.WPT

$(GEN_XISO): $(OUTPUT_DIR)/ICONS.WIC

$(GEN_XISO): $(OUTPUT_DIR)/COOLDOWN.WCD

$(CURDIR)/src/pack.obj: $(CURDIR)/src/pack_stream.h $(CURDIR)/src/pack_open.h $(CURDIR)/src/stream_budget.h $(CURDIR)/src/pack_profile.h
$(CURDIR)/src/avatar.obj: $(CURDIR)/src/avatar_compose.h $(CURDIR)/src/avatar_open.h

$(CURDIR)/src/main.obj: $(CURDIR)/src/lighting_fixture.h
$(CURDIR)/src/main.obj: $(CURDIR)/src/profile_fixture.h
$(CURDIR)/src/main.obj: $(CURDIR)/src/global_world_fixture.h $(CURDIR)/src/world_material_xbox.h
$(CURDIR)/src/main.obj: $(CURDIR)/src/index_batch.h $(CURDIR)/src/draw_profile.h
$(CURDIR)/src/pack.obj: $(CURDIR)/src/pack_selection.h

$(GEN_XISO): $(OUTPUT_DIR)/LIGHT.WLF
