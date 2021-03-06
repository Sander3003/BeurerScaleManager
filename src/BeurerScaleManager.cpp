/*!
 * \file BeurerScaleManager.cpp
 * \author Danilo Treffiletti <urban82@gmail.com>
 * \date 2014-07-02
 * \brief Implementation for the BeurerScaleManager class
 * \copyright 2014 (c) Danilo Treffiletti
 *
 *    This file is part of BeurerScaleManager.
 *
 *    BeurerScaleManager is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    BeurerScaleManager is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with BeurerScaleManager.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "BeurerScaleManager.hpp"
#include "ui_BeurerScaleManager.h"

#include <Usb/UsbDownloader.hpp>
#include <Usb/UsbData.hpp>
#include <Data/Models/UserDataModel.hpp>
#include <Data/Models/UserMeasurementModel.hpp>

#include <QtCore/QDebug>
#include <QtGui/QMessageBox>

namespace BSM {

BeurerScaleManager::BeurerScaleManager(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , usb(new Usb::UsbDownloader(this))
    , usb_data(new Usb::UsbData(this))
{
    setWindowTitle("Beurer Scale Manager");

    ui = new Ui::BeurerScaleManager();
    ui->setupUi(this);

    connect(usb, SIGNAL(progress(int)), ui->progressDownload, SLOT(setValue(int)));
    connect(usb, SIGNAL(completed(QByteArray)), this, SLOT(downloadCompleted(QByteArray)));
    connect(usb, SIGNAL(error()), this, SLOT(downloadError()));
}

BeurerScaleManager::~BeurerScaleManager()
{}

void BeurerScaleManager::startDownload()
{
    qDebug() << "START download";
    ui->btnStartDownload->setDisabled(true);
    ui->progressDownload->setValue(0);
    ui->comboUser->setDisabled(true);
    ui->tableMeasurements->setDisabled(true);

    // Clear comboUser
    QAbstractItemModel* oldModel = ui->comboUser->model();
    ui->tableMeasurements->setModel(0);
    delete oldModel;

    // Clear tableMeasurements
    oldModel = ui->tableMeasurements->model();
    ui->tableMeasurements->setModel(0);
    delete oldModel;

    usb->start();
}

void BeurerScaleManager::downloadCompleted(const QByteArray& data)
{
    qDebug() << "END download";
    ui->btnStartDownload->setEnabled(true);

    qDebug() << "Data received:" << data.size() << "bytes";
    qDebug() << data.toHex();

    if (usb_data->parse(data)) {
        qDebug() << "Parsed" << usb_data->getUserData().size() << "users";
        qDebug() << "Scale date and time is" << usb_data->getDateTime();
        qDebug() << *usb_data;

        ui->comboUser->setModel(new Data::Models::UserDataModel(usb_data->getUserData(), usb_data));
        ui->comboUser->setEnabled(true);

        int diffTime = usb_data->getDateTime().secsTo(QDateTime::currentDateTime());
        if (diffTime < -300 || diffTime > 300) {
            QMessageBox::warning(this,
                                 windowTitle() + " - " + tr("Wrong scale settings"),
                                 tr("The date and time set in the scale (%1) are not correct!<br><br>Please check the settings.").arg(usb_data->getDateTime().toString(Qt::SystemLocaleShortDate))
            );
        }
    }
}

void BeurerScaleManager::downloadError()
{
    qDebug() << "ERROR download";
    ui->btnStartDownload->setEnabled(true);

    QMessageBox::critical(this,
                         windowTitle() + " - " + tr("Download error"),
                         tr("No scale found or download error!<br><br>Please check USB cable and try again.")
    );
}

void BeurerScaleManager::selectUser(const int index)
{
    qDebug() << "Selected user at" << index;
    if (index < 0 || index >= usb_data->getUserData().size())
        return;

    Data::UserData* userData = usb_data->getUserData().at(index);
    qDebug() << userData;

    QAbstractItemModel* oldModel = ui->tableMeasurements->model();
    ui->tableMeasurements->setModel(new Data::Models::UserMeasurementModel(userData->getMeasurements(), userData));
    delete oldModel;

    ui->tableMeasurements->setEnabled(true);
    ui->tableMeasurements->selectRow(userData->getMeasurements().size() - 1);
}

} // namespace BSM
