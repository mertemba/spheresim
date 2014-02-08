/** \file
 * \author Max Mertens <max.mail@dameweb.de>
 * \section LICENSE
 * Copyright (c) 2014, Max Mertens.
 * All rights reserved.
 * This file is licensed under the "BSD 3-Clause License".
 * Full license text is under the file "LICENSE" provided with this code. */

#ifndef _STARTDIALOG_HPP_
#define _STARTDIALOG_HPP_

#include <QDialog>
#include <QtGlobal>

namespace Ui
{
	class StartDialog;
}

namespace SphereSim
{

	class StartDialog : public QDialog
	{
		Q_OBJECT
		
	protected:
		Ui::StartDialog* startDialog;
		
		bool selected;
		
		quint16 sphCount;
		
	public:
		StartDialog(QApplication* a);
		
		quint16 getSphereCount();
		
	public slots:
		void accepted_();
		void rejected_();
	};

}

#endif
