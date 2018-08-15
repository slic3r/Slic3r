#include "NonplanarFacet.hpp"


namespace Slic3r {

void
NonplanarFacet::calculate_stats() {
    //calculate min and max values
    this->stats.min.x = this->vertex[0].x;
    this->stats.min.y = this->vertex[0].y;
    this->stats.min.z = this->vertex[0].z;
    this->stats.max.x = this->vertex[0].x;
    this->stats.max.y = this->vertex[0].y;
    this->stats.max.z = this->vertex[0].z;
    for(int j = 1; j < 3; j++) {
        this->stats.min.x = std::min(this->stats.min.x, this->vertex[j].x);
        this->stats.min.y = std::min(this->stats.min.y, this->vertex[j].y);
        this->stats.min.z = std::min(this->stats.min.z, this->vertex[j].z);
        this->stats.max.x = std::max(this->stats.max.x, this->vertex[j].x);
        this->stats.max.y = std::max(this->stats.max.y, this->vertex[j].y);
        this->stats.max.z = std::max(this->stats.max.z, this->vertex[j].z);
    }
}

void
NonplanarFacet::translate(float x, float y, float z)
{
    //translate facet
    for(int j = 0; j < 3; j++) {
      this->vertex[j].x += x;
      this->vertex[j].y += y;
      this->vertex[j].z += z;
    }

    //translate min and max values
    this->stats.min.x += x;
    this->stats.min.y += y;
    this->stats.min.z += z;
    this->stats.max.x += x;
    this->stats.max.y += y;
    this->stats.max.z += z;
}

void
NonplanarFacet::scale(float versor[3])
{
    //scale facet
    for(int j = 0; j < 3; j++) {
        this->vertex[j].x *= versor[0];
        this->vertex[j].y *= versor[1];
        this->vertex[j].z *= versor[2];
    }

    //scale min and max values
    this->stats.min.x *= versor[0];
    this->stats.min.y *= versor[1];
    this->stats.min.z *= versor[2];
    this->stats.max.x *= versor[0];
    this->stats.max.y *= versor[1];
    this->stats.max.z *= versor[2];
}

bool
NonplanarFacet::check_printable_facet(float max_angle)
{
    for(int j = 0; j < 3; j++) {
      if(!this->check_angle(this->neighbor[j], max_angle)){
          return false;
      }
    }
    return true;
}

bool
NonplanarFacet::check_angle(int neighbor, float max_angle)
{
    return true;
}

}
