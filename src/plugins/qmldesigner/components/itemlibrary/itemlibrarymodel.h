/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef ITEMLIBRARYMODEL_H
#define ITEMLIBRARYMODEL_H

#include <QMap>
#include <QIcon>
#include <QVariant>
#include <QScriptEngine>
#include <private/qdeclarativelistmodel_p.h>

QT_FORWARD_DECLARE_CLASS(QMimeData);

namespace QmlDesigner {

class ItemLibraryInfo;
class ItemLibraryEntry;

namespace Internal {

template <class T>
class ItemLibrarySortedModel: public QDeclarativeListModel {
public:
    ItemLibrarySortedModel(QObject *parent = 0);
    ~ItemLibrarySortedModel();

    void clearElements();

    void addElement(T *element, int libId);
    void removeElement(int libId);

    bool elementVisible(int libId) const;
    bool setElementVisible(int libId, bool visible);

    const QMap<int, T *> &elements() const;

    T *elementModel(int libId);
    int findElement(int libId) const;
    int visibleElementPosition(int libId) const;

private:
    struct order_struct {
        int libId;
        bool visible;
    };

    QMap<int, T *> m_elementModels;
    QList<struct order_struct> m_elementOrder;
};


class ItemLibraryItemModel: public QScriptValue {
public:
    ItemLibraryItemModel(QScriptEngine *scriptEngine, int itemLibId, const QString &itemName);
    ~ItemLibraryItemModel();

    int itemLibId() const;
    QString itemName() const;

    void setItemIcon(const QIcon &itemIcon);
    void setItemIconSize(const QSize &itemIconSize);

    bool operator<(const ItemLibraryItemModel &other) const;

private:
    QWeakPointer<QScriptEngine> m_scriptEngine;
    int m_libId;
    QString m_name;
    QIcon m_icon;
    QSize m_iconSize;
};


class ItemLibrarySectionModel: public QScriptValue {
public:
    ItemLibrarySectionModel(QScriptEngine *scriptEngine, int sectionLibId, const QString &sectionName, QObject *parent = 0);

    QString sectionName() const;

    void addSectionEntry(ItemLibraryItemModel *sectionEntry);
    void removeSectionEntry(int itemLibId);

    int visibleItemIndex(int itemLibId);
    bool isItemVisible(int itemLibId);

    bool updateSectionVisibility(const QString &searchText, bool *changed);
    void updateItemIconSize(const QSize &itemIconSize);

    bool operator<(const ItemLibrarySectionModel &other) const;

private:
    QString m_name;
    ItemLibrarySortedModel<ItemLibraryItemModel> m_sectionEntries;
};


class ItemLibraryModel: public ItemLibrarySortedModel<ItemLibrarySectionModel> {
    Q_OBJECT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)

public:
    ItemLibraryModel(QScriptEngine *scriptEngine, QObject *parent = 0);
    ~ItemLibraryModel();

    QString searchText() const;

    void update(ItemLibraryInfo *itemLibraryInfo);

    QString getTypeName(int libId);
    QMimeData *getMimeData(int libId);
    QIcon getIcon(int libId);

public slots:
    void setSearchText(const QString &searchText);
    void setItemIconSize(const QSize &itemIconSize);

    int getItemSectionIndex(int itemLibId);
    int getSectionLibId(int itemLibId);
    bool isItemVisible(int itemLibId);

signals:
    void qmlModelChanged();
    void searchTextChanged();
    void visibilityChanged();
    void sectionVisibilityChanged(int changedSectionLibId);

private:
    void updateVisibility();

    int getWidth(const ItemLibraryEntry &entry);
    int getHeight(const ItemLibraryEntry &entry);
    QPixmap createDragPixmap(int width, int height);

    QWeakPointer<QScriptEngine> m_scriptEngine;
    QMap<int, ItemLibraryEntry> m_itemInfos;
    QMap<int, int> m_sections;

    QString m_searchText;
    QSize m_itemIconSize;
    int m_nextLibId;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // ITEMLIBRARYMODEL_H

