#ifndef ANNOTATIONSELECTIONINTERACTORSTYLE_H
#define ANNOTATIONSELECTIONINTERACTORSTYLE_H


#include <drawabletrianglemesh.hpp>
#include <annotation.hpp>

#include <vtkSmartPointer.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkPropAssembly.h>
#include <vtkCellPicker.h>
#include <QVTKOpenGLNativeWidget.h>


class AnnotationSelectionInteractorStyle: public QObject, public vtkInteractorStyleRubberBandPick
{
Q_OBJECT
public:

    constexpr static double TOLERANCE_RATIO = 2e-1;
    constexpr static double RADIUS_RATIO = 300;
    static AnnotationSelectionInteractorStyle* New();
    AnnotationSelectionInteractorStyle();
    vtkTypeMacro(AnnotationSelectionInteractorStyle, vtkInteractorStyleRubberBandPick)

    void OnRightButtonDown() override;
    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void resetSelection();

    vtkSmartPointer<vtkPropAssembly> getAssembly() const;
    void setAssembly(const vtkSmartPointer<vtkPropAssembly> &value);
    vtkSmartPointer<vtkRenderer> getRen() const;
    void setRen(const vtkSmartPointer<vtkRenderer> &value);
    vtkSmartPointer<vtkCellPicker> getCellPicker() const;
    void setCellPicker(const vtkSmartPointer<vtkCellPicker> &value);
    double getTolerance() const;

    QVTKOpenGLNativeWidget *getQvtkWidget() const;
    void setQvtkWidget(QVTKOpenGLNativeWidget *value);


    const std::vector<std::shared_ptr<SemantisedTriangleMesh::Annotation> > &getSelectedAnnotations() const;

    const std::shared_ptr<Drawables::DrawableTriangleMesh> &getMesh() const;
    void setMesh(const std::shared_ptr<Drawables::DrawableTriangleMesh> &newMesh);


signals:
    void updateView();

protected:

    vtkSmartPointer<vtkRenderer> ren;
    vtkSmartPointer<vtkPropAssembly> assembly;          //Assembly of actors
    vtkSmartPointer<vtkCellPicker> cellPicker;
    std::vector<std::shared_ptr<SemantisedTriangleMesh::Annotation> > selectedAnnotations;

    QVTKOpenGLNativeWidget * qvtkWidget;

    std::shared_ptr<Drawables::DrawableTriangleMesh> mesh;
    double tolerance;
    vtkIdType reachedID;
};

#endif // ANNOTATIONSELECTIONINTERACTORSTYLE_H
