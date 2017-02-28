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

#include <SDL.h>

namespace sys {

class window : util::not_copyable {
private:
	SDL_Window *m_wnd;
	SDL_GLContext m_glctx;

	void on_event(const SDL_Event &ev);
	void on_windowevent(const SDL_WindowEvent &ev);

protected:
	explicit window(const std::string &title,int width,int height);
	~window() noexcept;

	virtual void on_key(SDL_Keysym keysym,bool down) = 0;
	virtual void on_text(const char *text) = 0;
	virtual void on_mousemove(int x,int y) = 0;
	virtual void on_mousewheel(int y) = 0;
	virtual void on_mousebutton(int button,bool down) = 0;
	virtual void on_resize(int width,int height) = 0;

	virtual void on_frame(int delta_time) = 0;

public:
	void run_once();
};

}
