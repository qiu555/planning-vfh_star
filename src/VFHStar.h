#ifndef VFHSTAR_H
#define VFHSTAR_H

#include "TreeSearch.h"

namespace vfh_star {
class VFHStar : public TreeSearch
{
    public:
        VFHStar();
        virtual ~VFHStar();

        std::vector<base::Waypoint> getWaypoints(base::Pose const& start, double mainHeading, double horizon);
        base::geometry::Spline<3> getTrajectory(base::Pose const& start, double mainHeading, double horizon);
        std::vector<base::Trajectory> getTrajectories(base::Pose const& start, double mainHeading, double horizon, const Eigen::Affine3d &body2Trajectory);
	const TreeNode* computePath(base::Pose const& start, double mainHeading, double horizon, const Eigen::Affine3d &body2Trajectory);
	
        void setCostConf(const VFHStarConf& config);
        const VFHStarConf& getCostConf() const;

	base::Vector3d getHorizonOrigin() 
	{
	    return targetLinePoint;
	}

	base::Vector3d getHorizonVector() 
	{
	    return targetLine;
	}

    private:
        double getMotionDirection(const Eigen::Vector3d &start, const Eigen::Vector3d &end) const;

        double mainHeading;
        VFHStarConf cost_conf;

    protected:
        base::Vector3d targetLinePoint;
        base::Vector3d targetLineNormal;
        base::Vector3d targetLine;

	double angleDiff(const double &a1, const double &a2) const;

	/** Returns true if \c node is behind the goal line
         */
        virtual bool isTerminalNode(const TreeNode& node) const;

        /** Returns the estimated cost from the given node to the optimal node
         * reachable from that node. Note that this estimate must be a minorant,
         * i.e. must be smaller or equal than the actual value
         */
        virtual double getHeuristic(const TreeNode &node) const;

        /** Returns the cost of travelling from \c parent to \c node. It might
         * include a cost of "being at" \c node as well
         */
        virtual double getCostForNode(const vfh_star::ProjectedPose& projection, double direction, const vfh_star::TreeNode& parentNode) const;
    
        /** Returns the algebraic distance from \c pos to the goal line. If
         * the returned distance is negative, it means we crossed it.
         */
        double algebraicDistanceToGoalLine(const base::Position& pos) const;
};
} // vfh_star namespace

#endif // VFHSTAR_H
