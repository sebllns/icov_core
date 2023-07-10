#include "IcovDemoSideMenu.h"
#include "qboxlayout.h"
#include "qcheckbox.h"

IcovDemoSideMenu::IcovDemoSideMenu(QWidget *parent) : QWidget(parent) {

  QVBoxLayout *vLayout = new QVBoxLayout(this);

  vLayout->addWidget(new QCheckBox(tr("Show Access Blocks"), this));

  setLayout(vLayout);
}

IcovDemoSideMenu::~IcovDemoSideMenu()
{

}
