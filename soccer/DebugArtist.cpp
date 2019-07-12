#include "DebugArtist.hpp"
#include "LogUtils.hpp"

void DebugArtist::drawLine(const Geometry2d::Segment& line, const QColor& qc,
                           const std::string& layer) {
    if (log_frame_) {
        Packet::DebugPath* dbg = log_frame_->add_debug_paths();
        dbg->set_layer(findDebugLayer(layer));
        *dbg->add_points() = line.pt[0];
        *dbg->add_points() = line.pt[1];
        dbg->set_color(color(qc));
    }
}

void DebugArtist::drawLine(Geometry2d::Point p0, Geometry2d::Point p1,
                           const QColor& color, const std::string& layer) {
    if (log_frame_) {
        drawLine(Geometry2d::Segment(p0, p1), color, layer);
    }
}

void DebugArtist::drawText(const std::string& text, Geometry2d::Point pos,
                           const QColor& qc, const std::string& layer) {
    if (log_frame_) {
        Packet::DebugText* dbg = log_frame_->add_debug_texts();
        dbg->set_layer(findDebugLayer(layer));
        dbg->set_text(text);
        *dbg->mutable_pos() = pos;
        dbg->set_color(color(qc));
    }
}

void DebugArtist::drawSegment(const Geometry2d::Segment& line, const QColor& qc,
                              const std::string& layer) {
    if (log_frame_) {
        Packet::DebugPath* dbg = log_frame_->add_debug_paths();
        dbg->set_layer(findDebugLayer(layer));
        *dbg->add_points() = line.pt[0];
        *dbg->add_points() = line.pt[1];
        dbg->set_color(color(qc));
    }
}

int DebugArtist::findDebugLayer(const std::string& layer) {
    auto it = debug_layer_map_.find(layer);
    if (it == debug_layer_map_.end()) {
        debug_layer_map_[layer] = num_debug_layers_;
        debug_layers_.push_back(layer);
        int idx = num_debug_layers_;
        num_debug_layers_++;
        return idx;
    } else {
        // Existing layer
        return it->second;
    }
}

void DebugArtist::drawPolygon(const Geometry2d::Point* pts, int n,
                              const QColor& qc, const std::string& layer) {
    if (log_frame_) {
        Packet::DebugPath* dbg = log_frame_->add_debug_polygons();
        dbg->set_layer(findDebugLayer(layer));
        for (int i = 0; i < n; ++i) {
            *dbg->add_points() = pts[i];
        }
        dbg->set_color(color(qc));
    }
}

void DebugArtist::drawPolygon(const std::vector<Geometry2d::Point>& pts,
                              const QColor& qc, const std::string& layer) {
    drawPolygon(pts.data(), pts.size(), qc, layer);
}

void DebugArtist::drawPolygon(const Geometry2d::Polygon& polygon,
                              const QColor& qc, const std::string& layer) {
    this->drawPolygon(polygon.vertices, qc, layer);
}

void DebugArtist::drawCircle(Geometry2d::Point center, float radius,
                             const QColor& qc, const std::string& layer) {
    if (log_frame_) {
        Packet::DebugCircle* dbg = log_frame_->add_debug_circles();
        dbg->set_layer(findDebugLayer(layer));
        *dbg->mutable_center() = center;
        dbg->set_radius(radius);
        dbg->set_color(color(qc));
    }
}

void DebugArtist::drawArc(const Geometry2d::Arc& arc, const QColor& qc,
                          const std::string& layer) {
    Packet::DebugArc* dbg = log_frame_->add_debug_arcs();
    dbg->set_layer(findDebugLayer(layer));
    *dbg->mutable_center() = arc.center();
    dbg->set_radius(arc.radius());
    dbg->set_start(arc.start());
    dbg->set_end(arc.end());
    dbg->set_color(color(qc));
}

void DebugArtist::drawShape(const std::shared_ptr<Geometry2d::Shape>& obs,
                            const QColor& color, const std::string& layer) {
    std::shared_ptr<Geometry2d::Circle> circObs =
        std::dynamic_pointer_cast<Geometry2d::Circle>(obs);
    std::shared_ptr<Geometry2d::Polygon> polyObs =
        std::dynamic_pointer_cast<Geometry2d::Polygon>(obs);
    std::shared_ptr<Geometry2d::CompositeShape> compObs =
        std::dynamic_pointer_cast<Geometry2d::CompositeShape>(obs);
    if (circObs)
        drawCircle(circObs->center, circObs->radius(), color, layer);
    else if (polyObs)
        drawPolygon(polyObs->vertices, color, layer);
    else if (compObs) {
        for (const std::shared_ptr<Geometry2d::Shape>& obs :
             compObs->subshapes())
            drawShape(obs, color, layer);
    }
}

void DebugArtist::drawShapeSet(const Geometry2d::ShapeSet& shapes,
                               const QColor& color, const std::string& layer) {
    for (auto& shape : shapes.shapes()) {
        drawShape(shape, color, layer);
    }
}

void DebugArtist::setLogFrame(std::shared_ptr<Packet::LogFrame> log_frame) {
    log_frame_ = log_frame;
}
