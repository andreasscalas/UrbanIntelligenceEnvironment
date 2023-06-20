#include "triangleselectionstyle.hpp"
#include "vtkRenderWindow.h"

#include <vtkVertexGlyphFilter.h>
#include <vtkPointData.h>
#include <vtkRenderedAreaPicker.h>
#include <vtkExtractGeometry.h>
#include <vtkSelectVisiblePoints.h>
#include <vtkParametricFunctionSource.h>
#include <vtkParametricSpline.h>
#include <vtkSphereSource.h>
#include <vtkImplicitFunction.h>
#include <vtkPlanes.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderLargeImage.h>
#include <vtkLine.h>
#include <vtkWorldPointPicker.h>

#include <chrono>
using namespace std::chrono;

using namespace std;
using namespace SemantisedTriangleMesh;

TriangleSelectionStyle::TriangleSelectionStyle(){
    selectionMode = true;
    selectionType = SelectionType::RECTANGLE_AREA;
    visibleTrianglesOnly = true;
    lasso_started = false;
    alreadyStarted = false;
    showSelectedTriangles = true;
    firstVertex = nullptr;
    lastVertex = nullptr;
    this->annotation = std::make_shared<DrawableSurfaceAnnotation>();
    splinePoints = vtkSmartPointer<vtkPoints>::New();
    splineActor  = vtkSmartPointer<vtkActor>::New();
    splineActor->GetProperty()->SetColor(1.0,0,0);
    splineActor->GetProperty()->SetLineWidth(3.0);
    sphereAssembly = vtkSmartPointer<vtkPropAssembly>::New();          //Assembly of actors
    this->cellPicker = vtkSmartPointer<vtkCellPicker>::New();
}

void TriangleSelectionStyle::OnRightButtonDown(){

    if(mesh == nullptr) return;
    //If the user is trying to pick a point...
    //The click position of the mouse is takenannotatedTriangles
    int x, y;
    x = this->Interactor->GetEventPosition()[0];
    y = this->Interactor->GetEventPosition()[1];
    this->FindPokedRenderer(x, y);
    //Some tolerance is set for the picking
    this->cellPicker->SetTolerance(0);
    this->cellPicker->Pick(x, y, 0, ren);
    //If some point has been picked...
    vtkIdType pickedTriangleID = this->cellPicker->GetCellId();
    if(pickedTriangleID > 0 && pickedTriangleID < this->mesh->getTrianglesNumber()){

        vector<std::string> selected;
        if(lasso_started){
            auto t = mesh->getTriangle(static_cast<unsigned long>(pickedTriangleID));
            std::dynamic_pointer_cast<DrawableSurfaceAnnotation>(this->annotation)->addOutline(polygonContour);
            auto innerTriangles = mesh->regionGrowing(polygonContour, t);
            for(auto tit = innerTriangles.begin(); tit != innerTriangles.end(); tit++){
                selected.push_back((*tit)->getId());
            }
            splinePoints = vtkSmartPointer<vtkPoints>::New();
            assembly->RemovePart(sphereAssembly);
            sphereAssembly = vtkSmartPointer<vtkPropAssembly>::New();
            polygonContour.clear();
            lastVertex = nullptr;
            firstVertex = nullptr;
            lasso_started = false;
            this->assembly->RemovePart(splineActor);
        }else
            selected.push_back(std::to_string(static_cast<unsigned long>(pickedTriangleID)));

        defineSelection(selected);

    }
}


void TriangleSelectionStyle::OnMouseMove(){

    vtkInteractorStyleRubberBandPick::OnMouseMove();

}

