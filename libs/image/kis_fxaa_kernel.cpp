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
#include "kis_math_toolbox.h"
#include <KoUpdater.h>



#include <QThreadPool>




#define FXAA_THRESHOLD 10.0


struct pixelEdgeFlags
{
    bool edgeAtRight;
    bool edgeAtTop;
};

KisFXAAKernel::KisFXAAKernel()
{

}
/*
 * TODO: comment
 */

void calculateLuma(const KoColorSpace* cs, quint8* src, quint8* dst, qint32 nPixels);

void KisFXAAKernel::applyFXAA(KisPaintDeviceSP device,
                              const QRect &rect,
                              const QBitArray &channelFlags,
                              KoUpdater *progressUpdater,
                              int searchRadius)
{

    const KoColorSpace* cs = device->colorSpace();

    KisFixedPaintDeviceSP lumaFixedPD = new KisFixedPaintDevice(cs);
    lumaFixedPD->setRect(rect);
    lumaFixedPD->lazyGrowBufferWithoutInitialization();
    device->readBytes(lumaFixedPD->data(), lumaFixedPD->bounds());

    qInfo() << "applying AA filter";
    calculateLuma(cs, lumaFixedPD->data(), lumaFixedPD->data(), lumaFixedPD->allocatedPixels());

    // DEBUG: write lumaFixedPD to the lumaFixedPD
    // dst->writeBytes(lumaFixedPD->data(), lumaFixedPD->bounds());

    lumaFixedPD->convertTo(KoColorSpaceRegistry::instance()->graya16("Gray-D50-elle-V4-srgbtrc.icc"));

    QVector<pixelEdgeFlags> edgeFlagRowInit = QVector<pixelEdgeFlags>(lumaFixedPD->bounds().width(), pixelEdgeFlags{false,false});
    QVector<QVector<pixelEdgeFlags>> edgeFlags(lumaFixedPD->bounds().height(), edgeFlagRowInit);

    // KisSequentialConstIteratorProgress lumaIt(lumaFixedPD, rect);
    // while (lumaIt.nextPixel()) {
    //     calculateEdgeFlags(lumaFixedPD->colorSpace(), lumaIt.rawData(), edgeFlags, lumaIt.x(), lumaIt.y());
    // }

    // Foreach INNER pixel in tile

    // QRect rectTopLeft = rectTopLeft.adjusted(0, 0, -1, -1);
    // QRect rectTopRight = rectTopLeft.translated(1, 0);
    // QRect rectBottomLeft = rectTopLeft.translated(0, 1);
    // KisSequentialConstIterator topLeftIt(lumaFixedPD, rectTopLeft);
    // KisSequentialConstIterator topRightIt(lumaFixedPD, rectTopRight);
    // KisSequentialConstIterator bottomLeftIt(lumaFixedPD, rectBottomLeft);

    int greyIndex = -1;
    QList<KoChannelInfo *> channels = lumaFixedPD->colorSpace()->channels();
    for (int i=0; i<channels.length(); i++) {
        bool isRightType = channels[i]->channelType() == KoChannelInfo::COLOR;
        bool isRightValueType = channels[i]->channelValueType() == KoChannelInfo::UINT16;
        // qInfo() << "got channel with name: " << channels[i]->name() << " and right type " << isRightType << " and right value type " << isRightValueType;
        if (isRightType && isRightValueType) {
            greyIndex = i;
        }
    }
    KIS_ASSERT_RECOVER_RETURN(greyIndex != -1);
 
    QVector<PtrToDouble> toDoubleFuncPtr(lumaFixedPD->colorSpace()->channels().count());
    KisMathToolbox mathToolbox;
    if (!mathToolbox.getToDoubleChannelPtr(lumaFixedPD->colorSpace()->channels(), toDoubleFuncPtr)) {
        return;
    }

    const quint32 pixelsOfInputArea = abs(rect.width() * rect.height());
    QVector<int16_t> luma(pixelsOfInputArea, 0);

    // copy all of lumaFixedPD to luma
    quint32 nPixels = lumaFixedPD->allocatedPixels();
    KIS_ASSERT_RECOVER_RETURN(nPixels == pixelsOfInputArea);
    for (quint32 curPixel = 0; curPixel > nPixels; curPixel++) {
        const quint8 *data = lumaFixedPD->constData() + (curPixel * lumaFixedPD->pixelSize());
        luma[curPixel] = toDoubleFuncPtr[greyIndex](data, lumaFixedPD->colorSpace()->channels()[greyIndex]->pos());
    }
    if (progressUpdater) progressUpdater->setProgress(20);

    const int tileHeightMinus1 = rect.height() - 1;
    const int tileWidthMinus1 = rect.width() - 1;

    qInfo() << "got here";

    for (int y = 0; y < tileHeightMinus1; y++) {
        for (int x = 0; x < tileWidthMinus1; x++) {
            int poscurr = y * rect.width() + x;
            int posup = (y + 1) * rect.width() + x;
            int posright = y * rect.width() + x + 1;

            // indexing array of length 111616 with values poscurr 218 posup 730 and posright 219
            // at position 218 , 0 out of 511 , 217 
            // with rect QRect(512,1536 512x218) and edge flags rect QRect(512,1536 512x218)
            // qInfo() << "indexing array of length" << luma.length() << "with values poscurr" << poscurr << "posup" << posup << "and posright" << posright <<
            //     "at position" << x << "," << y << "out of" << tileWidthMinus1 << "," << tileHeightMinus1 <<
            //     "with rect" << rect << "and edge flags rect" << lumaFixedPD->bounds();
            // QThread::msleep(30);
            

            bool edgeAtRight = abs(luma[poscurr] - luma[posright]) > FXAA_THRESHOLD;
            bool edgeAtTop = abs(luma[poscurr] - luma[posup]) > FXAA_THRESHOLD;

            edgeFlags[y][x] = pixelEdgeFlags{edgeAtRight, edgeAtTop};
        }
    }

    qInfo() << "finished calculating edgeFlags";

    KisSequentialIteratorProgress finalIt(device, rect, progressUpdater);
    for (auto row : edgeFlags) {
        for (auto flags : row) {
            KoColor col(device->colorSpace());
            int r, g, b, a;
            r = g = b = 0;
            a = 255;
            if (flags.edgeAtRight) {
                r = 255;
            }
            if (flags.edgeAtTop) {
                b = 255;
            }
            col.fromQColor(QColor(r, g, b, a));
            const int pixelSize = device->colorSpace()->pixelSize();
            memcpy(finalIt.rawData(), col.data(), pixelSize);
            finalIt.nextPixel();
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
