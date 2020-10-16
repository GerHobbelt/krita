/*
 *  Copyright (c) 2020 Dmitrii Utkin <loentar@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "recorder_export_config.h"

#include <kis_config.h>

#include <QString>
#include <QDir>

namespace
{
const QString keyAnimationExport = "ANIMATION_EXPORT";
const QString keyFfmpegPath = "ffmpeg_path";
const QString keyVideoDirectory = "recorder_export/videodirectory";
const QString keyInputFps = "recorder_export/inputfps";
const QString keyFps = "recorder_export/fps";
const QString keyResize = "recorder_export/resize";
const QString keySize = "recorder_export/size";
const QString keyLockRatio = "recorder_export/lockratio";
const QString keyProfileIndex = "recorder_export/profileIndex";
const QString keyProfiles = "recorder_export/profiles";

const QList<RecorderProfile> defaultProfiles = {
    { "MP4 x264",   "mp4",  "-vf \"scale=$WIDTH:$HEIGHT\" -c:v libx264 -r $FPS -pix_fmt yuv420p" },
    { "GIF",        "gif",  "-vf \"fps=$FPS,scale=$WIDTH:$HEIGHT:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse\" -loop -1" },
    { "Matroska",   "mkv",  "-vf \"scale=$WIDTH:$HEIGHT\" -r $FPS" },
    { "WebM",       "webm", "-vf \"scale=$WIDTH:$HEIGHT\" -r $FPS" },
    { "MP4 x264 (+4s Flash effect)",  "mp4", "-filter_complex \""
                                            "[0]scale=$WIDTH:$HEIGHT[p0];"
                                            "[p0]reverse[p1];"
                                            "[p1]trim=start_frame=0:end_frame=1[p3];"
                                            "[p3]reverse[p4];"
                                            "[p4]fps=$FPS[p5];"
                                            "[p5]tpad=stop_mode=clone:stop_duration=3[p6];"
                                            "[p6]fade=type=in:color=white:start_time=0.8:duration=0.2[v1];"
                                            "[0]scale=$WIDTH:$HEIGHT[v2];"
                                            "[v2][v1]concat=n=2:v=1"
                                            "\" -c:v libx264 -r $FPS -pix_fmt yuv420p" },
    { "Custom1",  "editme", "-vf \"scale=$WIDTH:$HEIGHT\" -r $FPS" },
    { "Custom2",  "editme", "-vf \"scale=$WIDTH:$HEIGHT\" -r $FPS" },
    { "Custom3",  "editme", "-vf \"scale=$WIDTH:$HEIGHT\" -r $FPS" },
    { "Custom4",  "editme", "-vf \"scale=$WIDTH:$HEIGHT\" -r $FPS" }
};
}

RecorderExportConfig::RecorderExportConfig(bool readOnly)
    : config(new KisConfig(readOnly))
{
}

RecorderExportConfig::~RecorderExportConfig()
{
    delete config;
}


int RecorderExportConfig::inputFps() const
{
    return config->readEntry(keyInputFps, 30);
}

void RecorderExportConfig::setInputFps(int value)
{
    config->writeEntry(keyInputFps, value);
}


int RecorderExportConfig::fps() const
{
    return config->readEntry(keyFps, 30);
}

void RecorderExportConfig::setFps(int value)
{
    config->writeEntry(keyFps, value);
}


bool RecorderExportConfig::resize() const
{
    return config->readEntry(keyResize, false);
}

void RecorderExportConfig::setResize(bool value)
{
    config->writeEntry(keyResize, value);
}


QSize RecorderExportConfig::size() const
{
    return config->readEntry(keySize, QSize(1024, 1024));
}

void RecorderExportConfig::setSize(const QSize &value)
{
    config->writeEntry(keySize, value);
}


bool RecorderExportConfig::lockRatio() const
{
    return config->readEntry(keyLockRatio, false);
}

void RecorderExportConfig::setLockRatio(bool value)
{
    config->writeEntry(keyLockRatio, value);
}


int RecorderExportConfig::profileIndex() const
{
    return config->readEntry(keyProfileIndex, 0);
}

void RecorderExportConfig::setProfileIndex(int value)
{
    config->writeEntry(keyProfileIndex, value);
}

QList<RecorderProfile> RecorderExportConfig::profiles() const
{
    const QString &profilesStr = config->readEntry(keyProfiles, QString());
    if (profilesStr.isEmpty())
        return ::defaultProfiles;

    QList<RecorderProfile> profiles;
    const QStringList &profilesList = profilesStr.split("\n");
    for (const QString &profileStr : profilesList) {
        const QStringList& profileList = profileStr.split("|");
        if (profileList.size() != 3)
            continue;

        profiles.append({profileList[0], profileList[1], profileList[2]});
    }
    return profiles;
}

void RecorderExportConfig::setProfiles(const QList<RecorderProfile> &value)
{
    QString outValue;
    for (const RecorderProfile &profile : value) {
        outValue += profile.name % "|" % profile.extension % "|" % profile.arguments % "\n";
    }
    config->writeEntry(keyProfiles, outValue);
}


QList<RecorderProfile> RecorderExportConfig::defaultProfiles() const
{
    return ::defaultProfiles;
}


QString RecorderExportConfig::ffmpegPath() const
{
    return config->exportConfiguration(keyAnimationExport)->getPropertyLazy(keyFfmpegPath, "ffmpeg");
}

void RecorderExportConfig::setFfmpegPath(const QString &value)
{
    KisPropertiesConfigurationSP exportConfig = config->exportConfiguration(keyAnimationExport);
    exportConfig->setProperty(keyFfmpegPath, value);
    config->setExportConfiguration(keyAnimationExport, exportConfig);
}


QString RecorderExportConfig::videoDirectory() const
{
    return config->readEntry(keyVideoDirectory, QDir::homePath());
}

void RecorderExportConfig::setVideoDirectory(const QString &value)
{
    config->writeEntry(keyVideoDirectory, value);
}
