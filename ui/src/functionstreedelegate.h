/*
  Q Light Controller Plus
  functionstreedelegate.h

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

#ifndef FUNCTIONSTREEDELEGATE_H
#define FUNCTIONSTREEDELEGATE_H

#include <QStyledItemDelegate>

class FunctionsTreeWidget;

/** @addtogroup ui_functions
 * @{
 */

/**
 * Draws the type and fixture group of a function as pills in front of its
 * title, for names following the "TYP - Group - Description" convention.
 *
 * The three parts live in their own columns of the item - only the first is
 * visible, the other two are read from here - so this delegate paints rather
 * than parses, and the text of each part stays the plain string that sorting
 * and every other consumer uses.
 *
 * Titles start at a common offset so the eye can run down them, which is why
 * the width of the pill area is a property of the whole tree rather than of
 * one row. The widget measures it and sets it here.
 *
 * Rows whose name does not follow the convention, and folders, are handed
 * straight back to the base class.
 */
class FunctionsTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    FunctionsTreeDelegate(FunctionsTreeWidget* tree);

    /** Width reserved for the pills, so every title begins at the same place.
        Measured over the whole tree by the widget. */
    void setTagZoneWidth(int width);

    /** Space one pill needs for $text in $font. Used both when painting and
        when the widget measures the tree, so the two cannot disagree. */
    static int pillWidth(const QString& text, const QFont& font);

    /** Space the type and group pills of one row need together, including the
        gap that separates them from the title. */
    static int tagZoneWidth(const QString& type, const QString& group, const QFont& font);

    /** The pill font for $base: pills are bold, so they read as labels rather
        than as part of the title. */
    static QFont pillFont(const QFont& base);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const;

private:
    FunctionsTreeWidget* m_tree;
    int m_tagZoneWidth;
};

/** @} */

#endif // FUNCTIONSTREEDELEGATE_H
