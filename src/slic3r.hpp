#ifndef SLIC3R_HPP
#define SLIC3R_HPP

#include "ConfigBase.hpp"
#include "IO.hpp"
#include "Model.hpp"

namespace Slic3r {

class CLI {
    public:
    int run(int argc, char **argv);
    
    private:
    ConfigDef config_def;
    DynamicConfig config;
    DynamicPrintConfig print_config;
    FullPrintConfig full_print_config;
    t_config_option_keys input_files, actions, transforms;
    std::vector<Model> models;
    
    void print_help(bool include_print_options = false) const;
    void export_models(IO::ExportFormat format);
    std::string output_filepath(const Model &model, IO::ExportFormat format) const;
};

}

#endif
