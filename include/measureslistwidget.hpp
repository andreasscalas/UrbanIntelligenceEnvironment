#ifndef MEASURESLISTWIDGET_H
#define MEASURESLISTWIDGET_H

#include <drawabletrianglemesh.hpp>

#include <QPushButton>
#include <QTreeWidget>

namespace Ui {
class MeasuresListWidget;
}

class MeasuresListWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit MeasuresListWidget(QWidget *parent = nullptr);
    ~MeasuresListWidget();

    void update();
    std::shared_ptr<DrawableTriangleMesh> getMesh() const;
    void setMesh(std::shared_ptr<DrawableTriangleMesh> value);

signals:
    void updateSignal();
    void updateViewSignal();
    void selectAnnotation(std::string id, bool selected);

private slots:
    void updateSlot();
    void updateViewSlot();
    void slotShowAnnotation(bool);
    void slotDeleteAnnotation();
    void slotSelectAnnotation(bool selected);
private:
    Ui::MeasuresListWidget *ui;
    std::shared_ptr<DrawableTriangleMesh>  mesh;
    std::map<QPushButton*, std::shared_ptr<SemantisedTriangleMesh::Annotation> >  buttonAnnotationMap;
};

#endif // MEASURESLISTWIDGET_H
