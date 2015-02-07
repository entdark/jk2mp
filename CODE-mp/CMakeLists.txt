cmake_minimum_required(VERSION 2.6)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project(jomme)

set(cpu ${CMAKE_SYSTEM_PROCESSOR})
if (cpu MATCHES "i.86")
	set(cpu "x86")
endif()

# until amd64 works...
if (cpu STREQUAL "x86_64")
	set(CMAKE_LIBRARY_PATH  "/usr/lib32")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	set(CMAKE_C_SIZEOF_DATA_PTR 4) # Forces correct output format for NASM
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
	set(cpu "x86")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -O2 -msse2 -mmmx")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -O0")
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-overflow")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")
# 	# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-overflow=5")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result") # fixme!!
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-write-strings")
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -Wall -g -O2 -msse2 -mmmx")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -O0")
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-overflow")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reorder")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
#	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-overflow=5")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result") # fixme!!
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
	set_source_files_properties(png/rpng.cpp PROPERTIES COMPILE_FLAGS "-Wno-multichar")
endif()

if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
endif()
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-parentheses-equality")
endif()

check_c_compiler_flag(-Wno-unused-but-set-variable cc_has_unused_but_set)
if (cc_has_unused_but_set)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
endif()

check_cxx_compiler_flag(-Wno-unused-but-set-variable cxx_has_unused_but_set)
if (cxx_has_unused_but_set)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
endif()

# avoid -rdynamic or loaded libraries will stomp over cvars
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

add_definitions( -D_M_IX86=1 )  # tested to mean little endian...
add_definitions( -D_JK2 )
add_definitions( -DQSDL )
add_definitions( -DUSE_ICON )
add_definitions( -DDONT_TYPEDEF_INT32 )
add_definitions( -DFINAL_BUILD )
if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	add_definitions( -DNDEBUG )
else()
	set(suffix "-debug")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "mme")

