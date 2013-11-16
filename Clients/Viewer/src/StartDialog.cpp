/** \file
 * \author Max Mertens <max.mail@dameweb.de>
 * \section LICENSE
 * Copyright (c) 2013, Max Mertens.
 * All rights reserved.
 * This file is licensed under the "BSD 3-Clause License".
 * Full license text is under the file "LICENSE" provided with this code. */

#include <StartDialog.hpp>
#include "ui_StartDialog.h"

using namespace SphereSim;

StartDialog::StartDialog(QApplication* a)
{
	selected = false;
	sphCount = 64;
	startDialog = new Ui::StartDialog();
	startDialog->setupUi(this);
	QObject::connect(this, SIGNAL(accepted()), this, SLOT(accepted_()), Qt::AutoConnection);
	QObject::connect(this, SIGNAL(rejected()), this, SLOT(rejected_()), Qt::AutoConnection);
	exec();
}

void StartDialog::accepted_()
{
	sphCount = startDialog->sphCount->value();
	selected = true;
}

void StartDialog::rejected_()
{
	sphCount = 0;
	selected = true;
}

quint16 StartDialog::getSphereCount()
{
	while(!selected);
	return sphCount;
}