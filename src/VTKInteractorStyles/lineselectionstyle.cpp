#include "lineselectionstyle.hpp"
#include <vtkCellArray.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSphereSource.h>
#include <vtkProperty.h>
#include <vtkLine.h>
#include <vtkPolyDataMapper.h>
#include <vtkWorldPointPicker.h>

using namespace std;
using namespace SemantisedTriangleMesh;
using namespace Drawables;

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
        ren->RemoveActor(assembly);
        this->assembly->RemovePart(sphereAssembly);
        sphereAssembly = vtkSmartPointer<vtkPropAssembly>::NewInstance(sphereAssembly);
        polyLine.clear();
        lastVertex = nullptr;
        firstVertex = nullptr;
        lassoStarted = false;
        this->assembly->Modified();
        this->ren->AddActor(this->assembly);
        this->ren->Render();
        this->qvtkwidget->update();
        this->qvtkwidget->renderWindow()->Render();

    }
}

void LineSelectionStyle::OnMouseMove()
{
    vtkInteractorStyleRubberBandPick::OnMouseMove();
}

void LineSelectionStyle::OnLeftButtonDown()
{
    if(mesh == nullptr) return;
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
                    polyLine.push_back(firstVertex);
                }if(lastVertex != nullptr && lastVertex != actualVertex){
                    auto newPolyline = mesh->computeShortestPath(lastVertex, actualVertex, DistanceType::EUCLIDEAN_DISTANCE, true, false);
                    polyLine.insert(polyLine.end(), newPolyline.begin(), newPolyline.end());
                    std::vector<std::vector<std::shared_ptr<Vertex> > > newPolylines = {newPolyline};
                    defineSelection(newPolylines);

                }
                draw();
                lastVertex = actualVertex;
            }
        }

    vtkInteractorStyleRubberBandPick::OnLeftButtonDown();
}

void LineSelectionStyle::OnLeftButtonUp()
{
    vtkInteractorStyleRubberBandPick::OnLeftButtonUp();

}

void LineSelectionStyle::resetSelection()
{
    if(mesh == nullptr) return;
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

void LineSelectionStyle::defineSelection(std::vector<std::vector<std::shared_ptr<Vertex> > > polylines)
{
    for(uint i = 0; i < polylines.size(); i++)
    {
        polylinePoints->InsertNextPoint(points->GetPoint(static_cast<vtkIdType>(std::stoi(polylines.at(i).at(0)->getId()))));
        for(uint j = 1; j < polylines.at(i).size(); j++)
        {
            std::shared_ptr<Vertex> v1 = polylines.at(i).at(j - 1);
            auto v2 = polylines.at(i).at(j);
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
        reachedID++;
        annotation->addPolyLine(polylines.at(i));
    }
}

void LineSelectionStyle::finalizeAnnotation(std::string id, std::string tag, unsigned char color[])
{
    if(mesh == nullptr) return;
    vector<std::shared_ptr<SemantisedTriangleMesh::Edge> > selectedEdges;
    for(uint i = 0; i < mesh->getEdgesNumber(); i++){
        auto e = mesh->getEdge(i);
        if(e->searchFlag(FlagType::SELECTED) >= 0)
            selectedEdges.push_back(e);
    }

    if(selectedEdges.size() > 0){
        assembly->RemovePart(annotation->getCanvas());
        this->annotation->setId(id);
        this->annotation->setTag(tag);
        this->annotation->setColor(color);
        this->annotation->setMesh(mesh);
        this->annotation->update();
        this->mesh->addAnnotation(annotation);
        this->mesh->draw(assembly);
        this->annotation = std::make_shared<DrawableLineAnnotation>();
        this->annotation->setId("0");
        this->annotation->setTag("");
        unsigned char color[3] = {255, 0, 0};
        this->annotation->setColor(color);
        this->resetSelection();
    }
    emit(updateView());
}

void LineSelectionStyle::draw()
{
    ren->RemoveActor(assembly);
    this->assembly->RemovePart(splineActor);
    this->assembly->RemovePart(sphereAssembly);

    // Setup actor and mapper
    if(annotation->getPolyLines().size() > 0 || polyLine.size() > 1)
    {
        vtkSmartPointer<vtkPolyData> splineData = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPolyDataMapper> splineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        splineData->SetPoints(polylinePoints);
        splineData->SetLines(polyLineSegments);
        splineMapper->SetInputData(splineData);
        splineActor->SetMapper(splineMapper);
        splineActor->GetProperty()->SetLineWidth(5);
        splineActor->GetProperty()->SetColor(255,0,0);
        this->assembly->AddPart(splineActor);
    }
    this->assembly->AddPart(sphereAssembly);
    this->assembly->Modified();
    ren->AddActor(assembly);
    ren->Render();
    ren->GetRenderWindow()->Render();
    this->qvtkwidget->update();
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
