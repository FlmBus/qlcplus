/*
  Q Light Controller Plus
  functiontagcolors.h

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

#ifndef FUNCTIONTAGCOLORS_H
#define FUNCTIONTAGCOLORS_H

#include <QColor>
#include <QString>

/** @addtogroup ui_functions
 * @{
 */

/**
 * Colours for the type and group columns of the function tree, for function
 * names following our "TYP - Group - Description" convention.
 *
 * Both calls take the row's background colour rather than a light/dark flag,
 * because they guarantee a contrast ratio against it: no single colour can be
 * read on both a white and a near-black row, as one dark enough for white
 * sits near 2.5:1 against black.
 */

/** Accent colour for a type token. A short closed set, so these are picked by
 *  hand and verified. Unknown tokens get a neutral, which means a new type
 *  reads sensibly without a code change. */
QColor functionTypeTagColor(const QString& type, const QColor& background);

/** Accent colour for a fixture group. Group names are free text, so the colour
 *  is derived from the name instead of looked up: the hue comes from a stable
 *  hash of the name, while saturation and lightness are fixed to values that
 *  stay legible on $background. Same name always gives the same colour, on
 *  every machine and every run. */
QColor functionGroupTagColor(const QString& group, const QColor& background);

/** @} */

#endif // FUNCTIONTAGCOLORS_H
