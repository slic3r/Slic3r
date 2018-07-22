#include "Scene3D.hpp"
#include "Line.hpp"
#include "ClipperUtils.hpp"
#include "misc_ui.hpp"
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
namespace Slic3r { namespace GUI {

Scene3D::Scene3D(wxWindow* parent, const wxSize& size) :
    wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, size)
{ 

    this->glContext = new wxGLContext(this);
    this->Bind(wxEVT_PAINT, [this](wxPaintEvent &e) { this->repaint(e); });
    this->Bind(wxEVT_SIZE, [this](wxSizeEvent &e ){
        dirty = true;
        Refresh();
    });


    // Bind the varying mouse events
    this->Bind(wxEVT_MOTION, [this](wxMouseEvent &e) { this->mouse_move(e); });
    this->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_RIGHT_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_MIDDLE_DCLICK, [this](wxMouseEvent &e) { this->mouse_dclick(e); });
    this->Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent &e) { this->mouse_wheel(e); });
    
    Points p;
    const coord_t w = scale_(200), z = 0;
    p.push_back(Point(z,z));
    p.push_back(Point(z,w));
    p.push_back(Point(w,w));
    p.push_back(Point(w,z));
    set_bed_shape(p);
}

float clamp(float low, float x, float high){
    if(x < low) return low;
    if(x > high) return high;
    return x;
}

Linef3 Scene3D::mouse_ray(Point win){
    GLdouble proj[16], mview[16]; 
    glGetDoublev(GL_MODELVIEW_MATRIX, mview);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    GLint view[4];
    glGetIntegerv(GL_VIEWPORT, view);
    win.y = view[3]-win.y;
    GLdouble x = 0.0, y = 0.0, z = 0.0;
    gluUnProject(win.x,win.y,0,mview,proj,view,&x,&y,&z);
    Pointf3 first = Pointf3(x,y,z);
    GLint a = gluUnProject(win.x,win.y,1,mview,proj,view,&x,&y,&z);
    return Linef3(first,Pointf3(x,y,z)); 
}

void Scene3D::mouse_move(wxMouseEvent &e){
    if(e.Dragging()){
        //const auto s = GetSize();
        const auto pos = Point(e.GetX(),e.GetY());
        if(dragging){
            if (e.ShiftDown()) { // TODO: confirm alt -> shift is ok
                // Move the camera center on the Z axis based on mouse Y axis movement
                _camera_target.translate(0, 0, (pos.y - drag_start.y));
            } else if (e.LeftIsDown()) {
                // if dragging over blank area with left button, rotate
                //if (TURNTABLE_MODE) {
                const float TRACKBALLSIZE = 0.8f, GIMBAL_LOCK_THETA_MAX = 170.0f;
                                
                phi += (pos.x - drag_start.x) * TRACKBALLSIZE;
                theta -= (pos.y - drag_start.y) * TRACKBALLSIZE;
                theta = clamp(0, theta, GIMBAL_LOCK_THETA_MAX);
                /*} else {
                    my $size = $self->GetClientSize;
                    my @quat = trackball(
                        $orig->x / ($size->width / 2) - 1,
                        1 - $orig->y / ($size->height / 2),       #/
                        $pos->x / ($size->width / 2) - 1,
                        1 - $pos->y / ($size->height / 2),        #/
                    );
                    $self->_quat(mulquats($self->_quat, \@quat));
                }*/
            } else if (e.MiddleIsDown() || e.RightIsDown()) {
                // if dragging over blank area with right button, translate
                // get point in model space at Z = 0
                const auto current = mouse_ray(pos).intersect_plane(0);
                const auto old = mouse_ray(drag_start).intersect_plane(0);
                _camera_target.translate(current.vector_to(old));
            }
            //$self->on_viewport_changed->() if $self->on_viewport_changed;
            Refresh();
        }
        dragging = true;
        drag_start = pos; 
    }else{
        e.Skip();
    }
}

void Scene3D::mouse_up(wxMouseEvent &e){
    dragging = false;
    Refresh();
}

