#ifndef GRIDMAPSEGMENTER_H
#define GRIDMAPSEGMENTER_H

#include <Eigen/Geometry>
#include "ElevationGrid.h"
#include "TraversabilityGrid.h"
#include "DebugTypes.h"
#include <base/samples/laser_scan.h>
#include <stdint.h>

namespace vfh_star {
    
class TraversabilityMapGenerator
{
    public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	TraversabilityMapGenerator();
		
	bool addLaserScan(const base::samples::LaserScan& ls, const Eigen::Transform3d& body2Odo, const Eigen::Transform3d& laser2Body);
	
	void getGridDump(GridDump &gd) const;
	
	const TraversabilityGrid &getTraversabilityMap() const 
	{
	    return traversabilityGrid;
	};
	
    private:
	
	/**
	* This function updates the TraversabilityGrid in respect to the given
	* ElevationGrid.
	**/
	void updateTraversabilityGrid(const ElevationGrid &elGrid, TraversabilityGrid &trGrid);
	
	/**
	* Applys the Conservative Interpolation on the whole grid
	**/
	void smoothElevationGrid(const ElevationGrid &source, ElevationGrid &target);
	
	/**
	* The conservative interpolation fills holes in the Elevation grid, if the 
	* sourounding cells of a hole are known.
	**/
	void doConservativeInterpolation(const ElevationGrid& source, ElevationGrid& target, Eigen::Vector2i p);
	
	/**
	* This function calculates the slope to the sourounding cells of p
	* and sets the traversibillity of the cell p in the TraversabilityGrid
	**/
	void testNeighbourEntry(Eigen::Vector2i p, const ElevationGrid &elGrid, TraversabilityGrid &trGrid);

	/**
	* This function test if the robot is near the outer bound of
	* the grid and thus the grid need to be moved
	**/
	bool moveGridIfRobotNearBoundary(ElevationGrid& grid, const Eigen::Vector3d& robotPosition_world);

	ElevationGrid laserGrid;
	ElevationGrid interpolatedGrid;
	TraversabilityGrid traversabilityGrid;
	Eigen::Transform3d lastBody2Odo;
};

}

#endif // GRIDMAPSEGMENTER_H