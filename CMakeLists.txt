#
# DRNSF - An unofficial Crash Bandicoot level editor
# Copyright (C) 2017-2018  DRNSF contributors
#
# See the AUTHORS.md file for more details.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required (VERSION 3.8)
project (DRNSF VERSION 0.0)

# Option: AUTOMATIC_DEPS
if (WIN32)
    set (AUTOMATIC_DEPS OFF CACHE BOOL
        "Automatically download and manage DRNSF dependencies."
    )
    if (AUTOMATIC_DEPS)
        include (tools/automatic_deps_win32.cmake)
    endif ()
endif ()

# Option: STATIC_MSVC_RUNTIME
if (MSVC)
    set (STATIC_MSVC_RUNTIME OFF CACHE BOOL
    "Build DRNSF and its dependencies against static MSVC runtimes."
    )
    if (STATIC_MSVC_RUNTIME)
        foreach (fvar IN ITEMS
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
        )
            string (REPLACE "/MD" "/MT" ${fvar} "${${fvar}}")
        endforeach ()
    endif ()
endif ()

add_library (imgui OBJECT
    imgui/imgui.h
    imgui/imconfig.h
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_demo.cpp
)

add_executable (drnsf
    src/main.cc

    src/common.hh
    src/buildconfig.hh  # processed by configure_file below

    src/util.hh
    src/util.cc
    src/util_binreader.cc
    src/util_binwriter.cc

    src/fs.hh

    src/gui.hh
    src/gui.cc
    src/gui_widget.cc
    src/gui_composite.cc
    src/gui_window.cc
    src/gui_popup.cc
    src/gui_widget_gl.cc
    src/gui_widget_2d.cc
    src/gui_widget_im.cc
    src/gui_label.cc
    src/gui_treeview.cc
    src/gui_menu.cc
    src/gui_menubar.cc
    src/gui_dialogs.cc
    src/gui_im.cc

    src/gl.hh
    src/gl.cc

    src/render.hh
    src/render_viewport.cc
    src/render_figure.cc
    src/render_reticle_fig.cc
    src/render_frameonly_fig.cc
    src/render_animonly_fig.cc
    src/render_meshframe_fig.cc
    src/render_meshanim_fig.cc
    src/render_model_fig.cc
    src/render_world_fig.cc

    src/edit.hh
    src/edit_context.cc
    src/edit_menus.cc
    src/edit_asset_metactl.cc
    src/edit_asset_viewctl.cc
    src/edit_asset_propctl.cc
    src/edit_asset_editor.cc
    src/edit_asset_tree.cc
    src/edit_map_mainctl.cc
    src/edit_main_window.cc
    src/edit_im_canvas.cc
    src/edit_editor.cc
    src/edit_pane.cc
    src/edit_core.cc
    src/edit_mode.cc
    src/edit_mode_classic.cc
    src/edit_main_view.cc

    src/transact.hh
    src/transact_teller.cc
    src/transact_transaction.cc
    src/transact_nexus.cc

    src/res.hh
    src/res_atom.cc
    src/res_asset.cc

    src/gfx.hh

    src/misc.hh

    src/nsf.hh
    src/nsf_archive.cc
    src/nsf_spage.cc
    src/nsf_entry.cc
    src/nsf_raw_entry.cc
    src/nsf_wgeo_v2.cc

    $<TARGET_OBJECTS:imgui>

    src/embed_data.cc
)
set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT drnsf)

