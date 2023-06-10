#include "measurestyle.hpp"
#include "vtkRenderWindow.h"
#include "vtkSphereSource.h"

#include <geometricattribute.hpp>
#include <drawableeuclideanmeasure.hpp>
#include <drawablegeodesicmeasure.hpp>
#include <drawableboundingmeasure.hpp>
#include <drawableannotation.hpp>

#include <vector>

#include <vtkPolyData.h>
#include <vtkCamera.h>
#include <vtkPlane.h>
#include <vtkCutter.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkStripper.h>
#include <vtkFeatureEdges.h>
#include <vtkPropPicker.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkWorldPointPicker.h>

using namespace std;
using namespace SemantisedTriangleMesh;
MeasureStyle::MeasureStyle()
{
    measureStarted = false;
    leftPressed = false;
    middlePressed = false;
    drawAttributes = false;
    measureAssembly = vtkSmartPointer<vtkPropAssembly>::New();
    cellPicker = vtkSmartPointer<vtkCellPicker>::New();
    measureType = MeasureType::RULER;
    boundingOrigin = nullptr;
    boundingBegin = nullptr;
    boundingEnd = nullptr;
    onCreationAttribute = nullptr;
}

MeasureStyle::~MeasureStyle()
{
    mesh = nullptr;
    qvtkwidget = nullptr;
    if(onCreationAttribute != nullptr)
        onCreationAttribute.reset();
}

std::shared_ptr<DrawableTriangleMesh> MeasureStyle::getMesh() const
{
    return mesh;
}

void MeasureStyle::setMesh(std::shared_ptr<DrawableTriangleMesh>value)
{
    mesh = value;
    cellPicker = vtkSmartPointer<vtkCellPicker>::NewInstance(cellPicker);
    cellPicker->SetPickFromList(1);
    cellPicker->AddPickList(mesh->getSurfaceActor());
}


void MeasureStyle::manageRulerMovement(std::shared_ptr<SemantisedTriangleMesh::Vertex> start, std::shared_ptr<SemantisedTriangleMesh::Vertex> end)
{
    if(start == end)
        return;
    SemantisedTriangleMesh::Point measureVector = (*end) - (*start);
    double measure = measureVector.norm();
    if(measure < epsilon)
        return;
    this->measure = measure;
    if(measurePath.size() == 2)
        measurePath.pop_back();
    measurePath.push_back(end);
    dynamic_pointer_cast<SemantisedTriangleMesh::GeometricAttribute>(onCreationAttribute)->clearMeasurePointsID();
    auto eucAtt = dynamic_pointer_cast<DrawableEuclideanMeasure>(onCreationAttribute);
    eucAtt->addMeasurePointID(std::stoi(measurePath[0]->getId()));
    if(measurePath.size() > 1)
        eucAtt->addMeasurePointID(std::stoi(measurePath[1]->getId()));


}

void MeasureStyle::manageTapeMovement(std::shared_ptr<SemantisedTriangleMesh::Vertex> start, std::shared_ptr<SemantisedTriangleMesh::Vertex> end)
{
    auto path = mesh->computeShortestPath(start, end, DistanceType::EUCLIDEAN_DISTANCE, true, true);
    path.insert(path.begin(), start);
    measurePath.insert(measurePath.end(), path.begin(), path.end());
    Point measureVector;

    for(unsigned int i = 1; i < path.size(); i++){
        measureVector = (*path[i]) - (*path[i - 1]);
        measure += measureVector.norm();
    }

    auto geoAtt = dynamic_pointer_cast<DrawableGeodesicMeasure>(onCreationAttribute);
    geoAtt->clearMeasurePointsID();
    for(unsigned int i = 0; i < measurePath.size(); i++)
    {
        geoAtt->addMeasurePointID(std::stoi(measurePath[i]->getId()));
    }
}

void MeasureStyle::manageHeightMovement(std::shared_ptr<SemantisedTriangleMesh::Vertex> start)
{

//    auto heightAtt = dynamic_pointer_cast<DrawableHeightMeasure>(onCreationAttribute);
//    heightAtt->clearMeasurePointsID();
//    heightAtt->addMeasurePointID(std::stoi(start->getId()));
//    heightAtt->setDirection(0.0,0.0,1.0);
//    heightAtt->update();
//    measure = ((*start) - (*heightAtt->getBase())).length();
}


