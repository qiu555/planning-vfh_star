#include "VFHTreeVisualization.hpp"
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Point>
#include <boost/tuple/tuple.hpp>
#include <osg/LineWidth>
#include <base/float.h>

#include <iostream>

using namespace vizkit;
using namespace std;
using vfh_star::TreeNode;

struct VFHTreeVisualization::Data {
    // Copy of the value given to updateDataIntern.
    //
    // Making a copy is required because of how OSG works
    vfh_star::Tree data;

    // If true, only the nodes that are not eaves will be displayed
    // (true by default)
    bool removeLeaves;

    // If true, the color will depend on the heuristics, otherwise on the
    // actual node cost is displayed. False by default
    VFHTreeVisualization::COST_MODE costMode;

    bool hasSegment;
    std::vector<base::Vector3d> segment;

    int treeNodeCount;

    Data()
        : removeLeaves(false), costMode(VFHTreeVisualization::SHOW_COST), hasSegment(false), treeNodeCount(0) {}
};


VFHTreeVisualization::VFHTreeVisualization()
    : p(new Data)
{
}

VFHTreeVisualization::~VFHTreeVisualization()
{
    delete p;
}

void VFHTreeVisualization::setCostMode(COST_MODE mode)
{
    p->costMode = mode;
    setDirty();
}

VFHTreeVisualization::COST_MODE VFHTreeVisualization::getCostMode()
{
    return p->costMode;
}

void VFHTreeVisualization::removeLeaves(bool enable)
{
    p->removeLeaves = enable;
    setDirty();
}

bool VFHTreeVisualization::areLeavesRemoved()
{
    return p->removeLeaves;
}

void VFHTreeVisualization::setMaxNodeCount(int count)
{
    p->treeNodeCount = count;
    setDirty();
}

int VFHTreeVisualization::getMaxNodeCount()
{
    return p->treeNodeCount;
}

osg::ref_ptr<osg::Node> VFHTreeVisualization::createMainNode()
{
    // Geode is a common node used for vizkit plugins. It allows to display
    // "arbitrary" geometries
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::StateSet *state = geode->getOrCreateStateSet();
    state->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    return geode;
}

static double getDisplayCost(VFHTreeVisualization::COST_MODE mode, vfh_star::TreeNode const& node)
{
    if (mode == VFHTreeVisualization::SHOW_COST)
        return node.getCost();
    if (mode == VFHTreeVisualization::SHOW_HEURISTICS)
        return node.getHeuristic();
    if (mode == VFHTreeVisualization::SHOW_BOTH)
        return node.getHeuristicCost();
    return 0;
}

pair<double, double> VFHTreeVisualization::computeColorMapping(std::multimap<double, TreeNode const*> const &sorted_nodes) const
{
    // NOTE: we assume that nodes is not empty. This is checked by
    // updateMainNode
    if (sorted_nodes.empty())
        return make_pair(0, 0);

    double min_cost, max_cost;
    min_cost = max_cost = getDisplayCost(p->costMode, *(sorted_nodes.begin()->second));
    
    for (std::map<double, TreeNode const*>::const_iterator it = sorted_nodes.begin();
            it != sorted_nodes.end(); ++it)
    {
        const TreeNode &node(*(it->second));
        if (node.getHeuristic() < 0)
            continue;

        double c = getDisplayCost(p->costMode, node);

        if (c < min_cost)
            min_cost = c;
        if (c > max_cost)
            max_cost = c;
    }


    if (max_cost == 0)
        return make_pair(0, 0);

    return make_pair(1.0/max_cost, -min_cost);
}

