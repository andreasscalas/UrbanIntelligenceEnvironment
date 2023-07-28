#include "annotationselectiondialog.hpp"
#include "ui_annotationselectiondialog.h"
#include <qstringlistmodel.h>

using namespace Drawables;
AnnotationSelectionDialog::AnnotationSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnnotationSelectionDialog)
{
    this->setWindowTitle("Select annotation");
    ui->setupUi(this);
    selectedAnnotation = nullptr;
}

AnnotationSelectionDialog::~AnnotationSelectionDialog()
{
    delete ui;
}


void AnnotationSelectionDialog::on_selectAnnotationButton_clicked()
{
    QModelIndex index = this->ui->annotationsListView->currentIndex();
    QString itemText = index.data(Qt::DisplayRole).toString();

    for (unsigned int i = 0; i < annotationsList.size(); i++)
        if(annotationsList[i]->getTag().compare(itemText.toStdString()) == 0)
            selectedAnnotation = annotationsList[i];

    this->close();
}

const std::vector<std::shared_ptr<DrawableAnnotation> > &AnnotationSelectionDialog::getAnnotationsList() const
{
    return annotationsList;
}

void AnnotationSelectionDialog::setAnnotationsList(const std::vector<std::shared_ptr<DrawableAnnotation> > &newAnnotationsList)
{
    QStringListModel *model = new QStringListModel(this);
    QStringList stringList;
    this->annotationsList = newAnnotationsList;
    for (unsigned int i = 0; i < annotationsList.size(); i++)
        stringList.append(QString::fromStdString(annotationsList[i]->getTag()));

    model->setStringList(stringList);
    this->ui->annotationsListView->setModel(model);
    ui->annotationsListView->setSelectionMode(QAbstractItemView::SingleSelection);
}

const std::shared_ptr<DrawableAnnotation> &AnnotationSelectionDialog::getSelectedAnnotation() const
{
    return selectedAnnotation;
}
