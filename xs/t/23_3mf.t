#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;
use Test::More;
use IO::Uncompress::Unzip qw(unzip $UnzipError) ;
use Cwd qw(abs_path);
use File::Basename qw(dirname);

# Removes '\n' and '\r\n' from a string.
sub clean {
    my $text = shift;
    $text =~ s/\n//g;
    $text =~ s/\r//g;
    return $text;
}

my $current_path = abs_path($0);
my $expected_content_types = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n"
    ."<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
    ."<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
    ."<Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n"
    ."</Types>\n";
my $expected_relationships = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n"
    ."<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
    ."<Relationship Id=\"rel0\" Target=\"/3D/3dmodel.model\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" /></Relationships>\n";

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

# Test 2: Check read metadata/ objects/ components/ build items w/o or with tansformation matrics.
{
    my $input_path = dirname($current_path). "/models/3mf/gimblekeychain.3mf";
    # Create a new model.
    my $model = Slic3r::Model->new;
    $model->read_tmf($input_path);

    # Check the number of read matadata.
    is($model->metadata_count(), 8, 'Test 2: metadata count check.');

    # Check the number of read objects.
    is($model->objects_count(), 1, 'Test 2: objects count check.');

    # Check the number of read instances.
    is($model->get_object(0)->instances_count(), 1, 'Test 2: object instances count check.');

    # Check the read object part number.
    is($model->get_object(0)->part_number(), -1, 'Test 2: object part number check.');

    # Check the number of read volumes.
    is($model->get_object(0)->volumes_count(), 3, 'Test 2: object volumes count check.');

    # Check the number of read number of facets (triangles).
    is($model->get_object(0)->facets_count(), 19884, 'Test 2: object number of facets check.');

    # Check the affine transformation matrix decomposition.
    # Check translation.
    cmp_ok($model->get_object(0)->get_instance(0)->offset()->x(), '<=', 0.0001, 'Test 2: X translation check.');
    cmp_ok($model->get_object(0)->get_instance(0)->offset()->y(), '<=', 0.0001, 'Test 2: Y translation check.');
    cmp_ok($model->get_object(0)->get_instance(0)->z_translation() - 0.0345364, '<=', 0.0001, 'Test 2: Z translation check.');

    # Check scale.
    cmp_ok($model->get_object(0)->get_instance(0)->scaling_vector()->x() - 25.4, '<=', 0.0001, 'Test 2: X scale check.');
    cmp_ok($model->get_object(0)->get_instance(0)->scaling_vector()->y() - 25.4, '<=', 0.0001, 'Test 2: Y scale check.');
    cmp_ok($model->get_object(0)->get_instance(0)->scaling_vector()->z() - 25.4, '<=', 0.0001, 'Test 2: Z scale check.');

    # Check X, Y, & Z rotation.
    cmp_ok($model->get_object(0)->get_instance(0)->x_rotation() - 6.2828, '<=', 0.0001, 'Test 2: X rotation check.');
    cmp_ok($model->get_object(0)->get_instance(0)->y_rotation() - 6.2828, '<=', 0.0001, 'Test 2: Y rotation check.');
    cmp_ok($model->get_object(0)->get_instance(0)->rotation(), '<=', 0.0001, 'Test 2: Z rotation check.');

}

# Test 3: Read an STL and write it as 3MF.
{
    my $input_path = dirname($current_path).  "/models/stl/spikey_top.stl";
    my $output_path = dirname($current_path). "/models/3mf/spikey_top.3mf";

    my $model = Slic3r::Model->new;
    my $result = $model->read_stl($input_path);
    is($result, 1, 'Test 3: read the stl model file.');

    # Check initialization of 3mf specific atttributes.
    is($model->metadata_count(), 0, 'Test 3: read stl model metadata count check .');
    is($model->get_object(0)->instances_count(), 0, 'Test 3: object instances count check.');
    is($model->get_object(0)->part_number(), -1, 'Test 3: object partnumber check.');
    is($model->material_count(), 0, 'Test 3: model materials count check.');

    $result = $model->write_tmf($output_path);
    is($result, 1, 'Test 3: Write the 3mf model file.');

    unlink($output_path);
}

# Test 4: Read an 3MF containig multiple objects and volumes and write it as STL.
{
    my $input_path = dirname($current_path). "/models/3mf/gimblekeychain.3mf";
    my $output_path = dirname($current_path). "/models/stl/gimblekeychain.stl";

    my $model = Slic3r::Model->new;
    my $result = $model->read_tmf($input_path);
    is($result, 1, 'Test 4: Read 3MF file check.');

    $result = $model->write_stl($output_path);
    is($result, 1, 'Test 4: convert to stl check.');

    unlink($output_path);
}

# Test 5: Basic Test with model containing vertices and triangles.
{
    my $amf_test_file = dirname($current_path). "/models/amf/FaceColors.amf.xml";
    my $tmf_output_file = dirname($current_path). "/models/3mf/FaceColors.3mf";
    my $expected_model = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        ."<model unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" xmlns:slic3r=\"http://schemas.slic3r.org/3mf/2017/06\"> \n"
        ."    <slic3r:metadata version=\"" . Slic3r::VERSION()  . "\"/>\n"
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
    is (clean($model_output), clean($expected_model), "Test 5: 3dmodel.model file matching");
    # Check contents in content_types.xml.
    my $content_types_output ;
    unzip $tmf_output_file => \$content_types_output, Name => "[Content_Types].xml"
        or die "unzip failed: $UnzipError\n";
    is (clean($content_types_output), clean($expected_content_types), "Test 5: [Content_Types].xml file matching");
    # Check contents in _rels.xml.
    my $relationships_output ;
    unzip $tmf_output_file => \$relationships_output, Name => "_rels/.rels"
        or die "unzip failed: $UnzipError\n";
    is (clean($relationships_output), clean($expected_relationships), "Test 5: _rels/.rels file matching");

    unlink($tmf_output_file);
}

# Test 6: Read a Slic3r exported 3MF file.
{
    my $input_path = dirname($current_path). "/models/3mf/chess.3mf";
    my $output_path = dirname($current_path). "/models/3mf/chess_2.3mf";

    # Read a 3MF model.
    my $model = Slic3r::Model->new;
    my $result = $model->read_tmf($input_path);
    is($result, 1, 'Test 6: Read 3MF file check.');

    # Export the 3MF file.
    $result = $model->write_tmf($output_path);
    is($result, 1, 'Test 6: Write 3MF file check.');

    # Re-read the exported file.
    my $model_2 = Slic3r::Model->new;
    $result = $model_2->read_tmf($output_path);
    is($result, 1, 'Test 6: Read second 3MF file check.');

    is($model->metadata_count(), $model_2->metadata_count(), 'Test 6: metadata match check.');

    # Check the number of read objects.
    is($model->objects_count(), $model_2->objects_count(), 'Test 6: objects match check.');

    unlink($output_path);
}

# Finish finish test cases.
done_testing();

__END__
