/*
  Q Light Controller Plus
  functionstreewidget.cpp

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

#include <QContextMenuEvent>
#include <QBrush>
#include <QTreeWidgetItemIterator>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QHeaderView>
#include <QCollator>
#include <QDebug>

#include "functiontagcolors.h"
#include "functionstreewidget.h"
#include "function.h"
#include "doc.h"

/* Separator between the parts of a fixture function's name:
   "TYP - Group - Description" */
static const QString KNameSeparator(" - ");

/** Compare two strings the way a person reads them: case is ignored and runs
    of digits compare by value, so "INT - Bars - 50%" lands above
    "INT - Bars - 100%". */
static int naturalCompare(const QString& left, const QString& right)
{
    static QCollator collator;
    static bool initialized = false;
    if (initialized == false)
    {
        collator.setNumericMode(true);
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        initialized = true;
    }

    return collator.compare(left, right);
}

bool FunctionTreeItem::operator<(const QTreeWidgetItem& other) const
{
    int column = FunctionsTreeWidget::COL_NAME;
    if (treeWidget() != NULL)
        column = treeWidget()->sortColumn();

    /* The sorted column leads; the other two follow in the order the name
       itself is written. */
    QList<int> keys;
    keys << column;
    if (column == FunctionsTreeWidget::COL_TYPE)
        keys << FunctionsTreeWidget::COL_GROUP << FunctionsTreeWidget::COL_NAME;
    else if (column == FunctionsTreeWidget::COL_GROUP)
        keys << FunctionsTreeWidget::COL_TYPE << FunctionsTreeWidget::COL_NAME;
    else
        keys << FunctionsTreeWidget::COL_TYPE << FunctionsTreeWidget::COL_GROUP;

    foreach (int key, keys)
    {
        int result = naturalCompare(text(key), other.text(key));
        if (result != 0)
            return result < 0;
    }

    return false;
}

FunctionsTreeWidget::FunctionsTreeWidget(Doc *doc, QWidget *parent) :
    QTreeWidget(parent)
  , m_doc(doc)
{
    /* The name of a fixture function carries three dimensions:
       "TYP - Group - Description". Give each one its own column so they line
       up and can be sorted on, and keep COL_PATH - storage for a folder's
       path - just past the last visible one. */
    setColumnCount(COL_PATH);
    QStringList labels;
    labels << tr("Function") << tr("Type") << tr("Group");
    setHeaderLabels(labels);

    /* The description can be long and the two parsed columns never are. */
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    header()->setSectionResizeMode(COL_TYPE, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(COL_GROUP, QHeaderView::ResizeToContents);

    /* Sort by type first: that reproduces the order the single-column tree
       used to show, since the type code led every name. */
    sortItems(COL_TYPE, Qt::AscendingOrder);

    /* Monospace, so names and numbers line up when scanning down the list.
       Take the platform's own fixed-pitch font - Menlo on macOS, Consolas on
       Windows, usually DejaVu Sans Mono on Linux - rather than hardcoding a
       family that may not exist everywhere, and keep the interface font's
       size so row heights stay as they were. */
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QFont base = font();
    if (base.pointSizeF() > 0)
        mono.setPointSizeF(base.pointSizeF());
    else if (base.pixelSize() > 0)
        mono.setPixelSize(base.pixelSize());
    setFont(mono);

    QTreeWidgetItem *root = invisibleRootItem();
    root->setFlags(root->flags() & ~Qt::ItemIsDropEnabled);

    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
                this, SLOT(slotItemChanged(QTreeWidgetItem*)));
}

bool FunctionsTreeWidget::splitName(const QString& name, QString& type,
                                   QString& group, QString& description)
{
    QStringList parts = name.split(KNameSeparator);
    if (parts.count() < 3)
        return false;

    /* Only treat the first part as a type code when it looks like one: a
       short, upper case tag. Without this a song function that happens to
       have dashes in its name - "Intro - Build - Drop" - would be torn apart
       as if it were a fixture function. */
    QString candidate = parts.first();
    if (candidate.length() < 2 || candidate.length() > 4)
        return false;
    for (int i = 0; i < candidate.length(); i++)
    {
        if (candidate.at(i).isUpper() == false && candidate.at(i).isDigit() == false)
            return false;
    }

    type = parts.takeFirst();
    group = parts.takeFirst();
    /* Anything left belongs to the description, separators and all. */
    description = parts.join(KNameSeparator);

    return true;
}

