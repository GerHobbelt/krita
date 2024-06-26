/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2018 Emmet & Eoin O'Neill <emmetoneill.pdx@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_TOOL_UTILS_H
#define KIS_TOOL_UTILS_H

#include <kis_types.h>
#include <kritaui_export.h>

class QPoint;
class KoColor;
class KoCanvasBase;
class QPainterPath;

namespace KisToolUtils {

struct KRITAUI_EXPORT ColorSamplerConfig {
    ColorSamplerConfig();

    bool toForegroundColor;
    bool updateColor;
    bool addColorToCurrentPalette;
    bool normaliseValues;
    bool sampleMerged;
    int radius;
    int blend;

    void save() const;
    void load();
private:
    static const QString CONFIG_GROUP_NAME;
};

/**
 * Sample a color based on the given position on the given paint device.
 *
 * out_color   - Output parameter returning newly sampled color.
 * dev         - Paint device to sample from.
 * pos         - Position to sample from.
 * blendColor  - Optional color to be blended with.
 * radius      - Sampling area radius in pixels.
 * blend       - Blend percentage. 100% all sampled, 0% all blendColor.
 * pure        - Whether to bypass radius, blending, and active layer settings for pure sampling.
 *
 * RETURN      - Returns TRUE whenever a valid color is sampled.
 */
bool KRITAUI_EXPORT sampleColor(KoColor &out_color, KisPaintDeviceSP dev, const QPoint &pos,
                                KoColor const *const blendColor = nullptr, int radius = 1,
                                int blend = 100, bool pure = false);

/**
 * Recursively search a node with a non-transparent pixel
 */
KisNodeSP KRITAUI_EXPORT findNode(KisNodeSP node, const QPoint &point, bool wholeGroup, bool editableOnly = true);

KisNodeList KRITAUI_EXPORT findNodes(KisNodeSP node, const QPoint &point, bool wholeGroup,
                                     bool includeGroups = true, bool editableOnly = true);

/**
 * @brief shapeHoverInfoCrossLayer
 * get hover info of shapes on all layers.
 *
 * @param canvas -- current canvas.
 * @param point -- the point in document coordinates.
 * @param shapeType -- the shapeID of the found shape.
 * @param isHorizontal -- when this is a textShape, sets whether the writing mode is horizontal.
 * @return
 */
QPainterPath KRITAUI_EXPORT shapeHoverInfoCrossLayer(KoCanvasBase *canvas, const QPointF &point,
                                                     QString &shapeType, bool *isHorizontal = nullptr, bool skipCurrentShapes = true);
/**
 * @brief selectShapeCrossLayer
 * Tries to select a shape under the cursor regardless of which layer it is on,
 * to do so, it will always select the layer first and then the shape.
 * @param canvas -- the current canvas.
 * @param point -- the point in document coordinates.
 * @param shapeType -- the required shapeId, if empty, selects any koshape.
 * @return whether a shape was succesfully selected.
 */
bool KRITAUI_EXPORT selectShapeCrossLayer(KoCanvasBase *canvas, const QPointF &point,
                                          const QString &shapeType = QString(), bool skipCurrentShapes = true);


/**
 * return true if success
 * Clears the image. Selection is optional, use 0 to clear everything.
 */
bool KRITAUI_EXPORT clearImage(KisImageSP image, KisNodeList nodes, KisSelectionSP selection);

/**
 * Moves the cursor (hot spot) of the primary screen to the global screen position (x, y).
 *
 * This function is provided as a replacement to `QCursor::setPos` which does
 * not properly handle multiple monitors with different scale factors.
 */
void KRITAUI_EXPORT setCursorPos(const QPoint &point);
}

#endif // KIS_TOOL_UTILS_H
