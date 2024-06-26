/* This file is part of the KDE project

   SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "SvgTextChangeCommand.h"

#include <math.h>
#include <klocalizedstring.h>

#include "KoSvgTextShape.h"
#include "KoSvgTextShapeMarkupConverter.h"

SvgTextChangeCommand::SvgTextChangeCommand(KoSvgTextShape *shape,
                                           const QString &svg,
                                           const QString &defs,
                                           KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_shape(shape)
    , m_svg(svg)
    , m_defs(defs)
{
    Q_ASSERT(shape);
    setText(kundo2_i18n("Change SvgTextTool"));
    KoSvgTextShapeMarkupConverter converter(m_shape);
    converter.convertToSvg(&m_oldSvg, &m_oldDefs);
}

SvgTextChangeCommand::~SvgTextChangeCommand()
{
}

void SvgTextChangeCommand::redo()
{
    m_shape->update();
    KoSvgTextShapeMarkupConverter converter(m_shape);
    // Hardcoded resolution?
    converter.convertFromSvg(m_svg, m_defs, m_shape->boundingRect(), 72.0);
    m_shape->update();
}

void SvgTextChangeCommand::undo()
{
    m_shape->update();
    KoSvgTextShapeMarkupConverter converter(m_shape);
    // Hardcoded resolution?
    converter.convertFromSvg(m_oldSvg, m_oldDefs, m_shape->boundingRect(), 72.0);
    m_shape->update();
}