QString FunctionsTreeWidget::itemName(const QTreeWidgetItem* item) const
{
    if (item == NULL)
        return QString();

    Function* function = m_doc->function(itemFunctionId(item));
    if (function != NULL)
        return function->name();

    return item->text(COL_NAME);
}

void FunctionsTreeWidget::updateTree()
{
    blockSignals(true);

    clearTree();

    foreach (Function* function, m_doc->functions())
    {
        if (function->isVisible())
            updateFunctionItem(new FunctionTreeItem(parentItem(function)), function);
    }

    blockSignals(false);
}

void FunctionsTreeWidget::clearTree()
{
    m_foldersMap.clear();
    clear();
}

void FunctionsTreeWidget::functionNameChanged(quint32 fid)
{
    blockSignals(true);
    Function* function = m_doc->function(fid);
    if (function == NULL)
    {
        blockSignals(false);
        return;
    }

    QTreeWidgetItem* item = functionItem(function);
    if (item != NULL)
        updateFunctionItem(item, function);

    blockSignals(false);
}

QTreeWidgetItem *FunctionsTreeWidget::addFunction(quint32 fid)
{
    Function* function = m_doc->function(fid);
    if (function == NULL || function->isVisible() == false)
        return NULL;

    QTreeWidgetItem* item = functionItem(function);
    if (item != NULL)
        return item;

    blockSignals(true);
    QTreeWidgetItem* parent = parentItem(function);
    item = new FunctionTreeItem(parent);
    updateFunctionItem(item, function);
    if (parent != NULL)
        function->setPath(parent->text(COL_PATH));
    blockSignals(false);
    return item;
}

void FunctionsTreeWidget::updateFunctionItem(QTreeWidgetItem* item, const Function* function)
{
    Q_ASSERT(item != NULL);
    Q_ASSERT(function != NULL);
    QString type, group, description;
    if (splitName(function->name(), type, group, description))
    {
        item->setText(COL_NAME, description);
        item->setText(COL_TYPE, type);
        item->setText(COL_GROUP, group);
    }
    else
    {
        /* Not a fixture function: show the name as it is and leave the two
           parsed columns empty rather than inventing parts for it. */
        item->setText(COL_NAME, function->name());
        item->setText(COL_TYPE, QString());
        item->setText(COL_GROUP, QString());
    }

    /* The split name is still one name, so keep the whole of it within reach. */
    for (int i = COL_NAME; i < COL_PATH; i++)
        item->setToolTip(i, function->name());

    /* Colour the two parsed columns so the list can be scanned by eye. Both
       colours are derived against the row background, so they follow the
       system theme rather than assuming one. An unsplit name leaves both
       columns empty, and resetting the brushes keeps a renamed function from
       carrying a stale colour. */
    const QColor background = palette().color(QPalette::Base);
    if (type.isEmpty())
    {
        item->setForeground(COL_TYPE, QBrush());
        item->setForeground(COL_GROUP, QBrush());
    }
    else
    {
        item->setForeground(COL_TYPE, QBrush(functionTypeTagColor(type, background)));
        item->setForeground(COL_GROUP, QBrush(functionGroupTagColor(group, background)));
    }

    item->setIcon(COL_NAME, function->getIcon());
    item->setData(COL_NAME, Qt::UserRole, function->id());
    item->setData(COL_NAME, Qt::UserRole + 1, function->type());
    item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
}

QTreeWidgetItem* FunctionsTreeWidget::parentItem(const Function* function)
{
    Q_ASSERT(function != NULL);

    if (function->isVisible() == false)
        return NULL;

    return folderItem(function->path(true));
}

quint32 FunctionsTreeWidget::itemFunctionId(const QTreeWidgetItem* item) const
{
    if (item == NULL)
        return Function::invalidId();
    else
    {
        QVariant var = item->data(COL_NAME, Qt::UserRole);
        if (var.isValid() == false)
            return Function::invalidId();

        return var.toUInt();
    }
}

