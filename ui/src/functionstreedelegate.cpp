/*
  Q Light Controller Plus
  functionstreedelegate.cpp

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

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

#include "functionstreedelegate.h"
#include "functionstreewidget.h"
#include "functiontagcolors.h"

#define PILL_H_PADDING 6    //!< space inside a pill, left and right
#define PILL_V_INSET   2    //!< how far a pill sits inside the row
#define PILL_GAP       4    //!< between the two pills
#define PILL_RADIUS    3    //!< corner rounding
#define TITLE_GAP      8    //!< between the last pill and the title

FunctionsTreeDelegate::FunctionsTreeDelegate(FunctionsTreeWidget* tree)
    : QStyledItemDelegate(tree)
    , m_tree(tree)
    , m_tagZoneWidth(0)
{
}

void FunctionsTreeDelegate::setTagZoneWidth(int width)
{
    m_tagZoneWidth = width;
}

QFont FunctionsTreeDelegate::pillFont(const QFont& base)
{
    QFont font = base;
    font.setBold(true);
    return font;
}

int FunctionsTreeDelegate::pillWidth(const QString& text, const QFont& font)
{
    if (text.isEmpty())
        return 0;

    return QFontMetrics(font).horizontalAdvance(text) + (2 * PILL_H_PADDING);
}

int FunctionsTreeDelegate::tagZoneWidth(const QString& group, const QString& type,
                                        const QFont& font)
{
    const QFont pill = pillFont(font);

    int width = pillWidth(group, pill);
    const int typeWidth = pillWidth(type, pill);

    if (width > 0 && typeWidth > 0)
        width += PILL_GAP;
    width += typeWidth;

    if (width > 0)
        width += TITLE_GAP;

    return width;
}

void FunctionsTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
{
    /* The parsed parts live in columns the view does not show, so they are
       read from the model rather than parsed again here: paint() runs on every
       repaint, scroll and hover. */
    const QAbstractItemModel* model = index.model();
    QString type, group;

    if (model != NULL)
    {
        type = model->index(index.row(), FunctionsTreeWidget::COL_TYPE,
                            index.parent()).data().toString();
        group = model->index(index.row(), FunctionsTreeWidget::COL_GROUP,
                             index.parent()).data().toString();
    }

    // Untagged functions and folders are none of our business
    if (type.isEmpty() && group.isEmpty())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const QString title = opt.text;

    /* Let the style paint the row as it normally would - background,
       selection, focus rectangle, expand arrow, icon - but without the text,
       which we place ourselves after the pills. */
    opt.text.clear();
    QStyle* style = opt.widget != NULL ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    if (textRect.isValid() == false)
        return;

    const QColor background = opt.palette.color(QPalette::Base);
    const QFont pill = pillFont(opt.font);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setFont(pill);

    int x = textRect.left();
    const QRect pillRect = textRect.adjusted(0, PILL_V_INSET, 0, -PILL_V_INSET);

    /* Group pill, then type pill, in the order the name is written. Solid
       fill with the text in whichever of black or white reads on it, so a pill
       is legible whatever colour it ended up with - including on a selected
       row, where the row's own highlight never shows through. */
    if (group.isEmpty() == false && m_tree != NULL)
    {
        const QColor fill = functionGroupTagColor(m_tree->groupColorIndex(group), background);
        const int width = pillWidth(group, pill);

        painter->setBrush(fill);
        painter->drawRoundedRect(QRect(x, pillRect.top(), width, pillRect.height()),
                                 PILL_RADIUS, PILL_RADIUS);
        painter->setPen(readableTextOn(fill));
        painter->drawText(QRect(x, pillRect.top(), width, pillRect.height()),
                          Qt::AlignCenter, group);
        painter->setPen(Qt::NoPen);
        x += width + PILL_GAP;
    }

    if (type.isEmpty() == false)
    {
        const QColor fill = functionTypeTagColor(type, background);
        const int width = pillWidth(type, pill);

        painter->setBrush(fill);
        painter->drawRoundedRect(QRect(x, pillRect.top(), width, pillRect.height()),
                                 PILL_RADIUS, PILL_RADIUS);
        painter->setPen(readableTextOn(fill));
        painter->drawText(QRect(x, pillRect.top(), width, pillRect.height()),
                          Qt::AlignCenter, type);
    }

    painter->restore();

    /* Titles all start at the same offset, so they can be read as a column
       rather than at whatever ragged position the pills happen to end. A row
       whose pills are wider than the common zone - possible if the zone was
       capped - pushes its own title along instead of overlapping it. */
    const int titleLeft = qMax(textRect.left() + m_tagZoneWidth,
                               x + pillWidth(type, pill) + TITLE_GAP);
    const int titleWidth = textRect.right() - titleLeft;

    if (titleWidth <= 0)
        return;

    painter->save();
    painter->setFont(opt.font);
    painter->setPen(opt.state & QStyle::State_Selected
                    ? opt.palette.color(QPalette::HighlightedText)
                    : opt.palette.color(QPalette::Text));
    painter->drawText(QRect(titleLeft, textRect.top(), titleWidth, textRect.height()),
                      Qt::AlignVCenter | Qt::AlignLeft,
                      QFontMetrics(opt.font).elidedText(title, Qt::ElideRight, titleWidth));
    painter->restore();
}
