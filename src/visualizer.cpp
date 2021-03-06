#include <osg/Point>
#include <osg/ShapeDrawable>
#include <osg/Geometry>

#include "visualizer.h"

Visualizer::Visualizer()
    : scene_root_(new osg::Group())
   
{

}

Visualizer::~Visualizer()
{

}

void Visualizer::addPointCloud(PointCloud *point_cloud)
{
    osg::ref_ptr<osg::Vec3Array>  vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array>  normals = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array>  colors = new osg::Vec4Array;

    for (size_t i = 0, i_end = point_cloud->size(); i < i_end; i++) {
        const Point &point = point_cloud->at(i);
        vertices->push_back(osg::Vec3(point.x, point.y, point.z));
        normals->push_back(osg::Vec3(point.normal_x, point.normal_y, point.normal_z));
        colors->push_back(osg::Vec4(point.r / 255.0, point.g / 255.0, point.b / 255.0, point.a / 255.0));
    }

    size_t item_num = vertices->size();

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    geometry->setUseDisplayList(true);
    geometry->setUseVertexBufferObjects(true);
    geometry->setVertexData(osg::Geometry::ArrayData(vertices, osg::Geometry::BIND_PER_VERTEX));
    geometry->setNormalData(osg::Geometry::ArrayData(normals, osg::Geometry::BIND_PER_VERTEX));
    geometry->setColorData(osg::Geometry::ArrayData(colors, osg::Geometry::BIND_PER_VERTEX));
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, item_num));
    geometry->getOrCreateStateSet()->setAttribute(new osg::Point(2.0f));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry);
    scene_root_->addChild(geode);
    cloud_map_.insert(std::make_pair<PointCloud*, osg::Geode*>(point_cloud, geode.get()));
    
    return;
}

void Visualizer::addGraph(PointCloud *point_cloud)
{
    PointCloud::DeformationGraph* defo_graph = point_cloud->getDeformationGraph();
    GraphMap* graph_map = point_cloud->getGraphMap();
    
    osg::ref_ptr<osg::Group> nodes = new osg::Group();
    osg::ref_ptr<osg::Group> edges = new osg::Group();
    osg::ref_ptr<osg::Group> graph = new osg::Group();
    graph->addChild(nodes);
    graph->addChild(edges);
    
    for (PointCloud::DeformationGraph::NodeIt it(*defo_graph); it != lemon::INVALID; ++ it) {
        Point& point = point_cloud->at((*graph_map)[it]);
        osg::Geode *node = new osg::Geode();
        osg::ref_ptr<osg::ShapeDrawable> shape = new osg::ShapeDrawable(
            new osg::Sphere(osg::Vec3(point.x, point.y, point.z), 1.0f));
        shape->setColor(osg::Vec4(0.8f, 0.8f, 0.4f, 1.0f));
        node->addDrawable(shape);
        nodes->addChild(node);
    } 
    
    for (PointCloud::DeformationGraph::EdgeIt it(*defo_graph); it != lemon::INVALID; ++ it) {
        PointCloud::DeformationGraph::Node u = (*defo_graph).u(it);
        PointCloud::DeformationGraph::Node v = (*defo_graph).v(it);
        
        Point& pt_source = point_cloud->at((*graph_map)[u]);
        Point& pt_target = point_cloud->at((*graph_map)[v]);
        
//         std::cout << pt_source.x << pt_source.y << pt_source.z << std::endl;
//         std::cout << point_cloud->at((*graph_map)[u]).x << point_cloud->at((*graph_map)[u]).y << point_cloud->at((*graph_map)[u]).z << std::endl;
//         
        osg::Vec3 osg_source = OVEC_POINT_CAST(pt_source);
        osg::Vec3 osg_target = OVEC_POINT_CAST(pt_target);
        
        osg::Vec3 center = osg::Vec3((pt_source.x + pt_target.x)/2, (pt_source.y + pt_target.y)/2,
            (pt_source.z + pt_target.z)/2);
        float height = (osg_source - osg_target).length();
        float radius = 0.2f;
        
        // This is the default direction for the cylinders to face in OpenGL
        osg::Vec3 z = osg::Vec3(0,0,1);

        // Get diff between two points you want cylinder along
        osg::Vec3 diff = (osg_source - osg_target);

        // Get CROSS product (the axis of rotation)
        osg::Vec3 t = z ^ diff;

        // Get angle. length is magnitude of the vector
        double angle = acos((z * diff) / diff.length());
        
        osg::ref_ptr<osg::Cylinder> cylinder = new osg::Cylinder(center,radius,height);
        cylinder->setRotation(osg::Quat(angle, osg::Vec3(t.x(), t.y(), t.z())));
        osg::Geode *node = new osg::Geode();
        osg::ref_ptr<osg::ShapeDrawable> shape = new osg::ShapeDrawable(cylinder);
        shape->setColor(osg::Vec4(0.8f, 0.8f, 0.4f, 1.0f));
        node->addDrawable(shape);
        edges->addChild(node);
    }
    
    scene_root_->addChild(graph);
    graph_map_.insert(std::make_pair<PointCloud*, osg::Group*>(point_cloud, graph.get()));
     
    return;
}

void Visualizer::removePointCloud(PointCloud* point_cloud)
{
    if (cloud_map_.find(point_cloud) == cloud_map_.end())
        return;
    
    scene_root_->removeChild(cloud_map_[point_cloud]);
    std::map<PointCloud*, osg::Geode*>::iterator itr = cloud_map_.find(point_cloud);
    cloud_map_.erase(itr);
}

void Visualizer::removeGraph(PointCloud* point_cloud)
{
    if (graph_map_.find(point_cloud) == graph_map_.end())
        return;
    
    scene_root_->removeChild(graph_map_[point_cloud]);
    std::map<PointCloud*, osg::Group*>::iterator itr = graph_map_.find(point_cloud);
    graph_map_.erase(itr);
}

void Visualizer::updatePointCloud(PointCloud* point_cloud)
{
    removePointCloud(point_cloud);
    addPointCloud(point_cloud);
}

void Visualizer::updateGraph(PointCloud* point_cloud)
{
    removeGraph(point_cloud);
    addGraph(point_cloud);
}

void Visualizer::closeLight()
{
    scene_root_->getOrCreateStateSet()->setMode(
        GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
}


