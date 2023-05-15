#include "lineselectionstyle.hpp"
#include <vtkCellArray.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSphereSource.h>
#include <vtkProperty.h>
#include <vtkLine.h>
#include <vtkPolyDataMapper.h>

using namespace std;
using namespace SemantisedTriangleMesh;
LineSelectionStyle::LineSelectionStyle()
{
    selectionMode = true;
    visiblePointsOnly = true;
    lassoStarted = false;
    alreadyStarted = false;
    showSelectedPoints = true;
    firstVertex = nullptr;
    lastVertex = nullptr;
    reachedID = 0;
    this->annotation = std::make_shared<DrawableLineAnnotation>();
    this->annotation->setId("0");
    this->annotation->setTag("");
    unsigned char color[3] = {255, 0, 0};
    this->annotation->setColor(color);
    polylinePoints = vtkSmartPointer<vtkPoints>::New();
    polyLineSegments = vtkSmartPointer<vtkCellArray>::New();
    splineActor  = vtkSmartPointer<vtkActor>::New();
    splineActor->GetProperty()->SetColor(1.0,0,0);
    splineActor->GetProperty()->SetLineWidth(3.0);
    sphereAssembly = vtkSmartPointer<vtkPropAssembly>::New();          //Assembly of actors
}

void LineSelectionStyle::OnRightButtonDown()
{
    if(lassoStarted){
        this->annotation->addPolyLine(polyLine);
        polylinePoints = vtkSmartPointer<vtkPoints>::NewInstance(polylinePoints);
        polyLineSegments = vtkSmartPointer<vtkCellArray>::NewInstance(polyLineSegments);
        this->assembly->RemovePart(sphereAssembly);
        sphereAssembly = vtkSmartPointer<vtkPropAssembly>::NewInstance(sphereAssembly);
        polyLine.clear();
        reachedID = 0;
        lastVertex = nullptr;
        firstVertex = nullptr;
        lassoStarted = false;
        this->assembly->RemovePart(splineActor);
        this->assembly->Modified();
        modifySelectedLines();

    }
}

void LineSelectionStyle::OnMouseMove()
{
    vtkInteractorStyleRubberBandPick::OnMouseMove();
}

void LineSelectionStyle::OnLeftButtonDown()
{
    if(this->Interactor->GetControlKey())

        if(!lassoStarted)
            lassoStarted = true;
        else{
            this->cellPicker = static_cast<vtkCellPicker*>(this->GetInteractor()->GetPicker());
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
            if(cellID > 0 && cellID < this->mesh->getEdgesNumber()){
                auto t = mesh->getTriangle(static_cast<unsigned long>(cellID));
                double wc[3];
                double bestDistance = DBL_MAX;

                this->cellPicker->GetPickPosition(wc);
                Point* p = new Point(wc[0], wc[1], wc[2]);
                auto v_ = t->getV1();
                std::shared_ptr<Vertex> v;

                for(unsigned int i = 0; i < 3; i++){
                    double actualDistance = ((*v_) - (*p)).norm();
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
                    polyLine.push_back(firstVertex);
                    polylinePoints->InsertNextPoint(points->GetPoint(static_cast<vtkIdType>(std::stoi(firstVertex->getId()))));
                }if(lastVertex != nullptr && lastVertex != actualVertex){
                    auto newSegment = mesh->computeShortestPath(lastVertex, actualVertex, DistanceType::EUCLIDEAN_DISTANCE, false);
                    polyLine.insert(polyLine.end(), newSegment.begin(), newSegment.end());
                    for(unsigned int i = 0; i < newSegment.size(); i++){
                        std::shared_ptr<Vertex> v1;
                        if(i > 0)
                            v1 = newSegment[i - 1];
                        else
                            v1 = lastVertex;
                        auto v2 = newSegment[i];
                        auto e = v1->getCommonEdge(v2);
                        e->addFlag(FlagType::SELECTED);
                        polylinePoints->InsertNextPoint(points->GetPoint(static_cast<vtkIdType>(std::stoi(v2->getId()))));
                        reachedID++;
                        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
                        line->GetPointIds()->SetNumberOfIds(2);
                        line->GetPointIds()->SetId(0, static_cast<vtkIdType>(reachedID - 1));
                        line->GetPointIds()->SetId(1, static_cast<vtkIdType>(reachedID));
                        polyLineSegments->InsertNextCell(line);
                    }
                    ren->RemoveActor(assembly);
                    this->assembly->RemovePart(splineActor);
                    // Setup actor and mapper
                    vtkSmartPointer<vtkPolyData> splineData = vtkSmartPointer<vtkPolyData>::New();
                    vtkSmartPointer<vtkPolyDataMapper> splineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                    splineData->SetPoints(polylinePoints);
                    splineData->SetLines(polyLineSegments);
                    splineMapper->SetInputData(splineData);
                    splineActor->SetMapper(splineMapper);
                    splineActor->GetProperty()->SetLineWidth(5);
                    splineActor->GetProperty()->SetColor(255,0,0);
                    this->assembly->AddPart(splineActor);
                    ren->AddActor(assembly);
                    ren->Render();
                    ren->GetRenderWindow()->Render();
                }
                lastVertex = actualVertex;
            }
        }

    vtkInteractorStyleRubberBandPick::OnLeftButtonDown();
}

