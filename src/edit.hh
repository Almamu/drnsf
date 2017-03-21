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

#pragma once

#include <list>
#include <unordered_set>
#include <set>
#include <experimental/any>
#include "res.hh"
#include "transact.hh"
#include "gui.hh"

struct ImGuiIO;
struct ImDrawData;
class mod_module_list;

namespace edit {

class project : util::not_copyable {
private:
	res::name::space m_asset_ns;
	transact::nexus m_transact;

public:
	res::name::space &get_asset_ns()
	{
		return m_asset_ns;
	}

	transact::nexus &get_transact()
	{
		return m_transact;
	}
};

class pane;
class mode;

class core; // FIXME

class window : public gui::window {
	friend class core; // FIXME

	friend class pane;

private:
	std::shared_ptr<project> m_proj;
	std::list<pane *> m_panes;
	std::unique_ptr<mode> m_mode;

public:
	window();

	void frame(int delta_time) override;
};

using editor = window;

class pane : private util::not_copyable {
private:
	editor &m_ed;
	decltype(window::m_panes)::iterator m_iter;

public:
	explicit pane(editor &ed);
	~pane();

	virtual void show() = 0;

	virtual std::string get_title() const = 0;
};

class mode : util::not_copyable {
protected:
	mode() = default;

public:
	virtual ~mode() = default;

	virtual void update(double delta_time) {}
	virtual void render() {}
	virtual void show_gui() {}
};

class modedef : util::not_copyable {
private:
	std::string m_title;

protected:
	explicit modedef(std::string title);
	~modedef();

public:
	static const std::set<modedef*> &get_list();

	const std::string &get_title() const
	{
		return m_title;
	}

	virtual std::unique_ptr<mode> create(editor &ed) const = 0;
};

template <typename T>
class modedef_of : private modedef {
public:
	std::unique_ptr<mode> create(editor &ed) const override
	{
		return std::unique_ptr<mode>(new T(ed));
	}

	explicit modedef_of(std::string title) :
		modedef(title) {}
};

struct cam {
	float pitch = 30;
	float lens_near = 1.8;
	float lens_far = 200;
	float lens_adjust = 3.6;
	float lens_focus = 1.0;
};

class module;
class module_info;

class core : public gui::window {
	friend class module;
	friend class ::mod_module_list;

private:
	std::shared_ptr<project> m_proj = std::make_shared<project>();
	res::name::space &m_ns = m_proj->get_asset_ns();
	transact::nexus &m_nx = m_proj->get_transact();
	std::map<const module_info *,std::unique_ptr<module>> m_modules;
	std::map<std::string,std::experimental::any> m_module_shares;
	const module_info *m_selected_module = nullptr;
	cam m_cam;
	bool m_firstframe = true;
	edit::window m_wnd;

public:
	core();

	void frame(int delta);

	const decltype(m_modules) &get_modules() const;
};

class panel;

class module : util::not_copyable {
	friend class core;
	friend class ::mod_module_list;
	friend class panel;

private:
	core &m_core;

	std::list<panel *> m_panels;

	std::map<std::string,std::function<void()>> m_hooks;

	virtual void firstframe() {}

	virtual void frame(int delta) {}

	virtual void show_file_menu() {}
	virtual void show_edit_menu() {}
	virtual void show_view_menu() {}
	virtual void show_tools_menu() {}

protected:
	explicit module(core &core);

	template <typename T,typename... Args>
	T &share(std::string name,Args... args)
	{
		auto it = m_core.m_module_shares.find(name);
		if (it != m_core.m_module_shares.end())
			return std::experimental::any_cast<T &>(it->second);
		auto pair = m_core.m_module_shares.insert(
			typename decltype(m_core.m_module_shares)::value_type(
				name,
				std::experimental::any(
					T(std::forward<Args>(args)...)
				)
			)
		);
		return std::experimental::any_cast<T &>(pair.first->second);
	}

	void hook(std::string name,std::function<void()> f);
	void raise(std::string name);

	transact::nexus &nx = m_core.m_nx;
	res::name::space &ns = m_core.m_ns;

	auto cam() -> cam &;

public:
	virtual ~module() = default;

	const std::list<panel *> &get_panels() const;
};

class module_info : util::not_copyable {
public:
	using set = std::unordered_set<module_info *>;

private:
	static std::unique_ptr<set> m_set;

protected:
	module_info();
	~module_info();

public:
	static const set &get_set();

	virtual const char *get_name() const = 0;
	virtual const char *get_title() const = 0;
	virtual const char *get_description() const = 0;

	virtual std::unique_ptr<module> create(core &core) const = 0;
};

template <typename M>
class module_info_impl : public module_info {
public:
	constexpr static const char *description = "No description available.";

	const char *get_name() const override
	{
		return M::name;
	}

	const char *get_title() const override
	{
		return M::mod_name;
	}

	const char *get_description() const override
	{
		return description;
	}

	std::unique_ptr<module> create(core &core) const override
	{
		return std::unique_ptr<module>(new M(core));
	}
};

class panel : util::not_copyable {
private:
	using func_type = std::function<void()>;

	module &m_mod;
	std::string m_title;
	func_type m_func;

public:
	bool visible = false;

	panel(module &mod,std::string title,func_type func);
	~panel();

	void show() const;

	const std::string &get_title() const;
};

}
