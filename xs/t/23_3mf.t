#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;
use Test::More;
use IO::Uncompress::Unzip qw(unzip $UnzipError) ;
use Cwd qw(abs_path);
use File::Basename qw(dirname);

my $current_path = abs_path($0);
my $expected_content_types = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n"
    ."<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
    ."<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
    ."<Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n"
    ."</Types>\n";
my $expected_relationships = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n"
    ."<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
    ."<Relationship Id=\"rel0\" Target=\"/3D/3dmodel.model\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" /></Relationships>\n";

sub multiply_matrix{

}

# Test 1: Check read/write.
{
    my $input_path = dirname($current_path). "/models/3mf/box.3mf";
    my $output_path = dirname($current_path). "/models/3mf/box2.3mf";
    # Create a new model.
    my $model = Slic3r::Model->new;

    my $result = $model->read_tmf($input_path);
    is($result, 1, 'Basic 3mf read test.');

    $result = $model->write_tmf($output_path);
    is($result, 1, 'Basic 3mf write test.');

    # Delete the created file.
    unlink($output_path);
}

# Test 2: Check read metadata/ materials/ objects/ components/ build items w/o or with tansformation matrics. // ToDo @Samir55 Improve.
{
    my $input_path = dirname($current_path). "/models/3mf/gimblekeychain.3mf";
    # Create a new model.
    my $model = Slic3r::Model->new;
    $model->read_tmf($input_path);

    # Check the number of read matadata.
    is($model->metadata_count(), 8, 'Test 2: Metadata count check.');

    # Check the number of read materials.
    #is($model->material_count(), 1, 'Test 2: Materials count check.');

    # Check the number of read objects.
    is($model->objects_count(), 1, 'Test 2: Objects count check.');

    # Check the number of read instances.
    is($model->get_object(0)->instances_count(), 1, 'Test 2: Object instances count check.');

    # Check the number of read volumes.
    is($model->get_object(0)->volumes_count(), 3, 'Test 2: Object volumes count check.');

}

# Test 3: Read an STL and write it as 3MF.
{

}

# Test 4: Read an 3MF and write it as STL.
{

}

# (1) Basic Test with model containing vertices and triangles.
{
    my $amf_test_file = dirname($current_path). "/models/amf/FaceColors.amf.xml";
    my $tmf_output_file = dirname($current_path). "/models/3mf/FaceColors.3mf";
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
        #."        <item objectid=\"1\" transform=\"1 0 0 -0 1 0 0 0 1 0 0 0\"/>\n"
        ."    </build> \n"
        ."</model>\n";

    # Create a new model.
    my $model = Slic3r::Model->new;
    # Read a simple AMF file.
    $model->read_amf($amf_test_file);
    # Write in 3MF format.
    $model->write_tmf($tmf_output_file);
    # Check contents in 3dmodel.model.
    my $model_output ;
    unzip $tmf_output_file => \$model_output, Name => "3D/3dmodel.model"
        or die "unzip failed: $UnzipError\n";
    is( $model_output, $expected_model, "3dmodel.model file matching");
    # Check contents in content_types.xml.
    my $content_types_output ;
    unzip $tmf_output_file => \$content_types_output, Name => "[Content_Types].xml"
        or die "unzip failed: $UnzipError\n";
    is( $content_types_output, $expected_content_types, "[Content_Types].xml file matching");
    # Check contents in _rels.xml.
    my $relationships_output ;
    unzip $tmf_output_file => \$relationships_output, Name => "_rels/.rels"
        or die "unzip failed: $UnzipError\n";
    is( $relationships_output, $expected_relationships, "_rels/.rels file matching");
}

