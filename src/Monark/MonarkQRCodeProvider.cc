#include "MonarkQRCodeProvider.h"
#include <QPainter>
#include <QSvgRenderer>
#include "QGCLoggingCategory.h"


Q_DECLARE_LOGGING_CATEGORY(MonarkManagerLog)


MonarkQRCodeProvider::MonarkQRCodeProvider(QGCApplication *p_app, QGCToolbox* p_toolbox)
    : QGCTool               (p_app, p_toolbox)
    , QQuickImageProvider   (QQmlImageProviderBase::Image)
{
}

MonarkQRCodeProvider::~MonarkQRCodeProvider()
{

}

void MonarkQRCodeProvider::setToolbox(QGCToolbox *p_toolbox)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkQRCodeProvider::setToolbox()";
    QGCTool::setToolbox(p_toolbox);
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkQRCodeProvider::setToolbox()";
}

QImage MonarkQRCodeProvider::requestImage(QString const &id, QSize *p_size,  QSize const &requestedSize)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkQRCodeProvider::requestImage("<<id<<")";
    //"image://MONARKQRCodes/networkID,encryptionKey,power,freq,monarkID"
    auto dashIndex=id.indexOf("-");
    QString cleanId;
    if(dashIndex>=0)
    {
        ++dashIndex;
        cleanId = id.mid(dashIndex,id.length()-dashIndex);
    }
    else
    {
        cleanId=id;
    }
    auto const qrCode=qrcodegen::QrCode::encodeText(cleanId.toUtf8().constData(), qrcodegen::QrCode::Ecc::HIGH);
    auto const svg = _createSvg(qrCode,3);
    QSvgRenderer render(svg.toUtf8());
    QImage image(requestedSize,QImage::Format_Mono);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    render.render(&painter);
    if(p_size)
    {
        *p_size=requestedSize;
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkQRCodeProvider::requestImage("<<id<<")";
    return image;
}

QPixmap MonarkQRCodeProvider::requestPixmap(QString const &id, QSize *p_size,  QSize const &requestedSize)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkQRCodeProvider::requestPixmap("<<id<<")";
    auto const pixMap= QPixmap::fromImage(requestImage(id,p_size,requestedSize));
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkQRCodeProvider::requestPixmap("<<id<<")";
    return pixMap;
}

QString MonarkQRCodeProvider::_createSvg(qrcodegen::QrCode const& qrCode, uint16_t const borderSize)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkQRCodeProvider::_createSvg()";
    QString svg;
    QTextStream sb(&svg);
    sb << R"(<?xml version="1.0" encoding="UTF-8"?>)"
       << R"(<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">)"
       << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1" viewBox="0 0 )"
       << (qrCode.getSize() + borderSize * 2) << " " << (qrCode.getSize() + borderSize * 2)
       << R"(" stroke="none"><rect width="100%" height="100%" fill="#FFFFFF"/><path d=")";
    for (int y = 0; y < qrCode.getSize(); y++)
    {
        for (int x = 0; x < qrCode.getSize(); x++)
        {
            if (qrCode.getModule(x, y))
            {
                sb << (x == 0 && y == 0 ? "" : " ") << "M" << (x + borderSize) << "," << (y + borderSize)
                << "h1v1h-1z";
            }
        }
    }
    sb << R"(" fill="#000000"/></svg>)";
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkQRCodeProvider::_createSvg()";
    return svg;
}
