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

#pragma once

/*
 * render.hh
 *
 * FIXME explain
 */

#include <unordered_set>
#include "res.hh"
#include "gfx.hh"
#include "gui.hh"

namespace drnsf {
namespace render {

/*
 * render::camera
 *
 * A camera configuration for use in viewport widgets. Note that this is not
 * some kind of in-game camera structure, only a camera for rendering inside
 * the application.
 */
struct camera {
    static constexpr float default_yaw = 330.0f;
    static constexpr float default_pitch = -30.0f;
    static constexpr float default_distance = 5000.0f;

    static constexpr float min_distance = 500.0f;

    glm::vec3 pivot = { 0.0f, 0.0f, 0.0f };
    float yaw = default_yaw;
    float pitch = default_pitch;
    float distance = default_distance;
};

// defined later in this file
class figure;

/*
 * render::viewport
 *
 * FIXME explain
 */
class viewport : private gui::composite {
    friend class figure;

private:
    // inner class defined in render_viewport.cc
    class impl;

    // (var) M
    // The pointer to the internal implementation object (PIMPL).
    impl *M;

    // (var) m_figs
    // The set of all figures associated with the viewport, both visible
    // and invisible.
    std::unordered_set<figure *> m_figs;

    // (func) invalidate
    // Used by `render::figure'. Marks the viewport's display as invalid
    // or "dirty" so that it will be re-rendered when necessary.
    void invalidate();

public:
    // (ctor)
    // Constructs an empty viewport widget and places it in the given
    // parent container.
    viewport(gui::container &parent, gui::layout layout);

    // (dtor)
    // Destroys the widget, removing it from the parent container.
    ~viewport();

    using composite::show;
    using composite::hide;
    using composite::get_layout;
    using composite::set_layout;
    using composite::get_real_size;
    using composite::get_screen_pos;
};

/*
 * render::figure
 *
 * FIXME explain
 */
class figure : private util::nocopy {
    friend class viewport;

protected:
    // (inner class) env
    // This structure is used to pass information to the `draw' method
    // without the need to change the function signature every time new
    // information needs to be passed through.
    struct env {
        // (var) projection
        // The projection matrix.
        glm::mat4 projection;

        // (var) view
        // The view matrix.
        glm::mat4 view;

        // (var) view
        // The view matrix, but not translated according to the camera's x/y/z
        // pivot position. This could be used place a model such that it is
        // always located at the pivot point, as is done by the reticle.
        glm::mat4 view_nomove;
    };

private:
    // (var) m_vp
    // A reference to the viewport this figure exists within.
    viewport &m_vp;

    // (var) m_visible
    // True if the figure is visible; false otherwise. A figure which is
    // visible must invalidate its viewport whenever it changes, however a
    // hidden one need not do this (as it is invisible, and therefore does
    // not affect the scene). The viewport must also be invalidated when
    // the visibility of a figure changes.
    bool m_visible = false;

    // (pure func) draw
    // Derived classes must implement this method to draw themselves in
    // whatever way is appropriate. The view and projection matrices are given
    // as parameters.
    virtual void draw(const env &e) = 0;

protected:
    // (explicit ctor)
    // Constructs a figure which is associated with the given viewport. The
    // figure is initially not visible.
    explicit figure(viewport &vp);

    // (dtor)
    // Destroys the figure and removes it from the viewport. If the figure
    // was visible, the viewport is invalidated.
    ~figure();

    // (func) invalidate
    // Derived classes should call this function whenever their visual
    // appearance may have changed (such as a moved vertex, color change,
    // texture change, etc). Calling this function on a visible figure will
    // cause the associated viewport to be invalidated (marked as stale or
    // "dirty" so that it will be redrawn).
    void invalidate();

public:
    // (func) show
    // Makes the figure visible, if it was not visible. A change in
    // visibility will invalidate the viewport.
    void show();

    // (func) hide
    // Makes the figure invisible, if it was visible. A change in
    // visibility will invalidate the viewport.
    void hide();
};

/*
 * render::reticle_fig
 *
 * This figure draws a 400 x 400 x 400 wireframe cube with the origin at its
 * center.
 */
class reticle_fig : public figure {
private:
    // (var) m_matrix
    // The model matrix for this figure. The view and projection matrix come
    // from the viewport during a `draw' call.
    glm::mat4 m_matrix{1.0f};

    // (func) draw
    // Implements `figure::draw'.
    void draw(const env &e) override;

public:
    // (explicit ctor)
    // Constructs the reticle figure.
    explicit reticle_fig(viewport &vp) :
        figure(vp) {}