void TriangleSelectionStyle::OnLeftButtonDown(){

    if(this->Interactor->GetControlKey())
        switch(selectionType){

            case SelectionType::RECTANGLE_AREA:

                this->CurrentMode = VTKISRBP_SELECT;
                break;

            case SelectionType::LASSO_AREA:

                if(!lasso_started)
                    lasso_started = true;

                else{
                    //The click position of the mouse is taken
                    int x, y;
                    x = this->Interactor->GetEventPosition()[0];
                    y = this->Interactor->GetEventPosition()[1];
                    this->FindPokedRenderer(x, y);
                    //Some tolerance is set for the picking
                    vtkSmartPointer<vtkWorldPointPicker> picker = vtkSmartPointer<vtkWorldPointPicker>::New();
                    double pickPos[3];
                    int picked = picker->Pick(x, y, 0, this->GetCurrentRenderer());
                    picker->GetPickPosition(pickPos);
                    SemantisedTriangleMesh::Point pickedPos(pickPos[0], pickPos[1], pickPos[2]);
                    if(picked >= 0)
                    {
                        auto v = mesh->getClosestPoint(pickedPos);

                        vtkIdType pointID = static_cast<vtkIdType>(std::stoi(v->getId()));
                        auto actualVertex = mesh->getVertex(static_cast<unsigned long>(pointID));
                        vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
                        sphereSource->SetCenter(actualVertex->getX(), actualVertex->getY(), actualVertex->getZ());
                        sphereSource->SetRadius(sphereRadius);
                        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                        mapper->SetInputConnection(sphereSource->GetOutputPort());
                        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
                        actor->GetProperty()->SetColor(0,0,1);
                        actor->SetMapper(mapper);
                        sphereAssembly->AddPart(actor);
                        if(firstVertex == nullptr){
                            firstVertex = actualVertex;
                            polygonContour.push_back(firstVertex);
                        }if(lastVertex != nullptr && lastVertex != actualVertex){
                            auto newContourSegment = mesh->computeShortestPath(lastVertex, actualVertex, DistanceType::EUCLIDEAN_DISTANCE, true, false);
                            polygonContour.insert(polygonContour.end(), newContourSegment.begin(), newContourSegment.end());
                            draw();
                        }
                        lastVertex = actualVertex;
                    }
                }

                break;

            case SelectionType::PAINTED_LINE:
                break;

            default: break;
        }

    vtkInteractorStyleRubberBandPick::OnLeftButtonDown();
}

void TriangleSelectionStyle::OnLeftButtonUp(){

    vtkInteractorStyleRubberBandPick::OnLeftButtonUp();

    switch(selectionType){

        case SelectionType::RECTANGLE_AREA:
            if(this->CurrentMode==VTKISRBP_SELECT){
                this->CurrentMode = VTKISRBP_ORIENT;

                // Forward events

                vtkPlanes* frustum = static_cast<vtkRenderedAreaPicker*>(this->GetInteractor()->GetPicker())->GetFrustum();

                vtkSmartPointer<vtkExtractGeometry> extractGeometry = vtkSmartPointer<vtkExtractGeometry>::New();
                extractGeometry->SetImplicitFunction(static_cast<vtkImplicitFunction*>(frustum));

                #if VTK_MAJOR_VERSION <= 5
                    extractGeometry->SetInput(this->Triangles);
                #else
                    extractGeometry->SetInputData(this->triangles);
                #endif
                    extractGeometry->Update();

                vtkSmartPointer<vtkVertexGlyphFilter> glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
                glyphFilter->SetInputConnection(extractGeometry->GetOutputPort());
                glyphFilter->Update();

                vtkSmartPointer<vtkPolyData> selected;
                if(visibleTrianglesOnly){
                    vtkSmartPointer<vtkSelectVisiblePoints> selectVisiblePoints = vtkSmartPointer<vtkSelectVisiblePoints>::New();
                    selectVisiblePoints->SetInputConnection(glyphFilter->GetOutputPort());
                    selectVisiblePoints->SetRenderer(this->GetCurrentRenderer());
                    selectVisiblePoints->Update();
                    selected = selectVisiblePoints->GetOutput();
                }else
                    selected = glyphFilter->GetOutput();

                vector<std::string> newlySelected;
                vtkSmartPointer<vtkIdTypeArray> ids = vtkIdTypeArray::SafeDownCast(selected->GetPointData()->GetArray("OriginalMeshIds"));
                for(vtkIdType i = 0; ids!=nullptr && i < ids->GetNumberOfTuples(); i++){
                    vtkSmartPointer<vtkIdList> tids = vtkSmartPointer<vtkIdList>::New();
                    triangles->GetPointCells(ids->GetValue(i), tids);
                    for(vtkIdType j = 0; j < tids->GetNumberOfIds(); j++){
                        newlySelected.push_back(std::to_string(static_cast<unsigned long>(tids->GetId(j))));
                    }
                }

                defineSelection(newlySelected);

            }
            break;

    }

}