# (2) Basic test with model containing materials.
{
    my $amf_test_file = dirname($current_path). "/models/amf/SplitPyramid.amf";
    my $tmf_output_file = dirname($current_path). "/models/3mf/SplitPyramid.3mf";
    my $expected_model = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        ."<model unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" xmlns:m=\"http://schemas.microsoft.com/3dmanufacturing/material/2015/02\" xmlns:slic3r=\"http://link_to_Slic3r_schema.com/2017/06\"> \n"
        ."    <slic3r:metadata type=\"version\">1.3.0-dev</slic3r:metadata>\n"
        ."    <resources> \n"
        ."    <basematerials id=\"1\">\n"
        ."        <base name=\"Red\" displaycolor=\"#FFFFFFFF\"/>\n"
        ."        <base name=\"Blue\" displaycolor=\"#FFFFFFFF\"/>\n"
        ."    </basematerials>\n"
        ."    <slic3r:materials>\n"
        ."    </slic3r:materials>\n"
        ."        <object id=\"2\" type=\"model\">\n"
        ."            <mesh>\n"
        ."                <vertices>\n"
        ."                    <vertex x=\"0\" y=\"1\" z=\"0\"/>\n"
        ."                    <vertex x=\"1\" y=\"0\" z=\"0\"/>\n"
        ."                    <vertex x=\"0\" y=\"0\" z=\"0\"/>\n"
        ."                    <vertex x=\"0.5\" y=\"0.5\" z=\"1\"/>\n"
        ."                    <vertex x=\"0\" y=\"1\" z=\"0\"/>\n"
        ."                    <vertex x=\"1\" y=\"1\" z=\"0\"/>\n"
        ."                    <vertex x=\"1\" y=\"0\" z=\"0\"/>\n"
        ."                    <vertex x=\"0.5\" y=\"0.5\" z=\"1\"/>\n"
        ."                </vertices>\n"
        ."                <triangles>\n"
        ."                    <triangle v1=\"0\" v2=\"1\" v3=\"2\"/>\n"
        ."                    <triangle v1=\"2\" v2=\"1\" v3=\"3\"/>\n"
        ."                    <triangle v1=\"3\" v2=\"1\" v3=\"0\"/>\n"
        ."                    <triangle v1=\"2\" v2=\"3\" v3=\"0\"/>\n"
        ."                    <triangle v1=\"4\" v2=\"5\" v3=\"6\"/>\n"
        ."                    <triangle v1=\"6\" v2=\"5\" v3=\"7\"/>\n"
        ."                    <triangle v1=\"7\" v2=\"5\" v3=\"4\"/>\n"
        ."                    <triangle v1=\"7\" v2=\"4\" v3=\"6\"/>\n"
        ."                </triangles>\n"
        ."                <slic3r:volumes>\n"
        ."                    <slic3r:volume ts=\"0\" te=\"3\" modifier=\"0\" >\n"
        ."                    </slic3r:volume>\n"
        ."                    <slic3r:volume ts=\"4\" te=\"7\" modifier=\"0\" >\n"
        ."                    </slic3r:volume>\n"
        ."                </slic3r:volumes>\n"
        ."            </mesh>\n"
        ."        </object>\n"
        ."    </resources> \n"
        ."    <build> \n"
        ."    </build> \n"
        ."</model>\n";

    # Create a new model.
    my $model = Slic3r::Model->new;
    # Read a simple AMF file.
    $model->read_amf($amf_test_file);
    # Write in 3MF format.
    $model->write_tmf($tmf_output_file);
    # Check contents in 3dmodel.model.
    my $model_output ;
    unzip $tmf_output_file => \$model_output, Name => "3D/3dmodel.model"
        or die "unzip failed: $UnzipError\n";
    is( $model_output, $expected_model, "3dmodel.model file matching");
    # Check contents in content_types.xml.
    my $content_types_output ;
    unzip $tmf_output_file => \$content_types_output, Name => "[Content_Types].xml"
        or die "unzip failed: $UnzipError\n";
    is( $content_types_output, $expected_content_types, "[Content_Types].xml file matching");
    # Check contents in _rels.xml.
    my $relationships_output ;
    unzip $tmf_output_file => \$relationships_output, Name => "_rels/.rels"
        or die "unzip failed: $UnzipError\n";
    is( $relationships_output, $expected_relationships, "_rels/.rels file matching");
}

# Finish finish test cases.
done_testing();

__END__
