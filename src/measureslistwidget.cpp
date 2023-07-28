#include "measureslistwidget.hpp"
#include "ui_measureslistwidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <drawableannotation.hpp>

#include <categorybutton.hpp>
#include <attributewidget.hpp>

using namespace  SemantisedTriangleMesh;
using namespace Drawables;

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
        auto cb = new CategoryButton("Annotation "+QString::number(std::stoi(annotation->getId())), this, pCategory);
        this->setItemWidget(pCategory, 0, cb);
        connect(cb, SIGNAL(pressed(bool)), this, SLOT(slotSelectAnnotation(bool)));
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

        auto showButton = new QPushButton("Show annotation");
        showButton->setCheckable(true);
        showButton->setChecked(std::dynamic_pointer_cast<DrawableAnnotation>(annotation)->getDrawAnnotation());
        pLayout->addWidget(showButton);
        buttonAnnotationMap.insert(std::make_pair(showButton, annotation));
        connect(showButton, SIGNAL(clicked(bool)), this, SLOT(slotShowAnnotation(bool)));
        auto deleteButton = new QPushButton("Delete annotation");
        pLayout->addWidget(deleteButton);
        buttonAnnotationMap.insert(std::make_pair(deleteButton, annotation));
        connect(deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteAnnotation()));
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

void MeasuresListWidget::slotShowAnnotation(bool checked)
{
    auto annotation = buttonAnnotationMap.at(static_cast<QPushButton*>(sender()));
    std::dynamic_pointer_cast<DrawableAnnotation>(annotation)->setDrawAnnotation(checked);
    emit(updateViewSignal());
}

void MeasuresListWidget::slotDeleteAnnotation()
{
    auto annotation = buttonAnnotationMap.at(static_cast<QPushButton*>(sender()));
    mesh->removeAnnotation(annotation->getId());
    emit(updateViewSignal());
    update();

}

void MeasuresListWidget::slotSelectAnnotation(bool selected)
{
    auto button = static_cast<CategoryButton*>(sender());
    auto text = button->text().toStdString();
    std::string id = text.substr(11, text.size());
    emit(selectAnnotation(id, selected));
}

void MeasuresListWidget::updateSlot()
{
    emit updateSignal();
}
