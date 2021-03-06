//
// DRNSF - An unofficial Crash Bandicoot level editor
// Copyright (C) 2017-2018  DRNSF contributors
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
#include "edit.hh"

namespace drnsf {
namespace edit {

// declared in edit.hh
map_mainctl::map_mainctl(
    gui::container &parent,
    gui::layout layout,
    context &ctx) :
    viewport(parent, layout),
    m_ctx(ctx)
{
    h_asset_appear <<= [this](res::asset &asset) {
        auto world = dynamic_cast<gfx::world *>(&asset);
        if (world) {
            auto fig = new render::world_fig(*this);
            m_world_figs.emplace(
                world,
                std::unique_ptr<render::world_fig>(fig)
            );
            fig->show();
            fig->set_world(world);
        }
    };
    h_asset_appear.bind(m_ctx.get_proj()->on_asset_appear);
    h_asset_disappear <<= [this](res::asset &asset) {
        auto world = dynamic_cast<gfx::world *>(&asset);
        if (world) {
            m_world_figs.erase(world);
        }
    };
    h_asset_disappear.bind(m_ctx.get_proj()->on_asset_disappear);

    // FIXME - support context project switching

    m_reticle.show();
}

}
}