    // (func) get_matrix, set_matrix
    // Gets or sets the model matrix (m_matrix above).
    const glm::mat4 &get_matrix() const;
    void set_matrix(glm::mat4 matrix);
};

/*
 * render::frameonly_fig
 *
 * This figure draws a bunch of points corresponding to the vertex positions in
 * a given gfx::frame.
 *
 * The frame is given directly by pointer. Any code using this class must ensure
 * the current frame is valid at all times.
 */
class frameonly_fig : public figure {
private:
    // (var) m_frame
    // A non-ref pointer to the frame used by this figure.
    gfx::frame *m_frame = nullptr;

    // (var) m_matrix
    // The model matrix for this figure. The view and projection matrix come
    // from the viewport during a `draw' call.
    glm::mat4 m_matrix{1.0f};

    // (func) draw
    // Implements `figure::draw'.
    void draw(const env &e) override;

    // (handler) h_frame_vertices_change
    // Hooks the frame's vertices property change event so that the figure can
    // be updated when the frame's vertices are changed.
    decltype(decltype(gfx::frame::p_vertices)::on_change)::watch
        h_frame_vertices_change;

public:
    // (explicit ctor)
    // Constructs the figure. Initially, it does not reference any frame.
    explicit frameonly_fig(viewport &vp);

    // (func) get_frame, set_frame
    // Gets or sets the gfx::frame reference used by this figure. This may be a
    // null pointer, in which case nothing will be drawn.
    gfx::frame * const &get_frame() const;
    void set_frame(gfx::frame *frame);

    // (func) get_matrix, set_matrix
    // Gets or sets the model matrix (m_matrix above).
    const glm::mat4 &get_matrix() const;
    void set_matrix(glm::mat4 matrix);
};

/*
 * render::animonly_fig
 *
 * This figure draws a bunch of points corresponding to the vertex positions in
 * a given gfx::anim. Only the first frame of animation is drawn at this time.
 *
 * The anim is given directly by pointer. Any code using this class must ensure
 * the anim pointer is valid or null at all times. The frames in the anim need
 * not be valid, however; animonly_fig will track their validity itself and
 * handle any changes in their values or lifetime.
 *
 * This class is only a pseudo-figure. Rather than implementing the `figure'
 * class itself, this class drives an underlying frameonly_fig figure which
 * renders one frame of the given animation.
 */
class animonly_fig : private util::nocopy {
private:
    // (var) m_anim
    // A non-ref pointer to the anim used by this figure.
    gfx::anim *m_anim = nullptr;

    // (var) m_framefig
    // The underlying figure which performs the actual rendering for this class.
    // The animonly_fig drives the frameonly_fig by providing the frame pointers
    // specified in the animation.
    frameonly_fig m_framefig;

    // (var) m_frame_tracker
    // A helper object which tracks the frame ref and provides the actual frame
    // pointers needed by `m_framefig'.
    res::tracker<gfx::frame> m_frame_tracker;

    // (handler) h_anim_frames_change
    // Hooks the anim's frames property change event to keep track of the anim's
    // frames so that the figure can update itself when they change.
    decltype(decltype(gfx::anim::p_frames)::on_change)::watch
        h_anim_frames_change;

public:
    // (explicit ctor)
    // Constructs the figure. Initially, it does not reference any animation.
    explicit animonly_fig(viewport &vp);

    // (func) show, hide
    // Changes the visibility of the figure. Since this is not a real figure,
    // the show/hide calls are passed on to the actual underlying figure.
    void show();
    void hide();

    // (func) get_anim, set_anim
    // Gets or sets the gfx::anim reference used by this figure. This may be a
    // null pointer, in which case nothing will be drawn.
    gfx::anim * const &get_anim() const;
    void set_anim(gfx::anim *anim);

    // (func) get_matrix, set_matrix
    // Gets or sets the model matrix (held inside of the frameonly_fig).
    const glm::mat4 &get_matrix() const;
    void set_matrix(glm::mat4 matrix);
};

/*
 * render::meshframe_fig
 *
 * This figure draws a gfx::mesh / gfx::frame pairing.
 *
 * The frame and mesh are given directly by pointer. Any code which uses this
 * class must ensure that any non-null frame or mesh pointers are valid at all
 * times.
 */
class meshframe_fig : public figure {
private:
    // (var) m_mesh
    // A pointer to the mesh used by this figure. This may be null, in which
    // case rendering will not occur.
    gfx::mesh *m_mesh = nullptr;

    // (var) m_frame
    // A pointer to the frame used by this figure. This may be null, in which
    // case rendering will not occur.
    gfx::frame *m_frame = nullptr;

    // (var) m_matrix
    // The model matrix for this figure. The view and projection matrix come
    // from the viewport during a `draw' call.
    glm::mat4 m_matrix{1.0f};

    // (func) draw
    // Implements `figure::draw'.
    void draw(const env &e) override;

