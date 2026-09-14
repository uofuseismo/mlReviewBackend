#ifndef PRIVATE_REGIONS_HPP
#define PRIVATE_REGIONS_HPP
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include "lonTo180.hpp"
namespace
{

/// @result True indicates the event is in the UUSS's Yellowstone 
///         authoritative region. 
[[nodiscard]]
bool isInYellowstone(const double latitude, const double longitudeIn)
{
    if (latitude < -90 || latitude > 90)
    {
        throw std::invalid_argument("Latitude must in be in range [-90,90]");
    }
    auto longitude = ::lonTo180(longitudeIn);    
    namespace bg = boost::geometry;
    const bg::model::d2::point_xy<double> point {latitude, longitude};
    const bg::model::polygon<bg::model::d2::point_xy<double>> polygon
    {   
     {   
      {44.00,  -111.333},
      {44.00,  -109.750},
      {45.167, -109.750},
      {45.167, -111.333},
      {44.00,  -111.333}
     }   
    };  
    return bg::within(point, polygon);
}

/// @result True indicates the event is in the UUSS's Utah
///         authoritative region. 
[[nodiscard]] 
bool isInUtah(const double latitude, const double longitudeIn)
{
    if (latitude < -90 || latitude > 90)
    {   
        throw std::invalid_argument("Latitude must in be in range [-90,90]");
    }   
    auto longitude = ::lonTo180(longitudeIn);
    namespace bg = boost::geometry;
    const bg::model::d2::point_xy<double> point {latitude, longitude};
    const bg::model::polygon<bg::model::d2::point_xy<double>> polygon
    {   
     {   
      {36.75, -114.25},
      {36.75, -108.75},
      {42.50, -108.75},
      {42.50, -114.25},
      {36.75, -114.25}
     }   
    };  
    return bg::within(point, polygon);
}

/// @result True indicates the event is in the UUSS's authoritative region. 
[[nodiscard]] 
bool isInAuthoritativeRegion(const double latitude, const double longitude)
{
    if (isInUtah(latitude, longitude)){return true;}
    if (isInYellowstone(latitude, longitude)){return true;}
    return false;
}

}
#endif