void MeasureStyle::manageCaliberMovement()
{
    vtkSmartPointer<vtkPlane> cuttingPlane = vtkSmartPointer<vtkPlane>::New();
    double* orientation = this->CurrentRenderer->GetActiveCamera()->GetViewPlaneNormal();
    Point direction(orientation[0], orientation[1], orientation[2]);
    direction.normalise();
    Point caliperDirection((*boundingEnd) - (*boundingBegin));
    caliperDirection.normalise();
    Point normal = direction & caliperDirection;
    normal.normalise();
    cuttingPlane->SetOrigin(boundingBegin->getX(), boundingBegin->getY(), boundingBegin->getZ());
    cuttingPlane->SetNormal(normal.getX(), normal.getY(), normal.getZ());
    vtkSmartPointer<vtkPolyData> meshData = static_cast<vtkPolyData*>(mesh->getSurfaceActor()->GetMapper()->GetInput());

    vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
    cutter->SetInputData(meshData);
    cutter->SetCutFunction(cuttingPlane);
    vtkSmartPointer<vtkStripper> cutStrips = vtkSmartPointer<vtkStripper>::New();
    cutStrips->SetInputConnection(cutter->GetOutputPort());
    cutStrips->Update();
    vtkSmartPointer<vtkPolyData> cutPoly = vtkSmartPointer<vtkPolyData>::New();
    cutPoly->SetPoints(cutStrips->GetOutput()->GetPoints());
    cutPoly->SetPolys(cutStrips->GetOutput()->GetLines());
    vtkSmartPointer<vtkFeatureEdges> cutBoundary = vtkSmartPointer<vtkFeatureEdges>::New();
    cutBoundary->SetInputData(cutPoly);
    cutBoundary->Update();

    vtkSmartPointer<vtkPolyData> boundaryPolydata = cutBoundary->GetOutput();
    boundaryPolydata->GetLines()->InitTraversal();
    vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
    std::map<vtkIdType, std::shared_ptr<Point> > points;
    vector<std::shared_ptr<Point> > usefulProjected;
    Point projectedExtreme1 = caliperDirection * ((*boundingBegin) * caliperDirection);
    Point projectedExtreme2 = caliperDirection * ((*boundingEnd) * caliperDirection);
    while(boundaryPolydata->GetLines()->GetNextCell(idList))
        for(vtkIdType i = 0; i < idList->GetNumberOfIds(); i++)
        {
            double* p = boundaryPolydata->GetPoint(idList->GetId(i));
            std::shared_ptr<Point> p_ = std::make_shared<Point>(p[0],p[1],p[2]);
            std::shared_ptr<Point> projected = std::make_shared<Point>(caliperDirection * ((*p_) * caliperDirection));
            if(projected->isInSegment(projectedExtreme1, projectedExtreme2))
            {
                 points.at(idList->GetId(i)) = p_;
                 usefulProjected.push_back(projected);
            }
        }
    if(usefulProjected.size() == 0)
        return;
    auto extrema = Point::findExtremePoints(usefulProjected, caliperDirection);
    int pos1 = std::find(usefulProjected.begin(), usefulProjected.end(), extrema.first) - usefulProjected.begin();
    int pos2 = std::find(usefulProjected.begin(), usefulProjected.end(), extrema.second) - usefulProjected.begin();
    measurePath.clear();
    auto extremePoint1 = std::next(points.begin(), pos1)->second;
    auto extremePoint2 = std::next(points.begin(), pos2)->second;
    measurePath.push_back(mesh->getNearestNeighbours(*extremePoint1, 1, std::numeric_limits<double>::max())[0]);
    measurePath.push_back(mesh->getNearestNeighbours(*extremePoint2, 1, std::numeric_limits<double>::max())[0]);
    measure = ((*extremePoint2) - (*extremePoint1)).norm();
}

