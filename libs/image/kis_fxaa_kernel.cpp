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
#include "KoMixColorsOp.h"



#include <QThreadPool>




#define FXAA_THRESHOLD 10
#define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0


struct pixelEdgeFlags
{
    bool edgeAtLeft;
    bool edgeAtTop;
};

struct blendFactorData
{
    float left;
    float top;
    float right;
    float bottom;

};

bool operator==(const blendFactorData& lhs, const blendFactorData& rhs)
{
    return 
        lhs.left == rhs.left &&
        lhs.top == rhs.top &&
        lhs.right == rhs.right &&
        lhs.bottom == rhs.bottom;
}

typedef enum {
    Near,
    Far,
    None,
} edgeSide;

edgeSide operator-(const edgeSide& in) {
    if (in == Near) {
        return Far;
    }
    if (in == Far) {
        return Near;
    }
    return None;
};

struct edgeShape {
    edgeSide sides[2];
    int edgeDistances[2];

    void calculateCoverage(float &nearCoverage, float &farCoverage) const {
        float distances[2] = {
            edgeDistances[0] + 0.5f,
            edgeDistances[1] + 0.5f
        };
        float total = distances[0] + distances[1];
        nearCoverage = 0.0;
        farCoverage = 0.0;
        for (int i = 0; i < 2; i++) {
            edgeSide side = sides[i];
            if (side == None) {
                continue;
            }
            float center = distances[i] + total / 2.0;
            if (center < -0.5) {
                // this triangle ends before it reaches this pixel
                continue;
            }
            float coverage = 0.0;
            if (center <= 0.5) {
                float coverageBase = center + 0.5;
                // height at the pixel edge that is intersecting the triangle
                float coverageHeight = ((distances[i] - 0.5) / center) * 0.5;
                coverage = coverageBase * coverageHeight * 0.5;
            } else {
                float heightA = (1 - (distances[i] - 0.5) / center) * 0.5;
                float heightB = (1 - (distances[i] + 0.5) / center) * 0.5;
                coverage = (heightA + heightB) * 0.5;
            }
            if (side == Near) {
                nearCoverage += coverage;
            } else if (side == Far) {
                farCoverage += coverage;
            }
        }
    };
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
                              int searchRadius,
                              bool adjustForLocalContrast)
{
    int needsRectMarginNeg = searchRadius + 2;
    int needsRectMarginPos = searchRadius + 2;

    const KoColorSpace* cs = device->colorSpace();

    const QRect needsRect = rect.adjusted(-needsRectMarginNeg, -needsRectMarginNeg, needsRectMarginPos, needsRectMarginPos);

    KisFixedPaintDeviceSP lumaFixedPD = new KisFixedPaintDevice(cs);
    lumaFixedPD->setRect(needsRect);
    lumaFixedPD->lazyGrowBufferWithoutInitialization();
    device->readBytes(lumaFixedPD->data(), lumaFixedPD->bounds());

    qInfo() << "applying AA filter";
    calculateLuma(cs, lumaFixedPD->data(), lumaFixedPD->data(), lumaFixedPD->allocatedPixels());

    // DEBUG: write lumaFixedPD to the lumaFixedPD
    // dst->writeBytes(lumaFixedPD->data(), lumaFixedPD->bounds());

    lumaFixedPD->convertTo(KoColorSpaceRegistry::instance()->graya16("Gray-D50-elle-V4-srgbtrc.icc"));

    QVector<pixelEdgeFlags> edgeFlagRowInit = QVector<pixelEdgeFlags>(lumaFixedPD->bounds().width(), pixelEdgeFlags{false,false});
    QVector<QVector<pixelEdgeFlags>> edgeFlags(lumaFixedPD->bounds().height(), edgeFlagRowInit);

    // KisSequentialConstIteratorProgress lumaIt(lumaFixedPD, needsRect);
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

    const quint32 pixelsOfInputArea = abs(needsRect.width() * needsRect.height());
    QVector<quint16> luma(pixelsOfInputArea, 0);

    // copy all of lumaFixedPD to luma
    quint32 nPixels = lumaFixedPD->allocatedPixels();
    KIS_ASSERT_RECOVER_RETURN(nPixels == pixelsOfInputArea);
    for (quint32 curPixel = 0; curPixel < nPixels; curPixel++) {
        const quint8 *data = lumaFixedPD->constData() + (curPixel * lumaFixedPD->pixelSize());
        luma[curPixel] = toDoubleFuncPtr[greyIndex](data, lumaFixedPD->colorSpace()->channels()[greyIndex]->pos());
        // qInfo() << "luma:" << luma[curPixel] << "(" << curPixel << "/" << nPixels << ")";
    }
    if (progressUpdater) progressUpdater->setProgress(20);

    qInfo() << "got here";

    for (int y = 2; y < needsRect.height()-2; y++) {
        for (int x = 2; x < needsRect.width()-2; x++) {
            // int poscurr = y * needsRect.width() + x;
            // int postop = (y - 1) * needsRect.width() + x;
            // int posleft = y * needsRect.width() + x - 1;

            // // qInfo() << "indexing array of length" << luma.length() << "with values poscurr" << poscurr << "postop" << postop << "and posleft" << posleft <<
            // //     "at position" << x << "," << y << "out of" << needsRect.width() << "," << needsRect.height() <<
            // //     "with needsRect" << needsRect << "and edge flags needsRect" << lumaFixedPD->bounds();
            // // QThread::msleep(30);

            // bool edgeAtLeft = abs(luma[poscurr] - luma[posleft]) > FXAA_THRESHOLD;
            // bool edgeAtTop = abs(luma[poscurr] - luma[postop]) > FXAA_THRESHOLD;
            // // qInfo() << "Left: abs(" << luma[poscurr] << "-" << luma[posleft] << ") =" << abs(luma[poscurr] - luma[posleft]) << "; > " << FXAA_THRESHOLD << "=" << edgeAtLeft;
            // // qInfo() << "Top: abs(" << luma[poscurr] << "-" << luma[postop] << ") =" << abs(luma[poscurr] - luma[postop]) << "; > " << FXAA_THRESHOLD << "=" << edgeAtTop;

            // // edgeAtLeft = true;

            int pos = y * needsRect.width() + x;
            int posLeft = y * needsRect.width() + x - 1;
            int posTop = (y - 1) * needsRect.width() + x;
            int posRight = y * needsRect.width() + x + 1;
            int posBottom = (y + 1) * needsRect.width() + x;
            int posLeftLeft = y * needsRect.width() + x - 2;
            int posTopTop = (y - 2) * needsRect.width() + x;

            // Calculate the lumas we need to start with:
            float L = luma[pos];

            float LLeft = luma[posLeft];
            float LTop  = luma[posTop];

            // Calculate deltas we're starting with:
            float deltaLeft = abs(L - LLeft);
            float deltaTop = abs(L - LTop);

            // We do the usual threshold:
            pixelEdgeFlags edges = pixelEdgeFlags{deltaLeft > FXAA_THRESHOLD, deltaTop > FXAA_THRESHOLD};

            // Then discard if there is no edge:
            if (!edges.edgeAtLeft && !edges.edgeAtTop) {
                continue;
            }

            // If we're not adjusting for local contrast, write and continue
            if (!adjustForLocalContrast) {
                edgeFlags[y][x] = edges;
                continue;
            }

            // Calculate right and bottom deltas:
            float LRight  = luma[posRight];
            float LBottom  = luma[posBottom];
            float deltaRight = abs(L - LRight);
            float deltaBottom = abs(L - LBottom);

            // Calculate left-left and top-top deltas:
            float LLeftLeft  = luma[posLeftLeft];
            float LTopTop  = luma[posTopTop];
            float deltaLeftLeft = abs(L - LLeftLeft);
            float deltaTopTop = abs(L - LTopTop);

            // TODO: Incorporate left-bottom, left-top, top-left and top-right deltas

            // Calculate the maximum delta:
            float maxDelta = std::max({deltaLeft, deltaTop, deltaRight, deltaBottom, deltaLeftLeft, deltaTopTop});

            // Local contrast adaptation:
            edges = pixelEdgeFlags{
                edgeAtLeft: edges.edgeAtLeft && maxDelta < SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * deltaLeft,
                edgeAtTop: edges.edgeAtTop && maxDelta < SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * deltaTop,
            };

            edgeFlags[y][x] = edges;
        }
    }

    qInfo() << "finished calculating edgeFlags";

    // // Preview edge flags
    // KisSequentialIteratorProgress finalIt(device, rect, progressUpdater);
    // do {
    //     KoColor col(device->colorSpace());
    //     int r, g, b, a;
    //     r = g = b = 0;
    //     a = 255;
    //     int needsRect_x = finalIt.x() - rect.x() + needsRectMarginNeg;
    //     int needsRect_y = finalIt.y() - rect.y() + needsRectMarginNeg;
    //     if (edgeFlags[needsRect_y][needsRect_x].edgeAtLeft) {
    //         r = 255;
    //     }
    //     if (edgeFlags[needsRect_y][needsRect_x].edgeAtTop) {
    //         g = 255;
    //     }
    //     // b = finalIt.y() % 255;
    //     // if (r == 0 && b == 0) {
    //     //     // qInfo() << "x:" << finalIt.x() << "-" << rect.x() << "+" << searchRadius << "=" << needsRect_x <<
    //     //     //            "y:" << finalIt.y() << "-" << rect.y() << "+" << searchRadius << "=" << needsRect_y <<
    //     //     //            "edgeFlags:" << edgeFlags[needsRect_y][needsRect_x].edgeAtLeft << edgeFlags[needsRect_y][needsRect_x].edgeAtTop << ".";
    //     //     // QThread::msleep(30);
    //     //     // g = 255;
    //     // }
    //     col.fromQColor(QColor(r, g, b, a));
    //     const int pixelSize = device->colorSpace()->pixelSize();
    //     memcpy(finalIt.rawData(), col.data(), pixelSize);
            
    // } while (finalIt.nextPixel());

    QVector<blendFactorData> blendFactorsInit = QVector<blendFactorData>(rect.width(), blendFactorData{});
    QVector<QVector<blendFactorData>> blendFactors(rect.height(), blendFactorsInit);

    for (int y = 0; y < rect.height(); y++) {
        for (int x = 0; x < rect.width(); x++) {
            int nrPosX = x - rect.x() + needsRectMarginNeg;
            int nrPosY = y - rect.y() + needsRectMarginNeg;

            blendFactorData blends = {};
            
            if (edgeFlags[nrPosY][nrPosX].edgeAtTop) {
                int edgeLengthLeft = 1;
                while (edgeLengthLeft < searchRadius) {
                    pixelEdgeFlags flags = edgeFlags[nrPosY][nrPosX-edgeLengthLeft];
                    if (!flags.edgeAtTop) {
                        break;
                    }
                    edgeLengthLeft++;
                }

                int edgeLengthRight = 1;
                while (edgeLengthRight < searchRadius) {
                    pixelEdgeFlags flags = edgeFlags[nrPosY][nrPosX+edgeLengthRight];
                    if (!flags.edgeAtTop) {
                        break;
                    }
                    edgeLengthRight++;
                }

                // step back a pixel to the last one with an edge
                edgeLengthLeft--;
                edgeLengthRight--;

                struct edgeShapeFlags {
                    bool above;
                    bool in_line;
                };

                edgeShapeFlags edgesLeft = {
                    above: edgeFlags[nrPosY-1][nrPosX-edgeLengthLeft].edgeAtLeft,
                    in_line: edgeFlags[nrPosY][nrPosX-edgeLengthLeft].edgeAtLeft,
                };
                edgeShapeFlags edgesRight = {
                    above: edgeFlags[nrPosY-1][nrPosX-edgeLengthRight+1].edgeAtLeft,
                    in_line: edgeFlags[nrPosY][nrPosX-edgeLengthRight+1].edgeAtLeft,
                };

                edgeSide edgeSideLeft = None;
                edgeSide edgeSideRight = None;
                if (edgesLeft.above ^ edgesLeft.in_line) {
                    edgeSideLeft = edgesLeft.above ? Far : Near;
                }
                if (edgesRight.above ^ edgesRight.in_line) {
                    edgeSideRight = edgesRight.above ? Far : Near;
                }
                if (edgeSideLeft == None) {
                    edgeSideLeft = -edgeSideLeft;
                }
                if (edgeSideRight == None) {
                    edgeSideRight = -edgeSideRight;
                }

                edgeShape shape = {
                    sides: {edgeSideLeft, edgeSideRight},
                    edgeDistances: {edgeLengthLeft, edgeLengthRight},
                };
                float nearCoverage;
                float farCoverage;
                shape.calculateCoverage(nearCoverage, farCoverage);
                blends.top = nearCoverage;
                blends.bottom = farCoverage;
            }

            // if (edgeFlags[nrPosY][nrPosX].edgeAtLeft) {
            //     int edgeLengthUp = 1;
            //     while (edgeLengthUp < searchRadius) {
            //         pixelEdgeFlags flags = edgeFlags[nrPosY-edgeLengthUp][nrPosX];
            //         if (!flags.edgeAtLeft) {
            //             break;
            //         }
            //         edgeLengthUp++;
            //     }

            //     int edgeLengthDown = 1;
            //     while (edgeLengthDown < searchRadius) {
            //         pixelEdgeFlags flags = edgeFlags[nrPosY+edgeLengthDown][nrPosX];
            //         if (!flags.edgeAtLeft) {
            //             break;
            //         }
            //         edgeLengthDown++;
            //     }

            //     // step back a pixel to the last one with an edge
            //     edgeLengthUp--;
            //     edgeLengthDown--;
            // }

            blendFactors[y][x] = blends;
        }
    }


    KisSequentialIterator leftIt(device, rect.translated(-1, 0));
    KisSequentialIterator upIt(device, rect.translated(0, -1));
    KisSequentialIterator rightIt(device, rect.translated(1, 0));
    KisSequentialIterator downIt(device, rect.translated(0, 1));
    KisSequentialIteratorProgress finalIt(device, rect, progressUpdater);
    do {
        const int pixelSize = device->colorSpace()->pixelSize();

        blendFactorData factors = blendFactors[finalIt.y()][finalIt.x()]; // bottom and left

        // load bottom and right from pixel above and to the left respectively
        // TODO: expand the blends by 1 so this always works -- or maybe wrap?
        if (upIt.y() >= 0) {
            factors.bottom = blendFactors[upIt.y()][upIt.x()].bottom;
        } else {
            factors.bottom = 0;
        }
        if (leftIt.x() >= 0) {
            factors.right = blendFactors[leftIt.y()][leftIt.x()].right;
        } else {
            factors.right = 0;
        }

        if (factors == blendFactorData{}) {
            memcpy(finalIt.rawData(), finalIt.oldRawData(), pixelSize);
            continue;
        }

        bool horizontal = factors.left + factors.right > factors.top + factors.bottom;

        const quint8* pixels[3];

        float chosenFactors[2] = {};
        if (horizontal) {
            pixels[0] = leftIt.oldRawData();
            chosenFactors[0] = factors.left;
            pixels[1] = rightIt.oldRawData();
            chosenFactors[1] = factors.right;
        } else {
            pixels[0] = upIt.oldRawData();
            chosenFactors[0] = factors.top;
            pixels[1] = downIt.oldRawData();
            chosenFactors[1] = factors.bottom;
        }
        pixels[2] = finalIt.oldRawData();

        qint16 weights[3];
        {
            // blend center with neighbours (e.g. a, b) by factors (e.g. fa, fb)
            // weight the blending according to the magnitude of the factors
            // ((center * (1 - fa) + a * fa) * (fa/(fb+fa))) + ((center * (1 - fb) + b * fb) * (fb/(fb+fa)))
            // => [wolfram alpha says] (center * (-fa^2 + fa - fb^2 + fb) + a * fa^2 + b * fb^2)/(fa + fb)
            // we can ignore the common divisor as it'll be handled by sumOfWeights
            float fa = chosenFactors[0];
            float fb = chosenFactors[1];
            weights[0] = pow(fa, 2) * 255;
            weights[1] = pow(fb, 2) * 255;
            weights[2] = (-pow(fa, 2) + fa - pow(fb, 2) + fb) * 255;
        }
        int sumOfWeights = weights[0] + weights[1] + weights[2];

        KoColor final(device->colorSpace());
        device->colorSpace()->mixColorsOp()->mixColors(pixels, weights, 3, final.data(), sumOfWeights);
        memcpy(finalIt.rawData(), final.data(), pixelSize);
    } while (
        finalIt.nextPixel()
        && leftIt.nextPixel()
        && upIt.nextPixel()
        && rightIt.nextPixel()
        && downIt.nextPixel()
    );
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
