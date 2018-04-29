#ifndef PLATER2DOBJECT_HPP
#define PLATER2DOBJECT_HPP

namespace Slic3r { namespace GUI {
// 2D Preview of an object
class Plater2DObject {
public:
    std::string name {""};
    std::string identifier {""};
    bool selected {false};
};
} } // Namespace Slic3r::GUI
#endif
