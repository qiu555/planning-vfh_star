#ifndef VFHSTAR_TREESEARCH_H
#define VFHSTAR_TREESEARCH_H

#include <base/pose.h>
#include <base/waypoint.h>
#include <Eigen/StdVector>
#include <vector>
#include <list>

namespace vfh_star {
class TreeNode
{
    friend class Tree;
    public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	TreeNode();
	TreeNode(const base::Pose &pose, double dir);

        bool isRoot() const;
        bool isLeaf() const;
	
	const base::Pose &getPose() const;
	const TreeNode *getParent() const;
	double getDirection() const;
	int getDepth() const;

	double getCost() const;
	void setCost(double value);
	double getHeuristic() const;
	void setHeuristic(double value);
        double getHeuristicCost() const;

        double getPositionTolerance() const;
        void setPositionTolerance(double tol);
        double getHeadingTolerance() const;
        void setHeadingTolerance(double tol);
	
    private:
	std::vector<TreeNode *> children;
	TreeNode *parent;
	
	///pose of the node, note the orientation of the node and the direction may differ, 
	///because of kinematic constrains of the robot
	base::Pose pose;

	///direction, that was choosen, that lead to this node
	double direction;
	double cost;
	double heuristic;
	int depth;

        double positionTolerance;
        double headingTolerance;
};

class Tree
{
    public:
	Tree();
	~Tree();
	void addChild(TreeNode *parent, TreeNode *child);
	void removeChild(TreeNode *parent, TreeNode *child);
	
	const std::vector<TreeNode *> &getChildren(TreeNode *parent);
	TreeNode *getParent(TreeNode *child);
	TreeNode *getRootNode();

	void setRootNode(TreeNode *root);

        std::vector<base::Waypoint> buildTrajectoryTo(TreeNode const* leaf) const;

        int getSize() const;
        void clear();
        void verifyHeuristicConsistency(const TreeNode* from) const;

    private:
        std::list<TreeNode*> nodes;
};

class TreeSearch
{
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        typedef std::vector<double> Angles;
        typedef std::vector< std::pair<double, double> > AngleIntervals;

	TreeSearch();

	std::vector<base::Waypoint> getTrajectory(const base::Pose& start);
	
	void setObstacleSafetyDistance(const double &distance);
	void setRobotWidth(const double &width);
        Tree const& getTree() const;
	
	virtual ~TreeSearch();
        static double getHeading(const Eigen::Quaterniond &orientation);

    protected:
	Angles getDirectionsFromIntervals(const AngleIntervals& intervals);

        // The tree generated at the last call to getTrajectory
        Tree tree;

	///step size of forward projection
	double stepDistance;
	
	///maximum depth of search tree
	int maxTreeDepth;
	
        double angularSampling;
	double discountFactor;
	double obstacleSafetyDist;
	double robotWidth;

        Eigen::Vector3d targetLinePoint;
        Eigen::Vector3d targetLineNormal;

        /** Returns the estimated cost from the given node to the optimal node
         * reachable from that node. Note that this estimate must be a minorant,
         * i.e. must be smaller or equal than the actual value
         */
	virtual double getHeuristic(const TreeNode &node) const = 0;

        /** Returns the cost of travelling from \c node's parent to \c node
         * itself. It might include a cost of "being at" \c node as well
         */
	virtual double getCostForNode(const TreeNode& node) const = 0;

	/**
	* This method returns possible directions where 
	* the robot can drive to, from the given position
        *
        * The returned vector is a list of angle intervals
	**/
	virtual AngleIntervals getNextPossibleDirections(
                const base::Pose &curPose,
                double obstacleSafetyDist,
                double robotWidth) const = 0;

	/**
	* This function returns a pose in which the robot would
	* be if he would have driven towards the given direction.
	* This method should take the robot driving constrains
	* into account. 
	*/
	virtual base::Pose getProjectedPose(const base::Pose &curPose,
                double heading,
                double distance) const = 0;
};
} // vfh_star namespace

#endif // VFHSTAR_H