find_package (SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

find_package(LibMad REQUIRED)
include_directories(${LIBMAD_INCLUDE_DIR})

set(src_main_client
	client/FXExport.cpp
	client/FxPrimitives.cpp
	client/FxScheduler.cpp
	client/FxSystem.cpp
	client/FxTemplate.cpp
	client/FxUtil.cpp
	client/cl_cgame.cpp
	client/cl_cin.cpp
	client/cl_console.cpp
	client/cl_demos.cpp
	client/cl_input.cpp
	client/cl_keys.cpp
	client/cl_main.cpp
	client/cl_mme.cpp
	client/cl_net_chan.cpp
	client/cl_parse.cpp
	client/cl_scrn.cpp
	client/cl_ui.cpp
	client/snd_dma.cpp
	client/snd_effects.cpp
	client/snd_main.cpp
	client/snd_mem.cpp
	client/snd_mix.cpp
	client/snd_mme.cpp
)

set(src_main_encryption
	encryption/buffer.cpp
	encryption/cpp_interface.cpp
	encryption/sockets.cpp
)

set(src_main_ghoul2
	ghoul2/G2_API.cpp
	ghoul2/G2_bolts.cpp
	ghoul2/G2_bones.cpp
	ghoul2/G2_misc.cpp
	ghoul2/G2_surfaces.cpp
)

set(src_main_jpeg
	jpeg-6/jcapimin.cpp
	jpeg-6/jccoefct.cpp
	jpeg-6/jccolor.cpp
	jpeg-6/jcdctmgr.cpp
	jpeg-6/jchuff.cpp
	jpeg-6/jcinit.cpp
	jpeg-6/jcmainct.cpp
	jpeg-6/jcmarker.cpp
	jpeg-6/jcmaster.cpp
	jpeg-6/jcomapi.cpp
	jpeg-6/jcparam.cpp
	jpeg-6/jcphuff.cpp
	jpeg-6/jcprepct.cpp
	jpeg-6/jcsample.cpp
	jpeg-6/jctrans.cpp
	jpeg-6/jdapimin.cpp
	jpeg-6/jdapistd.cpp
	jpeg-6/jdatadst.cpp
	jpeg-6/jdatasrc.cpp
	jpeg-6/jdcoefct.cpp
	jpeg-6/jdcolor.cpp
	jpeg-6/jddctmgr.cpp
	jpeg-6/jdhuff.cpp
	jpeg-6/jdinput.cpp
	jpeg-6/jdmainct.cpp
	jpeg-6/jdmarker.cpp
	jpeg-6/jdmaster.cpp
	jpeg-6/jdpostct.cpp
	jpeg-6/jdsample.cpp
	jpeg-6/jdtrans.cpp
	jpeg-6/jerror.cpp
	jpeg-6/jfdctflt.cpp
	jpeg-6/jidctflt.cpp
	jpeg-6/jmemmgr.cpp
	jpeg-6/jmemnobs.cpp
	jpeg-6/jutils.cpp
)


set(src_main_png
	png/rpng.cpp
)

set(src_main_libpng
	libpng/png.c
	libpng/pngerror.c
	libpng/pngget.c
	libpng/pngmem.c
	libpng/pngpread.c
	libpng/pngread.c
	libpng/pngrio.c
	libpng/pngrtran.c
	libpng/pngrutil.c
	libpng/pngset.c
	libpng/pngtrans.c
	libpng/pngwio.c
	libpng/pngwrite.c
	libpng/pngwtran.c
	libpng/pngwutil.c
)

set(src_main_qcommon
	qcommon/CNetProfile.cpp
	qcommon/GenericParser2.cpp
	qcommon/RoffSystem.cpp
	qcommon/cm_load.cpp
	qcommon/cm_patch.cpp
	qcommon/cm_polylib.cpp
	qcommon/cm_test.cpp
	qcommon/cm_trace.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/hstring.cpp
	qcommon/huffman.cpp
	qcommon/md4.cpp
	qcommon/msg.cpp
	qcommon/net_chan.cpp
	qcommon/q_math.cpp
	qcommon/q_shared.cpp
	qcommon/strip.cpp
	qcommon/unzip.cpp
	qcommon/vm.cpp
	qcommon/vm_interpreted.cpp
	qcommon/vm_x86.cpp
)

set(src_renderer_common
	renderer/matcomp.c
	renderer/tr_backend.cpp
	renderer/tr_ghoul2.cpp
	renderer/tr_image.cpp
	renderer/tr_init.cpp
	renderer/tr_main.cpp
	renderer/tr_mesh.cpp
	renderer/tr_model.cpp
	renderer/tr_shader.cpp
)

set(src_main_renderer
	${src_renderer_common}
	renderer/tr_WorldEffects.cpp
	renderer/tr_animation.cpp
	renderer/tr_bsp.cpp
	renderer/tr_cmds.cpp
	renderer/tr_curve.cpp
	renderer/tr_flares.cpp
	renderer/tr_font.cpp
	renderer/tr_light.cpp
	renderer/tr_marks.cpp
	renderer/tr_mme.cpp
 	renderer/tr_mme_avi.cpp
	renderer/tr_mme_common.cpp
	renderer/tr_mme_font.cpp
	renderer/tr_mme_sse2.cpp
	renderer/tr_mme_stereo.cpp
	renderer/tr_noise.cpp
	renderer/tr_quicksprite.cpp
	renderer/tr_scene.cpp
	renderer/tr_shade.cpp
	renderer/tr_shade_calc.cpp
	renderer/tr_shadows.cpp
	renderer/tr_sky.cpp
	renderer/tr_surface.cpp
	renderer/tr_surfacesprites.cpp
	renderer/tr_world.cpp
)

set(src_main_server
	server/sv_bot.cpp
	server/sv_ccmds.cpp
	server/sv_client.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_net_chan.cpp
	server/sv_snapshot.cpp
	server/sv_world.cpp
)

set(src_main_win32
	win32/win_gamma.cpp
	win32/win_glimp.cpp
	win32/win_input.cpp
	win32/win_main.cpp
	win32/win_net.cpp
	win32/win_qgl.cpp
	win32/win_shared.cpp
	win32/win_snd.cpp
	win32/win_syscon.cpp
	win32/win_wndproc.cpp
)

set(src_main_zlib
	zlib/adler32.c
	zlib/crc32.cpp
	zlib/deflate.c
	zlib/infblock.c
	zlib/infcodes.c
	zlib/inffast.c
	zlib/inflate.c
	zlib/inftrees.c
	zlib/infutil.c
	zlib/trees.c
	zlib/zutil.c
)

ENABLE_LANGUAGE(ASM_NASM)

set(src_unix_common
	unix/linux_common.c
	unix/unix_main.cpp
	unix/unix_net.cpp
	unix/unix_shared.cpp
	unix/ftol.nasm
	unix/snapvector.nasm
)

set(src_main_unix
	${src_unix_common}
	unix/unix_glimp_common.cpp
)

set(src_sdl
	sdl/sdl_gamma.cpp
	sdl/sdl_glimp.cpp
	sdl/sdl_input.cpp
	sdl/sdl_snd.cpp
)

set(src_botlib
	botlib/be_aas_bspq3.cpp
	botlib/be_aas_cluster.cpp
	botlib/be_aas_debug.cpp
	botlib/be_aas_entity.cpp
	botlib/be_aas_file.cpp
	botlib/be_aas_main.cpp
	botlib/be_aas_move.cpp
	botlib/be_aas_optimize.cpp
	botlib/be_aas_reach.cpp
	botlib/be_aas_route.cpp
	botlib/be_aas_routealt.cpp
	botlib/be_aas_sample.cpp
	botlib/be_ai_char.cpp
	botlib/be_ai_chat.cpp
	botlib/be_ai_gen.cpp
	botlib/be_ai_goal.cpp
	botlib/be_ai_move.cpp
	botlib/be_ai_weap.cpp
	botlib/be_ai_weight.cpp
	botlib/be_ea.cpp
	botlib/be_interface.cpp
	botlib/l_crc.cpp
	botlib/l_libvar.cpp
	botlib/l_log.cpp
	botlib/l_memory.cpp
	botlib/l_precomp.cpp
	botlib/l_script.cpp
	botlib/l_struct.cpp
)

set(src_jomme
	${src_main_client}
	${src_main_encryption}
	${src_main_ghoul2}
	${src_main_jpeg}
	${src_main_png}
	${src_main_libpng}
	${src_main_qcommon}
	${src_main_renderer}
	${src_main_server}
	${src_main_zlib}
	${src_botlib}
	${src_main_unix}
	${src_sdl}
)

add_executable(jomme${suffix}
	${src_jomme}
)

set_target_properties(jomme${suffix} PROPERTIES COMPILE_DEFINITIONS "_JK2EXE;_FF_DISABLE;BOTLIB")

target_link_libraries(jomme${suffix}
	m pthread
	${SDL2_LIBRARY}
	${OPENGL_gl_LIBRARY}
	${LIBMAD_LIBRARY}
)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	target_link_libraries(jomme${suffix} dl)
endif()

set(src_ui_game
	game/bg_lib.c
	game/bg_misc.c
	game/bg_weapons.c
	game/q_math.c
	game/q_shared.c
)

set(src_ui_ui
	ui/ui_atoms.c
	ui/ui_force.c
	ui/ui_gameinfo.c
	ui/ui_main.c
	ui/ui_shared.c
	ui/ui_syscalls.c
)

add_library(ui${cpu}${suffix} SHARED
	${src_ui_game}
	${src_ui_ui}
)

set_target_properties(ui${cpu}${suffix} PROPERTIES PREFIX "")
set_target_properties(ui${cpu}${suffix} PROPERTIES COMPILE_DEFINITIONS "_USRDL;UI_EXPORTS;MISSIONPACK")

set(src_cgame_cgame
	cgame/bg_demos.c
	cgame/cg_consolecmds.c
	cgame/cg_demos.c
	cgame/cg_demos_camera.c
	cgame/cg_demos_capture.c
	cgame/cg_demos_dism.c
	cgame/cg_demos_dof.c
	cgame/cg_demos_hud.c
	cgame/cg_demos_line.c
	cgame/cg_demos_math.c
	cgame/cg_demos_move.c
	cgame/cg_demos_weather.c
	cgame/cg_draw.c
	cgame/cg_drawtools.c
	cgame/cg_effects.c
	cgame/cg_ents.c
	cgame/cg_event.c
	cgame/cg_info.c
	cgame/cg_light.c
	cgame/cg_localents.c
	cgame/cg_main.c
	cgame/cg_marks.c
	cgame/cg_newDraw.c
	cgame/cg_playeranimate.c
	cgame/cg_players.c
	cgame/cg_playerstate.c
	cgame/cg_predict.c
	cgame/cg_saga.c
	cgame/cg_scoreboard.c
	cgame/cg_servercmds.c
	cgame/cg_snapshot.c
	cgame/cg_syscalls.c
	cgame/cg_turret.c
	cgame/cg_view.c
	cgame/cg_weaponinit.c
	cgame/cg_weapons.c
	cgame/fx_blaster.c
	cgame/fx_bowcaster.c
	cgame/fx_bryarpistol.c
	cgame/fx_demp2.c
	cgame/fx_disruptor.c
	cgame/fx_flechette.c
	cgame/fx_force.c
	cgame/fx_heavyrepeater.c
	cgame/fx_rocketlauncher.c
)

set(src_cgame_game
	game/bg_lib.c
	game/bg_misc.c
	game/bg_panimate.c
	game/bg_pmove.c
	game/bg_saber.c
	game/bg_slidemove.c
	game/bg_weapons.c
	game/q_math.c
	game/q_shared.c
)

set(src_cgame_ui
	ui/ui_shared.c
)

add_library(cgame${cpu}${suffix} SHARED
	${src_cgame_cgame}
	${src_cgame_game}
	${src_cgame_ui}
)

set_target_properties(cgame${cpu}${suffix} PROPERTIES PREFIX "")
set_target_properties(cgame${cpu}${suffix} PROPERTIES COMPILE_DEFINITIONS "CGAME;MISSIONPACK")

set(src_game_game
	game/ai_main.c
	game/ai_util.c
	game/ai_wpnav.c
	game/bg_lib.c
	game/bg_misc.c
	game/bg_panimate.c
	game/bg_pmove.c
	game/bg_saber.c
	game/bg_slidemove.c
	game/bg_weapons.c
	game/g_active.c
	game/g_arenas.c
	game/g_bot.c
	game/g_client.c
	game/g_cmds.c
	game/g_combat.c
	game/g_items.c
	game/g_log.c
	game/g_main.c
	game/g_mem.c
	game/g_misc.c
	game/g_missile.c
	game/g_mover.c
	game/g_object.c
	game/g_saga.c
	game/g_session.c
	game/g_spawn.c
	game/g_svcmds.c
	game/g_syscalls.c
	game/g_target.c
	game/g_team.c
	game/g_trigger.c
	game/g_utils.c
	game/g_weapon.c
	game/q_math.c
	game/q_shared.c
	game/w_force.c
	game/w_saber.c
)

add_library(jk2mpgame${cpu}${suffix} SHARED
	${src_game_game}
)

set_target_properties(jk2mpgame${cpu}${suffix} PROPERTIES PREFIX "")
set_target_properties(jk2mpgame${cpu}${suffix} PROPERTIES COMPILE_DEFINITIONS "QAGAME;MISSIONPACK")
