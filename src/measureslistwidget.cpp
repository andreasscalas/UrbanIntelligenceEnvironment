#include "measureslistwidget.hpp"
#include "ui_measureslistwidget.h"

#include <QVBoxLayout>
#include <QLabel>

#include <categorybutton.hpp>
#include <attributewidget.hpp>

using namespace  SemantisedTriangleMesh;

MeasuresListWidget::MeasuresListWidget(QWidget *parent) :
    QTreeWidget(parent),
    ui(new Ui::MeasuresListWidget)
{
    ui->setupUi(this);

    this->setRootIsDecorated(false);
    this->setIndentation(0);

    this->setHeaderHidden(true);
}

MeasuresListWidget::~MeasuresListWidget()
{
    delete ui;
}

void MeasuresListWidget::update()
{
    this->clear();

    if(mesh == nullptr)
        return;
    for(unsigned int i = 0; i < mesh->getAnnotations().size(); i++)
    {
        auto annotation = mesh->getAnnotations()[i];
        QTreeWidgetItem* pCategory = new QTreeWidgetItem();
        this->addTopLevelItem(pCategory);
        this->setItemWidget(pCategory, 0,
            new CategoryButton("Annotation "+QString::number(std::stoi(annotation->getId())), this, pCategory));
        QFrame* pFrame = new QFrame(this);
        QBoxLayout* pLayout = new QVBoxLayout(pFrame);
        pLayout->addWidget(new QLabel("id: "+QString::number(std::stoi(annotation->getId()))));
        pLayout->addWidget(new QLabel("tag: "+QString::fromStdString(annotation->getTag())));
        switch(annotation->getType())
        {
            case AnnotationType::Point:
            {
                pLayout->addWidget(new QLabel("type: Point"));
                break;
            }
            case AnnotationType::Line:
            {
                pLayout->addWidget(new QLabel("type: Line"));
                break;
            }
            case AnnotationType::Surface:
            {
                pLayout->addWidget(new QLabel("type: Region"));
                break;
            }
        }

        pLayout->addWidget(new QLabel("attributes list:"));
        AttributeWidget* w = new AttributeWidget(this);
        w->setAnnotation(annotation);
        pLayout->addWidget(w);
        w->update();
        connect(w, SIGNAL(updateSignal()), this, SLOT(updateSlot()));
        connect(w, SIGNAL(updateViewSignal()), this, SLOT(updateViewSlot()));

        QTreeWidgetItem* pContainer = new QTreeWidgetItem();
        pContainer->setDisabled(true);
        pCategory->addChild(pContainer);
        this->setItemWidget(pContainer, 0, pFrame);
    }
}

std::shared_ptr<DrawableTriangleMesh> MeasuresListWidget::getMesh() const
{
    return mesh;
}

void MeasuresListWidget::setMesh(std::shared_ptr<DrawableTriangleMesh>  value)
{
    mesh = value;
}

void MeasuresListWidget::updateViewSlot()
{
    emit updateViewSignal();
}

void MeasuresListWidget::updateSlot()
{
    emit updateSignal();
}
