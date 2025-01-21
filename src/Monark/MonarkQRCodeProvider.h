#pragma once
#include <QQuickImageProvider>

#include "QGCToolbox.h"
#include "qrcodegen.h"
class MonarkQRCodeProvider : public QGCTool, public QQuickImageProvider
{
public:
    MonarkQRCodeProvider(QGCApplication* p_app, QGCToolbox* p_toolbox);
    ~MonarkQRCodeProvider       ();
    //void    generateQRCode        (QString const& encodeThis, uint16_t const borderSize=1, qrcodegen::QrCode::Ecc errorCorrection = qrcodegen::QrCode::Ecc::MEDIUM);
    // Overrdies from QQuickImageProvider
    QImage  requestImage    (QString const& id, QSize* p_size,  QSize const & requestedSize) override;
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize& requestedSize) override;
    // Overrides from QGCTool
    void    setToolbox      (QGCToolbox *p_toolbox) override;
private:

    static QString _createSvg(qrcodegen::QrCode const& qrCode, uint16_t const borderSize);

};