    // (handler) h_mesh_triangles_change, h_mesh_quads_change,
    //           h_mesh_colors_change
    // Hooks the relevant mesh properties' change events so that the figure can
    // update itself when they change.
    decltype(decltype(gfx::mesh::p_triangles)::on_change)::watch
        h_mesh_triangles_change;
    decltype(decltype(gfx::mesh::p_quads)::on_change)::watch
        h_mesh_quads_change;
    decltype(decltype(gfx::mesh::p_colors)::on_change)::watch
        h_mesh_colors_change;

    // (handler) h_frame_vertices_change
    // Hooks the frame's vertices property change event so that the figure can
    // update itself when a change occurs.
    decltype(decltype(gfx::frame::p_vertices)::on_change)::watch
        h_frame_vertices_change;

public:
    // (explicit ctor)
    // Constructs the figure. Initially, it does not reference any mesh
    // or frame.
    explicit meshframe_fig(viewport &vp);

    // (func) get_mesh, set_mesh
    // Gets or sets the gfx::mesh reference used by this figure. This may be a
    // null reference.
    gfx::mesh * const &get_mesh() const;
    void set_mesh(gfx::mesh *mesh);

    // (func) get_frame, set_frame
    // Gets or sets the gfx::frame reference used by this figure. This may be a
    // null reference.
    gfx::frame * const &get_frame() const;
    void set_frame(gfx::frame *frame);

    // (func) get_matrix, set_matrix
    // Gets or sets the model matrix (m_matrix above).
    const glm::mat4 &get_matrix() const;
    void set_matrix(glm::mat4 matrix);
};

/*
 * render::meshanim_fig
 *
 * This figure draws a gfx::mesh / gfx::anim pairing. Only the first frame of
 * animation is used at this time.
 *
 * The anim and mesh are given directly by pointer. Any code which uses this
 * class must ensure that any non-null anim or mesh pointers are valid at all
 * times. The frames of the anim need not be valid, however; meshframe_fig will
 * track their validity itself and handle any changes in their values or
 * lifetime.
 *
 * This class is only a pseudo-figure. Rather than implementing the `figure'
 * class itself, this class drives an underlying meshframe_fig figure which
 * renders one frame of the given animation.
 */
class meshanim_fig : private util::nocopy {
private:
    // (var) m_anim
    // A pointer to the anim used by this figure. This may be null, in which
    // case rendering will not occur.
    gfx::anim *m_anim = nullptr;

    // (var) m_meshframefig
    // The underlying figure which performs the actual rendering for this class.
    // The meshanim_fig drives the meshframe_fig by providing the frame pointers
    // specified in the animation.
    meshframe_fig m_meshframefig;

    // (var) m_frame_tracker
    // A helper object which tracks the frame ref and provides the actual frame
    // pointers needed by `m_meshframefig'.
    res::tracker<gfx::frame> m_frame_tracker;

    // (handler) h_anim_frames_change
    // Hooks the anim's frames property change event to keep track of the anim's
    // frames so that the figure can update itself when they change.
    decltype(decltype(gfx::anim::p_frames)::on_change)::watch
        h_anim_frames_change;

public:
    // (explicit ctor)
    // Constructs the figure. Initially, it does not reference any mesh
    // or frame.
    explicit meshanim_fig(viewport &vp);

    // (func) show, hide
    // Changes the visibility of the figure. Since this is not a real figure,
    // the show/hide calls are passed on to the actual underlying figure.
    void show();
    void hide();

    // (func) get_mesh, set_mesh
    // Gets or sets the gfx::mesh reference used by this figure. This may be a
    // null pointer.
    gfx::mesh * const &get_mesh() const;
    void set_mesh(gfx::mesh *mesh);

    // (func) get_anim, set_anim
    // Gets or sets the gfx::anim reference used by this figure. This may be a
    // null pointer.
    gfx::anim * const &get_anim() const;
    void set_anim(gfx::anim *anim);

    // (func) get_matrix, set_matrix
    // Gets or sets the model matrix (held inside of the meshframe_fig).
    const glm::mat4 &get_matrix() const;
    void set_matrix(glm::mat4 matrix);
};

/*
 * render::model_fig
 *
 * This figure draws a gfx::model. Only the first frame of animation is used at
 * this time.
 *
 * The model is given directly by pointer. Any code which uses this class must
 * ensure that the model pointer is valid or null at all times. The mesh and
 * anim referenced by the model need not be valid, however; model_fig will track
 * their validity itself and handle any changes in their values or lifetime.
 *
 * This class is onyl a pseudo-figure. Rather than implementing the `figure'
 * class itself, this class drives an underlying `meshanim_fig' object.
 */
class model_fig : private util::nocopy {
private:
    // (var) m_model
    // A pointer to the model used by this figure. This may be null, in which
    // case rendering will not occur.
    gfx::model *m_model = nullptr;

