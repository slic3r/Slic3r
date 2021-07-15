#ifndef SLIC3R_HPP
#define SLIC3R_HPP

#include "ConfigBase.hpp"
#include "IO.hpp"
#include "Model.hpp"

namespace Slic3r {

class CLI {
    public:
    int run(int argc, char **argv);

    const FullPrintConfig& full_print_config_ref() { return this->full_print_config; }
    
    private:
    ConfigDef config_def;
    DynamicConfig config;
    DynamicPrintConfig print_config;
    FullPrintConfig full_print_config;
    t_config_option_keys input_files, actions, transforms;
    std::vector<Model> models;
    std::string last_outfile;

    /// Prints usage of the CLI.
    void print_help(bool include_print_options = false) const;
    
    /// Exports loaded models to a file of the specified format, according to the options affecting output filename.
    void export_models(IO::ExportFormat format);
    
    bool has_print_action() const {
        return this->config.has("export_gcode") || this->config.has("export_sla_svg");
    };
    
    std::string output_filepath(const Model &model, IO::ExportFormat format) const;
};

}

#endif