void LineSelectionStyle::OnLeftButtonUp()
{
    vtkInteractorStyleRubberBandPick::OnLeftButtonUp();

}

void LineSelectionStyle::modifySelectedLines()
{
    if(showSelectedPoints)
    {
        this->annotation->setMesh(mesh);
        this->annotation->setMeshPoints(mesh->getMeshVertices());
        this->annotation->update();
        this->annotation->draw(assembly);
    }
    else{
        for(uint i = 0; i < mesh->getEdgesNumber(); i++)
        {
            auto e = mesh->getEdge(i);
            e->removeFlag(FlagType::SELECTED);
        }
    }
    this->mesh->draw(assembly);
    this->qvtkwidget->update();
}

void LineSelectionStyle::resetSelection()
{
    polyLine.clear();
    reachedID = 0;
    lastVertex = nullptr;
    firstVertex = nullptr;
    lassoStarted = false;
    polylinePoints = vtkSmartPointer<vtkPoints>::NewInstance(polylinePoints);
    polyLineSegments = vtkSmartPointer<vtkCellArray>::NewInstance(polyLineSegments);
    this->assembly->RemovePart(sphereAssembly);
    this->assembly->RemovePart(splineActor);
    this->assembly->RemovePart(annotation->getCanvas());
    this->assembly->Modified();
    this->annotation->clearPolylines();
    for(uint i = 0; i < mesh->getEdgesNumber(); i++){
        auto e = mesh->getEdge(i);
        e->removeFlag(FlagType::SELECTED);
    }

}

void LineSelectionStyle::finalizeAnnotation(std::string id, std::string tag, unsigned char color[])
{
    vector<std::shared_ptr<SemantisedTriangleMesh::Edge> > selectedEdges;
    for(uint i = 0; i < mesh->getEdgesNumber(); i++){
        auto e = mesh->getEdge(i);
        if(e->searchFlag(FlagType::SELECTED))
            selectedEdges.push_back(e);
    }

    if(selectedEdges.size() > 0){
        assembly->RemovePart(annotation->getCanvas());
        this->annotation->setId(id);
        this->annotation->setTag(tag);
        this->annotation->setColor(color);
        this->annotation->setMesh(mesh);
        this->mesh->addAnnotation(annotation);
        this->mesh->update();
        this->mesh->draw(assembly);
        this->annotation = std::make_shared<DrawableLineAnnotation>();
        this->annotation->setId("0");
        this->annotation->setTag("");
        unsigned char color[3] = {255, 0, 0};
        this->annotation->setColor(color);
        this->resetSelection();
    }
}


vtkSmartPointer<vtkActor> LineSelectionStyle::getSplineActor() const
{
    return splineActor;
}

void LineSelectionStyle::setSplineActor(const vtkSmartPointer<vtkActor> &value)
{
    splineActor = value;
}

