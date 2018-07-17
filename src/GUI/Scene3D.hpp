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
    
    // Camera settings
    float zoom = 5.0f, phi = 0.0f, theta = 0.0f;
    Pointf3 _camera_target = Pointf3(0.0f,0.0f,0.0f);
    
    // Optional point used for dragging calculations
    bool dragging = false;
    Point drag_start = Point(0,0);
    
    // Bed Stuff
    std::vector<float> bed_verts, grid_verts;
    Points bed_shape;
    BoundingBox bed_bound;
    
    void repaint(wxPaintEvent &e); // Redraws every frame
    
    bool dirty = true;             // Resize needs to be called before render
    void resize();                 // Handle glViewport and projection matrices
    
    bool init = false;             // Has opengl been initted
    void init_gl();                // Handles lights and materials
    
    // Useded in repaint
    void draw_background();
    void draw_ground();
    void draw_axes(Pointf3 center, float length, int width, bool alwaysvisible);
    
protected:
    Linef3 mouse_ray(Point win); // Utility for backtracking from window coordinates
    void draw_volumes();         // Draws volumes (for use in before_render)
    void set_bed_shape(Points _bed_shape);
    
    std::vector<Volume> volumes;
    Volume load_object(ModelVolume &mv, ModelInstance &mi);
    
    // Virtual methods to override
    virtual void mouse_up(wxMouseEvent &e);
    virtual void mouse_move(wxMouseEvent &e);
    virtual void mouse_dclick(wxMouseEvent &e);
    virtual void mouse_wheel(wxMouseEvent &e);
    virtual void before_render(){};
    virtual void after_render(){};
 };

} } // Namespace Slic3r::GUI
#endif
