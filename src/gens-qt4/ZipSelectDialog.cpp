/***************************************************************************
 * gens-qt4: Gens Qt4 UI.                                                  *
 * ZipSelectDialog.cpp: Multi-File Archive Selection Dialog.               *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                      *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                             *
 * Copyright (c) 2008-2010 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "ZipSelectDialog.hpp"

// Qt includes.
#include <QtGui/QPushButton>

// Text translation macro.
#define TR(text) \
	QCoreApplication::translate("ZipSelectDialog", (text), NULL, QCoreApplication::UnicodeUTF8)

namespace GensQt4
{

/**
 * ZipSelectDialog(): Initialize the Multi-File Archive Selection Dialog.
 */
ZipSelectDialog::ZipSelectDialog(QWidget *parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setupUi(this);
	
	// Disable the "OK" button initially.
	QPushButton *button = buttonBox->button(QDialogButtonBox::Ok);
	if (button)
		button->setEnabled(false);
	
	m_dirModel = new GensZipDirModel(this);
	treeView->setModel(m_dirModel);
}


/**
 * ~ZipSelectDialog(): Shut down the Multi-File Archive Selection Dialog.
 */
ZipSelectDialog::~ZipSelectDialog()
{
}


/**
 * setFileList(): Set the file list.
 * @param z_entry File list.
 */
void ZipSelectDialog::setFileList(const mdp_z_entry_t* z_entry)
{
	// TODO: Figure out Model/View Controller and implement
	// an mdp_z_entry_t model.
	
	// Clear the tree model first.
	m_dirModel->clear();
	
	// Save the list pointer and clear the selected file pointer.
	m_z_entry_list = z_entry;
	m_z_entry_sel = NULL;
	
	// TODO: Hierarchical file view.
	// For now, let's just do a standard list view.
	const mdp_z_entry_t *cur = m_z_entry_list;
	for (; cur != NULL; cur = cur->next)
	{
		if (!cur->filename)
		{
			// No filename. Go to the next file.
			continue;
		}
		
		QString filename = QString::fromUtf8(cur->filename);
		
		// TODO: Set icon based on file extension.
		QIcon icon = this->style()->standardIcon(QStyle::SP_FileIcon);
		m_dirModel->insertZEntry(cur, icon);
	}
}


/**
 * accept(): User accepted the selection.
 */
void ZipSelectDialog::accept(void)
{
	// Get the selected item.
	QModelIndexList indexList = treeView->selectionModel()->selectedIndexes();
	if (indexList.size() != 1)
		return;
	
	// Get the selected z_entry.
	m_z_entry_sel = m_dirModel->getZEntry(indexList[0]);
	
	// Call the base accept() function.
	this->QDialog::accept();
}


/**
 * on_treeView_clicked(): An item in the QTreeView was clicked.
 * @param index Item index.
 */
void ZipSelectDialog::on_treeView_clicked(const QModelIndex& index)
{
	// Enable the "OK" button.
	// TODO: Disable the "OK" button if the item is a directory.
	QPushButton *button = buttonBox->button(QDialogButtonBox::Ok);
	if (button)
		button->setEnabled(true);
}

}
