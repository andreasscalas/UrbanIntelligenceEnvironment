#ifndef ANNOTATIONSELECTIONDIALOG_H
#define ANNOTATIONSELECTIONDIALOG_H

#include <QDialog>
#include <drawableannotation.hpp>
#include <vector>
namespace Ui {
class AnnotationSelectionDialog;
}

class AnnotationSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnnotationSelectionDialog(QWidget *parent = nullptr);
    ~AnnotationSelectionDialog();

    const std::vector<std::shared_ptr<DrawableAnnotation> > &getAnnotationsList() const;
    void setAnnotationsList(const std::vector<std::shared_ptr<DrawableAnnotation> > &newAnnotationsList);

    const std::shared_ptr<DrawableAnnotation> &getSelectedAnnotation() const;

private slots:
    void on_selectAnnotationButton_clicked();

private:
    Ui::AnnotationSelectionDialog *ui;
    std::vector<std::shared_ptr<DrawableAnnotation>> annotationsList;
    std::shared_ptr<DrawableAnnotation> selectedAnnotation;
};

#endif // ANNOTATIONSELECTIONDIALOG_H