osg::Geometry* VFHTreeVisualization::createSolutionNode(TreeNode const* node, double color_a, double color_b)
{
    osg::Geometry* geom = new osg::Geometry;

    // Create the color and vertex arrays
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors   = new osg::Vec4Array;

    // Used later to represent the initial orientation
    double step = 0;
    while (!node->isRoot())
    {
        base::Position parent_p = node->getParent()->getPose().position;
        base::Position p = node->getPose().position;
        if (step == 0)
            step = (parent_p - p).norm();
//	vertices->push_back(osg::Vec3(parent_p.x(), parent_p.y(), parent_p.z() + 0.001));
//	vertices->push_back(osg::Vec3(p.x(), p.y(), p.z() + 0.001));

	vertices->push_back(osg::Vec3(parent_p.y(), -parent_p.x(), parent_p.z() + 0.001));
	vertices->push_back(osg::Vec3(p.y(), -p.x(), p.z() + 0.001));


        double cost = getDisplayCost(this->p->costMode, *node);

        double red = color_a * (cost + color_b) / 2 + 0.5;
        double green = 1.0 - red;
        double blue = 0.5;
        double alpha = 1.0;
	colors->push_back(osg::Vec4(red, green, blue, alpha));
	colors->push_back(osg::Vec4(red, green, blue, alpha));

        node = node->getParent();
    }

    // Add a node for the root's direction
    Eigen::Quaterniond q(Eigen::AngleAxisd(node->getDirection(), Eigen::Vector3d::UnitZ()));
    base::Vector3d root_dir = q * Eigen::Vector3d::UnitY();
    base::Position parent_p = node->getPose().position;
    base::Position p = parent_p - root_dir * step;
//    vertices->push_back(osg::Vec3(parent_p.x(), parent_p.y(), parent_p.z() + 0.001));
//    vertices->push_back(osg::Vec3(p.x(), p.y(), p.z() + 0.001));

    vertices->push_back(osg::Vec3(parent_p.y(), -parent_p.x(), parent_p.z() + 0.001));
	vertices->push_back(osg::Vec3(p.y(), -p.x(), p.z() + 0.001));

    colors->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    colors->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));

    geom->setColorArray(colors);
    geom->setColorBinding( osg::Geometry::BIND_PER_VERTEX );
    geom->setVertexArray(vertices);

    // Draw the vertices as points
    osg::ref_ptr<osg::DrawArrays> drawArrays =
        new osg::DrawArrays( osg::PrimitiveSet::LINES, 0, vertices->size() );
    geom->addPrimitiveSet(drawArrays.get());

    // Finally, setup the point attributes
    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth();
    lw->setWidth(10.0);
    geom->getOrCreateStateSet()->setAttribute( lw, osg::StateAttribute::ON );

    return geom;
}

osg::Geometry* VFHTreeVisualization::createTreeNode(std::multimap<double, TreeNode const*> const &sorted_nodes, double color_a, double color_b)
{
    osg::Geometry* geom = new osg::Geometry;

    // Create the color and vertex arrays
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors   = new osg::Vec4Array;

    int count = p->treeNodeCount;

    int nodeCounter = 0;
    
    for(std::map<double, TreeNode const*>::const_iterator it = sorted_nodes.begin();
            it != sorted_nodes.end(); ++it)
    {
        if (count != 0 && nodeCounter > count)
            break;

        nodeCounter++;
        
        TreeNode const* node = (it->second);
        
        if(node->isRoot())
            continue;

        base::Position parent_p = node->getParent()->getPose().position;
        base::Position p = node->getPose().position;
//    vertices->push_back(osg::Vec3(parent_p.x(), parent_p.y(), parent_p.z()));
//    vertices->push_back(osg::Vec3(p.x(), p.y(), p.z()));

	vertices->push_back(osg::Vec3(parent_p.y(), -parent_p.x(), parent_p.z()));
	vertices->push_back(osg::Vec3(p.y(), -p.x(), p.z()));


        if (node->getHeuristic() < 0) // used to mark invalid nodes
        {
            colors->push_back(osg::Vec4(0, 0, 0, 1));
            colors->push_back(osg::Vec4(0, 0, 0, 1));
        }
        else
        {
            double cost = getDisplayCost(this->p->costMode, *node);

            double red = color_a * (cost + color_b);
            double green = 1.0 - red;
            double blue = 0.5;
            double alpha = 1.0;
            colors->push_back(osg::Vec4(red, green, blue, alpha));
            colors->push_back(osg::Vec4(red, green, blue, alpha));
        }        
    }
    geom->setColorArray(colors);
    geom->setColorBinding( osg::Geometry::BIND_PER_VERTEX );
    geom->setVertexArray(vertices);

    // Draw the vertices as points
    osg::ref_ptr<osg::DrawArrays> drawArrays =
        new osg::DrawArrays( osg::PrimitiveSet::LINES, 0, vertices->size() );
    geom->addPrimitiveSet(drawArrays.get());

    // Finally, setup the point attributes
    osg::ref_ptr<osg::Point> point = new osg::Point();
    point->setSize(10.0);
    point->setDistanceAttenuation( osg::Vec3(0.5, 0.5, 0.5 ) );
    point->setMinSize( 0.2 );
    point->setMaxSize( 5.0 );
    geom->getOrCreateStateSet()->setAttribute( point, osg::StateAttribute::ON );

    return geom;
}

