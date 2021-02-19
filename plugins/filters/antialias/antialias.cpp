/*
 * This file is part of the KDE project
 *
 * Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#include "antialias.h"
#include <kis_fxaa_kernel.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>

#include <filter/kis_filter_category_ids.h>
#include <filter/kis_filter_configuration.h>
#include <kis_selection.h>
#include <kis_paint_device.h>
#include <kis_processing_information.h>
#include "kis_lod_transform.h"

#include <kpluginfactory.h>

#include <klocalizedstring.h>
#include <filter/kis_filter_registry.h>

#include "widgets/kis_multi_integer_filter_widget.h"

K_PLUGIN_FACTORY_WITH_JSON(KritaAntiAliasFactory, "kritaantialias.json", registerPlugin<KritaAntiAliasFilter>();)

KritaAntiAliasFilter::KritaAntiAliasFilter(QObject *parent, const QVariantList &)
    : QObject(parent)
{
    KisFilterRegistry::instance()->add(KisFilterSP(new KisFXAAFilter()));
}

KritaAntiAliasFilter::~KritaAntiAliasFilter()
{
}

KisFXAAFilter::KisFXAAFilter(): KisFilter(id(), FiltersCategoryInDevelopmentId, i18n("&AntiAlias..."))
{
    setColorSpaceIndependence(FULLY_INDEPENDENT);
    setSupportsPainting(false); // TODO: find out what this means
    setSupportsAdjustmentLayers(true);
    setShowConfigurationWidget(false);
    setSupportsLevelOfDetail(false); // TODO: find out what this means
}

void KisFXAAFilter::processImpl(KisPaintDeviceSP device, const QRect &rect, const KisFilterConfigurationSP config, KoUpdater *progressUpdater) const
{
    Q_ASSERT(device != 0);

    KisFilterConfigurationSP configuration = config ? config : new KisFilterConfiguration(id().id(), 1);

    KisLodTransformScalar t(device);

    // TODO: add configuration options

    QBitArray channelFlags;
    if (configuration) {
        channelFlags = configuration->channelFlags();
    }

    // TODO: make antialiasing type configurable?

    KisFXAAKernel::applyFXAA(device,
                             rect,
                             channelFlags,
                             progressUpdater,
                             config->getBool("transparency", false));
}

KisFilterConfigurationSP KisFXAAFilter::defaultConfiguration() const
{
    KisFilterConfigurationSP config = factoryConfiguration();
    // TODO: add configuration options

    return config;
}

KisConfigWidget * KisFXAAFilter::createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP, bool) const
{
    vKisIntegerWidgetParam param;
    param.push_back(KisIntegerWidgetParam(1, 16, 32, i18nc("radius to search for steps when antialiasing", "Search Radius"), "searchRadius"));
    KisMultiIntegerFilterWidget * w = new KisMultiIntegerFilterWidget(id().id(),  parent,  id().id(),  param);
    w->setConfiguration(defaultConfiguration());
    return w;
}

QRect KisFXAAFilter::neededRect(const QRect &rect, const KisFilterConfigurationSP _config, int lod) const
{
    KisLodTransformScalar t(lod);

    QVariant value;
    /**
     * NOTE: integer division by two is done on purpose,
     *       because the kernel size is always odd
     */
    // const int halfWidth = _config->getProperty("horizRadius", value) ? KisFXAAKernel::kernelSizeFromRadius(t.scale(value.toFloat())) / 2 : 5;
    // const int halfHeight = _config->getProperty("vertRadius", value) ? KisFXAAKernel::kernelSizeFromRadius(t.scale(value.toFloat())) / 2 : 5;

    // return rect.adjusted(-halfWidth * 2, -halfHeight * 2, halfWidth * 2, halfHeight * 2);
    return rect;
}

QRect KisFXAAFilter::changedRect(const QRect &rect, const KisFilterConfigurationSP _config, int lod) const
{
    KisLodTransformScalar t(lod);

    QVariant value;

    // const int halfWidth = _config->getProperty("horizRadius", value) ? KisFXAAKernel::kernelSizeFromRadius(t.scale(value.toFloat())) / 2 : 5;
    // const int halfHeight = _config->getProperty("vertRadius", value) ? KisFXAAKernel::kernelSizeFromRadius(t.scale(value.toFloat())) / 2 : 5;

    // return rect.adjusted( -halfWidth, -halfHeight, halfWidth, halfHeight);
    return rect;
}

#include "antialias.moc"
