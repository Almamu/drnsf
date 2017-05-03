//
// DRNSF - An unofficial Crash Bandicoot level editor
// Copyright (C) 2017  DRNSF contributors
//
// See the AUTHORS.md file for more details.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "common.hh"
#include <gtk/gtk.h>
#include "../imgui/imgui.h"
#include "edit.hh"
#include "res.hh"
#include "gfx.hh"

namespace drnsf {
namespace edit {

main_window::main_window() :
	m_wnd(APP_TITLE,1024,768),
	m_menu_file(m_wnd,"File"),
	m_menu_file_exit(m_menu_file,"Exit"),
	m_split(m_wnd),
	m_tabs(m_split.get_left()),
	m_assets_tab(m_tabs,"Assets"),
	m_detail_tab(m_tabs,"Details")
{
	h_menu_file_exit_click <<= [this]{
		gtk_main_quit();
	};
	h_menu_file_exit_click.bind(m_menu_file_exit.on_click);

	m_split.show();
	m_tabs.show();
}

void main_window::show()
{
	m_wnd.show();
}

void main_window::set_project(res::project &proj)
{
	m_map_view = nullptr;
	m_ed_p = nullptr;

	m_proj_p = &proj;
	m_ed_p = std::make_unique<editor>(*m_proj_p);
	m_map_view = std::make_unique<map_view>(m_split.get_right(),*m_ed_p);
	m_assets_view = std::make_unique<asset_editor>(m_assets_tab,proj);
	m_detail_view = std::make_unique<detail_view>(m_detail_tab,*m_ed_p);

	m_map_view->show();
	m_assets_view->show();
	m_detail_view->show();
}

}
}