QTreeWidgetItem* FunctionsTreeWidget::functionItem(const Function* function)
{
    Q_ASSERT(function != NULL);

    if (function->isVisible() == false)
        return NULL;

    QTreeWidgetItem* parent = parentItem(function);
    Q_ASSERT(parent != NULL);

    for (int i = 0; i < parent->childCount(); i++)
    {
        QTreeWidgetItem* item = parent->child(i);
        if (itemFunctionId(item) == function->id())
            return item;
    }

    return NULL;
}

/*********************************************************************
 * Tree folders
 *********************************************************************/

void FunctionsTreeWidget::addFolder()
{
    blockSignals(true);

    /* No selection means the root of the tree. A selected function has no
       path of its own, so fall back to its parent folder - which is NULL for
       one sitting at the root, and that is the root again. */
    QTreeWidgetItem *item = NULL;
    if (selectedItems().isEmpty() == false)
    {
        item = selectedItems().first();
        if (item->text(COL_PATH).isEmpty())
            item = item->parent();
    }

    QString fullPath;
    if (item != NULL)
    {
        fullPath = item->text(COL_PATH);
        if (fullPath.endsWith('/') == false)
            fullPath.append("/");
    }

    QString newName = "New folder";

    int folderCount = 1;

    while (m_foldersMap.contains(fullPath + newName))
    {
        newName = "New Folder " + QString::number(folderCount++);
    }

    fullPath += newName;

    QTreeWidgetItem *folder = NULL;
    if (item != NULL)
        folder = new FunctionTreeItem(item);
    else
        folder = new FunctionTreeItem(this);
    folder->setText(COL_NAME, newName);
    folder->setIcon(COL_NAME, QIcon(":/folder.png"));
    folder->setData(COL_NAME, Qt::UserRole, Function::invalidId());
    folder->setText(COL_PATH, fullPath);
    folder->setFlags(folder->flags() | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);

    m_foldersMap[fullPath] = folder;
    if (item != NULL)
        item->setExpanded(true);

    blockSignals(false);

    scrollToItem(folder, QAbstractItemView::PositionAtCenter);
}

void FunctionsTreeWidget::deleteFolder(QTreeWidgetItem *item)
{
    if (item == NULL)
        return;

    QList<QTreeWidgetItem*> childrenList;
    for (int i = 0; i < item->childCount(); i++)
        childrenList.append(item->child(i));

    QListIterator <QTreeWidgetItem*> it(childrenList);
    while (it.hasNext() == true)
    {
        QTreeWidgetItem *child = it.next();
        quint32 fid = child->data(COL_NAME, Qt::UserRole).toUInt();
        if (fid != Function::invalidId())
        {
            m_doc->deleteFunction(fid);
            delete child;
        }
        else
            deleteFolder(child);
    }

    QString name = item->text(COL_PATH);

    if (m_foldersMap.contains(name))
        m_foldersMap.remove(name);

    delete item;
}

QTreeWidgetItem *FunctionsTreeWidget::folderItem(QString name)
{
    if (name.isEmpty())
    {
        // a newly created function has an empty path:
        // place it in the currently selected folder, if any
        if (selectedItems().count() > 0)
        {
            QString currFolder = selectedItems().first()->text(COL_PATH);
            if (m_foldersMap.contains(currFolder))
                return m_foldersMap[currFolder];
        }
        return invisibleRootItem();
    }

    if (m_foldersMap.contains(name))
        return m_foldersMap[name];

    qDebug() << "Folder" << name << "doesn't exist. Creating it...";

    QTreeWidgetItem *parentNode = NULL;
    QString fullPath;
    QStringList levelsList = name.split("/");
    foreach (QString level, levelsList)
    {
        if (fullPath.isEmpty() == false)
            fullPath.append("/");
        fullPath.append(level);

        // create only missing levels
        if (m_foldersMap.contains(fullPath) == false)
        {
            QTreeWidgetItem *folder = NULL;
            if (parentNode != NULL)
                folder = new FunctionTreeItem(parentNode);
            else
                folder = new FunctionTreeItem(this);
            folder->setText(COL_NAME, level);
            folder->setIcon(COL_NAME, QIcon(":/folder.png"));
            folder->setData(COL_NAME, Qt::UserRole, Function::invalidId());
            folder->setText(COL_PATH, fullPath);
            folder->setFlags(folder->flags() | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);

            m_foldersMap[fullPath] = folder;
            parentNode = folder;
        }
        else
            parentNode = m_foldersMap[fullPath];
    }

    return m_foldersMap[name];
}

