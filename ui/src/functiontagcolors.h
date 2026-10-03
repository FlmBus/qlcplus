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

/** Accent colour for a fixture group, by the group's registration index.
 *
 *  Group names are free text, so there is no table to look them up in. Hashing
 *  the name was the obvious answer but gives collisions - with a dozen groups
 *  some inevitably share a hue. Taking the colour from the order groups are
 *  first seen instead makes every group distinct, and since a project loads in
 *  the same order everywhere, a group keeps its colour across machines.
 *  Indices are spread by the golden angle, so any number of groups stays as
 *  far apart in hue as possible and a new group never disturbs the others. */
QColor functionGroupTagColor(int groupIndex, const QColor& background);

/** Black or white, whichever reads better on $fill. For solid pills, where the
 *  text sits on the tag colour rather than on the row. */
QColor readableTextOn(const QColor& fill);

/** @} */

#endif // FUNCTIONTAGCOLORS_H
