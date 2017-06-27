#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;
use Test::More; #Todo add the number of tests
use IO::Uncompress::Unzip qw(unzip $UnzipError) ;

{
    use Cwd qw(abs_path);
    use File::Basename qw(dirname);
    my $path = abs_path($0);
    my $amf_test_file = dirname($path). "/amf/FaceColors.amf.xml";
    my $tmf_output_file = dirname($path). "/3mf/FaceColors.3mf";
    my $expected_model = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        ."<model unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" xmlns:m=\"http://schemas.microsoft.com/3dmanufacturing/material/2015/02\" xmlns:slic3r=\"http://link_to_Slic3r_schema.com/2017/06\"> \n"
        ."    <slic3r:metadata type=\"version\">1.3.0-dev</slic3r:metadata>\n"
        ."    <resources> \n"
        ."        <object id=\"1\" type=\"model\">\n"
        ."            <mesh>\n"
        ."                <vertices>\n"
        ."                    <vertex x=\"-1\" y=\"-0.999998\" z=\"0.999998\"/>\n"
        ."                    <vertex x=\"-1\" y=\"-0.999998\" z=\"-1\"/>\n"
        ."                    <vertex x=\"1\" y=\"-1\" z=\"-0.999998\"/>\n"
        ."                    <vertex x=\"0.999996\" y=\"-1\" z=\"1\"/>\n"
        ."                    <vertex x=\"-1\" y=\"1\" z=\"0.999997\"/>\n"
        ."                    <vertex x=\"1\" y=\"0.999998\" z=\"1\"/>\n"
        ."                    <vertex x=\"1\" y=\"0.999998\" z=\"-0.999998\"/>\n"
        ."                    <vertex x=\"-0.999995\" y=\"1\" z=\"-1\"/>\n"
        ."                </vertices>\n"
        ."                <triangles>\n"
        ."                    <triangle v1=\"0\" v2=\"1\" v3=\"2\"/>\n"
        ."                    <triangle v1=\"0\" v2=\"2\" v3=\"3\"/>\n"
        ."                    <triangle v1=\"4\" v2=\"5\" v3=\"6\"/>\n"
        ."                    <triangle v1=\"4\" v2=\"6\" v3=\"7\"/>\n"
        ."                    <triangle v1=\"0\" v2=\"4\" v3=\"7\"/>\n"
        ."                    <triangle v1=\"0\" v2=\"7\" v3=\"1\"/>\n"
        ."                    <triangle v1=\"1\" v2=\"7\" v3=\"6\"/>\n"
        ."                    <triangle v1=\"1\" v2=\"6\" v3=\"2\"/>\n"
        ."                    <triangle v1=\"2\" v2=\"6\" v3=\"5\"/>\n"
        ."                    <triangle v1=\"2\" v2=\"5\" v3=\"3\"/>\n"
        ."                    <triangle v1=\"4\" v2=\"0\" v3=\"3\"/>\n"
        ."                    <triangle v1=\"4\" v2=\"3\" v3=\"5\"/>\n"
        ."                </triangles>\n"
        ."                <slic3r:volumes>\n"
        ."                    <slic3r:volume ts=\"0\" te=\"11\" modifier=\"0\" >\n"
        ."                    </slic3r:volume>\n"
        ."                </slic3r:volumes>\n"
        ."            </mesh>\n"
        ."        </object>\n"
        ."    </resources> \n"
        ."    <build> \n"
        ."        <item objectid=\"1\" transform=\"1 0 0 -0 1 0 0 0 1 0 0 0\"/>\n"
        ."    </build> \n"
        ."</model>";


    my $model = Slic3r::Model->add_object;
    ok($model, 'Model initiated');
    # Read a file.
    $model->read_from_file($amf_test_file);
    # Write in 3MF format.
    Slic3r::Model->write_tmf($model, $tmf_output_file);
    # unzip from zip file.
    my $output ;
    unzip $tmf_output_file => \$output, Name => "3D/3dmodel.model"
        or die "unzip failed: $UnzipError\n";
    # check contents in 3dmodel.model
    is( $output, $expected_model, "3dmodel.model file equals");
    # check contents in content_types.xml

    # check contents in _rels.xml

    # finish finish test cases.
    done_testing();
}

__END__
