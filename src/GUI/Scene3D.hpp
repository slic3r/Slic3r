#ifndef SCENE3D_HPP
#define SCENE3D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
    #include <wx/glcanvas.h>
#include "Settings.hpp"
#include "Point.hpp"
#include "3DScene.hpp"
#include "BoundingBox.hpp"
#include "Model.hpp"

namespace Slic3r { namespace GUI {

struct Volume {
    wxColor color;
    Pointf3 origin;
    GLVertexArray model;
    BoundingBoxf3 bb;
};

class Scene3D : public wxGLCanvas {
public:
    Scene3D(wxWindow* parent, const wxSize& size);
private:
    wxGLContext* glContext;
    bool init = false, dirty = true, dragging = false;
    float zoom = 5.0f, phi = 0.0f, theta = 0.0f;
    Point drag_start = Point(0,0);
    Pointf3 _camera_target = Pointf3(0.0f,0.0f,0.0f);
    std::vector<float> bed_verts, grid_verts;
    Points bed_shape;
    BoundingBox bed_bound;
    void resize();
    void repaint(wxPaintEvent &e);
    void init_gl();
    void draw_background();
    void draw_ground();
    void draw_axes(Pointf3 center, float length, int width, bool alwaysvisible);
protected:
    void draw_volumes();
    virtual void mouse_up(wxMouseEvent &e); 
    virtual void mouse_move(wxMouseEvent &e);
    virtual void mouse_dclick(wxMouseEvent &e);
    virtual void mouse_wheel(wxMouseEvent &e);
    virtual void before_render(){};
    virtual void after_render(){};
    Volume load_object(ModelVolume &mv, ModelInstance &mi);
    void set_bed_shape(Points _bed_shape);
    std::vector<Volume> volumes;
 };

} } // Namespace Slic3r::GUI
#endif
