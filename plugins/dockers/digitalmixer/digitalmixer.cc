/*
 *  SPDX-FileCopyrightText: 2009 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "digitalmixer.h"
#include "digitalmixer_dock.h"

#include <kpluginfactory.h>

#include <KoDockFactoryBase.h>
#include <KoDockRegistry.h>

K_PLUGIN_FACTORY_WITH_JSON(DigitalMixerPluginFactory, "krita_digitalmixer.json", registerPlugin<DigitalMixerPlugin>();)

class DigitalMixerDockFactory : public KoDockFactoryBase {
public:
    DigitalMixerDockFactory()
    {
    }

    QString id() const override
    {
        return QString( "DigitalMixer" );
    }

    virtual Qt::DockWidgetArea defaultDockWidgetArea() const
    {
        return Qt::RightDockWidgetArea;
    }

    QDockWidget* createDockWidget() override
    {
        DigitalMixerDock * dockWidget = new DigitalMixerDock();
        
        dockWidget->setObjectName(id());

        return dockWidget;
    }

    DockPosition defaultDockPosition() const override
    {
        return DockMinimized;
    }
private:


};


DigitalMixerPlugin::DigitalMixerPlugin(QObject *parent, const QVariantList &)
    : QObject(parent)
{
    KoDockRegistry::instance()->add(new DigitalMixerDockFactory());
}

DigitalMixerPlugin::~DigitalMixerPlugin()
{
}

#include "digitalmixer.moc"
