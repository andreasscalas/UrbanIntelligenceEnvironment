#include "attributewidget.hpp"
#include "ui_attributewidget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <categorybutton.hpp>
#include <semanticattribute.hpp>

using namespace SemantisedTriangleMesh;

AttributeWidget::AttributeWidget(QWidget *parent):
    QTreeWidget(parent),
    ui(new Ui::AttributeWidget)
{

    ui->setupUi(this);
    this->setRootIsDecorated(false);
    this->setIndentation(0);

    this->setHeaderHidden(true);
}

AttributeWidget::~AttributeWidget()
{
    delete ui;
}

void AttributeWidget::update()
{
    this->clear();

    for(unsigned int i = 0; i < annotation->getAttributes().size(); i++)
    {
        auto attribute = annotation->getAttributes()[i];
        QTreeWidgetItem* pCategory = new QTreeWidgetItem();
        this->addTopLevelItem(pCategory);
        this->setItemWidget(pCategory, 0,
            new CategoryButton("Attribute "+QString::number(attribute->getId()), this, pCategory));
        QFrame* pFrame = new QFrame(this);
        QBoxLayout* pLayout = new QVBoxLayout(pFrame);
        pLayout->addWidget(new QLabel("id: "+QString::number(attribute->getId())));
        pLayout->addWidget(new QLabel("tag: "+QString::fromStdString(attribute->getKey())));
        if(std::dynamic_pointer_cast<SemanticAttribute>(attribute) == nullptr)
            pLayout->addWidget(new QLabel("measure: "+QString::number(*static_cast<double*>(attribute->getValue()))));
        else
            pLayout->addWidget(new QLabel("measure: "+QString::fromStdString(*static_cast<std::string*>(attribute->getValue()))));
        QPushButton* deletionButton =  new QPushButton("Remove attribute");
        buttonMeasureMap.insert(std::make_pair(deletionButton, attribute));
        this->connect(deletionButton, SIGNAL(clicked()), this, SLOT(deletionButtonClickedSlot()));
        pLayout->addWidget(deletionButton);
        QPushButton* measureButton =  new QPushButton("Show measure");
        measureButton->setCheckable(true);
        measureButton->setChecked(true);
        buttonMeasureMap.insert(std::make_pair(measureButton, attribute));
        this->connect(measureButton, SIGNAL(clicked()), this, SLOT(measureButtonClickedSlot()));
        pLayout->addWidget(measureButton);

        QTreeWidgetItem* pContainer = new QTreeWidgetItem();
        pContainer->setDisabled(true);
        pCategory->addChild(pContainer);
        this->setItemWidget(pContainer, 0, pFrame);
    }
}

std::shared_ptr<Annotation> AttributeWidget::getAnnotation() const
{
    return annotation;
}

void AttributeWidget::setAnnotation(std::shared_ptr<Annotation> value)
{
    annotation = value;
}

void AttributeWidget::measureButtonClickedSlot()
{
    QPushButton* button = qobject_cast<QPushButton *>(sender());
    auto attr = std::dynamic_pointer_cast<DrawableAttribute>(buttonMeasureMap[button]);
    if(attr != nullptr)
    {
        attr->setDrawAttribute(button->isChecked());
        emit updateViewSignal();
    }
}

void AttributeWidget::deletionButtonClickedSlot()
{

    QPushButton* button = qobject_cast<QPushButton *>(sender());
    auto drawable = std::dynamic_pointer_cast<DrawableAttribute>(buttonMeasureMap[button]);
    annotation->removeAttribute(drawable);
    emit updateSignal();
    update();
}
