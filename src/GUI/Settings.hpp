#ifndef SETTINGS_HPP
#define SETTINGS_HPP
namespace Slic3r { namespace GUI {

/// Stub class to hold onto GUI-specific settings options. 
/// TODO: Incorporate a copy of Slic3r::Config 
class Settings { 
    public:
        bool show_host;
        Settings(): show_host(false) {} //< Show host/controller tab
};

}} //namespace Slic3r::GUI

#endif // SETTINGS_HPP