void VFHTreeVisualization::updateMainNode ( osg::Node* node )
{
    osg::Geode* geode = static_cast<osg::Geode*>(node);
    while(geode->removeDrawables(0));

    if (p->hasSegment)
    {
        osg::Geometry* geom = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> colors   = new osg::Vec4Array;
        base::Vector3d pt = p->segment[0];
//        vertices->push_back(osg::Vec3(pt.x(), pt.y(), pt.z()));
        vertices->push_back(osg::Vec3(pt.y(), -pt.x(), pt.z()));
        pt = p->segment[1];
//        vertices->push_back(osg::Vec3(pt.x(), pt.y(), pt.z()));
        vertices->push_back(osg::Vec3(pt.y(), -pt.x(), pt.z()));
        colors->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));

        geom->setColorArray(colors);
        geom->setColorBinding( osg::Geometry::BIND_OVERALL );
        geom->setVertexArray(vertices);
        osg::ref_ptr<osg::DrawArrays> drawArrays =
            new osg::DrawArrays( osg::PrimitiveSet::LINES, 0, vertices->size() );
        geom->addPrimitiveSet(drawArrays.get());
        geode->addDrawable(geom);
    }

    list<TreeNode> const& nodes = p->data.getNodes();
    if (nodes.empty())
        return;

    //sort nodes by heuristic costs.
    std::multimap<double, TreeNode const*> sorted_nodes;
    
    if (p->removeLeaves)
    {
        for(list<TreeNode>::const_iterator it = nodes.begin();
                it != nodes.end(); it++)
        {
            if(it->isLeaf() && !it->isRoot())
                continue;
            
            sorted_nodes.insert(std::make_pair(it->getHeuristicCost(), &(*it)));
        }
    }
    else
    {
        for(list<TreeNode>::const_iterator it = nodes.begin();
                it != nodes.end(); it++)
            sorted_nodes.insert(std::make_pair(it->getHeuristicCost(), &(*it)));
    }
    
    // Add the final node regardless of the leaf mode
    if (p->data.getFinalNode())
        sorted_nodes.insert(std::make_pair(p->data.getFinalNode()->getHeuristicCost(), p->data.getFinalNode()));

    // Gets the mapping from cost to color
    double cost_a, cost_b;
    boost::tie(cost_a, cost_b) = computeColorMapping(sorted_nodes);

    geode->addDrawable(createTreeNode(sorted_nodes, cost_a, cost_b));
    if (p->data.getFinalNode())
        geode->addDrawable(createSolutionNode(p->data.getFinalNode(), cost_a, cost_b));
}

void VFHTreeVisualization::updateDataIntern(vfh_star::Tree const& value)
{
    p->data = value;
}

void VFHTreeVisualization::updateDataIntern(std::vector<base::Vector3d> const& segment)
{
    p->segment = segment;
    p->hasSegment = true;
}

