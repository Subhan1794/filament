cmake_minimum_required(VERSION 3.19)
project(ktxreader)

set(TARGET ktxreader)
set(PUBLIC_HDR_DIR include)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(PUBLIC_HDRS
        include/ktxreader/Ktx1Reader.h
        include/ktxreader/Ktx2Reader.h
)

set(SRCS
        src/Ktx1Reader.cpp
        src/Ktx2Reader.cpp
)

# ==================================================================================================
# Include and target definitions
# ==================================================================================================
include_directories(${PUBLIC_HDR_DIR})

add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${SRCS})

target_link_libraries(${TARGET} PUBLIC utils image filament basis_transcoder)

target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIR})
set_target_properties(${TARGET} PROPERTIES FOLDER Libs)

# ==================================================================================================
# Compiler flags
# ==================================================================================================
if (MSVC)
else()
    target_compile_options(${TARGET} PRIVATE -Wno-deprecated-register)
endif()

# ==================================================================================================
# Installation
# ==================================================================================================
install(TARGETS ${TARGET} ARCHIVE DESTINATION lib/${DIST_DIR})
install(DIRECTORY ${PUBLIC_HDR_DIR}/${TARGET} DESTINATION include)

# ==================================================================================================
# Tests
# ==================================================================================================

function(add_testfile filename)
    set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/tests/${filename}")
    set(target_path "${PROJECT_BINARY_DIR}/${filename}")
    set(TESTFILES ${TESTFILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND ${CMAKE_COMMAND} -E copy ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path})
endfunction()

add_testfile(color_grid_uastc_zstd.ktx2)
add_testfile(lightroom_ibl.ktx)
add_testfile(m3_level_offset_wrap4g.ktx2)
add_testfile(s1_sgd_wrap_1tb.ktx2)
add_testfile(t1_std_scheme4_wrap.ktx2)
add_testfile(t2_std_scheme5_wrap.ktx2)
add_testfile(t3_orig_scheme1_wrap.ktx2)
add_testfile(sgd_len_wrap_ep.ktx2)
add_testfile(sgd_len_wrap_sel.ktx2)
add_testfile(slice_desc.ktx2)
add_testfile(uastc_lvl_off_far.ktx2)
add_testfile(sgd_off_far.ktx2)
add_testfile(slice_s4_len12.ktx2)
add_testfile(t1_tablescraft.ktx2)
add_testfile(t2_tablescraft.ktx2)
add_testfile(n1_2nd_table.ktx2)
add_testfile(n2_3rd_table.ktx2)
add_testfile(n3_4th_table.ktx2)
add_testfile(n4_2nd_table_t2.ktx2)
add_testfile(n5_4th_table_m8.ktx2)
add_testfile(A13_off2_64m1.ktx2)
add_testfile(A14_off2_64m44.ktx2)
add_testfile(F01_l1off2_64m1.ktx2)
add_testfile(F02_l1off2_64m44.ktx2)
add_testfile(F13_l0off2_64m1_v2.ktx2)

if (FILAMENT_BUILD_TESTING AND NOT ANDROID AND NOT WASM AND NOT IOS)
    add_executable(test_ktxreader tests/test_ktxreader.cpp ${TESTFILES})
    target_link_libraries(test_ktxreader PRIVATE ${TARGET} gtest)
    set_target_properties(test_ktxreader PROPERTIES FOLDER Tests)
endif()