void Scene3D::mouse_wheel(wxMouseEvent &e){
        // Calculate the zoom delta and apply it to the current zoom factor
        auto _zoom = ((float)e.GetWheelRotation()) / e.GetWheelDelta();
        /*if ($Slic3r::GUI::Settings->{_}{invert_zoom}) {
            _zoom *= -1;
        }*/
        _zoom = clamp(-4, _zoom,4);
        _zoom /= 10;
        zoom /= 1-_zoom;
        /* 
        # In order to zoom around the mouse point we need to translate
        # the camera target
        my $size = Slic3r::Pointf->new($self->GetSizeWH);
        my $pos = Slic3r::Pointf->new($e->GetX, $size->y - $e->GetY); #-
        $self->_camera_target->translate(
            # ($pos - $size/2) represents the vector from the viewport center
            # to the mouse point. By multiplying it by $zoom we get the new,
            # transformed, length of such vector.
            # Since we want that point to stay fixed, we move our camera target
            # in the opposite direction by the delta of the length of such vector
            # ($zoom - 1). We then scale everything by 1/$self->_zoom since 
            # $self->_camera_target is expressed in terms of model units.
            -($pos->x - $size->x/2) * ($zoom) / $self->_zoom,
            -($pos->y - $size->y/2) * ($zoom) / $self->_zoom,
            0,
        ) if 0;
        */

        dirty = true;
        Refresh();
}

void Scene3D::mouse_dclick(wxMouseEvent &e){
    /*
    if (@{$self->volumes}) {
        $self->zoom_to_volumes;
    } else {
        $self->zoom_to_bed;
    }*/
    
    dirty = true;
    Refresh();
}

void Scene3D::resize(){
    if(!dirty)return;
    dirty = false;
    const auto s = GetSize();
    glViewport(0,0,s.GetWidth(),s.GetHeight());
    const auto x = s.GetWidth()/zoom,
               y = s.GetHeight()/zoom,
               depth = 1000.0f; // my $depth = 10 * max(@{ $self->max_bounding_box->size });
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(
        -x/2, x/2, -y/2, y/2,
        -depth, 2*depth
    );
    glMatrixMode(GL_MODELVIEW);
}

void Scene3D::set_bed_shape(Points _bed_shape){
    
    bed_shape = _bed_shape;
    const float GROUND_Z = -0.02f;
    
    // triangulate bed
    const auto expoly = ExPolygon(Polygon(bed_shape));
    const auto box = expoly.bounding_box();
    bed_bound = box;
    {
        std::vector<Polygon> triangles;
        expoly.triangulate(&triangles);
        bed_verts.clear();
        for(const auto &triangle : triangles){
            for(const auto &point : triangle.points){
                bed_verts.push_back(unscale(point.x));
                bed_verts.push_back(unscale(point.y));
                bed_verts.push_back(GROUND_Z);
            }
        }
    }
    {
        std::vector<Polyline> lines;
        Points tmp;
        for (coord_t x = box.min.x; x <= box.max.x; x += scale_(10)) {
            lines.push_back(Polyline());
            lines.back().append(Point(x,box.min.y));
            lines.back().append(Point(x,box.max.y));
        }
        for (coord_t y = box.min.y; y <= box.max.y; y += scale_(10)) {
            lines.push_back(Polyline());
            lines.back().append(Point(box.min.x,y));
            lines.back().append(Point(box.max.x,y));
        }
        // clip with a slightly grown expolygon because our lines lay on the contours and
        // may get erroneously clipped
        // my @lines = map Slic3r::Line->new(@$_[0,-1]),
        grid_verts.clear();
        const Polylines clipped = intersection_pl(lines,offset_ex(expoly,SCALED_EPSILON).at(0));
        for(const Polyline &line : clipped){
            for(const Point &point : line.points){
                grid_verts.push_back(unscale(point.x));
                grid_verts.push_back(unscale(point.y));
                grid_verts.push_back(GROUND_Z);
            }
        }
        // append bed contours
        for(const Line &line : expoly.lines()){
            grid_verts.push_back(unscale(line.a.x));
            grid_verts.push_back(unscale(line.a.y));
            grid_verts.push_back(GROUND_Z);
            grid_verts.push_back(unscale(line.b.x));
            grid_verts.push_back(unscale(line.b.y));
            grid_verts.push_back(GROUND_Z);
        }
    }
    
    //$self->origin(Slic3r::Pointf->new(0,0));
}

