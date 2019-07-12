#pragma once

#include <QColor>

#include <Geometry2d/CompositeShape.hpp>
#include <Geometry2d/ShapeSet.hpp>
#include <Geometry2d/Segment.hpp>
#include <Geometry2d/Point.hpp>
#include <Geometry2d/Polygon.hpp>
#include <Geometry2d/Arc.hpp>
#include <protobuf/LogFrame.pb.h>

class DebugArtist {
public:
    /**
     * @defgroup drawing_functions Drawing Functions
     * These drawing functions add certain shapes/lines to the current LogFrame.
     * Each time the FieldView updates, it reads the LogFrame and draws these
     * items.
     * This way debug data can be drawn on-screen and also logged.
     *
     * Each drawing function also associates the drawn content with a particular
     * 'layer'.  Separating drawing items into layers lets you choose at runtime
     * which items actually get drawn.
     */

    /** @ingroup drawing_functions */
    void drawLine(const Geometry2d::Segment& line,
                  const QColor& color = Qt::black,
                  const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawSegment(const Geometry2d::Segment& line,
                     const QColor& color = Qt::black,
                     const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawLine(Geometry2d::Point p0, Geometry2d::Point p1,
                  const QColor& color = Qt::black,
                  const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawCircle(Geometry2d::Point center, float radius,
                    const QColor& color = Qt::black,
                    const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawPolygon(const Geometry2d::Polygon& pts,
                     const QColor& color = Qt::black,
                     const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawArc(const Geometry2d::Arc& arc, const QColor& color = Qt::black,
                 const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawPolygon(const Geometry2d::Point* pts, int n,
                     const QColor& color = Qt::black,
                     const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawPolygon(const std::vector<Geometry2d::Point>& pts,
                     const QColor& color = Qt::black,
                     const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawText(const std::string& text, Geometry2d::Point pos,
                  const QColor& color = Qt::black,
                  const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawShape(const std::shared_ptr<Geometry2d::Shape>& obs,
                   const QColor& color = Qt::black,
                   const std::string& layer = std::string());
    /** @ingroup drawing_functions */
    void drawShapeSet(const Geometry2d::ShapeSet& shapes,
                      const QColor& color = Qt::black,
                      const std::string& layer = std::string());

    const std::vector<std::string>& debugLayers() const { return debug_layers_; }

    /// Returns the number of a debug layer given its name
    int findDebugLayer(const std::string& layer);

    void setLogFrame(std::shared_ptr<Packet::LogFrame> log_frame);

    /// Pointer to the current log frame.
    std::shared_ptr<Packet::LogFrame> log_frame_ = nullptr;

private:
    /// Map from debug layer name to ID
    std::map<std::string, int> debug_layer_map_;

    /// Debug layers in order by ID
    std::vector<std::string> debug_layers_;

    /// Number of debug layers
    int num_debug_layers_ = 0;
};
