/*
  Q Light Controller Plus
  functiontagcolors.cpp

  Copyright (c) Massimo Callegari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QHash>
#include <qmath.h>

#include "functiontagcolors.h"

/** Contrast ratio every tag colour has to reach against the row background.
    WCAG AA for text of this size. */
#define MIN_CONTRAST 4.5

/****************************************************************************
 * Contrast
 ****************************************************************************/

/** WCAG relative luminance of an sRGB colour. */
static qreal relativeLuminance(const QColor& color)
{
    qreal channel[3] = { color.redF(), color.greenF(), color.blueF() };

    for (int i = 0; i < 3; i++)
    {
        if (channel[i] <= 0.03928)
            channel[i] = channel[i] / 12.92;
        else
            channel[i] = qPow((channel[i] + 0.055) / 1.055, 2.4);
    }

    return 0.2126 * channel[0] + 0.7152 * channel[1] + 0.0722 * channel[2];
}

static qreal contrastRatio(const QColor& a, const QColor& b)
{
    const qreal la = relativeLuminance(a);
    const qreal lb = relativeLuminance(b);

    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
}

/**
 * Walk a colour's lightness until it reads against $background.
 *
 * Fixing HSL lightness alone is not quite enough: at the same lightness
 * yellow is perceptually far brighter than blue, so a single value leaves some
 * hues short. Nudging lightness per hue until the ratio is actually met keeps
 * the promise for every hue, and stays deterministic.
 */
static QColor withContrast(const QColor& color, const QColor& background)
{
    const bool darken = background.lightness() >= 128;

    int hue, saturation, lightness;
    color.getHsl(&hue, &saturation, &lightness);

    QColor result = color;

    // 2/255 steps: fine enough to stop just past the threshold, bounded loop
    while (contrastRatio(result, background) < MIN_CONTRAST)
    {
        lightness += darken ? -2 : 2;

        if (lightness < 0 || lightness > 255)
            break;

        result = QColor::fromHsl(hue, saturation, lightness);
    }

    return result;
}

/****************************************************************************
 * Types - a closed set, picked by hand
 ****************************************************************************/

QColor functionTypeTagColor(const QString& type, const QColor& background)
{
    static QHash<QString, QColor> light;
    static QHash<QString, QColor> dark;

    if (light.isEmpty())
    {
        /* The light set is deliberately desaturated: saturated yellow or cyan
           on white cannot be read however good it looks in a colour picker.
           MOV is an ochre for that reason, not a yellow. */
        light.insert(QStringLiteral("COL"), QColor("#c0392b")); // colour
        light.insert(QStringLiteral("INT"), QColor("#6b7280")); // intensity
        light.insert(QStringLiteral("MOV"), QColor("#946200")); // movement
        light.insert(QStringLiteral("OVL"), QColor("#1f6feb")); // overlay
        light.insert(QStringLiteral("EFX"), QColor("#1a7f37")); // effects

        dark.insert(QStringLiteral("COL"), QColor("#ff7b72"));
        dark.insert(QStringLiteral("INT"), QColor("#9ca3af"));
        dark.insert(QStringLiteral("MOV"), QColor("#e3b341"));
        dark.insert(QStringLiteral("OVL"), QColor("#79c0ff"));
        dark.insert(QStringLiteral("EFX"), QColor("#3fb950"));
    }

    const bool onDark = background.lightness() < 128;
    const QHash<QString, QColor>& table = onDark ? dark : light;

    const QString key = type.trimmed().toUpper();
    const QColor color = table.contains(key)
                       ? table.value(key)
                       : (onDark ? QColor("#8b949e") : QColor("#57606a"));

    // hand-picked already, but a wildly themed background is still possible
    return withContrast(color, background);
}

/****************************************************************************
 * Groups - free text, so derived rather than looked up
 ****************************************************************************/

QColor functionGroupTagColor(int groupIndex, const QColor& background)
{
    const bool onDark = background.lightness() < 128;

    if (groupIndex < 0)
        return withContrast(onDark ? QColor("#8b949e") : QColor("#57606a"), background);

    /* The golden angle: successive indices land as far apart on the colour
       wheel as they can, for any number of groups, and adding one never moves
       the others. Hashing the name would have been stateless but collides -
       with a dozen groups some would share a hue. */
    const qreal hue = fmod(qreal(groupIndex) * 137.508, 360.0);

    /* Saturation and lightness carry legibility, the hue carries identity.
       The starting lightness suits the theme; withContrast() then moves it
       until the ratio is genuinely met for this particular hue. */
    const int saturation = onDark ? 150 : 170;
    const int lightness = onDark ? 185 : 95;

    return withContrast(QColor::fromHsl(int(hue), saturation, lightness), background);
}

QColor readableTextOn(const QColor& fill)
{
    /* Prefer white, and only switch to black when black is clearly the better
       of the two.
       Simply taking whichever wins does not work: the point where they cross
       is at a relative luminance of about 0.18, which is still a dark colour,
       so fills that plainly want white text come out a hair in black's favour
       - a dark teal measured 4.62 against black and 4.54 against white, and
       got black. At that margin the winner is noise rather than a difference
       anyone can see. Requiring black to be half again better keeps those
       dark fills on white text, while the pale fills of a dark theme, where
       black wins several times over, still get black. */
    return contrastRatio(Qt::black, fill) >= (1.5 * contrastRatio(Qt::white, fill))
           ? QColor(Qt::black) : QColor(Qt::white);
}
