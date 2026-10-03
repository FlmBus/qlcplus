/*
  Q Light Controller Plus
  functionstreewidget.h

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


#ifndef FUNCTIONSTREEWIDGET_H
#define FUNCTIONSTREEWIDGET_H

#include <QTreeWidget>
#include <QHash>

class FunctionsTreeDelegate;
class Function;
class Doc;

/** @addtogroup ui UI
 * @{
 */

/**
 * FunctionsTreeWidget represents the tree of QLC+ functions,
 * organized in folders.
 * It can be used anywhere in QLC+ to display functions organized
 * by folders.
 * If drag & drop flags are turned on, it becomes a full node
 * editor, accessing functions' properties. Basically this mode
 * has to be used only by FunctionManager
 *
 * Data is organized in the following way:
 *
 * |          COL_NAME           |  COL_TYPE  | COL_GROUP |      COL_PATH      |
 *  ----------------------------- ------------ ----------- --------------------
 * | Text: description part of   | Text: type | Text:     | Text: path of      |
 * |       the function name, or |       code |   fixture |   folder (not set  |
 * |       the whole name when   |   (empty   |   group   |   for functions)   |
 * |       it is not of the form |   for      |   (as     |                    |
 * |       "TYP - Group - Descr" |   folders) |   above)  |                    |
 * | Data:                       |            |           |                    |
 * |   Qt::UserRole: function ID |            |           |                    |
 * |                 (or invalid)|            |           |                    |
 * |   Qt::UserRole + 1: function|            |           |                    |
 * |          type (Function::Type)          |           |                     |
 *  ----------------------------- ------------ ----------- --------------------
 *
 * COL_PATH is past the last visible column, so it is storage only - the same
 * trick mainline uses for it. The type and group columns are presentation
 * only: they are parsed back out of the function's name every time an item is
 * updated, and never written anywhere. A function's name remains one single
 * string in the engine and in the project file, so projects stay readable by
 * mainline QLC+.
 */

/**
 * A tree item that sorts on all three name columns at once. Whichever column
 * the user sorted by is the primary key, and the remaining two break ties in
 * the order the name itself is written, so that e.g. sorting by group still
 * leaves each group's functions ordered by type and then by description.
 *
 * Every item placed in a FunctionsTreeWidget should be one of these, so that
 * all of them sort by the same rules.
 */
class FunctionTreeItem : public QTreeWidgetItem
{
public:
    FunctionTreeItem(QTreeWidget* parent)
        : QTreeWidgetItem(parent) { }

    FunctionTreeItem(QTreeWidgetItem* parent)
        : QTreeWidgetItem(parent) { }

    bool operator<(const QTreeWidgetItem& other) const;
};

class FunctionsTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    /** Columns of the tree. Everything below COL_PATH is visible; COL_PATH
        itself is only a place to keep a folder's path around. */
    enum Column
    {
        COL_NAME = 0,
        COL_TYPE = 1,
        COL_GROUP = 2,
        COL_PATH = 3
    };

    FunctionsTreeWidget(Doc* doc, QWidget *parent = 0);

    /** Split a function name of the form "Group - TYP - Title" into its three
        parts. Returns false, leaving the outputs untouched, when $name does
        not follow the convention - as song-specific functions generally do
        not.

        "name" is the whole string as the engine stores it; the three parts are
        the group, the type and the title. */
    static bool splitName(const QString& name, QString& group,
                          QString& type, QString& title);

    /** The full, unsplit name of whatever $item represents: a function's name
        straight from the engine, or a folder's name. Use this instead of
        reading COL_NAME when the name is shown outside the tree. */
    QString itemName(const QTreeWidgetItem* item) const;

    /** Update all functions to function tree */
    void updateTree();

    void clearTree();

    void functionNameChanged(quint32 fid);

    /** Add the Function with the given ID and returns
     *  a pointer to the created item */
    QTreeWidgetItem* addFunction(quint32 fid);

    /** Return a suitable parent item for the $function's path */
    QTreeWidgetItem* parentItem(const Function* function);

    /** Get the ID of the function represented by $item. */
    quint32 itemFunctionId(const QTreeWidgetItem* item) const;

    /** Get the item that represents the given function. */
    QTreeWidgetItem* functionItem(const Function* function);

    /** True if at least one item with children is currently collapsed. */
    bool hasCollapsedItems() const;

    /** Expand everything, or collapse everything if nothing is collapsed. */
    void toggleExpandAll();

    /** Which colour a fixture group gets, as an index into the generated
        palette, or -1 for a group this tree has not seen. Groups are
        registered as their functions are added, so the index follows the order
        the project loads in - which is the same on every machine, and leaves
        existing groups alone when a new one appears. */
    int groupColorIndex(const QString& group) const;

private:
    /** Update $item's contents from the given $function */
    void updateFunctionItem(QTreeWidgetItem* item, const Function* function);

    /** Give $group a colour index if it does not have one yet. */
    void registerGroup(const QString& group);

    /** Re-measure the width the pills need, so every title starts at the same
        offset. Called whenever the items change. */
    void updateTagZoneWidth();

private:
    Doc* m_doc;
    FunctionsTreeDelegate* m_delegate;
    QHash <QString, int> m_groupColors;

    /*********************************************************************
     * Tree folders
     *********************************************************************/
public:
    void addFolder();

    void deleteFolder(QTreeWidgetItem *item);

private:
    QTreeWidgetItem *folderItem(QString name);

private slots:
    void slotItemChanged(QTreeWidgetItem *item);

    void slotUpdateChildrenPath(QTreeWidgetItem *root);

private:
    QHash <QString, QTreeWidgetItem *> m_foldersMap;

    /*********************************************************************
     * Drag & Drop events
     *********************************************************************/
protected:
    void resizeEvent(QResizeEvent *event);

    void mousePressEvent(QMouseEvent *event);

    void dropEvent(QDropEvent *event);

private:
    QList<QTreeWidgetItem *>m_draggedItems;
};

/** @} */

#endif // FUNCTIONSTREEWIDGET_H