void TriangleSelectionStyle::SetTriangles(vtkSmartPointer<vtkPolyData> triangles) {this->triangles = triangles;}

void TriangleSelectionStyle::resetSelection(){

    if(mesh == nullptr) return;
    for(uint i = 0; i < mesh->getTrianglesNumber(); i++){
        auto t = mesh->getTriangle(i);
        t->removeFlag(FlagType::SELECTED);
        mesh->setTriangleColor(t->getId(), mesh->ORIGINAL_COLOR);
    }
}

void TriangleSelectionStyle::defineSelection(std::vector<string> selected){
    if(mesh == nullptr) return;
    for(auto it = selected.begin(); it != selected.end(); it++){
        auto t = mesh->getTriangle(*it);
        if(selectionMode)
        {
            t->addFlag(FlagType::SELECTED);
            mesh->setTriangleColor(t->getId(), mesh->RED);
        }
        else
        {
            t->removeFlag(FlagType::SELECTED);
            mesh->setTriangleColor(t->getId(), mesh->ORIGINAL_COLOR);
        }
    }

    draw();

}

bool TriangleSelectionStyle::getShowSelectedTriangles() const{
    return showSelectedTriangles;
}

void TriangleSelectionStyle::setShowSelectedTriangles(bool value){
    showSelectedTriangles = value;
}

bool TriangleSelectionStyle::getVisibleTrianglesOnly() const{
    return visibleTrianglesOnly;
}

void TriangleSelectionStyle::setVisibleTrianglesOnly(bool value){
    visibleTrianglesOnly = value;
}

bool TriangleSelectionStyle::getSelectionMode() const{
    return selectionMode;
}

void TriangleSelectionStyle::setSelectionMode(bool value){
    selectionMode = value;
}

vtkSmartPointer<vtkRenderer> TriangleSelectionStyle::getRen() const{
    return ren;
}

void TriangleSelectionStyle::setRen(const vtkSmartPointer<vtkRenderer> &value){
    ren = value;
}

vtkSmartPointer<vtkPropAssembly> TriangleSelectionStyle::getAssembly() const{
    return assembly;
}

void TriangleSelectionStyle::setAssembly(const vtkSmartPointer<vtkPropAssembly> &value){
    assembly = value;
}

void TriangleSelectionStyle::finalizeAnnotation(std::string id, string tag, unsigned char color[]){

    if(mesh == nullptr) return;
    vector<std::shared_ptr<SemantisedTriangleMesh::Triangle> > selectedTriangles;
    for(uint i = 0; i < mesh->getTrianglesNumber(); i++){
        auto t = mesh->getTriangle(i);
        if(t->searchFlag(FlagType::SELECTED) >= 0)
        {
            t->removeFlag(FlagType::SELECTED);
            selectedTriangles.push_back(t);
            mesh->setTriangleColor(t->getId(), mesh->ORIGINAL_COLOR);
        }

    }

    if(selectedTriangles.size() > 0){

        this->annotation->setId(id);
        auto outlines = mesh->getOutlines(selectedTriangles);
        std::dynamic_pointer_cast<DrawableSurfaceAnnotation>(this->annotation)->setOutlines(outlines);
        this->annotation->setColor(color);
        this->annotation->setTag(tag);
        std::dynamic_pointer_cast<DrawableSurfaceAnnotation>(this->annotation)->setMeshPoints(mesh->getMeshVertices());
        this->annotation->setMesh(mesh);
        this->mesh->addAnnotation(annotation);
        std::dynamic_pointer_cast<DrawableSurfaceAnnotation>(this->annotation)->update();
        this->annotation = std::make_shared<DrawableSurfaceAnnotation>();
        emit(updateView());
    }

}

