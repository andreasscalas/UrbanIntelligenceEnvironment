#include "vtkRenderer.h"
#include <verticesselectionstyle.hpp>

#include <vtkSphereSource.h>
#include <vtkWorldPointPicker.h>
#include <vtkIdFilter.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkPointData.h>
#include <vtkActor.h>
#include <vtkRenderedAreaPicker.h>
#include <vtkExtractGeometry.h>
#include <vtkSelectVisiblePoints.h>
#include <vtkImplicitFunction.h>
#include <vtkPlanes.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkIdFilter.h>
#include <vtkRenderWindow.h>

using namespace std;
using namespace SemantisedTriangleMesh;
using namespace Drawables;

VerticesSelectionStyle::VerticesSelectionStyle() {

    selectionMode = true;
    visiblePointsOnly = true;
    leftPressed = false;
    this->assembly = vtkSmartPointer<vtkPropAssembly>::New();
    this->pointPicker = vtkSmartPointer<vtkPointPicker>::New();
    this->sphereAssembly = vtkSmartPointer<vtkPropAssembly>::New();
    this->annotation = std::make_shared<DrawablePointAnnotation>();

}


void VerticesSelectionStyle::OnRightButtonDown() {

	//If the user is trying to pick a point...
	//The click position of the mouse is taken
	int x, y;
    x = this->Interactor->GetEventPosition()[0];
    y = this->Interactor->GetEventPosition()[1];
    this->FindPokedRenderer(x, y);
    vtkSmartPointer<vtkWorldPointPicker> picker = vtkSmartPointer<vtkWorldPointPicker>::New();
    double pickPos[3];
    int picked = picker->Pick(x, y, 0, this->GetCurrentRenderer());
    picker->GetPickPosition(pickPos);

    std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex> > selected;
    SemantisedTriangleMesh::Point pickedPos(pickPos[0], pickPos[1], pickPos[2]);
    if(picked >= 0)
    {
        selected.push_back(mesh->getClosestPoint(pickedPos));
        defineSelection(selected);
        draw();
    }

	vtkInteractorStyleRubberBandPick::OnRightButtonDown();

}

void VerticesSelectionStyle::OnLeftButtonDown() {


    leftPressed = true;
	if (this->Interactor->GetControlKey())
		this->CurrentMode = VTKISRBP_SELECT;

	vtkInteractorStyleRubberBandPick::OnLeftButtonDown();

}

void VerticesSelectionStyle::OnLeftButtonUp() {

    vtkInteractorStyleRubberBandPick::OnLeftButtonUp();
    leftPressed = false;
	if (this->CurrentMode == VTKISRBP_SELECT) {

		this->CurrentMode = VTKISRBP_ORIENT;

		// Forward events

		vtkPlanes* frustum = static_cast<vtkRenderedAreaPicker*>(this->GetInteractor()->GetPicker())->GetFrustum();

		vtkSmartPointer<vtkExtractGeometry> extractGeometry = vtkSmartPointer<vtkExtractGeometry>::New();
        extractGeometry->SetImplicitFunction(static_cast<vtkImplicitFunction*>(frustum));

#if VTK_MAJOR_VERSION <= 5
		extractGeometry->SetInput(this->Points);
#else
        extractGeometry->SetInputData(this->points);
#endif
		extractGeometry->Update();

		vtkSmartPointer<vtkVertexGlyphFilter> glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
		glyphFilter->SetInputConnection(extractGeometry->GetOutputPort());
		glyphFilter->Update();

        vtkSmartPointer<vtkPolyData> selected;
		if (visiblePointsOnly) {
			vtkSmartPointer<vtkSelectVisiblePoints> selectVisiblePoints = vtkSmartPointer<vtkSelectVisiblePoints>::New();
			selectVisiblePoints->SetInputConnection(glyphFilter->GetOutputPort());
			selectVisiblePoints->SetRenderer(this->GetCurrentRenderer());
			selectVisiblePoints->Update();
			selected = selectVisiblePoints->GetOutput();
		}
		else
			selected = glyphFilter->GetOutput();


		vtkSmartPointer<vtkIdTypeArray> ids = vtkIdTypeArray::SafeDownCast(selected->GetPointData()->GetArray("OriginalMeshIds"));

		if (ids == nullptr)
			return;

        std::vector<std::shared_ptr<Vertex> > selectedPoints;
		for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); i++)
            selectedPoints.push_back(mesh->getVertex(ids->GetValue(i)));

        defineSelection(selectedPoints);
        draw();
    }

}

void VerticesSelectionStyle::OnMouseMove()
{
    vtkInteractorStyleRubberBandPick::OnMouseMove();
}

void VerticesSelectionStyle::resetSelection() {

    if(mesh == nullptr) return;

    this->assembly->RemovePart(sphereAssembly);
    this->sphereAssembly = vtkSmartPointer<vtkPropAssembly>::New();

    for (uint i = 0; i < mesh->getVerticesNumber(); i++) {
        auto v = mesh->getVertex(i);
        v->removeFlag(FlagType::SELECTED);
    }

    emit(updateView());
}


