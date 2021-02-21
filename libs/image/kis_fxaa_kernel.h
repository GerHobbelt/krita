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

#ifndef KIS_FXAA_KERNEL_H
#define KIS_FXAA_KERNEL_H

#include "kritaimage_export.h"
#include "kis_types.h"

#include <Eigen/Core>
#include <boost/optional.hpp>

class QRect;

class KRITAIMAGE_EXPORT KisFXAAKernel
{
public:
    KisFXAAKernel();

    /**
     * @brief applyFXAA
     * This applies the FXAA filter to the device.
     * @param device the device to apply to.
     * @param rect the affected rect.
     * @param channelFlags the affected channels.
     * @param progressUpdater the progress updater if it exists.
     * @param searchRadius is the radius of the search for steps.
     * this is useful for fringe effects.
     */
    static void applyFXAA(KisPaintDeviceSP device,
                          const QRect& rect,
                          const QBitArray &channelFlags,
                          KoUpdater *progressUpdater,
                          int searchRadius,
                          bool adjustForLocalContrast);
};

#endif // KIS_FXAA_KERNEL_H
