/*
 *  Copyright (c) 2017 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_fxaa_kernel.h"
#include "kis_global.h"
#include "kis_convolution_kernel.h"
#include <kis_convolution_painter.h>
#include <KoCompositeOpRegistry.h>
#include <QRect>
#include <KoColorSpace.h>
#include <kis_iterator_ng.h>
#include <QVector3D>
#include <kis_fixed_paint_device.h>
#include <KisSequentialIteratorProgress.h>


struct pixelEdgeFlags
{
    bool edgeAtRight;
    bool edgeAtBottom;
};

KisFXAAKernel::KisFXAAKernel()
{

}
/*
 * TODO: comment
 */

void KisFXAAKernel::applyFXAA(KisPaintDeviceSP device,
                              const QRect &rect,
                              const QBitArray &channelFlags,
                              KoUpdater *progressUpdater,
                              int searchRadius)
{

    const KoColorSpace* cs = device->colorSpace();

    KisFixedPaintDevice luma(cs);
    luma.setRect(rect);
    luma.lazyGrowBufferWithoutInitialization();
    device->readBytes(luma.data(), luma.bounds());

    qInfo() << "applying AA filter";
    calculateLuma(cs, luma.data(), luma.data(), luma.allocatedPixels());

    // DEBUG: write luma to the luma
    // dst->writeBytes(luma.data(), luma.bounds());

    luma.convertTo(KoColorSpaceRegistry::instance()->graya16("Gray-D50-elle-V4-srgbtrc.icc"));

    QVector<pixelEdgeFlags> edgeFlagRowInit = QVector<pixelEdgeFlags>(luma.bounds().width(), pixelEdgeFlags{false,false});
    QVector<QVector<pixelEdgeFlags>> edgeFlags(luma.bounds().height(), edgeFlagRowInit);

    // KisSequentialConstIteratorProgress lumaIt(luma, rect);
    // while (lumaIt.nextPixel()) {
    //     calculateEdgeFlags(luma.colorSpace(), lumaIt.rawData(), edgeFlags, lumaIt.x(), lumaIt.y());
    // }

    const int tileHeightMinus1 = rect.height() - 1;
    const int tileWidthMinus1 = rect.width() - 1;

    // Foreach INNER pixel in tile

    QRect rectTopLeft = rectTopLeft.adjusted(0, 0, -1, -1);
    QRect rectTopRight = rectTopLeft.translated(1, 0);
    QRect rectBottomLeft = rectTopLeft.translated(0, 1);
    KisSequentialConstIteratorProgress topLeftIt(luma, rectTopLeft);
    KisSequentialConstIteratorProgress topRightIt(luma, rectTopRight);
    KisSequentialConstIteratorProgress bottomLeftIt(luma, rectBottomLeft);
    while 
            double pos = y * rect.width() + x;
            double posdown = (y - 1) * rect.width() + x;
            double posright = y * rect.width() + x + 1;

            bool edgeAtRight = abs(luma[pos] - luma[posright]) > FXAA_THRESHOLD;
            bool edgeAtBottom = abs(luma[pos] - luma[posdown]) > FXAA_THRESHOLD;

            edgeFlags[x][y] = pixelEdgeFlags{edgeAtRight, edgeAtBottom};
        }
    }
}

void calculateLuma(const KoColorSpace* cs, quint8* src, quint8* dst, qint32 nPixels) {
    QHash<QString, QVariant> params;
    params["type"] = 2; // BT601
    KoColorTransformation* transfo;
    transfo = cs->createColorTransformation("desaturate_adjustment", params);
    transfo->transform(src, dst, nPixels);
}

// inline void calculateEdgeFlags(const KoColorSpace* cs, quint8* luma, QVector<QVector<pixelEdgeFlags>>& edgeFlags, int x, int y) {
// {

// }
