#ifndef ANNOTATIONCONSTRAINTDIALOG_H
#define ANNOTATIONCONSTRAINTDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <string>
#include <annotation.hpp>

namespace Ui {
    class AnnotationsRelationshipDialog;
}

class AnnotationsRelationshipDialog : public QDialog{
    Q_OBJECT

    public:
        AnnotationsRelationshipDialog (QWidget* parent);
        ~AnnotationsRelationshipDialog ();
        std::vector<std::shared_ptr<SemantisedTriangleMesh::Annotation> > getSubjects() const;
        void setSubjects(const std::vector<std::shared_ptr<SemantisedTriangleMesh::Annotation> > &value);

        std::string getTypeString() const;

    signals:
        void addSemanticConstraint(std::string, double, double, double, unsigned int, unsigned int, bool);
        void addSemanticRelationship(std::string, double, double, double, unsigned int, unsigned int, bool);

    private slots:
        void slotType(QString);
        void slotSave();


    private:
        Ui::AnnotationsRelationshipDialog *ui;
        std::string type;
        std::vector<std::shared_ptr<SemantisedTriangleMesh::Annotation> > subjects;
        bool directed;
};

#endif // ANNOTATIONCONSTRAINTDIALOG_H