    // (var) m_meshanimfig
    // The underlying figure which handles rendering the selected model's mesh
    // and animation. The model_fig drives the meshanim_fig by providing the
    // mesh and animation pointers specified in the model.
    meshanim_fig m_meshanimfig;

    // (var) m_mesh_tracker, m_anim_tracker
    // Helper objects which track the mesh and anim refs in the model and
    // provide the actual pointers needed by `m_meshanimfig'.
    res::tracker<gfx::mesh> m_mesh_tracker;
    res::tracker<gfx::anim> m_anim_tracker;

    // (handler) h_model_mesh_change, h_model_anim_change
    // Hooks the model's mesh and anim property change events to keep track of
    // the model's mesh and anim so that the figure can update itself when they
    // change.
    decltype(decltype(gfx::model::p_mesh)::on_change)::watch
        h_model_mesh_change;
    decltype(decltype(gfx::model::p_anim)::on_change)::watch
        h_model_anim_change;

public:
    // (explicit ctor)
    // Constructs the figure. Initially, it does not reference any model.
    explicit model_fig(viewport &vp);

    // (func) show, hide
    // Changes the visibility of the figure. Since this is not a real figure,
    // the show/hide calls are passed on to the actual underlying figure.
    void show();
    void hide();

    // (func) get_model, set_model
    // Gets or sets the gfx::model reference used by this figure. This may be a
    // null pointer.
    gfx::model * const &get_model() const;
    void set_model(gfx::model *model);

    // (func) get_matrix, set_matrix
    // Gets or sets the model matrix.
    const glm::mat4 &get_matrix() const;
    void set_matrix(glm::mat4 matrix);
};

/*
 * render::world_fig
 *
 * This figure draws a gfx::world's model at that world's location. Only the
 * first frame of the model's animation is used at this time.
 *
 * The world is given directly by pointer. Any code which uses this class must
 * ensure that the world pointer is valid or null at all times. The model need
 * not be valid, however; world_fig will track the validity itself and handle
 * any changes to the model ref or the model's lifetime.
 *
 * This class is only a pseudo-figure. Rather than implementing the `figure'
 * class itself, this class drives an underlying `model_fig' object.
 */
class world_fig : private util::nocopy {
private:
    // (var) m_world
    // A pointer to the world used by this figure. This may be null, in which
    // case rendering will not occur.
    gfx::world *m_world = nullptr;

    // (var) m_modelfig
    // The underlying figure which handles rendering the selected world's model.
    // The world_fig drives the model_fig by providing the model pointer and
    // world position specified in the world.
    model_fig m_modelfig;

    // (var) m_model_tracker
    // A helper object which tracks the model ref in the world and provides the
    // actual pointers needed by `m_modelfig'.
    res::tracker<gfx::model> m_model_tracker;

    // (var) m_matrix
    // The model matrix used by this figure. The actual model matrix used when
    // drawing is held in `m_modelfig', but the original matrix set on the
    // world_fig is stored here so it can be reused later if the world X/Y/Z
    // position changes.
    glm::mat4 m_matrix{1.0f};

    // (handler) h_world_model_change
    // Hooks the world's model property change event to keep track of the
    // world's model ref so that the figure can update itself when it changes.
    decltype(decltype(gfx::world::p_model)::on_change)::watch
        h_world_model_change;

    // (handler) h_world_x_change, h_world_y_change, h_world_z_change
    // Hooks the world's x/y/z property change events to update the figure when
    // the world's position changes.
    decltype(decltype(gfx::world::p_x)::on_change)::watch
        h_world_x_change;
    decltype(decltype(gfx::world::p_y)::on_change)::watch
        h_world_y_change;
    decltype(decltype(gfx::world::p_z)::on_change)::watch
        h_world_z_change;

public:
    // (explicit ctor)
    // Constructs the figure. Initially, it does not reference any world.
    explicit world_fig(viewport &vp);

    // (func) show, hide
    // Changes the visibility of the figure. Since this is not a real figure,
    // the show/hide calls are passed on to the actual underlying figure.
    void show();
    void hide();

    // (func) get_world, set_world
    // Gets or sets the gfx::world reference used by this figure. This may be a
    // null pointer.
    gfx::world * const &get_world() const;
    void set_world(gfx::world *world);

    // (func) get_matrix, set_matrix
    // Gets or sets the model matrix. The matrix passed to the underlying
    // model_fig is based on this matrix and the world's x/y/z position.
    const glm::mat4 &get_matrix() const;
    void set_matrix(glm::mat4 matrix);
};

}
}
