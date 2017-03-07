/*
 * This class represents a set of layers and their heights.
 * It is intended for smoothing the height distribution (avoid very thin
 * layers next to thick layers) and to correctly interpolate higher layers if
 * a layer height changes somewhere in a lower position at the object.
 * Uses http://www.eol.ucar.edu/homes/granger/bspline/doc/ for spline computation.
 */

#include "LayerHeightSpline.hpp"
#include <cmath> // std::abs

namespace Slic3r {

LayerHeightSpline::LayerHeightSpline(coordf_t object_height)
:   _object_height(object_height), _layer_height_spline(NULL)
{
	this->_is_valid = false;
	this->_update_required = true;
	this->_layers_updated = false;
	this->_layer_heights_updated = false;
	this->_cusp_value = -1;
}

LayerHeightSpline::LayerHeightSpline(const LayerHeightSpline &other)
:   _object_height(other._object_height), _layer_height_spline(NULL)
{

    this->_original_layers = other._original_layers;
    this->_internal_layers = other._internal_layers;
    this->_internal_layer_heights = other._internal_layer_heights;
	this->_is_valid = other._is_valid;
	this->_update_required = other._update_required;
	this->_layers_updated = other._layers_updated;
	this->_layer_heights_updated = other._layer_heights_updated;
	this->_cusp_value = other._cusp_value;
	if(this->_is_valid) {
		this->_updateBSpline();
	}
}

LayerHeightSpline::~LayerHeightSpline()
{
    if (this->_layer_height_spline) {
    	delete this->_layer_height_spline;
    }
}

/*
 * Indicates whether the object has valid data and the spline was successfully computed or not.
 */
bool LayerHeightSpline::hasData()
{
	return this->_is_valid;
}

/*
 * Does this object expect new layer heights during the slice step or should
 * we use the layer heights values provided by the spline?
 * An update is required if a config option is changed which affects the layer height.
 * An update is not required if the spline was modified by a user interaction.
 */
bool LayerHeightSpline::updateRequired()
{
	bool result = true; // update spline by default
	if(!this->_update_required && this->_is_valid) {
		result = false;
	}
	this->_update_required = true; // reset to default after request
	return result;
}

/*
 * Don't require an update for exactly one iteration.
 */
void LayerHeightSpline::suppressUpdate() {
	if (this->_is_valid) {
		this->_update_required = false;
	}
}

/*
 * Set absolute layer positions in object coordinates.
 * Heights (thickness of each layer) is generated from this list.
 */
bool LayerHeightSpline::setLayers(std::vector<coordf_t> layers)
{
	this->_original_layers = layers;

	// generate updated layer height list from layers
	this->_internal_layer_heights.clear();
	coordf_t last_z = 0;
	for (std::vector<coordf_t>::const_iterator l = this->_original_layers.begin(); l != this->_original_layers.end(); ++l) {
		this->_internal_layer_heights.push_back(*l-last_z);
		last_z = *l;
	}

	// add 0-values at both ends to achieve correct boundary conditions
	this->_internal_layers = this->_original_layers;
	this->_internal_layers.insert(this->_internal_layers.begin(), 0); // add z = 0 to the front
	//this->_internal_layers.push_back(this->_internal_layers.back()+1); // and object_height + 1 to the end
	this->_internal_layer_heights.insert(this->_internal_layer_heights.begin(), this->_internal_layer_heights[0]);
	//this->_internal_layer_heights.push_back(0);

	this->_layers_updated = true;
	this->_layer_heights_updated = false;

	return this->_updateBSpline();
}


/*
 * Update only the desired thickness of the layers, but not their positions!
 * This modifies the y-values for the spline computation and only affects
 * the resulting layers which can be obtained with getInterpolatedLayers.
 * The argument vector must be of the same size as the layers vector.
 */
bool LayerHeightSpline::updateLayerHeights(std::vector<coordf_t> heights)
{
	bool result = false;

	// do we receive the correct number of values?
	if(heights.size() == this->_internal_layers.size()-1) {
		this->_internal_layer_heights = heights;
		// add leading and trailing 0-value
		this->_internal_layer_heights.insert(this->_internal_layer_heights.begin(), this->_internal_layer_heights[0]);
		//this->_internal_layer_heights.push_back(0);
		result = this->_updateBSpline();
	}else{
		std::cerr << "Unable to update layer heights. You provided " << heights.size() << " layers, but " << this->_internal_layers.size()-1 << " expected" << std::endl;
	}

	this->_layers_updated = false;
	this->_layer_heights_updated = true;

	return result;
}

/*
 * Reset the this object, remove database and interpolated results.
 */
void LayerHeightSpline::clear()
{
	this->_original_layers.clear();
	this->_internal_layers.clear();
	this->_internal_layer_heights.clear();
	delete this->_layer_height_spline;
	this->_layer_height_spline = NULL;
	this->_is_valid = false;
	this->_layers_updated = false;
	this->_layer_heights_updated = false;
}


/*
 * Get a full set of layer z-positions by interpolation along the spline.
 */
std::vector<coordf_t> LayerHeightSpline::getInterpolatedLayers() const
{
	std::vector<coordf_t> layers;
	if(this->_is_valid) {
		// preserve first layer for bed contact
		layers.push_back(this->_original_layers[0]);
		coordf_t z = this->_original_layers[0];
		coordf_t h;
		coordf_t h_diff = 0;
		coordf_t eps = 0.0001;
		while(z <= this->_object_height) {
			h = 0;
			// find intersection between layer height and spline
			do {
				h += h_diff/2;
				h = this->_layer_height_spline->evaluate(z+h);
				h_diff = this->_layer_height_spline->evaluate(z+h) - h;
			} while(std::abs(h_diff) > eps);
			z += h;
			layers.push_back(z);
		}
		// how to make sure, the last layer is not higher than object while maintaining between min/max layer height?
	}
	return layers;
}

/*
 * Evaluate interpolated layer height (thickness) at given z-position
 */
const coordf_t LayerHeightSpline::getLayerHeightAt(coordf_t height)
{
	coordf_t result = 0;
	if (this->_is_valid) {
		result = this->_layer_height_spline->evaluate(height);
	}
	return result;
}

/*
 * Internal method to re-compute the spline
 */
bool LayerHeightSpline::_updateBSpline()
{
	bool result = false;
	//TODO: exception if not enough points?

	delete this->_layer_height_spline;
	this->_layer_height_spline = new BSpline<double>(&this->_internal_layers[0],
			this->_internal_layers.size(),
            &this->_internal_layer_heights[0],
            0,
            1,
            0);

	if (this->_layer_height_spline->ok()) {
		result = true;
	} else {
		result = false;
		std::cerr << "Spline setup failed." << std::endl;
	}

	this->_is_valid = result;

	return result;
}


}