void TriangleSelectionStyle::draw()
{
    ren->RemoveActor(assembly);
    this->assembly->RemovePart(splineActor);
    this->assembly->RemovePart(sphereAssembly);
    this->assembly->RemovePart(mesh->getCanvas());
    if(selectionType == SelectionType::LASSO_AREA)
    {

        auto polyLineSegments = vtkSmartPointer<vtkCellArray>::New();
        auto polylinePoints = vtkSmartPointer<vtkPoints>::New();
        if(polygonContour.size() > 1)
        {
            polylinePoints->InsertNextPoint(triangles->GetPoint(static_cast<vtkIdType>(std::stoi(polygonContour[0]->getId()))));
            for(unsigned int i = 1; i < polygonContour.size(); i++)
            {
                polylinePoints->InsertNextPoint(triangles->GetPoint(static_cast<vtkIdType>(std::stoi(polygonContour[i]->getId()))));
                vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
                line->GetPointIds()->SetNumberOfIds(2);
                line->GetPointIds()->SetId(0, static_cast<vtkIdType>(i - 1));
                line->GetPointIds()->SetId(1, static_cast<vtkIdType>(i));
                polyLineSegments->InsertNextCell(line);
            }
            auto polydata = vtkSmartPointer<vtkPolyData>::New();
            polydata->SetPoints(polylinePoints);
            polydata->SetLines(polyLineSegments);
            vtkSmartPointer<vtkPolyDataMapper> splineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            splineMapper->SetInputData(polydata);
            splineActor->SetMapper(splineMapper);
            splineActor->GetProperty()->SetLineWidth(5);
            this->assembly->AddPart(splineActor);
        }
        this->assembly->AddPart(sphereAssembly);
    } else
        mesh->draw(assembly);

    this->assembly->Modified();
    ren->AddActor(assembly);
    ren->Render();
    ren->GetRenderWindow()->Render();
    this->qvtkWidget->update();
}

QVTKOpenGLNativeWidget *TriangleSelectionStyle::getQvtkWidget() const
{
    return qvtkWidget;
}

void TriangleSelectionStyle::setQvtkWidget(QVTKOpenGLNativeWidget *value)
{
    qvtkWidget = value;
}

const std::shared_ptr<SemantisedTriangleMesh::Vertex> &TriangleSelectionStyle::getInnerVertex() const
{
    return innerVertex;
}

void TriangleSelectionStyle::setInnerVertex(const std::shared_ptr<SemantisedTriangleMesh::Vertex> &newInnerVertex)
{
    innerVertex = newInnerVertex;
}

const std::shared_ptr<SemantisedTriangleMesh::Vertex> &TriangleSelectionStyle::getLastVertex() const
{
    return lastVertex;
}

void TriangleSelectionStyle::setLastVertex(const std::shared_ptr<SemantisedTriangleMesh::Vertex> &newLastVertex)
{
    lastVertex = newLastVertex;
}

const std::shared_ptr<DrawableTriangleMesh> &TriangleSelectionStyle::getMesh() const
{
    return mesh;
}

void TriangleSelectionStyle::setMesh(const std::shared_ptr<DrawableTriangleMesh> &newMesh)
{
    mesh = newMesh;
    cellPicker = vtkSmartPointer<vtkCellPicker>::NewInstance(cellPicker);
    cellPicker->SetPickFromList(1);
    cellPicker->AddPickList(mesh->getSurfaceActor());
    this->sphereRadius = this->mesh->getMinEdgeLength();
}

const std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex> > &TriangleSelectionStyle::getPolygonContour() const
{
    return polygonContour;
}

void TriangleSelectionStyle::setPolygonContour(const std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex> > &newPolygonContour)
{
    polygonContour = newPolygonContour;
}

TriangleSelectionStyle::SelectionType TriangleSelectionStyle::getSelectionType() const
{
    return selectionType;
}

void TriangleSelectionStyle::setSelectionType(TriangleSelectionStyle::SelectionType newSelectionType)
{
    selectionType = newSelectionType;
}