void MeasureStyle::manageBoundingMovement()
{
    std::vector<std::shared_ptr<SemantisedTriangleMesh::Point> > points;
    bool selected = false;
    if(boundingOrigin != nullptr)
        boundingOrigin.reset();
    boundingOrigin = std::make_shared<Point>(0, 0, 0);
    for(unsigned int i = 0; i < mesh->getAnnotations().size(); i++)
        if(dynamic_pointer_cast<DrawableAnnotation>(mesh->getAnnotations()[i])->getSelected())
        {
            selected = true;
            auto involved = mesh->getAnnotations()[i]->getInvolvedVertices();
            points.insert(points.end(), involved.begin(), involved.end());
        }
    for(unsigned int i = 0; i < points.size(); i++)
        *boundingOrigin += *points[i];
    *boundingOrigin /= points.size();
    if(!selected)
        return;
    Point boundingDirection((*boundingEnd) - (*boundingBegin));
    if(boundingDirection.norm() == 0)
        return;
    boundingDirection.normalise();

    auto extrema = Point::findExtremePoints(points, boundingDirection);
    measurePath.clear();
    measurePath.push_back(static_pointer_cast<Vertex>(extrema.first));
    measurePath.push_back(static_pointer_cast<Vertex>(extrema.second));

    auto boundAtt = dynamic_pointer_cast<DrawableBoundingMeasure>(onCreationAttribute);
    boundAtt->clearMeasurePointsID();
    boundAtt->addMeasurePointID(std::stoi(measurePath[0]->getId()));
    boundAtt->addMeasurePointID(std::stoi(measurePath[1]->getId()));
    boundAtt->setOrigin(boundingOrigin);
    boundAtt->setDirection(boundingDirection);
    boundAtt->update();
}

void MeasureStyle::reset()
{
    measure = 0.0;
    measurePath.clear();
    this->last = nullptr;
    if(measureType == MeasureType::BOUNDING)
    {
        if(boundingOrigin != nullptr)
            boundingOrigin.reset();
        if(boundingBegin != nullptr)
            boundingBegin.reset();
        if(boundingEnd != nullptr)
            boundingEnd.reset();
    }
    if(onCreationAttribute != nullptr)
        onCreationAttribute.reset();

    switch (measureType) {
        case MeasureType::RULER:
        {
            onCreationAttribute = std::make_shared<DrawableEuclideanMeasure>();
            break;
        }
        case MeasureType::TAPE:
        {
            onCreationAttribute = std::make_shared<DrawableGeodesicMeasure>();
            break;
        }
        case MeasureType::CALIBER:
        {
            //a = new DrawableCaliberMeasure();
            break;
        }
        case MeasureType::BOUNDING:
        {
            onCreationAttribute = std::make_shared<DrawableBoundingMeasure>();
            dynamic_pointer_cast<DrawableBoundingMeasure>(onCreationAttribute)->setDrawPlanes(true);
            break;
        }
        case MeasureType::HEIGHT:
        {
            //onCreationAttribute = new DrawableHeightMeasure();
            break;
        }
    }

    onCreationAttribute->setValue(new double(0.0));
    onCreationAttribute->setDrawAttribute(true);
    onCreationAttribute->setDrawValue(true);
    onCreationAttribute->setMesh(mesh);

    boundingOrigin = nullptr;
    boundingBegin = nullptr;
    boundingEnd = nullptr;
    measureStarted = false;
    leftPressed = false;
    middlePressed = false;
    meshRenderer->RemoveActor(measureAssembly);
    cellPicker = vtkSmartPointer<vtkCellPicker>::NewInstance(cellPicker);
    cellPicker->SetPickFromList(1);
    cellPicker->AddPickList(mesh->getSurfaceActor());
    emit(updateView());

}

std::shared_ptr<DrawableAttribute> MeasureStyle::finalizeAttribute(unsigned int id, string key)
{
    onCreationAttribute->setId(id);
    onCreationAttribute->setKey(key);
    auto returnAttribute = onCreationAttribute;
    returnAttribute->setDrawValue(true);
    if(measureType == MeasureType::BOUNDING)
        dynamic_pointer_cast<DrawableBoundingMeasure>(returnAttribute)->setDrawPlanes(false);
    onCreationAttribute = nullptr;
    reset();
    return returnAttribute;
}

