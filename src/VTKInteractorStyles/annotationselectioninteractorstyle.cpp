#include "annotationselectioninteractorstyle.hpp"
#include <vtkCellArray.h>
#include <annotationselectiondialog.hpp>
#include <vtkRenderer.h>
#include <drawableannotation.hpp>

using namespace std;
using namespace SemantisedTriangleMesh;
AnnotationSelectionInteractorStyle::AnnotationSelectionInteractorStyle()
{
    this->cellPicker = vtkSmartPointer<vtkCellPicker>::New();
}

void AnnotationSelectionInteractorStyle::OnRightButtonDown()
{
}

void AnnotationSelectionInteractorStyle::OnMouseMove()
{
    vtkInteractorStyleRubberBandPick::OnMouseMove();
}

void AnnotationSelectionInteractorStyle::OnLeftButtonDown()
{
    if(this->Interactor->GetControlKey()){

        this->cellPicker->AddPickList(mesh->getSurfaceActor());
        this->cellPicker->PickFromListOn();
        //The click position of the mouse is taken
        int x, y;
        x = this->Interactor->GetEventPosition()[0];
        y = this->Interactor->GetEventPosition()[1];
        this->FindPokedRenderer(x, y);
        //Some tolerance is set for the picking
        this->cellPicker->Pick(x, y, 0, ren);
        vtkIdType cellID = this->cellPicker->GetCellId();

        //If some point has been picked...
        if(cellID > 0 && cellID < this->mesh->getTrianglesNumber()){
            auto t = mesh->getTriangle(static_cast<unsigned long>(cellID));
            double wc[3];
            double bestDistance = DBL_MAX;
            this->cellPicker->GetPickPosition(wc);
            auto p = new Point(wc[0], wc[1], wc[2]);
            auto v_ = t->getV1();
            std::shared_ptr<Vertex> v;
            auto annotations = mesh->getAnnotations();
            for(uint i = 0; i < mesh->getVerticesNumber(); i++)
                mesh->getVertex(i)->setInfo(nullptr);

            for(unsigned int i = 0; i < 3; i++){
                double actualDistance = ((*v_) - (*p)).norm();
                if(actualDistance < bestDistance){
                    bestDistance = actualDistance;
                    v = v_;
                }
                v_ = t->getNextVertex(v_);
            }

            vector<std::shared_ptr<DrawableAnnotation> > selected;
            for(unsigned int i = 0; i < annotations.size(); i++)
                if(annotations[i]->isPointInAnnotation(v))
                    selected.push_back(dynamic_pointer_cast<DrawableAnnotation>(annotations[i]));

            if(selected.size() != 0){
                std::shared_ptr<DrawableAnnotation> selectedAnnotation;
                if(selected.size() > 1){
                    AnnotationSelectionDialog* dialog = new AnnotationSelectionDialog();
                    dialog->setAnnotationsList(selected);
                    dialog->exec();
                    selectedAnnotation = dialog->getSelectedAnnotation();
                }else
                    selectedAnnotation = selected[0];
                auto ait = std::find(annotations.begin(), annotations.end(), selectedAnnotation);

                if(ait != annotations.end()){
                    auto annotation = dynamic_pointer_cast<DrawableAnnotation>(*ait);
                    annotation->setSelected(!annotation->getSelected());
                    auto selectedMeshAnnotation = mesh->getAnnotations()[ait - annotations.begin()];
                    auto sait = std::find(selectedAnnotations.begin(), selectedAnnotations.end(), selectedMeshAnnotation);
                    if(sait == selectedAnnotations.end())
                        selectedAnnotations.push_back(selectedMeshAnnotation);
                    else
                        selectedAnnotations.erase(sait);
                }

                modifySelectedAnnotations();
            }

        }
    }else
        vtkInteractorStyleRubberBandPick::OnLeftButtonDown();
}

void AnnotationSelectionInteractorStyle::OnLeftButtonUp()
{
    vtkInteractorStyleRubberBandPick::OnLeftButtonUp();
}

void AnnotationSelectionInteractorStyle::resetSelection()
{
    for(unsigned int i = 0; i < mesh->getAnnotations().size(); i++)
        dynamic_pointer_cast<DrawableAnnotation>(mesh->getAnnotations()[i])->setSelected(false);
}

void AnnotationSelectionInteractorStyle::modifySelectedAnnotations()
{
    mesh->draw(assembly);
    assembly->Modified();
    qvtkWidget->update();
}

vtkSmartPointer<vtkPropAssembly> AnnotationSelectionInteractorStyle::getAssembly() const
{
    return assembly;
}

void AnnotationSelectionInteractorStyle::setAssembly(const vtkSmartPointer<vtkPropAssembly> &value)
{
    assembly = value;
}

vtkSmartPointer<vtkRenderer> AnnotationSelectionInteractorStyle::getRen() const
{
    return ren;
}

void AnnotationSelectionInteractorStyle::setRen(const vtkSmartPointer<vtkRenderer> &value)
{
    ren = value;
}


double AnnotationSelectionInteractorStyle::getTolerance() const
{
    return tolerance;
}

QVTKOpenGLNativeWidget *AnnotationSelectionInteractorStyle::getQvtkWidget() const
{
    return qvtkWidget;
}

void AnnotationSelectionInteractorStyle::setQvtkWidget(QVTKOpenGLNativeWidget *value)
{
    qvtkWidget = value;
}


vtkSmartPointer<vtkCellPicker> AnnotationSelectionInteractorStyle::getCellPicker() const
{
    return cellPicker;
}

void AnnotationSelectionInteractorStyle::setCellPicker(const vtkSmartPointer<vtkCellPicker> &value)
{
    cellPicker = value;
}


const std::vector<std::shared_ptr<SemantisedTriangleMesh::Annotation> > &AnnotationSelectionInteractorStyle::getSelectedAnnotations() const
{
    return selectedAnnotations;
}

const std::shared_ptr<DrawableTriangleMesh> &AnnotationSelectionInteractorStyle::getMesh() const
{
    return mesh;
}

void AnnotationSelectionInteractorStyle::setMesh(const std::shared_ptr<DrawableTriangleMesh> &newMesh)
{
    mesh = newMesh;
}
