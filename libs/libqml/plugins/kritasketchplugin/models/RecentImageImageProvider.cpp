/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2012 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "RecentImageImageProvider.h"
#include <QFile>
#include <QImage>

#include <KoStore.h>
#include <KisDocument.h>

RecentImageImageProvider::RecentImageImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage RecentImageImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    int width = 38;
    int height = 38;

    if (size) {
        *size = QSize(width, height);
    }

    QSize sz(requestedSize.width() > 0 ? requestedSize.width() : width,
             requestedSize.height() > 0 ? requestedSize.height() : height);

    QFile f(id);
    QImage thumbnail;

    if (f.exists()) {
        if (f.fileName().endsWith(".kra", Qt::CaseInsensitive)) {
            // try to use any embedded thumbnail
            KoStore *store = KoStore::createStore(id, KoStore::Read);
            KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(store, QImage());

            QString thumbnailPath = QLatin1String("Thumbnails/thumbnail.png");
            QString previewPath = QLatin1String("preview.png");
            bool thumbnailExists = store->hasFile(thumbnailPath);
            bool previewExists = store->hasFile(previewPath);
            QString pathToUse = thumbnailExists ? thumbnailPath : (previewExists ? previewPath : "");

            if (!pathToUse.isEmpty() && store->open(pathToUse)) {
                // Hooray! No long delay for the user...
                const QByteArray thumbnailData = store->read(store->size());

                if (thumbnail.loadFromData(thumbnailData) &&
                        (thumbnail.width() >= width || thumbnail.height() >= height)) {
                    thumbnail = thumbnail.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
            }
            delete store;

        }
        else {
            QImage img(id);
            if (img.width() >= sz.width() || img.height() >= sz.height()) {
                thumbnail = img.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    }
    return thumbnail;
}