void FunctionsTreeWidget::slotItemChanged(QTreeWidgetItem *item)
{
    blockSignals(true);
    qDebug() << "[FunctionsTreeWidget] TREE item changed";
    if (item->text(COL_PATH).isEmpty())
    {
        blockSignals(false);
        return;
    }

    QTreeWidgetItem *parent = item->parent();
    QString fullPath;
    if (parent != NULL)
    {
        fullPath = parent->text(COL_PATH);
        if (fullPath.endsWith('/') == false)
            fullPath.append("/");
    }
    fullPath.append(item->text(COL_NAME));

    m_foldersMap.remove(item->text(COL_PATH));
    item->setText(COL_PATH, fullPath);
    m_foldersMap[fullPath] = item;
    slotUpdateChildrenPath(item);

    blockSignals(false);
}

void FunctionsTreeWidget::slotUpdateChildrenPath(QTreeWidgetItem *root)
{
    if (root->childCount() == 0)
        return;
    for (int i = 0; i < root->childCount(); i++)
    {
        QTreeWidgetItem *child = root->child(i);

        // child can be a function node or another folder
        QString path = child->text(COL_PATH);
        if (path.isEmpty()) // function node
        {
            quint32 fid = child->data(COL_NAME, Qt::UserRole).toUInt();
            Function *func = m_doc->function(fid);
            if (func != NULL)
                func->setPath(root->text(COL_PATH));
        }
        else
        {
            slotItemChanged(child);
        }
    }
}

void FunctionsTreeWidget::mousePressEvent(QMouseEvent *event)
{
    /* Shift-click on an expand/collapse arrow acts on the whole tree, in the
       direction the clicked arrow would have gone. The arrow is drawn to the
       left of the item's own rect, so a click before visualRect() started is
       a click on the branch indicator rather than on the item - which also
       keeps plain shift-click range selection on item text working. */
    if (event->modifiers() & Qt::ShiftModifier)
    {
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid() && event->pos().x() < visualRect(index).left())
        {
            QTreeWidgetItem *item = itemFromIndex(index);
            if (item != NULL && item->childCount() > 0)
            {
                if (item->isExpanded())
                    collapseAll();
                else
                    expandAll();
                return;
            }
        }
    }

    QTreeWidget::mousePressEvent(event);

    m_draggedItems = selectedItems(); //itemAt(event->pos());
}

bool FunctionsTreeWidget::hasCollapsedItems() const
{
    QTreeWidgetItemIterator it(const_cast<FunctionsTreeWidget*>(this));
    while (*it != NULL)
    {
        QTreeWidgetItem *item = *it;
        if (item->childCount() > 0 && item->isExpanded() == false)
            return true;
        ++it;
    }

    return false;
}

void FunctionsTreeWidget::toggleExpandAll()
{
    if (hasCollapsedItems())
        expandAll();
    else
        collapseAll();
}

void FunctionsTreeWidget::dropEvent(QDropEvent *event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTreeWidgetItem *dropItem = itemAt(event->pos());
#else
    QTreeWidgetItem *dropItem = itemAt(event->position().toPoint());
#endif
    if (m_draggedItems.count() == 0 || dropItem == NULL)
        return;

    // drops are allowed only on folders
    if (dropItem->text(COL_PATH).isEmpty())
        return;

    foreach (QTreeWidgetItem *item, m_draggedItems)
    {
        quint32 dragFID = item->data(COL_NAME, Qt::UserRole).toUInt();
        Function *dragFunc = m_doc->function(dragFID);
        if (dragFunc != NULL)
        {
            QTreeWidget::dropEvent(event);
            dragFunc->setPath(dropItem->text(COL_PATH));
        }
        else
        {
            // m_draggedItem is a folder
            QTreeWidget::dropEvent(event);
            slotItemChanged(item);
        }
    }

    m_draggedItems.clear();
}