void Scene3D::init_gl(){
    if(this->init)return;
    this->init = true;

    glClearColor(0, 0, 0, 1);
    glColor3f(1, 0, 0);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set antialiasing/multisampling
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
    //glEnable(GL_MULTISAMPLE) if ($self->{can_multisample});
    
    // ambient lighting
    GLfloat ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    
    // light from camera
    GLfloat pos[] = {1.0f, 0.0f, 1.0f, 0.0f}, spec[] = {0.8f, 0.8f, 0.8f, 1.0f}, diff[] = {0.4f, 0.4f, 0.4f, 1.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, pos);
    glLightfv(GL_LIGHT1, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  diff);
    
    // Enables Smooth Color Shading; try GL_FLAT for (lack of) fun. Default: GL_SMOOTH
    glShadeModel(GL_SMOOTH);
    
    GLfloat fbdiff[] = {0.3f, 0.3f, 0.3f,1}, fbspec[] = {1.0f, 1.0f, 1.0f, 1.0f}, fbemis[] = {0.1f,0.1f,0.1f,0.9f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, fbdiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, fbspec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, fbemis);
    
    // A handy trick -- have surface material mirror the color.
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    //glEnable(GL_MULTISAMPLE) if ($self->{can_multisample});
}

void Scene3D::draw_background(){
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glBegin(GL_QUADS);
    auto bottom = ui_settings->color->BOTTOM_COLOR(), top = ui_settings->color->TOP_COLOR();
    if(ui_settings->color->SOLID_BACKGROUNDCOLOR()){
         bottom = top = ui_settings->color->BACKGROUND_COLOR();
    }
    glColor3ub(bottom.Red(), bottom.Green(), bottom.Blue());
    glVertex2f(-1.0,-1.0);
    glVertex2f(1,-1.0);
    glColor3ub(top.Red(), top.Green(), top.Blue());
    glVertex2f(1, 1);
    glVertex2f(-1.0, 1);
    glEnd();
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void Scene3D::draw_ground(){
    glDisable(GL_DEPTH_TEST);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    /*my $triangle_vertex;
    if (HAS_VBO) {
        ($triangle_vertex) =
            glGenBuffersARB_p(1);
        $self->bed_triangles->bind($triangle_vertex);
        glBufferDataARB_p(GL_ARRAY_BUFFER_ARB, $self->bed_triangles, GL_STATIC_DRAW_ARB);
        glVertexPointer_c(3, GL_FLOAT, 0, 0);
    } else {*/
    // fall back on old behavior
    glVertexPointer(3, GL_FLOAT, 0, bed_verts.data());
    const auto ground = ui_settings->color->GROUND_COLOR(), grid = ui_settings->color->GRID_COLOR();
        
    glColor4ub(ground.Red(), ground.Green(), ground.Blue(),ground.Alpha());
    glNormal3d(0,0,1);
    glDrawArrays(GL_TRIANGLES, 0, bed_verts.size() / 3);
    
    // we need depth test for grid, otherwise it would disappear when looking
    // the object from below
    glEnable(GL_DEPTH_TEST);

    // draw grid
    glLineWidth(2);
    /*my $grid_vertex;
    if (HAS_VBO) {
        ($grid_vertex) =
            glGenBuffersARB_p(1);
        $self->bed_grid_lines->bind($grid_vertex);
        glBufferDataARB_p(GL_ARRAY_BUFFER_ARB, $self->bed_grid_lines, GL_STATIC_DRAW_ARB);
        glVertexPointer_c(3, GL_FLOAT, 0, 0);
    } else {*/
    // fall back on old behavior
    glVertexPointer(3, GL_FLOAT, 0, grid_verts.data());
    
    glColor4ub(grid.Red(), grid.Green(), grid.Blue(),grid.Alpha());
    glNormal3d(0,0,1);
    glDrawArrays(GL_LINES, 0, grid_verts.size() / 3);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glDisable(GL_BLEND);
    /*if (HAS_VBO) { 
        # Turn off buffer objects to let the rest of the draw code work.
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        glDeleteBuffersARB_p($grid_vertex);
        glDeleteBuffersARB_p($triangle_vertex);
    }*/
}

void Scene3D::draw_axes (Pointf3 center, float length, int width, bool always_visible){
    /*
    my $volumes_bb = $self->volumes_bounding_box;
    
    {
        # draw axes
        # disable depth testing so that axes are not covered by ground
        glDisable(GL_DEPTH_TEST);
        my $origin = $self->origin;
        my $axis_len = max(
            max(@{ $self->bed_bounding_box->size }),
            1.2 * max(@{ $volumes_bb->size }),
        );
        glLineWidth(2);
        glBegin(GL_LINES);
        # draw line for x axis
        glColor3f(1, 0, 0);
        glVertex3f(@$origin, $ground_z);
        glVertex3f($origin->x + $axis_len, $origin->y, $ground_z);  #,,
        # draw line for y axis
        glColor3f(0, 1, 0);
        glVertex3f(@$origin, $ground_z);
        glVertex3f($origin->x, $origin->y + $axis_len, $ground_z);  #++
        glEnd();
        # draw line for Z axis
        # (re-enable depth test so that axis is correctly shown when objects are behind it)
        glEnable(GL_DEPTH_TEST);
        glBegin(GL_LINES);
        glColor3f(0, 0, 1);
        glVertex3f(@$origin, $ground_z);
        glVertex3f(@$origin, $ground_z+$axis_len);
        glEnd();
    }
   */ 
    if (always_visible) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
    }
    glLineWidth(width);
    glBegin(GL_LINES);
    // draw line for x axis
    glColor3f(1, 0, 0);
    glVertex3f(center.x, center.y, center.z);
    glVertex3f(center.x + length, center.y, center.z);
    // draw line for y axis
    glColor3f(0, 1, 0);
    glVertex3f(center.x, center.y, center.z);
    glVertex3f(center.x, center.y + length, center.z);
    // draw line for Z axis
    glColor3f(0, 0, 1);
    glVertex3f(center.x, center.y, center.z);
    glVertex3f(center.x, center.y, center.z + length);
    glEnd(); 
    glEnable(GL_DEPTH_TEST);
}
void Scene3D::draw_volumes(){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    
    for(const Volume &volume : volumes){
        glPushMatrix();
        glTranslatef(volume.origin.x, volume.origin.y, volume.origin.z);
        glCullFace(GL_BACK);
        glVertexPointer(3, GL_FLOAT, 0, volume.model.verts.data());
        glNormalPointer(GL_FLOAT, 0, volume.model.norms.data());
        glColor4ub(volume.color.Red(), volume.color.Green(), volume.color.Blue(), volume.color.Alpha());
        glDrawArrays(GL_TRIANGLES, 0, volume.model.verts.size()/3);
        glPopMatrix();
    }
    
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

void Scene3D::repaint(wxPaintEvent& e) {
    if(!this->IsShownOnScreen())return;
    // There should be a context->IsOk check once wx is updated
    if(!this->SetCurrent(*(this->glContext)))return;
    init_gl();
    resize();

    glClearColor(1, 1, 1, 1);
    glClearDepth(1);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glRotatef(-theta, 1, 0, 0); // pitch
    glRotatef(phi, 0, 0, 1);   // yaw
    /*} else {
        my @rotmat = quat_to_rotmatrix($self->quat);
        glMultMatrixd_p(@rotmat[0..15]);
    }*/
    glTranslatef(-_camera_target.x, -_camera_target.y, -_camera_target.z);
     
    // light from above
    GLfloat pos[] = {-0.5f, -0.5f, 1.0f, 0.0f}, spec[] = {0.2f, 0.2f, 0.2f, 1.0f}, diff[] = {0.5f, 0.5f, 0.5f, 1.0f};    
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff);

    before_render();

    draw_background();
    draw_ground();
        /*my $origin = $self->origin;
        my $axis_len = max(
            max(@{ $self->bed_bounding_box->size }),
            1.2 * max(@{ $volumes_bb->size }),
        );*/
    draw_axes(Pointf3(0.0f,0.0f,0.0f),
    unscale(bed_bound.radius()),2,true/*origin,calulcated length,2, true*/);
    
    // draw objects
    glEnable(GL_LIGHTING);
    draw_volumes();
    
    after_render();
    
    if (dragging/*defined $self->_drag_start_pos || defined $self->_drag_start_xy*/) {
        draw_axes(_camera_target, 10.0f, 1, true/*camera,10,1,true*/);
        draw_axes(_camera_target, 10.0f, 4, false/*camera,10,4,false*/);
    }

    glFlush();
    SwapBuffers();
    // Calling glFinish has a performance penalty, but it seems to fix some OpenGL driver hang-up with extremely large scenes.
    glFinish();

}

} } // Namespace Slic3r::GUI
