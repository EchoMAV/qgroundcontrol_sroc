/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GPSUnitsController.h"
#include "QGCGeo.h"

GPSUnitsController::GPSUnitsController(void)
{}

QString GPSUnitsController::convertToMGRS(QGeoCoordinate coordinate)
{
    QString mgrs = convertGeoToMGRS(coordinate);
    return mgrs;
}