# Enable C++17
set_target_properties (drnsf PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Option: FRONTEND
if (WIN32)
    set (DEFAULT_FRONTEND WINAPI)
else ()
    set (DEFAULT_FRONTEND X11)
endif ()
set (FRONTEND "${DEFAULT_FRONTEND}" CACHE STRING
    "User interface frontend"
)
set_property (CACHE FRONTEND PROPERTY STRINGS
    X11
    WINAPI
)
if (FRONTEND STREQUAL X11)
    set (USE_GUI ON)
    set (USE_X11 ON)
    set (USE_GL ON)
elseif (FRONTEND STREQUAL WINAPI)
    set (USE_GUI ON)
    set (USE_WINAPI ON)
    set (USE_GL ON)
else ()
    message (FATAL_ERROR "Unrecognized FRONTEND value: ${FRONTEND}")
endif ()

# Option: FEATURE_INTERNAL_TEST
set (FEATURE_INTERNAL_TEST ON CACHE BOOL
    "Enable the internal-test command. (requires googletest)"
)

# Dependency: pkg-config
find_package (PkgConfig REQUIRED)

# Dependency: cairo
if (USE_X11)
    pkg_search_module (CAIRO REQUIRED cairo-xlib IMPORTED_TARGET)
    target_link_libraries (drnsf PRIVATE PkgConfig::CAIRO)
elseif (USE_WINAPI)
    pkg_search_module (CAIRO REQUIRED cairo-win32 IMPORTED_TARGET)
    target_link_libraries (drnsf PRIVATE PkgConfig::CAIRO)
endif ()

# Dependency: X11
if (USE_X11)
    find_package (X11 REQUIRED)
    target_include_directories (drnsf PRIVATE ${X11_INCLUDE_DIRS})
    target_link_libraries (drnsf PRIVATE ${X11_LIBRARIES})
endif ()

# Dependency: OpenGL
find_package (OpenGL REQUIRED)
target_link_libraries (drnsf PRIVATE OpenGL::GL)

# Dependency: libepoxy
pkg_search_module (EPOXY REQUIRED epoxy IMPORTED_TARGET)
target_link_libraries (drnsf PRIVATE PkgConfig::EPOXY)

# Dependency: GLM
# (header-only, included as a git submodule)
target_include_directories (drnsf PRIVATE ${CMAKE_SOURCE_DIR}/glm)

# Dependency: Google Test
# (included as a git submodule)
if (FEATURE_INTERNAL_TEST)
    set (BUILD_GTEST ON CACHE INTERNAL
        "Use googletest."
        FORCE)
    set (BUILD_GMOCK OFF CACHE INTERNAL
        "Use googlemock."
        FORCE)
    set (INSTALL_GTEST OFF CACHE INTERNAL
        "Install googletest when installing drnsf."
        FORCE
    )
    set (INSTALL_GMOCK OFF CACHE INTERNAL
        "Install googlemock when installing drnsf."
        FORCE
    )
    if (MSVC)
        set (gtest_force_shared_crt ON CACHE INTERNAL
            "Use non-DLL runtime for googletest."
            FORCE
        )
    endif ()
    add_subdirectory (googletest)
    target_link_libraries (drnsf PRIVATE gtest)
endif()

# Dependency: C++17 Filesystem
if (CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries (drnsf PRIVATE stdc++fs)
endif ()

# GCC Warnings
if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options (drnsf PRIVATE -Wall -Wextra)
    target_compile_options (drnsf PRIVATE -Wno-unused-parameter)
endif ()

# Add directory for generated header/include files
set (GEN_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen_include")
file (MAKE_DIRECTORY "${GEN_INCLUDE_DIR}")
target_include_directories (drnsf
    PRIVATE "${GEN_INCLUDE_DIR}"
)

# Process buildconfig.hh
configure_file (
    src/buildconfig.hh
    "${GEN_INCLUDE_DIR}/drnsf_buildconfig_out.hh"
    NEWLINE_STYLE LF
)

# Dependency: Python 3
find_package (PythonInterp 3 REQUIRED)

# Tool: embedgen
# This command generates the drnsf_embed_data.hh file from the `data/'
# directory so that it can be added to the exectuable.
add_custom_target (embed_data_dir
    COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/embedgen.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/data"
    "${GEN_INCLUDE_DIR}/drnsf_embed_data.hh"
    COMMENT "Packing data/ directory for embedding..."
    SOURCES data/
)
add_dependencies (drnsf embed_data_dir)
