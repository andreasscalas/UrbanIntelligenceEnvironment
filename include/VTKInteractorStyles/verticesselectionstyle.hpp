#ifndef VERTICESSELECTIONSTYLE_H
#define VERTICESSELECTIONSTYLE_H

#include <drawabletrianglemesh.hpp>
#include <drawablepointannotation.hpp>

#include <vector>
#include <map>
#include <string>

#include <vtkSmartPointer.h>
#include <vtkPointPicker.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkPolyData.h>
#include <QVTKOpenGLNativeWidget.h>

#define VTKISRBP_ORIENT 0
#define VTKISRBP_SELECT 1

/**
 * @brief The VerticesSelectionStyle class controls the interaction with the points of a mesh
 */
class VerticesSelectionStyle : public QObject, public vtkInteractorStyleRubberBandPick {
Q_OBJECT
public:

    static VerticesSelectionStyle* New();

    VerticesSelectionStyle();

    vtkTypeMacro(VerticesSelectionStyle, vtkInteractorStyleRubberBandPick)

    virtual void OnRightButtonDown() override;

	virtual void OnLeftButtonDown() override;

	virtual void OnLeftButtonUp() override;

    virtual void OnMouseMove() override;

	void resetSelection();

    void defineSelection(std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex> > selected);

    void draw();

    void finalizeAnnotation(std::string id, std::string tag, unsigned char color[]);

    vtkSmartPointer<vtkPolyData> getPoints() const;
    void setPoints(const vtkSmartPointer<vtkPolyData>& value);
    std::map<unsigned long, bool>* getSelectedPoints() const;
	vtkSmartPointer<vtkPointPicker> getPointPicker() const;
	void setPointPicker(const vtkSmartPointer<vtkPointPicker>& value);
	bool getSelectionMode() const;
	void setSelectionMode(bool value);
	bool getVisiblePointsOnly() const;
    void setVisiblePointsOnly(bool value);
    vtkSmartPointer<vtkPropAssembly> getAssembly() const;
    void setAssembly(const vtkSmartPointer<vtkPropAssembly>& value);
    QVTKOpenGLNativeWidget* getQvtkwidget() const;
    void setQvtkwidget(QVTKOpenGLNativeWidget* value);
    std::map<unsigned long, bool> *getPointsSelectionStatus() const;
    void setPointsSelectionStatus(std::map<unsigned long, bool> *value);

    const std::shared_ptr<Drawables::DrawableTriangleMesh> &getMesh() const;
    void setMesh(const std::shared_ptr<Drawables::DrawableTriangleMesh> &newMesh);


    vtkSmartPointer<vtkRenderer> getRenderer() const;
    void setRenderer(vtkSmartPointer<vtkRenderer> newRen);

signals:
    void updateView();

private:
    constexpr static double RADIUS_RATIO = 1000;

    vtkSmartPointer<vtkPropAssembly> assembly;          //Assembly of actors
    vtkSmartPointer<vtkPropAssembly> sphereAssembly;    //Assembly of spheres
    vtkSmartPointer<vtkPolyData> points;
    vtkSmartPointer<vtkPointPicker> pointPicker;        //The point picker
    vtkSmartPointer<vtkRenderer> ren;
    QVTKOpenGLNativeWidget* qvtkwidget;
    bool selectionMode;
    bool visiblePointsOnly;
    bool leftPressed;
    double sphereRadius;
    std::shared_ptr<Drawables::DrawableTriangleMesh> mesh;
    std::shared_ptr<Drawables::DrawablePointAnnotation> annotation;

};



#endif // VERTICESSELECTIONSTYLE_H
