/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "player.h"

#include "IcovDemoSideMenu.h"
#include "histogramwidget.h"
#include "playercontrols.h"
#include "playlistmodel.h"
#include "videowidget.h"

#include <QAudioProbe>
#include <QMediaMetaData>
#include <QMediaPlaylist>
#include <QMediaService>
#include <QVideoProbe>
#include <QtWidgets>

Player::Player(QWidget *parent)
    : QWidget(parent), imageLabel(new QLabel(this)) {
  //! [create-objs]

  QPushButton *openButton = new QPushButton(tr("Open"), this);

  connect(openButton, &QPushButton::clicked, this, &Player::open);

  QBoxLayout *displayLayout = new QHBoxLayout;
  displayLayout->addWidget(imageLabel, 2);
  // displayLayout->addWidget(new IcovDemoSideMenu(this));

  QBoxLayout *controlLayout = new QHBoxLayout;
  controlLayout->setContentsMargins(0, 0, 0, 0);
  controlLayout->addWidget(openButton);
  controlLayout->addStretch(4);

  QBoxLayout *layout = new QVBoxLayout;
  layout->addLayout(displayLayout, 4);
  layout->addLayout(controlLayout);
#if defined(Q_OS_QNX)
  // On QNX, the main window doesn't have a title bar (or any other
  // decorations). Create a status bar for the status information instead.
  m_statusLabel = new QLabel;
  m_statusBar = new QStatusBar;
  m_statusBar->addPermanentWidget(m_statusLabel);
  m_statusBar->setSizeGripEnabled(
      false); // Without mouse grabbing, it doesn't work very well.
  layout->addWidget(m_statusBar);
#endif

  setLayout(layout);
}

Player::~Player() {}

void Player::open() {
  QFileDialog fileDialog(this);
  fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog.setWindowTitle(tr("Open Files"));

  QStringList mimeTypeFilters;

  //  for (const QByteArray &mimeTypeName : supportedMimeTypes)
  //    mimeTypeFilters.append(mimeTypeName);
  //  mimeTypeFilters.sort();
  //  dialog.setMimeTypeFilters(mimeTypeFilters);
  //  dialog.selectMimeTypeFilter("image/jpeg");

  QStringList supportedMimeTypes; // = QImageReader::supportedMimeTypes();
  if (!supportedMimeTypes.isEmpty()) {
    fileDialog.setMimeTypeFilters(supportedMimeTypes);
  }

  fileDialog.setDirectory(
      QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
          .value(0, QDir::homePath()));
  if (fileDialog.exec() == QDialog::Accepted) {
    loadFile(fileDialog.selectedFiles().first());
  }
  // addToPlaylist(fileDialog.selectedUrls());
}

bool Player::loadFile(const QString &fileName) {
  QImageReader reader(fileName);
  reader.setAutoTransform(true);
  const QImage newImage = reader.read();
  if (newImage.isNull()) {
    QMessageBox::information(
        this, QGuiApplication::applicationDisplayName(),
        tr("Cannot load %1: %2")
            .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
    return false;
  }
  //! [2]

  setImage(newImage);

  setWindowFilePath(fileName);

  const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
                              .arg(QDir::toNativeSeparators(fileName))
                              .arg(image.width())
                              .arg(image.height())
                              .arg(image.depth());
  // statusBar()->showMessage(message);

  // create a scene and add it your view
  //  QGraphicsScene* scene = new QGraphicsScene;
  //  ui->view->setScene(scene);

  //  // Add the vertical lines first, paint them red
  //  for (int x=0; x<=500; x+=50)
  //    scene->addLine(x,0,x,500, QPen(Qt::red));

  //  // Now add the horizontal lines, paint them green
  //  for (int y=0; y<=500; y+=50)
  //    scene->addLine(0,y,500,y, QPen(Qt::green));

  //  // Fit the view in the scene's bounding rect
  //  ui->view->fitInView(scene->itemsVBoundingRect());
  return true;
}

void Player::setImage(const QImage &newImage) {
  image = newImage;
  if (image.colorSpace().isValid())
    image.convertToColorSpace(QColorSpace::SRgb);
  imageLabel->setPixmap(QPixmap::fromImage(image));
  imageLabel->adjustSize();
}
