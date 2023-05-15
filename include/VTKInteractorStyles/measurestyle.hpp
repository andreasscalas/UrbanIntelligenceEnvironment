#ifndef MEASURESTYLE_H
#define MEASURESTYLE_H

#include <drawableattribute.hpp>
#include <drawabletrianglemesh.hpp>

#include <vtkInteractorStyleTrackballCamera.h>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkCellPicker.h>
#include <vtkPropAssembly.h>

class MeasureStyle : public vtkInteractorStyleTrackballCamera
{
public:
    enum class MeasureType {RULER, TAPE, CALIBER, BOUNDING, HEIGHT};
    const double epsilon = 1e-7;

    static MeasureStyle* New();
    MeasureStyle();
    ~MeasureStyle() override;
    vtkTypeMacro(MeasureStyle,vtkInteractorStyleTrackballCamera)

    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnRightButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMouseWheelBackward() override;
    void OnMouseWheelForward() override;

    void manageRulerMovement(std::shared_ptr<SemantisedTriangleMesh::Vertex> start, std::shared_ptr<SemantisedTriangleMesh::Vertex> end);
    void manageTapeMovement(std::shared_ptr<SemantisedTriangleMesh::Vertex> start, std::shared_ptr<SemantisedTriangleMesh::Vertex> end);
    void manageHeightMovement(std::shared_ptr<SemantisedTriangleMesh::Vertex> start);
    void manageCaliberMovement();
    void manageBoundingMovement();

    void reset();

    std::shared_ptr<DrawableAttribute>  finalizeAttribute(unsigned int id, std::string key);
    //Getters and setters
    std::shared_ptr<DrawableTriangleMesh> getMesh() const;
    void setMesh(std::shared_ptr<DrawableTriangleMesh> value);
    QVTKOpenGLNativeWidget *getQvtkwidget() const;
    void setQvtkwidget(QVTKOpenGLNativeWidget *value);
    vtkSmartPointer<vtkRenderer> getMeshRenderer() const;
    void setMeshRenderer(const vtkSmartPointer<vtkRenderer> &value);
    MeasureType getMeasureType() const;
    void setMeasureType(const MeasureType &value);
    double getMeasure() const;
    std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex>> getMeasurePath() const;
    std::shared_ptr<SemantisedTriangleMesh::Point> getBoundingDirection() const;
    std::shared_ptr<DrawableAttribute> getOnCreationAttribute() const;

    bool getDrawAttributes() const;
    void setDrawAttributes(bool value);

    void updateView();
protected:
    std::shared_ptr<DrawableTriangleMesh> mesh;
    QVTKOpenGLNativeWidget* qvtkwidget;
    vtkSmartPointer<vtkCellPicker> cellPicker;
    vtkSmartPointer<vtkRenderer> meshRenderer;
    vtkSmartPointer<vtkPropAssembly> measureAssembly;
    std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex> > measurePath;
    MeasureType measureType;
    std::shared_ptr<SemantisedTriangleMesh::Vertex> last;
    std::shared_ptr<SemantisedTriangleMesh::Point> boundingBegin, boundingEnd, boundingOrigin;
    bool measureStarted, leftPressed, middlePressed, drawAttributes;
    double measure;
    std::shared_ptr<DrawableAttribute> onCreationAttribute;

};

#endif // MEASURESTYLE_H