vtkSmartPointer<vtkPropAssembly> LineSelectionStyle::getSphereAssembly() const
{
    return sphereAssembly;
}

void LineSelectionStyle::setSphereAssembly(const vtkSmartPointer<vtkPropAssembly> &value)
{
    sphereAssembly = value;
}

vtkSmartPointer<vtkPropAssembly> LineSelectionStyle::getAssembly() const
{
    return assembly;
}

void LineSelectionStyle::setAssembly(const vtkSmartPointer<vtkPropAssembly> &value)
{
    assembly = value;
}

vtkSmartPointer<vtkRenderer> LineSelectionStyle::getRen() const
{
    return ren;
}

void LineSelectionStyle::setRen(const vtkSmartPointer<vtkRenderer> &value)
{
    ren = value;
}

vtkSmartPointer<vtkParametricSpline> LineSelectionStyle::getSpline() const
{
    return spline;
}

void LineSelectionStyle::setSpline(const vtkSmartPointer<vtkParametricSpline> &value)
{
    spline = value;
}

vtkSmartPointer<vtkPoints> LineSelectionStyle::getPolylinePoints() const
{
    return polylinePoints;
}

void LineSelectionStyle::setPolylinePoints(const vtkSmartPointer<vtkPoints> &value)
{
    polylinePoints = value;
}

std::map<unsigned long, bool> *LineSelectionStyle::getPointsSelectionStatus() const
{
    return pointsSelectionStatus;
}

void LineSelectionStyle::setPointsSelectionStatus(std::map<unsigned long, bool> *value)
{
    pointsSelectionStatus = value;
}

double LineSelectionStyle::getSphereRadius() const
{
    return sphereRadius;
}

void LineSelectionStyle::setSphereRadius(double value)
{
    sphereRadius = value;
}

bool LineSelectionStyle::getSelectionMode() const
{
    return selectionMode;
}

void LineSelectionStyle::setSelectionMode(bool value)
{
    selectionMode = value;
}

bool LineSelectionStyle::getVisiblePointsOnly() const
{
    return visiblePointsOnly;
}

void LineSelectionStyle::setVisiblePointsOnly(bool value)
{
    visiblePointsOnly = value;
}

bool LineSelectionStyle::getShowSelectedPoints() const
{
    return showSelectedPoints;
}

void LineSelectionStyle::setShowSelectedPoints(bool value)
{
    showSelectedPoints = value;
}

bool LineSelectionStyle::getAlreadyStarted() const
{
    return alreadyStarted;
}

void LineSelectionStyle::setAlreadyStarted(bool value)
{
    alreadyStarted = value;
}

bool LineSelectionStyle::getLassoStarted() const
{
    return lassoStarted;
}

void LineSelectionStyle::setLassoStarted(bool value)
{
    lassoStarted = value;
}

double LineSelectionStyle::getTolerance() const
{
    return tolerance;
}

QVTKOpenGLNativeWidget *LineSelectionStyle::getQvtkwidget() const
{
    return qvtkwidget;
}

void LineSelectionStyle::setQvtkwidget(QVTKOpenGLNativeWidget *value)
{
    qvtkwidget = value;
}

const std::shared_ptr<DrawableTriangleMesh> &LineSelectionStyle::getMesh() const
{
    return mesh;
}

void LineSelectionStyle::setMesh(const std::shared_ptr<DrawableTriangleMesh> &newMesh)
{
    mesh = newMesh;
    this->sphereRadius = this->mesh->getAABBDiagonalLength() / RADIUS_RATIO;
    this->tolerance = this->mesh->getMinEdgeLength() * TOLERANCE_RATIO;
}

vtkSmartPointer<vtkPolyData> LineSelectionStyle::getPoints() const
{
    return points;
}

void LineSelectionStyle::setPoints(const vtkSmartPointer<vtkPolyData> &value)
{
    points = value;
}


vtkSmartPointer<vtkCellPicker> LineSelectionStyle::getCellPicker() const
{
    return cellPicker;
}

void LineSelectionStyle::setCellPicker(const vtkSmartPointer<vtkCellPicker> &value)
{
    cellPicker = value;
}
