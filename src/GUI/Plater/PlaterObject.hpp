#ifndef PLATEROBJECT_HPP
#define PLATEROBJECT_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "ExPolygonCollection.hpp"
#include "Model.hpp"

namespace Slic3r { namespace GUI {

class PlaterObject {
public:
    std::string name {""};
    int identifier {0};
    std::string input_file {""};
    int input_file_obj_idx {-1};

    bool selected {false};
    int selected_instance {-1};

    Slic3r::ExPolygonCollection thumbnail;
    Slic3r::ExPolygonCollection transformed_thumbnail;

    // read only
    std::vector<Slic3r::ExPolygonCollection> instance_thumbnails;

    Slic3r::ExPolygonCollection& make_thumbnail(const Slic3r::Model model, int obj_idx);
    Slic3r::ExPolygonCollection& make_thumbnail(const std::shared_ptr<Slic3r::Model> model, int obj_idx);
    
    Slic3r::ExPolygonCollection& transform_thumbnail(const Slic3r::Model model, int obj_idx);
    Slic3r::ExPolygonCollection& transform_thumbnail(const std::shared_ptr<Slic3r::Model> model, int obj_idx);

    bool instance_contains(Slic3r::Point point) const;

    PlaterObject& operator=(const PlaterObject& other);
    PlaterObject& operator=(PlaterObject&& other);

    PlaterObject(const PlaterObject& other) = default;
    PlaterObject() = default;

protected:
    const std::string LogChannel {"PlaterObject"};

};

}} // Namespace Slic3r::GUI
#endif
