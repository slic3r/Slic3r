#ifndef PLATEROBJECT_HPP
#define PLATEROBJECT_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "ExPolygonCollection.hpp"

namespace Slic3r { namespace GUI {

class PlaterObject {
public:
    wxString name {L""};
    wxString identifier {L""};
    wxString input_file {L""};
    int input_file_obj_idx {-1};

    
    Slic3r::ExPolygonCollection thumbnail;
    Slic3r::ExPolygonCollection transformed_thumbnail;

    // read only
    std::vector<Slic3r::ExPolygonCollection> instance_thumbnails;

    bool selected {false};
    int selected_instance {-1};

    Slic3r::ExPolygonCollection& make_thumbnail(Slic3r::Model model, int obj_index);
    Slic3r::ExPolygonCollection& transform_thumbnail(Slic3r::Model model, int obj_index);
};

}} // Namespace Slic3r::GUI
#endif