void MeasureStyle::OnMouseMove()
{

    if(this->Interactor->GetControlKey() && measureStarted && (measureType == MeasureType::CALIBER || measureType == MeasureType::BOUNDING))
    {
        int x, y;
        x = this->Interactor->GetEventPosition()[0];
        y = this->Interactor->GetEventPosition()[1];
        this->FindPokedRenderer(x, y);
        vtkSmartPointer<vtkCoordinate> coord = vtkSmartPointer<vtkCoordinate>::New();
        coord->SetCoordinateSystemToDisplay();
        coord->SetValue(x,y,0);
        double* worldCoord = coord->GetComputedWorldValue(meshRenderer);
        if(boundingEnd != nullptr)
            boundingEnd.reset();
        boundingEnd = std::make_shared<Point>(worldCoord[0], worldCoord[1], worldCoord[2]);
        switch (measureType) {
            case MeasureType::CALIBER:
            {
                manageCaliberMovement();
                break;
            }
            case MeasureType::BOUNDING:
            {
                manageBoundingMovement();
                break;
            }
            default:
                exit(4343); //IMPOSSIBLE
        }
        onCreationAttribute->update();
        if(drawAttributes)
            onCreationAttribute->draw(measureAssembly);
    }
    if(leftPressed || middlePressed)
        emit(updateView());
    vtkInteractorStyleTrackballCamera::OnMouseMove();

}

void MeasureStyle::OnLeftButtonDown()
{
    leftPressed = true;
    if(this->Interactor->GetControlKey()){

        this->cellPicker->AddPickList(mesh->getSurfaceActor());
        this->cellPicker->PickFromListOn();
        //The click position of the mouse is taken
        int x, y;
        x = this->Interactor->GetEventPosition()[0];
        y = this->Interactor->GetEventPosition()[1];
        this->FindPokedRenderer(x, y);
        //Some tolerance is set for the picking
        vtkSmartPointer<vtkWorldPointPicker> picker = vtkSmartPointer<vtkWorldPointPicker>::New();
        picker->AddPickList(mesh->getSurfaceActor());
        picker->PickFromListOn();
        double pickPos[3];
        int picked = picker->Pick(x, y, 0, this->GetCurrentRenderer());
        picker->GetPickPosition(pickPos);
        SemantisedTriangleMesh::Point pickedPos(pickPos[0], pickPos[1], pickPos[2]);

        vtkIdType cellID = this->cellPicker->GetCellId();

        if(measureType == MeasureType::BOUNDING || measureType == MeasureType::CALIBER)
        {
            vtkSmartPointer<vtkCoordinate> coord = vtkSmartPointer<vtkCoordinate>::New();
            coord->SetCoordinateSystemToDisplay();
            coord->SetValue(x, y, 0);
            double* worldCoord = coord->GetComputedWorldValue(meshRenderer);
            if(!measureStarted)
            {
                if(boundingBegin != nullptr)
                    boundingBegin.reset();
                boundingBegin = std::make_shared<Point>(worldCoord[0], worldCoord[1], worldCoord[2]);
                measureStarted = true;
            }
        } else if(picked >= 0)
        {
            auto v = mesh->getClosestPoint(pickedPos);

            for(unsigned int i = 0; i < mesh->getAnnotations().size(); i++){
                if(mesh->getAnnotations()[i]->isPointInAnnotation(v) && dynamic_pointer_cast<DrawableAnnotation>(mesh->getAnnotations()[i])->getSelected()){
                    if(measureType == MeasureType::HEIGHT)
                    {
                        manageHeightMovement(v);
                        last = v;
                    } else if(!measureStarted)
                    {
                        measurePath.clear();
                        measure = 0.0;
                        switch(measureType){
                            case MeasureType::RULER:
                            {
                                measurePath.push_back(v);
                                auto eucAtt = dynamic_pointer_cast<DrawableEuclideanMeasure>(onCreationAttribute);
                                eucAtt->addMeasurePointID(std::stoi(v->getId()));
                                eucAtt->addMeasurePointID(std::stoi(v->getId()));
                                break;
                            }
                            case MeasureType::TAPE:
                            {
                                auto geoAtt = dynamic_pointer_cast<DrawableGeodesicMeasure>(onCreationAttribute);
                                geoAtt->addMeasurePointID(std::stoi(v->getId()));
                                break;
                            }
                        }

                        measureStarted = true;
                        last = v;
                    } else
                    {
                        auto start = last;
                        auto end = v;
                        switch(measureType){
                            case MeasureType::RULER:
                                manageRulerMovement(start, end);
                                break;
                            case MeasureType::TAPE:
                                manageTapeMovement(start, end);
                                last = end;
                                break;
                        }
                    }

                    onCreationAttribute->update();
                    draw();
                }
            }


        }
    }else
        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();

}

