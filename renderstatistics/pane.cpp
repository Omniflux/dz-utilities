/*
* Project:   dzUtilities::RenderStatistics
* Github:    https://github.com/tpiekarski/dzUtilities
* Copyright: (c) 2017-2018 Thomas Piekarski <t.piekarski@deloquencia.de>
* License:   Mozilla Public License, v. 2.0
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
*/

#include "constants.h"
#include "dzstatistics.h"
#include "logger.h"
#include "pane.h"
#include "statistics.h"
#include <dz3dviewport.h>
#include <dzapp.h>
#include <dzmainwindow.h>
#include <dzrenderer.h>
#include <dzrendermgr.h>
#include <dzscene.h>
#include <dzstyle.h>
#include <dzstyledefs.h>
#include <dzviewport.h>
#include <dzviewportmgr.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtGui/qboxlayout.h>
#include <QtGui/qimage.h>

RenderStatisticsPane::RenderStatisticsPane() : DzPane("Render Statistics") {
  logger = new RenderStatisticsLogger(true);
  renderManager = dzApp->getRenderMgr();
  connectSignals();
  setupPaneLayout();
  renderingCounter = INITIAL_RENDERING_COUNTER;
}

RenderStatisticsPane::~RenderStatisticsPane() {
  logger->log("Destructing render statistics pane.");
  clear();

  if (logger != nullptr) {
    delete(logger);
    logger = nullptr;
  }
}

void RenderStatisticsPane::redraw() {
  clear();
  statisticsLayout->redraw();
}

void RenderStatisticsPane::clear() {
  if (statistics.empty()) {
    return;
  }

  logger->log(QString("Clearing render statistics of %1 rendering(s).").arg(statistics.size()));

  renderingCounter = INITIAL_RENDERING_COUNTER;
  statistics.clear();
  statistics.shrink_to_fit();
}

void RenderStatisticsPane::connectSignals() {
  if (!connect(renderManager, SIGNAL(renderStarting()), this, SLOT(processStartRendering()))) {
    logger->log(QString("Failed connecting renderStarting signal for %1 to the processStartRendering slot of %2.")
      .arg(renderManager->objectName()).arg(this->objectName()));
  }
  
  if (!connect(renderManager, SIGNAL(renderFinished(bool)), this, SLOT(processFinishRendering(bool)))) {
    logger->log(QString("Failed connecting renderFinished signal for %1 to the processFinishRendering slot of %2.")
      .arg(renderManager->objectName()).arg(this->objectName()));
  }
}

void RenderStatisticsPane::buildOptionsMenu(DzActionMenu* menu) const {
  menu->insertAction("RenderStatisticsClearAction");
}

void RenderStatisticsPane::processStartRendering() {
  logger->log("Rendering started.");

  DzRenderer* renderer = renderManager->getActiveRenderer();
  statistics.emplace_back(
    DzRenderStatistics(
      renderer->getName(), 
      dzScene->getNumNodes(),
      dzApp->getInterface()->getViewportMgr()->getActiveViewport()->get3DViewport()->getCamera(),
      logger
    )
  );
}

void RenderStatisticsPane::processFinishRendering(bool succeeded) {
  logger->log("Rendering finished.");

  DzRenderStatistics* currentStatistics = &statistics.back();
  currentStatistics->stopClock();
  currentStatistics->setCounter(++renderingCounter);
  currentStatistics->setRenderImage(saveLastRenderImage(renderingCounter).toStdString());
  currentStatistics->setSucceeded(succeeded);

  logger->log(currentStatistics);
  statisticsLayout->addDataRow();
}

void RenderStatisticsPane::setupPaneLayout() {
  const int margin = style()->pixelMetric(DZ_PM_GeneralMargin);

  paneLayout = new QVBoxLayout(this);
  paneLayout->setMargin(margin);
  paneLayout->setSpacing(margin);

  statisticsLayout = new QStatisticsLayout(&statistics, logger);
  paneLayout->addLayout(statisticsLayout);
  paneLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

  setLayout(paneLayout);
}

QString RenderStatisticsPane::saveLastRenderImage(const int renderingCounter) {
  const QString filename = QString(RENDER_FILE_NAME_TEMPLATE).arg(QString::number(renderingCounter));
  const QString filePath = QString("%1/%2").arg(dzApp->getTempPath(), filename);
  const QImage lastRenderImage(renderManager->getLastSavedRenderPath());

  if (! lastRenderImage.save(filePath, 0, -1)) {
    logger->log(QString("Failed storing render image at %1.").arg(filePath));

    return NOTHING;
  }

  return filename;
}
