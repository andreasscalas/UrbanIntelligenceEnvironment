#ifndef TRIANGLESELECTIONSTYLE_H
#define TRIANGLESELECTIONSTYLE_H

#include <drawabletrianglemesh.hpp>
#include <drawablesurfaceannotation.hpp>

#include <vector>
#include <map>
#include <string>

#include <vtkSmartPointer.h>
#include <vtkCellPicker.h>
#include <vtkActor.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkSpline.h>
#include <vtkRenderer.h>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkPolyData.h>
#define VTKISRBP_ORIENT 0
#define VTKISRBP_SELECT 1

/**
 * @brief The TriangleSelectionStyle class controls the interaction with the Triangles of a mesh
 */
class TriangleSelectionStyle : public QObject, public vtkInteractorStyleRubberBandPick{

Q_OBJECT
public:

    enum class SelectionType{
        RECTANGLE_AREA,
        LASSO_AREA,
        PAINTED_LINE
    };
    constexpr static double TOLERANCE = 5e-2;
    constexpr static double RADIUS_RATIO = 100;

    static TriangleSelectionStyle* New();
    TriangleSelectionStyle();
    vtkTypeMacro(TriangleSelectionStyle,vtkInteractorStyleRubberBandPick)

    void OnRightButtonDown() override;
    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void SetTriangles(vtkSmartPointer<vtkPolyData> triangles);
    void resetSelection();
    void defineSelection(std::vector<unsigned long> selected);
    void finalizeAnnotation(std::string id, std::string tag, unsigned char color[]);

    std::shared_ptr<SemantisedTriangleMesh::Annotation> getAnnotation() const;
    bool getShowSelectedTriangles() const;
    void setShowSelectedTriangles(bool value);
    bool getVisibleTrianglesOnly() const;
    void setVisibleTrianglesOnly(bool value);
    bool getSelectionMode() const;
    void setSelectionMode(bool value);
    vtkSmartPointer<vtkRenderer> getRen() const;
    void setRen(const vtkSmartPointer<vtkRenderer> &value);
    vtkSmartPointer<vtkPropAssembly> getAssembly() const;
    void setAssembly(const vtkSmartPointer<vtkPropAssembly> &value);

    QVTKOpenGLNativeWidget *getQvtkWidget() const;
    void setQvtkWidget(QVTKOpenGLNativeWidget *value);

    const std::shared_ptr<SemantisedTriangleMesh::Vertex> &getInnerVertex() const;
    void setInnerVertex(const std::shared_ptr<SemantisedTriangleMesh::Vertex> &newInnerVertex);

    const std::shared_ptr<SemantisedTriangleMesh::Vertex> &getLastVertex() const;
    void setLastVertex(const std::shared_ptr<SemantisedTriangleMesh::Vertex> &newLastVertex);

    const std::shared_ptr<DrawableTriangleMesh> &getMesh() const;
    void setMesh(const std::shared_ptr<DrawableTriangleMesh> &newMesh);

    const std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex> > &getPolygonContour() const;
    void setPolygonContour(const std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex> > &newPolygonContour);

    SelectionType getSelectionType() const;
    void setSelectionType(SelectionType newSelectionType);

signals:
    void updateView();
private:
        vtkSmartPointer<vtkPropAssembly> assembly;          //Assembly of actors
        vtkSmartPointer<vtkPropAssembly> sphereAssembly;    //Assembly of spheres
        vtkSmartPointer<vtkPolyData> Triangles;
        vtkSmartPointer<vtkActor> SplineActor;
        vtkSmartPointer<vtkCellPicker> cellPicker;      //The cell picker
        vtkSmartPointer<vtkPoints> splinePoints;
        vtkSmartPointer<vtkSpline> spline;
        vtkSmartPointer<vtkRenderer> ren;
        QVTKOpenGLNativeWidget* qvtkWidget;
        double sphereRadius;
        bool selectionMode;
        bool visibleTrianglesOnly;
        bool showSelectedTriangles;
        bool alreadyStarted;
        bool lasso_started;
        SelectionType selectionType;
        std::shared_ptr<SemantisedTriangleMesh::Annotation> annotation;
        std::shared_ptr<DrawableTriangleMesh> mesh;
        std::shared_ptr<SemantisedTriangleMesh::Vertex> firstVertex;
        std::shared_ptr<SemantisedTriangleMesh::Vertex> lastVertex;
        std::shared_ptr<SemantisedTriangleMesh::Vertex> innerVertex;
        std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex>> polygonContour;

};

#endif // TRIANGLESELECTIONSTYLE_H
