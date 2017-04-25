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
#include "edit.hh"
#include "gfx.hh"
#include "misc.hh"
#include "nsf.hh"

namespace drnsf {
namespace edit {

class mode_classic;

namespace {

class tree_pane : public pane {
private:
	mode_classic &m_mode;

public:
	explicit tree_pane(old_editor &ed,mode_classic &mode) :
		pane(ed,"classic_tree"),
		m_mode(mode) {}

	void show() override;

	std::string get_title() const override;
};

class asset_pane : public pane {
private:
	mode_classic &m_mode;

public:
	explicit asset_pane(old_editor &ed,mode_classic &mode) :
		pane(ed,"classic_asset"),
		m_mode(mode) {}

	void show() override;

	std::string get_title() const override;

	template <typename T>
	bool field(const T &value,std::string label)
	{
		gui::im::label(label);
		gui::im::NextColumn();
		gui::im::AlignFirstTextHeightToWidgets();
		gui::im::label("[[ $ ]]"_fmt(typeid(T).name()));
		gui::im::NextColumn();
		return false;
	}

	template <typename T>
	std::enable_if_t<std::is_integral<T>::value,bool>
		field(T &value,std::string label)
	{
		int ivalue = value;
		gui::im::label(label);
		gui::im::NextColumn();
		bool changed = gui::im::InputInt("##value",&ivalue);
		gui::im::NextColumn();
		if (changed) {
			value = ivalue;
		}
		return changed;
	}

	template <typename T>
	std::enable_if_t<std::is_floating_point<T>::value,bool>
		field(T &value,std::string label)
	{
		float fvalue = value;
		gui::im::label(label);
		gui::im::NextColumn();
		bool changed = gui::im::InputFloat("##value",&fvalue);
		gui::im::NextColumn();
		if (changed) {
			value = fvalue;
		}
		return changed;
	}

	bool field(gfx::color &value,std::string label)
	{
		gui::im::label(label);
		gui::im::NextColumn();
		float fcolor[3] = {
			value.r / 255.0f,
			value.g / 255.0f,
			value.b / 255.0f
		};
		bool changed = gui::im::ColorEdit3("##value",fcolor);
		if (changed) {
			value.r = fcolor[0] * 255;
			value.g = fcolor[1] * 255;
			value.b = fcolor[2] * 255;
		}
		gui::im::NextColumn();
		return changed;
	}

	bool field(gfx::vertex &value,std::string label)
	{
		gui::im::label(label);
		gui::im::NextColumn();
		bool changed = gui::im::InputFloat3("##value",value.v);
		gui::im::NextColumn();
		return changed;
	}

	template <typename T>
	bool field(res::ref<T> &value,std::string label)
	{
		gui::im::label(label);
		gui::im::NextColumn();
		if (value) {
			gui::im::label(value.full_path());
			if (value.ok()) {
				gui::im::SameLine();
				gui::im::label("( OK )");
			} else if (value.get()) {
				gui::im::SameLine();
				gui::im::label("( Wrong Type! )");
			} else {
				gui::im::SameLine();
				gui::im::label("( Not Found )");
			}
		} else {
			gui::im::label("[ null ]");
		}
		gui::im::NextColumn();
		return false;
	}

	template <typename T>
	bool field(std::vector<T> &list,std::string label)
	{
		gui::im::AlignFirstTextHeightToWidgets();
		bool is_open = gui::im::TreeNode(label.c_str());
		if (!is_open) {
			gui::im::NextColumn();
			gui::im::label("[ List of $ ]"_fmt(list.size()));
			gui::im::NextColumn();
			return false;
		}
		gui::im::NextColumn();
		gui::im::AlignFirstTextHeightToWidgets();
		gui::im::label("");
		gui::im::NextColumn();
		bool changed = false;
		for (auto &&i : util::range_of(list)) {
			gui::im::scope index_scope(i);
			changed |= field(list[i],"[$]"_fmt(i));
		}
		gui::im::TreePop();
		return changed;
	}

	template <typename T>
	void field(res::prop<T> &prop,std::string label)
	{
		gui::im::scope prop_scope(&prop);
		auto value = prop.get();
		bool changed = field(value,label);
		if (changed) {
			m_ed.get_project().get_transact() << [&](TRANSACT) {
				TS.describe("Edit '$'"_fmt(label));
				prop.set(TS,std::move(value));
			};
		}
	}
};

template <typename Reflector,typename Base>
static void dyn_reflect(Reflector &rfl,Base &value)
{
	// None of the given types match this.
}

template <typename Reflector,typename Base,typename T,typename... Remaining>
static void dyn_reflect(Reflector &rfl,Base &value)
{
	T *t = dynamic_cast<T *>(&value);
	if (t) {
		return t->reflect(rfl);
	} else {
		return dyn_reflect<Reflector,Base,Remaining...>(rfl,value);
	}
}

}

class mode_classic : public mode {
private:
	tree_pane m_tree;
	asset_pane m_asset;

public:
	res::atom m_selected_asset;

	explicit mode_classic(old_editor &ed) :
		mode(ed),
		m_tree(ed,*this),
		m_asset(ed,*this) {}
};

static modedef_of<mode_classic> g_mode_classic_def("Classic");

static void tree_pane_show_atom(const res::atom &atom,res::atom &selection)
{
	namespace im = gui::im;

	for (auto &&child : atom.get_children()) {
		if (im::Selectable(child.name().c_str(),selection == child)) {
			selection = child;
		}
		im::PushID(child.name().c_str());
		im::Indent();
		tree_pane_show_atom(child,selection);
		im::Unindent();
		im::PopID();
	}
}

void tree_pane::show()
{
	tree_pane_show_atom(
		m_ed.get_project().get_asset_root(),
		m_mode.m_selected_asset
	);
}

std::string tree_pane::get_title() const
{
	return "Asset Tree";
}

void asset_pane::show()
{
	namespace im = gui::im;

	auto &&sel = m_mode.m_selected_asset;

	if (!sel) {
		im::Text("No selected asset.");
		return;
	}

	if (!sel.get()) {
		im::Text("No asset currently exists with this name.");
		return;
	}

	im::Columns(2);
	dyn_reflect<
		asset_pane,
		res::asset,
		gfx::frame,
		gfx::anim,
		gfx::mesh,
		gfx::model,
		misc::raw_data,
		nsf::archive,
		nsf::spage,
		nsf::raw_entry,
		nsf::wgeo_v2,
		nsf::entry> (*this,*sel.get());
	im::Columns(1);
}

std::string asset_pane::get_title() const
{
	if (m_mode.m_selected_asset) {
		return "Asset: $"_fmt(m_mode.m_selected_asset);
	} else {
		return "Asset: [No selection]";
	}
}

}
}