void MeasureStyle::OnLeftButtonUp()
{
    leftPressed = false;
    if(measureStarted && measureType == MeasureType::BOUNDING)
        measureStarted = false;

    onCreationAttribute->update();
    draw();
    vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
}

void MeasureStyle::OnRightButtonDown()
{
    if(this->Interactor->GetControlKey() && measureStarted && measureType != MeasureType::BOUNDING)
    {
        if(measurePath.size() > 0)
        {
            dynamic_pointer_cast<GeometricAttribute>(onCreationAttribute)->removeMeasurePointID(std::stoi(measurePath.back()->getId()));
            measurePath.pop_back();
            if(measurePath.size() > 0)
                last = measurePath.back();
            else
                measureStarted = false;
        } else
            measureStarted = false;
        onCreationAttribute->update();
        draw();
    }

    vtkInteractorStyleTrackballCamera::OnRightButtonDown();

}

void MeasureStyle::OnMiddleButtonUp()
{
    middlePressed = false;
    vtkInteractorStyleTrackballCamera::OnMiddleButtonUp();
}

void MeasureStyle::OnMiddleButtonDown()
{
    middlePressed = true;
    if(measureStarted && this->Interactor->GetControlKey())
        reset();

    vtkInteractorStyleTrackballCamera::OnMiddleButtonDown();
}

void MeasureStyle::OnMouseWheelBackward()
{
    emit(updateView());
    vtkInteractorStyleTrackballCamera::OnMouseWheelBackward();
}

void MeasureStyle::OnMouseWheelForward()
{
    emit(updateView());
    vtkInteractorStyleTrackballCamera::OnMouseWheelForward();
}

QVTKOpenGLNativeWidget *MeasureStyle::getQvtkwidget() const
{
    return qvtkwidget;
}

void MeasureStyle::setQvtkwidget(QVTKOpenGLNativeWidget *value)
{
    qvtkwidget = value;
}

vtkSmartPointer<vtkRenderer> MeasureStyle::getMeshRenderer() const
{
    return meshRenderer;
}

void MeasureStyle::setMeshRenderer(const vtkSmartPointer<vtkRenderer> &value)
{
    meshRenderer = value;
    meshRenderer->AddActor(measureAssembly);
}

MeasureStyle::MeasureType MeasureStyle::getMeasureType() const
{
    return measureType;
}

void MeasureStyle::setMeasureType(const MeasureType &value)
{
    measureType = value;
    reset();
}

double MeasureStyle::getMeasure() const
{
    return measure;
}

std::vector<std::shared_ptr<SemantisedTriangleMesh::Vertex> > MeasureStyle::getMeasurePath() const
{
    return measurePath;
}

std::shared_ptr<DrawableAttribute> MeasureStyle::getOnCreationAttribute() const
{
    return onCreationAttribute;
}

bool MeasureStyle::getDrawAttributes() const
{
    return drawAttributes;
}

void MeasureStyle::setDrawAttributes(bool value)
{
    drawAttributes = value;
}

vtkSmartPointer<vtkPropAssembly> MeasureStyle::getMeasureAssembly() const
{
    return measureAssembly;
}

void MeasureStyle::setMeasureAssembly(vtkSmartPointer<vtkPropAssembly> newMeasureAssembly)
{
    measureAssembly = newMeasureAssembly;
}

void MeasureStyle::draw()
{

    if(drawAttributes)
        onCreationAttribute->draw(measureAssembly);
    measureAssembly->Modified();
    meshRenderer->Render();
    meshRenderer->GetRenderWindow()->Render();
    this->qvtkwidget->update();
}