void VerticesSelectionStyle::defineSelection(std::vector<std::shared_ptr<Vertex> > selected) {
    if(mesh == nullptr) return;
    for (auto v : selected)
        if(selectionMode)
            v->addFlag(FlagType::SELECTED);
}

void VerticesSelectionStyle::draw() {

    if(mesh == nullptr) return;
    ren->RemoveActor(this->assembly);
    assembly->RemovePart(sphereAssembly);
    this->sphereAssembly = vtkSmartPointer<vtkPropAssembly>::New();
    bool thereAreSelected = false;

    for (unsigned long i = 0; i < static_cast<unsigned long>(mesh->getVerticesNumber()); i++)
        if (mesh->getVertex(i)->searchFlag(FlagType::SELECTED) >= 0)
        {
            vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
            auto p = mesh->getVertex(i);
            p->addFlag(FlagType::SELECTED);
            sphereSource->SetCenter(p->getX(), p->getY(), p->getZ());
            sphereSource->SetRadius(sphereRadius);
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(sphereSource->GetOutputPort());
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->GetProperty()->SetColor(0,0,1);
            actor->SetMapper(mapper);
            sphereAssembly->AddPart(actor);
            thereAreSelected = true;
        }

    if(thereAreSelected)
        assembly->AddPart(sphereAssembly);

    this->assembly->Modified();
    ren->AddActor(this->assembly);
    ren->Render();
    ren->GetRenderWindow()->Render();
    this->qvtkwidget->update();

}

void VerticesSelectionStyle::finalizeAnnotation(std::string id, string tag, unsigned char color[])
{
    if(mesh == nullptr) return;
    vector<std::shared_ptr<SemantisedTriangleMesh::Vertex> > selectedPoints;
    for(uint i = 0; i < mesh->getVerticesNumber(); i++){
        auto v = mesh->getVertex(i);
        if(v->searchFlag(FlagType::SELECTED) >= 0)
            selectedPoints.push_back(v);
    }

    if(selectedPoints.size() > 0){

        this->annotation->setId(id);
        this->annotation->setTag(tag);
        this->annotation->setColor(color);
        this->annotation->setPoints(selectedPoints);
        this->annotation->setMesh(mesh);
        this->mesh->addAnnotation(annotation);
        this->annotation = std::make_shared<DrawablePointAnnotation>();
        this->resetSelection();
        this->draw();
    }
}

vtkSmartPointer<vtkPolyData> VerticesSelectionStyle::getPoints() const
{
    return points;
}

void VerticesSelectionStyle::setPoints(const vtkSmartPointer<vtkPolyData>& value)
{
    points = value;
}


vtkSmartPointer<vtkPointPicker> VerticesSelectionStyle::getPointPicker() const
{
	return pointPicker;
}

void VerticesSelectionStyle::setPointPicker(const vtkSmartPointer<vtkPointPicker>& value)
{
	pointPicker = value;
}

bool VerticesSelectionStyle::getSelectionMode() const
{
	return selectionMode;
}

void VerticesSelectionStyle::setSelectionMode(bool value)
{
	selectionMode = value;
}

bool VerticesSelectionStyle::getVisiblePointsOnly() const
{
	return visiblePointsOnly;
}

void VerticesSelectionStyle::setVisiblePointsOnly(bool value)
{
	visiblePointsOnly = value;
}

vtkSmartPointer<vtkPropAssembly> VerticesSelectionStyle::getAssembly() const
{
	return assembly;
}

void VerticesSelectionStyle::setAssembly(const vtkSmartPointer<vtkPropAssembly>& value)
{
	assembly = value;
}

QVTKOpenGLNativeWidget* VerticesSelectionStyle::getQvtkwidget() const
{
	return qvtkwidget;
}

void VerticesSelectionStyle::setQvtkwidget(QVTKOpenGLNativeWidget* value)
{
	qvtkwidget = value;
}

const std::shared_ptr<DrawableTriangleMesh> &VerticesSelectionStyle::getMesh() const
{
    return mesh;
}

void VerticesSelectionStyle::setMesh(const std::shared_ptr<DrawableTriangleMesh> &newMesh)
{
    mesh = newMesh;
    vtkSmartPointer<vtkIdFilter> idFilterMesh = vtkSmartPointer<vtkIdFilter>::New();
    idFilterMesh->SetInputData(mesh->getPointsActor()->GetMapper()->GetInputAsDataSet());
    idFilterMesh->PointIdsOn();
    idFilterMesh->SetPointIdsArrayName("OriginalMeshIds");
    idFilterMesh->Update();
    vtkSmartPointer<vtkPolyData> inputMesh = static_cast<vtkPolyData*>(idFilterMesh->GetOutput());
    this->sphereRadius = this->mesh->getAABBDiagonalLength() / RADIUS_RATIO;
}

vtkSmartPointer<vtkRenderer> VerticesSelectionStyle::getRenderer() const
{
    return ren;
}

void VerticesSelectionStyle::setRenderer(vtkSmartPointer<vtkRenderer> newRen)
{
    ren = newRen;
}

