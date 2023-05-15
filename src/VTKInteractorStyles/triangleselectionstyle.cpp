#include "triangleselectionstyle.hpp"

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
    SplineActor  = vtkSmartPointer<vtkActor>::New();
    SplineActor->GetProperty()->SetColor(1.0,0,0);
    SplineActor->GetProperty()->SetLineWidth(3.0);
    sphereAssembly = vtkSmartPointer<vtkPropAssembly>::New();          //Assembly of actors
    this->cellPicker = vtkSmartPointer<vtkCellPicker>::New();
}

void TriangleSelectionStyle::OnRightButtonDown(){

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

        vector<unsigned long> selected;
        if(lasso_started){
            auto t = mesh->getTriangle(static_cast<unsigned long>(pickedTriangleID));
            std::dynamic_pointer_cast<DrawableSurfaceAnnotation>(this->annotation)->addOutline(polygonContour);
            auto innerTriangles = mesh->regionGrowing(polygonContour, t);
            for(auto tit = innerTriangles.begin(); tit != innerTriangles.end(); tit++){
                selected.push_back(std::stoi((*tit)->getId()));
            }
            splinePoints = vtkSmartPointer<vtkPoints>::New();
            assembly->RemovePart(sphereAssembly);
            sphereAssembly = vtkSmartPointer<vtkPropAssembly>::New();
            polygonContour.clear();
            lastVertex = nullptr;
            firstVertex = nullptr;
            lasso_started = false;
            this->assembly->RemovePart(SplineActor);
        }else
            selected.push_back(static_cast<unsigned long>(pickedTriangleID));

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
                    this->cellPicker->Pick(x, y, 0, ren);
                    vtkIdType cellID = this->cellPicker->GetCellId();

                    //If some point has been picked...
                    if(cellID > 0 && cellID < this->mesh->getTrianglesNumber()){
                        auto t = mesh->getTriangle(static_cast<unsigned long>(cellID));
                        double wc[3];
                        double bestDistance = DBL_MAX;

                        this->cellPicker->GetPickPosition(wc);
                        auto p = std::make_shared<Vertex>(wc[0], wc[1], wc[2]);
                        auto v_ = t->getV1();
                        std::shared_ptr<Vertex> v;

                        for(int i = 0; i < 3; i++){
                            double actualDistance = ((*v_)-(*p)).norm();
                            if(actualDistance < bestDistance){
                                bestDistance = actualDistance;
                                v = v_;
                            }
                            v_ = t->getNextVertex(v_);
                        }

                        vtkIdType pointID = static_cast<vtkIdType>(std::stoi(v->getId()));
                        vtkSmartPointer<vtkParametricSpline> spline = vtkSmartPointer<vtkParametricSpline>::New();
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
                        assembly->AddPart(sphereAssembly);
                        if(firstVertex == nullptr){
                            firstVertex = actualVertex;
                            polygonContour.push_back(firstVertex);
                        }if(lastVertex != nullptr && lastVertex != actualVertex){
                            auto newContourSegment = mesh->computeShortestPath(lastVertex, actualVertex, DistanceType::COMBINED_DISTANCE, false);
                            polygonContour.insert(polygonContour.end(), newContourSegment.begin(), newContourSegment.end());
                            for(unsigned int i = 0; i < newContourSegment.size(); i++)
                                splinePoints->InsertNextPoint(Triangles->GetPoint(static_cast<vtkIdType>(std::stoi(newContourSegment[i]->getId()))));
                            spline->SetPoints(splinePoints);
                            vtkSmartPointer<vtkParametricFunctionSource> functionSource = vtkSmartPointer<vtkParametricFunctionSource>::New();
                            functionSource->SetParametricFunction(spline);
                            functionSource->Update();
                            ren->RemoveActor(assembly);
                            this->assembly->RemovePart(SplineActor);
                            // Setup actor and mapper
                            vtkSmartPointer<vtkPolyDataMapper> splineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                            splineMapper->SetInputConnection(functionSource->GetOutputPort());
                            SplineActor->SetMapper(splineMapper);
                            SplineActor->GetProperty()->SetLineWidth(5);
                            this->assembly->AddPart(SplineActor);
                            ren->AddActor(assembly);
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
                    extractGeometry->SetInputData(this->Triangles);
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

                vector<unsigned long> newlySelected;
                vtkSmartPointer<vtkIdTypeArray> ids = vtkIdTypeArray::SafeDownCast(selected->GetPointData()->GetArray("OriginalMeshIds"));
                for(vtkIdType i = 0; ids!=nullptr && i < ids->GetNumberOfTuples(); i++){
                    vtkSmartPointer<vtkIdList> tids = vtkSmartPointer<vtkIdList>::New();
                    Triangles->GetPointCells(ids->GetValue(i), tids);
                    for(vtkIdType j = 0; j < tids->GetNumberOfIds(); j++){
                        newlySelected.push_back(static_cast<unsigned long>(tids->GetId(j)));
                    }
                }

                defineSelection(newlySelected);

            }
            break;

    }

}

void TriangleSelectionStyle::SetTriangles(vtkSmartPointer<vtkPolyData> triangles) {this->Triangles = triangles;}

void TriangleSelectionStyle::resetSelection(){

    for(uint i = 0; i < mesh->getTrianglesNumber(); i++){
        auto t = mesh->getTriangle(i);
        t->removeFlag(FlagType::SELECTED);
    }
}

void TriangleSelectionStyle::defineSelection(vector<unsigned long> selected){
    for(vector<unsigned long>::iterator it = selected.begin(); it != selected.end(); it++){
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
    this->mesh->draw(assembly);
    emit(updateView());

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
        std::dynamic_pointer_cast<DrawableSurfaceAnnotation>(this->annotation)->setOutlines(mesh->getOutlines(selectedTriangles));
        this->annotation->setColor(color);
        this->annotation->setTag(tag);
        std::dynamic_pointer_cast<DrawableSurfaceAnnotation>(this->annotation)->setMeshPoints(mesh->getMeshVertices());
        this->annotation->setMesh(mesh);
        this->mesh->addAnnotation(annotation);
        std::dynamic_pointer_cast<DrawableSurfaceAnnotation>(this->annotation)->update();
        this->mesh->draw(assembly);
        this->annotation = std::make_shared<DrawableSurfaceAnnotation>();
        emit(updateView());
    }
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
